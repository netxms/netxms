/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: snmp.cpp
**
**/

#include "nxcore.h"


//
// OID translation structure
//

struct OID_TABLE
{
   TCHAR *pszOid;
   DWORD dwNodeType;
   DWORD dwNodeFlags;
};


//
// Static data
//

static OID_TABLE *m_pOidTable = NULL;
static DWORD m_dwOidTableSize = 0;
static DWORD m_dwRequestId = 1;


//
// Initialize SNMP subsystem
//

void SnmpInit(void)
{
   DB_RESULT hResult;
   DWORD i;

   // Load OID to type translation table
   hResult = DBSelect(g_hCoreDB, _T("SELECT snmp_oid,node_type,node_flags FROM oid_to_type ORDER BY pair_id"));
   if (hResult != NULL)
   {
      m_dwOidTableSize = DBGetNumRows(hResult);
      m_pOidTable = (OID_TABLE *)malloc(sizeof(OID_TABLE) * m_dwOidTableSize);
      for(i = 0; i < m_dwOidTableSize; i++)
      {
         m_pOidTable[i].pszOid = _tcsdup(DBGetField(hResult, i, 0));
         m_pOidTable[i].dwNodeType = DBGetFieldULong(hResult, i, 1);
         m_pOidTable[i].dwNodeFlags = DBGetFieldULong(hResult, i, 2);
      }
      DBFreeResult(hResult);
   }
}


//
// Determine node type by OID
//

DWORD OidToType(TCHAR *pszOid, DWORD *pdwFlags)
{
   DWORD i;

   for(i = 0; i < m_dwOidTableSize; i++)
      if (MatchString(m_pOidTable[i].pszOid, pszOid, TRUE))
      {
         if (pdwFlags != NULL)
            *pdwFlags = m_pOidTable[i].dwNodeFlags;
         return m_pOidTable[i].dwNodeType;
      }

   *pdwFlags = 0;
   return NODE_TYPE_GENERIC;
}


//
// Get value for SNMP variable
// If szOidStr is not NULL, string representation of OID is used, otherwise -
// binary representation from oidBinary and dwOidLen
//

DWORD SnmpGet(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, const char *szOidStr,
              const DWORD *oidBinary, DWORD dwOidLen, void *pValue,
              DWORD dwBufferSize, BOOL bVerbose, BOOL bStringResult)
{
   SNMP_Transport *pTransport;
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD dwNameLen, pdwVarName[MAX_OID_LEN], dwResult;

   // Open SNMP session
   pTransport = new SNMP_Transport;
   dwResult = pTransport->CreateUDPTransport(NULL, htonl(dwAddr));
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      WriteLog(MSG_SNMP_TRANSPORT_FAILED, EVENTLOG_ERROR_TYPE, NULL);
   }
   else
   {
      // Create PDU and send request
      pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, (char *)szCommunity, m_dwRequestId++, dwVersion);
      if (szOidStr != NULL)
      {
         dwNameLen = SNMPParseOID(szOidStr, pdwVarName, MAX_OID_LEN);
         if (dwNameLen == 0)
         {
            WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szOidStr);
            dwResult = SNMP_ERR_BAD_OID;
         }
      }
      else
      {
         memcpy(pdwVarName, oidBinary, dwOidLen * sizeof(DWORD));
         dwNameLen = dwOidLen;
      }

      if (dwResult == SNMP_ERR_SUCCESS)   // Still no errors
      {
         pRqPDU->BindVariable(new SNMP_Variable(pdwVarName, dwNameLen));
         dwResult = pTransport->DoRequest(pRqPDU, &pRespPDU, 1000, 3);

         // Analyze response
         if (dwResult == SNMP_ERR_SUCCESS)
         {
            if ((pRespPDU->GetNumVariables() > 0) &&
                (pRespPDU->GetErrorCode() == SNMP_PDU_ERR_SUCCESS))
            {
               SNMP_Variable *pVar = pRespPDU->GetVariable(0);

               if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                   (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
               {
                  if (bStringResult)
                  {
                     pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                  }
                  else
                  {
                     switch(pVar->GetType())
                     {
                        case ASN_INTEGER:
                        case ASN_UINTEGER32:
                        case ASN_COUNTER32:
                        case ASN_GAUGE32:
                        case ASN_TIMETICKS:
                           *((long *)pValue) = pVar->GetValueAsInt();
                           break;
                        case ASN_IP_ADDR:
                           *((long *)pValue) = ntohl(pVar->GetValueAsUInt());
                           break;
                        case ASN_OCTET_STRING:
                           pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                           break;
                        case ASN_OBJECT_ID:
                           pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                           break;
                        default:
                           WriteLog(MSG_SNMP_UNKNOWN_TYPE, EVENTLOG_ERROR_TYPE, "x", pVar->GetType());
                           dwResult = SNMP_ERR_BAD_TYPE;
                           break;
                     }
                  }
               }
               else
               {
                  dwResult = SNMP_ERR_NO_OBJECT;
               }
            }
            else
            {
               if (pRespPDU->GetErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
                  dwResult = SNMP_ERR_NO_OBJECT;
               else
                  dwResult = SNMP_ERR_AGENT;
            }
            delete pRespPDU;
         }
         else
         {
            if (bVerbose)
               WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
         }
      }

      delete pRqPDU;
   }

   delete pTransport;
   return dwResult;
}


