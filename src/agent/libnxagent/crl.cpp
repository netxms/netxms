/*
** NetXMS - Network Management System
** Copyright (C) 2007-2025 Victor Kirhenshtein
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
** File: crl.cpp
**
**/

#include "libnxagent.h"
#include <netxms-version.h>
#include <nxstat.h>

#define DEBUG_TAG _T("crypto.crl")

#include <curl/curl.h>

#ifndef CURL_MAX_HTTP_HEADER
// workaround for older cURL versions
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

#if OPENSSL_VERSION_NUMBER < 0x10000000L
static int X509_CRL_get0_by_cert(X509_CRL *crl, X509_REVOKED **ret, X509 *x)
{
   ASN1_INTEGER *serial = X509_get_serialNumber(x);
   STACK_OF(X509_REVOKED) *revoked = X509_CRL_get_REVOKED(crl);
   int count = sk_X509_REVOKED_num(revoked);
   for(int i = 0; i < count; i++)
   {
      X509_REVOKED *r = sk_X509_REVOKED_value(revoked, i);
      if (ASN1_INTEGER_cmp(serial, r->serialNumber) == 0)
      {
         *ret = r;
         return 1;
      }
   }
   return 0;
}
#endif

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *data, size_t size, size_t count, void *context)
{
   return _write(*static_cast<int*>(context), data, static_cast<int>(size * count));
}

/**
 * Download from from remote location
 */
static bool DownloadFile(const TCHAR *file, const char *url)
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): started download from \"%hs\" to \"%s\""), url, file);

   TCHAR tmpFile[MAX_PATH];
   _tcslcpy(tmpFile, file, MAX_PATH);
   _tcslcat(tmpFile, _T(".part"), MAX_PATH);
   int fileHandle = _topen(tmpFile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
   if (fileHandle == -1)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): cannot open file \"%s\" (%s)"), tmpFile, _tcserror(errno));
      return false;
   }

   bool success = false;
   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
      char curlError[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlError);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileHandle);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Agent/" NETXMS_VERSION_STRING_A);
      if (curl_easy_setopt(curl, CURLOPT_URL, url) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): file \"%s\" from \"%hs\" download completed"), file, url);
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): transfer error for \"%hs\" (%hs)"), url, curlError);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): invalid URL \"%hs\""), url);
      }
      curl_easy_cleanup(curl);
   }
   _close(fileHandle);

   if (success)
   {
      NX_STAT_STRUCT fs;
      if (CALL_STAT(tmpFile, &fs) == 0)
      {
         _tremove(file);
         if (fs.st_size != 0)
         {
            _trename(tmpFile, file);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): empty document retrieved from \"%hs\""), url);
            _tremove(tmpFile);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("DownloadFile(): unexpected file system error while accessing \"%s\" (%s)"), tmpFile, _tcserror(errno));
         _tremove(tmpFile);
         success = false;
      }
   }

   return success;
}

/**
 * CRL descriptor
 */
class CRL
{
private:
   char *m_url;
   TCHAR *m_fileName;
   X509_CRL *m_content;

   CRL(const char *url, const TCHAR *fileName);

public:
   ~CRL();

   void loadFromFile();
   void downloadFromRemote();
   void reload();

   const TCHAR *getFileName() const { return m_fileName; }
   bool isValid() const { return m_content != nullptr; }
   bool isCertificateRevoked(X509 *cert, const X509 *issuer);

   static CRL *createRemote(const char *url);
   static CRL *createLocal(const TCHAR *fileName);
};

/**
 * Create local CRL
 */
CRL *CRL::createLocal(const TCHAR *fileName)
{
   return new CRL(nullptr, fileName);
}

/**
 * Create remote CRL
 */
CRL *CRL::createRemote(const char *url)
{
   TCHAR fileName[MAX_PATH];
   GetNetXMSDirectory(nxDirData, fileName);
   _tcslcat(fileName, FS_PATH_SEPARATOR _T("crl") FS_PATH_SEPARATOR, MAX_PATH);
   size_t l = _tcslen(fileName);
#ifdef UNICODE
   size_t chars = utf8_to_wchar(url, -1, &fileName[l], MAX_PATH - l - 1);
   fileName[l + chars] = 0;
#else
   strlcat(fileName, url, MAX_PATH);
#endif

   for(TCHAR *p = &fileName[l]; *p != 0; p++)
      if ((*p == _T(':')) || (*p == _T('/')) || (*p == _T('\\')) || (*p == _T('?')) || (*p == _T('&')))
         *p = _T('_');

   return new CRL(url, fileName);
}

