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
** File: snmp.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG_SNMP_DISCOVERY _T("snmp.discovery")
#define DEBUG_TAG_SNMP_ROUTES    _T("snmp.routes")

/**
 * MIB compilation mutex
 */
Mutex m_mibCompilationMutex;

/**
 * Extract IPv4 address encoded as OID elements at given offset
 */
static inline uint32_t IPv4AddressFromOID(const SNMP_ObjectId& oid, uint32_t offset)
{
   if (offset > oid.length() - 4)
      return 0;
   const uint32_t *v = oid.value();
   return (v[offset] << 24) | (v[offset + 1] << 16) | (v[offset + 2] << 8) | v[offset + 3];
}

/**
 * Handler for route enumeration via ipRouteTable
 */
static uint32_t HandlerIPRouteTable(SNMP_Variable *varbind, SNMP_Transport *snmpTransport, std::pair<RoutingTable*, size_t> *context)
{
   RoutingTable *routingTable = context->first;
   size_t limit = context->second;
   if ((limit != 0) && (routingTable->size() >= limit))
      return SNMP_ERR_ABORTED;

   SNMP_ObjectId oid(varbind->getName());

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpTransport->getSnmpVersion());

   oid.changeElement(9, 2);  // Interface index
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 7);  // Next hop
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 8);  // Route type
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 3);  // Metric
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 9);  // Protocol
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(9, 1);  // ipRouteDest
   request.bindVariable(new SNMP_Variable(oid));

   uint32_t dwResult;
   SNMP_PDU *response;
   if ((dwResult = snmpTransport->doRequest(&request, &response)) != SNMP_ERR_SUCCESS)
      return dwResult;

   if (response->getNumVariables() != request.getNumVariables())
   {
      delete response;
      return SNMP_ERR_NO_OBJECT;
   }

   ROUTE route;
   route.destination = InetAddress(ntohl(response->getVariable(5)->getValueAsUInt()), ntohl(varbind->getValueAsUInt()));
   route.ifIndex = response->getVariable(0)->getValueAsUInt();
   route.nextHop = InetAddress(ntohl(response->getVariable(1)->getValueAsUInt()));
   route.routeType = response->getVariable(2)->getValueAsUInt();
   int metric = response->getVariable(3)->getValueAsInt();
   route.metric = (metric >= 0) ? metric : 0;
   route.protocol = static_cast<RoutingProtocol>(response->getVariable(4)->getValueAsInt());
   routingTable->add(route);
   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for route enumeration via ipForwardTable
 */
static uint32_t HandlerIPForwardTable(SNMP_Variable *varbind, SNMP_Transport *snmpTransport, std::pair<RoutingTable*, size_t> *context)
{
   RoutingTable *routingTable = context->first;
   size_t limit = context->second;
   if ((limit != 0) && (routingTable->size() >= limit))
      return SNMP_ERR_ABORTED;

   SNMP_ObjectId oid(varbind->getName());

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmpTransport->getSnmpVersion());

   oid.changeElement(10, 5);  // ipForwardIfIndex
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 6);  // ipForwardType
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 11);  // ipForwardMetric1
   request.bindVariable(new SNMP_Variable(oid));

   uint32_t dwResult;
   SNMP_PDU *response;
   if ((dwResult = snmpTransport->doRequest(&request, &response)) != SNMP_ERR_SUCCESS)
      return dwResult;

   if (response->getNumVariables() != request.getNumVariables())
   {
      delete response;
      return SNMP_ERR_NO_OBJECT;
   }

   ROUTE route;
   route.destination = InetAddress(IPv4AddressFromOID(oid, 11), ntohl(varbind->getValueAsUInt()));
   route.ifIndex = response->getVariable(0)->getValueAsUInt();
   route.nextHop = InetAddress(IPv4AddressFromOID(oid, 17));
   route.routeType = response->getVariable(1)->getValueAsUInt();
   int metric = response->getVariable(2)->getValueAsInt();
   route.metric = (metric >= 0) ? metric : 0;
   route.protocol = static_cast<RoutingProtocol>(oid.getElement(15));
   routingTable->add(route);
   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for route enumeration via ipCidrRouteTable
 */
