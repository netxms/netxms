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
 * Object tool acl entry
 */
typedef struct
{
   UINT32 toolId;
   UINT32 userId;
} OBJECT_TOOL_ACL;

/**
 * Tool startup info
 */
struct TOOL_STARTUP_INFO
{
   UINT32 toolId;
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
BOOL IsTableTool(UINT32 toolId)
{
   DB_RESULT hResult;
   LONG nType;
   BOOL bResult = FALSE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT tool_type FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   hResult = DBSelectPrepared(hStmt);
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
   DBFreeStatement(hStmt);
   return bResult;
}


/**
 * Check if user has access to the tool
 */

BOOL CheckObjectToolAccess(UINT32 toolId, UINT32 userId)
{
   DB_RESULT hResult;
	int i, nRows;
	UINT32 dwId;
   BOOL bResult = FALSE;

   if (userId == 0)
      return TRUE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT user_id FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
		nRows = DBGetNumRows(hResult);
      for(i = 0; i < nRows; i++)
      {
			dwId = DBGetFieldULong(hResult, i, 0);
			if ((dwId == userId) || (dwId == GROUP_EVERYONE))
			{
				bResult = TRUE;
				break;
			}
			if (dwId & GROUP_FLAG)
			{
				if (CheckUserMembership(userId, dwId))
				{
					bResult = TRUE;
					break;
				}
			}
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   DBFreeStatement(hStmt);
   return bResult;
}

/**
 * Agent table tool execution thread
 */
static void GetAgentTable(void *pArg)
{
   NXCPMessage msg;
   TCHAR *pszEnum, *pszRegEx, *pszLine, buffer[256];
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
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT col_name,col_format,col_substr FROM object_tools_table_columns WHERE tool_id=? ORDER BY col_number"));
      if (hStmt == NULL)
      {
         DBConnectionPoolReleaseConnection(hdb);
         return;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ((TOOL_STARTUP_INFO *)pArg)->toolId);

      hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         dwNumCols = DBGetNumRows(hResult);
         if (dwNumCols > 0)
         {
            pnSubstrPos = (int *)malloc(sizeof(int) * dwNumCols);
            for(i = 0; i < dwNumCols; i++)
            {
               DBGetField(hResult, i, 0, buffer, 256);
					table.addColumn(buffer, DBGetFieldULong(hResult, i, 1));
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
                              memcpy(buffer, &pszLine[pMatchList[nPos].rm_so], dwLen * sizeof(TCHAR));
                              buffer[dwLen] = 0;
										table.set(j, buffer);
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
      DBFreeStatement(hStmt);
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
   TCHAR buffer[4096];
   Interface *pInterface;
   UINT32 dwIndex;
	bool convert;

   if (pVar != NULL)
   {
      switch(nFmt)
      {
         case CFMT_MAC_ADDR:
            pVar->getValueAsMACAddr(buffer);
            break;
         case CFMT_IP_ADDR:
            pVar->getValueAsIPAddr(buffer);
            break;
         case CFMT_IFINDEX:   // Column is an interface index, convert to interface name
            dwIndex = pVar->getValueAsUInt();
            pInterface = pNode->findInterfaceByIndex(dwIndex);
            if (pInterface != NULL)
            {
               nx_strncpy(buffer, pInterface->getName(), 4096);
            }
            else
            {
               if (dwIndex == 0)    // Many devices uses index 0 for internal MAC, etc.
                  _tcscpy(buffer, _T("INTERNAL"));
               else
                  _sntprintf(buffer, 64, _T("%d"), dwIndex);
            }
            break;
         default:
				convert = true;
            pVar->getValueAsPrintableString(buffer, 4096, &convert);
            break;
      }
   }
   else
   {
      buffer[0] = 0;
   }
	table->set(column, buffer);
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
   TCHAR buffer[256];
   SNMP_ENUM_ARGS args;
	Table table;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Prepare data message
   msg.setCode(CMD_TABLE_DATA);
   msg.setId(((TOOL_STARTUP_INFO *)pArg)->dwRqId);

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT col_name,col_oid,col_format FROM object_tools_table_columns WHERE tool_id=? ORDER BY col_number"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, ((TOOL_STARTUP_INFO *)pArg)->toolId);

   hResult = DBSelectPrepared(hStmt);
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
            DBGetField(hResult, i, 0, buffer, 256);
            args.ppszOidList[i] = DBGetField(hResult, i, 1, NULL, 0);
            args.pnFormatList[i] = DBGetFieldLong(hResult, i, 2);
				table.addColumn(buffer, args.pnFormatList[i]);
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
   DBFreeStatement(hStmt);

   // Send responce to client
   ((TOOL_STARTUP_INFO *)pArg)->pSession->sendMessage(&msg);
   ((TOOL_STARTUP_INFO *)pArg)->pSession->decRefCount();
   safe_free(((TOOL_STARTUP_INFO *)pArg)->pszToolData);
   free(pArg);
}

/**
 * Execute table tool
 */
UINT32 ExecuteTableTool(UINT32 toolId, Node *pNode, UINT32 dwRqId, ClientSession *pSession)
{
   LONG nType;
   UINT32 dwRet = RCC_SUCCESS;
   TOOL_STARTUP_INFO *pStartup;
   DB_RESULT hResult;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT tool_type,tool_data,flags FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         nType = DBGetFieldLong(hResult, 0, 0);
         if ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT))
         {
            pSession->incRefCount();
            pStartup = (TOOL_STARTUP_INFO *)malloc(sizeof(TOOL_STARTUP_INFO));
            pStartup->toolId = toolId;
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
   DBFreeStatement(hStmt);
   return dwRet;
}


/**
 * Delete object tool from database
 */
UINT32 DeleteObjectToolFromDB(UINT32 toolId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return RCC_DB_FAILURE;
	}

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_table_columns WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_input_fields WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_OBJTOOL_DELETED, toolId);
   return RCC_SUCCESS;
}

/**
 * Change Object Tool Disable status to opposit
 */
UINT32 ChangeObjectToolStatus(UINT32 toolId, bool enabled)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT flags FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   UINT32 rcc = RCC_SUCCESS;
   UINT32 flags = 0;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      flags = DBGetFieldULong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBFreeStatement(hStmt);

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
      hStmt = DBPrepare(hdb, _T("UPDATE object_tools SET flags=? WHERE tool_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, flags);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, toolId);
         if (DBExecute(hStmt))
         {
            NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, toolId);
            rcc = RCC_SUCCESS;
         }
         DBFreeStatement(hStmt);
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
   TCHAR buffer[MAX_DB_STRING];
   UINT32 i, aclSize, *pdwAcl;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return RCC_DB_FAILURE;
	}

