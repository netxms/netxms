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
// Lock/unlock trap configuration database
//

static DWORD LockTrapCfg(BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(bLock ? CMD_LOCK_TRAP_CFG : CMD_UNLOCK_TRAP_CFG);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}


//
// Client interface: lock action configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCLockTrapCfg(void)
{
   return LockTrapCfg(TRUE);
}


//
// Client interface: unlock action configuration database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockTrapCfg(void)
{
   return LockTrapCfg(FALSE);
}


//
// Fill trap configuration record from message
//

static void TrapCfgFromMsg(CSCPMessage *pMsg, NXC_TRAP_CFG_ENTRY *pTrap)
{
   DWORD i, dwId1, dwId2;

   pTrap->dwEventId = pMsg->GetVariableLong(VID_EVENT_ID);
   pTrap->dwOidLen = pMsg->GetVariableLong(VID_TRAP_OID_LEN);
   pTrap->pdwObjectId = (DWORD *)malloc(sizeof(DWORD) * pTrap->dwOidLen);
   pMsg->GetVariableInt32Array(VID_TRAP_OID, pTrap->dwOidLen, pTrap->pdwObjectId);
   pTrap->dwNumMaps = pMsg->GetVariableLong(VID_TRAP_NUM_MAPS);
   pTrap->pMaps = (NXC_OID *)malloc(sizeof(NXC_OID) * pTrap->dwNumMaps);
   for(i = 0, dwId1 = VID_TRAP_PLEN_BASE, dwId2 = VID_TRAP_PNAME_BASE; i < pTrap->dwNumMaps; i++, dwId1++, dwId2++)
   {
      pTrap->pMaps[i].dwOidLen = pMsg->GetVariableLong(dwId1);
      pTrap->pMaps[i].pdwObjectId = (DWORD *)malloc(sizeof(DWORD) * pTrap->pMaps[i].dwOidLen);
      pMsg->GetVariableInt32Array(dwId2, pTrap->pMaps[i].dwOidLen, pTrap->pMaps[i].pdwObjectId);
   }
}


//
// Load trap configuration from server
//

DWORD LIBNXCL_EXPORTABLE NXCLoadTrapCfg(DWORD *pdwNumTraps, NXC_TRAP_CFG_ENTRY **ppTrapList)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumTraps = 0, dwTrapId = 0;
   NXC_TRAP_CFG_ENTRY *pList = NULL;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_LOAD_TRAP_CFG);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      do
      {
         pResponce = WaitForMessage(CMD_TRAP_CFG_RECORD, dwRqId, g_dwCommandTimeout);
         if (pResponce != NULL)
         {
            dwTrapId = pResponce->GetVariableLong(VID_TRAP_ID);
            if (dwTrapId != 0)  // 0 is end of list indicator
            {
               pList = (NXC_TRAP_CFG_ENTRY *)realloc(pList, 
                           sizeof(NXC_TRAP_CFG_ENTRY) * (dwNumTraps + 1));
               pList[dwNumTraps].dwId = dwTrapId;
               TrapCfgFromMsg(pResponce, &pList[dwNumTraps]);
               dwNumTraps++;
            }
            delete pResponce;
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
      for(j = 0; i < pTrapList[i].dwNumMaps; j++)
         safe_free(pTrapList[i].pMaps[j].pdwObjectId);
      safe_free(pTrapList[i].pMaps);
      safe_free(pTrapList[i].pdwObjectId);
   }
   free(pTrapList);
}


//
// Delete trap configuration record by ID
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteTrap(DWORD dwTrapId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_DELETE_TRAP);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TRAP_ID, dwTrapId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}
