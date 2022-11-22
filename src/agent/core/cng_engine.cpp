/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: cng_engine.cpp
**
**/

#include "nxagentd.h"
#include <nxcrypto.h>
#include <openssl/engine.h>
#include <openssl/ssl.h>

#define DEBUG_TAG _T("crypto.cng")

static bool s_initialized = false;
static int s_idxEngineContext = -1;
static int s_idxCertificateId = -1;
static int s_idxKeyHandle = -1;
static RSA_METHOD *s_rsaMethod = nullptr;

/**
 * Log error
 */
#define LOG_ERROR(m) LogAPIError(__func__, __FILE__, __LINE__, 0, (m))

/**
 * Log API error
 */
#define LOG_API_ERROR(e) LogAPIError(__func__, __FILE__, __LINE__, (e), nullptr)

/**
 * Log last API error
 */
#define LOG_LAST_API_ERROR() LogAPIError(__func__, __FILE__, __LINE__, GetLastError(), nullptr)

/**
 * Log API error inside given function
 */
static void LogAPIError(const char *func, const char *file, int line, DWORD error, const TCHAR *message)
{
   const char *fname = strrchr(file, '\\');
   if (fname != nullptr)
      fname++;
   else
      fname = file;
   TCHAR buffer[1024];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Error in %hs (%hs:%d): %s"), func, fname, line, 
         (message != nullptr) ? message : GetSystemErrorText(error, buffer, 1024));
}

/**
 * RSA private encrypt
 */
static int RSAPrivateEncrypt(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("RSAPrivateEncrypt called for %p (padding=%d, flen=%d)"), rsa, padding, flen);
   return -1;  // not supported
}

/**
 * RSA private decrypt
 */
static int RSAPrivateDecrypt(int flen, const unsigned char *from, unsigned char *to, RSA *rsa, int padding)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("RSAPrivateDecrypt called for %p"), rsa);
   return -1;  // Not supported
}

/**
 * RSA sign
 */
int RSASign(int dtype, const unsigned char *hash, unsigned int hashlen, unsigned char *signature, unsigned int *siglen, const RSA *rsa)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("RSASign called for %p (dtype = %d)"), rsa, dtype);

   BCRYPT_PKCS1_PADDING_INFO padding;
   switch (dtype)
   {
      case NID_sha256:
         padding.pszAlgId = BCRYPT_SHA256_ALGORITHM;
         break;
      case NID_sha384:
         padding.pszAlgId = BCRYPT_SHA384_ALGORITHM;
         break;
      case NID_sha512:
         padding.pszAlgId = BCRYPT_SHA512_ALGORITHM;
         break;
      case NID_sha1:
         padding.pszAlgId = BCRYPT_SHA1_ALGORITHM;
         break;
      case NID_md5:
         padding.pszAlgId = BCRYPT_MD5_ALGORITHM;
         break;
      case NID_md5_sha1:
         padding.pszAlgId = nullptr;
         break;
      default:
         LOG_ERROR(_T("Unsupported digest type"));
         return 0;
   }

   NCRYPT_KEY_HANDLE keyHandle = reinterpret_cast<NCRYPT_KEY_HANDLE>(RSA_get_ex_data(rsa, s_idxKeyHandle));
   DWORD len;
   SECURITY_STATUS status;
   if ((status = NCryptSignHash(keyHandle, &padding, (BYTE*)hash, hashlen, signature, RSA_size(rsa), &len, BCRYPT_PAD_PKCS1 | NCRYPT_SILENT_FLAG)) != ERROR_SUCCESS)
   {
      LOG_API_ERROR(status);
      return 0;
   }

   *siglen = len;
   return 1;
}

/**
 * RSA init
 */
static int RSAInit(RSA *rsa)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("RSAInit called for %p"), rsa);
   return 1;
}

/**
 * RSA finish
 */
static int RSAFinish(RSA *rsa)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("RSAFinish called for %p"), rsa);
   NCRYPT_KEY_HANDLE keyHandle = reinterpret_cast<NCRYPT_KEY_HANDLE>(RSA_get_ex_data(rsa, s_idxKeyHandle));
   NCryptFreeObject(keyHandle);
   RSA_set_ex_data(rsa, s_idxKeyHandle, nullptr);
   return 1;
}

/**
 * Key export data
 */
