/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
 * SNMP proxy socket pollers
 */
static ObjectArray<BackgroundSocketPollerHandle> s_snmpProxySocketPollers(0, 8, Ownership::True);
static Mutex s_snmpProxySocketPollersLock(MutexType::FAST);

/**
 * SNMP proxy stats
 */
static VolatileCounter64 s_serverRequests = 0;
static VolatileCounter64 s_snmpRequests = 0;
static VolatileCounter64 s_snmpResponses = 0;
extern uint64_t g_snmpTraps;

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
      case 'T':   // SNMP traps
         ret_uint64(value, g_snmpTraps);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Callback for address range scan
 */
static void RangeScanCallback(const InetAddress& addr, uint32_t rtt, void *context)
{
   TCHAR buffer[64];
   static_cast<StringList*>(context)->add(addr.toString(buffer));
}

/**
 * Handler for list SNMP.ScanAddressRange
 */
LONG H_SNMPAddressRangeScan(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   if (!session->isMasterServer() || !(g_dwFlags & AF_ENABLE_SNMP_PROXY))
   {
      session->debugPrintf(5, _T("Request for address range scan via SNMP rejected"));
      return SYSINFO_RC_UNSUPPORTED;
   }

   char startAddr[128], endAddr[128], community[256];
   TCHAR portText[64], versionText[64];
   if (!AgentGetParameterArgA(cmd, 1, startAddr, 128) ||
       !AgentGetParameterArgA(cmd, 2, endAddr, 128) ||
       !AgentGetParameterArg(cmd, 3, portText, 64) ||
       !AgentGetParameterArg(cmd, 4, versionText, 64) ||
       !AgentGetParameterArgA(cmd, 5, community, 256))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   InetAddress start = InetAddress::parse(startAddr);
   InetAddress end = InetAddress::parse(endAddr);
   uint16_t port = (portText[0] != 0) ? static_cast<uint16_t>(_tcstoul(portText, nullptr, 0)) : 161;
   SNMP_Version snmpVersion = (versionText[0] != 0) ? SNMP_VersionFromInt(_tcstol(versionText, nullptr, 10)) : SNMP_VERSION_2C;
   if (!start.isValid() || !end.isValid() || (port == 0) || (snmpVersion == SNMP_VERSION_DEFAULT))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   if (community[0] == 0)
      strcpy(community, "public");

   uint32_t rc = SnmpScanAddressRange(start, end, port, snmpVersion, community, RangeScanCallback, value);
   session->debugPrintf(5, _T("Address range %s - %s scan via SNMP %s"), start.toString().cstr(), end.toString().cstr(), (rc == SNMP_ERR_SUCCESS) ? _T("completed successfully") : _T("failed"));
   return (rc == SNMP_ERR_SUCCESS) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Read PDU from network
 */
static bool ReadPDU(SOCKET hSocket, BYTE *pdu, uint32_t *size)
{
	int bytes = recv(hSocket, (char *)pdu, SNMP_BUFFER_SIZE, 0);
	if (bytes >= 0)
	{
		*size = bytes;
		return true;
	}
	return false;
}

/**
 * Proxy context
 */
struct ProxyContext
{
   BackgroundSocketPollerHandle *pollerHandle;
   InetAddress addr;
   shared_ptr<CommSession> session;
   NXCPMessage *request;
   const BYTE *pduIn;
   size_t sizeIn;
   int retries;
   uint32_t timeout;
   BYTE response[SNMP_BUFFER_SIZE];

   ProxyContext(const InetAddress& _addr, const shared_ptr<CommSession>& _session, NXCPMessage *_request, uint32_t _timeout) : addr(_addr), session(_session)
   {
      pollerHandle = nullptr;
      session = _session;
      request = _request;
      pduIn = request->getBinaryFieldPtr(VID_PDU, &sizeIn);
      retries = 3;
      timeout = _timeout;
   }
   ~ProxyContext()
   {
      delete request;
   }
};

/**
 * Callback for background poller
 */
static void SocketPollerCallback(BackgroundSocketPollResult result, SOCKET hSocket, ProxyContext *context)
{
   if (result == BackgroundSocketPollResult::TIMEOUT)
   {
      context->retries--;
      if (context->retries > 0)
      {
         if (send(hSocket, (char *)context->pduIn, (int)context->sizeIn, 0) == (int)context->sizeIn)
         {
            InterlockedIncrement64(&s_snmpRequests);
            context->pollerHandle->poller.poll(hSocket, context->timeout, SocketPollerCallback, context);
            return;
         }
      }
   }

   InterlockedDecrement(&context->pollerHandle->usageCount);

   NXCPMessage response(CMD_REQUEST_COMPLETED, context->request->getId(), context->session->getProtocolVersion());
   if (result == BackgroundSocketPollResult::SUCCESS)
   {
      uint32_t responseSize;
      if (ReadPDU(hSocket, context->response, &responseSize))
      {
         InterlockedIncrement64(&s_snmpResponses);
         response.setField(VID_PDU_SIZE, responseSize);
         response.setField(VID_PDU, context->response, responseSize);
         response.setField(VID_RCC, ERR_SUCCESS);
         context->session->debugPrintf(7, _T("proxySnmpRequest(%u, %s): success (%d retries left)"),
                  context->request->getId(), context->addr.toString().cstr(), context->retries);
      }
      else
      {
         TCHAR buffer[1024];
         context->session->debugPrintf(7, _T("proxySnmpRequest(%u, %s): read failure (%d: %s)"),
                  context->request->getId(), context->addr.toString().cstr(), WSAGetLastError(), GetLastSocketErrorText(buffer, 1024));
         response.setField(VID_RCC, ERR_SOCKET_ERROR);
      }
   }
   else if (result == BackgroundSocketPollResult::TIMEOUT)
   {
      response.setField(VID_RCC, ERR_REQUEST_TIMEOUT);
      context->session->debugPrintf(7, _T("proxySnmpRequest(%u, %s): timeout"), context->request->getId(), context->addr.toString().cstr());
   }
   else
   {
      response.setField(VID_RCC, ERR_SOCKET_ERROR);
      context->session->debugPrintf(7, _T("proxySnmpRequest(%u, %s): socket error"), context->request->getId(), context->addr.toString().cstr());
   }

   context->session->postMessage(&response);
   delete context;
   closesocket(hSocket);
}

/**
 * Select poller and start background poll
 */
static inline void StartBackgroundPoll(SOCKET hSocket, ProxyContext *context)
{
   s_snmpProxySocketPollersLock.lock();
   BackgroundSocketPollerHandle *sp = nullptr;
   for(int i = 0; i < s_snmpProxySocketPollers.size(); i++)
   {
      BackgroundSocketPollerHandle *p = s_snmpProxySocketPollers.get(i);
      if (InterlockedIncrement(&p->usageCount) < SOCKET_POLLER_MAX_SOCKETS)
      {
         sp = p;
         break;
      }
      InterlockedDecrement(&p->usageCount);
   }
   if (sp == nullptr)
   {
      sp = new BackgroundSocketPollerHandle();
      sp->usageCount = 1;
      s_snmpProxySocketPollers.add(sp);
   }
   s_snmpProxySocketPollersLock.unlock();
   context->pollerHandle = sp;
   sp->poller.poll(hSocket, context->timeout, SocketPollerCallback, context);
}

/**
 * Send SNMP request to target, receive response, and send it to server
 */
void CommSession::proxySnmpRequest(NXCPMessage *request)
{
   uint32_t requestId = request->getId();
   uint32_t rcc = ERR_SUCCESS;

   InterlockedIncrement64(&s_serverRequests);

   size_t sizeIn;
   const BYTE *pduIn = request->getBinaryFieldPtr(VID_PDU, &sizeIn);
   if ((pduIn != nullptr) && (sizeIn > 0))
   {
      InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
      SOCKET hSocket = CreateSocket(addr.getFamily(), SOCK_DGRAM, 0);
      if (hSocket != INVALID_SOCKET)
      {
         SockAddrBuffer sa;
         addr.fillSockAddr(&sa, request->getFieldAsUInt16(VID_PORT));
         if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
         {
            uint32_t serverTimeout = request->getFieldAsUInt32(VID_TIMEOUT);
            uint32_t timeout = (g_snmpTimeout != 0) ? g_snmpTimeout : ((serverTimeout != 0) ? serverTimeout : 1000);   // 1 second if not set
            auto context = new ProxyContext(addr, self(), request, timeout);
            if (context != nullptr)
            {
               if (send(hSocket, (char *)pduIn, (int)sizeIn, 0) == (int)sizeIn)
               {
                  InterlockedIncrement64(&s_snmpRequests);
                  StartBackgroundPoll(hSocket, context);
                  request = nullptr;   // Prevent destruction
                  hSocket = INVALID_SOCKET;  // Prevent close
               }
               else
               {
                  rcc = ERR_SOCKET_ERROR;
                  debugPrintf(7, _T("proxySnmpRequest(%d, %s): initial send failure"), requestId, addr.toString().cstr());
               }
            }
            else
            {
               rcc = ERR_OUT_OF_RESOURCES;
               debugPrintf(7, _T("proxySnmpRequest(%d, %s): memory allocation failure"), requestId, addr.toString().cstr());
            }
         }
         else
         {
            int error = WSAGetLastError();
            TCHAR buffer[1024];
            debugPrintf(7, _T("proxySnmpRequest(%d, %s): connect() call failed (%d: %s)"), requestId, addr.toString().cstr(), error, GetLastSocketErrorText(buffer, 1024));
            rcc = ERR_SOCKET_ERROR;
         }
         if (hSocket != INVALID_SOCKET)
            closesocket(hSocket);
      }
      else
      {
         rcc =  ERR_SOCKET_ERROR;
         debugPrintf(7, _T("proxySnmpRequest(%d, %s): socket() call failed (%d)"), requestId, addr.toString().cstr(), WSAGetLastError());
      }
   }
   else
   {
      rcc = ERR_MALFORMED_COMMAND;
      debugPrintf(7, _T("proxySnmpRequest(%d): input PDU is missing or empty"), requestId);
   }

   if (rcc != ERR_SUCCESS)
   {
      NXCPMessage response(CMD_REQUEST_COMPLETED, requestId, m_protocolVersion);
      response.setField(VID_RCC, rcc);
      sendMessage(&response);
   }

	delete request;
}
