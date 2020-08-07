/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

/**
 * Handler for route enumeration
 */
static UINT32 HandlerRoute(SNMP_Variable *pVar, SNMP_Transport *pTransport, RoutingTable *routingTable)
{
   size_t nameLen = pVar->getName().length();
	if ((nameLen < 5) || (nameLen > MAX_OID_LEN))
	{
		nxlog_debug_tag(_T("topo.ipv4"), 4, _T("HandlerRoute(): strange nameLen %d (name=%s)"), nameLen, pVar->getName().toString().cstr());
		return SNMP_ERR_SUCCESS;
	}

   uint32_t oidName[MAX_OID_LEN];
   memcpy(oidName, pVar->getName().value(), nameLen * sizeof(UINT32));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), pTransport->getSnmpVersion());

   oidName[nameLen - 5] = 2;  // Interface index
   request.bindVariable(new SNMP_Variable(oidName, nameLen));

   oidName[nameLen - 5] = 7;  // Next hop
   request.bindVariable(new SNMP_Variable(oidName, nameLen));

   oidName[nameLen - 5] = 8;  // Route type
   request.bindVariable(new SNMP_Variable(oidName, nameLen));

   oidName[nameLen - 5] = 11;  // Destination mask
   request.bindVariable(new SNMP_Variable(oidName, nameLen));

   uint32_t dwResult;
   SNMP_PDU *response;
   if ((dwResult = pTransport->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3)) != SNMP_ERR_SUCCESS)
      return dwResult;

   if (response->getNumVariables() != request.getNumVariables())
   {
      delete response;
      return SNMP_ERR_NO_OBJECT;
   }

   ROUTE route;
   route.dwDestAddr = ntohl(pVar->getValueAsUInt());
   route.dwIfIndex = response->getVariable(0)->getValueAsUInt();
   route.dwNextHop = ntohl(response->getVariable(1)->getValueAsUInt());
   route.dwRouteType = response->getVariable(2)->getValueAsUInt();
   route.dwDestMask = ntohl(response->getVariable(3)->getValueAsUInt());
   routingTable->add(&route);
   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Get routing table via SNMP
 */
RoutingTable *SnmpGetRoutingTable(SNMP_Transport *pTransport)
{
   RoutingTable *routingTable = new RoutingTable(64, 64);
   if (routingTable == nullptr)
      return nullptr;

   if (SnmpWalk(pTransport, _T(".1.3.6.1.2.1.4.21.1.1"), HandlerRoute, routingTable, false, true) != SNMP_ERR_SUCCESS)
   {
      delete_and_null(routingTable);
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
         UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
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
      UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
      if (rc == SNMP_ERR_SUCCESS)
      {
         success = (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) || (response->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME);
         delete response;
      }
   }
   return success;
}

/**
 * Check SNMP v3 connectivity
 */
static bool SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext,
         const StringList &testOids, const TCHAR *id, int32_t zoneUIN, bool separateRequests)
{
   pTransport->setSnmpVersion(SNMP_VERSION_3);

	// Check original SNMP V3 settings, if set
	if ((originalContext != NULL) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, originalContext->getUser(),
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
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         ObjectArray<SNMP_SecurityContext> contexts(count);
         for(int i = 0; i < count; i++)
         {
            char name[MAX_DB_STRING], authPasswd[MAX_DB_STRING], privPasswd[MAX_DB_STRING];
            DBGetFieldA(hResult, i, 0, name, MAX_DB_STRING);
            DBGetFieldA(hResult, i, 3, authPasswd, MAX_DB_STRING);
            DBGetFieldA(hResult, i, 4, privPasswd, MAX_DB_STRING);
            contexts.add(new SNMP_SecurityContext(name, authPasswd, privPasswd, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2)));
         }

         DBFreeResult(hResult);
         DBConnectionPoolReleaseConnection(hdb);

         bool found = false;
         for(int i = 0; (i < contexts.size()) && !found && !IsShutdownInProgress(); i++)
         {
            SNMP_SecurityContext *ctx = contexts.get(i);
            pTransport->setSecurityContext(ctx);
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, ctx->getUser(), ctx->getAuthMethod(), ctx->getPrivMethod());
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
         DBConnectionPoolReleaseConnection(hdb);
         nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 3, _T("SnmpCheckV3CommSettings(%s): DBSelect() failed"), id);
      }

      DBFreeStatement(hStmt);
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
         SNMP_SecurityContext *originalContext, const StringList &testOids, int32_t zoneUIN)
{
   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): starting check (proxy=%d, originalPort=%d)"), ipAddr.toString(ipAddrText), snmpProxy, (int)originalPort);

   SNMP_Transport *pTransport = NULL;
   StringList *communities = NULL;


   bool separateRequests = ConfigReadBoolean(_T("SNMP.Discovery.SeparateProbeRequests"), false);

   IntegerArray<uint16_t> ports;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT port FROM snmp_ports WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)

            ports.add(DBGetFieldLong(hResult, i, 0));
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);


   if (ports.size() == 0)
      ports.add(161);

   for(int j = -1; (j < ports.size()) && !IsShutdownInProgress(); j++)
   {
      UINT16 port;
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

      pTransport->setSnmpVersion(SNMP_VERSION_2C);
restart_check:
      // Check current community first
      if ((originalContext != NULL) && (originalContext->getSecurityModel() != SNMP_SECURITY_MODEL_USM))
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
      if (communities == NULL)
      {
         communities = new StringList();
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT community FROM snmp_communities WHERE zone=? OR zone=-1 ORDER BY zone DESC, id ASC"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);

            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != NULL)
            {
               int count = DBGetNumRows(hResult);
               for(int i = 0; i < count; i++)
                  communities->addPreallocated(DBGetField(hResult, i, 0, NULL, 0));
               DBFreeResult(hResult);
            }
            DBFreeStatement(hStmt);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }

      for(int i = 0; (i < communities->size()) && !IsShutdownInProgress(); i++)
      {
#ifdef UNICODE
         char *community = MBStringFromWideString(communities->get(i));
#else
         const char *community = communities->get(i);
#endif
         if ((originalContext == NULL) ||
             (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM) ||
             strcmp(community, originalContext->getCommunity()))
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): trying version %d community '%hs'"),
                     ipAddrText, pTransport->getSnmpVersion(), community);
            pTransport->setSecurityContext(new SNMP_SecurityContext(community));
#ifdef UNICODE
            MemFree(community);
#endif
            if (SnmpTestRequest(pTransport, testOids, separateRequests))
            {
               *version = pTransport->getSnmpVersion();
               goto success;
            }
         }
#ifdef UNICODE
         else
         {
            MemFree(community);
         }
#endif
      }

      if ((pTransport->getSnmpVersion() == SNMP_VERSION_2C) && !IsShutdownInProgress())
      {
         pTransport->setSnmpVersion(SNMP_VERSION_1);
         goto restart_check;
      }
      delete pTransport;
   }

fail:
   delete communities;
	nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): failed"), ipAddrText);
	return nullptr;

success:
   delete communities;
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): success (version=%d)"), ipAddrText, pTransport->getSnmpVersion());
	return pTransport;
}