union KeyExportData
{
   BCRYPT_KEY_BLOB info;
   BCRYPT_DH_KEY_BLOB dh;
   BCRYPT_DSA_KEY_BLOB dsa;
# if _WIN32_WINNT >= 0x0602      /* Win 8 */
   BCRYPT_DSA_KEY_BLOB_V2 dsa2;
# endif
   BCRYPT_ECCKEY_BLOB ecc;
   BCRYPT_RSAKEY_BLOB rsa;
   unsigned char data[1];   /* Used to reach the data after the blobs */
};

/**
 * Get OpenSSL private/public key pair from CNG key handle
 */
static EVP_PKEY *RSAGetPKey(ENGINE *engine, NCRYPT_KEY_HANDLE khandle)
{
   DWORD size;
   SECURITY_STATUS status;
   if ((status = NCryptExportKey(khandle, 0, BCRYPT_PUBLIC_KEY_BLOB, nullptr, nullptr, 0, &size, NCRYPT_SILENT_FLAG)) != ERROR_SUCCESS)
   {
      LOG_API_ERROR(status);
      return nullptr;
   }

   KeyExportData *exported = static_cast<KeyExportData*>(MemAlloc(size));
   if (exported == nullptr)
   {
      LOG_ERROR(_T("Memory allocation failed"));
      return nullptr;
   }

   if ((status = NCryptExportKey(khandle, 0, BCRYPT_PUBLIC_KEY_BLOB, nullptr, (PBYTE)exported, size, &size, NCRYPT_SILENT_FLAG)) != ERROR_SUCCESS)
   {
      LOG_API_ERROR(status);
      MemFree(exported);
      return nullptr;
   }

   if (exported->info.Magic != BCRYPT_RSAPUBLIC_MAGIC)
   {
      LOG_ERROR(_T("Unsupported key type"));
      MemFree(exported);
      return nullptr;
   }

   size_t off_e = sizeof(exported->rsa);
   size_t off_n = off_e + exported->rsa.cbPublicExp;
   BIGNUM *e = nullptr;
   BIGNUM *n = nullptr;
   RSA *rsa = nullptr;
   EVP_PKEY *pkey = nullptr;

   if (((e = BN_bin2bn(&exported->data[off_e], exported->rsa.cbPublicExp, nullptr)) == nullptr) ||
       ((n = BN_bin2bn(&exported->data[off_n], exported->rsa.cbModulus, nullptr)) == nullptr))
   {
      LOG_ERROR(_T("BN_bin2bn failed"));
      goto failure;
   }
   if (((rsa = RSA_new_method(engine)) == nullptr) || !RSA_set0_key(rsa, n, e, nullptr))
   {
      LOG_ERROR(_T("Cannot create RSA structure"));
      goto failure;
   }
   RSA_set_ex_data(rsa, s_idxKeyHandle, (void*)khandle);
   if (((pkey = EVP_PKEY_new()) == nullptr) || !EVP_PKEY_assign_RSA(pkey, rsa))
   {
      LOG_ERROR(_T("Cannot create EVP_PKEY structure"));
      EVP_PKEY_free(pkey);
      pkey = nullptr;
   }

   MemFree(exported);
   return pkey;

failure:
   RSA_free(rsa);
   BN_free(n);
   BN_free(e); 
   MemFree(exported);
   return nullptr;
}

/**
 * Engine context
 */
struct EngineContext
{
   char *storeName;
   bool isSystemStore;

   EngineContext()
   {
      storeName = nullptr;
      isSystemStore = true;
   }
   ~EngineContext()
   {
      MemFree(storeName);
   }
};

/**
 * Init engine
 */
static int InitEngine(ENGINE *e)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("InitEngine called for %p"), e);
   if (!s_initialized)
   {
      s_idxEngineContext = ENGINE_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
      s_idxCertificateId = SSL_CTX_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
      s_idxKeyHandle = RSA_get_ex_new_index(0, NULL, nullptr, nullptr, nullptr);

      s_rsaMethod = RSA_meth_dup(RSA_PKCS1_OpenSSL());
      if (s_rsaMethod != nullptr)
      {
         RSA_meth_set1_name(s_rsaMethod, "CNG RSA");
         RSA_meth_set_flags(s_rsaMethod, 0);
         RSA_meth_set0_app_data(s_rsaMethod, nullptr);
         RSA_meth_set_priv_enc(s_rsaMethod, RSAPrivateEncrypt);
         RSA_meth_set_priv_dec(s_rsaMethod, RSAPrivateDecrypt);
         RSA_meth_set_init(s_rsaMethod, RSAInit);
         RSA_meth_set_finish(s_rsaMethod, RSAFinish);
         RSA_meth_set_sign(s_rsaMethod, RSASign);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Call to RSA_meth_dup() failed"));
      }

      s_initialized = true;
   }
   ENGINE_set_ex_data(e, s_idxEngineContext, new EngineContext());
   ENGINE_set_RSA(e, s_rsaMethod);
   return 1;
}

