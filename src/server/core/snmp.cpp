/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
// Generate new request ID
//

DWORD SnmpNewRequestId(void)
{
   return m_dwRequestId++;
}


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
         m_pOidTable[i].pszOid = DBGetField(hResult, i, 0, NULL, 0);
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

DWORD SnmpGet(DWORD dwVersion, SNMP_Transport *pTransport,
              const TCHAR *szOidStr, const DWORD *oidBinary, DWORD dwOidLen, void *pValue,
              DWORD dwBufferSize, DWORD dwFlags)
{
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD dwNameLen, pdwVarName[MAX_OID_LEN], dwResult = SNMP_ERR_SUCCESS;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Create PDU and send request
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, m_dwRequestId++, dwVersion);
   if (szOidStr != NULL)
   {
      dwNameLen = SNMPParseOID(szOidStr, pdwVarName, MAX_OID_LEN);
      if (dwNameLen == 0)
      {
         nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szOidStr);
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
      pRqPDU->bindVariable(new SNMP_Variable(pdwVarName, dwNameLen));
      dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);

      // Analyze response
      if (dwResult == SNMP_ERR_SUCCESS)
      {
         if ((pRespPDU->getNumVariables() > 0) &&
             (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = pRespPDU->getVariable(0);

            if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
            {
               if (dwFlags & SG_RAW_RESULT)
               {
						pVar->getRawValue((BYTE *)pValue, dwBufferSize);
               }
               else if (dwFlags & SG_HSTRING_RESULT)
               {
						int rawLen = (dwBufferSize - 1) / 2;
						BYTE *raw = (BYTE *)malloc(rawLen);
						rawLen = pVar->getRawValue(raw, rawLen);
						BinToStr(raw, rawLen, (TCHAR *)pValue);
						free(raw);
               }
               else if (dwFlags & SG_STRING_RESULT)
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
                        *((LONG *)pValue) = pVar->GetValueAsInt();
                        break;
                     case ASN_IP_ADDR:
                        *((DWORD *)pValue) = ntohl(pVar->GetValueAsUInt());
                        break;
                     case ASN_OCTET_STRING:
                        pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                        break;
                     case ASN_OBJECT_ID:
                        pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                        break;
                     default:
                        nxlog_write(MSG_SNMP_UNKNOWN_TYPE, EVENTLOG_ERROR_TYPE, "x", pVar->GetType());
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
            if (pRespPDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
               dwResult = SNMP_ERR_NO_OBJECT;
            else
               dwResult = SNMP_ERR_AGENT;
         }
         delete pRespPDU;
      }
      else
      {
         if (dwFlags & SG_VERBOSE)
            nxlog_write(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
      }
   }

   delete pRqPDU;
   return dwResult;
}


//
// Enumerate multiple values by walking through MIB, starting at given root
//

DWORD SnmpEnumerate(DWORD dwVersion, SNMP_Transport *pTransport, const TCHAR *szRootOid,
                    DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *),
                    void *pUserArg, BOOL bVerbose)
{
   DWORD pdwRootName[MAX_OID_LEN], dwRootLen, pdwName[MAX_OID_LEN], dwNameLen, dwResult;
   SNMP_PDU *pRqPDU, *pRespPDU;
   BOOL bRunning = TRUE;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Get root
   dwRootLen = SNMPParseOID(szRootOid, pdwRootName, MAX_OID_LEN);
   if (dwRootLen == 0)
   {
      nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
      dwResult = SNMP_ERR_BAD_OID;
   }
   else
   {
      memcpy(pdwName, pdwRootName, dwRootLen * sizeof(DWORD));
      dwNameLen = dwRootLen;

      // Walk the MIB
      while(bRunning)
      {
         pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, m_dwRequestId++, dwVersion);
         pRqPDU->bindVariable(new SNMP_Variable(pdwName, dwNameLen));
         dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);

         // Analyze response
         if (dwResult == SNMP_ERR_SUCCESS)
         {
            if ((pRespPDU->getNumVariables() > 0) &&
                (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
            {
               SNMP_Variable *pVar = pRespPDU->getVariable(0);

               if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                   (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
               {
                  // Should we stop walking?
                  if ((pVar->GetName()->Length() < dwRootLen) ||
                      (memcmp(pdwRootName, pVar->GetName()->GetValue(), dwRootLen * sizeof(DWORD))) ||
                      ((pVar->GetName()->Length() == dwNameLen) &&
                       (!memcmp(pVar->GetName()->GetValue(), pdwName, pVar->GetName()->Length() * sizeof(DWORD)))))
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
                  dwResult = pHandler(dwVersion, pVar, pTransport, pUserArg);
                  if (dwResult != SNMP_ERR_SUCCESS)
                  {
                     bRunning = FALSE;
                  }
               }
               else
               {
                  dwResult = SNMP_ERR_NO_OBJECT;
                  bRunning = FALSE;
               }
            }
            else
            {
               if (pRespPDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
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
               nxlog_write(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
            bRunning = FALSE;
         }
         delete pRqPDU;
      }
   }
   return dwResult;
}


//
// Handler for enumerating indexes
//

static DWORD HandlerIndex(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   if (((INTERFACE_LIST *)pArg)->iEnumPos < ((INTERFACE_LIST *)pArg)->iNumEntries)
      ((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iEnumPos].dwIndex = pVar->GetValueAsUInt();
   ((INTERFACE_LIST *)pArg)->iEnumPos++;
   return SNMP_ERR_SUCCESS;
}


//
// Handler for enumerating IP addresses
//

static DWORD HandlerIpAddr(DWORD dwVersion, SNMP_Variable *pVar,
                           SNMP_Transport *pTransport, void *pArg)
{
   DWORD dwIndex, dwNetMask, dwNameLen, dwResult;
   DWORD oidName[MAX_OID_LEN];

   dwNameLen = pVar->GetName()->Length();
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
   oidName[dwNameLen - 5] = 3;  // Retrieve network mask for this IP
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, &dwNetMask, sizeof(DWORD), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 5] = 2;  // Retrieve interface index for this IP
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, &dwIndex, sizeof(DWORD), 0);
   if (dwResult == SNMP_ERR_SUCCESS)
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
               if (_tcslen(((INTERFACE_LIST *)pArg)->pInterfaces[i].szName) < MAX_OBJECT_NAME - 6)
               {
                  TCHAR szBuffer[8];

                  _sntprintf(szBuffer, 8, _T(":%d"), ((INTERFACE_LIST *)pArg)->pInterfaces[i].iNumSecondary++);
                  _tcscat(((INTERFACE_LIST *)pArg)->pInterfaces[((INTERFACE_LIST *)pArg)->iNumEntries - 1].szName, szBuffer);
               }
               i = ((INTERFACE_LIST *)pArg)->iNumEntries - 1;
            }
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpAddr = ntohl(pVar->GetValueAsUInt());
            ((INTERFACE_LIST *)pArg)->pInterfaces[i].dwIpNetMask = dwNetMask;
            break;
         }
   }
   return dwResult;
}


//
// Get interface list via SNMP
//

INTERFACE_LIST *SnmpGetInterfaceList(DWORD dwVersion, SNMP_Transport *pTransport, Node *node, BOOL useIfXTable)
{
   LONG i, iNumIf;
   TCHAR szOid[128], szBuffer[256];
   INTERFACE_LIST *pIfList = NULL;
   int useAliases;
   BOOL bSuccess = FALSE;

   // Get number of interfaces
   if (SnmpGet(dwVersion, pTransport, _T(".1.3.6.1.2.1.2.1.0"), NULL, 0,
                &iNumIf, sizeof(LONG), 0) != SNMP_ERR_SUCCESS)
      return NULL;
      
	// Using aliases
	useAliases = ConfigReadInt(_T("UseInterfaceAliases"), 0);

   // Create empty list
   pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
   pIfList->iNumEntries = iNumIf;
   pIfList->iEnumPos = 0;
   pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * iNumIf);
   memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * pIfList->iNumEntries);

   // Gather interface indexes
   if (SnmpEnumerate(dwVersion, pTransport, _T(".1.3.6.1.2.1.2.2.1.1"),
                     HandlerIndex, pIfList, FALSE) == SNMP_ERR_SUCCESS)
   {
      // Some devices can report incorrect total number of interfaces
      // I have seen it on 3COM SuperStack II 3300
      if (pIfList->iEnumPos < iNumIf)
      {
         pIfList->iNumEntries = pIfList->iEnumPos;
         iNumIf = pIfList->iEnumPos;
      }

      // Enumerate interfaces
      for(i = 0; i < iNumIf; i++)
      {
         // Get interface alias if needed
         if (useAliases > 0)
         {
		      _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.31.1.1.1.18.%d"), pIfList->pInterfaces[i].dwIndex);
		      if (SnmpGet(dwVersion, pTransport, szOid, NULL, 0,
		                   pIfList->pInterfaces[i].szName, MAX_DB_STRING, 0) != SNMP_ERR_SUCCESS)
				{
					pIfList->pInterfaces[i].szName[0] = 0;		// It's not an error if we cannot get interface alias
				}
				else
				{
					StrStrip(pIfList->pInterfaces[i].szName);
				}
         }

			// Try to get interface name from ifXTable, if unsuccessful or disabled, use ifTable
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%d"), pIfList->pInterfaces[i].dwIndex);
         if (!useIfXTable ||
				 (SnmpGet(dwVersion, pTransport, szOid, NULL, 0, szBuffer, 256, 0) != SNMP_ERR_SUCCESS))
         {
		      _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.2.%d"), pIfList->pInterfaces[i].dwIndex);
		      if (SnmpGet(dwVersion, pTransport, szOid, NULL, 0, szBuffer, 256, 0) != SNMP_ERR_SUCCESS)
		         break;
		   }

			// Build full interface object name
         switch(useAliases)
         {
         	case 1:	// Use only alias if available, otherwise name
         		if (pIfList->pInterfaces[i].szName[0] == 0)
	         		nx_strncpy(pIfList->pInterfaces[i].szName, szBuffer, MAX_DB_STRING);	// Alias is empty or not available
         		break;
         	case 2:	// Concatenate alias with name
         	case 3:	// Concatenate name with alias
         		if (pIfList->pInterfaces[i].szName[0] != 0)
         		{
						if (useAliases == 2)
						{
         				if  (_tcslen(pIfList->pInterfaces[i].szName) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&pIfList->pInterfaces[i].szName[_tcslen(pIfList->pInterfaces[i].szName)], MAX_DB_STRING - _tcslen(pIfList->pInterfaces[i].szName), _T(" (%s)"), szBuffer);
		      				pIfList->pInterfaces[i].szName[MAX_DB_STRING - 1] = 0;
		      			}
						}
						else
						{
							TCHAR temp[MAX_DB_STRING];

							_tcscpy(temp, pIfList->pInterfaces[i].szName);
		         		nx_strncpy(pIfList->pInterfaces[i].szName, szBuffer, MAX_DB_STRING);
         				if  (_tcslen(pIfList->pInterfaces[i].szName) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&pIfList->pInterfaces[i].szName[_tcslen(pIfList->pInterfaces[i].szName)], MAX_DB_STRING - _tcslen(pIfList->pInterfaces[i].szName), _T(" (%s)"), temp);
		      				pIfList->pInterfaces[i].szName[MAX_DB_STRING - 1] = 0;
		      			}
						}
         		}
         		else
         		{
	         		nx_strncpy(pIfList->pInterfaces[i].szName, szBuffer, MAX_DB_STRING);	// Alias is empty or not available
					}
         		break;
         	default:	// Use only name
         		nx_strncpy(pIfList->pInterfaces[i].szName, szBuffer, MAX_DB_STRING);
         		break;
         }

         // Interface type
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.3.%d"), pIfList->pInterfaces[i].dwIndex);
         if (SnmpGet(dwVersion, pTransport, szOid, NULL, 0,
                      &pIfList->pInterfaces[i].dwType, sizeof(DWORD), 0) != SNMP_ERR_SUCCESS)
			{
				pIfList->pInterfaces[i].dwType = IFTYPE_OTHER;
			}

         // MAC address
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.6.%d"), pIfList->pInterfaces[i].dwIndex);
         memset(szBuffer, 0, MAC_ADDR_LENGTH);
         if (SnmpGet(dwVersion, pTransport, szOid, NULL, 0, szBuffer, 256, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			{
	         memcpy(pIfList->pInterfaces[i].bMacAddr, szBuffer, MAC_ADDR_LENGTH);
			}
			else
			{
				// Unable to get MAC address
	         memset(pIfList->pInterfaces[i].bMacAddr, 0, MAC_ADDR_LENGTH);
			}
      }

      if (i == iNumIf)
      {
         // Interface IP address'es and netmasks
         if (SnmpEnumerate(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.20.1.1"),
                           HandlerIpAddr, pIfList, FALSE) == SNMP_ERR_SUCCESS)
         {
            // Handle special cases
				if (node->getNodeType() == NODE_TYPE_NORTEL_ACCELAR)
				{
					// Calculate slot/port pair from ifIndex
					for(i = 0; i < pIfList->iNumEntries; i++)
					{
						DWORD slot = pIfList->pInterfaces[i].dwIndex / 64;
						if ((slot > 0) && (slot <= 10))
						{
							pIfList->pInterfaces[i].dwSlotNumber = slot;
							pIfList->pInterfaces[i].dwPortNumber = pIfList->pInterfaces[i].dwIndex % 64 + 1;
						}
					}

               GetAccelarVLANIfList(dwVersion, pTransport, pIfList);
				}

				// Nortel BayStack hack - move to driver
				if (!_tcsncmp(node->getObjectId(), _T(".1.3.6.1.4.1.45.3"), 17))
				{
					DWORD slotSize;
					if (!_tcsncmp(node->getObjectId(), _T(".1.3.6.1.4.1.45.3.74"), 20))	// 56xx
					{
						slotSize = 128;
					}
					else if (!_tcsncmp(node->getObjectId(), _T(".1.3.6.1.4.1.45.3.40"), 20))	// BPS2000
					{
						slotSize = 32;
					}
					else
					{
						slotSize = 64;
					}

					// Calculate slot/port pair from ifIndex
					for(i = 0; i < pIfList->iNumEntries; i++)
					{
						DWORD slot = pIfList->pInterfaces[i].dwIndex / slotSize + 1;
						if ((slot > 0) && (slot <= 8))
						{
							pIfList->pInterfaces[i].dwSlotNumber = slot;
							pIfList->pInterfaces[i].dwPortNumber = pIfList->pInterfaces[i].dwIndex % slotSize;
						}
					}

					// Modern switches (55xx, 56xx) supports rapidCity MIBs
               GetAccelarVLANIfList(dwVersion, pTransport, pIfList);

					DWORD mgmtIpAddr, mgmtNetMask;

					if ((SnmpGet(dwVersion, pTransport, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.2.1"), NULL, 0, &mgmtIpAddr, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS) &&
					    (SnmpGet(dwVersion, pTransport, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.3.1"), NULL, 0, &mgmtNetMask, sizeof(DWORD), 0) == SNMP_ERR_SUCCESS))
					{
						int index = pIfList->iNumEntries;
						pIfList->iNumEntries++;
						pIfList->pInterfaces = (INTERFACE_INFO *)realloc(pIfList->pInterfaces, sizeof(INTERFACE_INFO) * pIfList->iNumEntries);
						memset(&pIfList->pInterfaces[index], 0, sizeof(INTERFACE_INFO));
						pIfList->pInterfaces[index].dwIpAddr = mgmtIpAddr;
						pIfList->pInterfaces[index].dwIpNetMask = 0;//mgmtNetMask;
						pIfList->pInterfaces[index].dwType = IFTYPE_OTHER;
						_tcscpy(pIfList->pInterfaces[index].szName, _T("mgmt"));
						SnmpGet(dwVersion, pTransport, _T(".1.3.6.1.4.1.45.1.6.4.2.2.1.10.1"), NULL, 0, pIfList->pInterfaces[index].bMacAddr, MAC_ADDR_LENGTH, SG_RAW_RESULT);

						// Update wrongly reported MAC addresses
						for(i = 0; i < pIfList->iNumEntries; i++)
						{
							if ((pIfList->pInterfaces[i].dwSlotNumber != 0) &&
								 (!memcmp(pIfList->pInterfaces[i].bMacAddr, "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH) ||
							     !memcmp(pIfList->pInterfaces[i].bMacAddr, pIfList->pInterfaces[index].bMacAddr, MAC_ADDR_LENGTH)))
							{
								memcpy(pIfList->pInterfaces[i].bMacAddr, pIfList->pInterfaces[index].bMacAddr, MAC_ADDR_LENGTH);
								pIfList->pInterfaces[i].bMacAddr[5] += (BYTE)pIfList->pInterfaces[i].dwPortNumber;
							}
						}
					}
				}

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

static DWORD HandlerArp(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen, dwIndex = 0;
   BYTE bMac[64];
   DWORD dwResult;

   dwNameLen = pVar->GetName()->Length();
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));

   oidName[dwNameLen - 6] = 1;  // Retrieve interface index
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, &dwIndex, sizeof(DWORD), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
      return dwResult;

   oidName[dwNameLen - 6] = 2;  // Retrieve MAC address for this IP
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, dwNameLen, bMac, 64, 0);
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


//
// Get ARP cache via SNMP
//

ARP_CACHE *SnmpGetArpCache(DWORD dwVersion, SNMP_Transport *pTransport)
{
   ARP_CACHE *pArpCache;

   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   if (pArpCache == NULL)
      return NULL;

   pArpCache->dwNumEntries = 0;
   pArpCache->pEntries = NULL;

   if (SnmpEnumerate(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.22.1.3"), 
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

int SnmpGetInterfaceStatus(DWORD dwVersion, SNMP_Transport *pTransport, DWORD dwIfIndex)
{
   DWORD dwAdminStatus = 0, dwOperStatus = 0;
   int iStatus;
   TCHAR szOid[256];

   // Interface administrative status
   _sntprintf(szOid, 256, _T(".1.3.6.1.2.1.2.2.1.7.%d"), dwIfIndex);
   SnmpGet(dwVersion, pTransport, szOid, NULL, 0, &dwAdminStatus, sizeof(DWORD), 0);

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
         _sntprintf(szOid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%d"), dwIfIndex);
         SnmpGet(dwVersion, pTransport, szOid, NULL, 0, &dwOperStatus, sizeof(DWORD), 0);
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

static DWORD HandlerRoute(DWORD dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   DWORD oidName[MAX_OID_LEN], dwNameLen, dwResult;
   ROUTE route;
	ROUTING_TABLE *rt = (ROUTING_TABLE *)pArg;

   dwNameLen = pVar->GetName()->Length();
	if ((dwNameLen < 5) || (dwNameLen > MAX_OID_LEN))
	{
		DbgPrintf(4, _T("HandlerRoute(): strange dwNameLen %d (name=%s)"), dwNameLen, pVar->GetName()->GetValueAsText());
		return SNMP_ERR_SUCCESS;
	}
   memcpy(oidName, pVar->GetName()->GetValue(), dwNameLen * sizeof(DWORD));
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


//
// Get routing table via SNMP
//

ROUTING_TABLE *SnmpGetRoutingTable(DWORD dwVersion, SNMP_Transport *pTransport)
{
   ROUTING_TABLE *pRT;

   pRT = (ROUTING_TABLE *)malloc(sizeof(ROUTING_TABLE));
   if (pRT == NULL)
      return NULL;

   pRT->iNumEntries = 0;
   pRT->pRoutes = NULL;

   if (SnmpEnumerate(dwVersion, pTransport, _T(".1.3.6.1.2.1.4.21.1.1"), 
                     HandlerRoute, pRT, FALSE) != SNMP_ERR_SUCCESS)
   {
      DestroyRoutingTable(pRT);
      pRT = NULL;
   }
   return pRT;
}


//
// Check SNMP v3 connectivity
//

static SNMP_SecurityContext *SnmpCheckV3CommSettings(SNMP_Transport *pTransport, SNMP_SecurityContext *originalContext)
{
	char buffer[1024];

	// Check original SNMP V3 settings, if set
	if ((originalContext != NULL) && (originalContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		DbgPrintf(5, _T("SnmpCheckV3CommSettings: trying %hs/%d:%d"), originalContext->getUser(),
		          originalContext->getAuthMethod(), originalContext->getPrivMethod());
		pTransport->setSecurityContext(new SNMP_SecurityContext(originalContext));
		if (SnmpGet(SNMP_VERSION_3, pTransport, _T(".1.3.6.1.2.1.1.2.0"), NULL,
						0, buffer, 1024, 0) == SNMP_ERR_SUCCESS)
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


//
// Determine SNMP parameters for node
// On success, returns new security context object (dynamically created).
// On failure, returns NULL
//

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
