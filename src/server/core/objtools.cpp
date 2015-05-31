/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Raden Solutions
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
#include <nxtools.h>

/**
 * Tool startup info
 */
struct TOOL_STARTUP_INFO
{
   UINT32 dwToolId;
   UINT32 dwRqId;
   UINT32 dwFlags;
   Node *pNode;
   ClientSession *pSession;
   TCHAR *pszToolData;
};

/**
 * SNMP table enumerator arguments
 */
struct SNMP_ENUM_ARGS
{
   UINT32 dwNumCols;
   TCHAR **ppszOidList;
   LONG *pnFormatList;
   UINT32 dwFlags;
   Node *pNode;
	Table *table;
};

/**
 * Rollback all querys, release BD connection, free prepared statement if not NULL and return RCC_DB_FAILURE code
 */
static UINT32 ReturnDBFailure(DB_HANDLE hdb, DB_STATEMENT hStmt)
{
   DBRollback(hdb);
   if (hStmt != NULL)
      DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return RCC_DB_FAILURE;
}

/**
 * Check if tool with given id exist and is a table tool
 */
BOOL IsTableTool(UINT32 dwToolId)
{
   DB_RESULT hResult;
   LONG nType;
   BOOL bResult = FALSE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT tool_type FROM object_tools WHERE tool_id=?"));
   if (statment == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);

   hResult = DBSelectPrepared(statment);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         nType = DBGetFieldLong(hResult, 0, 0);
         bResult = ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT));
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   DBFreeStatement(statment);
   return bResult;
}


/**
 * Check if user has access to the tool
 */

BOOL CheckObjectToolAccess(UINT32 dwToolId, UINT32 dwUserId)
{
   DB_RESULT hResult;
	int i, nRows;
	UINT32 dwId;
   BOOL bResult = FALSE;

   if (dwUserId == 0)
      return TRUE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT user_id FROM object_tools_acl WHERE tool_id=?"));
   if (statment == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);

   hResult = DBSelectPrepared(statment);
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
   DBConnectionPoolReleaseConnection(hdb);
   DBFreeStatement(statment);
   return bResult;
}

/**
 * Agent table tool execution thread
 */
static void GetAgentTable(void *pArg)
{
   NXCPMessage msg;
   TCHAR *pszEnum, *pszRegEx, *pszLine, szBuffer[256];
   AgentConnection *pConn;
   UINT32 i, j, dwNumRows, dwNumCols, dwResult, dwLen;
   int *pnSubstrPos, nPos;
   regex_t preg;
   regmatch_t *pMatchList;
   DB_RESULT hResult;
	Table table;

   // Prepare data message
   msg.setCode(CMD_TABLE_DATA);
   msg.setId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

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
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT col_name,col_format,col_substr FROM object_tools_table_columns WHERE tool_id=? ORDER BY col_number"));
      if (statment == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
         return;
      }
      DBBind(statment, 1, DB_SQLTYPE_INTEGER, ((TOOL_STARTUP_INFO *)pArg)->dwToolId);

      hResult = DBSelectPrepared(statment);
      if (hResult != NULL)
      {
         dwNumCols = DBGetNumRows(hResult);
         if (dwNumCols > 0)
         {
            pnSubstrPos = (int *)malloc(sizeof(int) * dwNumCols);
            for(i = 0; i < dwNumCols; i++)
            {
               DBGetField(hResult, i, 0, szBuffer, 256);
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

                     msg.setField(VID_RCC, RCC_SUCCESS);
							table.fillMessage(msg, 0, -1);
                  }
                  else
                  {
                     msg.setField(VID_RCC, (dwResult == ERR_UNKNOWN_PARAMETER) ? RCC_UNKNOWN_PARAMETER : RCC_COMM_FAILURE);
                  }
                  delete pConn;
               }
               else
               {
                  msg.setField(VID_RCC, RCC_COMM_FAILURE);
               }
               regfree(&preg);
            }
            else     // Regexp compilation failed
            {
               msg.setField(VID_RCC, RCC_BAD_REGEXP);
            }
            free(pnSubstrPos);
         }
         else  // No columns defined
         {
            msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
         }
         DBFreeResult(hResult);
      }
      else     // Cannot load column info from DB
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
      DBFreeStatement(statment);
   }
   else
   {
      msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
   }

   // Send responce to client
   ((TOOL_STARTUP_INFO *)pArg)->pSession->sendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->decRefCount();
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
   free(pArg);
}

