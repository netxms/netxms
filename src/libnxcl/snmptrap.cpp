/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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

#include "libnxcl.h"


//
// Client interface: lock trap configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCLockTrapCfg(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_LOCK_TRAP_CFG);
}


//
// Client interface: unlock trap configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockTrapCfg(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_UNLOCK_TRAP_CFG);
}


//
// Fill trap configuration record from message
//

static void TrapCfgFromMsg(CSCPMessage *pMsg, NXC_TRAP_CFG_ENTRY *pTrap)
{
   DWORD i, dwId1, dwId2, dwId3;

   pTrap->dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
   pMsg->GetVariableStr(VID_DESCRIPTION, pTrap->szDescription, MAX_DB_STRING);
   pTrap->dwOidLen = pMsg->GetVariableLong(VID_TRAP_OID_LEN);
   pTrap->pdwObjectId = (DWORD *)malloc(sizeof(DWORD) * pTrap->dwOidLen);
   pMsg->GetVariableInt32Array(VID_TRAP_OID, pTrap->dwOidLen, pTrap->pdwObjectId);
   pTrap->dwNumMaps = pMsg->GetVariableLong(VID_TRAP_NUM_MAPS);
   pTrap->pMaps = (NXC_OID_MAP *)malloc(sizeof(NXC_OID_MAP) * pTrap->dwNumMaps);
   for(i = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE;
       i < pTrap->dwNumMaps; i++, dwId1++, dwId2++)
   {
      pTrap->pMaps[i].dwOidLen = pMsg->GetVariableLong(dwId1);
      pTrap->pMaps[i].pdwObjectId = (DWORD *)malloc(sizeof(DWORD) * pTrap->pMaps[i].dwOidLen);
      pMsg->GetVariableInt32Array(dwId2, pTrap->pMaps[i].dwOidLen, pTrap->pMaps[i].pdwObjectId);
      pMsg->GetVariableStr(dwId3, pTrap->pMaps[i].szDescription, MAX_DB_STRING);
   }
}


//
// Load trap configuration from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadTrapCfg(NXC_SESSION hSession, DWORD *pdwNumTraps, NXC_TRAP_CFG_ENTRY **ppTrapList)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumTraps = 0, dwTrapId = 0;
   NXC_TRAP_CFG_ENTRY *pList = NULL;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_LOAD_TRAP_CFG);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      do
      {
         pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_TRAP_CFG_RECORD, dwRqId);
         if (pResponse != NULL)
         {
            dwTrapId = pResponse->GetVariableLong(VID_TRAP_ID);
            if (dwTrapId != 0)  // 0 is end of list indicator
            {
               pList = (NXC_TRAP_CFG_ENTRY *)realloc(pList, 
                           sizeof(NXC_TRAP_CFG_ENTRY) * (dwNumTraps + 1));
               pList[dwNumTraps].dwId = dwTrapId;
               TrapCfgFromMsg(pResponse, &pList[dwNumTraps]);
               dwNumTraps++;
            }
            delete pResponse;
         }
         else
         {
            dwRetCode = RCC_TIMEOUT;
            dwTrapId = 0;
         }
      }
      while(dwTrapId != 0);
   }

   // Destroy results on failure or save on success
   if (dwRetCode == RCC_SUCCESS)
   {
      *ppTrapList = pList;
      *pdwNumTraps = dwNumTraps;
   }
   else
   {
      safe_free(pList);
      *ppTrapList = NULL;
      *pdwNumTraps = 0;
   }

   return dwRetCode;
}


//
// Destroy list of traps
//

void LIBNXCL_EXPORTABLE NXCDestroyTrapList(DWORD dwNumTraps, NXC_TRAP_CFG_ENTRY *pTrapList)
{
   DWORD i, j;

   if (pTrapList == NULL)
      return;

   for(i = 0; i < dwNumTraps; i++)
   {
      for(j = 0; j < pTrapList[i].dwNumMaps; j++)
         safe_free(pTrapList[i].pMaps[j].pdwObjectId);
      safe_free(pTrapList[i].pMaps);
      safe_free(pTrapList[i].pdwObjectId);
   }
   free(pTrapList);
}


