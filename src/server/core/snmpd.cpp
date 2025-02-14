/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: snmpd.cpp
**
**/

#include "nxcore.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("snmp.agent")

/**
 * Base class for MIB subtree handler
 */
class MIBSubTree
{
protected:
   SNMP_ObjectId m_baseOID;

public:
   MIBSubTree(const SNMP_ObjectId& baseOID) : m_baseOID(baseOID) {}
   virtual ~MIBSubTree() = default;

   virtual SNMP_Variable *get(const SNMP_ObjectId& oid) = 0;
   virtual SNMP_Variable *getNext(const SNMP_ObjectId& oid) = 0;
};

/**
 * Single-value sub-tree (handles only baseOID.0)
 */
class SingleValueMIBSubTree : public MIBSubTree
{
protected:
   std::function<SNMP_Variable *(const SNMP_ObjectId&)> m_handler;
   SNMP_ObjectId m_elementOID;

public:
   SingleValueMIBSubTree(const SNMP_ObjectId& baseOID, std::function<SNMP_Variable *(const SNMP_ObjectId&)> handler) : MIBSubTree(baseOID), m_handler(handler), m_elementOID(baseOID, 0) {}

   virtual SNMP_Variable *get(const SNMP_ObjectId& oid)
   {
      return (oid.compare(m_elementOID) == OID_EQUAL) ? m_handler(m_elementOID) : nullptr;
   }

   virtual SNMP_Variable *getNext(const SNMP_ObjectId& oid)
   {
      int result = oid.compare(m_baseOID);
      return ((result == OID_PRECEDING) || (result == OID_SHORTER) || (result == OID_EQUAL)) ? get(m_elementOID) : nullptr;
   }
};

/**
 * MIB tree handlers
 */
static ObjectArray<MIBSubTree> s_mib(0, 16, Ownership::True);

/**
 * MIB tree lock
 */
static RWLock s_mibLock;

/**
 * Allowed SNMP versions (bit mask)
 */
static uint32_t s_allowedVersions = 7;

/**
 * Community string
 */
static char s_community[64] = "public";

/**
 * Create SNMP transport for receiver
 */
static SNMP_Transport *CreateTransport(SOCKET hSocket)
{
   if (hSocket == INVALID_SOCKET)
      return nullptr;

   SNMP_Transport *t = new SNMP_UDPTransport(hSocket);
   t->enableEngineIdAutoupdate(true);
   t->setPeerUpdatedOnRecv(true);
   t->setSecurityContext(new SNMP_SecurityContext(s_community));
   return t;
}

/**
 * Check access for incoming PDU
 */
static bool CheckAccess(SNMP_PDU *pdu)
{
   if (pdu->getVersion() < SNMP_VERSION_3)
      return !strcmp(pdu->getCommunity(), s_community);

   return false;
}

/**
 * Check if given SNMP version is allowed by agent
 */
static inline bool IsVersionAllowed(SNMP_Version v)
{
   switch(v)
   {
      case SNMP_VERSION_1:
         return (s_allowedVersions & 1) != 0;
      case SNMP_VERSION_2C:
         return (s_allowedVersions & 2) != 0;
      case SNMP_VERSION_3:
         return (s_allowedVersions & 4) != 0;
      default:
         return false;
   }
}

/**
 * Process incoming request
 */
static void ProcessRequest(SNMP_Transport *transport, SNMP_PDU *request)
{
   SNMP_PDU response(SNMP_RESPONSE, request->getRequestId(), request->getVersion());
   bool getNext = (request->getCommand() == SNMP_GET_NEXT_REQUEST);

   s_mibLock.readLock();
   for(int i = 0; i < request->getNumVariables(); i++)
   {
      SNMP_Variable *vreq = request->getVariable(i);
      SNMP_Variable *vrsp = nullptr;
      for(int j = 0; (j < s_mib.size()) && (vrsp == nullptr); j++)
      {
         MIBSubTree *ms = s_mib.get(j);
         vrsp = getNext ? ms->getNext(vreq->getName()) : ms->get(vreq->getName());
      }
      response.bindVariable((vrsp != nullptr) ? vrsp : new SNMP_Variable(vreq->getName(), getNext ? ASN_END_OF_MIBVIEW : ASN_NO_SUCH_OBJECT));
   }
   s_mibLock.unlock();

   transport->sendMessage(&response, 1000);
}

/**
 * SNMP request receiver thread
 */