/**
 * Add SNMP variable value to results list
 */
static void AddSNMPResult(Table *table, int column, SNMP_Variable *pVar, LONG nFmt, Node *pNode)
{
   TCHAR szBuffer[4096];
   Interface *pInterface;
   UINT32 dwIndex;
	bool convert;

   if (pVar != NULL)
   {
      switch(nFmt)
      {
         case CFMT_MAC_ADDR:
            pVar->getValueAsMACAddr(szBuffer);
            break;
         case CFMT_IP_ADDR:
            pVar->getValueAsIPAddr(szBuffer);
            break;
         case CFMT_IFINDEX:   // Column is an interface index, convert to interface name
            dwIndex = pVar->getValueAsUInt();
            pInterface = pNode->findInterfaceByIndex(dwIndex);
            if (pInterface != NULL)
            {
               nx_strncpy(szBuffer, pInterface->getName(), 4096);
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

/**
 * Handler for SNMP table enumeration
 */
static UINT32 TableHandler(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   TCHAR szOid[MAX_OID_LEN * 4], szSuffix[MAX_OID_LEN * 4];
   SNMP_PDU *pRqPDU, *pRespPDU;
   UINT32 i, dwResult, pdwVarName[MAX_OID_LEN];
   size_t nameLen;

   // Create index (OID suffix) for columns
   if (((SNMP_ENUM_ARGS *)pArg)->dwFlags & TF_SNMP_INDEXED_BY_VALUE)
   {
      _sntprintf(szSuffix, MAX_OID_LEN * 4, _T(".%u"), pVar->getValueAsUInt());
   }
   else
   {
      SNMP_ObjectId *pOid;

      nameLen = SNMPParseOID(((SNMP_ENUM_ARGS *)pArg)->ppszOidList[0], pdwVarName, MAX_OID_LEN);
      pOid = pVar->getName();
      SNMPConvertOIDToText(pOid->getLength() - nameLen,
         (UINT32 *)&(pOid->getValue())[nameLen], szSuffix, MAX_OID_LEN * 4);
   }

   // Get values for other columns
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), dwVersion);
   for(i = 1; i < ((SNMP_ENUM_ARGS *)pArg)->dwNumCols; i++)
   {
      _tcscpy(szOid, ((SNMP_ENUM_ARGS *)pArg)->ppszOidList[i]);
      _tcscat(szOid, szSuffix);
      nameLen = SNMPParseOID(szOid, pdwVarName, MAX_OID_LEN);
      if (nameLen != 0)
      {
         pRqPDU->bindVariable(new SNMP_Variable(pdwVarName, nameLen));
      }
   }

   dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, SnmpGetDefaultTimeout(), 3);
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

/**
 * SNMP table tool execution thread
 */
static void GetSNMPTable(void *pArg)
{
   DB_RESULT hResult;
   NXCPMessage msg;
   UINT32 i, dwNumCols;
   TCHAR szBuffer[256];
   SNMP_ENUM_ARGS args;
	Table table;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Prepare data message
   msg.setCode(CMD_TABLE_DATA);
   msg.setId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

   DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT col_name,col_oid,col_format FROM object_tools_table_columns WHERE tool_id=? ORDER BY col_number"));
   if (statment == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return;
   }
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, ((TOOL_STARTUP_INFO *)pArg)->dwToolId);

   hResult = DBSelectPrepared(statment);
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
            args.ppszOidList[i] = DBGetField(hResult, i, 1, NULL, 0);
            args.pnFormatList[i] = DBGetFieldLong(hResult, i, 2);
				table.addColumn(szBuffer, args.pnFormatList[i]);
         }

         // Enumerate
         if (((TOOL_STARTUP_INFO *)pArg)->pNode->callSnmpEnumerate(args.ppszOidList[0], TableHandler, &args) == SNMP_ERR_SUCCESS)
         {
            // Fill in message with results
            msg.setField(VID_RCC, RCC_SUCCESS);
				table.setTitle(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
				table.fillMessage(msg, 0, -1);
         }
         else
         {
            msg.setField(VID_RCC, RCC_SNMP_ERROR);
         }

         // Cleanup
         for(i = 0; i < dwNumCols; i++)
            safe_free(args.ppszOidList[i]);
         free(args.ppszOidList);
         free(args.pnFormatList);
      }
      else
      {
         msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
      }
      DBFreeResult(hResult);
   }
   else
   {
      msg.setField(VID_RCC, RCC_DB_FAILURE);
   }

   DBConnectionPoolReleaseConnection(hdb);
   DBFreeStatement(statment);

   // Send responce to client
   ((TOOL_STARTUP_INFO *)pArg)->pSession->sendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->decRefCount();
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
   free(pArg);
}