/**
 * Finish engine
 */
static int FinishEngine(ENGINE *e)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("FinishEngine called for %p"), e);
   auto ctx = static_cast<EngineContext*>(ENGINE_get_ex_data(e, s_idxEngineContext));
   delete ctx;
   ENGINE_set_ex_data(e, s_idxEngineContext, nullptr);
   return 1;
}

/**
 * Destroy engine
 */
static int DestroyEngine(ENGINE *e)
{
   nxlog_debug_tag(DEBUG_TAG, 7, _T("DestroyEngine called for %p"), e);
   return 1;
}

/**
 * Match specific certificate name attribute
 */
static inline bool MatchCertificateNameAttribute(PCCERT_CONTEXT context, const TCHAR *selector, const char *attr, const TCHAR *id)
{
   size_t l = _tcslen(selector);
   if (_tcsncmp(id, selector, l))
      return false;

   TCHAR value[1024];
   CertGetNameString(context, CERT_NAME_ATTR_TYPE, 0, (void*)attr, value, 1024);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: %s \"%s\" with \"%s\""), selector, &id[l], value);
   return _tcsicmp(&id[l], value) == 0;
}

/**
 * Wrapper for MemAlloc() to use in CRYPT_DECODE_PARA
 */
static LPVOID WINAPI MemAllocWrapper(size_t size)
{
   return MemAlloc(size);
}

/**
 * Wrapper for MemFree() to use in CRYPT_DECODE_PARA
 */
static void WINAPI MemFreeWrapper(LPVOID p)
{
   return MemFree(p);
}

/**
 * Match any certificate
 */
bool MatchWindowsStoreCertificate(PCCERT_CONTEXT context, const TCHAR *id)
{
   if (id == nullptr)
      return false;

   if (!_tcsnicmp(id, _T("name:"), 5))
   {
      TCHAR value[1024];
      CertGetNameString(context, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, nullptr, value, 1024);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: name: \"%s\" with \"%s\""), &id[5], value);
      return _tcsicmp(&id[5], value) == 0;
   }

   if (!_tcsnicmp(id, _T("email:"), 6))
   {
      TCHAR value[1024];
      CertGetNameString(context, CERT_NAME_EMAIL_TYPE, 0, nullptr, value, 1024);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: email: \"%s\" with \"%s\""), &id[6], value);
      return _tcsicmp(&id[6], value) == 0;
   }

   if (!_tcsnicmp(id, _T("subject:"), 8))
   {
      TCHAR value[1024];
      DWORD strType = CERT_X500_NAME_STR;
      CertGetNameString(context, CERT_NAME_RDN_TYPE, 0, &strType, value, 1024);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: subject: \"%s\" with \"%s\""), &id[8], value);
      return _tcsicmp(&id[8], value) == 0;
   }

   if (!_tcsnicmp(id, _T("template:"), 9))
   {
      PCERT_EXTENSION e = CertFindExtension(szOID_CERTIFICATE_TEMPLATE, context->pCertInfo->cExtension, context->pCertInfo->rgExtension);
      if (e == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: call to CertFindExtension failed"));
         return false;
      }

      CRYPT_DECODE_PARA dp;
      dp.cbSize = sizeof(dp);
      dp.pfnAlloc = MemAllocWrapper;
      dp.pfnFree = MemFreeWrapper;

      CERT_TEMPLATE_EXT *tmpl = nullptr;
      DWORD size = 0;
      if (!CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_CERTIFICATE_TEMPLATE, e->Value.pbData, e->Value.cbData,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, &dp, &tmpl, &size))
      {
         TCHAR buffer[1024];
         nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: call to CryptDecodeObjectEx failed (%s)"),
            GetSystemErrorText(GetLastError(), buffer, 1024));
         return false;
      }

      nxlog_debug_tag(DEBUG_TAG, 8, _T("MatchWindowsStoreCertificate: subject: \"%s\" with \"%hs\""), &id[9], tmpl->pszObjId);
      char *mbid = MBStringFromWideString(&id[9]);
      bool success = (strcmp(mbid, tmpl->pszObjId) == 0);

      MemFree(mbid);
      MemFree(tmpl);
      return success;
   }

   return
      MatchCertificateNameAttribute(context, _T("cn:"), szOID_COMMON_NAME, id) ||
      MatchCertificateNameAttribute(context, _T("org:"), szOID_ORGANIZATION_NAME, id) ||
      MatchCertificateNameAttribute(context, _T("deviceSerial:"), szOID_DEVICE_SERIAL_NUMBER, id);
}