   // Insert or update common properties
   int nType = pMsg->getFieldAsUInt16(VID_TOOL_TYPE);
   UINT32 toolId = pMsg->getFieldAsUInt32(VID_TOOL_ID);
   bool newTool = false;
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("object_tools"), _T("tool_id"), toolId))
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

   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, toolId);
   if (newTool)
   {
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, uuid::generate());
   }

   if (!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   // Update ACL
   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   if(!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   aclSize = pMsg->getFieldAsUInt32(VID_ACL_SIZE);
   if (aclSize > 0)
   {
      pdwAcl = (UINT32 *)malloc(sizeof(UINT32) * aclSize);
      pMsg->getFieldAsInt32Array(VID_ACL, aclSize, pdwAcl);
      for(i = 0; i < aclSize; i++)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (?,?)"));
         if (hStmt == NULL)
            return ReturnDBFailure(hdb, hStmt);
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
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
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   if (!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   if ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT))
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
            pMsg->getFieldAsString(dwId++, buffer, MAX_DB_STRING);

            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, buffer, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(dwId++), DB_BIND_DYNAMIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt16(dwId++));
            DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt16(dwId++));

            if (!DBExecute(hStmt))
               return ReturnDBFailure(hdb, hStmt);
         }
         DBFreeStatement(hStmt);
      }
   }

   hStmt = DBPrepare(hdb, _T("DELETE FROM object_tools_input_fields WHERE tool_id=?"));
   if (hStmt == NULL)
      return ReturnDBFailure(hdb, hStmt);
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   if (!DBExecute(hStmt))
      return ReturnDBFailure(hdb, hStmt);
   DBFreeStatement(hStmt);

   UINT32 numFields = pMsg->getFieldAsUInt16(VID_NUM_FIELDS);
   if (numFields > 0)
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_input_fields (tool_id,name,input_type,display_name,config,sequence_num) VALUES (?,?,?,?,?,?)"));
      if (hStmt == NULL)
         return ReturnDBFailure(hdb, hStmt);
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

      UINT32 fieldId = VID_FIELD_LIST_BASE;
      for(i = 0; i < numFields; i++)
      {
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, pMsg->getFieldAsUInt16(fieldId++));
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pMsg->getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
         DBBind(hStmt, 5, DB_SQLTYPE_TEXT, pMsg->getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, pMsg->getFieldAsInt16(fieldId++));
         fieldId += 5;

         if (!DBExecute(hStmt))
            return ReturnDBFailure(hdb, hStmt);
      }
   }

   DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_OBJTOOLS_CHANGED, toolId);
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
   if (_uuid_parse(guid, temp) == -1)
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
   if ((toolType == TOOL_TYPE_TABLE_SNMP) || (toolType == TOOL_TYPE_TABLE_AGENT))
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

	ConfigEntry *inputFieldsRoot = config->findEntry(_T("inputFields"));
   if (inputFieldsRoot != NULL)
   {
	   ObjectArray<ConfigEntry> *inputFields = inputFieldsRoot->getOrderedSubEntries(_T("inputField#*"));
      if (inputFields->size() > 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_tools_input_fields (tool_id,name,input_type,display_name,config) VALUES (?,?,?,?,?)"));
         if (hStmt == NULL)
            return ImportFailure(hdb, hStmt);

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
         for(int i = 0; i < inputFields->size(); i++)
         {
            ConfigEntry *c = inputFields->get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, c->getSubEntryValue(_T("name")), DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)c->getSubEntryValueAsInt(_T("type")));
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, c->getSubEntryValue(_T("displayName")), DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_TEXT, c->getSubEntryValue(_T("config")), DB_BIND_STATIC);

            if (!DBExecute(hStmt))
            {
               delete inputFields;
               return ImportFailure(hdb, hStmt);
            }
         }
         DBFreeStatement(hStmt);
      }
      delete inputFields;
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
 * Create export records for object tool input fields
 */
