/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: wincert.cpp
**
**/

#include "nxagentd.h"
#include <wincrypt.h>
#include <openssl/x509.h>

#define DEBUG_TAG _T("wincert")

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
 * Match certificate in Windows certificate store against a selector id
 * (name:/email:/subject:/template:/cn:/org:/deviceSerial:). This is a pure
 * CRYPT32 helper (no CNG), used by both the tunnel and the WindowsCertificateStore
 * metrics; it lives here rather than in cng_engine.cpp so it is available on the
 * XP build, which excludes the CNG engine.
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
 * Get expiration date for certificate as string
 */
static inline String GetCertificateExpirationDate(X509 *cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   TCHAR buffer[64];
   _tcsftime(buffer, 64, _T("%Y-%m-%d"), localtime(&e));
   return String(buffer);
}

/**
 * Get number of days until certificate expiration
 */
static inline int GetCertificateDaysUntilExpiration(X509 *cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   time_t now = time(nullptr);
   return static_cast<int>((e - now) / 86400);
}

/**
 * Open Windows certificate store by name
 * Store name format: [machine\]StoreName or [user\]StoreName
 * Default scope is local machine, default store is "MY"
 */
static HCERTSTORE OpenCertificateStore(const TCHAR *storeName)
{
   DWORD scope = CERT_SYSTEM_STORE_LOCAL_MACHINE;
   char storeNameA[256] = "MY";

   if (storeName[0] != 0)
   {
      const TCHAR *separator = _tcschr(storeName, _T('\\'));
      if (separator != nullptr)
      {
         if (!_tcsnicmp(storeName, _T("user"), separator - storeName))
            scope = CERT_SYSTEM_STORE_CURRENT_USER;
         // "machine\" prefix keeps default scope

#ifdef UNICODE
         wchar_to_mb(separator + 1, -1, storeNameA, 256);
#else
         strlcpy(storeNameA, separator + 1, 256);
#endif
      }
      else
      {
#ifdef UNICODE
         wchar_to_mb(storeName, -1, storeNameA, 256);
#else
         strlcpy(storeNameA, storeName, 256);
#endif
      }
   }

   HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, 0,
      CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG | scope, storeNameA);
   if (hStore == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("OpenCertificateStore: cannot open store \"%hs\" (%s)"),
         storeNameA, (scope == CERT_SYSTEM_STORE_LOCAL_MACHINE) ? _T("local machine") : _T("current user"));
   }
   return hStore;
}

/**
 * Convert Windows certificate context to OpenSSL X509 structure
 */
static X509 *CertContextToX509(PCCERT_CONTEXT context)
{
   const unsigned char *in = context->pbCertEncoded;
   return d2i_X509(nullptr, &in, context->cbCertEncoded);
}

/**
 * Get SHA-1 thumbprint of a certificate as hex string
 */
static void GetCertificateThumbprint(PCCERT_CONTEXT context, TCHAR *buffer, size_t bufferSize)
{
   BYTE hash[20];
   DWORD hashSize = sizeof(hash);
   if (CertGetCertificateContextProperty(context, CERT_SHA1_HASH_PROP_ID, hash, &hashSize))
   {
      BinToStr(hash, hashSize, buffer);
   }
   else
   {
      buffer[0] = 0;
   }
}

/**
 * Check if certificate has an associated private key
 */
static bool CertificateHasPrivateKey(PCCERT_CONTEXT context)
{
   DWORD keySpec = 0;
   BOOL callerFree = FALSE;
   HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hKey = 0;
   BOOL success = CryptAcquireCertificatePrivateKey(context,
      CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_CACHE_FLAG, nullptr, &hKey, &keySpec, &callerFree);
   if (success && callerFree)
   {
#if (_WIN32_WINNT >= 0x0600)
      // CNG (ncrypt.dll) is Vista+; XP never returns a CERT_NCRYPT_KEY_SPEC key.
      if (keySpec == CERT_NCRYPT_KEY_SPEC)
         NCryptFreeObject(hKey);
      else
#endif
         CryptReleaseContext(hKey, 0);
   }
   return success ? true : false;
}

/**
 * Handler for WindowsCertificateStore.Certificate.* parameters
 * First argument: store name (default "MY")
 * Second argument: certificate selector (name:, subject:, template:, etc.)
 */
