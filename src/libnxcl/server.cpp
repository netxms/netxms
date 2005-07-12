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
** $module: server.cpp
**
**/

#include "libnxcl.h"


//
// Get list of configuration variables
//

DWORD LIBNXCL_EXPORTABLE NXCGetServerVariables(NXC_SESSION hSession, 
                                               NXC_SERVER_VARIABLE **ppVarList, 
                                               DWORD *pdwNumVars)
{
   CSCPMessage msg, *pResponce;
   DWORD i, dwId, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *pdwNumVars = 0;
   *ppVarList = NULL;

   // Build request message
   msg.SetCode(CMD_GET_CONFIG_VARLIST);
   msg.SetId(dwRqId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for responce
   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 30000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumVars = pResponce->GetVariableLong(VID_NUM_VARIABLES);
         *ppVarList = (NXC_SERVER_VARIABLE *)malloc(sizeof(NXC_SERVER_VARIABLE) * (*pdwNumVars));

         for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++)
         {
            pResponce->GetVariableStr(dwId++, (*ppVarList)[i].szName, MAX_OBJECT_NAME);
            pResponce->GetVariableStr(dwId++, (*ppVarList)[i].szValue, MAX_DB_STRING);
            (*ppVarList)[i].bNeedRestart = pResponce->GetVariableShort(dwId++) ? TRUE : FALSE;
         }
      }
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Set value of server's variable
//

DWORD LIBNXCL_EXPORTABLE NXCSetServerVariable(NXC_SESSION hSession, TCHAR *pszVarName,
                                              TCHAR *pszValue)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_CONFIG_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   msg.SetVariable(VID_VALUE, pszValue);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete server's variable
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteServerVariable(NXC_SESSION hSession, TCHAR *pszVarName)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_CONFIG_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
