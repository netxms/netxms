#ifndef _nxcrypto_h_
#define _nxcrypto_h_

#if defined(_WITH_ENCRYPTION) && !defined(ORA_PROC)

#if WITH_OPENSSL

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

#elif WITH_COMMONCRYPTO

#include <CommonCrypto/CommonCrypto.h>
#include <Security/SecBase.h>
#include <Security/SecKey.h>

struct RSA
{
	SecKeyRef pubkey;
	SecKeyRef privkey;
};

#endif

#else /* no encryption */

// Prevent compilation errors on function prototypes
#define RSA void

#endif

#endif
