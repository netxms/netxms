/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
static DWORD HandlerArp(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen, dwIndex = 0;
   BYTE bMac[64];
   DWORD dwResult;

   dwNameLen = pVar->GetName()->getLength();
   memcpy(oidName, pVar->GetName()->getValue(), dwNameLen * sizeof(DWORD));

   oidName[dwNameLen - 6] = 1;  // Retrieve interface index
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, &dwIndex, sizeof(DWORD), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 6] = 2;  // Retrieve MAC address for this IP
	dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, bMac, 64, SG_RAW_RESULT);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      ((ARP_CACHE *)pArg)->dwNumEntries++;
      ((ARP_CACHE *)pArg)->pEntries = (ARP_ENTRY *)realloc(((ARP_CACHE *)pArg)->pEntries,
               sizeof(ARP_ENTRY) * ((ARP_CACHE *)pArg)->dwNumEntries);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIpAddr = ntohl(pVar->GetValueAsUInt());
      memcpy(((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].bMacAddr, bMac, 6);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIndex = dwIndex;
   }
   return dwResult;
}

/**
 * Get ARP cache via SNMP
 */
ARP_CACHE *SnmpGetArpCache(DWORD dwVersion, SNMP_Transport *pTransport)
{
   ARP_CACHE *pArpCache;

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   if (pArpCache == NULL)
      return NULL;

   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = NULL;

   if (SnmpWalk(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.22.1.3"), 
                HandlerArp, pArpCache, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyArpCache(pArpCache);
      pArpCache = NULL;
   }
   return pArpCache;
}

/**
 * Get interface status via SNMP
 */
void SnmpGetInterfaceStatus(DWORD dwVersion, SNMP_Transport *pTransport, DWORD dwIfIndex, int *adminState, int *operState)
{
   DWORD dwAdminStatus = 0, dwOperStatus = 0;
   TCHAR szOid[256];

   // Interface administrative status
   _sntprintf(szOid, 256, _T(".1.3.6.1.2.1.2.2.1.7.%d"), dwIfIndex);
   SnmpGet(dwVersion, pTransport, szOid, NULL, 0, &dwAdminStatus, sizeof(DWORD), 0);

   switch(dwAdminStatus)
   {
		case IF_ADMIN_STATE_DOWN:
			*adminState = IF_ADMIN_STATE_DOWN;
			*operState = IF_OPER_STATE_DOWN;
         break;
      case IF_ADMIN_STATE_UP:
		case IF_ADMIN_STATE_TESTING:
			*adminState = (int)dwAdminStatus;
         // Get interface operational status
         _sntprintf(szOid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%d"), dwIfIndex);
         SnmpGet(dwVersion, pTransport, szOid, NULL, 0, &dwOperStatus, sizeof(DWORD), 0);
         switch(dwOperStatus)
         {
            case 3:
					*operState = IF_OPER_STATE_TESTING;
               break;
            case 2:  // down: interface is down
				case 7:	// lowerLayerDown: down due to state of lower-layer interface(s)
					*operState = IF_OPER_STATE_DOWN;
               break;
            case 1:
					*operState = IF_OPER_STATE_UP;
               break;
            default:
					*operState = IF_OPER_STATE_UNKNOWN;
               break;
         }
         break;
      default:
			*adminState = IF_ADMIN_STATE_UNKNOWN;
			*operState = IF_OPER_STATE_UNKNOWN;
         break;
   }
}

/**
 * Handler for route enumeration
 */
static DWORD HandlerRoute(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen, dwResult;
   ROUTE route;
	ROUTING_TABLE *rt = (ROUTING_TABLE *)pArg;

   dwNameLen = pVar->GetName()->getLength();
	if ((dwNameLen < 5) || (dwNameLen > MAX_OID_LEN))
	{
		DbgPrintf(4, _T("HandlerRoute(): strange dwNameLen %d (name=%s)"), dwNameLen, pVar->GetName()->getValueAsText());
		return SNMP_ERR_SUCCESS;
	}
   memcpy(oidName, pVar->GetName()->getValue(), dwNameLen * sizeof(DWORD));
   route.dwDestAddr = ntohl(pVar->GetValueAsUInt());

   oidName[dwNameLen - 5] = 2;  // Interface index
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                           &route.dwIfIndex, sizeof(DWORD), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 5] = 7;  // Next hop
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                           &route.dwNextHop, sizeof(DWORD), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 5] = 8;  // Route type
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                           &route.dwRouteType, sizeof(DWORD), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 5] = 11;  // Destination mask
   if ((dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen,
                           &route.dwDestMask, sizeof(DWORD), 0)) != SNMP_ERR_SUCCESS)
      return dwResult;

   rt->iNumEntries++;
   rt->pRoutes = (ROUTE *)realloc(rt->pRoutes, sizeof(ROUTE) * rt->iNumEntries);
   memcpy(&rt->pRoutes[rt->iNumEntries - 1], &route, sizeof(ROUTE));
   return SNMP_ERR_SUCCESS;
}

/**
 * Get routing table via SNMP
 */