/**
 * CRL constructor
 */
CRL::CRL(const char *url, const TCHAR *fileName)
{
   m_url = MemCopyStringA(url);
   m_fileName = MemCopyString(fileName);
   m_content= nullptr;
}

/**
 * CRL destructor
 */
CRL::~CRL()
{
   MemFree(m_url);
   MemFree(m_fileName);
   if (m_content != nullptr)
      X509_CRL_free(m_content);
}

/**
 * Load CRL from file
 */
void CRL::loadFromFile()
{
   FILE *fp = _tfopen(m_fileName, _T("r"));
   if (fp == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot open CRL file \"%s\" (%s)"), m_fileName, _tcserror(errno));
      return;
   }

   if (m_content != nullptr)
      X509_CRL_free(m_content);

   m_content = PEM_read_X509_CRL(fp, nullptr, nullptr, nullptr);

   if (m_content == nullptr)
   {
      fseek(fp, 0, SEEK_SET);
      m_content = d2i_X509_CRL_fp(fp, nullptr); // Attempt DER format
   }

   if (m_content == nullptr)
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Error loading CRL file \"%s\""), m_fileName);

   fclose(fp);
}

/**
 * Download CRL from remote location
 */
void CRL::downloadFromRemote()
{
   if (DownloadFile(m_fileName, m_url))
   {
      loadFromFile();
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot download CRL from \"%hs\""), m_url);
   }
}

/**
 * Reload CRL
 */
void CRL::reload()
{
   if (m_url != nullptr)
      downloadFromRemote();
   else
      loadFromFile();
}

/**
 * Check if given certificate is revoked by issuer
 */
bool CRL::isCertificateRevoked(X509 *cert, const X509 *issuer)
{
   if (m_content == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CRL \"%s\" is not valid"), m_fileName);
      return false;
   }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   int verify = X509_CRL_verify(m_content, X509_get0_pubkey(issuer));
#else
   EVP_PKEY *pkey = X509_get_pubkey(const_cast<X509*>(issuer));
   int verify = X509_CRL_verify(m_content, pkey);
   EVP_PKEY_free(pkey);
#endif
   if (verify <= 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("CRL \"%s\" is not valid for issuer %s"),
               m_fileName, GetCertificateSubjectString(issuer).cstr());
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Checking certificate %s against CRL \"%s\""),
            GetCertificateSubjectString(cert).cstr(), m_fileName);
   X509_REVOKED *revoked;
   return X509_CRL_get0_by_cert(m_content, &revoked, cert) == 1;
}

/**
 * CRL list
 */
static StringObjectMap<CRL> s_crls(Ownership::True);
static Mutex s_crlLock;

/**
 * Add local CRL
 */
void LIBNXAGENT_EXPORTABLE AddLocalCRL(const TCHAR *fileName)
{
   s_crlLock.lock();
   if (!s_crls.contains(fileName))
   {
      CRL *crl = CRL::createLocal(fileName);
      crl->loadFromFile();
      s_crls.set(crl->getFileName(), crl);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Added local CRL \"%s\""), fileName);
   }
   s_crlLock.unlock();
}

/**
 * Add remote CRL
 */
void LIBNXAGENT_EXPORTABLE AddRemoteCRL(const char *url, bool allowDownload)
{
   CRL *crl = CRL::createRemote(url);
   s_crlLock.lock();
   if (!s_crls.contains(crl->getFileName()))
   {
      crl->loadFromFile();
      if (!crl->isValid() && allowDownload)
         crl->downloadFromRemote();
      s_crls.set(crl->getFileName(), crl);
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Added remote CRL \"%hs\""), url);
   }
   else
   {
      delete crl;
   }
   s_crlLock.unlock();
}

/**
 * Check certificate against CRLs. Returns true if certificate is revoked.
 */
bool LIBNXAGENT_EXPORTABLE CheckCertificateRevocation(X509 *cert, const X509 *issuer)
{
   bool revoked = false;
   s_crlLock.lock();
   auto it = s_crls.begin();
   while(it.hasNext() && !revoked)
   {
      CRL *crl = it.next()->value;
      revoked = crl->isCertificateRevoked(cert, issuer);
   }
   s_crlLock.unlock();
   return revoked;
}

/**
 * Reload all registered CRLs
 */
void LIBNXAGENT_EXPORTABLE ReloadAllCRLs()
{
   s_crlLock.lock();
   auto it = s_crls.begin();
   while(it.hasNext())
      it.next()->value->reload();
   s_crlLock.unlock();
}
