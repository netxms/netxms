/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
static UINT32 HandlerRoute(SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   UINT32 oidName[MAX_OID_LEN], dwResult;
   ROUTE route;
	ROUTING_TABLE *rt = (ROUTING_TABLE *)pArg;

   size_t nameLen = pVar->getName().length();
	if ((nameLen < 5) || (nameLen > MAX_OID_LEN))
	{
		nxlog_debug_tag(_T("topo.ipv4"), 4, _T("HandlerRoute(): strange nameLen %d (name=%s)"), nameLen, (const TCHAR *)pVar->getName().toString());
		return SNMP_ERR_SUCCESS;
	}
   memcpy(oidName, pVar->getName().value(), nameLen * sizeof(UINT32));
   route.dwDestAddr = ntohl(pVar->getValueAsUInt());

   oidName[nameLen - 5] = 2;  // Interface index
   if ((dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen,
                             &route.dwIfIndex, sizeof(UINT32), 0, NULL)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 7;  // Next hop
   if ((dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen,
                             &route.dwNextHop, sizeof(UINT32), 0, NULL)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 8;  // Route type
   if ((dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen,
                             &route.dwRouteType, sizeof(UINT32), 0, NULL)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 11;  // Destination mask
   if ((dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen,
                             &route.dwDestMask, sizeof(UINT32), 0, NULL)) != SNMP_ERR_SUCCESS)
      return dwResult;

   rt->iNumEntries++;
   rt->pRoutes = (ROUTE *)realloc(rt->pRoutes, sizeof(ROUTE) * rt->iNumEntries);
   memcpy(&rt->pRoutes[rt->iNumEntries - 1], &route, sizeof(ROUTE));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get routing table via SNMP
 */
ROUTING_TABLE *SnmpGetRoutingTable(SNMP_Transport *pTransport)
{
   ROUTING_TABLE *pRT;

   pRT = MemAllocStruct<ROUTING_TABLE>();
   if (pRT == NULL)
      return NULL;

   pRT->iNumEntries = 0;
   pRT->pRoutes = NULL;

   if (SnmpWalk(pTransport, _T(".1.3.6.1.2.1.4.21.1.1"), HandlerRoute, pRT) != SNMP_ERR_SUCCESS)
   {
      DestroyRoutingTable(pRT);
      pRT = NULL;
   }
   return pRT;
}

/**
 * Send request and check if we get any response
 */
bool SnmpTestRequest(SNMP_Transport *snmp, StringList *testOids)
{
   bool success = false;
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   for(int i = 0; i < testOids->size(); i++)
      request.bindVariable(new SNMP_Variable(testOids->get(i)));
   SNMP_PDU *response;
   UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rc == SNMP_ERR_SUCCESS)
   {
      success = (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) || (response->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME);
      delete response;
   }
   return success;
}

/**
 * Check SNMP v3 connectivity
 */
static bool SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext, StringList *testOids, const TCHAR *id, UINT32 zoneUIN)
{
   pTransport->setSnmpVersion(SNMP_VERSION_3);

	// Check original SNMP V3 settings, if set
	if ((originalContext != NULL) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, originalContext->getUser(),
		          originalContext->getAuthMethod(), originalContext->getPrivMethod());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
		if (SnmpTestRequest(pTransport, testOids))
		{
         nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckV3CommSettings(%s): success"), id);
         return true;
		}
	}

	// Try pre-configured SNMP v3 USM credentials
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials WHERE zone=? OR zone=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, SNMP_CONFIG_GLOBAL);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         ObjectArray<SNMP_SecurityContext> contexts(count, 16, false);
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
            if (SnmpTestRequest(pTransport, testOids))
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
SNMP_Transport *SnmpCheckCommSettings(UINT32 snmpProxy, const InetAddress& ipAddr, INT16 *version, UINT16 originalPort,
         SNMP_SecurityContext *originalContext, StringList *testOids, UINT32 zoneUIN)
{
   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): starting check (proxy=%d, originalPort=%d)"), ipAddr.toString(ipAddrText), snmpProxy, (int)originalPort);

   SNMP_Transport *pTransport = NULL;
   StringList *communities = NULL;

   TCHAR portList[MAX_CONFIG_VALUE];
	ConfigReadStr(_T("SNMPPorts"), portList, MAX_CONFIG_VALUE, _T("161"));
	Trim(portList);
	if (portList[0] == 0)
	   _tcscpy(portList, _T("161"));
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): global port list: %s)"), ipAddrText, portList);
   StringList *ports = new StringList(portList, _T(","));

   Zone *zone = FindZoneByUIN(zoneUIN);
   if (zone != NULL)
      ports->insertAll(0, zone->getSnmpPortList()); // Port list defined at zone level has priority

   for(int j = -1; (j < ports->size()) && !IsShutdownInProgress(); j++)
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
         port = (UINT16)_tcstoul(ports->get(j), NULL, 10);
         if ((port == originalPort) || (port == 0))
            continue;
      }

      nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): checking port %d"), ipAddrText, (int)port);

      AgentConnection *pConn = NULL;
      if (snmpProxy != 0)
      {
         Node *proxyNode = (Node *)FindObjectById(snmpProxy, OBJECT_NODE);
         if (proxyNode == NULL)
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): invalid proxy node ID %u"), ipAddrText, snmpProxy);
            goto fail;
         }
         pConn = proxyNode->createAgentConnection();
         if (pConn == NULL)
         {
            nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): cannot create proxy connection"), ipAddrText);
            goto fail;
         }
         pTransport = new SNMP_ProxyTransport(pConn, ipAddr, port);
      }
      else
      {
         pTransport = new SNMP_UDPTransport();
         static_cast<SNMP_UDPTransport*>(pTransport)->createUDPTransport(ipAddr, port);
      }

      // Check for V3 USM
      if (SnmpCheckV3CommSettings(pTransport, originalContext, testOids, ipAddrText, zoneUIN))
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
         if (SnmpTestRequest(pTransport, testOids))
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
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT community FROM snmp_communities WHERE zone=? OR zone=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, SNMP_CONFIG_GLOBAL);

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
            free(community);
#endif
            if (SnmpTestRequest(pTransport, testOids))
            {
               *version = pTransport->getSnmpVersion();
               goto success;
            }
         }
#ifdef UNICODE
         else
         {
            free(community);
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
   delete ports;
	nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): failed"), ipAddrText);
	return NULL;

success:
   delete communities;
   delete ports;
   nxlog_debug_tag(DEBUG_TAG_SNMP_DISCOVERY, 5, _T("SnmpCheckCommSettings(%s): success (version=%d)"), ipAddrText, pTransport->getSnmpVersion());
	return pTransport;
}