//
// Enumerate multiple values by walking throgh MIB, starting at given root
//

DWORD SnmpEnumerate(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, 
                    const char *szRootOid,
                    void (* pHandler)(DWORD, DWORD, const char *, SNMP_Variable *, void *), 
                    void *pUserArg, BOOL bVerbose)
{
   DWORD pdwRootName[MAX_OID_LEN], dwRootLen, pdwName[MAX_OID_LEN], dwNameLen, dwResult;
   SNMP_PDU *pRqPDU, *pRespPDU;
   SNMP_Transport *pTransport;
   BOOL bRunning = TRUE;

   // Open SNMP session
   pTransport = new SNMP_Transport;
   dwResult = pTransport->CreateUDPTransport(NULL, htonl(dwAddr));
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      WriteLog(MSG_SNMP_TRANSPORT_FAILED, EVENTLOG_ERROR_TYPE, NULL);
   }
   else
   {
      // Get root
      dwRootLen = SNMPParseOID(szRootOid, pdwRootName, MAX_OID_LEN);
      if (dwRootLen == 0)
      {
         WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
         dwResult = SNMP_ERR_BAD_OID;
      }
      else
      {
         memcpy(pdwName, pdwRootName, dwRootLen * sizeof(DWORD));
         dwNameLen = dwRootLen;

         // Walk the MIB
         while(bRunning)
         {
            pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, (char *)szCommunity, m_dwRequestId++, dwVersion);
            pRqPDU->BindVariable(new SNMP_Variable(pdwName, dwNameLen));
            dwResult = pTransport->DoRequest(pRqPDU, &pRespPDU, 1000, 3);

            // Analyze response
            if (dwResult == SNMP_ERR_SUCCESS)
            {
               if ((pRespPDU->GetNumVariables() > 0) &&
                   (pRespPDU->GetErrorCode() == SNMP_PDU_ERR_SUCCESS))
               {
                  SNMP_Variable *pVar = pRespPDU->GetVariable(0);

                  if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                      (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
                  {
                     // Should we stop walking?
                     if ((pVar->GetName()->Length() < dwRootLen) ||
                         (memcmp(pdwRootName, pVar->GetName()->GetValue(), dwRootLen * sizeof(DWORD))))
                     {
                        bRunning = FALSE;
                        delete pRespPDU;
                        delete pRqPDU;
                        break;
                     }
                     memcpy(pdwName, pVar->GetName()->GetValue(), 
                            pVar->GetName()->Length() * sizeof(DWORD));
                     dwNameLen = pVar->GetName()->Length();

                     // Call user's callback function for processing
                     pHandler(dwVersion, dwAddr, szCommunity, pVar, pUserArg);
                  }
                  else
                  {
                     dwResult = SNMP_ERR_NO_OBJECT;
                     bRunning = FALSE;
                  }
               }
               else
               {
                  if (pRespPDU->GetErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
                     dwResult = SNMP_ERR_NO_OBJECT;
                  else
                     dwResult = SNMP_ERR_AGENT;
                  bRunning = FALSE;
               }
               delete pRespPDU;
            }
            else
            {
               if (bVerbose)
                  WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
               bRunning = FALSE;
            }
            delete pRqPDU;
         }
      }
   }

   delete pTransport;
   return dwResult;
}