static void CreateObjectToolInputFieldExportRecords(DB_HANDLE hdb, String &xml, UINT32 id)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name,input_type,display_name,config FROM object_tools_input_fields WHERE tool_id=?"));
   if (hStmt == NULL)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         xml.append(_T("\t\t\t<inputFields>\n"));
         for(int i = 0; i < count; i++)
         {
            xml.append(_T("\t\t\t\t<inputField id=\""));
            xml.append(i + 1);
            xml.append(_T("\">\n\t\t\t\t\t<name>"));
            xml.appendPreallocated(DBGetFieldForXML(hResult, i, 0));
            xml.append(_T("</name>\n\t\t\t\t\t<type>"));
            xml.append(DBGetFieldLong(hResult, i, 1));
            xml.append(_T("</type>\n\t\t\t\t\t<displayName>"));
            xml.appendPreallocated(DBGetFieldForXML(hResult, i, 2));
            xml.append(_T("</displayName>\n\t\t\t\t\t<config>"));
            xml.appendPreallocated(DBGetFieldForXML(hResult, i, 3));
            xml.append(_T("</config>\n\t\t\t\t</inputField>\n"));
         }
         xml.append(_T("\t\t\t</inputFields>\n"));
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
         CreateObjectToolInputFieldExportRecords(hdb, xml, id);
         xml.append(_T("\t\t</objectTool>\n"));
      }
      DBFreeResult(hResult);
   }

   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Load object tool's input field definitions
 */
