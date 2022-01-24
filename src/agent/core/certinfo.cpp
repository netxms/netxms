/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Raden Solutions
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
** File: certinfo.cpp
**
**/

#include "nxagentd.h"
#include <openssl/pkcs12.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#define CI_DEBUG_TAG _T("x509.certificate")

/**
 * Get expiration date for certificate
 */
static inline String GetCertificateExpirationDate(X509* cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   TCHAR buffer[64];
   _tcsftime(buffer, 64, _T("%Y-%m-%d"), localtime(&e));
   return String(buffer);
}

/**
 * Get number of days until certificate expiration
 */
static inline int GetCertificateDaysUntilExpiration(X509* cert)
{
   time_t e = GetCertificateExpirationTime(cert);
   time_t now = time(nullptr);
   return static_cast<int>((e - now) / 86400);
}

/**
 * Handler for X509.Certificate.* parameters
 */
LONG H_CertificateInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   TCHAR fileName[MAX_PATH];
   AgentGetParameterArg(param, 1, fileName, MAX_PATH);
   if (fileName[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   FILE* file = _tfopen(fileName, _T("rb"));
   if (file == nullptr)
   {
      nxlog_debug_tag(CI_DEBUG_TAG, 6, _T("Cannot open certificate file %s (%s)"), fileName, _tcserror(errno));
      return SYSINFO_RC_ERROR;
   }

   // Try parsing as PEM
   X509* cert = PEM_read_X509(file, nullptr, nullptr, nullptr);

   // Try parsing as DER
   if (cert == nullptr)
   {
      nxlog_debug_tag(CI_DEBUG_TAG, 6, _T("Cannot parse certificate file %s as PEM, trying DER format"), fileName);
      rewind(file);
      cert = d2i_X509_fp(file, &cert);
   }

   // Try parsing as PKCS12
   if (cert == nullptr)
   {
      nxlog_debug_tag(CI_DEBUG_TAG, 6, _T("Cannot parse certificate file %s as DER, trying PKCS#12 format"), fileName);
      rewind(file);
      PKCS12* pkcs12 = nullptr;
      if (d2i_PKCS12_fp(file, &pkcs12) != nullptr)
      {
         char password[256];
         AgentGetParameterArgA(param, 2, password, 256);
         EVP_PKEY *pkey;
         if (PKCS12_parse(pkcs12, password, &pkey, &cert, nullptr))
         {
            EVP_PKEY_free(pkey);
         }
         else
         {
            nxlog_debug_tag(CI_DEBUG_TAG, 6, _T("Cannot parse PKCS#12 structures from file %s"), fileName);
         }
         PKCS12_free(pkcs12);
      }
      else
      {
         nxlog_debug_tag(CI_DEBUG_TAG, 6, _T("Cannot decode PKCS#12 structures from file %s"), fileName);
      }
   }
   fclose(file);

   if (cert == nullptr)
      return SYSINFO_RC_ERROR;

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

   return SYSINFO_RC_SUCCESS;
}
