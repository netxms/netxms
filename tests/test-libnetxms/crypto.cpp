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
