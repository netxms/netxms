/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: snmptrapd.cpp
**
**/

#include "nxcore.h"
#include <nxcore_discovery.h>

#define DEBUG_TAG _T("snmp.trap")

/**
 * Last used trap ID
 */
static VolatileCounter64 s_trapId = 0;

/**
 * SNMP trap
 */
struct SnmpTrap
{
   uint64_t id;
   SNMP_PDU *pdu;
   time_t timestamp;
   InetAddress addr;
   int32_t zoneUIN;
   uint16_t port;
   bool isInformRq;
   StringBuffer varbinds;
   uint32_t nodeId;

   SnmpTrap(SNMP_PDU *_pdu, const InetAddress& _addr, int32_t _zoneUIN, uint16_t _port, bool _isInformRq) : addr(_addr)
   {
      id = InterlockedIncrement64(&s_trapId);
      pdu = _pdu;
      timestamp = time(nullptr);
      zoneUIN = _zoneUIN;
      port = _port;
      isInformRq = _isInformRq;
      nodeId = 0;
   }

   ~SnmpTrap()
   {
      delete pdu;
   }

   void buildVarbindList();
};

/**
 * Total number of received SNMP traps
 */
VolatileCounter64 g_snmpTrapsReceived = 0;

/**
 * Max SNMP packet length
 */
#define MAX_PACKET_LENGTH     65536

/**
 * SNMP trap queues
 */
ObjectQueue<SnmpTrap> g_snmpTrapProcessorQueue(1024, Ownership::False);
ObjectQueue<SnmpTrap> g_snmpTrapWriterQueue(1024, Ownership::False);

/**
 * Get last SNMP Trap id
 */
int64_t GetLastSnmpTrapId()
{
   return s_trapId;
}

/**
 * Generate event for matched trap
 */
static void GenerateTrapEvent(SnmpTrap *trap, const shared_ptr<Node>& node, const SNMPTrapMapping& mapping)
{
   SNMP_PDU *pdu = trap->pdu;

   EventBuilder event(mapping.getEventCode(), trap->nodeId);
   event.origin(EventOrigin::SNMP);
   event.tag(mapping.getEventTag());
   event.param(_T("oid"), pdu->getTrapId().toString());

   // Extract varbinds from trap and add them as event's parameters
   int numMaps = mapping.getParameterMappingCount();
   for(int i = 0; i < numMaps; i++)
   {
      const SNMPTrapParameterMapping *pm = mapping.getParameterMapping(i);
      if (pm->isPositional())
      {
         // Extract by varbind position
         // Position numbering in mapping starts from 1,
         // SNMP v2/v3 trap contains uptime and trap OID at positions 0 and 1,
         // so map first mapping position to index 2 and so on
         int index = (pdu->getVersion() == SNMP_VERSION_1) ? pm->getPosition() - 1 : pm->getPosition() + 1;
         SNMP_Variable *varbind = pdu->getVariable(index);
         if (varbind != nullptr)
         {
            bool convertToHex = true;
            TCHAR name[64], buffer[3072];
            _sntprintf(name, 64, _T("%d"), pm->getPosition());
            event.param(name,
               ((g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION) && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                  varbind->getValueAsPrintableString(buffer, 3072, &convertToHex) :
                  varbind->getValueAsString(buffer, 3072));
         }
      }
      else
      {
         // Extract by varbind OID
         int count = pdu->getNumVariables();
         for(int j = 0; j < count; j++)
         {
            SNMP_Variable *varbind = pdu->getVariable(j);
            int result = varbind->getName().compare(*(pm->getOid()));
            if ((result == OID_EQUAL) || (result == OID_LONGER))
            {
               bool convertToHex = true;
               TCHAR buffer[3072];
               event.param(varbind->getName().toString(),
                  ((g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION) && !(pm->getFlags() & TRAP_VARBIND_FORCE_TEXT)) ?
                     varbind->getValueAsPrintableString(buffer, 3072, &convertToHex) :
                     varbind->getValueAsString(buffer, 3072));
               break;
            }
         }
      }
   }

   event.param(_T("sourcePort"), trap->port);

   NXSL_VM *vm;
   if ((mapping.getScript() != nullptr) && !mapping.getScript()->isEmpty())
   {
      vm = CreateServerScriptVM(mapping.getScript(), node);
      if (vm != nullptr)
      {
         vm->setGlobalVariable("$trap", vm->createValue(pdu->getTrapId().toString()));
         NXSL_Array *varbinds = new NXSL_Array(vm);
         for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2; i < pdu->getNumVariables(); i++)
         {
            varbinds->append(vm->createValue(vm->createObject(&g_nxslSnmpVarBindClass, new SNMP_Variable(*pdu->getVariable(i)))));
         }
         vm->setGlobalVariable("$varbinds", vm->createValue(varbinds));
         event.vm(vm);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("GenerateTrapEvent: cannot load transformation script for trap mapping [%u]"), mapping.getId());
      }
   }
   else
   {
      vm = nullptr;
   }
   event.post();
   delete vm;
}

