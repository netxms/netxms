/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: snmptrap.cpp
**
**/

#include "nms_core.h"


//
// Constants
//

#define MAX_PACKET_LENGTH     65536


//
// Static data
//

static MUTEX m_mutexTrapCfgAccess = NULL;
static NXC_TRAP_CFG_ENTRY *m_pTrapCfg = NULL;
static DWORD m_dwNumTraps = 0;


//
// Load trap configuration from database
//

static BOOL LoadTrapCfg(void)
{
   DB_RESULT hResult;
   BOOL bResult = TRUE;
   TCHAR szQuery[256];
   DWORD i, j, pdwBuffer[MAX_OID_LEN];

   // Load traps
   hResult = DBSelect(g_hCoreDB, "SELECT trap_id,snmp_oid,event_id,description FROM snmp_trap_cfg ORDER BY sequence");
   if (hResult != NULL)
   {
      m_dwNumTraps = DBGetNumRows(hResult);
      m_pTrapCfg = (NXC_TRAP_CFG_ENTRY *)malloc(sizeof(NXC_TRAP_CFG_ENTRY) * m_dwNumTraps);
      memset(m_pTrapCfg, 0, sizeof(NXC_TRAP_CFG_ENTRY) * m_dwNumTraps);
      for(i = 0; i < m_dwNumTraps; i++)
      {
         m_pTrapCfg[i].dwId = DBGetFieldULong(hResult, i, 0);
         m_pTrapCfg[i].dwOidLen = SNMPParseOID(DBGetField(hResult, i, 1), pdwBuffer, MAX_OID_LEN);
         if (m_pTrapCfg[i].dwOidLen > 0)
         {
            m_pTrapCfg[i].pdwObjectId = (DWORD *)nx_memdup(pdwBuffer, m_pTrapCfg[i].dwOidLen * sizeof(DWORD));
         }
         else
         {
            WriteLog(MSG_INVALID_TRAP_OID, EVENTLOG_ERROR_TYPE, "s", DBGetField(hResult, i, 1));
            bResult = FALSE;
         }
         m_pTrapCfg[i].dwEventId = DBGetFieldULong(hResult, i, 2);
         _tcsncpy(m_pTrapCfg[i].szDescription, DBGetField(hResult, i, 3), MAX_DB_STRING);
      }
      DBFreeResult(hResult);

      // Load parameter mappings
      for(i = 0; i < m_dwNumTraps; i++)
      {
         sprintf(szQuery, "SELECT snmp_oid,description FROM snmp_trap_pmap "
                          "WHERE trap_id=%ld ORDER BY parameter", m_pTrapCfg[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            m_pTrapCfg[i].dwNumMaps = DBGetNumRows(hResult);
            m_pTrapCfg[i].pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * m_pTrapCfg[i].dwNumMaps);
            for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            {
               m_pTrapCfg[i].pMaps[j].dwOidLen = SNMPParseOID(DBGetField(hResult, j, 0), pdwBuffer, MAX_OID_LEN);
               if (m_pTrapCfg[i].pMaps[j].dwOidLen > 0)
               {
                  m_pTrapCfg[i].pMaps[j].pdwObjectId = 
                     (DWORD *)nx_memdup(pdwBuffer, m_pTrapCfg[i].pMaps[j].dwOidLen * sizeof(DWORD));
               }
               else
               {
                  WriteLog(MSG_INVALID_TRAP_ARG_OID, EVENTLOG_ERROR_TYPE, "sd", 
                           DBGetField(hResult, j, 0), m_pTrapCfg[i].dwId);
                  bResult = FALSE;
               }
               _tcsncpy(m_pTrapCfg[i].pMaps[j].szDescription, DBGetField(hResult, j, 1), MAX_DB_STRING);
            }
            DBFreeResult(hResult);
         }
         else
         {
            bResult = FALSE;
         }
      }
   }
   else
   {
      bResult = FALSE;
   }
   return bResult;
}


//
// Initialize trap handling
//

void InitTraps(void)
{
   m_mutexTrapCfgAccess = MutexCreate();
   LoadTrapCfg();
}


//
// Generate event for matched trap
//