/**
 * Execute table tool
 */
UINT32 ExecuteTableTool(UINT32 dwToolId, Node *pNode, UINT32 dwRqId, ClientSession *pSession)
{
   LONG nType;
   UINT32 dwRet = RCC_SUCCESS;
   TOOL_STARTUP_INFO *pStartup;
   DB_RESULT hResult;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT tool_type,tool_data,flags FROM object_tools WHERE tool_id=?"));
   if (statment == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);
   hResult = DBSelectPrepared(statment);
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
            pStartup->dwFlags = DBGetFieldULong(hResult, 0, 2);
            pStartup->pNode = pNode;
            pStartup->pSession = pSession;
            ThreadPoolExecute(g_mainThreadPool, (nType == TOOL_TYPE_TABLE_SNMP) ? GetSNMPTable : GetAgentTable, pStartup);
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

   DBConnectionPoolReleaseConnection(hdb);
   DBFreeStatement(statment);
   return dwRet;
}


/**
 * Delete object tool from database
 */
UINT32 DeleteObjectToolFromDB(UINT32 dwToolId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return RCC_DB_FAILURE;
	}

   DB_STATEMENT statment = DBPrepare(hdb, _T("DELETE FROM object_tools WHERE tool_id=?"));
   if (statment == NULL)
      return ReturnDBFailure(hdb, statment);
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);
   if(!DBExecute(statment))
      return ReturnDBFailure(hdb, statment);
   DBFreeStatement(statment);

   statment = DBPrepare(hdb, _T("DELETE FROM object_tools_acl WHERE tool_id=?"));
   if (statment == NULL)
      return ReturnDBFailure(hdb, statment);
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);
   if(!DBExecute(statment))
      return ReturnDBFailure(hdb, statment);
   DBFreeStatement(statment);

   statment = DBPrepare(hdb, _T("DELETE FROM object_tools_table_columns WHERE tool_id=?"));
   if (statment == NULL)
      return ReturnDBFailure(hdb, statment);
   DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);
   if(!DBExecute(statment))
      return ReturnDBFailure(hdb, statment);
   DBFreeStatement(statment);

   DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_OBJTOOL_DELETED, dwToolId);
   return RCC_SUCCESS;
}

/**
 * Change Object Tool Disable status to opposit
 */
