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

#include "nms_core.h"


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


//
// Initialize SNMP subsystem
//

void SnmpInit(void)
{
   DB_RESULT hResult;
   DWORD i;

   init_mib();

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

   return NODE_TYPE_GENERIC;
}


//
// Convert OID to from binary to string representation
//

void OidToStr(oid *pOid, int iOidLen, char *szBuffer, DWORD dwBufferSize)
{
   int i;
   char *pCurr;

   szBuffer[0] = 0;
   for(i = 0, pCurr = szBuffer; i < iOidLen; i++)
   {
      if (strlen(szBuffer) + 4 >= dwBufferSize)
         break;   // String is too large
      sprintf(pCurr, ".%d", pOid[i]);
      while(*pCurr)
         pCurr++;
   }
}


//
// Get value for SNMP variable
// If szOidStr is not NULL, string representation of OID is used, otherwise -
// binary representation from oidBinary and iOidLen
//

BOOL SnmpGet(DWORD dwAddr, const char *szCommunity, const char *szOidStr,
             const oid *oidBinary, size_t iOidLen, void *pValue,
             DWORD dwBufferSize, BOOL bVerbose, BOOL bStringResult)
{
   struct snmp_session session;
   struct snmp_pdu *pdu, *response;
   HSNMPSESSION hSession;
   oid oidName[MAX_OID_LEN];
   size_t iNameLen = MAX_OID_LEN;
   struct variable_list *pVar;
   int iStatus;
   char szNodeName[32];
   BOOL bResult = TRUE;

   // Open SNMP session
   snmp_sess_init(&session);
   session.version = SNMP_VERSION_1;
   session.peername = IpToStr(dwAddr, szNodeName);
   session.community = (unsigned char *)szCommunity;
   session.community_len = strlen((char *)session.community);

   hSession = snmp_sess_open(&session);
   if(hSession == NULL)
   {
      WriteLog(MSG_SNMP_OPEN_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      bResult = FALSE;
   }
   else
   {
      // Create PDU and send request
      pdu = snmp_pdu_create(SNMP_MSG_GET);
      if (szOidStr != NULL)
      {
         if (read_objid(szOidStr, oidName, &iNameLen) == 0)
         {
            WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szOidStr);
            bResult = FALSE;
         }
      }
      else
      {
         memcpy(oidName, oidBinary, iOidLen * sizeof(oid));
         iNameLen = iOidLen;
      }

      if (bResult)   // Still no errors
      {
         snmp_add_null_var(pdu, oidName, iNameLen);
         iStatus = snmp_sess_synch_response(hSession, pdu, &response);

         // Analyze response
         if ((iStatus == STAT_SUCCESS) && (response->errstat == SNMP_ERR_NOERROR))
         {
            for(pVar = response->variables; pVar != NULL; pVar = pVar->next_variable)
            {
               switch(pVar->type)
               {
                  case ASN_INTEGER:
                  case ASN_UINTEGER:
                  case ASN_COUNTER:
                  case ASN_GAUGE:
                     if (bStringResult)
                        sprintf((char *)pValue, "%ld", *pVar->val.integer);
                     else
                        *((long *)pValue) = *pVar->val.integer;
                     break;
                  case ASN_IPADDRESS:
                     if (bStringResult)
                        IpToStr(ntohl(*pVar->val.integer), (char *)pValue);
                     else
                        *((long *)pValue) = ntohl(*pVar->val.integer);
                     break;
                  case ASN_OCTET_STR:
                     memcpy(pValue, pVar->val.string, min(pVar->val_len, dwBufferSize - 1));
                     ((char *)pValue)[min(pVar->val_len, dwBufferSize - 1)] = 0;
                     break;
                  case ASN_OBJECT_ID:
                     OidToStr(pVar->val.objid, pVar->val_len / sizeof(oid), (char *)pValue, dwBufferSize);
                     break;
                  default:
                     WriteLog(MSG_SNMP_UNKNOWN_TYPE, EVENTLOG_ERROR_TYPE, "d", pVar->type);
                     bResult = FALSE;
                     break;
               }
            }
         }
         else
         {
            if (iStatus == STAT_SUCCESS)
            {
               if (bVerbose)
                  WriteLog(MSG_SNMP_BAD_PACKET, EVENTLOG_ERROR_TYPE, "s", snmp_errstring(response->errstat));
               bResult = FALSE;
            }
            else
            {
               if (bVerbose)
                  WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", iStatus);
               bResult = FALSE;
            }
         }

         // Destroy response PDU if needed
         if (response)
            snmp_free_pdu(response);
      }

      // Close session and cleanup
      snmp_sess_close(hSession);
   }

   return bResult;
}