static EnumerationCallbackResult HandlerIPCidrRouteTable(const SNMP_Variable *varbind, const SNMP_Snapshot *snapshot, void *routingTable)
{
   SNMP_ObjectId oid(varbind->getName());

   ROUTE route;
   route.destination = InetAddress(IPv4AddressFromOID(oid, 11), IPv4AddressFromOID(oid, 15));
   route.nextHop = InetAddress(IPv4AddressFromOID(oid, 20));
   route.protocol = static_cast<RoutingProtocol>(varbind->getValueAsInt());

   oid.changeElement(10, 5);  // ipCidrRouteIfIndex
   route.ifIndex = snapshot->getAsUInt32(oid);

   oid.changeElement(10, 11);  // ipCidrRouteMetric1
   int metric = snapshot->getAsInt32(oid);
   route.metric = (metric >= 0) ? metric : 0;

   oid.changeElement(10, 6);  // ipCidrRouteType
   route.routeType = snapshot->getAsUInt32(oid);

   static_cast<RoutingTable*>(routingTable)->add(route);
   return _CONTINUE;
}

/**
 * Handler for route enumeration via ipCidrRouteTable
 */
static EnumerationCallbackResult HandlerInetCidrRouteTable(const SNMP_Variable *varbind, const SNMP_Snapshot *snapshot, void *routingTable)
{
   SNMP_ObjectId oid(varbind->getName());

   ROUTE route;
   int shift;
   route.destination = InetAddressFromOID(oid.value() + 11, true, &shift);
   route.nextHop = InetAddressFromOID(oid.value() + shift + oid.getElement(11 + shift) + 12, false, nullptr); // oid[11 + shift] contains length of policy element
   route.protocol = static_cast<RoutingProtocol>(varbind->getValueAsInt());

   oid.changeElement(10, 7);  // inetCidrRouteIfIndex
   route.ifIndex = snapshot->getAsUInt32(oid);

   oid.changeElement(10, 12);  // inetCidrRouteMetric1
   int metric = snapshot->getAsInt32(oid);
   route.metric = (metric >= 0) ? metric : 0;

   oid.changeElement(10, 8);  // inetCidrRouteType
   route.routeType = snapshot->getAsUInt32(oid);

   static_cast<RoutingTable*>(routingTable)->add(route);
   return _CONTINUE;
}

/**
 * Get routing table via SNMP
 */
shared_ptr<RoutingTable> SnmpGetRoutingTable(SNMP_Transport *snmp, const Node& node)
{
   shared_ptr<RoutingTable> routingTable = make_shared<RoutingTable>();

   size_t limit = node.getCustomAttributeAsInt32(L"SysConfig:Topology.RoutingTable.MaxSize" , ConfigReadInt(L"Topology.RoutingTable.MaxSize", 4000));

   // Use snapshots because some devices does not respond to GET requests to individual table entries
   // Attempt to use inetCidrRouteTable first
   bool success = false;
   SNMP_Snapshot *snapshot = SNMP_Snapshot::create(snmp, { 1, 3, 6, 1, 2, 1, 4, 24, 7, 1 }, limit * 20); // Each table entry is about 20 OIDs
   if (snapshot != nullptr)
   {
      success = (snapshot->walk({ 1, 3, 6, 1, 2, 1, 4, 24, 7, 1, 9 }, HandlerInetCidrRouteTable, routingTable.get()) == _CONTINUE);
      delete snapshot;
      if (success)
         nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): %d routes retrieved from inetCidrRouteTable"), node.getName(), node.getId(), routingTable->size());
   }

   // If not successful, try ipCidrRouteTable
   if (!success || routingTable->isEmpty())
   {
      snapshot = SNMP_Snapshot::create(snmp, { 1, 3, 6, 1, 2, 1, 4, 24, 4, 1 }, limit * 20); // Each table entry is about 20 OIDs
      if (snapshot != nullptr)
      {
         success = (snapshot->walk({ 1, 3, 6, 1, 2, 1, 4, 24, 4, 1, 7 }, HandlerIPCidrRouteTable, routingTable.get()) == _CONTINUE);
         delete snapshot;
         if (success)
            nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): %d routes retrieved from ipCidrRouteTable"), node.getName(), node.getId(), routingTable->size());
      }
   }

   // If not successful, try ipForwardTable
   if (!success || routingTable->isEmpty())
   {
      std::pair<RoutingTable*, size_t> context(routingTable.get(), limit);
      success = (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 24, 2, 1, 2 }, HandlerIPForwardTable, &context, false, true) == SNMP_ERR_SUCCESS);
      if (success)
         nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): %d routes retrieved from ipForwardTable"), node.getName(), node.getId(), routingTable->size());
   }

   // If not successful, try ipRouteTable
   if (!success || routingTable->isEmpty())
   {
      std::pair<RoutingTable*, size_t> context(routingTable.get(), limit);
      success = (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 21, 1, 11 }, HandlerIPRouteTable, &context, false, true) == SNMP_ERR_SUCCESS);
      if (success)
         nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): %d routes retrieved from ipRouteTable"), node.getName(), node.getId(), routingTable->size());
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): %d routes retrieved"), node.getName(), node.getId(), routingTable->size());
   }
   else
   {
      routingTable.reset();
      nxlog_debug_tag(DEBUG_TAG_SNMP_ROUTES, 5, _T("SnmpGetRoutingTable(%s [%u]): failed to get routing table via SNMP"), node.getName(), node.getId());
   }
   return routingTable;
}

