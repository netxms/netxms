/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: objtools.cpp
**
**/

#include "libnxcl.h"


//
// Get list of object tools
//

UINT32 LIBNXCL_EXPORTABLE NXCGetObjectTools(NXC_SESSION hSession, UINT32 *pdwNumTools,
                                           NXC_OBJECT_TOOL **ppToolList)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwResult, dwRqId, dwId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_OBJECT_TOOLS);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumTools = 0;
   *ppToolList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumTools = pResponse->getFieldAsUInt32(VID_NUM_TOOLS);
         *ppToolList = (NXC_OBJECT_TOOL *)malloc(sizeof(NXC_OBJECT_TOOL) * (*pdwNumTools));
         memset(*ppToolList, 0, sizeof(NXC_OBJECT_TOOL) * (*pdwNumTools));
         for(i = 0, dwId = VID_OBJECT_TOOLS_BASE; i < *pdwNumTools; i++, dwId += 10)
         {
            (*ppToolList)[i].dwId = pResponse->getFieldAsUInt32(dwId);
            pResponse->getFieldAsString(dwId + 1, (*ppToolList)[i].szName, MAX_DB_STRING);
            (*ppToolList)[i].wType = pResponse->getFieldAsUInt16(dwId + 2);
            (*ppToolList)[i].pszData = pResponse->getFieldAsString(dwId + 3);
            (*ppToolList)[i].dwFlags = pResponse->getFieldAsUInt32(dwId + 4);
            pResponse->getFieldAsString(dwId + 5, (*ppToolList)[i].szDescription, MAX_DB_STRING);
            (*ppToolList)[i].objectFilter = pResponse->getFieldAsString(dwId + 6);
            (*ppToolList)[i].pszConfirmationText = pResponse->getFieldAsString(dwId + 7);
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

void LIBNXCL_EXPORTABLE NXCDestroyObjectToolList(UINT32 dwNumTools, NXC_OBJECT_TOOL *pList)
{
   UINT32 i;

   if (pList != NULL)
   {
      for(i = 0; i < dwNumTools; i++)
      {
         safe_free(pList[i].pszData);
         safe_free(pList[i].objectFilter);
         safe_free(pList[i].pszConfirmationText);
      }
      free(pList);
   }
}


//
// Execute table tool
//

UINT32 LIBNXCL_EXPORTABLE NXCExecuteTableTool(NXC_SESSION hSession, UINT32 dwNodeId,
                                             UINT32 dwToolId, Table **ppData)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwResult, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_EXEC_TABLE_TOOL);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *ppData = NULL;

   dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_TABLE_DATA, dwRqId, 180000);
      if (pResponse != NULL)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
         if (dwResult == RCC_SUCCESS)
         {
				*ppData = new Table(pResponse);
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
// Delete object tool
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteObjectTool(NXC_SESSION hSession, UINT32 dwToolId)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_OBJECT_TOOL);
   msg.setId(dwRqId);
   msg.setField(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get object tool details
//

UINT32 LIBNXCL_EXPORTABLE NXCGetObjectToolDetails(NXC_SESSION hSession, UINT32 dwToolId,
                                                 NXC_OBJECT_TOOL_DETAILS **ppData)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_OBJECT_TOOL_DETAILS);
   msg.setId(dwRqId);
   msg.setField(VID_TOOL_ID, dwToolId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *ppData = (NXC_OBJECT_TOOL_DETAILS *)malloc(sizeof(NXC_OBJECT_TOOL_DETAILS));
         memset(*ppData, 0, sizeof(NXC_OBJECT_TOOL_DETAILS));
         (*ppData)->dwId = dwToolId;
         (*ppData)->dwFlags = pResponse->getFieldAsUInt32(VID_FLAGS);
         (*ppData)->wType = pResponse->getFieldAsUInt16(VID_TOOL_TYPE);
         (*ppData)->pszData = pResponse->getFieldAsString(VID_TOOL_DATA);
         (*ppData)->pszConfirmationText = pResponse->getFieldAsString(VID_CONFIRMATION_TEXT);
         pResponse->getFieldAsString(VID_NAME, (*ppData)->szName, MAX_DB_STRING);
         pResponse->getFieldAsString(VID_DESCRIPTION, (*ppData)->szDescription, MAX_DB_STRING);
         (*ppData)->objectFilter = pResponse->getFieldAsString(VID_TOOL_FILTER);
         (*ppData)->dwACLSize = pResponse->getFieldAsUInt32(VID_ACL_SIZE);
         (*ppData)->pdwACL = (UINT32 *)malloc(sizeof(UINT32) * (*ppData)->dwACLSize);
         pResponse->getFieldAsInt32Array(VID_ACL, (*ppData)->dwACLSize, (*ppData)->pdwACL);
         if (((*ppData)->wType == TOOL_TYPE_TABLE_SNMP) ||
             ((*ppData)->wType == TOOL_TYPE_TABLE_AGENT))
         {
            UINT32 i, dwId;

            (*ppData)->wNumColumns = pResponse->getFieldAsUInt16(VID_NUM_COLUMNS);
            (*ppData)->pColList = (NXC_OBJECT_TOOL_COLUMN *)malloc(sizeof(NXC_OBJECT_TOOL_COLUMN) * (*ppData)->wNumColumns);
            for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < (UINT32)(*ppData)->wNumColumns; i++)
            {
               pResponse->getFieldAsString(dwId++, (*ppData)->pColList[i].szName, MAX_DB_STRING);
               pResponse->getFieldAsString(dwId++, (*ppData)->pColList[i].szOID, MAX_DB_STRING);
               (*ppData)->pColList[i].nFormat = (int)pResponse->getFieldAsUInt16(dwId++);
               (*ppData)->pColList[i].nSubstr = (int)pResponse->getFieldAsUInt16(dwId++);
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
      safe_free(pData->pszData);
      safe_free(pData->objectFilter);
      safe_free(pData->pColList);
      safe_free(pData->pdwACL);
      free(pData);
   }
}