LONG H_WindowsCertStoreInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR storeName[256], certSelector[1024];
   AgentGetParameterArg(param, 1, storeName, 256);
   AgentGetParameterArg(param, 2, certSelector, 1024);

   if (certSelector[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   HCERTSTORE hStore = OpenCertificateStore(storeName);
   if (hStore == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   PCCERT_CONTEXT context = nullptr;
   while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
   {
      if (MatchWindowsStoreCertificate(context, certSelector))
      {
         if (*arg == 'K') // HasPrivateKey - doesn't need OpenSSL conversion
         {
            ret_int(value, CertificateHasPrivateKey(context) ? 1 : 0);
            rc = SYSINFO_RC_SUCCESS;
            CertFreeCertificateContext(context);
            break;
         }

         if (*arg == 'F') // Thumbprint - doesn't need OpenSSL conversion
         {
            GetCertificateThumbprint(context, value, MAX_RESULT_LENGTH);
            rc = SYSINFO_RC_SUCCESS;
            CertFreeCertificateContext(context);
            break;
         }

         X509 *cert = CertContextToX509(context);
         if (cert == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("H_WindowsCertStoreInfo: cannot convert certificate to OpenSSL format"));
            CertFreeCertificateContext(context);
            rc = SYSINFO_RC_ERROR;
            break;
         }

         switch (*arg)
         {
            case 'D': // Expiration date
               ret_string(value, GetCertificateExpirationDate(cert));
               break;
            case 'E': // Expiration time
               ret_uint64(value, GetCertificateExpirationTime(cert));
               break;
            case 'I': // Issuer
               ret_string(value, GetCertificateIssuerString(cert));
               break;
            case 'S': // Subject
               ret_string(value, GetCertificateSubjectString(cert));
               break;
            case 'T': // Template ID
               ret_string(value, GetCertificateTemplateId(cert));
               break;
            case 'U': // Days until expiration
               ret_int(value, GetCertificateDaysUntilExpiration(cert));
               break;
         }
         X509_free(cert);
         rc = SYSINFO_RC_SUCCESS;
         CertFreeCertificateContext(context);
         break;
      }
   }
   CertCloseStore(hStore, 0);

   return rc;
}

/**
 * Handler for WindowsCertificateStore.Certificates(*) table
 * Argument: store name (default "MY")
 */
LONG H_WindowsCertStoreTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR storeName[256];
   AgentGetParameterArg(param, 1, storeName, 256);

   HCERTSTORE hStore = OpenCertificateStore(storeName);
   if (hStore == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("THUMBPRINT"), DCI_DT_STRING, _T("Thumbprint"), true);
   value->addColumn(_T("SUBJECT"), DCI_DT_STRING, _T("Subject"));
   value->addColumn(_T("ISSUER"), DCI_DT_STRING, _T("Issuer"));
   value->addColumn(_T("TEMPLATE_ID"), DCI_DT_STRING, _T("Template ID"));
   value->addColumn(_T("EXPIRATION_DATE"), DCI_DT_STRING, _T("Expiration Date"));
   value->addColumn(_T("EXPIRATION_TIME"), DCI_DT_UINT64, _T("Expiration Time"));
   value->addColumn(_T("EXPIRES_IN"), DCI_DT_INT, _T("Days Until Expiration"));
   value->addColumn(_T("HAS_PRIVATE_KEY"), DCI_DT_INT, _T("Has Private Key"));

   PCCERT_CONTEXT context = nullptr;
   while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
   {
      X509 *cert = CertContextToX509(context);
      if (cert == nullptr)
         continue;

      value->addRow();

      TCHAR thumbprint[64];
      GetCertificateThumbprint(context, thumbprint, 64);
      value->set(0, thumbprint);
      value->set(1, GetCertificateSubjectString(cert));
      value->set(2, GetCertificateIssuerString(cert));
      value->set(3, GetCertificateTemplateId(cert));
      value->set(4, GetCertificateExpirationDate(cert));
      value->set(5, static_cast<uint64_t>(GetCertificateExpirationTime(cert)));
      value->set(6, GetCertificateDaysUntilExpiration(cert));
      value->set(7, CertificateHasPrivateKey(context) ? 1 : 0);

      X509_free(cert);
   }
   CertCloseStore(hStore, 0);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for WindowsCertificateStore.Certificates(*) list
 * Returns list of certificate thumbprints in the given store
 */
LONG H_WindowsCertStoreList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR storeName[256];
   AgentGetParameterArg(param, 1, storeName, 256);

   HCERTSTORE hStore = OpenCertificateStore(storeName);
   if (hStore == nullptr)
      return SYSINFO_RC_ERROR;

   PCCERT_CONTEXT context = nullptr;
   while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
   {
      TCHAR thumbprint[64];
      GetCertificateThumbprint(context, thumbprint, 64);
      value->add(thumbprint);
   }
   CertCloseStore(hStore, 0);

   return SYSINFO_RC_SUCCESS;
}
