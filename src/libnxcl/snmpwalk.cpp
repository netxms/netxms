/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: snmpwalk.cpp
**
**/

#include "libnxcl.h"


//
// Do SNMP walk
//

DWORD LIBNXCL_EXPORTABLE NXCSnmpWalk(NXC_SESSION hSession, DWORD dwNode,
                                     TCHAR *pszRootOID, void *pUserData,
                                     void (* pfCallback)(TCHAR *, DWORD, TCHAR *, void *))
{
   CSCPMessage msg, *pData;
   DWORD i, dwNumVars, dwRetCode, dwRqId, dwId, dwType;
   TCHAR szVarName[4096], szValue[4096];
   BOOL bStop = FALSE;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_START_SNMP_WALK);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SNMP_OID, pszRootOID);
   msg.SetVariable(VID_OBJECT_ID, dwNode);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      do
      {
         pData = ((NXCL_Session *)hSession)->WaitForMessage(CMD_SNMP_WALK_DATA, dwRqId);
         if (pData != NULL)
         {
            dwNumVars = pData->GetVariableLong(VID_NUM_VARIABLES);
            for(i = 0, dwId = VID_SNMP_WALKER_DATA_BASE; i < dwNumVars; i++)
            {
               pData->GetVariableStr(dwId++, szVarName, 4096);
               dwType = pData->GetVariableLong(dwId++);
               pData->GetVariableStr(dwId++, szValue, 4096);
               pfCallback(szVarName, dwType, szValue, pUserData);
            }
            bStop = pData->IsEndOfSequence();
            delete pData;
         }
         else
         {
            dwRetCode = RCC_TIMEOUT;
         }
      } while((dwRetCode == RCC_SUCCESS) && (!bStop));
   }

   return dwRetCode;
}
