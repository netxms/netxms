/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: objtools.cpp
**
**/

#include "nxcore.h"
#include <netxms-regex.h>


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
   TCHAR **ppszOidList;
   LONG *pnFormatList;
   DWORD dwFlags;
   Node *pNode;
	Table *table;
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

   _sntprintf(szBuffer, 256, _T("SELECT tool_type FROM object_tools WHERE tool_id=%d"), dwToolId);
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
	int i, nRows;
	DWORD dwId;
   BOOL bResult = FALSE;

   if (dwUserId == 0)
      return TRUE;

   _sntprintf(szBuffer, 256, _T("SELECT user_id FROM object_tools_acl WHERE tool_id=%d"), dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
		nRows = DBGetNumRows(hResult);
      for(i = 0; i < nRows; i++)
      {
			dwId = DBGetFieldULong(hResult, i, 0);
			if ((dwId == dwUserId) || (dwId == GROUP_EVERYONE))
			{
				bResult = TRUE;
				break;
			}
			if (dwId & GROUP_FLAG)
			{
				if (CheckUserMembership(dwUserId, dwId))
				{
					bResult = TRUE;
					break;
				}
			}
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
   DWORD i, j, dwNumRows, dwNumCols, dwResult, dwLen;
   int *pnSubstrPos, nPos;
   regex_t preg;
   regmatch_t *pMatchList;
   DB_RESULT hResult;
	Table table;

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
	table.setTitle(((TOOL_STARTUP_INFO *)pArg)->pszToolData);

   if ((pszEnum != NULL) && (pszRegEx != NULL))
   {
      // Load column information
      _sntprintf(szBuffer, 4096, _T("SELECT col_name,col_format,col_substr FROM object_tools_table_columns WHERE tool_id=%d ORDER BY col_number"),
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
               DBGetField(hResult, i, 0, szBuffer, 256);
               DecodeSQLString(szBuffer);
					table.addColumn(szBuffer, DBGetFieldULong(hResult, i, 1));
               pnSubstrPos[i] = DBGetFieldLong(hResult, i, 2);
            }
	         if (_tregcomp(&preg, pszRegEx, REG_EXTENDED | REG_ICASE) == 0)
	         {
               pConn = ((TOOL_STARTUP_INFO *)pArg)->pNode->createAgentConnection();
               if (pConn != NULL)
               {
                  dwResult = pConn->getList(pszEnum);
                  if (dwResult == ERR_SUCCESS)
                  {
                     dwNumRows = pConn->getNumDataLines();
                     pMatchList = (regmatch_t *)malloc(sizeof(regmatch_t) * (dwNumCols + 1));
                     for(i = 0; i < dwNumRows; i++)
                     {
                        pszLine = (TCHAR *)pConn->getDataLine(i);
                        if (_tregexec(&preg, pszLine, dwNumCols + 1, pMatchList, 0) == 0)
                        {
									table.addRow();

                           // Write data for current row into message
                           for(j = 0; j < dwNumCols; j++)
                           {
                              nPos = pnSubstrPos[j];
                              dwLen = pMatchList[nPos].rm_eo - pMatchList[nPos].rm_so;
                              memcpy(szBuffer, &pszLine[pMatchList[nPos].rm_so], dwLen * sizeof(TCHAR));
                              szBuffer[dwLen] = 0;
										table.set(j, szBuffer);
                           }
                        }
                     }
                     free(pMatchList);

                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
							table.fillMessage(msg, 0, -1);
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
         DBFreeResult(hResult);
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
   ((TOOL_STARTUP_INFO *)pArg)->pSession->sendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->decRefCount();
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
   free(pArg);
   return THREAD_OK;
}


//
// Add SNMP variable value to results list
//

static void AddSNMPResult(Table *table, int column, SNMP_Variable *pVar,
                          LONG nFmt, Node *pNode)
{
   TCHAR szBuffer[4096];
   Interface *pInterface;
   DWORD dwIndex;
	bool convert;

   if (pVar != NULL)
   {
      switch(nFmt)
      {
         case CFMT_MAC_ADDR:
            pVar->GetValueAsMACAddr(szBuffer);
            break;
         case CFMT_IP_ADDR:
            pVar->GetValueAsIPAddr(szBuffer);
            break;
         case CFMT_IFINDEX:   // Column is an interface index, convert to interface name
            dwIndex = pVar->GetValueAsUInt();
            pInterface = pNode->findInterface(dwIndex, INADDR_ANY);
            if (pInterface != NULL)
            {
               nx_strncpy(szBuffer, pInterface->Name(), 4096);
            }
            else
            {
               if (dwIndex == 0)    // Many devices uses index 0 for internal MAC, etc.
                  _tcscpy(szBuffer, _T("INTERNAL"));
               else
                  _sntprintf(szBuffer, 64, _T("%d"), dwIndex);
            }
            break;
         default:
				convert = true;
            pVar->getValueAsPrintableString(szBuffer, 4096, &convert);
            break;
      }
   }
   else
   {
      szBuffer[0] = 0;
   }
	table->set(column, szBuffer);
}


//
// Handler for SNMP table enumeration
//

static DWORD TableHandler(DWORD dwVersion, SNMP_Variable *pVar,
                          SNMP_Transport *pTransport, void *pArg)
{
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4];
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD i, dwResult, dwNameLen, pdwVarName[MAX_OID_LEN];

   // Create index (OID suffix) for columns
   if (((SNMP_ENUM_ARGS *)pArg)->dwFlags & TF_SNMP_INDEXED_BY_VALUE)
   {
      _sntprintf(szSuffix, MAX_OID_LEN * 4, _T(".%u"), pVar->GetValueAsUInt());
   }
   else
   {
      SNMP_ObjectId *pOid;

      dwNameLen = SNMPParseOID(((SNMP_ENUM_ARGS *)pArg)->ppszOidList[0], pdwVarName, MAX_OID_LEN);
      pOid = pVar->GetName();
      SNMPConvertOIDToText(pOid->getLength() - dwNameLen, 
         (DWORD *)&(pOid->getValue())[dwNameLen], szSuffix, MAX_OID_LEN * 4);
   }

   // Get values for other columns
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);
   for(i = 1; i < ((SNMP_ENUM_ARGS *)pArg)->dwNumCols; i++)
   {
      _tcscpy(szOid, ((SNMP_ENUM_ARGS *)pArg)->ppszOidList[i]);
      _tcscat(szOid, szSuffix);
      dwNameLen = SNMPParseOID(szOid, pdwVarName, MAX_OID_LEN);
      if (dwNameLen != 0)
      {
         pRqPDU->bindVariable(new SNMP_Variable(pdwVarName, dwNameLen));
      }
   }

   dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);
   delete pRqPDU;
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      if ((pRespPDU->getNumVariables() > 0) &&
          (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
			((SNMP_ENUM_ARGS *)pArg)->table->addRow();

         // Add first column to results
         AddSNMPResult(((SNMP_ENUM_ARGS *)pArg)->table, 0, pVar,
                       ((SNMP_ENUM_ARGS *)pArg)->pnFormatList[0],
                       ((SNMP_ENUM_ARGS *)pArg)->pNode);

         for(i = 1; i < ((SNMP_ENUM_ARGS *)pArg)->dwNumCols; i++)
            AddSNMPResult(((SNMP_ENUM_ARGS *)pArg)->table, i,
                          pRespPDU->getVariable(i - 1), 
                          ((SNMP_ENUM_ARGS *)pArg)->pnFormatList[i],
                          ((SNMP_ENUM_ARGS *)pArg)->pNode);
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
   DWORD i, dwNumCols;
   SNMP_ENUM_ARGS args;
	Table table;

   // Prepare data message
   msg.SetCode(CMD_TABLE_DATA);
   msg.SetId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

   _sntprintf(szBuffer, 256, _T("SELECT col_name,col_oid,col_format FROM object_tools_table_columns WHERE tool_id=%d ORDER BY col_number"),
              ((TOOL_STARTUP_INFO *)pArg)->dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      dwNumCols = DBGetNumRows(hResult);
      if (dwNumCols > 0)
      {
         args.dwNumCols = dwNumCols;
         args.ppszOidList = (TCHAR **)malloc(sizeof(TCHAR *) * dwNumCols);
         args.pnFormatList = (LONG *)malloc(sizeof(LONG) * dwNumCols);
         args.dwFlags = ((TOOL_STARTUP_INFO *)pArg)->dwFlags;
         args.pNode = ((TOOL_STARTUP_INFO *)pArg)->pNode;
			args.table = &table;
         for(i = 0; i < dwNumCols; i++)
         {
            DBGetField(hResult, i, 0, szBuffer, 256);
            DecodeSQLString(szBuffer);
            args.ppszOidList[i] = DBGetField(hResult, i, 1, NULL, 0);
            args.pnFormatList[i] = DBGetFieldLong(hResult, i, 2);
				table.addColumn(szBuffer, args.pnFormatList[i]);
         }

         // Enumerate
         if (((TOOL_STARTUP_INFO *)pArg)->pNode->callSnmpEnumerate(args.ppszOidList[0], TableHandler, &args) == SNMP_ERR_SUCCESS)
         {
            // Fill in message with results
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
				table.setTitle(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
				table.fillMessage(msg, 0, -1);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_SNMP_ERROR);
         }

         // Cleanup
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
   ((TOOL_STARTUP_INFO *)pArg)->pSession->sendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->decRefCount();
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
   DWORD dwRet = RCC_SUCCESS;
   TOOL_STARTUP_INFO *pStartup;
   TCHAR szBuffer[256];
   DB_RESULT hResult;

   _sntprintf(szBuffer, 256, _T("SELECT tool_type,tool_data,flags FROM object_tools WHERE tool_id=%d"), dwToolId);
   hResult = DBSelect(g_hCoreDB, szBuffer);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         nType = DBGetFieldLong(hResult, 0, 0);
         if ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT))
         {
            pSession->incRefCount();
            pStartup = (TOOL_STARTUP_INFO *)malloc(sizeof(TOOL_STARTUP_INFO));
            pStartup->dwToolId = dwToolId;
            pStartup->dwRqId = dwRqId;
            pStartup->pszToolData = DBGetField(hResult, 0, 1, NULL, 0);
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

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM object_tools WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM object_tools_acl WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM object_tools_table_columns WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);

   NotifyClientSessions(NX_NOTIFY_OBJTOOL_DELETED, dwToolId);
   return RCC_SUCCESS;
}


//
// Update object tool from message
//

DWORD UpdateObjectToolFromMessage(CSCPMessage *pMsg)
{
   DB_RESULT hResult;
   BOOL bUpdate = FALSE;
   TCHAR *pszName, *pszData, *pszDescription, *pszOID, *pszTmp, *pszConfirm;
   TCHAR szBuffer[MAX_DB_STRING], szQuery[4096];
   DWORD i, dwToolId, dwAclSize, *pdwAcl;
   int nType;

   // Check if tool already exist
   dwToolId = pMsg->GetVariableLong(VID_TOOL_ID);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT tool_id FROM object_tools WHERE tool_id=%d"), dwToolId);
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
   pMsg->GetVariableStr(VID_CONFIRMATION_TEXT, szBuffer, MAX_DB_STRING);
   pszConfirm = EncodeSQLString(szBuffer);
   pszTmp = pMsg->GetVariableStr(VID_TOOL_DATA);
   pszData = EncodeSQLString(pszTmp);
   free(pszTmp);
   nType = pMsg->GetVariableShort(VID_TOOL_TYPE);
   if (bUpdate)
      _sntprintf(szQuery, 4096, _T("UPDATE object_tools SET tool_name='%s',tool_type=%d,")
                                _T("tool_data='%s',description='%s',flags=%d,")
                                _T("matching_oid='%s',confirmation_text='%s' ")
                                _T("WHERE tool_id=%d"),
                pszName, nType, pszData, pszDescription,
                pMsg->GetVariableLong(VID_FLAGS), pszOID, pszConfirm, dwToolId);
   else
      _sntprintf(szQuery, 4096, _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,")
                                _T("tool_data,description,flags,matching_oid,")
                                _T("confirmation_text) VALUES ")
                                _T("(%d,'%s',%d,'%s','%s',%d,'%s','%s')"),
                dwToolId, pszName, nType, pszData,
                pszDescription, pMsg->GetVariableLong(VID_FLAGS),
                pszOID, pszConfirm);
   free(pszName);
   free(pszDescription);
   free(pszData);
   free(pszOID);
   free(pszConfirm);
   DBQuery(g_hCoreDB, szQuery);

   // Update ACL
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM object_tools_acl WHERE tool_id=%d"), dwToolId);
   DBQuery(g_hCoreDB, szQuery);
   dwAclSize = pMsg->GetVariableLong(VID_ACL_SIZE);
   if (dwAclSize > 0)
   {
      pdwAcl = (DWORD *)malloc(sizeof(DWORD) * dwAclSize);
      pMsg->GetVariableInt32Array(VID_ACL, dwAclSize, pdwAcl);
      for(i = 0; i < dwAclSize; i++)
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (%d,%d)"),
                   dwToolId, pdwAcl[i]);
         DBQuery(g_hCoreDB, szQuery);
      }
   }

   // Update columns configuration
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM object_tools_table_columns WHERE tool_id=%d"), dwToolId);
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
         _sntprintf(szQuery, 4096, _T("INSERT INTO object_tools_table_columns (tool_id,")
                                   _T("col_number,col_name,col_oid,col_format,col_substr) ")
                                   _T("VALUES (%d,%d,'%s','%s',%d,%d)"),
                    dwToolId, i, pszName, szBuffer, pMsg->GetVariableShort(dwId),
                    pMsg->GetVariableShort(dwId + 1));
         free(pszName);
         DBQuery(g_hCoreDB, szQuery);
      }
   }

   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, dwToolId);
   return RCC_SUCCESS;
}