/**
 * Build trap varbind list
 */
void SnmpTrap::buildVarbindList()
{
   TCHAR oidText[1024], data[4096];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Varbinds for %s %s from %s:"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"), pdu->getTrapId().toString(oidText, 1024), addr.toString(data));
   for(int i = (pdu->getVersion() == SNMP_VERSION_1) ? 0 : 2; i < pdu->getNumVariables(); i++)
   {
      SNMP_Variable *v = pdu->getVariable(i);
      if (!varbinds.isEmpty())
         varbinds.append(_T("; "));

      v->getName().toString(oidText, 1024);

      bool convertToHex = true;
      if (g_flags & AF_ALLOW_TRAP_VARBIND_CONVERSION)
         v->getValueAsPrintableString(data, 4096, &convertToHex);
      else
         v->getValueAsString(data, 4096);

      nxlog_debug_tag(DEBUG_TAG, 5, _T("   %s == '%s'"), oidText, data);

      varbinds.append(oidText);
      varbinds.append(_T(" == '"));
      varbinds.append(data);
      varbinds.append(_T('\''));
   }
}

/**
 * Process trap
 */
static void ProcessTrap(SnmpTrap *trap)
{
   TCHAR buffer[4096];

   // Match IP address to object
   shared_ptr<Node> node = FindNodeByIP(trap->zoneUIN, (g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) != 0, trap->addr);
   if (node != nullptr)
   {
      trap->nodeId = node->getId();
      trap->zoneUIN = node->getZoneUIN();
   }

   SNMP_PDU *pdu = trap->pdu;
   if ((node != nullptr) && (node->getSnmpCodepage()[0] != 0))
   {
      pdu->setCodepage(node->getSnmpCodepage());
   }
   else if (g_snmpCodepage[0] != 0)
   {
      pdu->setCodepage(g_snmpCodepage);
   }

   // Prepare varbind display string
   if ((node != nullptr) || (g_flags & AF_LOG_ALL_SNMP_TRAPS) || (nxlog_get_debug_level_tag(DEBUG_TAG) >= 5))
   {
      trap->buildVarbindList();
   }

   // Process trap if it is coming from host registered in database
   if (node != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SNMP trap from %s matched to node %s [%u]"), trap->addr.toString(buffer), node->getName(), node->getId());
      node->incSnmpTrapCount();
      if (node->checkTrapShouldBeProcessed())
      {
         if ((node->getStatus() != STATUS_UNMANAGED) || (g_flags & AF_TRAPS_FROM_UNMANAGED_NODES))
         {
            // Pass trap to loaded modules
            bool processedByModule = false;
            ENUMERATE_MODULES(pfTrapHandler)
            {
               if (CURRENT_MODULE.pfTrapHandler(pdu, node))
               {
                  processedByModule = true;
                  break;   // Trap was processed by the module
               }
            }

            shared_ptr<SNMPTrapMapping> trapMapping = FindBestMatchTrapMapping(pdu->getTrapId());
            if (trapMapping != nullptr)
            {
               GenerateTrapEvent(trap, node, *trapMapping);
            }
            else if (!processedByModule && (g_flags & AF_ENABLE_UNMATCHED_TRAP_EVENT))    // Process unmatched traps not processed by module
            {
               // Generate default event for unmatched traps
               TCHAR oidText[1024];
               EventBuilder(EVENT_SNMP_UNMATCHED_TRAP, *node)
                  .origin(EventOrigin::SNMP)
                  .param(_T("oid"), pdu->getTrapId().toString(oidText, 1024))
                  .param(_T("varbinds"), trap->varbinds)
                  .param(_T("sourcePort"), trap->port)
                  .post();
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Node %s [%d] is in UNMANAGED state, SNMP trap ignored"), node->getName(), node->getId());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Node %s [%d] is in SNMP trap flood state, trap is dropped"), node->getName(), node->getId());
      }
   }
   else if (g_flags & AF_SNMP_TRAP_DISCOVERY)  // unknown node, discovery enabled
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SNMP trap not matched to node, adding new IP address %s for discovery"), trap->addr.toString(buffer));
      CheckPotentialNode(trap->addr, trap->zoneUIN, DA_SRC_SNMP_TRAP, 0);
   }
   else  // unknown node, discovery disabled
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SNMP trap from %s not matched to any node"), trap->addr.toString(buffer));
   }

   // Queue log write or discard
   if ((node != nullptr) || (g_flags & AF_LOG_ALL_SNMP_TRAPS))
   {
      // Notify connected clients
      TCHAR oidText[1024];
      NXCPMessage msg(CMD_TRAP_LOG_RECORDS, 0);
      msg.setField(VID_NUM_RECORDS, static_cast<uint32_t>(1));
      msg.setField(VID_RECORDS_ORDER, static_cast<uint16_t>(RECORD_ORDER_NORMAL));
      msg.setField(VID_TRAP_LOG_MSG_BASE, trap->id);
      msg.setFieldFromTime(VID_TRAP_LOG_MSG_BASE + 1, trap->timestamp);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 2, trap->addr);
      msg.setField(VID_TRAP_LOG_MSG_BASE + 3, (node != nullptr) ? node->getId() : static_cast<uint32_t>(0));
      msg.setField(VID_TRAP_LOG_MSG_BASE + 4, pdu->getTrapId().toString(oidText, 1024));
      msg.setField(VID_TRAP_LOG_MSG_BASE + 5, trap->varbinds);
      EnumerateClientSessions(
         [node, &msg] (ClientSession *session) -> void
         {
            if (session->isAuthenticated() &&
                (session->getSystemAccessRights() & SYSTEM_ACCESS_VIEW_TRAP_LOG) &&
                session->isSubscribedTo(NXC_CHANNEL_SNMP_TRAPS) &&
                ((node == nullptr) || node->checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ_ALARMS)))
            {
               session->postMessage(msg);
            }
         });

      g_snmpTrapWriterQueue.put(trap);
   }
   else
   {
      delete trap;
   }
}

