#include <nms_common.h>
#include <nms_util.h>
#include <nxcrypto.h>
#include <testtools.h>

static const char TEXT[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

/**
 * MD4 test vectors
 */
struct
{
   const char *string;
   const char *hash;
} s_md4TestVectors[] =
{
   { "", "31d6cfe0d16ae931b73c59d7e0c089c0" },
   { "a", "bde52cb31de33e46245e05fbdbd6fb24" },
   { "abc", "a448017aaf21d8525fc10ae87aa6729d" },
   { "message digest", "d9130a8164549fe818874806e1c7014b" },
   { "abcdefghijklmnopqrstuvwxyz", "d79e1c308aa5bbcdeea8ed63df412da9" },
   { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "043f8582f241db351ce627e153e7f0e4" },
   { "12345678901234567890123456789012345678901234567890123456789012345678901234567890", "e33b4ddc9c38f2199c3e7b164fcc0536" },
   { nullptr, nullptr }
};

/**
 * Test MD4
 */
void TestMD4()
{
   StartTest(_T("MD4 hash function"));

   for(int i = 0; s_md4TestVectors[i].string != nullptr; i++)
   {
      BYTE expectedHash[MD4_DIGEST_SIZE];
      StrToBinA(s_md4TestVectors[i].hash, expectedHash, MD4_DIGEST_SIZE);

      MD4_STATE state;
      MD4Init(&state);
      MD4Update(&state, s_md4TestVectors[i].string, strlen(s_md4TestVectors[i].string));

      BYTE actualHash[MD4_DIGEST_SIZE];
      MD4Final(&state, actualHash);

      AssertTrue(!memcmp(expectedHash, actualHash, MD4_DIGEST_SIZE));
   }

   EndTest();
}

/**
 * DES test vectors (FIPS known answers)
 */
struct
{
   const char *key;
   const char *plainText;
   const char *cipherText;
} s_desTestVectors[] =
{
   { "133457799BBCDFF1", "0123456789ABCDEF", "85E813540F0AB405" },
   { "0123456789ABCDEF", "4E6F772069732074", "3FA40E8A984D4815" },
   { "0000000000000000", "0000000000000000", "8CA64DE9C1B123A7" },
   { "FFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFF", "7359B2163E4EDC58" },
   { nullptr, nullptr, nullptr }
};

/**
 * Test DES
 */
void TestDES()
{
   StartTest(_T("DES cipher"));

   for(int i = 0; s_desTestVectors[i].key != nullptr; i++)
   {
      BYTE key[8], plainText[8], cipherText[8];
      StrToBinA(s_desTestVectors[i].key, key, 8);
      StrToBinA(s_desTestVectors[i].plainText, plainText, 8);
      StrToBinA(s_desTestVectors[i].cipherText, cipherText, 8);

      DES_KEY_SCHEDULE ks;
      DESExpandKey(key, &ks);

      BYTE block[8];
      DESEncryptBlock(&ks, plainText, block);
      AssertTrue(!memcmp(block, cipherText, 8));

      DESDecryptBlock(&ks, cipherText, block);
      AssertTrue(!memcmp(block, plainText, 8));
   }

   // Key parity bits must be ignored
   BYTE key[8], alteredKey[8];
   StrToBinA("133457799BBCDFF1", key, 8);
   for(int i = 0; i < 8; i++)
      alteredKey[i] = key[i] ^ 0x01;

   DES_KEY_SCHEDULE ks, alteredKs;
   DESExpandKey(key, &ks);
   DESExpandKey(alteredKey, &alteredKs);
   AssertTrue(!memcmp(&ks, &alteredKs, sizeof(DES_KEY_SCHEDULE)));

   EndTest();
}

/**
 * Test DES in CBC mode
 */
void TestDESCBC()
{
   StartTest(_T("DES cipher (CBC mode)"));

   BYTE key[8], iv[8];
   StrToBinA("0123456789ABCDEF", key, 8);
   StrToBinA("1234567890ABCDEF", iv, 8);

   DES_KEY_SCHEDULE ks;
   DESExpandKey(key, &ks);

   // Input length is not a multiple of 8, so last block is padded with zeroes.
   // SNMPv3 privacy relies on that, so encrypted data is compared with known value.
   static const char text[] = "NetXMS DES self test vector!";
   size_t textLength = strlen(text);
   AssertTrue(textLength % 8 != 0);

   BYTE expected[32];
   StrToBinA("A323893FF38F22AFEE7F1EF7E28816A7E9D7669846D56EA94BED6394FE15403A", expected, 32);

   BYTE encrypted[32];
   size_t encryptedSize = DESEncryptDataCBC(&ks, iv, reinterpret_cast<const BYTE*>(text), textLength, encrypted);
   AssertEquals(encryptedSize, static_cast<size_t>(32));
   AssertTrue(!memcmp(encrypted, expected, 32));

   BYTE decrypted[32];
   DESDecryptDataCBC(&ks, iv, encrypted, encryptedSize, decrypted);
   AssertTrue(!memcmp(decrypted, text, textLength));

   // Decryption in place
   DESDecryptDataCBC(&ks, iv, encrypted, encryptedSize, encrypted);
   AssertTrue(!memcmp(encrypted, text, textLength));

   EndTest();
}

/**
 * Test RSA encryption
 */
void TestRSA()
{
   StartTest(_T("RSA encryption"));

   RSA_KEY k = RSAGenerateKey(NETXMS_RSA_KEYLEN);
   AssertNotNull(k);

   BYTE encrypted[4096];
   ssize_t esize = RSAPublicEncrypt(TEXT, strlen(TEXT) + 1, encrypted, sizeof(encrypted), k, RSA_PKCS1_OAEP_PADDING);
   AssertTrue(esize > 0);

   char clearText[4096];
   ssize_t csize = RSAPrivateDecrypt(encrypted, esize, clearText, sizeof(clearText), k, RSA_PKCS1_OAEP_PADDING);
   AssertTrue(csize == strlen(TEXT) + 1);
   AssertTrue(!strcmp(TEXT, clearText));

   size_t esize2 = 0;
   BYTE *encrypted2 = RSAPublicEncrypt(TEXT, strlen(TEXT) + 1, &esize2, k, RSA_PKCS1_OAEP_PADDING);
   AssertNotNull(encrypted2);
   AssertTrue(esize2 > 0);

   csize = RSAPrivateDecrypt(encrypted2, esize2, clearText, sizeof(clearText), k, RSA_PKCS1_OAEP_PADDING);
   AssertTrue(csize == strlen(TEXT) + 1);
   AssertTrue(!strcmp(TEXT, clearText));

   MemFree(encrypted2);
   RSAFree(k);
   EndTest();
}
