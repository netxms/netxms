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
#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
#endif


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
   TCHAR *pszToolData;
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

   _stprintf(szBuffer, _T("SELECT tool_type FROM object_tools WHERE tool_id=%d"), dwToolId);
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

   _stprintf(szBuffer, _T("SELECT tool_id FROM object_tools_acl WHERE tool_id=%d AND user_id=%d"),
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
   CSCPMessage msg;
   TCHAR *pszEnum, *pszRegEx, *pszLine, szBuffer[4096];
   AgentConnection *pConn;
   DWORD i, j, dwNumRows, dwNumCols, dwNumValidRows, dwResult, dwId, dwLen;
   int *pnSubstrPos, nPos;
   regex_t preg;
   regmatch_t *pMatchList;
   DB_RESULT hResult;

   // Prepare data message
   msg.SetCode(CMD_TABLE_DATA);
   msg.SetId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

   // Parse tool data. For agent table, it should have the following format:
   // table_title<separator>enum<separator>matching_regexp
   // where <separator> is a character with code 0x7F
   pszEnum = _tcschr(((TOOL_STARTUP_INFO *)pArg)->pszToolData, _T('\x7F'));
   if (pszEnum != NULL)
   {
      *pszEnum = 0;
      pszEnum++;
      pszRegEx = _tcschr(pszEnum, _T('\x7F'));
      if (pszRegEx != NULL)
      {
         *pszRegEx = 0;
         pszRegEx++;
      }
   }

   if ((pszEnum != NULL) && (pszRegEx != NULL))
   {
      // Load column information
      _stprintf(szBuffer, _T("SELECT col_name,col_format,col_substr FROM object_tools_table_columns WHERE tool_id=%d ORDER BY col_number"),
                ((TOOL_STARTUP_INFO *)pArg)->dwToolId);
      hResult = DBSelect(g_hCoreDB, szBuffer);
      if (hResult != NULL)
      {
         dwNumCols = DBGetNumRows(hResult);
         if (dwNumCols > 0)
         {
            pnSubstrPos = (int *)malloc(sizeof(int) * dwNumCols);
            for(i = 0; i < dwNumCols; i++)
            {
               nx_strncpy(szBuffer, DBGetField(hResult, i, 0), 256);
               DecodeSQLString(szBuffer);
               msg.SetVariable(VID_COLUMN_NAME_BASE + i, szBuffer);
               msg.SetVariable(VID_COLUMN_FMT_BASE + i, DBGetFieldULong(hResult, i, 1));
               pnSubstrPos[i] = DBGetFieldLong(hResult, i, 2);
            }
	         if (regcomp(&preg, pszRegEx, REG_EXTENDED | REG_ICASE) == 0)
	         {
               pConn = ((TOOL_STARTUP_INFO *)pArg)->pNode->CreateAgentConnection();
               if (pConn != NULL)
               {
                  dwResult = pConn->GetList(pszEnum);
                  if (dwResult == ERR_SUCCESS)
                  {
                     dwNumRows = pConn->GetNumDataLines();
                     pMatchList = (regmatch_t *)malloc(sizeof(regmatch_t) * (dwNumCols + 1));
                     for(i = 0, dwNumValidRows = 0, dwId = VID_ROW_DATA_BASE; i < dwNumRows; i++)
                     {
                        pszLine = (TCHAR *)pConn->GetDataLine(i);
                        if (regexec(&preg, pszLine, dwNumCols + 1, pMatchList, 0) == 0)
                        {
                           // Write data for current row into message
                           for(j = 0; j < dwNumCols; j++)
                           {
                              nPos = pnSubstrPos[j];
                              dwLen = pMatchList[nPos].rm_eo - pMatchList[nPos].rm_so;
                              memcpy(szBuffer, &pszLine[pMatchList[nPos].rm_so], dwLen * sizeof(TCHAR));
                              szBuffer[dwLen] = 0;
                              msg.SetVariable(dwId++, szBuffer);
                           }
                           dwNumValidRows++;
                        }
                     }
                     free(pMatchList);

                     // Set remaining variables
                     msg.SetVariable(VID_NUM_ROWS, dwNumValidRows);
                     msg.SetVariable(VID_TABLE_TITLE, ((TOOL_STARTUP_INFO *)pArg)->pszToolData);
                     msg.SetVariable(VID_NUM_COLUMNS, dwNumCols);
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                  }
                  else
                  {
                     msg.SetVariable(VID_RCC, (dwResult == ERR_UNKNOWN_PARAMETER) ? RCC_UNKNOWN_PARAMETER : RCC_COMM_FAILURE);
                  }
                  delete pConn;
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
               }
               regfree(&preg);
            }
            else     // Regexp compilation failed
            {
               msg.SetVariable(VID_RCC, RCC_BAD_REGEXP);
            }
            free(pnSubstrPos);
         }
         else  // No columns defined
         {
            msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
         }
      }
      else     // Cannot load column info from DB
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
   }

   // Send responce to client
   ((TOOL_STARTUP_INFO *)pArg)->pSession->SendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->DecRefCount();
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
   free(pArg);
   return THREAD_OK;
}


