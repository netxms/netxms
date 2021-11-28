/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: portcheck.h
**
**/

#ifndef __portcheck_h__
#define __portcheck_h__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>

#define SUBAGENT_DEBUG_TAG _T("portcheck")

enum
{
   PROTOCOL_UDP,
   PROTOCOL_TCP
};

uint32_t GetTimeoutFromArgs(const TCHAR *metric, int argIndex);
SOCKET NetConnectTCP(const char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout);

/**
 * Write to the socket
 */
static inline bool NetWrite(SOCKET hSocket, const void *data, size_t size)
{
   return SendEx(hSocket, data, size, 0, INVALID_MUTEX_HANDLE) == static_cast<ssize_t>(size);
}

/**
 * Read from socket
 */
static inline ssize_t NetRead(SOCKET hSocket, void *buffer, size_t size)
{
#ifdef _WIN32
   return recv(hSocket, static_cast<char*>(buffer), static_cast<int>(size), 0);
#else
   ssize_t rc;
   do
   {
      rc = recv(hSocket, static_cast<char*>(buffer), size, 0);
   } while((rc == -1) && (errno == EINTR));
   return rc;
#endif
}

/**
 * Close socket gracefully
 */
static inline void NetClose(SOCKET nSocket)
{
   shutdown(nSocket, SHUT_RDWR);
   closesocket(nSocket);
}

/**
 * Service check return codes
 */
enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
	PC_ERR_HANDSHAKE,
	PC_ERR_INTERNAL
};

/**
 * Flags
 */
#define SCF_NEGATIVE_TIME_ON_ERROR  0x0001

LONG H_CheckPOP3(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckPOP3(char *, const InetAddress&, short, char *, char *, UINT32);
LONG H_CheckSSH(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckSSH(char *, const InetAddress&, short, char *, char *, UINT32);
LONG H_CheckSMTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckSMTP(char *, const InetAddress&, short, char *, UINT32);
LONG H_CheckHTTP(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckHTTP(char *, const InetAddress&, short, char *, char *, char *, UINT32);
int CheckHTTPS(char *, const InetAddress&, short, char *, char *, char *, UINT32);
LONG H_CheckCustom(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckCustom(char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout);
LONG H_CheckTelnet(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckTelnet(char *, const InetAddress&, short, char *, char *, UINT32);
LONG H_CheckTLS(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
int CheckTLS(char *hostname, const InetAddress& addr, uint16_t port, uint32_t timeout);
LONG H_TLSCertificateInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

extern char g_szDomainName[];
extern char g_hostName[];
extern char g_szFailedDir[];
extern uint32_t g_serviceCheckFlags;

#endif // __portcheck__h__