static bool LoadInputFieldDefinitions(UINT32 toolId, DB_HANDLE hdb, NXCPMessage *msg, UINT32 countFieldId, UINT32 baseFieldId)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name,input_type,display_name,config,sequence_num FROM object_tools_input_fields WHERE tool_id=? ORDER BY name"));
   if (hStmt == NULL)
      return false;

   bool success = false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      msg->setField(countFieldId, (UINT16)count);
      UINT32 fieldId = baseFieldId;
      for(int i = 0; i < count; i++)
      {
         TCHAR buffer[128];

         DBGetField(hResult, i, 0, buffer, 128);
         msg->setField(fieldId++, buffer);

         msg->setField(fieldId++, (INT16)DBGetFieldLong(hResult, i, 1));

         DBGetField(hResult, i, 2, buffer, 128);
         msg->setField(fieldId++, buffer);

         TCHAR *cfg = DBGetField(hResult, i, 3, NULL, 0);
         msg->setField(fieldId++, cfg);

         int seq = DBGetFieldLong(hResult, i, 4);
         if (seq == -1)
            seq = i;
         msg->setField(fieldId++, (INT16)seq);

         fieldId += 5;
      }
      DBFreeResult(hResult);
      success = true;
   }

   DBFreeStatement(hStmt);
   return success;
}

/**
 * Get all object tools available for given user into NXCP message
 */
UINT32 GetObjectToolsIntoMessage(NXCPMessage *msg, UINT32 userId, bool fullAccess)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT tool_id,user_id FROM object_tools_acl"));
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   int aclSize = DBGetNumRows(hResult);
   OBJECT_TOOL_ACL *acl = (OBJECT_TOOL_ACL *)malloc(aclSize * sizeof(OBJECT_TOOL_ACL));
   for(int i = 0; i < aclSize; i++)
   {
      acl[i].toolId = DBGetFieldULong(hResult, i, 0);
      acl[i].userId = DBGetFieldULong(hResult, i, 1);
   }
   DBFreeResult(hResult);

   hResult = DBSelect(hdb, _T("SELECT tool_id,tool_name,tool_type,tool_data,flags,description,tool_filter,confirmation_text,command_name,command_short_name,icon FROM object_tools"));
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      free(acl);
      return RCC_DB_FAILURE;
   }

   UINT32 recordCount = 0;
   UINT32 fieldId = VID_OBJECT_TOOLS_BASE;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      UINT32 toolId = DBGetFieldULong(hResult, i, 0);
      bool hasAccess = fullAccess;
      if (!fullAccess)
      {
         for(int j = 0; j < aclSize; j++)
         {
            if (acl[j].toolId == toolId)
            {
               if ((acl[j].userId == userId) ||
                   (acl[j].userId == GROUP_EVERYONE) ||
                   ((acl[j].userId & GROUP_FLAG) && CheckUserMembership(userId, acl[j].userId)))
               {
                  hasAccess = true;
                  break;
               }
            }
         }
      }

      if (hasAccess)
      {
         TCHAR buffer[MAX_DB_STRING];

         msg->setField(fieldId, toolId);

         // name
         DBGetField(hResult, i, 1, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 1, buffer);

         msg->setField(fieldId + 2, (WORD)DBGetFieldLong(hResult, i, 2));

         // data
         TCHAR *data = DBGetField(hResult, i, 3, NULL, 0);
         msg->setField(fieldId + 3, data);
         free(data);

         msg->setField(fieldId + 4, DBGetFieldULong(hResult, i, 4));

         // description
         DBGetField(hResult, i, 5, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 5, buffer);

         // matching OID
         DBGetField(hResult, i, 6, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 6, buffer);

         // confirmation text
         DBGetField(hResult, i, 7, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 7, buffer);

         // command name
         DBGetField(hResult, i, 8, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 8, buffer);

         // command short name
         DBGetField(hResult, i, 9, buffer, MAX_DB_STRING);
         msg->setField(fieldId + 9, buffer);

         // icon
         TCHAR *imageDataHex = DBGetField(hResult, i, 10, NULL, 0);
         if (imageDataHex != NULL)
         {
            size_t size = _tcslen(imageDataHex) / 2;
            BYTE *imageData = (BYTE *)malloc(size);
            size_t bytes = StrToBin(imageDataHex, imageData, size);
            msg->setField(fieldId + 10, imageData, (UINT32)bytes);
            free(imageData);
            free(imageDataHex);
         }
         else
         {
            msg->setField(fieldId + 10, (BYTE *)NULL, 0);
         }

         LoadInputFieldDefinitions(toolId, hdb, msg, fieldId + 11, fieldId + 20);

         recordCount++;
         fieldId += 10000;
      }
   }
   msg->setField(VID_NUM_TOOLS, recordCount);

   DBFreeResult(hResult);
   free(acl);

   DBConnectionPoolReleaseConnection(hdb);
   return RCC_SUCCESS;
}