/**
 * Trap processor thread
 */
static void ProcessorThread()
{
   ThreadSetName("SNMPTrapProc");

   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap processor started"));

   while(true)
   {
      SnmpTrap *trap = g_snmpTrapProcessorQueue.getOrBlock();
      if (trap == INVALID_POINTER_VALUE)
         break;
      ProcessTrap(trap);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap processor stopped"));
}

/**
 * Trap write thread
 */
static void WriterThread()
{
   ThreadSetName("SNMPTrapWrt");

   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap database writer started"));
   int maxRecords = ConfigReadInt(_T("DBWriter.MaxRecordsPerTransaction"), 1000);

   TCHAR ipAddrText[64], oidText[1024];
   while(true)
   {
      SnmpTrap *trap = g_snmpTrapWriterQueue.getOrBlock();
      if (trap == INVALID_POINTER_VALUE)
         break;

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      DB_STATEMENT hStmt = DBPrepare(hdb,
               (g_dbSyntax == DB_SYNTAX_TSDB) ?
                        _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,ip_addr,object_id,zone_uin,trap_oid,trap_varlist) VALUES (?,to_timestamp(?),?,?,?,?,?)") :
                        _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,ip_addr,object_id,zone_uin,trap_oid,trap_varlist) VALUES (?,?,?,?,?,?,?)"), true);
      if (hStmt == nullptr)
      {
         delete trap;
         DBConnectionPoolReleaseConnection(hdb);
         continue;
      }

      int count = 0;
      DBBegin(hdb);
      while(true)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, trap->id);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(trap->timestamp));
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, trap->addr.toString(ipAddrText), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, trap->nodeId);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, trap->zoneUIN);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, trap->pdu->getTrapId().toString(oidText, 1024), DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, trap->varbinds, DB_BIND_STATIC);

         if (!DBExecute(hStmt))
         {
            delete trap;
            break;
         }
         delete trap;
         count++;
         if (count == maxRecords)
            break;
         trap = g_snmpTrapWriterQueue.get();
         if ((trap == nullptr) || (trap == INVALID_POINTER_VALUE))
            break;
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      if (trap == INVALID_POINTER_VALUE)
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap database writer stopped"));
}