UINT32 ChangeObjectToolStatus(UINT32 toolId, bool enabled)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT flags FROM object_tools WHERE tool_id=?"));
   if (statment == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   UINT32 rcc = RCC_SUCCESS;
   UINT32 flags = 0;

   DBBind(statment, 1, DB_SQLTYPE_INTEGER, toolId);
   DB_RESULT hResult = DBSelectPrepared(statment);
   if (hResult != NULL)
   {
      flags = DBGetFieldULong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBFreeStatement(statment);

   if (rcc == RCC_SUCCESS)
   {
      if (enabled)
      {
         flags &= ~TF_DISABLED;
      }
      else
      {
         flags |= TF_DISABLED;
      }
      rcc = RCC_DB_FAILURE;
      statment = DBPrepare(hdb, _T("UPDATE object_tools SET flags=? WHERE tool_id=?"));
      if (statment != NULL)
      {
         DBBind(statment, 1, DB_SQLTYPE_INTEGER, flags);
         DBBind(statment, 2, DB_SQLTYPE_INTEGER, toolId);
         if (DBExecute(statment))
         {
            NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, toolId);
            rcc = RCC_SUCCESS;
         }
         DBFreeStatement(statment);
      }
   }

	DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Update/Insert object tool from NXCP message
 */
UINT32 UpdateObjectToolFromMessage(NXCPMessage *pMsg)
{
   TCHAR szBuffer[MAX_DB_STRING];
   UINT32 i, dwAclSize, *pdwAcl;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return RCC_DB_FAILURE;
	}

   // Insert or update common properties
   int nType = pMsg->getFieldAsUInt16(VID_TOOL_TYPE);
   UINT32 dwToolId = pMsg->getFieldAsUInt32(VID_TOOL_ID);
   bool newTool = false;
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("object_tools"), _T("tool_id"), dwToolId))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE object_tools SET tool_name=?,tool_type=?,")
                             _T("tool_data=?,description=?,flags=?,")
                             _T("tool_filter=?,confirmation_text=?,command_name=?,")
                             _T("command_short_name=?,icon=? ")
                             _T("WHERE tool_id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools (tool_name,tool_type,")
                             _T("tool_data,description,flags,tool_filter,")
                             _T("confirmation_text,command_name,command_short_name,")
                             _T("icon,tool_id,guid) VALUES ")
                             _T("(?,?,?,?,?,?,?,?,?,?,?,?)"));
      newTool = true;
   }
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(VID_NAME), DB_BIND_DYNAMIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, nType);
   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, pMsg->getFieldAsString(VID_TOOL_DATA), DB_BIND_DYNAMIC);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(VID_DESCRIPTION), DB_BIND_DYNAMIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt32(VID_FLAGS));
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, pMsg->getFieldAsString(VID_TOOL_FILTER), DB_BIND_DYNAMIC);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(VID_CONFIRMATION_TEXT), DB_BIND_DYNAMIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(VID_COMMAND_NAME), DB_BIND_DYNAMIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(VID_COMMAND_SHORT_NAME), DB_BIND_DYNAMIC);

   size_t size;
   BYTE *imageData = pMsg->getBinaryFieldPtr(VID_IMAGE_DATA, &size);
   if (size > 0)
   {
      TCHAR *imageHexData = (TCHAR *)malloc((size * 2 + 1) * sizeof(TCHAR));
      BinToStr(imageData, size, imageHexData);
      DBBind(hStmt, 10, DB_SQLTYPE_TEXT, imageHexData, DB_BIND_DYNAMIC);
   }
   else
   {
      DBBind(hStmt, 10, DB_SQLTYPE_TEXT, _T(""), DB_BIND_STATIC);
   }

   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, dwToolId);
   if (newTool)
   {
      uuid_t guid;
      uuid_generate(guid);
      TCHAR guidText[64];
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, uuid_to_string(guid, guidText), DB_BIND_TRANSIENT);
   }

   if (!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   // Update ACL
   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwToolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   dwAclSize = pMsg->getFieldAsUInt32(VID_ACL_SIZE);
   if (dwAclSize > 0)
   {
      pdwAcl = (UINT32 *)malloc(sizeof(UINT32) * dwAclSize);
      pMsg->getFieldAsInt32Array(VID_ACL, dwAclSize, pdwAcl);
      for(i = 0; i < dwAclSize; i++)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (?,?)"));
         if (hStmt == NULL)
            return ReturnDBFailure(hdb, hStmt);
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwToolId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, pdwAcl[i]);
         if(!DBExecute(hStmt))
            return ReturnDBFailure(hdb, hStmt);
         DBFreeStatement(hStmt);
      }
   }

   // Update columns configuration
   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_table_columns WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwToolId);

   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   if ((nType == TOOL_TYPE_TABLE_SNMP) ||
       (nType == TOOL_TYPE_TABLE_AGENT))
   {
      UINT32 dwId, dwNumColumns;

      dwNumColumns = pMsg->getFieldAsUInt16(VID_NUM_COLUMNS);
      if (dwNumColumns > 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_table_columns (tool_id,")
                                _T("col_number,col_name,col_oid,col_format,col_substr) ")
                                _T("VALUES (?,?,?,?,?,?)"));
         if (hStmt == NULL)
            return ReturnDBFailure(hdb, hStmt);

         for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < dwNumColumns; i++)
         {
            pMsg->getFieldAsString(dwId++, szBuffer, MAX_DB_STRING);

            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwToolId);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, szBuffer, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(dwId++), DB_BIND_DYNAMIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt16(dwId++));
            DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt16(dwId++));

            if (!DBExecute(hStmt))
               return ReturnDBFailure(hdb, hStmt);
         }
         DBFreeStatement(hStmt);
      }
   }

   DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, dwToolId);
   return RCC_SUCCESS;
}

