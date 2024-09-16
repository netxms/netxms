/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxcrypto.h
**
**/

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

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#endif

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
String LIBNETXMS_EXPORTABLE GetCertificateIssuerString(const X509 *cert);
time_t LIBNETXMS_EXPORTABLE GetCertificateExpirationTime(const X509 *cert);
time_t LIBNETXMS_EXPORTABLE GetCertificateIssueTime(const X509 *cert);
String LIBNETXMS_EXPORTABLE GetCertificateTemplateId(const X509 *cert);
String LIBNETXMS_EXPORTABLE GetCertificateCRLDistributionPoint(const X509 *cert);
X509_STORE LIBNETXMS_EXPORTABLE *CreateTrustedCertificatesStore(const StringSet& trustedCertificates, bool useSystemStore);

bool LIBNETXMS_EXPORTABLE InitCryptoLib(uint32_t enabledCiphers);

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
typedef EVP_PKEY* RSA_KEY;
static inline EVP_PKEY *EVP_PKEY_from_RSA_KEY(RSA_KEY k)
{
   return k;
}
#else
typedef RSA* RSA_KEY;
static inline EVP_PKEY *EVP_PKEY_from_RSA_KEY(RSA_KEY k)
{
   EVP_PKEY *ek = EVP_PKEY_new();
   if (ek == nullptr)
      return nullptr;
   EVP_PKEY_assign_RSA(ek, k);
   return ek;
}
#endif

RSA_KEY LIBNETXMS_EXPORTABLE RSALoadKey(const TCHAR *keyFile);
bool LIBNETXMS_EXPORTABLE RSASaveKey(RSA_KEY key, const TCHAR *keyFile);
RSA_KEY LIBNETXMS_EXPORTABLE RSAKeyFromData(const BYTE *data, size_t size, bool withPrivate);
RSA_KEY LIBNETXMS_EXPORTABLE RSAGenerateKey(int bits);
BYTE LIBNETXMS_EXPORTABLE *RSASerializePublicKey(RSA_KEY key, bool useX509Format, size_t *size);

/**
 * Destroy RSA key
 */
static inline void RSAFree(RSA_KEY key)
{
   if (key != nullptr)
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
      EVP_PKEY_free(key);
#else
      RSA_free(key);
#endif
}

/**
 * Encrypt data block with RSA public key
 */
static inline ssize_t RSAPublicEncrypt(const void *in, size_t inlen, BYTE *out, size_t outlen, RSA_KEY rsa, int padding)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(rsa, nullptr);
   if (EVP_PKEY_encrypt_init(ctx) <= 0)
   {
      EVP_PKEY_CTX_free(ctx);
      return -1;
   }
   EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
   size_t bytes = outlen;
   int rc = EVP_PKEY_encrypt(ctx, out, &bytes, static_cast<const BYTE*>(in), inlen);
   EVP_PKEY_CTX_free(ctx);
   return (rc > 0) ? bytes : -1;
#else
   return RSA_public_encrypt(static_cast<int>(inlen), static_cast<const BYTE*>(in), out, rsa, padding);
#endif
}

/**
 * Encrypt data block with RSA public key. Output buffer dynamically allocated and should be freed with MemFree.
 */
static inline BYTE *RSAPublicEncrypt(const void *in, size_t inlen, size_t *outlen, RSA_KEY rsa, int padding)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(rsa, nullptr);
   if (EVP_PKEY_encrypt_init(ctx) <= 0)
   {
      EVP_PKEY_CTX_free(ctx);
      return nullptr;
   }
   EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
   BYTE *out = nullptr;
   size_t bytes = 0;
   if (EVP_PKEY_encrypt(ctx, nullptr, &bytes, static_cast<const BYTE*>(in), inlen) > 0)
   {
      out = MemAllocArrayNoInit<BYTE>(bytes);
      if (EVP_PKEY_encrypt(ctx, out, &bytes, static_cast<const BYTE*>(in), inlen) > 0)
      {
         *outlen = bytes;
      }
      else
      {
         MemFreeAndNull(out);
      }
   }
   EVP_PKEY_CTX_free(ctx);
   return out;
#else
   BYTE *out = MemAllocArrayNoInit<BYTE>(RSA_size(rsa));
   int bytes = RSA_public_encrypt(static_cast<int>(inlen), static_cast<const BYTE*>(in), out, rsa, padding);
   if (bytes > 0)
   {
      *outlen = bytes;
   }
   else
   {
      MemFreeAndNull(out);
   }
   return out;
#endif
}

/**
 * Decrypt data block with RSA private key
 */
static inline ssize_t RSAPrivateDecrypt(const BYTE *in, size_t inlen, void *out, size_t outlen, RSA_KEY rsa, int padding)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(rsa, nullptr);
   EVP_PKEY_decrypt_init(ctx);
   EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
   size_t bytes = outlen;
   int rc = EVP_PKEY_decrypt(ctx, static_cast<BYTE*>(out), &bytes, in, inlen);
   EVP_PKEY_CTX_free(ctx);
   return (rc > 0) ? bytes : -1;
#else
   return RSA_private_decrypt(static_cast<int>(inlen), in, static_cast<BYTE*>(out), rsa, padding);
#endif
}

/**
 * Decrypt data block with RSA public key
 */
static inline ssize_t RSAPublicDecrypt(const BYTE *in, size_t inlen, void *out, size_t outlen, RSA_KEY rsa, int padding)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(rsa, nullptr);
   EVP_PKEY_verify_recover_init(ctx);
   EVP_PKEY_CTX_set_rsa_padding(ctx, padding);
   size_t bytes = outlen;
   int rc = EVP_PKEY_verify_recover(ctx, static_cast<BYTE*>(out), &bytes, in, inlen);
   EVP_PKEY_CTX_free(ctx);
   return (rc > 0) ? bytes : -1;
#else
   return RSA_public_decrypt(static_cast<int>(inlen), in, static_cast<BYTE*>(out), rsa, padding);
#endif
}

/**
 * Get maximum buffer size required for encryption/decryption operations with given buffer
 */
static inline size_t RSASize(RSA_KEY rsa)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   return EVP_PKEY_get_size(rsa);
#else
   return RSA_size(rsa);
#endif
}

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, uint32_t dwMsgLen, const CERT_CONTEXT *pCert, BYTE *pBuffer, size_t bufferSize, uint32_t *pdwSigLen);
#endif   /* _WIN32 */

#endif

#endif