/**
 * Get object tool details into NXCP message
 */
UINT32 GetObjectToolDetailsIntoMessage(UINT32 toolId, NXCPMessage *msg)
{
   UINT32 rcc = RCC_DB_FAILURE;
   DB_RESULT hResult = NULL;
   TCHAR buffer[MAX_DB_STRING], *data;
   int aclSize, toolType;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT tool_name,tool_type,tool_data,description,flags,tool_filter,confirmation_text,command_name,command_short_name,icon FROM object_tools WHERE tool_id=?"));
   if (hStmt == NULL)
      goto cleanup;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
      goto cleanup;

   if (DBGetNumRows(hResult) == 0)
   {
      rcc = RCC_INVALID_TOOL_ID;
      goto cleanup;
   }

	msg->setField(VID_TOOL_ID, toolId);
   msg->setField(VID_NAME, DBGetField(hResult, 0, 0, buffer, MAX_DB_STRING));

   toolType = DBGetFieldLong(hResult, 0, 1);
   msg->setField(VID_TOOL_TYPE, (INT16)toolType);

   data = DBGetField(hResult, 0, 2, NULL, 0);
   msg->setField(VID_TOOL_DATA, data);
   free(data);

   msg->setField(VID_DESCRIPTION, DBGetField(hResult, 0, 3, buffer, MAX_DB_STRING));
   msg->setField(VID_FLAGS, DBGetFieldULong(hResult, 0, 4));
   msg->setField(VID_TOOL_FILTER, DBGetField(hResult, 0, 5, buffer, MAX_DB_STRING));
   msg->setField(VID_CONFIRMATION_TEXT, DBGetField(hResult, 0, 6, buffer, MAX_DB_STRING));
   msg->setField(VID_COMMAND_NAME, DBGetField(hResult, 0, 7, buffer, MAX_DB_STRING));
   msg->setField(VID_COMMAND_SHORT_NAME, DBGetField(hResult, 0, 8, buffer, MAX_DB_STRING));

   // icon
   data = DBGetField(hResult, 0, 9, NULL, 0);
   if (data != NULL)
   {
      size_t size = _tcslen(data) / 2;
      BYTE *imageData = (BYTE *)malloc(size);
      size_t bytes = StrToBin(data, imageData, size);
      msg->setField(VID_IMAGE_DATA, imageData, (UINT32)bytes);
      free(imageData);
      free(data);
   }
   else
   {
      msg->setField(VID_IMAGE_DATA, (BYTE *)NULL, 0);
   }

   DBFreeResult(hResult);
   hResult = NULL;
   DBFreeStatement(hStmt);

   // Access list
   hStmt = DBPrepare(hdb, _T("SELECT user_id FROM object_tools_acl WHERE tool_id=?"));
   if (hStmt == NULL)
      goto cleanup;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

   hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
      goto cleanup;

   aclSize = DBGetNumRows(hResult);
   msg->setField(VID_ACL_SIZE, (UINT32)aclSize);
   if (aclSize > 0)
   {
      UINT32 *acl = (UINT32 *)malloc(sizeof(UINT32) * aclSize);
      for(int i = 0; i < aclSize; i++)
         acl[i] = DBGetFieldULong(hResult, i, 0);
      msg->setFieldFromInt32Array(VID_ACL, aclSize, acl);
      free(acl);
   }
   DBFreeResult(hResult);
   hResult = NULL;

   // Column information for table tools
   if ((toolType == TOOL_TYPE_TABLE_SNMP) || (toolType == TOOL_TYPE_TABLE_AGENT))
   {
      DBFreeStatement(hStmt);
      hStmt = DBPrepare(hdb, _T("SELECT col_name,col_oid,col_format,col_substr ")
                             _T("FROM object_tools_table_columns WHERE tool_id=? ")
                             _T("ORDER BY col_number"));
      if (hStmt == NULL)
         goto cleanup;
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, toolId);

      hResult = DBSelectPrepared(hStmt);
      if (hResult == NULL)
         goto cleanup;

      int count = DBGetNumRows(hResult);
      msg->setField(VID_NUM_COLUMNS, (INT16)count);
      UINT32 fieldId = VID_COLUMN_INFO_BASE;
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);
         msg->setField(fieldId++, buffer);
         msg->setField(fieldId++, DBGetField(hResult, i, 1, buffer, MAX_DB_STRING));
         msg->setField(fieldId++, (UINT16)DBGetFieldLong(hResult, i, 2));
         msg->setField(fieldId++, (UINT16)DBGetFieldLong(hResult, i, 3));
      }
      DBFreeResult(hResult);
      hResult = NULL;
   }

   if (!LoadInputFieldDefinitions(toolId, hdb, msg, VID_NUM_FIELDS, VID_FIELD_LIST_BASE))
      goto cleanup;

   rcc = RCC_SUCCESS;

