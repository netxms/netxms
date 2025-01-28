/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2025 Raden Solutions
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
** File: cert.cpp
**
**/

#include "libnetxms.h"
#include <nxcrypto.h>
#include <nxstat.h>
#include <openssl/asn1t.h>
#include <openssl/x509v3.h>

#define DEBUG_TAG    _T("crypto.cert")

#ifdef _WITH_ENCRYPTION

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static inline const ASN1_TIME *X509_get0_notAfter(const X509 *x)
{
   return X509_get_notAfter(x);
}

static inline const ASN1_TIME *X509_get0_notBefore(const X509 *x)
{
   return X509_get_notBefore(x);
}

static inline const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *s)
{
   return s->data;
}
#endif

/**
 * Get field from X.509 name
 */
static bool GetX509NameField(X509_NAME *name, int nid, TCHAR *buffer, size_t size)
{
   int idx = X509_NAME_get_index_by_NID(name, nid, -1);
   if (idx == -1)
      return false;

   X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, idx);
   if (entry == nullptr)
      return false;

   ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
   if (data == nullptr)
      return false;

   unsigned char *text;
   ASN1_STRING_to_UTF8(&text, data);
#ifdef UNICODE
   utf8_to_wchar((char *)text, -1, buffer, size);
#else
   utf8_to_mb((char *)text, -1, buffer, size);
#endif
   buffer[size - 1] = 0;
   OPENSSL_free(text);
   return true;
}

/**
 * Get single field from certificate subject
 */
bool LIBNETXMS_EXPORTABLE GetCertificateSubjectField(const X509 *cert, int nid, TCHAR *buffer, size_t size)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   X509_NAME *subject = X509_get_subject_name(cert);
#else
   X509_NAME *subject = X509_get_subject_name(const_cast<X509*>(cert));
#endif
   return (subject != nullptr) ? GetX509NameField(subject, nid, buffer, size) : false;
}

/**
 * Get CN from certificate
 */
bool LIBNETXMS_EXPORTABLE GetCertificateCN(const X509 *cert, TCHAR *buffer, size_t size)
{
   return GetCertificateSubjectField(cert, NID_commonName, buffer, size);
}

/**
 * Get OU from certificate
 */
bool LIBNETXMS_EXPORTABLE GetCertificateOU(const X509 *cert, TCHAR *buffer, size_t size)
{
   return GetCertificateSubjectField(cert, NID_organizationalUnitName, buffer, size);
}

/**
 * Convert X.509 name to string representation
 */
static String X509NameToString(X509_NAME *name)
{
   StringBuffer text;
   int count = X509_NAME_entry_count(name);
   for(int i = 0; i < count; i++)
   {
      X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, i);

      if (!text.isEmpty())
         text.append(_T(','));

      ASN1_OBJECT *o = X509_NAME_ENTRY_get_object(entry);
      text.appendUtf8String(OBJ_nid2sn(OBJ_obj2nid(o)));
      text.append(_T('='));

      ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
      if (data == nullptr)
         continue;

      unsigned char *entryText;
      ASN1_STRING_to_UTF8(&entryText, data);
      text.appendUtf8String(reinterpret_cast<char*>(entryText));
      OPENSSL_free(entryText);
   }
   return text;
}

/**
 * Get subject string (C=XX,ST=state,L=locality,O=org,OU=unit,CN=cn) from certificate
 */
String LIBNETXMS_EXPORTABLE GetCertificateSubjectString(const X509 *cert)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   X509_NAME *subject = X509_get_subject_name(cert);
#else
   X509_NAME *subject = X509_get_subject_name(const_cast<X509*>(cert));
#endif
   return (subject != nullptr) ? X509NameToString(subject) : String();
}

/**
 * Get issuer string (C=XX,ST=state,L=locality,O=org,OU=unit,CN=cn) from certificate
 */
String LIBNETXMS_EXPORTABLE GetCertificateIssuerString(const X509 *cert)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   X509_NAME *issuer = X509_get_issuer_name(cert);
#else
   X509_NAME *issuer = X509_get_issuer_name(const_cast<X509*>(cert));
