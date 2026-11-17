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
 * MD5 test vectors (RFC 1321)
 */
struct
{
   const char *string;
   const char *hash;
} s_md5TestVectors[] =
{
   { "", "d41d8cd98f00b204e9800998ecf8427e" },
   { "a", "0cc175b9c0f1b6a831c399e269772661" },
   { "abc", "900150983cd24fb0d6963f7d28e17f72" },
   { "message digest", "f96b697d7cb7938d525a2f31aaf161d0" },
   { "abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b" },
   { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f" },
   { "12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a" },
   { nullptr, nullptr }
};

/**
 * Test MD5
 */
void TestMD5()
{
   StartTest(_T("MD5 hash function"));

   for(int i = 0; s_md5TestVectors[i].string != nullptr; i++)
   {
      BYTE expectedHash[MD5_DIGEST_SIZE];
      StrToBinA(s_md5TestVectors[i].hash, expectedHash, MD5_DIGEST_SIZE);

      BYTE actualHash[MD5_DIGEST_SIZE];
      CalculateMD5Hash(s_md5TestVectors[i].string, strlen(s_md5TestVectors[i].string), actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, MD5_DIGEST_SIZE));
   }

   // Incremental updates with every split point and unaligned input must match one-shot hash
   BYTE data[1024];
   for(size_t i = 0; i < sizeof(data); i++)
      data[i] = static_cast<BYTE>(i * 7 + (i >> 8));
   BYTE expectedHash[MD5_DIGEST_SIZE];
   CalculateMD5Hash(data + 3, 300, expectedHash);
   for(size_t split = 0; split <= 300; split++)
   {
      MD5_STATE state;
      MD5Init(&state);
      MD5Update(&state, data + 3, split);
      MD5Update(&state, data + 3 + split, 300 - split);
      BYTE actualHash[MD5_DIGEST_SIZE];
      MD5Final(&state, actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, MD5_DIGEST_SIZE));
   }

   EndTest();

   StartTest(_T("MD5 hash for repeated pattern"));
   static const size_t patternSizes[] = { 1, 3, 8, 10, 63, 64, 65, 200 };
   for(size_t patternSize : patternSizes)
   {
      BYTE expanded[4096];
      for(size_t i = 0; i < sizeof(expanded); i++)
         expanded[i] = data[i % patternSize];
      CalculateMD5Hash(expanded, sizeof(expanded), expectedHash);
      BYTE actualHash[MD5_DIGEST_SIZE];
      MD5HashForPattern(data, patternSize, sizeof(expanded), actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, MD5_DIGEST_SIZE));
   }
   EndTest();
}

/**
 * SHA-1 test vectors (FIPS 180-1)
 */
struct
{
   const char *string;
   const char *hash;
} s_sha1TestVectors[] =
{
   { "", "da39a3ee5e6b4b0d3255bfef95601890afd80709" },
   { "abc", "a9993e364706816aba3e25717850c26c9cd0d89d" },
   { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "84983e441c3bd26ebaae4aa1f95129e5e54670f1" },
   { nullptr, nullptr }
};

/**
 * Test SHA-1
 */