cleanup:
   if (hResult != NULL)
      DBFreeResult(hResult);
   if (hStmt != NULL)
      DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);

   if (rcc != RCC_SUCCESS)
      msg->deleteAllFields();
   return rcc;
}

/**
 * Command execution data constructor
 */
ServerCommandExecData::ServerCommandExecData(TCHAR *command, bool sendOutput, UINT32 requestId, ClientSession *session)
{
   m_command = command;
   m_sendOutput = sendOutput;
   if (m_sendOutput)
   {
      m_requestId = requestId;
      m_session = session;
      m_session->incRefCount();
   }
   else
   {
      m_requestId = 0;
      m_session = NULL;
   }
}

/**
 * Command execution data destructor
 */
ServerCommandExecData::~ServerCommandExecData()
{
   if (m_session != NULL)
      m_session->decRefCount();
   free(m_command);
}

/**
 * Worker thread for server side command execution
 */
void ExecuteServerCommand(void *arg)
{
   ServerCommandExecData *data = (ServerCommandExecData *)arg;
   DbgPrintf(5, _T("Running server-side command: %s"), data->getCommand());
   if (data->sendOutput())
   {
      NXCPMessage msg;
      msg.setCode(CMD_REQUEST_COMPLETED);
      msg.setId(data->getRequestId());

      FILE *pipe = _tpopen(data->getCommand(), _T("r"));
      if (pipe != NULL)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         data->getSession()->sendMessage(&msg);

         msg.deleteAllFields();
         msg.setCode(CMD_COMMAND_OUTPUT);
         while(true)
         {
            TCHAR line[4096];

            TCHAR *ret = safe_fgetts(line, 4096, pipe);
            if (ret == NULL)
               break;

            msg.setField(VID_MESSAGE, line);
            data->getSession()->sendMessage(&msg);
            msg.deleteAllFields();
         }

         pclose(pipe);
         msg.deleteAllFields();
         msg.setEndOfSequence();
      }
      else
      {
         msg.setField(VID_RCC, RCC_EXEC_FAILED);
      }
      data->getSession()->sendMessage(&msg);
   }
   else
   {
	   if (_tsystem(data->getCommand()) == -1)
         DbgPrintf(5, _T("Failed to execute command \"%s\""), data->getCommand());
   }
   delete data;
}