#endif
   return (issuer != nullptr) ? X509NameToString(issuer) : String();
}

/**
 * Get certificate expiration time
 */
template<const ASN1_TIME* (*GetTime)(const X509*)> static inline time_t GetCertificateTime(const X509 *cert)
{
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L) && !defined(LIBRESSL_VERSION_NUMBER)
   struct tm expTime;
   ASN1_TIME_to_tm(GetTime(cert), &expTime);
   return timegm(&expTime);
#elif (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
   ASN1_TIME *epoch = ASN1_TIME_set(nullptr, static_cast<time_t>(0));
   int days, seconds;
   ASN1_TIME_diff(&days, &seconds, epoch, GetTime(cert));
   ASN1_TIME_free(epoch);
   return static_cast<time_t>(days) * 86400 + seconds;
#else
   struct tm expTime;
   memset(&expTime, 0, sizeof(expTime));

   const ASN1_TIME *atime = GetTime(cert);

   size_t i = 0;
   const char *s = reinterpret_cast<const char*>(atime->data);
   if (atime->type == V_ASN1_UTCTIME) /* two digit year */
   {
      if (atime->length < 12)
         return 0;  // Invalid format
      expTime.tm_year = (s[i] - '0') * 10 + (s[i + 1] - '0');
      if (expTime.tm_year < 70)
         expTime.tm_year += 100;
      i += 2;
   }
   else if (atime->type == V_ASN1_GENERALIZEDTIME) /* four digit year */
   {
      if (atime->length < 14)
         return 0;  // Invalid format
      expTime.tm_year = (s[i] - '0') * 1000 + (s[i + 1] - '0') * 100 + (s[i + 2] - '0') * 10 + (s[i + 3] - '0');
      expTime.tm_year -= 1900;
      i += 4;
   }
   else
   {
      return 0;  // Unknown type
   }
   expTime.tm_mon = (s[i] - '0') * 10 + (s[i + 1] - '0') - 1; // -1 since January is 0 not 1.
   i += 2;
   expTime.tm_mday = (s[i] - '0') * 10 + (s[i + 1] - '0');
   i += 2;
   expTime.tm_hour = (s[i] - '0') * 10 + (s[i + 1] - '0');
   i += 2;
   expTime.tm_min  = (s[i] - '0') * 10 + (s[i + 1] - '0');
   i += 2;
   expTime.tm_sec  = (s[i] - '0') * 10 + (s[i + 1] - '0');
   return timegm(&expTime);
#endif
}

/**
 * Get certificate expiration time
 */
time_t LIBNETXMS_EXPORTABLE GetCertificateExpirationTime(const X509 *cert)
{
   return GetCertificateTime<X509_get0_notAfter>(cert);
}

/**
 * Get certificate creation time
 */
time_t LIBNETXMS_EXPORTABLE GetCertificateIssueTime(const X509 *cert)
{
   return GetCertificateTime<X509_get0_notBefore>(cert);
}

/**
 * Certificate template extension structure as described in Microsoft documentation
 * https://docs.microsoft.com/en-us/windows/win32/api/certenroll/nn-certenroll-ix509extensiontemplate
 *
 * ----------------------------------------------------------------------
 * -- CertificateTemplate
 * -- XCN_OID_CERTIFICATE_TEMPLATE (1.3.6.1.4.1.311.21.7)
 * ----------------------------------------------------------------------
 *
 * CertificateTemplate ::= SEQUENCE
 * {
 * templateID              EncodedObjectID,
 * templateMajorVersion    TemplateVersion,
 * templateMinorVersion    TemplateVersion OPTIONAL
 * }
 */
struct CERTIFICATE_TEMPLATE
{
   ASN1_OBJECT *templateId;
   ASN1_INTEGER *templateMajorVersion;
   ASN1_INTEGER *templateMinorVersion;
};

/**
 * Define ASN.1 sequence for CERTIFICATE_TEMPLATE
 */
