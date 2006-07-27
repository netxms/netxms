/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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

#include "nxcore.h"


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
static BOOL m_bLogAllTraps = FALSE;
static INT64 m_qnTrapId = 1;


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
   hResult = DBSelect(g_hCoreDB, "SELECT trap_id,snmp_oid,event_code,description FROM snmp_trap_cfg");
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
         m_pTrapCfg[i].dwEventCode = DBGetFieldULong(hResult, i, 2);
         nx_strncpy(m_pTrapCfg[i].szDescription, DBGetField(hResult, i, 3), MAX_DB_STRING);
         DecodeSQLString(m_pTrapCfg[i].szDescription);
      }
      DBFreeResult(hResult);

      // Load parameter mappings
      for(i = 0; i < m_dwNumTraps; i++)
      {
         sprintf(szQuery, "SELECT snmp_oid,description FROM snmp_trap_pmap "
                          "WHERE trap_id=%d ORDER BY parameter", m_pTrapCfg[i].dwId);
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
               nx_strncpy(m_pTrapCfg[i].pMaps[j].szDescription, DBGetField(hResult, j, 1), MAX_DB_STRING);
               DecodeSQLString(m_pTrapCfg[i].pMaps[j].szDescription);
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
   DB_RESULT hResult;

   m_mutexTrapCfgAccess = MutexCreate();
   LoadTrapCfg();
   m_bLogAllTraps = ConfigReadInt(_T("LogAllSNMPTraps"), FALSE);

   hResult = DBSelect(g_hCoreDB, "SELECT max(trap_id) FROM snmp_trap_log");
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         m_qnTrapId = DBGetFieldInt64(hResult, 0, 0) + 1;
      DBFreeResult(hResult);
   }
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
   PostEvent(m_pTrapCfg[dwIndex].dwEventCode, dwObjectId, szFormat,
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
// Handler for EnumerateSessions()
//

static void BroadcastNewTrap(ClientSession *pSession, void *pArg)
{
   pSession->OnNewSNMPTrap((CSCPMessage *)pArg);
}


//
// Process trap
//

static void ProcessTrap(SNMP_PDU *pdu, struct sockaddr_in *pOrigin)
{
   DWORD i, dwOriginAddr, dwBufPos, dwBufSize, dwMatchLen, dwMatchIdx;
   TCHAR *pszTrapArgs, szBuffer[4096];
   SNMP_Variable *pVar;
   Node *pNode;
   int iResult;

   dwOriginAddr = ntohl(pOrigin->sin_addr.s_addr);
   DbgPrintf(AF_DEBUG_SNMP, "Received SNMP trap %s from %s", 
             pdu->GetTrapId()->GetValueAsText(), IpToStr(dwOriginAddr, szBuffer));

   // Match IP address to object
   pNode = FindNodeByIP(dwOriginAddr);

   // Write trap to log if required
   if (m_bLogAllTraps)
   {
      CSCPMessage msg;
      TCHAR *pszEscTrapArgs, szQuery[8192];
      DWORD dwTimeStamp = (DWORD)time(NULL);

      dwBufSize = pdu->GetNumVariables() * 4096 + 16;
      pszTrapArgs = (TCHAR *)malloc(sizeof(TCHAR) * dwBufSize);
      pszTrapArgs[0] = 0;
      for(i = 0, dwBufPos = 0; i < pdu->GetNumVariables(); i++)
      {
         pVar = pdu->GetVariable(i);
         dwBufPos += _sntprintf(&pszTrapArgs[dwBufPos], dwBufSize - dwBufPos, _T("%s%s == '%s'"),
                                (i == 0) ? _T("") : _T("; "),
                                pVar->GetName()->GetValueAsText(), 
                                pVar->GetValueAsString(szBuffer, 3000));
      }

      // Write new trap to database
      pszEscTrapArgs = EncodeSQLString(pszTrapArgs);
      _sntprintf(szQuery, 8192, _T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,")
                                _T("ip_addr,object_id,trap_oid,trap_varlist) VALUES ")
                                _T("(") INT64_FMT _T(",%d,'%s',%d,'%s','%s')"),
                 m_qnTrapId, dwTimeStamp, IpToStr(dwOriginAddr, szBuffer),
                 (pNode != NULL) ? pNode->Id() : (DWORD)0, pdu->GetTrapId()->GetValueAsText(),
                 pszEscTrapArgs);
      free(pszEscTrapArgs);
      QueueSQLRequest(szQuery);

      // Notify connected clients
      msg.SetCode(CMD_TRAP_LOG_RECORDS);
      msg.SetVariable(VID_NUM_RECORDS, (DWORD)1);
      msg.SetVariable(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE, (QWORD)m_qnTrapId);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 1, dwTimeStamp);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 2, dwOriginAddr);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 3, (pNode != NULL) ? pNode->Id() : (DWORD)0);
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 4, (TCHAR *)pdu->GetTrapId()->GetValueAsText());
      msg.SetVariable(VID_TRAP_LOG_MSG_BASE + 5, pszTrapArgs);
      EnumerateClientSessions(BroadcastNewTrap, &msg);
      free(pszTrapArgs);

      m_qnTrapId++;
   }

   // Process trap if it is coming from host registered in database
   if (pNode != NULL)
   {
      // Find if we have this trap in our list
      MutexLock(m_mutexTrapCfgAccess, INFINITE);

      // Try to find closest match
      for(i = 0, dwMatchLen = 0; i < m_dwNumTraps; i++)
      {
         if (m_pTrapCfg[i].dwOidLen > 0)
         {
            iResult = pdu->GetTrapId()->Compare(m_pTrapCfg[i].pdwObjectId, m_pTrapCfg[i].dwOidLen);
            if (iResult == OID_EQUAL)
            {
               dwMatchLen = m_pTrapCfg[i].dwOidLen;
               dwMatchIdx = i;
               break;   // Find exact match
            }
            if (iResult == OID_SHORTER)
            {
               if (m_pTrapCfg[i].dwOidLen > dwMatchLen)
               {
                  dwMatchLen = m_pTrapCfg[i].dwOidLen;
                  dwMatchIdx = i;
               }
            }
         }
      }

      if (dwMatchLen > 0)
      {
         GenerateTrapEvent(pNode->Id(), dwMatchIdx, pdu);
      }
      else     // Process unmatched traps
      {
         // Pass trap to loaded modules
         for(i = 0; i < g_dwNumModules; i++)
            if (g_pModuleList[i].pfTrapHandler != NULL)
               if (g_pModuleList[i].pfTrapHandler(pdu, pNode))
                  break;   // Message was processed by the module

         // Handle unprocessed traps
         if (i == g_dwNumModules)
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

	SetSocketReuseFlag(hSocket);

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

   delete pTransport;
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
      msg.SetVariable(VID_EVENT_CODE, m_pTrapCfg[i].dwEventCode);
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
         _stprintf(szQuery, _T("DELETE FROM snmp_trap_cfg WHERE trap_id=%d"), dwId);
         QueueSQLRequest(szQuery);
         _stprintf(szQuery, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=%d"), dwId);
         QueueSQLRequest(szQuery);
         dwResult = RCC_SUCCESS;
         break;
      }
   }

   MutexUnlock(m_mutexTrapCfgAccess);
   return dwResult;
}