/**
 * Send request and check if we get any response
 */
bool SnmpTestRequest(SNMP_Transport *snmp, const StringList &testOids, bool separateRequests)
{
   bool success = false;
   if (separateRequests)
   {
      for(int i = 0; !success && (i < testOids.size()); i++)
      {
         SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
         request.bindVariable(new SNMP_Variable(testOids.get(i)));
         SNMP_PDU *response;
         uint32_t rc = snmp->doRequest(&request, &response);
         if (rc == SNMP_ERR_SUCCESS)
         {
            success = (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) || (response->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME);
            delete response;
         }
      }
   }
   else
   {
      SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
      for(int i = 0; i < testOids.size(); i++)
         request.bindVariable(new SNMP_Variable(testOids.get(i)));
      SNMP_PDU *response;
      uint32_t rc = snmp->doRequest(&request, &response);
      if (rc == SNMP_ERR_SUCCESS)
      {
         success = (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) || (response->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME);
         delete response;
      }
   }
   return success;
}

/**
 * Get list of known SNMP communities for given zone
 */
unique_ptr<StringList> SnmpGetKnownCommunities(int32_t zoneUIN)
{
   auto communities = new StringList();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT community FROM snmp_communities WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         wchar_t community[MAX_DB_STRING];
         for(int i = 0; i < count; i++)
            communities->add(DBGetField(hResult, i, 0, community, MAX_DB_STRING));
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return unique_ptr<StringList>(communities);
}

/**
 * Check SNMP v3 connectivity
 */
static bool SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext,
         const StringList &testOids, const TCHAR *id, int32_t zoneUIN, bool separateRequests)
{
   pTransport->setSnmpVersion(SNMP_VERSION_3);

	// Check original SNMP V3 settings, if set
	if ((originalContext != nullptr) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, originalContext->getUserName(),
		          originalContext->getAuthMethod(), originalContext->getPrivMethod());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
		if (SnmpTestRequest(pTransport, testOids, separateRequests))
		{
         nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): success"), id);
         return true;
		}
	}

	// Try pre-configured SNMP v3 USM credentials
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         ObjectArray<SNMP_SecurityContext> contexts(count);
         for(int i = 0; i < count; i++)
         {
            char name[MAX_DB_STRING], authPasswd[MAX_DB_STRING], privPasswd[MAX_DB_STRING];
            DBGetFieldA(hResult, i, 0, name, MAX_DB_STRING);
            DBGetFieldA(hResult, i, 3, authPasswd, MAX_DB_STRING);
            DBGetFieldA(hResult, i, 4, privPasswd, MAX_DB_STRING);
            contexts.add(new SNMP_SecurityContext(name, authPasswd, privPasswd,
                     static_cast<SNMP_AuthMethod>(DBGetFieldLong(hResult, i, 1)),
                     static_cast<SNMP_EncryptionMethod>(DBGetFieldLong(hResult, i, 2))));
         }

         DBFreeResult(hResult);
         DBFreeStatement(hStmt);
         DBConnectionPoolReleaseConnection(hdb);

         bool found = false;
         for(int i = 0; (i < contexts.size()) && !found && !IsShutdownInProgress(); i++)
         {
            SNMP_SecurityContext *ctx = contexts.get(i);
            pTransport->setSecurityContext(ctx);
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, ctx->getUserName(), ctx->getAuthMethod(), ctx->getPrivMethod());
            if (SnmpTestRequest(pTransport, testOids, separateRequests))
            {
               nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): success"), id);
               found = true;

               // Delete unused contexts
               for(int j = i + 1; j < contexts.size(); j++)
                  delete contexts.get(j);

               break;
            }
         }

         if (found)
            return true;
      }
      else
      {
         DBFreeStatement(hStmt);
         DBConnectionPoolReleaseConnection(hdb);
         nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 3, _T("SnmpCheckV3CommSettings(%s): DBSelect() failed"), id);
      }
   }
   else
   {
      DBConnectionPoolReleaseConnection(hdb);
      nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 3, _T("SnmpCheckV3CommSettings(%s): DBPrepare() failed"), id);
   }

	nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): failed"), id);
	return false;
}

