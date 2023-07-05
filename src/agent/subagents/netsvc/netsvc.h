/*
** NetXMS Network Service check subagent
** Copyright (C) 2013-2023 Raden Solutions
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
**/

#ifndef __netsvc__h__
#define __netsvc__h__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <netxms-regex.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("netsvc")

enum
{
   PC_ERR_NONE,
   PC_ERR_BAD_PARAMS,
   PC_ERR_CONNECT,
   PC_ERR_NOMATCH,
   PC_ERR_INTERNAL,
   PC_ERR_HANDSHAKE
};

#define NETSVC_AF_VERIFYPEER              0x0001
#define NETSVC_AF_NEGATIVE_TIME_ON_ERROR  0x0002

#define NETSVC_MAX_URL_LENGTH    1024

/**
 * URL parser
 */
class URLParser
{
private:
#if HAVE_DECL_CURL_URL
   CURLU *m_url;
#else
   char *m_url;
   char *m_authority;
   char m_defaultPort[8];
#endif
   char *m_scheme;
   char *m_host;
   char *m_port;
   bool m_valid;

#if !HAVE_DECL_CURL_URL
   void parseAuthority();
#endif

public:
   URLParser(const char *url);
   ~URLParser();

   const char *scheme();
   const char *host();
   const char *port();

   bool isValid() const { return m_valid; }
};

LONG NetworkServiceStatus_HTTP(CURL *curl, const OptionList &options, char *url, PCRE *compiledPattern, int *result);
LONG NetworkServiceStatus_Other(CURL *curl, const OptionList& options, const char *url, int *result);
LONG NetworkServiceStatus_SMTP(CURL *curl, const OptionList& options, const char *url, int *result);
LONG NetworkServiceStatus_SSH(const char *host, const char *port, const OptionList& options, int *result);
LONG NetworkServiceStatus_TCP(const char *host, const char *port, const OptionList& options, int *result);
LONG NetworkServiceStatus_Telnet(const char *host, const char *port, const OptionList& options, int *result);

LONG H_CheckHTTP(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_CheckPOP3(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CheckSSH(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CheckTCP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CheckTelnet(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_CheckTLS(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_HTTPChecksum(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TLSCertificateInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

int CheckHTTP(const char *hostname, const InetAddress& addr, uint16_t port, bool useTLS, const char *uri, const char *hostHeader, const char *match, uint32_t timeout);
int CheckPOP3(const InetAddress& addr, uint16_t port, bool enableTLS, const char *username, const char *password, uint32_t timeout);
int CheckSMTP(const InetAddress& addr, uint16_t port, bool enableTLS, const char *to, uint32_t timeout);
int CheckSSH(const char *hostname, const InetAddress &addr, uint16_t port, uint32_t timeout);
int CheckTCP(const char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout);
int CheckTelnet(const char *hostname, const InetAddress &addr, uint16_t port, uint32_t timeout);
int CheckTLS(const char *hostname, const InetAddress &addr, uint16_t port, uint32_t timeout);

CURL *PrepareCurlHandle(const InetAddress& addr, uint16_t port, const char *schema, uint32_t timeout);
void CurlCommonSetup(CURL *curl, const char *url, const OptionList& options, uint32_t timeout);
int CURLCodeToCheckResult(CURLcode cc);
uint32_t GetTimeoutFromArgs(const TCHAR* metric, int argIndex);

SOCKET NetConnectTCP(const char* hostname, const InetAddress& addr, uint16_t port, uint32_t timeout);

/**
 * Read from socket
 */
static inline ssize_t NetRead(SOCKET hSocket, void* buffer, size_t size)
{
#ifdef _WIN32
   return recv(hSocket, static_cast<char*>(buffer), static_cast<int>(size), 0);
#else
   ssize_t rc;
   do
   {
      rc = recv(hSocket, static_cast<char*>(buffer), size, 0);
   }
   while ((rc == -1) && (errno == EINTR));
   return rc;
#endif
}

/**
 * Write to the socket
 */
static inline bool NetWrite(SOCKET hSocket, const void* data, size_t size)
{
   return SendEx(hSocket, data, size, 0, nullptr) == static_cast<ssize_t>(size);
}

/**
 * Close socket gracefully
 */
static inline void NetClose(SOCKET hSocket)
{
   shutdown(hSocket, SHUT_RDWR);
   closesocket(hSocket);
}

extern uint32_t g_netsvcFlags;
extern uint32_t g_netsvcTimeout;
extern char g_netsvcDomainName[];

#endif // __netsvc__h__
