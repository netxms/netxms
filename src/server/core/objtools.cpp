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
** $module: objtools.cpp
**
**/

#include "nxcore.h"


//
// Tool startup info
//

struct TOOL_STARTUP_INFO
{
   DWORD dwToolId;
   DWORD dwRqId;
   DWORD dwFlags;
   Node *pNode;
   ClientSession *pSession;
};


//
// SNMP table enumerator arguments
//

struct SNMP_ENUM_ARGS
{
   DWORD dwNumCols;
   DWORD dwNumRows;
   TCHAR **ppszOidList;
   LONG *pnFormatList;
   DWORD dwFlags;
   NETXMS_VALUES_LIST values;
};


//
// Check if tool with given id exist and is a table tool
//

BOOL IsTableTool(DWORD dwToolId)
{
   DB_RESULT hResult;
   TCHAR szBuffer[256];
   LONG nType;
   BOOL bResult = FALSE;

   _stprintf(szBuffer, _T("SELECT tool_type FROM object_tools WHERE tool_id=%ld"), dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         nType = DBGetFieldLong(hResult, 0, 0);
         bResult = ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT));
      }
      DBFreeResult(hResult);
   }
   return bResult;
}


//
// Check if user has access to the tool
//

BOOL CheckObjectToolAccess(DWORD dwToolId, DWORD dwUserId)
{
   DB_RESULT hResult;
   TCHAR szBuffer[256];
   BOOL bResult = FALSE;

   if (dwUserId == 0)
      return TRUE;

   _stprintf(szBuffer, _T("SELECT tool_id FROM object_tools_acl WHERE tool_id=%ld AND user_id=%ld"),
             dwToolId, dwUserId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         bResult = TRUE;
      }
      DBFreeResult(hResult);
   }
   return bResult;
}


//
// Agent table tool execution thread
//

static THREAD_RESULT THREAD_CALL GetAgentTable(void *pArg)
{
   return THREAD_OK;
}


//
// Add SNMP variable value to results list
//

static void AddSNMPResult(NETXMS_VALUES_LIST *pValues, SNMP_Variable *pVar, LONG nFmt)
{
   TCHAR szBuffer[4096];

printf("ADD RESULT FROM %p\n",pVar);
   if (pVar != NULL)
   {
      switch(nFmt)
      {
         case CFMT_MAC_ADDR:
            pVar->GetValueAsMACAddr(szBuffer);
            break;
         default:
            pVar->GetValueAsString(szBuffer, 4096);
            break;
      }
printf("VALUE == \"%s\"\n", szBuffer);
   }
   else
   {
      szBuffer[0] = 0;
   }
   NxAddResultString(pValues, szBuffer);
}


//
// Handler for SNMP table enumeration
//

static void TableHandler(DWORD dwVersion, DWORD dwAddr, WORD wPort,
                         const char *szCommunity, SNMP_Variable *pVar,
                         SNMP_Transport *pTransport, void *pArg)
{
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4];
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD i, dwResult, dwNameLen, pdwVarName[MAX_OID_LEN];

   // Create index (OID suffix) for columns
   if (((SNMP_ENUM_ARGS *)pArg)->dwFlags & TF_SNMP_INDEXED_BY_VALUE)
   {
      _stprintf(szSuffix, _T(".%lu"), pVar->GetValueAsUInt());
   }
   else
   {
      SNMP_ObjectId *pOid;

      dwNameLen = SNMPParseOID(((SNMP_ENUM_ARGS *)pArg)->ppszOidList[0], pdwVarName, MAX_OID_LEN);
      pOid = pVar->GetName();
      SNMPConvertOIDToText(pOid->Length() - dwNameLen, 
         (DWORD *)&(pOid->GetValue())[dwNameLen], szSuffix, MAX_OID_LEN * 4);
   }
printf("SUFFIX: \"%s\"\n", szSuffix);

   // Get values for other columns
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, (char *)szCommunity, SnmpNewRequestId(), dwVersion);
   for(i = 1; i < ((SNMP_ENUM_ARGS *)pArg)->dwNumCols; i++)
   {
      _tcscpy(szOid, ((SNMP_ENUM_ARGS *)pArg)->ppszOidList[i]);
      _tcscat(szOid, szSuffix);
      dwNameLen = SNMPParseOID(szOid, pdwVarName, MAX_OID_LEN);
      if (dwNameLen != 0)
      {
         pRqPDU->BindVariable(new SNMP_Variable(pdwVarName, dwNameLen));
      }
   }

   dwResult = pTransport->DoRequest(pRqPDU, &pRespPDU, 1000, 3);
   delete pRqPDU;
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      if ((pRespPDU->GetNumVariables() > 0) &&
       (pRespPDU->GetErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         ((SNMP_ENUM_ARGS *)pArg)->dwNumRows++;

         // Add first column to results
         AddSNMPResult(&((SNMP_ENUM_ARGS *)pArg)->values, pVar, ((SNMP_ENUM_ARGS *)pArg)->pnFormatList[0]);

         for(i = 1; i < ((SNMP_ENUM_ARGS *)pArg)->dwNumCols; i++)
            AddSNMPResult(&((SNMP_ENUM_ARGS *)pArg)->values, 
                          pRespPDU->GetVariable(i - 1), 
                          ((SNMP_ENUM_ARGS *)pArg)->pnFormatList[i]);
      }
      delete pRespPDU;
   }
}