/**
 * Determine SNMP parameters for node
 * On success, returns new security context object (dynamically created).
 * On failure, returns NULL
 */
SNMP_Transport *SnmpCheckCommSettings(uint32_t snmpProxy, const InetAddress& ipAddr, SNMP_Version *version, uint16_t originalPort,
         SNMP_SecurityContext *originalContext, const StringList &testOids, int32_t zoneUIN, bool initialDiscovery)
{
   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): starting check (proxy=%d, originalPort=%d)"), ipAddr.toString(ipAddrText), snmpProxy, (int)originalPort);

   SNMP_Transport *pTransport = nullptr;
   unique_ptr<StringList> communities;
   bool separateRequests = ConfigReadBoolean(_T("SNMP.Discovery.SeparateProbeRequests"), false);

   IntegerArray<uint16_t> ports = GetWellKnownPorts(_T("snmp"), zoneUIN);
   for(int j = -1; (j < ports.size()) && !IsShutdownInProgress(); j++)
   {
      uint16_t port;
      if (j == -1)
      {
         if (originalPort == 0)
            continue;
         port = originalPort;
      }
      else
      {
         port = ports.get(j);
         if (port == originalPort)
            continue;
      }

      nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): checking port %d"), ipAddrText, (int)port);

      if (snmpProxy != 0)
      {
         shared_ptr<NetObj> proxyNode = FindObjectById(snmpProxy, OBJECT_NODE);
         if (proxyNode == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): invalid proxy node ID %u"), ipAddrText, snmpProxy);
            goto fail;
         }
         shared_ptr<AgentConnectionEx> connection = static_cast<Node&>(*proxyNode).createAgentConnection();
         if (connection == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): cannot create proxy connection"), ipAddrText);
            goto fail;
         }
         pTransport = new SNMP_ProxyTransport(connection, ipAddr, port);
      }
      else
      {
         pTransport = new SNMP_UDPTransport();
         static_cast<SNMP_UDPTransport*>(pTransport)->createUDPTransport(ipAddr, port);
      }

      // Check for V3 USM
      if (!(initialDiscovery && (g_flags & AF_DISABLE_SNMP_V3_PROBE)))
      {
         if (SnmpCheckV3CommSettings(pTransport, originalContext, testOids, ipAddrText, zoneUIN, separateRequests))
         {
            *version = SNMP_VERSION_3;
            goto success;
         }

         if (IsShutdownInProgress())
         {
            delete pTransport;
            goto fail;
         }
      }

      if (initialDiscovery && ((g_flags & (AF_DISABLE_SNMP_V2_PROBE | AF_DISABLE_SNMP_V1_PROBE)) == (AF_DISABLE_SNMP_V2_PROBE | AF_DISABLE_SNMP_V1_PROBE)))
      {
         delete pTransport;
         goto fail;
      }

      pTransport->setSnmpVersion((initialDiscovery && (g_flags & AF_DISABLE_SNMP_V2_PROBE)) ? SNMP_VERSION_1 : SNMP_VERSION_2C);
restart_check:
      // Check current community first
      if ((originalContext != nullptr) && (originalContext->getSecurityModel() != SNMP_SECURITY_MODEL_USM))
      {
         nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): trying version %d community '%hs'"),
                  ipAddrText, pTransport->getSnmpVersion(), originalContext->getCommunity());
         pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
         if (SnmpTestRequest(pTransport, testOids, separateRequests))
         {
            *version = pTransport->getSnmpVersion();
            goto success;
         }
      }

      // Check community from list
      if (communities == nullptr)
         communities = SnmpGetKnownCommunities(zoneUIN);

      for(int i = 0; (i < communities->size()) && !IsShutdownInProgress(); i++)
      {
         char *community = MBStringFromWideString(communities->get(i));
         if ((originalContext == nullptr) ||
             (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM) ||
             strcmp(community, originalContext->getCommunity()))
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): trying version %d community '%hs'"),
                     ipAddrText, pTransport->getSnmpVersion(), community);
            pTransport->setSecurityContext(new SNMP_SecurityContext(community));
            MemFree(community);
            if (SnmpTestRequest(pTransport, testOids, separateRequests))
            {
               *version = pTransport->getSnmpVersion();
               goto success;
            }
         }
         else
         {
            MemFree(community);
         }
      }

      if ((pTransport->getSnmpVersion() == SNMP_VERSION_2C) && !IsShutdownInProgress() && !(initialDiscovery && (g_flags & AF_DISABLE_SNMP_V1_PROBE)))
      {
         pTransport->setSnmpVersion(SNMP_VERSION_1);
         goto restart_check;
      }
      delete pTransport;
   }