ASN1_NDEF_SEQUENCE(CERTIFICATE_TEMPLATE) =
{
   ASN1_SIMPLE(CERTIFICATE_TEMPLATE, templateId, ASN1_OBJECT),
   ASN1_SIMPLE(CERTIFICATE_TEMPLATE, templateMajorVersion, ASN1_INTEGER),
   ASN1_SIMPLE(CERTIFICATE_TEMPLATE, templateMinorVersion, ASN1_INTEGER)
} ASN1_NDEF_SEQUENCE_END(CERTIFICATE_TEMPLATE)

/**
 * Function implementations for CERTIFICATE_TEMPLATE structure
 */
IMPLEMENT_ASN1_FUNCTIONS(CERTIFICATE_TEMPLATE)

/**
 * Get certificate's template ID (extension 1.3.6.1.4.1.311.21.7)
 */
String LIBNETXMS_EXPORTABLE GetCertificateTemplateId(const X509 *cert)
{
   ASN1_OBJECT *oid = OBJ_txt2obj("1.3.6.1.4.1.311.21.7", 1);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   int index = X509_get_ext_by_OBJ(cert, oid, -1);
#else
   int index = X509_get_ext_by_OBJ(const_cast<X509*>(cert), oid, -1);
#endif
   ASN1_OBJECT_free(oid);
   if (index == -1)
      return String();
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   X509_EXTENSION *ext = X509_get_ext(cert, index);
#else
   X509_EXTENSION *ext = X509_get_ext(const_cast<X509*>(cert), index);
#endif
   if (ext == nullptr)
      return String();
   ASN1_STRING *value = X509_EXTENSION_get_data(ext);
   if (value == nullptr)
      return String();
   const unsigned char *bytes = ASN1_STRING_get0_data(value);
   CERTIFICATE_TEMPLATE *t = d2i_CERTIFICATE_TEMPLATE(nullptr, &bytes, ASN1_STRING_length(value));
   if (t == nullptr)
      return String();
   char oidA[256];
   OBJ_obj2txt(oidA, 256, t->templateId, 1);
   CERTIFICATE_TEMPLATE_free(t);
#ifdef UNICODE
   WCHAR oidW[256];
   mb_to_wchar(oidA, -1, oidW, 256);
   return String(oidW);
#else
   return String(oidA);
#endif
}

/**
 * Get HTTP/HTTPS CRL distribution point from certificate
 */
String LIBNETXMS_EXPORTABLE GetCertificateCRLDistributionPoint(const X509 *cert)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   STACK_OF(DIST_POINT) *dps = static_cast<STACK_OF(DIST_POINT)*>(X509_get_ext_d2i(cert, NID_crl_distribution_points, nullptr, nullptr));
#else
   STACK_OF(DIST_POINT) *dps = static_cast<STACK_OF(DIST_POINT)*>(X509_get_ext_d2i(const_cast<X509*>(cert), NID_crl_distribution_points, nullptr, nullptr));
#endif
   if (dps == nullptr)
      return String();

   StringBuffer result;
   for (int i = 0; i < sk_DIST_POINT_num(dps); i++)
   {
      DIST_POINT *dp = sk_DIST_POINT_value(dps, i);
      if (dp->distpoint->type == 0)
      {
         GENERAL_NAMES *names = dp->distpoint->name.fullname;
         for (int j = 0; j < sk_GENERAL_NAME_num(names); j++)
         {
            GENERAL_NAME *n = sk_GENERAL_NAME_value(names, j);
            if (n->type == GEN_URI)
            {
               ASN1_IA5STRING *uri = n->d.uniformResourceIdentifier;
               int l = ASN1_STRING_length(uri);
               if (l > 7)
               {
                  const unsigned char *data = ASN1_STRING_get0_data(uri);
                  if (!memcmp(data, "http:", 5) || !memcmp(data, "https:", 6))
                  {
                     result.appendUtf8String(reinterpret_cast<const char*>(data), l);
                     break;
                  }
               }
            }
         }
      }
   }
   sk_DIST_POINT_free(dps);
   return result;
}

/**
 * Create default certificate store with trusted certificates
 */
