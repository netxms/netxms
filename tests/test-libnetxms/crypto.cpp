#include <nms_common.h>
#include <nms_util.h>
#include <nxcrypto.h>
#include <testtools.h>

static const char TEXT[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

/**
 * Test RSA encryption
 */
void TestRSA()
{
   StartTest(_T("RSA encryption"));

   RSA_KEY k = RSAGenerateKey(NETXMS_RSA_KEYLEN);

   BYTE encrypted[4096];
   ssize_t esize = RSAPublicEncrypt(TEXT, strlen(TEXT) + 1, encrypted, sizeof(encrypted), k, RSA_PKCS1_OAEP_PADDING);
   AssertTrue(esize > 0);

   char clearText[4096];
   ssize_t csize = RSAPrivateDecrypt(encrypted, esize, clearText, sizeof(clearText), k, RSA_PKCS1_OAEP_PADDING);
   AssertTrue(csize == strlen(TEXT) + 1);
   AssertTrue(!strcmp(TEXT, clearText));

   RSAFree(k);
   EndTest();
}