//
// Create new trap configuration record
//

DWORD CreateNewTrap(DWORD *pdwTrapId)
{
   DWORD dwResult = RCC_SUCCESS;
   TCHAR szQuery[256];

   MutexLock(m_mutexTrapCfgAccess, INFINITE);
   
   *pdwTrapId = CreateUniqueId(IDG_SNMP_TRAP);
   m_pTrapCfg = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapCfg, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
   memset(&m_pTrapCfg[m_dwNumTraps], 0, sizeof(NXC_TRAP_CFG_ENTRY));
   m_pTrapCfg[m_dwNumTraps].dwId = *pdwTrapId;
   m_pTrapCfg[m_dwNumTraps].dwEventCode = EVENT_SNMP_UNMATCHED_TRAP;
   m_dwNumTraps++;

   MutexUnlock(m_mutexTrapCfgAccess);

   _stprintf(szQuery, _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_code,description) ")
                      _T("VALUES (%d,'',%d,'')"), *pdwTrapId, (DWORD)EVENT_SNMP_UNMATCHED_TRAP);
   if (!DBQuery(g_hCoreDB, szQuery))
      dwResult = RCC_DB_FAILURE;

   return dwResult;
}


//
// Update trap configuration record from message
//

DWORD UpdateTrapFromMsg(CSCPMessage *pMsg)
{
   DWORD i, j, dwId1, dwId2, dwId3, dwTrapId, dwResult = RCC_INVALID_TRAP_ID;
   TCHAR szQuery[1024], szOID[1024], *pszEscDescr;
   BOOL bSuccess;

   dwTrapId = pMsg->GetVariableLong(VID_TRAP_ID);

   MutexLock(m_mutexTrapCfgAccess, INFINITE);
   for(i = 0; i < m_dwNumTraps; i++)
   {
      if (m_pTrapCfg[i].dwId == dwTrapId)
      {
         // Read trap configuration from event
         m_pTrapCfg[i].dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
         m_pTrapCfg[i].dwOidLen = pMsg->GetVariableLong(VID_TRAP_OID_LEN);
         m_pTrapCfg[i].pdwObjectId = (DWORD *)realloc(m_pTrapCfg[i].pdwObjectId, sizeof(DWORD) * m_pTrapCfg[i].dwOidLen);
         pMsg->GetVariableInt32Array(VID_TRAP_OID, m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId);
         pMsg->GetVariableStr(VID_DESCRIPTION, m_pTrapCfg[i].szDescription, MAX_DB_STRING);

         // Destroy current parameter mapping
         for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
            safe_free(m_pTrapCfg[i].pMaps[j].pdwObjectId);
         safe_free(m_pTrapCfg[i].pMaps);

         // Read new mappings from message
         m_pTrapCfg[i].dwNumMaps = pMsg->GetVariableLong(VID_TRAP_NUM_MAPS);
         m_pTrapCfg[i].pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * m_pTrapCfg[i].dwNumMaps);
         for(j = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE; 
             j < m_pTrapCfg[i].dwNumMaps; j++, dwId1++, dwId2++)
         {
            m_pTrapCfg[i].pMaps[j].dwOidLen = pMsg->GetVariableLong(dwId1);
            m_pTrapCfg[i].pMaps[j].pdwObjectId = 
               (DWORD *)malloc(sizeof(DWORD) * m_pTrapCfg[i].pMaps[j].dwOidLen);
            pMsg->GetVariableInt32Array(dwId2, m_pTrapCfg[i].pMaps[j].dwOidLen, 
                                        m_pTrapCfg[i].pMaps[j].pdwObjectId);
            pMsg->GetVariableStr(dwId3, m_pTrapCfg[i].pMaps[j].szDescription, MAX_DB_STRING);
         }

         // Update database
         pszEscDescr = EncodeSQLString(m_pTrapCfg[i].szDescription);
         SNMPConvertOIDToText(m_pTrapCfg[i].dwOidLen, m_pTrapCfg[i].pdwObjectId, szOID, 1024);
         _sntprintf(szQuery, 1024, _T("UPDATE snmp_trap_cfg SET snmp_oid='%s',event_code=%d,description='%s' WHERE trap_id=%d"),
                    szOID, m_pTrapCfg[i].dwEventCode, pszEscDescr, m_pTrapCfg[i].dwId);
         free(pszEscDescr);
         bSuccess = DBQuery(g_hCoreDB, szQuery);
         if (bSuccess)
         {
            _sntprintf(szQuery, 1024, _T("DELETE FROM snmp_trap_pmap WHERE trap_id=%d"), m_pTrapCfg[i].dwId);
            bSuccess = DBQuery(g_hCoreDB, szQuery);
            if (bSuccess)
            {
               for(j = 0; j < m_pTrapCfg[i].dwNumMaps; j++)
               {
                  SNMPConvertOIDToText(m_pTrapCfg[i].pMaps[j].dwOidLen,
                                       m_pTrapCfg[i].pMaps[j].pdwObjectId,
                                       szOID, 1024);
                  pszEscDescr = EncodeSQLString(m_pTrapCfg[i].pMaps[j].szDescription);
                  _sntprintf(szQuery, 1024, _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,")
                                            _T("snmp_oid,description) VALUES (%d,%d,'%s','%s')"),
                             m_pTrapCfg[i].dwId, j + 1, szOID, pszEscDescr);
                  free(pszEscDescr);
                  bSuccess = DBQuery(g_hCoreDB, szQuery);
                  if (!bSuccess)
                     break;
               }
            }
         }

         dwResult = bSuccess ? RCC_SUCCESS : RCC_DB_FAILURE;
         break;
      }
   }

   MutexUnlock(m_mutexTrapCfgAccess);
   return dwResult;
}