/**
 * Import failure exit
 */
static bool ImportFailure(DB_HANDLE hdb, DB_STATEMENT hStmt)
{
   if (hStmt != NULL)
      DBFreeStatement(hStmt);
   DBRollback(hdb);
   DBConnectionPoolReleaseConnection(hdb);
   DbgPrintf(4, _T("ImportObjectTool: database failure"));
   return false;
}

/**
 * Import object tool
 */
bool ImportObjectTool(ConfigEntry *config)
{
   const TCHAR *guid = config->getSubEntryValue(_T("guid"));
   if (guid == NULL)
   {
      DbgPrintf(4, _T("ImportObjectTool: missing GUID"));
      return false;
   }

   uuid_t temp;
   if (uuid_parse(guid, temp) == -1)
   {
      DbgPrintf(4, _T("ImportObjectTool: GUID (%s) is invalid"), guid);
      return false;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Step 1: find existing tool ID by GUID
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT tool_id FROM object_tools WHERE guid=?"));
   if (hStmt == NULL)
   {
      return ImportFailure(hdb, NULL);
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
   {
      return ImportFailure(hdb, hStmt);
   }

   UINT32 toolId;
   if (DBGetNumRows(hResult) > 0)
   {
      toolId = DBGetFieldULong(hResult, 0, 0);
   }
   else
   {
      toolId = 0;
   }
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   // Step 2: create or update tool record
	if (!DBBegin(hdb))
	{
      return ImportFailure(hdb, NULL);
	}

   if (toolId != 0)
   {
      hStmt = DBPrepare(hdb, _T("UPDATE object_tools SET tool_name=?,tool_type=?,")
                             _T("tool_data=?,description=?,flags=?,")
                             _T("tool_filter=?,confirmation_text=?,command_name=?,")
                             _T("command_short_name=?,icon=? ")
                             _T("WHERE tool_id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools (tool_name,tool_type,")
                             _T("tool_data,description,flags,tool_filter,")
                             _T("confirmation_text,command_name,command_short_name,")
                             _T("icon,tool_id,guid) VALUES ")
                             _T("(?,?,?,?,?,?,?,?,?,?,?,?)"));
   }

   if (hStmt == NULL)
	{
      return ImportFailure(hdb, NULL);
	}

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("name")), DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, config->getSubEntryValueAsInt(_T("type")));
   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, config->getSubEntryValue(_T("data")), DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("description")), DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, config->getSubEntryValueAsUInt(_T("flags")));
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, config->getSubEntryValue(_T("filter")), DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("confirmation")), DB_BIND_STATIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("commandName")), DB_BIND_STATIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("commandShortName")), DB_BIND_STATIC);
   DBBind(hStmt, 10, DB_SQLTYPE_TEXT, config->getSubEntryValue(_T("image")), DB_BIND_STATIC);
   if (toolId == 0)
   {
      toolId = CreateUniqueId(IDG_OBJECT_TOOL);
      DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, toolId);
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
   }
   else
   {
      DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, toolId);
   }

   if (!DBExecute(hStmt))
      return ImportFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   // Update ACL
   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
      return ImportFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if (!DBExecute(hStmt))
      return ImportFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   // Default ACL for imported tools - accessible by everyone
   hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (?,?)"));
   if (hStmt == NULL)
      return ImportFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, GROUP_EVERYONE);
   if (!DBExecute(hStmt))
      return ImportFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   // Update columns configuration
   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_table_columns WHERE tool_id=?"));
   if (hStmt == NULL)
      return ImportFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   if (!DBExecute(hStmt))
      return ImportFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   int toolType = config->getSubEntryValueAsInt(_T("type"));
   if ((toolType == TOOL_TYPE_TABLE_SNMP) ||
       (toolType == TOOL_TYPE_TABLE_AGENT))
   {
   	ConfigEntry *root = config->findEntry(_T("columns"));
	   if (root != NULL)
	   {
		   ObjectArray<ConfigEntry> *columns = root->getOrderedSubEntries(_T("column#*"));
         if (columns->size() > 0)
         {
            hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_table_columns (tool_id,")
                                   _T("col_number,col_name,col_oid,col_format,col_substr) ")
                                   _T("VALUES (?,?,?,?,?,?)"));
            if (hStmt == NULL)
               return ImportFailure(hdb, hStmt);

            for(int i = 0; i < columns->size(); i++)
            {
               ConfigEntry *c = columns->get(i);
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)i);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, c->getSubEntryValue(_T("name")), DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, c->getSubEntryValue(_T("oid")), DB_BIND_STATIC);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)c->getSubEntryValueAsInt(_T("format")));
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)c->getSubEntryValueAsInt(_T("captureGroup")));

               if (!DBExecute(hStmt))
               {
                  delete columns;
                  return ImportFailure(hdb, hStmt);
               }
            }
            DBFreeStatement(hStmt);
         }
         delete columns;
      }
   }

   DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, toolId);
   return true;
}