/**
 * Enqueue SNMP trap for processing
 */
void EnqueueSNMPTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, int32_t zoneUIN, int srcPort, SNMP_Transport *snmpTransport, SNMP_Engine *localEngine)
{
   InterlockedIncrement64(&g_snmpTrapsReceived);

   bool isInformRq = (pdu->getCommand() == SNMP_INFORM_REQUEST);
   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 4)
   {
      TCHAR ipAddrText[64], oidText[1024];
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Received SNMP %s %s from %s"), isInformRq ? _T("INFORM-REQUEST") : _T("TRAP"),
                pdu->getTrapId().toString(oidText, 1024), srcAddr.toString(ipAddrText));
   }

   if (isInformRq)
   {
      SNMP_PDU response(SNMP_RESPONSE, pdu->getRequestId(), pdu->getVersion());
      if (snmpTransport->getSecurityContext() == nullptr)
      {
         snmpTransport->setSecurityContext(new SNMP_SecurityContext(pdu->getCommunity()));
      }
      response.setMessageId(pdu->getMessageId());
      response.setContextEngineId(localEngine->getId(), localEngine->getIdLen());
      snmpTransport->sendMessage(&response, 0);
   }

   g_snmpTrapProcessorQueue.put(new SnmpTrap(pdu, srcAddr, zoneUIN, srcPort, isInformRq));
}

/**
 * Context finder - tries to find SNMPv3 security context by IP address
 */
static SNMP_SecurityContext *ContextFinder(struct sockaddr *addr, socklen_t addrLen)
{
   InetAddress ipAddr = InetAddress::createFromSockaddr(addr);
   shared_ptr<Node> node = FindNodeByIP((g_flags & AF_TRAP_SOURCES_IN_ALL_ZONES) ? ALL_ZONES : 0, ipAddr);
   TCHAR buffer[64];
   nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: looking for SNMP security context for node %s %s"),
      ipAddr.toString(buffer), (node != nullptr) ? node->getName() : _T("<unknown>"));
   return (node != nullptr) ? node->getSnmpSecurityContext() : nullptr;
}

/**
 * Create SNMP transport for receiver
 */
static SNMP_Transport *CreateTransport(SOCKET hSocket)
{
   if (hSocket == INVALID_SOCKET)
      return nullptr;

   auto t = new SNMP_UDPTransport(hSocket);
   t->enableEngineIdAutoupdate(true);
   t->setPeerUpdatedOnRecv(true);
   t->expandBuffer();
   return t;
}

/**
 * SNMP trap receiver thread
 */
static void ReceiverThread()
{
   ThreadSetName("SNMPTrapRecv");

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

   uint16_t listenerPort = static_cast<uint16_t>(ConfigReadULong(_T("SNMP.Traps.ListenerPort"), 162));

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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
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
   servAddr.sin_port = htons(listenerPort);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(listenerPort);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), listenerPort);
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv4 socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
      bindFailures++;
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }

#ifdef WITH_IPV6
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Trying to bind on UDP [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), listenerPort);
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind IPv6 socket for SNMP trap receiver (%s)"), GetLastSocketErrorText(buffer, 1024));
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
      nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap receiver aborted - cannot bind at least one socket"));
      return;
   }

   if (hSocket != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP traps on UDP socket %s:%u"), InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(ipAddrText), listenerPort);
   }
#ifdef WITH_IPV6
   if (hSocket6 != INVALID_SOCKET)
   {
      TCHAR ipAddrText[64];
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Listening for SNMP traps on UDP socket %s:%u"), InetAddress(servAddr6.sin6_addr.s6_addr).toString(ipAddrText), listenerPort);
   }
