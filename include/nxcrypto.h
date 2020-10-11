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

#if OPENSSL_VERSION_NUMBER >= 0x00907000
#define OPENSSL_CONST const
#else
#define OPENSSL_CONST
#endif

/**** Crypto helper functions ****/

#ifdef __cplusplus

bool LIBNETXMS_EXPORTABLE GetCertificateSubjectField(X509 *cert, int nid, TCHAR *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE GetCertificateCN(X509 *cert, TCHAR *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE GetCertificateOU(X509 *cert, TCHAR *buffer, size_t size);
String LIBNETXMS_EXPORTABLE GetCertificateSubjectString(X509 *cert);
bool LIBNETXMS_EXPORTABLE GetCertificateIssuerField(X509 *cert, int nid, TCHAR *buffer, size_t size);
String LIBNETXMS_EXPORTABLE GetCertificateIssuerString(X509 *cert);
time_t LIBNETXMS_EXPORTABLE GetCertificateExpirationTime(const X509 *cert);
String LIBNETXMS_EXPORTABLE GetCertificateTemplateId(const X509 *cert);
X509_STORE LIBNETXMS_EXPORTABLE *CreateTrustedCertificatesStore(const StringSet& trustedCertificates, bool useSystemStore);

#endif   /* __cplusplus */

#else /* no encryption */

// Prevent compilation errors on function prototypes
#define RSA void

#endif

#endif