fail:
	nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): failed"), ipAddrText);
	return nullptr;

success:
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): success (version=%d)"), ipAddrText, pTransport->getSnmpVersion());
	return pTransport;
}

/**
 * MIB compiler executor
 */
class MibCompilerExecutor : public ProcessExecutor
{
private:
   uint32_t m_requestId;
   ClientSession *m_session;

   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   MibCompilerExecutor(const TCHAR *command, ClientSession *session, uint32_t requestId);
   virtual ~MibCompilerExecutor();
};

/**
 * Command execution constructor
 */
MibCompilerExecutor::MibCompilerExecutor(const TCHAR *command, ClientSession *session, uint32_t requestId) : ProcessExecutor(command, false, true)
{
   m_requestId = requestId;
   m_session = session;
   m_session->incRefCount();
   m_sendOutput = true;
}

/**
 * Command execution destructor
 */
MibCompilerExecutor::~MibCompilerExecutor()
{
   m_session->decRefCount();
   m_mibCompilationMutex.unlock();
}

/**
 * Send output to console
 */
void MibCompilerExecutor::onOutput(const char *text, size_t length)
{
   NXCPMessage msg(CMD_COMMAND_OUTPUT, m_requestId);
   wchar_t *buffer = WideStringFromMBStringSysLocale(text);
   msg.setField(VID_MESSAGE, buffer);
   m_session->sendMessage(msg);
   MemFree(buffer);
}

/**
 * Send message to make console stop listening to output
 */
void MibCompilerExecutor::endOfOutput()
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, m_requestId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   m_session->sendMessage(msg);

   NotifyClientSessions(NX_NOTIFY_MIB_UPDATED, 0);
   nxlog_debug_tag(_T("snmp.mib"), 6, _T("CompileMibFiles: MIB compiler execution completed"));
}

/**
 * Compile MIB files
 */
uint32_t CompileMibFiles(ClientSession *session, uint32_t requestId)
{
   if (!m_mibCompilationMutex.tryLock())
   {
      nxlog_debug_tag(_T("snmp.mib"), 6, _T("CompileMibFiles: MIB compiler already locked by another thread"));
      return RCC_COMPONENT_LOCKED;
   }

   TCHAR dataDir[MAX_PATH];
   GetNetXMSDirectory(nxDirData, dataDir);

   TCHAR serverMibFolder[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, serverMibFolder);
   _tcslcat(serverMibFolder, DDIR_MIBS, MAX_PATH);

   TCHAR binFolder[MAX_PATH];
   GetNetXMSDirectory(nxDirBin, binFolder);

   TCHAR cmdLine[512];
   _sntprintf(cmdLine, 512, _T("%s") FS_PATH_SEPARATOR _T("nxmibc") EXECUTABLE_FILE_SUFFIX _T(" -m -a -d \"%s\" -d \"%s") DDIR_MIBS _T("\" -o \"%s") FS_PATH_SEPARATOR _T("netxms.cmib\""), binFolder, serverMibFolder, dataDir, dataDir);
   nxlog_debug_tag(_T("snmp.mib"), 6, _T("CompileMibFiles: running MIB compiler as: %s"), cmdLine);

   MibCompilerExecutor *executor = new MibCompilerExecutor(cmdLine, session, requestId);
   if (!executor->execute())
   {
      nxlog_debug_tag(_T("snmp.mib"), 4, _T("CompileMibFiles: cannot run MIB compiler (command line was %s)"), cmdLine);
      delete executor;
      m_mibCompilationMutex.unlock();
      return RCC_INTERNAL_ERROR;
   }

   return RCC_SUCCESS;
}