X509_STORE LIBNETXMS_EXPORTABLE *CreateTrustedCertificatesStore(const StringSet& trustedCertificates, bool useSystemStore)
{
   X509_STORE *store = X509_STORE_new();
   if (store == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("CreateTrustedCertificatesStore: cannot create certificate store"));
      return nullptr;
   }

   X509_LOOKUP *dirLookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
   X509_LOOKUP *fileLookup = X509_STORE_add_lookup(store, X509_LOOKUP_file());

   if (!trustedCertificates.isEmpty())
   {
      auto it = trustedCertificates.begin();
      while(it.hasNext())
      {
         const TCHAR *trustedRoot = it.next();
         NX_STAT_STRUCT st;
         if (CALL_STAT_FOLLOW_SYMLINK(trustedRoot, &st) != 0)
            continue;

#ifdef UNICODE
         char mbTrustedRoot[MAX_PATH];
         WideCharToMultiByteSysLocale(trustedRoot, mbTrustedRoot, MAX_PATH);
#else
         const char *mbTrustedRoot = trustedRoot;
#endif
         int added = S_ISDIR(st.st_mode) ?
                  X509_LOOKUP_add_dir(dirLookup, mbTrustedRoot, X509_FILETYPE_PEM) :
                  X509_LOOKUP_load_file(fileLookup, mbTrustedRoot, X509_FILETYPE_PEM);
         if (added)
            nxlog_debug_tag(DEBUG_TAG, 6, _T("CreateTrustedCertificatesStore: trusted %s \"%s\" added"), S_ISDIR(st.st_mode) ? _T("certificate directory") : _T("certificate"), trustedRoot);
      }
   }

   if (useSystemStore)
   {
#ifdef _WIN32
      HCERTSTORE hStore = CertOpenSystemStore(0, L"ROOT");
      if (hStore != nullptr)
      {
         PCCERT_CONTEXT context = nullptr;
         while ((context = CertEnumCertificatesInStore(hStore, context)) != nullptr)
         {
            TCHAR certName[1024];
            CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, certName, 1024);

            const unsigned char *in = context->pbCertEncoded;
            X509 *cert = d2i_X509(nullptr, &in, context->cbCertEncoded);
            if (cert != nullptr)
            {
               if (X509_STORE_add_cert(store, cert))
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("CreateTrustedCertificatesStore: added trusted root certificate \"%s\" from system store"), certName);
               else
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("CreateTrustedCertificatesStore: cannot add trusted root certificate \"%s\" from system store"), certName);
               X509_free(cert);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateTrustedCertificatesStore: cannot convert certificate \"%s\""), certName);
            }
         }
         CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);
      }
      else
      {
         TCHAR buffer[1024];
         nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateTrustedCertificatesStore: cannot open certificate store \"ROOT\" (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      }
#else    /* _WIN32 */
      static const char *defaultStoreLocations[] = {
         "/etc/ssl/certs",               // Ubuntu, Debian, and many other Linux distros
         "/usr/local/share/certs",       // FreeBSD
         "/etc/pki/tls/certs",           // Fedora/RHEL
         "/etc/openssl/certs",           // NetBSD
         "/var/ssl/certs",               // AIX
         nullptr
      };
      for(int i = 0; defaultStoreLocations[i] != nullptr; i++)
      {
         NX_STAT_STRUCT st;
         if (NX_STAT(defaultStoreLocations[i], &st) == 0)
         {
            int added = S_ISDIR(st.st_mode) ?
                     X509_LOOKUP_add_dir(dirLookup, defaultStoreLocations[i], X509_FILETYPE_PEM) :
                     X509_LOOKUP_load_file(fileLookup, defaultStoreLocations[i], X509_FILETYPE_PEM);
            if (added)
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("CreateTrustedCertificatesStore: added system certificate store at \"%hs\""), defaultStoreLocations[i]);
               break;
            }
         }
      }
#endif   /* _WIN32 */
   }

   return store;
}

#endif   /* _WITH_ENCRYPTION */