static void SNMPAgentReceiver()
{
   ThreadSetName("SNMPD");

   static BYTE defaultEngineId[] = { 0x80, 0x00, 0xDF, 0x4B, 0x05, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
   BYTE engineId[32];
   size_t engineIdLen;
   TCHAR engineIdText[96];
   if (ConfigReadStr(_T("SNMP.EngineId"), engineIdText, 96, _T("")))
   {
      TranslateStr(engineIdText, _T(":"), _T(""));
      engineIdLen = StrToBin(engineIdText, engineId, sizeof(engineId));
      if (engineIdLen < 1)
      {
         memcpy(engineId, defaultEngineId, 12);
         engineIdLen = 12;
      }
   }
   else
   {
      memcpy(engineId, defaultEngineId, 12);
      engineIdLen = 12;
   }
   SNMP_Engine localEngine(engineId, engineIdLen);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Local SNMP engine ID set to %s"), localEngine.toString().cstr());

   uint16_t snmpdPort = static_cast<uint16_t>(ConfigReadULong(_T("SNMP.Agent.ListenerPort"), 161));

   SOCKET hSocket = CreateSocket(AF_INET, SOCK_DGRAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = CreateSocket(AF_INET6, SOCK_DGRAM, 0);
#endif

#ifdef WITH_IPV6
   if ((hSocket == INVALID_SOCKET) && (hSocket6 == INVALID_SOCKET))
#else
   if (hSocket == INVALID_SOCKET)
#endif
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
      return;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#endif

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif
   if (!_tcscmp(g_szListenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
      memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
   }
   else
   {
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
         servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
         servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
         memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
         memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
   }
   servAddr.sin_port = htons(snmpdPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(snmpdPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv4 socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv6 socket for SNMP agent (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to at least one socket
   if (bindFailures == 2)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP agent cannot start - cannot bind at least one socket"));
      return;
   }

   if (hSocket != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP requests on UDP socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), snmpdPort);
   }
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP requests on UDP socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), snmpdPort);
   }
#endif

   SNMP_Transport *snmp = CreateTransport(hSocket);
#ifdef WITH_IPV6
   SNMP_Transport *snmp6 = CreateTransport(hSocket6);
#endif

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("SNMP agent started on port %u"), snmpdPort);

   // Wait for packets
   SocketPoller sp;
   while(!IsShutdownInProgress())
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int rc = sp.poll(1000);
      if ((rc > 0) && !IsShutdownInProgress())
      {
         SockAddrBuffer addr;
         socklen_t addrLen = sizeof(SockAddrBuffer);
         SNMP_PDU *pdu;
#ifdef WITH_IPV6
         SNMP_Transport *transport = sp.isSet(hSocket) ? snmp : snmp6;
#else
         SNMP_Transport *transport = snmp;
#endif
         int bytes = transport->readMessage(&pdu, 2000, (struct sockaddr *)&addr, &addrLen);
         if ((bytes > 0) && (pdu != nullptr))
         {
            InetAddress sourceAddr = InetAddress::createFromSockaddr((struct sockaddr *)&addr);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMP agent received PDU of type %d from %s"), pdu->getCommand(), (const TCHAR *)sourceAddr.toString());

            if (IsVersionAllowed(pdu->getVersion()))
            {
               if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_GET_REQUEST) && (pdu->getAuthoritativeEngine().getIdLen() == 0))
               {
                  // Engine ID discovery
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMP EngineId discovery"));

                  SNMP_PDU *response = new SNMP_PDU(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
                  response->setReportable(false);
                  response->setMessageId(pdu->getMessageId());
                  response->setContextEngineId(localEngine.getId(), localEngine.getIdLen());

                  SNMP_Variable *var = new SNMP_Variable({ 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 });
                  var->setValueFromUInt32(ASN_INTEGER, 2);
                  response->bindVariable(var);

                  SNMP_SecurityContext *context = new SNMP_SecurityContext();
                  localEngine.setTime((int)time(nullptr));
                  context->setAuthoritativeEngine(localEngine);
                  context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
                  context->setAuthMethod(SNMP_AUTH_NONE);
                  context->setPrivMethod(SNMP_ENCRYPT_NONE);
                  transport->setSecurityContext(context);

                  transport->sendMessage(response, 0);
                  delete response;
               }
               else if (((pdu->getCommand() == SNMP_GET_REQUEST) || (pdu->getCommand() == SNMP_GET_NEXT_REQUEST)) && CheckAccess(pdu))
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("Request passed validation"));
                  ProcessRequest(transport, pdu);
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Requested SNMP version is not allowed"));
            }
            delete pdu;
         }
         else
         {
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
   }

   delete snmp;
#ifdef WITH_IPV6
   delete snmp6;
#endif
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP agent stopped"));
}

/**
 * Agent receiver thread
 */
static THREAD s_receiverThread = INVALID_THREAD_HANDLE;

/**
 * SNMP agent object ID
 */
static SNMP_ObjectId s_agentObjectId { 1, 3, 6, 1, 4, 1, 57163, 1, 2 };

/**
 * Start SNMP agent
 */
void StartSNMPAgent()
{
   if (!ConfigReadBoolean(_T("SNMP.Agent.Enable"), false))
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Build-in SNMP agent is disabled"));
      return;
   }

   ConfigReadStrA(_T("SNMP.Agent.Community"), s_community, sizeof(s_community), "public");

   // Setup MIB tree
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.1")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromString(ASN_OCTET_STRING, _T("NetXMS Server ") NETXMS_VERSION_STRING);
         return v;
      }));
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.2")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromObjectId(ASN_OBJECT_ID, s_agentObjectId);
         return v;
      }));
   s_mib.add(new SingleValueMIBSubTree(SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.1.3")),
      [] (const SNMP_ObjectId& oid) -> SNMP_Variable*
      {
         auto v = new SNMP_Variable(oid);
         v->setValueFromUInt32(ASN_TIMETICKS, static_cast<uint32_t>(time(nullptr) - g_serverStartTime) * 10);
         return v;
      }));

   s_receiverThread = ThreadCreateEx(SNMPAgentReceiver);
}

/**
 * Stop SNMP agent
 */
void StopSNMPAgent()
{
   ThreadJoin(s_receiverThread);
}

/**
 * Process configuration changes related to SNMP agent
 */
void OnSNMPAgentConfigurationChange(const wchar_t *name, const wchar_t *value)
{
   if (!wcscmp(name, L"SNMP.Agent.Community"))
   {
      memset(s_community, 0, sizeof(s_community));
      wchar_to_utf8(value, -1, s_community, sizeof(s_community) - 1);
   }
}