#endif

   SNMP_Transport *snmp = CreateTransport(hSocket);
#ifdef WITH_IPV6
   SNMP_Transport *snmp6 = CreateTransport(hSocket6);
#endif

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("SNMP trap receiver started on port %u"), listenerPort);

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
         int bytes = transport->readMessage(&pdu, 2000, (struct sockaddr *)&addr, &addrLen, ContextFinder);
         if ((bytes > 0) && (pdu != nullptr))
         {
            InetAddress sourceAddr = InetAddress::createFromSockaddr((struct sockaddr *)&addr);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: received PDU of type %d from %s"), pdu->getCommand(), (const TCHAR *)sourceAddr.toString());
            if ((pdu->getCommand() == SNMP_TRAP) || (pdu->getCommand() == SNMP_INFORM_REQUEST))
            {
               if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_INFORM_REQUEST))
               {
                  SNMP_SecurityContext *context = transport->getSecurityContext();
                  context->setAuthoritativeEngine(localEngine);
               }
               EnqueueSNMPTrap(pdu, sourceAddr, 0, ntohs(SA_PORT(&addr)), transport, &localEngine);
               pdu = nullptr; // prevent delete (PDU will be deleted by trap processor)
            }
            else if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_GET_REQUEST) && (pdu->getAuthoritativeEngine().getIdLen() == 0))
            {
               // Engine ID discovery
               nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: EngineId discovery"));

               SNMP_PDU *response = new SNMP_PDU(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
               response->setReportable(false);
               response->setMessageId(pdu->getMessageId());
               response->setContextEngineId(localEngine.getId(), localEngine.getIdLen());

               SNMP_Variable *var = new SNMP_Variable({ 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 });
               var->setValueFromUInt32(ASN_INTEGER, 2);
               response->bindVariable(var);

               SNMP_SecurityContext *context = new SNMP_SecurityContext();
               localEngine.setTime(static_cast<uint32_t>(time(nullptr)));
               context->setAuthoritativeEngine(localEngine);
               context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
               context->setAuthMethod(SNMP_AUTH_NONE);
               context->setPrivMethod(SNMP_ENCRYPT_NONE);
               transport->setSecurityContext(context);

               transport->sendMessage(response, 0);
               delete response;
            }
            else if (pdu->getCommand() == SNMP_REPORT)
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMPTrapReceiver: REPORT PDU with error %s"), (const TCHAR *)pdu->getVariable(0)->getName().toString());
            }
            delete pdu;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("SNMPTrapReceiver: error reading PDU from socket (rc=%d, errno=%d, errtext=\"%s\")"), bytes, errno, _tcserror(errno));
            // Sleep on error
            ThreadSleepMs(100);
         }
      }
   }

   delete snmp;
#ifdef WITH_IPV6
   delete snmp6;
#endif
   nxlog_debug_tag(DEBUG_TAG, 1, _T("SNMP trap receiver stopped"));
}

/**
 * Worker threads
 */
static THREAD s_receiverThread = INVALID_THREAD_HANDLE;
static THREAD s_processorThread = INVALID_THREAD_HANDLE;
static THREAD s_writerThread = INVALID_THREAD_HANDLE;

/**
 * Start SNMP trap receiver
 */
void StartSnmpTrapReceiver()
{
   int64_t id = ConfigReadInt64(_T("LastSNMPTrapId"), 0);
   if (id > s_trapId)
      s_trapId = id;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(trap_id) FROM snmp_trap_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_trapId = std::max(DBGetFieldInt64(hResult, 0, 0), static_cast<int64_t>(s_trapId));
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   s_receiverThread = ThreadCreateEx(ReceiverThread);
   s_processorThread = ThreadCreateEx(ProcessorThread);
   s_writerThread = ThreadCreateEx(WriterThread);
}

/**
 * Stop SNMP trap receiver
 */
void StopSnmpTrapReceiver()
{
   g_snmpTrapProcessorQueue.put(INVALID_POINTER_VALUE);
   g_snmpTrapWriterQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_receiverThread);
   ThreadJoin(s_processorThread);
   ThreadJoin(s_writerThread);
}
