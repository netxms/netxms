#ifndef _nxcrypto_h_
#define _nxcrypto_h_

#if defined(_WITH_ENCRYPTION) && !defined(ORA_PROC)

#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/opensslv.h>
#include <openssl/err.h>

#ifdef _WIN32
#include <wincrypt.h>
#endif

#ifdef NETXMS_NO_AES
#ifndef OPENSSL_NO_AES
#define OPENSSL_NO_AES
#endif
#endif

#ifdef NETXMS_NO_BF
#ifndef OPENSSL_NO_BF
#define OPENSSL_NO_BF
#endif
#endif

#ifdef NETXMS_NO_IDEA
#ifndef OPENSSL_NO_IDEA
#define OPENSSL_NO_IDEA
#endif
#endif

#ifdef NETXMS_NO_DES
#ifndef OPENSSL_NO_DES
#define OPENSSL_NO_DES
#endif
#endif

/**** Crypto helper functions ****/

struct MD4_STATE
{
   uint32_t block[16];
   BYTE buffer[64];
   uint32_t lo;
   uint32_t hi;
   uint32_t a;
   uint32_t b;
   uint32_t c;
   uint32_t d;
};

void LIBNETXMS_EXPORTABLE MD4Init(MD4_STATE *state);
void LIBNETXMS_EXPORTABLE MD4Update(MD4_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE MD4Final(MD4_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
struct MD_STATE
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_MD_CTX *context;
#else
   EVP_MD_CTX context;
#endif
};

typedef MD_STATE MD5_STATE;
#else
typedef unsigned char MD5_STATE[128];
#endif

void LIBNETXMS_EXPORTABLE MD5Init(MD5_STATE *state);
void LIBNETXMS_EXPORTABLE MD5Update(MD5_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE MD5Final(MD5_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
typedef MD_STATE SHA1_STATE;
#else
typedef unsigned char SHA1_STATE[128];
#endif

void LIBNETXMS_EXPORTABLE SHA1Init(SHA1_STATE *state);
void LIBNETXMS_EXPORTABLE SHA1Update(SHA1_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA1Final(SHA1_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
typedef MD_STATE SHA224_STATE;
#else
typedef unsigned char SHA224_STATE[192];
#endif

void LIBNETXMS_EXPORTABLE SHA224Init(SHA224_STATE *state);
void LIBNETXMS_EXPORTABLE SHA224Update(SHA224_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA224Final(SHA224_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
typedef MD_STATE SHA256_STATE;
#else
typedef unsigned char SHA256_STATE[192];
#endif

void LIBNETXMS_EXPORTABLE SHA256Init(SHA256_STATE *state);
void LIBNETXMS_EXPORTABLE SHA256Update(SHA256_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA256Final(SHA256_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
typedef MD_STATE SHA384_STATE;
#else
typedef unsigned char SHA384_STATE[192];
#endif

void LIBNETXMS_EXPORTABLE SHA384Init(SHA384_STATE *state);
void LIBNETXMS_EXPORTABLE SHA384Update(SHA384_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA384Final(SHA384_STATE *state, BYTE *hash);

#ifdef _WITH_ENCRYPTION
typedef MD_STATE SHA512_STATE;
#else
typedef unsigned char SHA512_STATE[192];
#endif

void LIBNETXMS_EXPORTABLE SHA512Init(SHA512_STATE *state);
void LIBNETXMS_EXPORTABLE SHA512Update(SHA512_STATE *state, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA512Final(SHA512_STATE *state, BYTE *hash);

bool LIBNETXMS_EXPORTABLE GetCertificateSubjectField(const X509 *cert, int nid, TCHAR *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE GetCertificateCN(const X509 *cert, TCHAR *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE GetCertificateOU(const X509 *cert, TCHAR *buffer, size_t size);
String LIBNETXMS_EXPORTABLE GetCertificateSubjectString(const X509 *cert);
bool LIBNETXMS_EXPORTABLE GetCertificateIssuerField(const X509 *cert, int nid, TCHAR *buffer, size_t size);
String LIBNETXMS_EXPORTABLE GetCertificateIssuerString(const X509 *cert);
time_t LIBNETXMS_EXPORTABLE GetCertificateExpirationTime(const X509 *cert);
time_t LIBNETXMS_EXPORTABLE GetCertificateIssueTime(const X509 *cert);
String LIBNETXMS_EXPORTABLE GetCertificateTemplateId(const X509 *cert);
String LIBNETXMS_EXPORTABLE GetCertificateCRLDistributionPoint(const X509 *cert);
X509_STORE LIBNETXMS_EXPORTABLE *CreateTrustedCertificatesStore(const StringSet& trustedCertificates, bool useSystemStore);

RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile);
RSA LIBNETXMS_EXPORTABLE *RSAKeyFromData(const BYTE *data, size_t size, bool withPrivate);
void LIBNETXMS_EXPORTABLE RSAFree(RSA *key);
RSA LIBNETXMS_EXPORTABLE *RSAGenerateKey(int bits);

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, uint32_t dwMsgLen, const CERT_CONTEXT *pCert, BYTE *pBuffer, size_t bufferSize, uint32_t *pdwSigLen);
#endif   /* _WIN32 */

#endif

#endif