void TestSHA1()
{
   StartTest(_T("SHA-1 hash function"));

   for(int i = 0; s_sha1TestVectors[i].string != nullptr; i++)
   {
      BYTE expectedHash[SHA1_DIGEST_SIZE];
      StrToBinA(s_sha1TestVectors[i].hash, expectedHash, SHA1_DIGEST_SIZE);

      BYTE actualHash[SHA1_DIGEST_SIZE];
      CalculateSHA1Hash(s_sha1TestVectors[i].string, strlen(s_sha1TestVectors[i].string), actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, SHA1_DIGEST_SIZE));
   }

   // One million repetitions of "a" (FIPS 180-1), hashed in uneven chunks
   BYTE expectedHash[SHA1_DIGEST_SIZE];
   StrToBinA("34aa973cd4c4daa4f61eeb2bdbad27316534016f", expectedHash, SHA1_DIGEST_SIZE);
   char chunk[1000];
   memset(chunk, 'a', sizeof(chunk));
   SHA1_STATE state;
   SHA1Init(&state);
   for(size_t total = 0; total < 1000000; )
   {
      size_t size = std::min(static_cast<size_t>(997), 1000000 - total);
      SHA1Update(&state, chunk, size);
      total += size;
   }
   BYTE actualHash[SHA1_DIGEST_SIZE];
   SHA1Final(&state, actualHash);
   AssertTrue(!memcmp(expectedHash, actualHash, SHA1_DIGEST_SIZE));

   // Incremental updates with every split point and unaligned input must match one-shot hash
   BYTE data[1024];
   for(size_t i = 0; i < sizeof(data); i++)
      data[i] = static_cast<BYTE>(i * 7 + (i >> 8));
   CalculateSHA1Hash(data + 3, 300, expectedHash);
   for(size_t split = 0; split <= 300; split++)
   {
      SHA1Init(&state);
      SHA1Update(&state, data + 3, split);
      SHA1Update(&state, data + 3 + split, 300 - split);
      SHA1Final(&state, actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, SHA1_DIGEST_SIZE));
   }

   EndTest();

   StartTest(_T("SHA-1 hash for repeated pattern"));
   static const size_t patternSizes[] = { 1, 3, 8, 10, 63, 64, 65, 200 };
   for(size_t patternSize : patternSizes)
   {
      BYTE expanded[4096];
      for(size_t i = 0; i < sizeof(expanded); i++)
         expanded[i] = data[i % patternSize];
      CalculateSHA1Hash(expanded, sizeof(expanded), expectedHash);
      SHA1HashForPattern(data, patternSize, sizeof(expanded), actualHash);
      AssertTrue(!memcmp(expectedHash, actualHash, SHA1_DIGEST_SIZE));
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

/**
 * Test password decryption (nxencpasswd format)
 */
void TestDecryptPassword()
{
   StartTest(_T("Password decryption"));

   TCHAR decrypted[256];

   // 32 byte block, generated by "nxencpasswd netxms secret"
   AssertTrue(DecryptPassword(_T("netxms"), _T("hP3fIE7/w/rCpgp+8SvHuMKmCn7xK8e4wqYKfvErx7g="), decrypted, 256));
   AssertTrue(!_tcscmp(decrypted, _T("secret")));

   // 64 byte block
   AssertTrue(DecryptPassword(_T("netxms"), _T("Wo8TJj5Z12amriqO6aOZ5f6njYjZGE5dAfwPfMmTfT7v1G42IYyFw19lfVRp66lgwqYKfvErx7jCpgp+8SvHuA=="), decrypted, 256));
   AssertTrue(!_tcscmp(decrypted, _T("a long password that needs the 64 byte block")));

   // 44 character value with "==" padding passes symbol check but decodes to 31 bytes: must fail and pass value through
   AssertFalse(DecryptPassword(_T("netxms"), _T("hP3fIE7/w/rCpgp+8SvHuMKmCn7xK8e4wqYKfvErx7=="), decrypted, 256));
   AssertTrue(!_tcscmp(decrypted, _T("hP3fIE7/w/rCpgp+8SvHuMKmCn7xK8e4wqYKfvErx7==")));

   // Plain text password is passed through unchanged
   AssertFalse(DecryptPassword(_T("netxms"), _T("plaintext"), decrypted, 256));
   AssertTrue(!_tcscmp(decrypted, _T("plaintext")));

   // In-place decryption (same buffer for input and output)
   _tcscpy(decrypted, _T("hP3fIE7/w/rCpgp+8SvHuMKmCn7xK8e4wqYKfvErx7g="));
   AssertTrue(DecryptPassword(_T("netxms"), decrypted, decrypted, 256));
   AssertTrue(!_tcscmp(decrypted, _T("secret")));

   EndTest();
}