//
// Enumerate multiple values by walking throgh MIB, starting at given root
//

BOOL SnmpEnumerate(DWORD dwAddr, const char *szCommunity, const char *szRootOid,
                   void (* pHandler)(DWORD, const char *, variable_list *, void *), 
                   void *pUserArg, BOOL bVerbose)
{
   struct snmp_session session;
   struct snmp_pdu *pdu, *response;
   HSNMPSESSION hSession;
   oid oidName[MAX_OID_LEN], oidRoot[MAX_OID_LEN];
   size_t iNameLen, iRootLen = MAX_OID_LEN;
   struct variable_list *pVar;
   int iStatus;
   char szNodeName[32];
   BOOL bRunning = TRUE, bResult = TRUE;

   // Open SNMP session
   snmp_sess_init(&session);
   session.version = SNMP_VERSION_1;
   session.peername = IpToStr(dwAddr, szNodeName);
   session.community = (unsigned char *)szCommunity;
   session.community_len = strlen((char *)session.community);

   hSession = snmp_sess_open(&session);
   if(hSession == NULL)
   {
      WriteLog(MSG_SNMP_OPEN_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      return FALSE;
   }

   // Get root
   if (read_objid(szRootOid, oidRoot, &iRootLen) == 0)
   {
      WriteLog(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
      return FALSE;
   }
   memcpy(oidName, oidRoot, iRootLen * sizeof(oid));
   iNameLen = iRootLen;

   // Walk the MIB
   while(bRunning)
   {
      pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
      snmp_add_null_var(pdu, oidName, iNameLen);
      iStatus = snmp_sess_synch_response(hSession, pdu, &response);

      if ((iStatus == STAT_SUCCESS) && (response->errstat == SNMP_ERR_NOERROR))
      {
         for(pVar = response->variables; pVar != NULL; pVar = pVar->next_variable)
         {
            // Should we stop walking?
            if ((pVar->name_length < iRootLen) ||
                (memcmp(oidRoot, pVar->name, iRootLen * sizeof(oid))))
            {
               bRunning = 0;
               break;
            }
            memmove(oidName, pVar->name, pVar->name_length * sizeof(oid));
            iNameLen = pVar->name_length;

            // Call user's callback function for processing
            pHandler(dwAddr, szCommunity, pVar, pUserArg);
         }
      }
      else
      {
         if (iStatus == STAT_SUCCESS)
         {
            if (bVerbose)
               WriteLog(MSG_SNMP_BAD_PACKET, EVENTLOG_ERROR_TYPE, "s", snmp_errstring(response->errstat));
            bResult = FALSE;
         }
         else
         {
            if (bVerbose)
               WriteLog(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", iStatus);
            bResult = FALSE;
         }
         bRunning = FALSE;
      }

      // Destroy responce PDU if needed
      if (response)
      {
         snmp_free_pdu(response);
      }
   }

   // Close session and cleanup
   snmp_sess_close(hSession);
   return bResult;
}


//
// Handler for enumerating indexes
//

static void HandlerIndex(DWORD dwAddr, const char *szCommunity, variable_list *pVar,void *pArg)
{
   if (((INTERFACE_LIST *)pArg)->iEnumPos < ((INTERFACE_LIST *)pArg)->iNumEntries)
      ((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iEnumPos].dwIndex = *pVar->val.integer;
   ((INTERFACE_LIST *)pArg)->iEnumPos++;
}


//
// Handler for enumerating IP addresses
//

static void HandlerIpAddr(DWORD dwAddr, const char *szCommunity, variable_list *pVar, void *pArg)
{
   DWORD dwIndex, dwNetMask;
   oid oidName[MAX_OID_LEN];

   memcpy(oidName, pVar->name, pVar->name_length * sizeof(oid));
   oidName[pVar->name_length - 5] = 3;  // Retrieve network mask for this IP
   if (!SnmpGet(dwAddr, szCommunity, NULL, oidName, pVar->name_length,
                &dwNetMask, sizeof(DWORD), FALSE, FALSE))
      return;

   oidName[pVar->name_length - 5] = 2;  // Retrieve interface index for this IP
   if (SnmpGet(dwAddr, szCommunity, NULL, oidName, pVar->name_length,
               &dwIndex, sizeof(DWORD), FALSE, FALSE))
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
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpAddr = ntohl(*pVar->val.integer);
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpNetMask = dwNetMask;
            break;
         }
   }
}


//
// Get interface list via SNMP
//

INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwAddr, const char *szCommunity, DWORD dwNodeType)
{
   long i, iNumIf;
   char szOid[128], szBuffer[256];
   INTERFACE_LIST *pIfList = NULL;

   // Get number of interfaces
   if (!SnmpGet(dwAddr, szCommunity, ".1.3.6.1.2.1.2.1.0", NULL, 0,
                &iNumIf, sizeof(long), FALSE, FALSE))
      return NULL;

   // Create empty list
   pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
   pIfList->iNumEntries = iNumIf;
   pIfList->iEnumPos = 0;
   pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * iNumIf);
   memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * pIfList->iNumEntries);

   // Gather interface indexes
   SnmpEnumerate(dwAddr, szCommunity, ".1.3.6.1.2.1.2.2.1.1", HandlerIndex, pIfList, FALSE);

   // Enumerate interfaces
   for(i = 0; i < iNumIf; i++)
   {
      // Interface name
      sprintf(szOid, ".1.3.6.1.2.1.2.2.1.2.%d", pIfList->pInterfaces[i].dwIndex);
      if (!SnmpGet(dwAddr, szCommunity, szOid, NULL, 0,
                   pIfList->pInterfaces[i].szName, MAX_OBJECT_NAME,
                   FALSE, FALSE))
         continue;

      // Interface type
      sprintf(szOid, ".1.3.6.1.2.1.2.2.1.3.%d", pIfList->pInterfaces[i].dwIndex);
      if (!SnmpGet(dwAddr, szCommunity, szOid, NULL, 0,
                   &pIfList->pInterfaces[i].dwType, sizeof(DWORD),
                   FALSE, FALSE))
         continue;

      // MAC address
      sprintf(szOid, ".1.3.6.1.2.1.2.2.1.6.%d", pIfList->pInterfaces[i].dwIndex);
      memset(szBuffer, 0, MAC_ADDR_LENGTH);
      if (!SnmpGet(dwAddr, szCommunity, szOid, NULL, 0,
                   szBuffer, 256, FALSE, TRUE))
         continue;
      memcpy(pIfList->pInterfaces[i].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
   }

   // Interface IP address'es and netmasks
   SnmpEnumerate(dwAddr, szCommunity, ".1.3.6.1.2.1.4.20.1.1", HandlerIpAddr, pIfList, FALSE);

   // Handle special cases
   if (dwNodeType == NODE_TYPE_NORTEL_ACCELAR)
      GetAccelarVLANIfList(dwAddr, szCommunity, pIfList);

   CleanInterfaceList(pIfList);

   return pIfList;
}