//
// Add SNMP variable value to results list
//

static void AddSNMPResult(NETXMS_VALUES_LIST *pValues, SNMP_Variable *pVar, LONG nFmt)
{
   TCHAR szBuffer[4096];

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

static DWORD TableHandler(DWORD dwVersion, DWORD dwAddr, WORD wPort,
                          const char *szCommunity, SNMP_Variable *pVar,
                          SNMP_Transport *pTransport, void *pArg)
{
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4];
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD i, dwResult, dwNameLen, pdwVarName[MAX_OID_LEN];

   // Create index (OID suffix) for columns
   if (((SNMP_ENUM_ARGS *)pArg)->dwFlags & TF_SNMP_INDEXED_BY_VALUE)
   {
      _stprintf(szSuffix, _T(".%u"), pVar->GetValueAsUInt());
   }
   else
   {
      SNMP_ObjectId *pOid;

      dwNameLen = SNMPParseOID(((SNMP_ENUM_ARGS *)pArg)->ppszOidList[0], pdwVarName, MAX_OID_LEN);
      pOid = pVar->GetName();
      SNMPConvertOIDToText(pOid->Length() - dwNameLen, 
         (DWORD *)&(pOid->GetValue())[dwNameLen], szSuffix, MAX_OID_LEN * 4);
   }

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
   return dwResult;
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

   _stprintf(szBuffer, _T("SELECT col_name,col_oid,col_format FROM object_tools_table_columns WHERE tool_id=%d ORDER BY col_number"),
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
            nx_strncpy(szBuffer, DBGetField(hResult, i, 0), 256);
            DecodeSQLString(szBuffer);
            msg.SetVariable(VID_COLUMN_NAME_BASE + i, szBuffer);
            args.ppszOidList[i] = _tcsdup(DBGetField(hResult, i, 1));
            args.pnFormatList[i] = DBGetFieldLong(hResult, i, 2);
            msg.SetVariable(VID_COLUMN_FMT_BASE + i, (DWORD)args.pnFormatList[i]);
         }

         // Enumerate
         if (((TOOL_STARTUP_INFO *)pArg)->pNode->CallSnmpEnumerate(args.ppszOidList[0], TableHandler, &args) == SNMP_ERR_SUCCESS)
         {
            // Fill in message with results
            msg.SetVariable(VID_TABLE_TITLE, ((TOOL_STARTUP_INFO *)pArg)->pszToolData);
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
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
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

   _stprintf(szBuffer, _T("SELECT tool_type,tool_data,flags FROM object_tools WHERE tool_id=%d"), dwToolId);
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
            pStartup->pszToolData = _tcsdup(DBGetField(hResult, 0, 1));
            DecodeSQLString(pStartup->pszToolData);
            pStartup->dwFlags = DBGetFieldULong(hResult, 0, 2);
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


//
// Delete object tool from database
//

DWORD DeleteObjectToolFromDB(DWORD dwToolId)
{
   TCHAR szQuery[256];

   _stprintf(szQuery, _T("DELETE FROM object_tools WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   _stprintf(szQuery, _T("DELETE FROM object_tools_acl WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   _stprintf(szQuery, _T("DELETE FROM object_tools_table_columns WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, 0);
   return RCC_SUCCESS;
}


//
// Update object tool from message
//

DWORD UpdateObjectToolFromMessage(CSCPMessage *pMsg)
{
   DB_RESULT hResult;
   BOOL bUpdate = FALSE;
   TCHAR *pszName, *pszData, *pszDescription, *pszOID, *pszTmp;
   TCHAR szBuffer[MAX_DB_STRING], szQuery[4096];
   DWORD i, dwToolId, dwAclSize, *pdwAcl;
   int nType;

   // Check if tool already exist
   dwToolId = pMsg->GetVariableLong(VID_TOOL_ID);
   _stprintf(szQuery, _T("SELECT tool_id FROM object_tools WHERE tool_id=%d"), dwToolId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bUpdate = TRUE;
      DBFreeResult(hResult);
   }

   // Insert or update common properties
   pMsg->GetVariableStr(VID_NAME, szBuffer, MAX_DB_STRING);
   pszName = EncodeSQLString(szBuffer);
   pMsg->GetVariableStr(VID_DESCRIPTION, szBuffer, MAX_DB_STRING);
   pszDescription = EncodeSQLString(szBuffer);
   pMsg->GetVariableStr(VID_TOOL_OID, szBuffer, MAX_DB_STRING);
   pszOID = EncodeSQLString(szBuffer);
   pszTmp = pMsg->GetVariableStr(VID_TOOL_DATA);
   pszData = EncodeSQLString(pszTmp);
   free(pszTmp);
   nType = pMsg->GetVariableShort(VID_TOOL_TYPE);
   if (bUpdate)
      _sntprintf(szQuery, 4096, _T("UPDATE object_tools SET tool_name='%s',tool_type=%d,"
                                   "tool_data='%s',description='%s',flags=%d,"
                                   "matching_oid='%s' WHERE tool_id=%d"),
                pszName, nType, pszData, pszDescription,
                pMsg->GetVariableLong(VID_FLAGS), pszOID, dwToolId);
   else
      _sntprintf(szQuery, 4096, _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,"
                                   "tool_data,description,flags,matching_oid) VALUES "
                                   "(%d,'%s',%d,'%s','%s',%d,'%s')"),
                dwToolId, pszName, nType, pszData,
                pszDescription, pMsg->GetVariableLong(VID_FLAGS), pszOID);
   free(pszName);
   free(pszDescription);
   free(pszData);
   free(pszOID);
   DBQuery(g_hCoreDB, szQuery);

   // Update ACL
   _stprintf(szQuery, _T("DELETE FROM object_tools_acl WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);
   dwAclSize = pMsg->GetVariableLong(VID_ACL_SIZE);
   if (dwAclSize > 0)
   {
      pdwAcl = (DWORD *)malloc(sizeof(DWORD) * dwAclSize);
      pMsg->GetVariableInt32Array(VID_ACL, dwAclSize, pdwAcl);
      for(i = 0; i < dwAclSize; i++)
      {
         _stprintf(szQuery, _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (%d,%d)"),
                   dwToolId, pdwAcl[i]);
         DBQuery(g_hCoreDB, szQuery);
      }
   }

   // Update columns configuration
   _stprintf(szQuery, _T("DELETE FROM object_tools_table_columns WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);
   if ((nType == TOOL_TYPE_TABLE_SNMP) ||
       (nType == TOOL_TYPE_TABLE_AGENT))
   {
      DWORD dwId, dwNumColumns;

      dwNumColumns = pMsg->GetVariableShort(VID_NUM_COLUMNS);
      for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < dwNumColumns; i++, dwId += 2)
      {
         pMsg->GetVariableStr(dwId++, szBuffer, MAX_DB_STRING);
         pszName = EncodeSQLString(szBuffer);
         pMsg->GetVariableStr(dwId++, szBuffer, MAX_DB_STRING);
         _sntprintf(szQuery, 4096, _T("INSERT INTO object_tools_table_columns (tool_id,"
                                      "col_number,col_name,col_oid,col_format,col_substr) "
                                      "VALUES (%d,%d,'%s','%s',%d,%d)"),
                    dwToolId, i, pszName, szBuffer, pMsg->GetVariableShort(dwId),
                    pMsg->GetVariableShort(dwId + 1));
         free(pszName);
         DBQuery(g_hCoreDB, szQuery);
      }
   }

   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, 0);
   return RCC_SUCCESS;
}
