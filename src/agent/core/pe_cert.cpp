/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: pe_cert.cpp
**
**/

#include "nxagentd.h"
#include <netxms-version.h>
#include <wincrypt.h>
#include <wintrust.h>

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

/**
 * Product name expected in VERSIONINFO resource of agent upgrade package
 */
#define UPGRADE_PACKAGE_PRODUCT_NAME   _T("NetXMS Agent")

/**
 * Default trusted publisher (subject CN of NetXMS release signing certificate)
 */
#define DEFAULT_TRUSTED_PUBLISHER      _T("Raden Solutions, SIA")

/**
 * Get PE certificate information
 */
BOOL GetPeCertificateInfo(LPCWSTR filePath, PE_CERT_INFO *certInfo)
{
    BOOL result = FALSE;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    DWORD dwEncoding, dwContentType, dwFormatType;
    CERT_INFO certSearchParams = {0};
    PCCERT_CONTEXT pCertContext = NULL;

    // Declared before the first "goto cleanup" so the jumps do not bypass their initialization
    DWORD signerInfoSize = 0;
    PCMSG_SIGNER_INFO signerInfo = NULL;
    DWORD hashSize = 0;
    DWORD nameSize = 0;


    // Get message handle and cert store handle
    if (!CryptQueryObject(
        CERT_QUERY_OBJECT_FILE,
        filePath,
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &dwEncoding,
        &dwContentType,
        &dwFormatType,
        &hStore,
        &hMsg,
        NULL)) {
        goto cleanup;
    }

    // Get signer information
    if (!CryptMsgGetParam(
        hMsg,
        CMSG_SIGNER_INFO_PARAM,
        0,
        NULL,
        &signerInfoSize)) {
        goto cleanup;
    }

    signerInfo = (PCMSG_SIGNER_INFO)MemAllocZeroed(signerInfoSize);
    if (!signerInfo) {
        goto cleanup;
    }

    if (!CryptMsgGetParam(
        hMsg,
        CMSG_SIGNER_INFO_PARAM,
        0,
        signerInfo,
        &signerInfoSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Find certificate in store
    certSearchParams.Issuer = signerInfo->Issuer;
    certSearchParams.SerialNumber = signerInfo->SerialNumber;

    pCertContext = CertFindCertificateInStore(
        hStore,
        ENCODING,
        0,
        CERT_FIND_SUBJECT_CERT,
        &certSearchParams,
        NULL);

    if (!pCertContext) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Get certificate fingerprint (hash)
    if (!CertGetCertificateContextProperty(
        pCertContext,
        CERT_SHA1_HASH_PROP_ID,
        NULL,
        &hashSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    certInfo->fingerprint = (BYTE*)MemAllocZeroed(hashSize);
    certInfo->fingerprintSize = hashSize;

    if (!CertGetCertificateContextProperty(
        pCertContext,
        CERT_SHA1_HASH_PROP_ID,
        certInfo->fingerprint,
        &hashSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Get subject and issuer names
    nameSize = CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0);

    if (nameSize > 0) {
        certInfo->subjectName = (LPWSTR)MemAllocZeroed(nameSize * sizeof(WCHAR));
        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            certInfo->subjectName,
            nameSize);
    }

    nameSize = CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        NULL,
        0);

    if (nameSize > 0) {
        certInfo->issuerName = (LPWSTR)MemAllocZeroed(nameSize * sizeof(WCHAR));
        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            certInfo->issuerName,
            nameSize);
    }

    MemFree(signerInfo);
    result = TRUE;

cleanup:
    if (pCertContext) CertFreeCertificateContext(pCertContext);
    if (hStore) CertCloseStore(hStore, 0);
    if (hMsg) CryptMsgClose(hMsg);
    return result;
}

/**
 * Check if file signature verification is disabled via registry (same switch as checked by VerifyFileSignature)
 */
static bool IsFileSignatureVerificationDisabled()
{
   bool disabled = false;
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD value = 0;
      DWORD size = sizeof(DWORD);
      if (RegQueryValueEx(hKey, _T("DisableFileSignatureVerification"), nullptr, nullptr, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS)
         disabled = (value != 0);
      RegCloseKey(hKey);
   }
   return disabled;
}

/**
 * Get semicolon-separated list of trusted package publishers. Configuration parameter
 * takes precedence over registry value; built-in default is used when neither is set.
 */
static String GetTrustedPackagePublishers()
{
   shared_ptr<Config> config = g_config;
   const TCHAR *value = config->getValue(_T("/CORE/TrustedPackagePublishers"));
   if (value != nullptr)
      return String(value);

   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      TCHAR buffer[8192];
      DWORD size = sizeof(buffer) - sizeof(TCHAR);
      DWORD type;
      if ((RegQueryValueEx(hKey, _T("TrustedPackagePublishers"), nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS) && (type == REG_SZ))
      {
         buffer[size / sizeof(TCHAR)] = 0;
         RegCloseKey(hKey);
         return String(buffer);
      }
      RegCloseKey(hKey);
   }

   return String(DEFAULT_TRUSTED_PUBLISHER);
}

/**
 * Check if given signer subject name is in the trusted publisher list
 */
static bool IsTrustedPublisher(const TCHAR *subjectName)
{
   bool trusted = false;
   GetTrustedPackagePublishers().split(_T(";"), true,
      [subjectName, &trusted] (const String& publisher)
      {
         if (!publisher.isEmpty() && !_tcsicmp(publisher.cstr(), subjectName))
            trusted = true;
      });
   return trusted;
}

/**
 * Validate agent upgrade package: signature validity, signer identity, product identity,
 * and version gate (older packages are rejected unless downgrade is allowed).
 */
uint32_t ValidateUpgradePackage(const TCHAR *pkgFile, bool allowDowngrade)
{
   if (IsFileSignatureVerificationDisabled())
   {
      nxlog_debug_tag(_T("upgrade"), 4, _T("ValidateUpgradePackage(%s): file signature verification is disabled, package validation skipped"), pkgFile);
      return ERR_SUCCESS;
   }

   if (!VerifyFileSignature(pkgFile))
   {
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: cannot verify signature of installer package \"%s\""), pkgFile);
      return ERR_BAD_SIGNATURE;
   }

   PE_CERT_INFO certInfo;
   if (!GetPeCertificateInfo(pkgFile, &certInfo) || (certInfo.subjectName == nullptr))
   {
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: cannot extract signer certificate from installer package \"%s\""), pkgFile);
      return ERR_UNTRUSTED_PACKAGE;
   }

   if (!IsTrustedPublisher(certInfo.subjectName))
   {
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: installer package \"%s\" is signed by \"%s\" which is not in the trusted publisher list"),
         pkgFile, certInfo.subjectName);
      return ERR_UNTRUSTED_PACKAGE;
   }

   DWORD unused;
   DWORD viSize = GetFileVersionInfoSize(pkgFile, &unused);
   if (viSize == 0)
   {
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: installer package \"%s\" does not have version information"), pkgFile);
      return ERR_UNTRUSTED_PACKAGE;
   }

   BYTE *versionInfo = static_cast<BYTE*>(MemAlloc(viSize));
   if (!GetFileVersionInfo(pkgFile, 0, viSize, versionInfo))
   {
      MemFree(versionInfo);
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: cannot read version information from installer package \"%s\""), pkgFile);
      return ERR_UNTRUSTED_PACKAGE;
   }

   // Check product name (accept match in any language block)
   struct LANGANDCODEPAGE
   {
      WORD language;
      WORD codePage;
   } *translations;
   UINT translationsSize;
   bool productNameMatch = false;
   TCHAR actualProductName[256] = _T("");
   if (VerQueryValue(versionInfo, _T("\\VarFileInfo\\Translation"), reinterpret_cast<void**>(&translations), &translationsSize))
   {
      for(UINT i = 0; i < translationsSize / sizeof(LANGANDCODEPAGE); i++)
      {
         TCHAR subBlock[64];
         _sntprintf(subBlock, 64, _T("\\StringFileInfo\\%04x%04x\\ProductName"), translations[i].language, translations[i].codePage);
         TCHAR *productName;
         UINT productNameSize;
         if (VerQueryValue(versionInfo, subBlock, reinterpret_cast<void**>(&productName), &productNameSize) && (productNameSize > 0))
         {
            TCHAR name[256];
            _tcslcpy(name, productName, 256);
            Trim(name);  // Inno Setup pads VERSIONINFO strings with trailing spaces
            if (!_tcsicmp(name, UPGRADE_PACKAGE_PRODUCT_NAME))
            {
               productNameMatch = true;
               break;
            }
            if (actualProductName[0] == 0)
               _tcslcpy(actualProductName, name, 256);
         }
      }
   }
   if (!productNameMatch)
   {
      MemFree(versionInfo);
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: installer package \"%s\" has unexpected product name \"%s\""), pkgFile, actualProductName);
      return ERR_UNTRUSTED_PACKAGE;
   }

   // Version gate: reject packages older than the running agent unless downgrade is allowed
   VS_FIXEDFILEINFO *fixedInfo;
   UINT fixedInfoSize;
   if (!VerQueryValue(versionInfo, _T("\\"), reinterpret_cast<void**>(&fixedInfo), &fixedInfoSize) || (fixedInfoSize < sizeof(VS_FIXEDFILEINFO)))
   {
      MemFree(versionInfo);
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: cannot read fixed version information from installer package \"%s\""), pkgFile);
      return ERR_UNTRUSTED_PACKAGE;
   }

   DWORD packageVersionMS = fixedInfo->dwFileVersionMS;
   DWORD packageVersionLS = fixedInfo->dwFileVersionLS;
   MemFree(versionInfo);

   uint64_t packageVersion = (static_cast<uint64_t>(packageVersionMS) << 32) | packageVersionLS;
   uint64_t agentVersion = (static_cast<uint64_t>((NETXMS_VERSION_MAJOR << 16) | NETXMS_VERSION_MINOR) << 32) |
         ((static_cast<uint32_t>(NETXMS_VERSION_RELEASE) << 16) | NETXMS_VERSION_BUILD);
   if ((packageVersion < agentVersion) && !allowDowngrade)
   {
      nxlog_write(NXLOG_WARNING, _T("Agent upgrade rejected: installer package \"%s\" version %u.%u.%u.%u is older than running agent version %d.%d.%d.%d and downgrade is not allowed for requesting server"),
         pkgFile, packageVersionMS >> 16, packageVersionMS & 0xFFFF, packageVersionLS >> 16, packageVersionLS & 0xFFFF,
         NETXMS_VERSION_MAJOR, NETXMS_VERSION_MINOR, NETXMS_VERSION_RELEASE, NETXMS_VERSION_BUILD);
      return ERR_DOWNGRADE_NOT_ALLOWED;
   }

   nxlog_debug_tag(_T("upgrade"), 4, _T("ValidateUpgradePackage(%s): package accepted (signer \"%s\", version %u.%u.%u.%u)"),
      pkgFile, certInfo.subjectName, packageVersionMS >> 16, packageVersionMS & 0xFFFF, packageVersionLS >> 16, packageVersionLS & 0xFFFF);
   return ERR_SUCCESS;
}