//
// Handler for ARP enumeration
//

static void HandlerArp(DWORD dwAddr, const char *szCommunity, variable_list *pVar,void *pArg)
{
   oid oidName[MAX_OID_LEN];
   BYTE bMac[64];
   DWORD dwIndex = 0;

   memcpy(oidName, pVar->name, pVar->name_length * sizeof(oid));

   oidName[pVar->name_length - 6] = 1;  // Retrieve interface index
   SnmpGet(dwAddr, szCommunity, NULL, oidName, pVar->name_length, &dwIndex,
           sizeof(DWORD), FALSE, FALSE);

   oidName[pVar->name_length - 6] = 2;  // Retrieve MAC address for this IP
   if (SnmpGet(dwAddr, szCommunity, NULL, oidName, pVar->name_length, bMac,
               64, FALSE, FALSE))
   {
      ((ARP_CACHE *)pArg)->dwNumEntries++;
      ((ARP_CACHE *)pArg)->pEntries = (ARP_ENTRY *)realloc(((ARP_CACHE *)pArg)->pEntries,
               sizeof(ARP_ENTRY) * ((ARP_CACHE *)pArg)->dwNumEntries);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIpAddr = ntohl(*pVar->val.integer);
      memcpy(((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].bMacAddr, bMac, 6);
      ((ARP_CACHE *)pArg)->pEntries[((ARP_CACHE *)pArg)->dwNumEntries - 1].dwIndex = dwIndex;
   }
}


//
// Get ARP cache via SNMP
//

ARP_CACHE *SnmpGetArpCache(DWORD dwAddr, const char *szCommunity)
{
   ARP_CACHE *pArpCache;

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   if (pArpCache == NULL)
      return NULL;

   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = NULL;

   if (!SnmpEnumerate(dwAddr, szCommunity, ".1.3.6.1.2.1.4.22.1.3", HandlerArp, pArpCache, FALSE))
   {
      DestroyArpCache(pArpCache);
      pArpCache = NULL;
   }
   return pArpCache;
}
