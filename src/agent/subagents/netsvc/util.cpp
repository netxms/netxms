/*
** NetXMS - Network Management System
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
**/

#include "netsvc.h"

/**
 * Connect to given host
 */
SOCKET NetConnectTCP(const char* hostname, const InetAddress& addr, uint16_t port, uint32_t timeout)
{
   InetAddress hostAddr = (hostname != nullptr) ? InetAddress::resolveHostName(hostname) : addr;
   if (!hostAddr.isValidUnicast() && !hostAddr.isLoopback())
      return INVALID_SOCKET;

   return ConnectToHost(hostAddr, port, (timeout != 0) ? timeout : g_netsvcTimeout);
}

/**
 * Translate CURL error code into service check result code
 */
int CURLCodeToCheckResult(CURLcode cc)
{
   switch(cc)
   {
      case CURLE_OK:
         return PC_ERR_NONE;
      case CURLE_UNSUPPORTED_PROTOCOL:
      case CURLE_URL_MALFORMAT:
      case CURLE_NOT_BUILT_IN:
      case CURLE_UNKNOWN_OPTION:
      case CURLE_LDAP_INVALID_URL:
         return PC_ERR_BAD_PARAMS;
      case CURLE_COULDNT_RESOLVE_PROXY:
      case CURLE_COULDNT_RESOLVE_HOST:
      case CURLE_COULDNT_CONNECT:
         return PC_ERR_CONNECT;
      default:
         return PC_ERR_HANDSHAKE;
   }
}

/**
 * Get timeout from metric arguments (used by legacy metrics)
 *
 * @return default timeout is returned if no timeout specified in args
 */
uint32_t GetTimeoutFromArgs(const TCHAR *metric, int argIndex)
{
   TCHAR timeoutText[64];
   if (!AgentGetParameterArg(metric, argIndex, timeoutText, 64))
      return g_netsvcTimeout;
   TCHAR* eptr;
   uint32_t timeout = _tcstol(timeoutText, &eptr, 0);
   return ((timeout != 0) && (*eptr == 0)) ? timeout : g_netsvcTimeout;
}
