/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
static bool ReadPDU(SOCKET hSocket, BYTE *pdu, UINT32 *pdwSize)
{
	SocketPoller sp;
	sp.add(hSocket);
	if (sp.poll(g_snmpTimeout) <= 0)
	   return false;

	int bytes = recv(hSocket, (char *)pdu, SNMP_BUFFER_SIZE, 0);
	if (bytes >= 0)
	{
		*pdwSize = bytes;
		return true;
	}
	return false;
}

/**
 * Send SNMP request to target, receive response, and send it to server
 */
void CommSession::proxySnmpRequest(NXCPMessage *request)
{
   UINT32 requestId = request->getId();
   NXCPMessage response(CMD_REQUEST_COMPLETED, requestId);

   s_serverRequests++;
   size_t sizeIn;
   const BYTE *pduIn = request->getBinaryFieldPtr(VID_PDU, &sizeIn);
   if ((pduIn != NULL) && (sizeIn > 0))
   {
      InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
      SOCKET hSocket = socket(addr.getFamily(), SOCK_DGRAM, 0);
      if (hSocket != INVALID_SOCKET)
      {
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
                  if (send(hSocket, (char *)pduIn, (int)sizeIn, 0) == (int)sizeIn)
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
                     else
                     {
                        debugPrintf(7, _T("proxySnmpRequest(%d): read failure or timeout (%d)"), requestId, WSAGetLastError());
                     }
                  }
                  else
                  {
                     debugPrintf(7, _T("proxySnmpRequest(%d): send() call failed (%d)"), requestId, WSAGetLastError());
                  }
               }
               free(pduOut);
               response.setField(VID_RCC, (nRetries == 3) ? ERR_REQUEST_TIMEOUT : ERR_SUCCESS);
               debugPrintf(7, _T("proxySnmpRequest(%d): %s (%d retries)"), requestId, (nRetries == 3) ? _T("failure") : _T("success"), nRetries);
            }
            else
            {
               response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
               debugPrintf(7, _T("proxySnmpRequest(%d): memory allocation failure"), requestId);
            }
         }
         else
         {
            response.setField(VID_RCC, ERR_SOCKET_ERROR);
            debugPrintf(7, _T("proxySnmpRequest(%d): connect() call failed (%d)"), requestId, WSAGetLastError());
         }
         closesocket(hSocket);
      }
      else
      {
         response.setField(VID_RCC, ERR_SOCKET_ERROR);
         debugPrintf(7, _T("proxySnmpRequest(%d): socket() call failed (%d)"), requestId, WSAGetLastError());
      }
   }
   else
   {
      response.setField(VID_RCC, ERR_MALFORMED_COMMAND);
      debugPrintf(7, _T("proxySnmpRequest(%d): input PDU is missing or empty"), requestId);
   }
	sendMessage(&response);
	delete request;
	decRefCount();
}
