/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Handler for ARP enumeration
 */
static UINT32 HandlerArp(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   UINT32 oidName[MAX_OID_LEN], dwIndex = 0;
   BYTE bMac[64];
   UINT32 dwResult;

   size_t nameLen = pVar->getName()->getLength();
   memcpy(oidName, pVar->getName()->getValue(), nameLen * sizeof(UINT32));

   oidName[nameLen - 6] = 1;  // Retrieve interface index
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen, &dwIndex, sizeof(UINT32), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 6] = 2;  // Retrieve MAC address for this IP
	dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen, bMac, 64, SG_RAW_RESULT);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      ((ARP_CACHE *)pArg)->dwNumEntries++;
      ((ARP_CACHE *)pArg)->pEntries = (ARP_ENTRY *)realloc(((ARP_CACHE *)pArg)->pEntries,
               sizeof(ARP_ENTRY) * ((ARP_CACHE *)pArg)->dwNumEntries);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].ipAddr = ntohl(pVar->getValueAsUInt());
      memcpy(((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].bMacAddr, bMac, 6);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIndex = dwIndex;
   }
   return dwResult;
}

/**
 * Get ARP cache via SNMP
 */
ARP_CACHE *SnmpGetArpCache(UINT32 dwVersion, SNMP_Transport *pTransport)
{
   ARP_CACHE *pArpCache;

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   if (pArpCache == NULL)
      return NULL;

   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = NULL;

   if (SnmpWalk(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.22.1.3"), HandlerArp, pArpCache, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyArpCache(pArpCache);
      pArpCache = NULL;
   }
   return pArpCache;
}

/**
 * Handler for route enumeration
 */
static UINT32 HandlerRoute(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   UINT32 oidName[MAX_OID_LEN], dwResult;
   ROUTE route;
	ROUTING_TABLE *rt = (ROUTING_TABLE *)pArg;

   size_t nameLen = pVar->getName()->getLength();
	if ((nameLen < 5) || (nameLen > MAX_OID_LEN))
	{
		DbgPrintf(4, _T("HandlerRoute(): strange nameLen %d (name=%s)"), nameLen, pVar->getName()->getValueAsText());
		return SNMP_ERR_SUCCESS;
	}
   memcpy(oidName, pVar->getName()->getValue(), nameLen * sizeof(UINT32));
   route.dwDestAddr = ntohl(pVar->getValueAsUInt());

   oidName[nameLen - 5] = 2;  // Interface index
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen,
                           &route.dwIfIndex, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 7;  // Next hop
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen,
                           &route.dwNextHop, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 8;  // Route type
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen,
                           &route.dwRouteType, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[nameLen - 5] = 11;  // Destination mask
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen,
                           &route.dwDestMask, sizeof(UINT32), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   rt->iNumEntries++;
   rt->pRoutes = (ROUTE *)realloc(rt->pRoutes, sizeof(ROUTE) * rt->iNumEntries);
   memcpy(&rt->pRoutes[rt->iNumEntries - 1], &route, sizeof(ROUTE));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get routing table via SNMP
 */
ROUTING_TABLE *SnmpGetRoutingTable(UINT32 dwVersion, SNMP_Transport *pTransport)
{
   ROUTING_TABLE *pRT;

   pRT = (ROUTING_TABLE *)malloc(sizeof(ROUTING_TABLE));
   if (pRT == NULL)
      return NULL;

   pRT->iNumEntries = 0;
   pRT->pRoutes = NULL;

   if (SnmpWalk(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.21.1.1"),
                HandlerRoute, pRT, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyRoutingTable(pRT);
      pRT = NULL;
   }
   return pRT;
}

/**
 * Check SNMP v3 connectivity
 */
bool SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext, StringList *testOids, const TCHAR *id)
{
	char buffer[1024];

	// Check original SNMP V3 settings, if set
	if ((originalContext != NULL) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		DbgPrintf(5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, originalContext->getUser(),
		          originalContext->getAuthMethod(), originalContext->getPrivMethod());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
      for(int i = 0; i < testOids->size(); i++)
      {
         if (SnmpGet(SNMP_VERSION_3, pTransport, testOids->get(i), NULL, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
         {
			   DbgPrintf(5, _T("SnmpCheckV3CommSettings(%s): success"), id);
			   return true;
         }
      }
	}

	// Try pre-configured SNMP v3 USM credentials
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT hResult = DBSelect(hdb, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials"));
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
		for(int i = 0; (i < contexts.size()) && !found; i++)
		{
		   SNMP_SecurityContext *ctx = contexts.get(i);
			pTransport->setSecurityContext(ctx);
			DbgPrintf(5, _T("SnmpCheckV3CommSettings(%s): trying %hs/%d:%d"), id, ctx->getUser(), ctx->getAuthMethod(), ctx->getPrivMethod());
         for(int j = 0; j < testOids->size(); j++)
         {
            if (SnmpGet(SNMP_VERSION_3, pTransport, testOids->get(j), NULL, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
            {
				   DbgPrintf(5, _T("SnmpCheckV3CommSettings(%s): success"), id);
				   found = true;

				   // Delete unused contexts
				   for(int j = i + 1; j < contexts.size(); j++)
				      delete contexts.get(j);

				   break;
            }
			}
		}

		if (found)
			return true;
	}
	else
	{
      DBConnectionPoolReleaseConnection(hdb);
		DbgPrintf(3, _T("SnmpCheckV3CommSettings(%s): DBSelect() failed"), id);
	}

	DbgPrintf(5, _T("SnmpCheckV3CommSettings(%s): failed"), id);
	return false;
}

/**
 * Determine SNMP parameters for node
 * On success, returns new security context object (dynamically created).
 * On failure, returns NULL
 */
SNMP_Transport *SnmpCheckCommSettings(UINT32 snmpProxy, const InetAddress& ipAddr, INT16 *version, UINT16 originalPort, SNMP_SecurityContext *originalContext, StringList *testOids)
{
   DbgPrintf(5, _T("SnmpCheckCommSettings(%s): starting check (proxy=%d, originalPort=%d)"), (const TCHAR *)ipAddr.toString(), snmpProxy, (int)originalPort);

	int i, count, snmpVer = SNMP_VERSION_2C;
	TCHAR buffer[1024];
   SNMP_Transport *pTransport;
   StringList *communities = NULL;

   TCHAR tmp[MAX_CONFIG_VALUE];
	ConfigReadStr(_T("SNMPPorts"), tmp, MAX_CONFIG_VALUE, _T("161"));
   StringList *ports = new StringList(tmp, _T(","));
   for(int j = -1; j < ports->size(); j++)
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
         if (port == originalPort)
            continue;
      }

      DbgPrintf(5, _T("SnmpCheckCommSettings(%s): checking port %d"), (const TCHAR *)ipAddr.toString(), (int)port);

      AgentConnection *pConn = NULL;
      if (snmpProxy != 0)
      {
         Node *proxyNode = (Node *)FindObjectById(snmpProxy, OBJECT_NODE);
         if (proxyNode == NULL)
         {
            DbgPrintf(5, _T("SnmpCheckCommSettings(%s): invalid proxy node ID %d"), (const TCHAR *)ipAddr.toString(), snmpProxy);
            goto fail;
         }
         pConn = proxyNode->createAgentConnection();
         if (pConn == NULL)
         {
            DbgPrintf(5, _T("SnmpCheckCommSettings(%s): cannot create proxy connection"), (const TCHAR *)ipAddr.toString());
            goto fail;
         }
         pTransport = new SNMP_ProxyTransport(pConn, ipAddr, port);
      }
      else
      {
         pTransport = new SNMP_UDPTransport();
         ((SNMP_UDPTransport *)pTransport)->createUDPTransport(ipAddr, port);
      }

      // Check for V3 USM
      if (SnmpCheckV3CommSettings(pTransport, originalContext, testOids, (const TCHAR *)ipAddr.toString()))
      {
         *version = SNMP_VERSION_3;
         goto success;
      }

restart_check:
      // Check current community first
      if ((originalContext != NULL) && (originalContext->getSecurityModel() != SNMP_SECURITY_MODEL_USM))
      {
         DbgPrintf(5, _T("SnmpCheckCommSettings(%s): trying version %d community '%hs'"), (const TCHAR *)ipAddr.toString(), snmpVer, originalContext->getCommunity());
         pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
         for(int i = 0; i < testOids->size(); i++)
         {
            if (SnmpGet(snmpVer, pTransport, testOids->get(i), NULL, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
            {
               *version = snmpVer;
               goto success;
            }
         }
      }

      // Check community from list
      if (communities == NULL)
      {
         communities = new StringList();
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_RESULT hResult = DBSelect(hdb, _T("SELECT community FROM snmp_communities"));
         if (hResult != NULL)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
               communities->addPreallocated(DBGetField(hResult, i, 0, NULL, 0));
            DBFreeResult(hResult);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }

      for(i = 0; i < count; i++)
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
            DbgPrintf(5, _T("SnmpCheckCommSettings(%s): trying version %d community '%hs'"), (const TCHAR *)ipAddr.toString(), snmpVer, community);
            pTransport->setSecurityContext(new SNMP_SecurityContext(community));
#ifdef UNICODE
            free(community);
#endif
            for(int j = 0; j < testOids->size(); j++)
            {
               if (SnmpGet(snmpVer, pTransport, testOids->get(j), NULL, 0, buffer, sizeof(buffer), 0) == SNMP_ERR_SUCCESS)
               {
                  *version = snmpVer;
                  goto success;
               }
            }
         }
#ifdef UNICODE
         else
         {
            free(community);
         }
#endif
      }

      if (snmpVer == SNMP_VERSION_2C)
      {
         snmpVer = SNMP_VERSION_1;
         goto restart_check;
      }
      delete pTransport;
   }

fail:
   delete communities;
   delete ports;
	DbgPrintf(5, _T("SnmpCheckCommSettings(%s): failed"), (const TCHAR *)ipAddr.toString());
	return NULL;

success:
   delete communities;
   delete ports;
   DbgPrintf(5, _T("SnmpCheckCommSettings(%s): success (version=%d)"), (const TCHAR *)ipAddr.toString(), pTransport->getSnmpVersion());
	return pTransport;
}