ROUTING_TABLE *SnmpGetRoutingTable(DWORD dwVersion, SNMP_Transport *pTransport)
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
static SNMP_SecurityContext *SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext)
{
	char buffer[1024];

	// Check original SNMP V3 settings, if set
	if ((originalContext != NULL) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		DbgPrintf(5, _T("SnmpCheckV3CommSettings: trying %hs/%d:%d"), originalContext->getUser(),
		          originalContext->getAuthMethod(), originalContext->getPrivMethod());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
		if (SnmpGet(SNMP_VERSION_3, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
		{
			DbgPrintf(5, _T("SnmpCheckV3CommSettings: success"));
			return new SNMP_SecurityContext(originalContext);
		}
	}

	// Try preconfigured SNMP v3 USM credentials
	DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials"));
	if (hResult != NULL)
	{
		char name[MAX_DB_STRING], authPasswd[MAX_DB_STRING], privPasswd[MAX_DB_STRING];
		SNMP_SecurityContext *ctx;
		int i, count = DBGetNumRows(hResult);

		for(i = 0; i < count; i++)
		{
			DBGetFieldA(hResult, i, 0, name, MAX_DB_STRING);
			DBGetFieldA(hResult, i, 3, authPasswd, MAX_DB_STRING);
			DBGetFieldA(hResult, i, 4, privPasswd, MAX_DB_STRING);
			ctx = new SNMP_SecurityContext(name, authPasswd, privPasswd,
			                               DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));
			pTransport->setSecurityContext(ctx);
			DbgPrintf(5, _T("SnmpCheckV3CommSettings: trying %hs/%d:%d"), ctx->getUser(), ctx->getAuthMethod(), ctx->getPrivMethod());
			if (SnmpGet(SNMP_VERSION_3, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL,
							0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
			{
				DbgPrintf(5, _T("SnmpCheckV3CommSettings: success"));
				break;
			}
		}
		DBFreeResult(hResult);

		if (i < count)
			return new SNMP_SecurityContext(ctx);
	}
	else
	{
		DbgPrintf(3, _T("SnmpCheckV3CommSettings: DBSelect() failed"));
	}
	
	DbgPrintf(5, _T("SnmpCheckV3CommSettings: failed"));
	return NULL;
}

/**
 * Determine SNMP parameters for node
 * On success, returns new security context object (dynamically created).
 * On failure, returns NULL
 */
SNMP_SecurityContext *SnmpCheckCommSettings(SNMP_Transport *pTransport, int *version, SNMP_SecurityContext *originalContext)
{
	int i, count, snmpVer = SNMP_VERSION_2C;
	TCHAR buffer[1024];
	char defCommunity[MAX_COMMUNITY_LENGTH];
	DB_RESULT hResult;

	// Check for V3 USM
	SNMP_SecurityContext *securityContext = SnmpCheckV3CommSettings(pTransport, originalContext);
	if (securityContext != NULL)
	{
		*version = SNMP_VERSION_3;
		return securityContext;
	}

	ConfigReadStrA(_T("DefaultCommunityString"), defCommunity, MAX_COMMUNITY_LENGTH, "public");

restart_check:
	// Check current community first
	if ((originalContext != NULL) && (originalContext->getSecurityModel() != SNMP_SECURITY_MODEL_USM))
	{
		DbgPrintf(5, _T("SnmpCheckCommSettings: trying version %d community '%hs'"), snmpVer, originalContext->getCommunity());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
		if (SnmpGet(snmpVer, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL,
						0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
		{
			*version = snmpVer;
			return new SNMP_SecurityContext(originalContext);
		}
	}

	// Check default community
	DbgPrintf(5, _T("SnmpCheckCommSettings: trying version %d community '%hs'"), snmpVer, defCommunity);
	pTransport->setSecurityContext(new SNMP_SecurityContext(defCommunity));
	if (SnmpGet(snmpVer, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL,
		         0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
	{
		*version = snmpVer;
		return new SNMP_SecurityContext(defCommunity);
	}

	// Check community from list
	hResult = DBSelect(g_hCoreDB, _T("SELECT community FROM snmp_communities"));
	if (hResult != NULL)
	{
		char temp[256];

		count = DBGetNumRows(hResult);
		for(i = 0; i < count; i++)
		{
			DBGetFieldA(hResult, i, 0, temp, 256);
			DbgPrintf(5, _T("SnmpCheckCommSettings: trying version %d community '%hs'"), snmpVer, temp);
			pTransport->setSecurityContext(new SNMP_SecurityContext(temp));
			if (SnmpGet(snmpVer, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL, 0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
			{
				*version = snmpVer;
				break;
			}
		}
		DBFreeResult(hResult);
		if (i < count)
			return new SNMP_SecurityContext(temp);
	}
	else
	{
		DbgPrintf(3, _T("SnmpCheckCommSettings: DBSelect() failed"));
	}

	if (snmpVer == SNMP_VERSION_2C)
	{
		snmpVer = SNMP_VERSION_1;
		goto restart_check;
	}

	DbgPrintf(5, _T("SnmpCheckCommSettings: failed"));
	return NULL;
}