/**
 * Load SSL client certificate
 */
static int LoadSSLClientCert(ENGINE *e, SSL *ssl, STACK_OF(X509_NAME) *caChain, X509 **pcert,
      EVP_PKEY **pkey, STACK_OF(X509) **pother, UI_METHOD *ui_method, void *callbackData)
{
   auto ctx = static_cast<EngineContext*>(ENGINE_get_ex_data(e, s_idxEngineContext));

   HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, 0, 
         CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG |
            (ctx->isSystemStore ? CERT_SYSTEM_STORE_LOCAL_MACHINE : CERT_SYSTEM_STORE_CURRENT_USER),
         (ctx->storeName != nullptr) ? ctx->storeName : "MY");
   if (hStore == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: cannot open store \"%hs\" for %s"),
         (ctx->storeName != nullptr) ? ctx->storeName : "MY", ctx->isSystemStore ? _T("local system") : _T("current user"));
      return 0;
   }

   const TCHAR *certificateId = static_cast<const TCHAR*>(SSL_CTX_get_ex_data(SSL_get_SSL_CTX(ssl), s_idxCertificateId));

   HCRYPTPROV_OR_NCRYPT_KEY_HANDLE keyHandle = 0;
   PCCERT_CONTEXT context = nullptr;
   while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
   {
      if (MatchWindowsStoreCertificate(context, certificateId))
      {
         TCHAR certName[1024];
         CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, certName, 1024);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: certificate %s matched"), certName);

         DWORD spec;
         BOOL owned;
         BOOL success = CryptAcquireCertificatePrivateKey(context, CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG, nullptr, &keyHandle, &spec, &owned);
         if (success && (spec == CERT_NCRYPT_KEY_SPEC) && owned)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: certificate %s selected"), certName);
            break;
         }

         nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: cannot acquire private key (%s)"), GetSystemErrorText(GetLastError(), certName, 1024));
         if (success && owned && (spec != CERT_NCRYPT_KEY_SPEC))
            CryptReleaseContext(keyHandle, 0);
      }
   }
   CertCloseStore(hStore, 0);

   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: no matching certificates in store \"%hs\" for %s"),
         (ctx->storeName != nullptr) ? ctx->storeName : "MY", ctx->isSystemStore ? _T("local system") : _T("current user"));
      return 0;
   }

   const unsigned char *in = context->pbCertEncoded;
   X509 *cert = d2i_X509(nullptr, &in, context->cbCertEncoded);
   if (cert == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("LoadSSLClientCert: cannot decode certificate"));
      NCryptFreeObject(keyHandle);
      CertFreeCertificateContext(context);
      return 0;
   }

   *pkey = RSAGetPKey(e, keyHandle);
   if (*pkey == nullptr)
   {
      NCryptFreeObject(keyHandle);
      X509_free(cert);
      return 0;
   }

   *pcert = cert;
   return 1;
}

/**
 * Create crypto engine for CNG API
 */
ENGINE *CreateCNGEngine()
{
   ENGINE *e = ENGINE_new();
   ENGINE_set_id(e, "cng");
   ENGINE_set_name(e, "CNG Engine");
   ENGINE_set_flags(e, ENGINE_FLAGS_NO_REGISTER_ALL);
   ENGINE_set_init_function(e, InitEngine);
   ENGINE_set_finish_function(e, FinishEngine);
   ENGINE_set_destroy_function(e, DestroyEngine);
   ENGINE_set_load_ssl_client_cert_function(e, LoadSSLClientCert);
   return e;
}

/**
 * Set certificate matching ID for CNG engine
 */
void SSLSetCertificateId(SSL_CTX *sslctx, const TCHAR *id)
{
   SSL_CTX_set_ex_data(sslctx, s_idxCertificateId, (void*)id);
}
