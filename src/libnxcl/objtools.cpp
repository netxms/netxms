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


//
// Execute table tool
//

DWORD LIBNXCL_EXPORTABLE NXCExecuteTableTool(NXC_SESSION hSession, DWORD dwNodeId,
                                             DWORD dwToolId, NXC_TABLE_DATA **ppData)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwResult, dwRqId, dwId, dwNumRecords;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_EXEC_TABLE_TOOL);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *ppData = NULL;

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_TABLE_DATA, dwRqId, 180000);
      if (pResponse != NULL)
      {
         dwResult = pResponse->GetVariableLong(VID_RCC);
         if (dwResult == RCC_SUCCESS)
         {
            *ppData = (NXC_TABLE_DATA *)malloc(sizeof(NXC_TABLE_DATA));
            (*ppData)->pszTitle = pResponse->GetVariableStr(VID_TABLE_TITLE);
            (*ppData)->dwNumCols = pResponse->GetVariableLong(VID_NUM_COLUMNS);
            (*ppData)->dwNumRows = pResponse->GetVariableLong(VID_NUM_ROWS);
            (*ppData)->pnColFormat = (LONG *)malloc(sizeof(LONG) * (*ppData)->dwNumCols);
            (*ppData)->ppszColNames = (TCHAR **)malloc(sizeof(TCHAR *) * (*ppData)->dwNumCols);
            dwNumRecords = (*ppData)->dwNumCols * (*ppData)->dwNumRows;
            (*ppData)->ppszData = (TCHAR **)malloc(sizeof(TCHAR *) * dwNumRecords);

            // Column information
            for(i = 0; i < (*ppData)->dwNumCols; i++)
            {
               (*ppData)->ppszColNames[i] = pResponse->GetVariableStr(VID_COLUMN_NAME_BASE + i);
               (*ppData)->pnColFormat[i] = pResponse->GetVariableLong(VID_COLUMN_FMT_BASE + i);
            }

            // Column data
            for(i = 0, dwId = VID_ROW_DATA_BASE; i < dwNumRecords; i++, dwId++)
               (*ppData)->ppszData[i] = pResponse->GetVariableStr(dwId);
         }
         delete pResponse;
      }
      else
      {
         dwResult = RCC_TIMEOUT;
      }
   }

   return dwResult;
}


//
// Destroy received table data
//

void LIBNXCL_EXPORTABLE NXCDestroyTableData(NXC_TABLE_DATA *pData)
{
   DWORD i;

   if (pData == NULL)
      return;

   for(i = 0; i < pData->dwNumCols; i++)
      safe_free(pData->ppszColNames[i]);
   safe_free(pData->ppszColNames);
   safe_free(pData->pnColFormat);

   for(i = 0; i < pData->dwNumCols * pData->dwNumRows; i++)
      safe_free(pData->ppszData[i]);
   safe_free(pData->ppszData);
   safe_free(pData->pszTitle);
   free(pData);
}


//
// Lock object tools configuration
//

DWORD LIBNXCL_EXPORTABLE NXCLockObjectTools(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_LOCK_OBJECT_TOOLS);
}


//
// Unlock object tools configuration
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockObjectTools(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_UNLOCK_OBJECT_TOOLS);
}


//
// Delete object tool
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteObjectTool(NXC_SESSION hSession, DWORD dwToolId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_OBJECT_TOOL);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get object tool details
//

DWORD LIBNXCL_EXPORTABLE NXCGetObjectToolDetails(NXC_SESSION hSession, DWORD dwToolId,
                                                 NXC_OBJECT_TOOL_DETAILS **ppData)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_OBJECT_TOOL_DETAILS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *ppData = (NXC_OBJECT_TOOL_DETAILS *)malloc(sizeof(NXC_OBJECT_TOOL_DETAILS));
         memset(*ppData, 0, sizeof(NXC_OBJECT_TOOL_DETAILS));
         (*ppData)->dwId = dwToolId;
         (*ppData)->dwFlags = pResponse->GetVariableLong(VID_FLAGS);
         (*ppData)->wType = pResponse->GetVariableShort(VID_TOOL_TYPE);
         (*ppData)->pszData = pResponse->GetVariableStr(VID_TOOL_DATA);
         pResponse->GetVariableStr(VID_NAME, (*ppData)->szName, MAX_DB_STRING);
         pResponse->GetVariableStr(VID_DESCRIPTION, (*ppData)->szDescription, MAX_DB_STRING);
         (*ppData)->dwACLSize = pResponse->GetVariableLong(VID_ACL_SIZE);
         (*ppData)->pdwACL = (DWORD *)malloc(sizeof(DWORD) * (*ppData)->dwACLSize);
         pResponse->GetVariableInt32Array(VID_ACL, (*ppData)->dwACLSize, (*ppData)->pdwACL);
         if (((*ppData)->wType == TOOL_TYPE_TABLE_SNMP) ||
             ((*ppData)->wType == TOOL_TYPE_TABLE_AGENT))
         {
            DWORD i, dwId;

            (*ppData)->wNumColumns = pResponse->GetVariableShort(VID_NUM_COLUMNS);
            (*ppData)->pColList = (NXC_OBJECT_TOOL_COLUMN *)malloc(sizeof(NXC_OBJECT_TOOL_COLUMN) * (*ppData)->wNumColumns);
            for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < (DWORD)(*ppData)->wNumColumns; i++)
            {
               pResponse->GetVariableStr(dwId++, (*ppData)->pColList[i].szName, MAX_DB_STRING);
               pResponse->GetVariableStr(dwId++, (*ppData)->pColList[i].szOID, MAX_DB_STRING);
               (*ppData)->pColList[i].nFormat = (int)pResponse->GetVariableShort(dwId++);
               (*ppData)->pColList[i].nSubstr = (int)pResponse->GetVariableShort(dwId++);
            }
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
// Destroy object tool detailed info structure
//

void LIBNXCL_EXPORTABLE NXCDestroyObjectToolDetails(NXC_OBJECT_TOOL_DETAILS *pData)
{
   if (pData != NULL)
   {
      safe_free(pData->pColList);
      safe_free(pData->pdwACL);
      free(pData);
   }
}