//
// Handler for enumerating indexes
//

static void HandlerIndex(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, 
                         SNMP_Variable *pVar, void *pArg)
{
   if (((INTERFACE_LIST *)pArg)->iEnumPos < ((INTERFACE_LIST *)pArg)->iNumEntries)
      ((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iEnumPos].dwIndex = pVar->GetValueAsUInt();
   ((INTERFACE_LIST *)pArg)->iEnumPos++;
}


//
// Handler for enumerating IP addresses
//

static void HandlerIpAddr(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, 
                          SNMP_Variable *pVar, void *pArg)
{
   DWORD dwIndex, dwNetMask, dwNameLen;
   DWORD oidName[MAX_OID_LEN];

   dwNameLen = pVar->GetName()->Length();
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
   oidName[dwNameLen - 5] = 3;  // Retrieve network mask for this IP
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
                &dwNetMask, sizeof(DWORD), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return;

   oidName[dwNameLen - 5] = 2;  // Retrieve interface index for this IP
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
               &dwIndex, sizeof(DWORD), FALSE, FALSE) == SNMP_ERR_SUCCESS)
   {
      int i;

      for(i = 0; i < ((INTERFACE_LIST *)pArg)->iNumEntries; i++)
         if (((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIndex == dwIndex)
         {
            if (((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpAddr != 0)
            {
               // This interface entry already filled, so we have additional IP addresses
               // on a single interface
               ((INTERFACE_LIST *)pArg)->iNumEntries++;
               ((INTERFACE_LIST *)pArg)->pInterfaces = (INTERFACE_INFO *)realloc(((INTERFACE_LIST *)pArg)->pInterfaces,
                     sizeof(INTERFACE_INFO) * ((INTERFACE_LIST *)pArg)->iNumEntries);
               memcpy(&(((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iNumEntries - 1]), 
                      &(((INTERFACE_LIST *)pArg)->pInterfaces[i]), sizeof(INTERFACE_INFO));
               if (strlen(((INTERFACE_LIST *)pArg)->pInterfaces[i].szName) < MAX_OBJECT_NAME - 4)
               {
                  char szBuffer[8];

                  sprintf(szBuffer,":%d", ((INTERFACE_LIST *)pArg)->pInterfaces[i].iNumSecondary++);
                  strcat(((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iNumEntries - 1].szName, szBuffer);
               }
               i = ((INTERFACE_LIST *)pArg)->iNumEntries - 1;
            }
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpAddr = ntohl(pVar->GetValueAsUInt());
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpNetMask = dwNetMask;
            break;
         }
   }
}


//
// Get interface list via SNMP
//

INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwVersion, DWORD dwAddr, 
                                     const char *szCommunity, DWORD dwNodeType)
{
   long i, iNumIf;
   char szOid[128], szBuffer[256];
   INTERFACE_LIST *pIfList = NULL;
   BOOL bSuccess = FALSE;

   // Get number of interfaces
   if (SnmpGet(dwVersion, dwAddr, szCommunity, ".1.3.6.1.2.1.2.1.0", NULL, 0,
                &iNumIf, sizeof(long), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return NULL;

   // Create empty list
   pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
   pIfList->iNumEntries = iNumIf;
   pIfList->iEnumPos = 0;
   pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * iNumIf);
   memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * pIfList->iNumEntries);

   // Gather interface indexes
   if (SnmpEnumerate(dwVersion, dwAddr, szCommunity, ".1.3.6.1.2.1.2.2.1.1",
                     HandlerIndex, pIfList, FALSE) == SNMP_ERR_SUCCESS)
   {
      // Enumerate interfaces
      for(i = 0; i < iNumIf; i++)
      {
         // Interface name
         sprintf(szOid, ".1.3.6.1.2.1.2.2.1.2.%d", pIfList->pInterfaces[i].dwIndex);
         if (SnmpGet(dwVersion, dwAddr, szCommunity, szOid, NULL, 0,
                      pIfList->pInterfaces[i].szName, MAX_OBJECT_NAME,
                      FALSE, FALSE) != SNMP_ERR_SUCCESS)
            break;

         // Interface type
         sprintf(szOid, ".1.3.6.1.2.1.2.2.1.3.%d", pIfList->pInterfaces[i].dwIndex);
         if (SnmpGet(dwVersion, dwAddr, szCommunity, szOid, NULL, 0,
                      &pIfList->pInterfaces[i].dwType, sizeof(DWORD),
                      FALSE, FALSE) != SNMP_ERR_SUCCESS)
            break;

         // MAC address
         sprintf(szOid, ".1.3.6.1.2.1.2.2.1.6.%d", pIfList->pInterfaces[i].dwIndex);
         memset(szBuffer, 0, MAC_ADDR_LENGTH);
         if (SnmpGet(dwVersion, dwAddr, szCommunity, szOid, NULL, 0,
                      szBuffer, 256, FALSE, TRUE) != SNMP_ERR_SUCCESS)
            break;
         memcpy(pIfList->pInterfaces[i].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
      }

      if (i == iNumIf)
      {
         // Interface IP address'es and netmasks
         if (SnmpEnumerate(dwVersion, dwAddr, szCommunity, ".1.3.6.1.2.1.4.20.1.1",
                           HandlerIpAddr, pIfList, FALSE) == SNMP_ERR_SUCCESS)
         {
            // Handle special cases
            if (dwNodeType == NODE_TYPE_NORTEL_ACCELAR)
               GetAccelarVLANIfList(dwVersion, dwAddr, szCommunity, pIfList);
            bSuccess = TRUE;
         }
      }
   }

   if (bSuccess)
   {
      CleanInterfaceList(pIfList);
   }
   else
   {
      DestroyInterfaceList(pIfList);
      pIfList = NULL;
   }

   return pIfList;
}


//
// Handler for ARP enumeration
//

static void HandlerArp(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, 
                       SNMP_Variable *pVar, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen, dwIndex = 0;
   BYTE bMac[64];

   dwNameLen = pVar->GetName()->Length();
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));

   oidName[dwNameLen - 6] = 1;  // Retrieve interface index
   SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen, &dwIndex,
           sizeof(DWORD), FALSE, FALSE);

   oidName[dwNameLen - 6] = 2;  // Retrieve MAC address for this IP
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen, bMac,
               64, FALSE, FALSE) == SNMP_ERR_SUCCESS)
   {
      ((ARP_CACHE *)pArg)->dwNumEntries++;
      ((ARP_CACHE *)pArg)->pEntries = (ARP_ENTRY *)realloc(((ARP_CACHE *)pArg)->pEntries,
               sizeof(ARP_ENTRY) * ((ARP_CACHE *)pArg)->dwNumEntries);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIpAddr = ntohl(pVar->GetValueAsUInt());
      memcpy(((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].bMacAddr, bMac, 6);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIndex = dwIndex;
   }
}


//
// Get ARP cache via SNMP
//

ARP_CACHE *SnmpGetArpCache(DWORD dwVersion, DWORD dwAddr, const char *szCommunity)
{
   ARP_CACHE *pArpCache;

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   if (pArpCache == NULL)
      return NULL;

   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = NULL;

   if (SnmpEnumerate(dwVersion, dwAddr, szCommunity, ".1.3.6.1.2.1.4.22.1.3", 
                     HandlerArp, pArpCache, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyArpCache(pArpCache);
      pArpCache = NULL;
   }
   return pArpCache;
}


//
// Get interface status via SNMP
// Possible return values can be NORMAL, CRITICAL, DISABLED, TESTING and UNKNOWN
//

int SnmpGetInterfaceStatus(DWORD dwNodeAddr, DWORD dwVersion, char *pszCommunity, DWORD dwIfIndex)
{
   DWORD dwAdminStatus = 0, dwOperStatus = 0;
   int iStatus;
   char szOid[256];

   // Interface administrative status
   sprintf(szOid, ".1.3.6.1.2.1.2.2.1.7.%ld", dwIfIndex);
   SnmpGet(dwVersion, dwNodeAddr, pszCommunity, szOid, NULL, 0,
           &dwAdminStatus, sizeof(DWORD), FALSE, FALSE);

   switch(dwAdminStatus)
   {
      case 3:
         iStatus = STATUS_TESTING;
         break;
      case 2:
         iStatus = STATUS_DISABLED;
         break;
      case 1:     // Interface administratively up, check operational status
         // Get interface operational status
         sprintf(szOid, ".1.3.6.1.2.1.2.2.1.8.%ld", dwIfIndex);
         SnmpGet(dwVersion, dwNodeAddr, pszCommunity, szOid, NULL, 0,
                 &dwOperStatus, sizeof(DWORD), FALSE, FALSE);
         switch(dwOperStatus)
         {
            case 3:
               iStatus = STATUS_TESTING;
               break;
            case 2:  // Interface is down
               iStatus = STATUS_CRITICAL;
               break;
            case 1:
               iStatus = STATUS_NORMAL;
               break;
            default:
               iStatus = STATUS_UNKNOWN;
               break;
         }
         break;
      default:
         iStatus = STATUS_UNKNOWN;
         break;
   }
   return iStatus;
}


//
// Handler for route enumeration
//

static void HandlerRoute(DWORD dwVersion, DWORD dwAddr, const char *szCommunity, 
                         SNMP_Variable *pVar, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen;
   ROUTE route;

   dwNameLen = pVar->GetName()->Length();
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
   route.dwDestAddr = ntohl(pVar->GetValueAsUInt());

   oidName[dwNameLen - 5] = 2;  // Interface index
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
               &route.dwIfIndex, sizeof(DWORD), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return;

   oidName[dwNameLen - 5] = 7;  // Next hop
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
               &route.dwNextHop, sizeof(DWORD), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return;
   route.dwNextHop = ntohl(route.dwNextHop);

   oidName[dwNameLen - 5] = 8;  // Route type
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
               &route.dwRouteType, sizeof(DWORD), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return;

   oidName[dwNameLen - 5] = 11;  // Destination mask
   if (SnmpGet(dwVersion, dwAddr, szCommunity, NULL, oidName, dwNameLen,
               &route.dwDestMask, sizeof(DWORD), FALSE, FALSE) != SNMP_ERR_SUCCESS)
      return;
   route.dwDestMask = ntohl(route.dwDestMask);

   ((ROUTING_TABLE *)pArg)->iNumEntries++;
   ((ROUTING_TABLE *)pArg)->pRoutes = 
      (ROUTE *)realloc(((ROUTING_TABLE *)pArg)->pRoutes,
                       sizeof(ROUTE) * ((ROUTING_TABLE *)pArg)->iNumEntries);
   memcpy(&((ROUTING_TABLE *)pArg)->pRoutes[((ROUTING_TABLE *)pArg)->iNumEntries - 1],
          &route, sizeof(ROUTE));
}


//
// Get routing table via SNMP
//

ROUTING_TABLE *SnmpGetRoutingTable(DWORD dwVersion, DWORD dwAddr, const char *szCommunity)
{
   ROUTING_TABLE *pRT;

   pRT = (ROUTING_TABLE *)malloc(sizeof(ROUTING_TABLE));
   if (pRT == NULL)
      return NULL;

   pRT->iNumEntries = 0;
   pRT->pRoutes = NULL;

   if (SnmpEnumerate(dwVersion, dwAddr, szCommunity, ".1.3.6.1.2.1.4.21.1.1", 
                     HandlerRoute, pRT, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyRoutingTable(pRT);
      pRT = NULL;
   }
   return pRT;
}