//
// Delete trap configuration record by ID
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteTrap(NXC_SESSION hSession, DWORD dwTrapId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_TRAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TRAP_ID, dwTrapId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Create new trap configuration record
//

DWORD LIBNXCL_EXPORTABLE NXCCreateTrap(NXC_SESSION hSession, DWORD *pdwTrapId)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CREATE_TRAP);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         *pdwTrapId = pResponse->GetVariableLong(VID_TRAP_ID);
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   return dwResult;
}


//
// Update trap configuration record
//

DWORD LIBNXCL_EXPORTABLE NXCModifyTrap(NXC_SESSION hSession, NXC_TRAP_CFG_ENTRY *pTrap)
{
   CSCPMessage msg;
   DWORD i, dwRqId, dwId1, dwId2, dwId3;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_MODIFY_TRAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TRAP_ID, pTrap->dwId);
   msg.SetVariable(VID_TRAP_OID_LEN, pTrap->dwOidLen); 
   msg.SetVariableToInt32Array(VID_TRAP_OID, pTrap->dwOidLen, pTrap->pdwObjectId);
   msg.SetVariable(VID_EVENT_CODE, pTrap->dwEventCode);
   msg.SetVariable(VID_DESCRIPTION, pTrap->szDescription);
   msg.SetVariable(VID_TRAP_NUM_MAPS, pTrap->dwNumMaps);
   for(i = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE, dwId3 = VID_TRAP_PDESCR_BASE; 
       i < pTrap->dwNumMaps; i++, dwId1++, dwId2++)
   {
      msg.SetVariable(dwId1, pTrap->pMaps[i].dwOidLen);
      msg.SetVariableToInt32Array(dwId2, pTrap->pMaps[i].dwOidLen, pTrap->pMaps[i].pdwObjectId);
      msg.SetVariable(dwId3, pTrap->pMaps[i].szDescription);
   }
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Process SNMP trap log records coming from server
//

void ProcessTrapLogRecords(NXCL_Session *pSession, CSCPMessage *pMsg)
{
   DWORD i, dwNumRecords, dwId;
   NXC_SNMP_TRAP_LOG_RECORD rec;
   int nOrder;

   dwNumRecords = pMsg->GetVariableLong(VID_NUM_RECORDS);
   nOrder = (int)pMsg->GetVariableShort(VID_RECORDS_ORDER);
   DebugPrintf(_T("ProcessTrapLogRecords(): %d records in message, in %s order"),
               dwNumRecords, (nOrder == RECORD_ORDER_NORMAL) ? _T("normal") : _T("reversed"));
   for(i = 0, dwId = VID_TRAP_LOG_MSG_BASE; i < dwNumRecords; i++)
   {
      rec.qwId = pMsg->GetVariableInt64(dwId++);
      rec.dwTimeStamp = pMsg->GetVariableLong(dwId++);
      rec.dwIpAddr = pMsg->GetVariableLong(dwId++);
      rec.dwObjectId = pMsg->GetVariableLong(dwId++);
      pMsg->GetVariableStr(dwId++, rec.szTrapOID, MAX_DB_STRING);
      rec.pszTrapVarbinds = pMsg->GetVariableStr(dwId++);

      // Call client's callback to handle new record
      pSession->CallEventHandler(NXC_EVENT_NEW_SNMP_TRAP, nOrder, &rec);
      free(rec.pszTrapVarbinds);
   }

   // Notify requestor thread if all messages was received
   if (pMsg->IsEndOfSequence())
      pSession->CompleteSync(SYNC_TRAP_LOG, RCC_SUCCESS);
}


//
// Synchronize trap log
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCSyncSNMPTrapLog(NXC_SESSION hSession, DWORD dwMaxRecords)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
   ((NXCL_Session *)hSession)->PrepareForSync(SYNC_TRAP_LOG);

   msg.SetCode(CMD_GET_TRAP_LOG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_MAX_RECORDS, dwMaxRecords);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = ((NXCL_Session *)hSession)->WaitForSync(SYNC_TRAP_LOG, INFINITE);
   else
      ((NXCL_Session *)hSession)->UnlockSyncOp(SYNC_TRAP_LOG);

   return dwRetCode;
}
