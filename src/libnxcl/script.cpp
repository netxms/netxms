/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: script.cpp
**
**/

#include "libnxcl.h"


//
// Get list of scripts in the library
//

UINT32 LIBNXCL_EXPORTABLE NXCGetScriptList(NXC_SESSION hSession, UINT32 *pdwNumScripts,
                                          NXC_SCRIPT_INFO **ppList)
{
   CSCPMessage msg, *pResponse;
   UINT32 i, dwId, dwResult, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_SCRIPT_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumScripts = 0;
   *ppList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumScripts = pResponse->GetVariableLong(VID_NUM_SCRIPTS);
         *ppList = (NXC_SCRIPT_INFO *)malloc(sizeof(NXC_SCRIPT_INFO) * (*pdwNumScripts));
         for(i = 0, dwId = VID_SCRIPT_LIST_BASE; i < *pdwNumScripts; i++)
         {
            (*ppList)[i].dwId = pResponse->GetVariableLong(dwId++);
            pResponse->GetVariableStr(dwId++, (*ppList)[i].szName, MAX_DB_STRING);
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
// Get script
//

UINT32 LIBNXCL_EXPORTABLE NXCGetScript(NXC_SESSION hSession, UINT32 dwId, TCHAR **ppszCode)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwResult, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_SCRIPT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SCRIPT_ID, dwId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *ppszCode = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *ppszCode = pResponse->GetVariableStr(VID_SCRIPT_CODE);
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
// Update script
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateScript(NXC_SESSION hSession, UINT32 *pdwId,
                                         TCHAR *pszName, TCHAR *pszCode)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPDATE_SCRIPT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SCRIPT_ID, *pdwId);
   msg.SetVariable(VID_NAME, pszName);
   msg.SetVariable(VID_SCRIPT_CODE, pszCode);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         *pdwId = pResponse->GetVariableLong(VID_SCRIPT_ID);
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Rename script
//

UINT32 LIBNXCL_EXPORTABLE NXCRenameScript(NXC_SESSION hSession, UINT32 dwId,
                                         TCHAR *pszName)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_RENAME_SCRIPT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SCRIPT_ID, dwId);
   msg.SetVariable(VID_NAME, pszName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete script
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteScript(NXC_SESSION hSession, UINT32 dwId)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_SCRIPT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SCRIPT_ID, dwId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