/**
 * Create export records for object tool columns
 */
static void CreateObjectToolColumnExportRecords(DB_HANDLE hdb, String &xml, UINT32 id)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT col_number,col_name,col_oid,col_format,col_substr FROM object_tools_table_columns WHERE tool_id=?"));
   if (hStmt == NULL)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         xml.append(_T("\t\t\t<columns>\n"));
         for(int i = 0; i < count; i++)
         {
            xml.append(_T("\t\t\t\t<column id=\""));
            xml.append(DBGetFieldLong(hResult, i, 0) + 1);
            xml.append(_T("\">\n\t\t\t\t\t<name>"));
            xml.appendPreallocated(DBGetFieldForXML(hResult, i, 1));
            xml.append(_T("</name>\n\t\t\t\t\t<oid>"));
            xml.appendPreallocated(DBGetFieldForXML(hResult, i, 2));
            xml.append(_T("</oid>\n\t\t\t\t\t<format>"));
            xml.append(DBGetFieldLong(hResult, i, 3));
            xml.append(_T("</format>\n\t\t\t\t\t<captureGroup>"));
            xml.append(DBGetFieldLong(hResult, i, 4));
            xml.append(_T("</captureGroup>\n\t\t\t\t</column>\n"));
         }
         xml.append(_T("\t\t\t</columns>\n"));
      }
      DBFreeResult(hResult);
   }

   DBFreeStatement(hStmt);
}

/**
 * Cerate export record for given object tool
 */
void CreateObjectToolExportRecord(String &xml, UINT32 id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT tool_name,guid,tool_type,tool_data,description,flags,tool_filter,confirmation_text,command_name,command_short_name,icon FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         xml.append(_T("\t\t<objectTool id=\""));
         xml.append(id);
         xml.append(_T("\">\n\t\t\t<name>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 0));
         xml.append(_T("</name>\n\t\t\t<guid>"));
         xml.appendPreallocated(DBGetField(hResult, 0, 1, NULL, 0));
         xml.append(_T("</guid>\n\t\t\t<type>"));
         xml.append(DBGetFieldLong(hResult, 0, 2));
         xml.append(_T("</type>\n\t\t\t<data>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 3));
         xml.append(_T("</data>\n\t\t\t<description>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 4));
         xml.append(_T("</description>\n\t\t\t<flags>"));
         xml.append(DBGetFieldLong(hResult, 0, 5));
         xml.append(_T("</flags>\n\t\t\t<filter>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 6));
         xml.append(_T("</filter>\n\t\t\t<confirmation>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 7));
         xml.append(_T("</confirmation>\n\t\t\t<commandName>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 8));
         xml.append(_T("</commandName>\n\t\t\t<commandShortName>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 9));
         xml.append(_T("</commandShortName>\n\t\t\t<image>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 10));
         xml.append(_T("</image>\n"));
         CreateObjectToolColumnExportRecords(hdb, xml, id);
         xml.append(_T("\t\t</objectTool>\n"));
      }
      DBFreeResult(hResult);
   }

   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
}
