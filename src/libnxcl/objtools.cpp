/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: objtools.cpp
**
**/

#include "libnxcl.h"


//
// Get list of object tools
//

DWORD LIBNXCL_EXPORTABLE NXCGetObjectTools(NXC_SESSION hSession, DWORD *pdwNumTools,
                                           NXC_OBJECT_TOOL **ppToolList)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwResult, dwRqId, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_OBJECT_TOOLS);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumTools = 0;
   *ppToolList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumTools = pResponse->GetVariableLong(VID_NUM_TOOLS);
         *ppToolList = (NXC_OBJECT_TOOL *)malloc(sizeof(NXC_OBJECT_TOOL) * (*pdwNumTools));
         memset(*ppToolList, 0, sizeof(NXC_OBJECT_TOOL) * (*pdwNumTools));
         for(i = 0, dwId = VID_OBJECT_TOOLS_BASE; i < *pdwNumTools; i++, dwId += 10)
         {
            (*ppToolList)[i].dwId = pResponse->GetVariableLong(dwId);
            pResponse->GetVariableStr(dwId + 1, (*ppToolList)[i].szName, MAX_DB_STRING);
            (*ppToolList)[i].wType = pResponse->GetVariableShort(dwId + 2);
            (*ppToolList)[i].pszData = pResponse->GetVariableStr(dwId + 3);
            (*ppToolList)[i].dwFlags = pResponse->GetVariableLong(dwId + 4);
            pResponse->GetVariableStr(dwId + 5, (*ppToolList)[i].szDescription, MAX_DB_STRING);
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Destroy list of object tools previously created by NXCGetObjectTools
//

void LIBNXCL_EXPORTABLE NXCDestroyObjectToolList(DWORD dwNumTools, NXC_OBJECT_TOOL *pList)
{
   DWORD i;

   if (pList != NULL)
   {
      for(i = 0; i < dwNumTools; i++)
         safe_free(pList[i].pszData);
      free(pList);
   }
}