static void GenerateTrapEvent(DWORD dwObjectId, DWORD dwIndex, SNMP_PDU *pdu)
{
   TCHAR *pszArgList[32], szBuffer[256];
   TCHAR szFormat[] = "sssssssssssssssssssssssssssssssss";
   DWORD i, j;
   int iResult;

   for(i = 0; i < m_pTrapCfg[dwIndex].dwNumMaps; i++)
   {
      for(j = 0; j < pdu->GetNumVariables(); j++)
      {
         iResult = pdu->GetVariable(j)->GetName()->Compare(
               m_pTrapCfg[dwIndex].pMaps[i].pdwObjectId,
               m_pTrapCfg[dwIndex].pMaps[i].dwOidLen);
         if ((iResult == OID_EQUAL) || (iResult == OID_SHORTER))
         {
            pszArgList[i] = _tcsdup(pdu->GetVariable(j)->GetValueAsString(szBuffer, 256));
            break;
         }
      }
      if (j == pdu->GetNumVariables())
         pszArgList[i] = _tcsdup(_T("<null>"));
   }

   szFormat[m_pTrapCfg[dwIndex].dwNumMaps + 1] = 0;
   PostEvent(m_pTrapCfg[dwIndex].dwEventId, dwObjectId, szFormat,
             pdu->GetTrapId()->GetValueAsText(),
             pszArgList[0], pszArgList[1], pszArgList[2], pszArgList[3],
             pszArgList[4], pszArgList[5], pszArgList[6], pszArgList[7],
             pszArgList[8], pszArgList[9], pszArgList[10], pszArgList[11],
             pszArgList[12], pszArgList[13], pszArgList[14], pszArgList[15],
             pszArgList[16], pszArgList[17], pszArgList[18], pszArgList[19],
             pszArgList[20], pszArgList[21], pszArgList[22], pszArgList[23],
             pszArgList[24], pszArgList[25], pszArgList[26], pszArgList[27],
             pszArgList[28], pszArgList[29], pszArgList[30], pszArgList[31]);

   for(i = 0; i < m_pTrapCfg[dwIndex].dwNumMaps; i++)
      free(pszArgList[i]);
}


//
// Process trap
//

static void ProcessTrap(SNMP_PDU *pdu, struct sockaddr_in *pOrigin)
{
   DWORD i, dwOriginAddr, dwBufPos, dwBufSize;
   TCHAR *pszTrapArgs, szBuffer[512];
   SNMP_Variable *pVar;
   Node *pNode;
   int iResult;

   dwOriginAddr = ntohl(pOrigin->sin_addr.s_addr);
   DbgPrintf(AF_DEBUG_SNMP, "Received SNMP trap %s from %s", 
             pdu->GetTrapId()->GetValueAsText(), IpToStr(dwOriginAddr, szBuffer));

   // Match IP address to object
   pNode = FindNodeByIP(dwOriginAddr);
   if (pNode != NULL)
   {
      // Find if we have this trap in our list
      MutexLock(m_mutexTrapCfgAccess, INFINITE);
      for(i = 0; i < m_dwNumTraps; i++)
      {
         iResult = pdu->GetTrapId()->Compare(m_pTrapCfg[i].pdwObjectId, m_pTrapCfg[i].dwOidLen);
         if ((iResult == OID_EQUAL) || (iResult == OID_SHORTER))
            break;   // Find the match
      }

      if (i < m_dwNumTraps)
      {
         GenerateTrapEvent(pNode->Id(), i, pdu);
      }
      else     // Process unmatched traps
      {
         // Build trap's parameters string
         dwBufSize = pdu->GetNumVariables() * 4096 + 16;
         pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
         pszTrapArgs[0] = 0;
         for(i = 0, dwBufPos = 0; i < pdu->GetNumVariables(); i++)
         {
            pVar = pdu->GetVariable(i);
            dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                   (i == 0) ? _T("") : _T("; "),
                                   pVar->GetName()->GetValueAsText(), 
                                   pVar->GetValueAsString(szBuffer, 512));
         }

         // Generate default event for unmatched traps
         PostEvent(EVENT_SNMP_UNMATCHED_TRAP, pNode->Id(), "ss", 
                   pdu->GetTrapId()->GetValueAsText(), pszTrapArgs);
         free(pszTrapArgs);
      }
      MutexUnlock(m_mutexTrapCfgAccess);
   }
}


//
// SNMP trap receiver thread
//

THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg)
{
   SOCKET hSocket;
   struct sockaddr_in addr;
   int iBytes;
   socklen_t nAddrLen;
   SNMP_Transport *pTransport;
   SNMP_PDU *pdu;

   hSocket = socket(AF_INET, SOCK_DGRAM, 0);
   if (hSocket == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "SNMPTrapReceiver");
      return THREAD_OK;
   }

   // Fill in local address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(162);

   // Bind socket
   if (bind(hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", 162, "SNMPTrapReceiver", WSAGetLastError());
      closesocket(hSocket);
      return THREAD_OK;
   }

   pTransport = new SNMP_Transport(hSocket);
   DbgPrintf(AF_DEBUG_SNMP, _T("SNMP Trap Receiver started"));

   // Wait for packets
   while(!ShutdownInProgress())
   {
      nAddrLen = sizeof(struct sockaddr_in);
      iBytes = pTransport->Read(&pdu, 2000, (struct sockaddr *)&addr, &nAddrLen);
      if ((iBytes > 0) && (pdu != NULL))
      {
         if (pdu->GetCommand() == SNMP_TRAP)
            ProcessTrap(pdu, &addr);
         delete pdu;
      }
      else
      {
         // Sleep on error
         ThreadSleepMs(100);
      }
   }

   DbgPrintf(AF_DEBUG_SNMP, _T("SNMP Trap Receiver terminated"));
   return THREAD_OK;
}


//
// Send all traps to client
//

void SendTrapsToClient(ClientSession *pSession, DWORD dwRqId)
{
   DWORD i, j, dwId1, dwId2, dwId3;
   CSCPMessage msg;

   // Prepare message
   msg.SetCode(CMD_TRAP_CFG_RECORD);
   msg.SetId(dwRqId);

   MutexLock(m_mutexTrapCfgAccess, INFINITE);
   for(i = 0; i < m_dwNumTraps; i++)
   {
      msg.SetVariable(VID_TRAP_ID, m_pTrapCfg[i].dwId);
      msg.SetVariable(VID_TRAP_OID_LEN, m_pTrapCfg[i].dwOidLen); 
      msg.SetVariableToInt32Array(VID_TRAP_OID, m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId);
      msg.SetVariable(VID_EVENT_ID, m_pTrapCfg[i].dwEventId);
      msg.SetVariable(VID_DESCRIPTION, m_pTrapCfg[i].szDescription);
      msg.SetVariable(VID_TRAP_NUM_MAPS, m_pTrapCfg[i].dwNumMaps);
      for(j = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE; 
          j < m_pTrapCfg[i].dwNumMaps; j++, dwId1++, dwId2++)
      {
         msg.SetVariable(dwId1, m_pTrapCfg[i].pMaps[j].dwOidLen);
         msg.SetVariableToInt32Array(dwId2, m_pTrapCfg[i].pMaps[j].dwOidLen, m_pTrapCfg[i].pMaps[j].pdwObjectId);
         msg.SetVariable(dwId3, m_pTrapCfg[i].pMaps[j].szDescription);
      }
      pSession->SendMessage(&msg);
      msg.DeleteAllVariables();
   }
   MutexUnlock(m_mutexTrapCfgAccess);

   msg.SetVariable(VID_TRAP_ID, (DWORD)0);
   pSession->SendMessage(&msg);
}


//
// Delete trap configuration record
//

DWORD DeleteTrap(DWORD dwId)
{
   DWORD i, j, dwResult = RCC_INVALID_TRAP_ID;
   TCHAR szQuery[256];

   MutexLock(m_mutexTrapCfgAccess, INFINITE);
   
   for(i = 0; i < m_dwNumTraps; i++)
   {
      if (m_pTrapCfg[i].dwId == dwId)
      {
         // Free allocated resources
         for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            safe_free(m_pTrapCfg[i].pMaps[j].pdwObjectId);
         safe_free(m_pTrapCfg[i].pMaps);
         safe_free(m_pTrapCfg[i].pdwObjectId);

         // Remove trap entry from list
         m_dwNumTraps--;
         memmove(&m_pTrapCfg[i], &m_pTrapCfg[i + 1], sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps - i));

         // Remove trap entry from database
         _stprintf(szQuery, _T("DELETE FROM snmp_trap_cfg WHERE trap_id=%ld"), dwId);
         QueueSQLRequest(szQuery);
         _stprintf(szQuery, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=%ld"), dwId);
         QueueSQLRequest(szQuery);
         dwResult = RCC_SUCCESS;
         break;
      }
   }

   MutexUnlock(m_mutexTrapCfgAccess);
   return dwResult;
}