//
// SNMP table tool execution thread
//

static THREAD_RESULT THREAD_CALL GetSNMPTable(void *pArg)
{
   TCHAR szBuffer[256];
   DB_RESULT hResult;
   CSCPMessage msg;
   DWORD i, dwId, dwNumCols;
   SNMP_ENUM_ARGS args;

   // Prepare data message
   msg.SetCode(CMD_TABLE_DATA);
   msg.SetId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

   _stprintf(szBuffer, _T("SELECT col_name,col_oid,col_format FROM object_tools_snmp_tables WHERE tool_id=%ld ORDER BY col_number"),
             ((TOOL_STARTUP_INFO *)pArg)->dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      dwNumCols = DBGetNumRows(hResult);
      if (dwNumCols > 0)
      {
         args.dwNumRows = 0;
         args.dwNumCols = dwNumCols;
         args.ppszOidList = (TCHAR **)malloc(sizeof(TCHAR *) * dwNumCols);
         args.pnFormatList = (LONG *)malloc(sizeof(LONG) * dwNumCols);
         args.dwFlags = ((TOOL_STARTUP_INFO *)pArg)->dwFlags;
         args.values.dwNumStrings = 0;
         args.values.ppStringList = NULL;
         for(i = 0; i < dwNumCols; i++)
         {
            msg.SetVariable(VID_COLUMN_NAME_BASE + i, DBGetField(hResult, i, 0));
            args.ppszOidList[i] = _tcsdup(DBGetField(hResult, i, 1));
            args.pnFormatList[i] = DBGetFieldLong(hResult, i, 2);
            msg.SetVariable(VID_COLUMN_FMT_BASE + i, (DWORD)args.pnFormatList[i]);
         }

         // Enumerate
         if (((TOOL_STARTUP_INFO *)pArg)->pNode->CallSnmpEnumerate(args.ppszOidList[0], TableHandler, &args) == SNMP_ERR_SUCCESS)
         {
            // Fill in message with results
            msg.SetVariable(VID_NUM_COLUMNS, dwNumCols);
            msg.SetVariable(VID_NUM_ROWS, args.dwNumRows);
            for(i = 0, dwId = VID_ROW_DATA_BASE; i < args.values.dwNumStrings; i++)
               msg.SetVariable(dwId++, args.values.ppStringList[i]);
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_SNMP_ERROR);
         }

         // Cleanup
         for(i = 0; i < args.values.dwNumStrings; i++)
            safe_free(args.values.ppStringList[i]);
         safe_free(args.values.ppStringList);
         for(i = 0; i < dwNumCols; i++)
            safe_free(args.ppszOidList[i]);
         free(args.ppszOidList);
         free(args.pnFormatList);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
      }
      DBFreeResult(hResult);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send responce to client
   ((TOOL_STARTUP_INFO *)pArg)->pSession->SendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->DecRefCount();
   free(pArg);
   return THREAD_OK;
}


//
// Execute table tool
//

DWORD ExecuteTableTool(DWORD dwToolId, Node *pNode, DWORD dwRqId, ClientSession *pSession)
{
   LONG nType;
   DWORD dwRet = SYSINFO_RC_SUCCESS;
   TOOL_STARTUP_INFO *pStartup;
   TCHAR szBuffer[256];
   DB_RESULT hResult;

   _stprintf(szBuffer, _T("SELECT tool_type,flags FROM object_tools WHERE tool_id=%ld"), dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         nType = DBGetFieldLong(hResult, 0, 0);
         if ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT))
         {
            pSession->IncRefCount();
            pStartup = (TOOL_STARTUP_INFO *)malloc(sizeof(TOOL_STARTUP_INFO));
            pStartup->dwToolId = dwToolId;
            pStartup->dwRqId = dwRqId;
            pStartup->dwFlags = DBGetFieldULong(hResult, 0, 1);
            pStartup->pNode = pNode;
            pStartup->pSession = pSession;
            ThreadCreate((nType == TOOL_TYPE_TABLE_SNMP) ? GetSNMPTable : GetAgentTable,
                         0, pStartup);
         }
         else
         {
            dwRet = RCC_INCOMPATIBLE_OPERATION;
         }
      }
      else
      {
         dwRet = RCC_INVALID_TOOL_ID;
      }
      DBFreeResult(hResult);
   }
   else
   {
      dwRet = RCC_DB_FAILURE;
   }
   return dwRet;
}