//
// Generate ID for new object tool
//

UINT32 LIBNXCL_EXPORTABLE NXCGenerateObjectToolId(NXC_SESSION hSession, UINT32 *pdwToolId)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.setCode(CMD_GENERATE_OBJECT_TOOL_ID);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwToolId = pResponse->getFieldAsUInt32(VID_TOOL_ID);
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Update object tool configuration
//

UINT32 LIBNXCL_EXPORTABLE NXCUpdateObjectTool(NXC_SESSION hSession,
                                             NXC_OBJECT_TOOL_DETAILS *pData)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Prepare message
   msg.setCode(CMD_UPDATE_OBJECT_TOOL);
   msg.setId(dwRqId);
   msg.setField(VID_TOOL_ID, pData->dwId);
   msg.setField(VID_NAME, pData->szName);
   msg.setField(VID_TOOL_TYPE, pData->wType);
   msg.setField(VID_FLAGS, pData->dwFlags);
   msg.setField(VID_DESCRIPTION, pData->szDescription);
   msg.setField(VID_TOOL_DATA, pData->pszData);
   msg.setField(VID_CONFIRMATION_TEXT, CHECK_NULL_EX(pData->pszConfirmationText));
   msg.setField(VID_ACL_SIZE, pData->dwACLSize);
   msg.setField(VID_TOOL_FILTER, CHECK_NULL_EX(pData->objectFilter));
   msg.setFieldFromInt32Array(VID_ACL, pData->dwACLSize, pData->pdwACL);
   if ((pData->wType == TOOL_TYPE_TABLE_SNMP) ||
       (pData->wType == TOOL_TYPE_TABLE_AGENT))
   {
      int i;
      UINT32 dwId;

      msg.setField(VID_NUM_COLUMNS, pData->wNumColumns);
      for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < (int)pData->wNumColumns; i++)
      {
         msg.setField(dwId++, pData->pColList[i].szName);
         msg.setField(dwId++, pData->pColList[i].szOID);
         msg.setField(dwId++, (WORD)pData->pColList[i].nFormat);
         msg.setField(dwId++, (WORD)pData->pColList[i].nSubstr);
      }
   }

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Check if given object tool is appropriate for given node
//
/*
BOOL LIBNXCL_EXPORTABLE NXCIsAppropriateTool(NXC_OBJECT_TOOL *pTool, NXC_OBJECT *pObject)
{
   BOOL bResult;

   if (pObject == NULL)
      return TRUE;

   if (pObject->iClass == OBJECT_NODE)
   {
      bResult = TRUE;

      if ((pTool->dwFlags & TF_REQUIRES_SNMP) && (!(pObject->node.dwFlags & NF_IS_SNMP)))
         bResult = FALSE;
      if ((pTool->dwFlags & TF_REQUIRES_AGENT) &&
          (!(pObject->node.dwFlags & NF_IS_NATIVE_AGENT)))
         bResult = FALSE;
      if (pTool->dwFlags & TF_REQUIRES_OID_MATCH)
      {
         const TCHAR *pszPattern;

         pszPattern = CHECK_NULL_EX(pTool->pszMatchingOID);
         if (*pszPattern == 0)
            pszPattern = _T("*");
         if (!MatchString(pszPattern, CHECK_NULL_EX(pObject->node.pszSnmpObjectId), TRUE))
            bResult = FALSE;
      }
   }
   else
   {
      bResult = FALSE;
   }
   return bResult;
} */


//
// Run server script for object
//

UINT32 LIBNXCL_EXPORTABLE NXCExecuteServerCommand(NXC_SESSION hSession, UINT32 nodeId, const TCHAR *command)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
	msg.setCode(CMD_EXECUTE_SERVER_COMMAND);
   msg.setId(dwRqId);
	msg.setField(VID_OBJECT_ID, nodeId);
	msg.setField(VID_COMMAND, command);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
