/*
** NetXMS multiplatform core agent
** Copyright (C) 2014 Raden Solutions
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
** File: snmptrapproxy.cpp
**
**/

#include "nxagentd.h"

struct UdpMessage
{
   UINT32 ipAddr;
   short port;
   BYTE* rawMessage;
   int lenght;
};

/**
 * Static data
 */
static Queue *SNMPTrapQueue = NULL;

/**
 * Shutdown trap sender
 */
void ShutdownSNMPTrapSender()
{
	SNMPTrapQueue->SetShutdownMode();
}

/**
 * SNMP trap read thread
 */
THREAD_RESULT THREAD_CALL SNMPTrapReciever(void *pArg)
{
   SOCKET hSocket = (g_dwFlags & AF_DISABLE_IPV4) ? INVALID_SOCKET : socket(AF_INET, SOCK_DGRAM, 0);
   if ((hSocket == INVALID_SOCKET) && !(g_dwFlags & AF_DISABLE_IPV4))
   {
      DebugPrintf(INVALID_INDEX, 1, _T("SNMPTrapRead(): Socket was not opened"));
      return THREAD_OK;
   }

   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
      SetSocketExclusiveAddrUse(hSocket);
      SetSocketReuseFlag(hSocket);
   }

   // Fill in local address structure
   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;

   //rework
   addr.sin_addr.s_addr = ResolveHostName(g_szSNMPTrapListenAddress);
   if (!_tcscmp(g_szSNMPTrapListenAddress, _T("*")))
	{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
      InetAddress bindAddress = InetAddress::resolveHostName(g_szSNMPTrapListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
		   addr.sin_addr.s_addr = bindAddress.getAddressV4();
      }
      else
      {
   		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
	}
   addr.sin_port = htons(g_dwSNMPTrapPort);

   // Bind socket
   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
      if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
      {
         TCHAR *error = WideStringFromUTF8String(strerror(errno));
         DebugPrintf(INVALID_INDEX, 1, _T("SNMPTrapRead(): Socket error on binding %s"), error);
         safe_free(error);

         closesocket(hSocket);
         return THREAD_OK;
      }
   }
   else
   {
      return THREAD_OK;
   }

   DebugPrintf(INVALID_INDEX, 1, _T("Start message listenning for SNMP traps"));

   SNMP_TrapProxyTransport *pTransport; //rewrite class to support

   if (!(g_dwFlags & AF_DISABLE_IPV4))
   {
      pTransport = new SNMP_TrapProxyTransport(hSocket);
      pTransport->enableEngineIdAutoupdate(true);
      pTransport->setPeerUpdatedOnRecv(true);
      DebugPrintf(INVALID_INDEX, 1, _T("SNMP Trap Receiver started on port %d, IPv4"), g_dwSNMPTrapPort);
   }

   BYTE *rawMessage = NULL;
   int iBytes = 0;
   socklen_t nAddrLen = sizeof(struct sockaddr_in);

   // Wait for packets
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      rawMessage = NULL;
      iBytes = 0;
      iBytes = pTransport->readMessage(&rawMessage, 2000, (struct sockaddr *)&addr, &nAddrLen);
      if ((iBytes > 0) && (rawMessage != NULL))
      {
         UdpMessage *message = new UdpMessage();
         message->ipAddr = ntohl(addr.sin_addr.s_addr);
         message->port = (short)g_dwSNMPTrapPort;
         message->lenght = iBytes;
         message->rawMessage = rawMessage;
         SNMPTrapQueue->Put(message);
         DebugPrintf(INVALID_INDEX, 1, _T("Got trap and put it in the sending que."));
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   delete pTransport;
   DebugPrintf(INVALID_INDEX, 1, _T("SNMP Trap Receiver terminated"));
   return THREAD_OK;
}

/**
 * SNMP trap read thread
 */
THREAD_RESULT THREAD_CALL SNMPTrapSender(void *pArg)
{
   SNMPTrapQueue = new Queue;
   while(1)
   {
      DebugPrintf(INVALID_INDEX, 1, _T("Started SNMP Trap sender thread & wait for message."));
      UdpMessage *pdu = (UdpMessage *)SNMPTrapQueue->GetOrBlock();
      if (pdu == INVALID_POINTER_VALUE)
      {
         DebugPrintf(INVALID_INDEX, 1, _T("Send SNMP thead stoped by que."));
         break;
      }
      bool sent = false;

      CSCPMessage *msg = new CSCPMessage();
      msg->SetCode(CMD_SNMP_TRAP);
      msg->SetId(GetNewMessageID());
      msg->SetVariable(VID_IP_ADDRESS, pdu->ipAddr);
      msg->SetVariable(VID_PORT, pdu->port);
      msg->SetVariable(VID_PDU_SIZE, pdu->lenght);
      msg->SetVariable(VID_PDU, pdu->rawMessage, pdu->lenght);

      if (g_dwFlags & AF_SUBAGENT_LOADER)
      {
         sent = SendRawMessageToMasterAgent(msg->createMessage());
      }
      else
      {
         MutexLock(g_hSessionListAccess);
         for(int i = 0; i < g_dwMaxSessions; i++)
         {
            if (g_pSessionList[i] != NULL)
            {
               if (g_pSessionList[i]->canAcceptTraps())
               {
                  g_pSessionList[i]->sendRawMessage(msg->createMessage());
                  sent = true;
               }
            }
         }
         MutexUnlock(g_hSessionListAccess);
      }

      free(msg);
      safe_free(pdu->rawMessage);
      if(!sent)
      {
         DebugPrintf(INVALID_INDEX, 1, _T("Could not send forward trap to server"));
         SNMPTrapQueue->Put(pdu);
			ThreadSleep(1);
      }
      else
      {
         DebugPrintf(INVALID_INDEX, 1, _T("Trap sucesfully forwarded to server"));
         safe_free(pdu);
      }
   }
   delete SNMPTrapQueue;
   SNMPTrapQueue = NULL;
	DebugPrintf(INVALID_INDEX, 1, _T("SNMP Trap sender thread terminated"));
   return THREAD_OK;
}

/** Implementation of class SNMP_TrapProxyTransport **/

/**
 * Constructor
 */
SNMP_TrapProxyTransport::SNMP_TrapProxyTransport(SOCKET hSocket) : SNMP_UDPTransport(hSocket)
{
}

/**
 * Read PDU from socket but do not decode and parse it
 */
int SNMP_TrapProxyTransport::readMessage(BYTE **rawData, UINT32 dwTimeout,
                                   struct sockaddr *pSender, socklen_t *piAddrSize)
{
   int bytes;
   size_t pduLength;

   if (m_dwBytesInBuffer < 2)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   pduLength = preParsePDU();
   if (pduLength == 0)
   {
      // Clear buffer
      clearBuffer();
      return 0;
   }

   // Move existing data to the beginning of buffer if there are not enough space at the end
   if (pduLength > m_dwBufferSize - m_dwBufferPos)
   {
      memmove(m_pBuffer, &m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer);
      m_dwBufferPos = 0;
   }

   // Read entire PDU into buffer
   while(m_dwBytesInBuffer < pduLength)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   *rawData = (BYTE*)malloc(pduLength);
   memcpy(*rawData, &m_pBuffer[m_dwBufferPos], pduLength);

   m_dwBytesInBuffer -= pduLength;
   if (m_dwBytesInBuffer == 0)
      m_dwBufferPos = 0;

   return (int)pduLength;
}
