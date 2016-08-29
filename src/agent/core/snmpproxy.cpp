/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: snmpproxy.cpp
**
**/

#include "nxagentd.h"

/**
 * SNMP buffer size
 */
#define SNMP_BUFFER_SIZE		65536

/**
 * SNMP proxy stats
 */
static UINT64 s_serverRequests = 0;
static UINT64 s_snmpRequests = 0;
static UINT64 s_snmpResponses = 0;

/**
 * Handler for SNMP proxy information parameters
 */
LONG H_SNMPProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'R':   // SNMP requests
         ret_uint64(value, s_snmpRequests);
         break;
      case 'r':   // SNMP responses
         ret_uint64(value, s_snmpResponses);
         break;
      case 'S':   // server requests
         ret_uint64(value, s_serverRequests);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Read PDU from network
 */
static BOOL ReadPDU(SOCKET hSocket, BYTE *pdu, UINT32 *pdwSize)
{
   fd_set rdfs;
   struct timeval tv;
	int nBytes;
#ifndef _WIN32
   int iErr;
	UINT32 dwTimeout = g_dwSNMPTimeout;
   QWORD qwTime;

   do
   {
#endif
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
#ifdef _WIN32
      tv.tv_sec = g_dwSNMPTimeout / 1000;
      tv.tv_usec = (g_dwSNMPTimeout % 1000) * 1000;
#else
      tv.tv_sec = dwTimeout / 1000;
      tv.tv_usec = (dwTimeout % 1000) * 1000;
#endif
#ifdef _WIN32
      if (select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv) <= 0)
         return FALSE;
#else
      qwTime = GetCurrentTimeMs();
      if ((iErr = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv)) <= 0)
      {
         if (((iErr == -1) && (errno != EINTR)) ||
             (iErr == 0))
         {
            return FALSE;
         }
      }
      qwTime = GetCurrentTimeMs() - qwTime;  // Elapsed time
      dwTimeout -= min(((UINT32)qwTime), dwTimeout);
   } while(iErr < 0);
#endif

	nBytes = recv(hSocket, (char *)pdu, SNMP_BUFFER_SIZE, 0);
	if (nBytes >= 0)
	{
		*pdwSize = nBytes;
		return TRUE;
	}
	return FALSE;
}

/**
 * Send SNMP request to target, receive response, and send it to server
 */
void CommSession::proxySnmpRequest(NXCPMessage *request)
{
   NXCPMessage response;
   response.setCode(CMD_REQUEST_COMPLETED);
   response.setId(request->getId());

   s_serverRequests++;
	UINT32 dwSizeIn = request->getFieldAsUInt32(VID_PDU_SIZE);
	if (dwSizeIn > 0)
	{
		BYTE *pduIn = (BYTE *)malloc(dwSizeIn);
		if (pduIn != NULL)
		{
			request->getFieldAsBinary(VID_PDU, pduIn, dwSizeIn);

			SOCKET hSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (hSocket != INVALID_SOCKET)
			{
				InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
            SockAddrBuffer sa;
            addr.fillSockAddr(&sa, request->getFieldAsUInt16(VID_PORT));
				if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
				{
					BYTE *pduOut = (BYTE *)malloc(SNMP_BUFFER_SIZE);
					if (pduOut != NULL)
					{
                  int nRetries;
						for(nRetries = 0; nRetries < 3; nRetries++)
						{
							if (send(hSocket, (char *)pduIn, dwSizeIn, 0) == (int)dwSizeIn)
							{
							   s_snmpRequests++;
                        UINT32 dwSizeOut;
								if (ReadPDU(hSocket, pduOut, &dwSizeOut))
								{
								   s_snmpResponses++;
									response.setField(VID_PDU_SIZE, dwSizeOut);
									response.setField(VID_PDU, pduOut, dwSizeOut);
									break;
								}
							}
						}
						free(pduOut);
						response.setField(VID_RCC, (nRetries == 3) ? ERR_REQUEST_TIMEOUT : ERR_SUCCESS);
					}
					else
					{
						response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
					}
				}
				else
				{
					response.setField(VID_RCC, ERR_SOCKET_ERROR);
				}
				closesocket(hSocket);
			}
			else
			{
				response.setField(VID_RCC, ERR_SOCKET_ERROR);
			}
			free(pduIn);
		}
		else
		{
			response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
		}
	}
	else
	{
		response.setField(VID_RCC, ERR_MALFORMED_COMMAND);
	}
	sendMessage(&response);
	delete request;
	decRefCount();
}
