/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Raden Solutions
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
** File: graph.cpp
**
**/

#include "nxcore.h"

/**
 * Load graph's ACL - load for all graphs if graphId is 0
 */
GRAPH_ACL_ENTRY *LoadGraphACL(DB_HANDLE hdb, UINT32 graphId, int *pnACLSize)
{
   int i, nSize;
   GRAPH_ACL_ENTRY *pACL = NULL;
   DB_RESULT hResult;

   if (graphId == 0)
   {
      hResult = DBSelect(hdb, _T("SELECT graph_id,user_id,user_rights FROM graph_acl"));
   }
   else
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT graph_id,user_id,user_rights FROM graph_acl WHERE graph_id=%d"), graphId);
      hResult = DBSelect(hdb, szQuery);
   }
   if (hResult != NULL)
   {
      nSize = DBGetNumRows(hResult);
      if (nSize > 0)
      {
         pACL = (GRAPH_ACL_ENTRY *)malloc(sizeof(GRAPH_ACL_ENTRY) * nSize);
         for(i = 0; i < nSize; i++)
         {
            pACL[i].dwGraphId = DBGetFieldULong(hResult, i, 0);
            pACL[i].dwUserId = DBGetFieldULong(hResult, i, 1);
            pACL[i].dwAccess = DBGetFieldULong(hResult, i, 2);
         }
      }
      *pnACLSize = nSize;
      DBFreeResult(hResult);
   }
   else
   {
      *pnACLSize = -1;  // Database error
   }
   return pACL;
}

/**
 * Check access to the graph
 */
BOOL CheckGraphAccess(GRAPH_ACL_ENTRY *pACL, int nACLSize, UINT32 graphId, UINT32 graphUserId, UINT32 graphDesiredAccess)
{
   int i;

   for(i = 0; i < nACLSize; i++)
   {
      if (pACL[i].dwGraphId == graphId)
      {
         if ((pACL[i].dwUserId == graphUserId) ||
             ((pACL[i].dwUserId & GROUP_FLAG) && CheckUserMembership(graphUserId, pACL[i].dwUserId)))
         {
            if ((pACL[i].dwAccess & graphDesiredAccess) == graphDesiredAccess)
               return TRUE;
         }
      }
   }
   return FALSE;
}

/**
 * Check access to the graph
 */
UINT32 GetGraphAccessCheckResult(UINT32 graphId, UINT32 graphUserId)
{
   // Check existence and access rights
   TCHAR szQuery[16384];
   DB_RESULT hResult;
   UINT32 dwOwner;
   GRAPH_ACL_ENTRY *pACL = NULL;
   int nACLSize;
   UINT32 rcc = RCC_DB_FAILURE;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT owner_id FROM graphs WHERE graph_id=%d"), graphId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         dwOwner = DBGetFieldULong(hResult, 0, 0);
         pACL = LoadGraphACL(hdb, graphId, &nACLSize);
         if (nACLSize != -1)
         {
            if ((graphUserId == 0) ||
                (graphUserId == dwOwner) ||
                CheckGraphAccess(pACL, nACLSize, graphId, graphUserId, NXGRAPH_ACCESS_WRITE))
            {
               rcc = RCC_SUCCESS;
            }
            else
            {
               rcc = RCC_ACCESS_DENIED;
            }
            safe_free(pACL);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
      }
      else
      {
         rcc = RCC_INVALID_GRAPH_ID;
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
};

/**
 * Check if graph name already exist
 */
GRAPH_ACL_AND_ID IsGraphNameExists(const TCHAR *graphName)
{
   TCHAR szQuery[256];
   GRAPH_ACL_ENTRY *pACL = NULL;
   DB_RESULT hResult;
   GRAPH_ACL_AND_ID result;
   result.graphId = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check existence and access rights
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT graph_id FROM graphs WHERE name=%s"),
              (const TCHAR *)DBPrepareString(hdb, graphName));
   hResult = DBSelect(hdb, szQuery);

   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         result.graphId = DBGetFieldULong(hResult, 0, 0);
         result.status = RCC_OBJECT_ALREADY_EXISTS;
      }
      else
      {
         result.status = RCC_SUCCESS;
      }
      DBFreeResult(hResult);
   }
   else
   {
      result.status = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Fills message with available graph list
 */
void FillGraphListMsg(NXCPMessage *msg, UINT32 userId, bool templageGraphs)
{
	int nACLSize;
	TCHAR *pszStr;
	UINT32 dwId, numGraphs;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	GRAPH_ACL_ENTRY *pACL = LoadGraphACL(hdb, 0, &nACLSize);
	if (nACLSize != -1)
	{
		DB_RESULT hResult = DBSelect(hdb, _T("SELECT graph_id,owner_id,flags,name,config,filters FROM graphs"));
		if (hResult != NULL)
		{
			UINT32 *pdwUsers = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
			UINT32 *pdwRights = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
			int nRows = DBGetNumRows(hResult);
			int i;
			for(i = 0, numGraphs = 0, dwId = VID_GRAPH_LIST_BASE; i < nRows; i++)
			{
				UINT32 dwGraphId = DBGetFieldULong(hResult, i, 0);
				UINT32 dwOwner = DBGetFieldULong(hResult, i, 1);
				UINT32 flags = DBGetFieldULong(hResult, i, 2);
				if (((userId == 0) ||
				    (userId == dwOwner) ||
				    CheckGraphAccess(pACL, nACLSize, dwGraphId, userId, NXGRAPH_ACCESS_READ)) &&
				    (((GRAPH_FLAG_TEMPLATE & flags) > 0) == templageGraphs))
				{
					msg->setField(dwId++, dwGraphId);
					msg->setField(dwId++, dwOwner);
               msg->setField(dwId++, flags);

					pszStr = DBGetField(hResult, i, 3, NULL, 0);
               msg->setField(dwId++, CHECK_NULL_EX(pszStr));
               safe_free(pszStr);

					pszStr = DBGetField(hResult, i, 4, NULL, 0);
               msg->setField(dwId++, CHECK_NULL_EX(pszStr));
               safe_free(pszStr);

					pszStr = DBGetField(hResult, i, 5, NULL, 0);
               msg->setField(dwId++, CHECK_NULL_EX(pszStr));
               safe_free(pszStr);

					// ACL for graph
               UINT32 graphACLSize;
               int j;
					for(j = 0, graphACLSize = 0; j < nACLSize; j++)
					{
						if (pACL[j].dwGraphId == dwGraphId)
						{
							pdwUsers[graphACLSize] = pACL[j].dwUserId;
							pdwRights[graphACLSize] = pACL[j].dwAccess;
							graphACLSize++;
						}
					}
					msg->setField(dwId++, graphACLSize);
					msg->setFieldFromInt32Array(dwId++, graphACLSize, pdwUsers);
					msg->setFieldFromInt32Array(dwId++, graphACLSize, pdwRights);

					dwId += 1;
					numGraphs++;

				}
			}
			DBFreeResult(hResult);
			free(pdwUsers);
			free(pdwRights);
			msg->setField(VID_NUM_GRAPHS, numGraphs);
			msg->setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg->setField(VID_RCC, RCC_DB_FAILURE);
		}
		safe_free(pACL);
	}
	else
	{
		msg->setField(VID_RCC, RCC_DB_FAILURE);
	}

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Save graph
 */
void SaveGraph(NXCPMessage *pRequest, UINT32 userId, NXCPMessage *msg)
{
	bool isNew, sucess;
	UINT32 id, graphUserId, graphAccess, accessRightStatus, flags;
	TCHAR *name, *config, *filters;
	TCHAR szQuery[16384], dwGraphName[255];
	int i, nACLSize;

   UINT32 graphId = pRequest->getFieldAsUInt32(VID_GRAPH_ID);
	pRequest->getFieldAsString(VID_NAME,dwGraphName,255);
	UINT16 overwrite = pRequest->getFieldAsUInt16(VID_OVERVRITE);

   GRAPH_ACL_AND_ID nameUniq = IsGraphNameExists(dwGraphName);

   if (nameUniq.graphId == graphId)
   {
      nameUniq.status = RCC_SUCCESS;
   }

	if (graphId == 0)
	{
		graphId = nameUniq.graphId ? nameUniq.graphId : CreateUniqueId(IDG_GRAPH);
		isNew = true;
		accessRightStatus = RCC_SUCCESS;
	}
	else
	{
	   accessRightStatus = GetGraphAccessCheckResult(graphId, userId);
		isNew = false;
	}

   if (accessRightStatus == RCC_SUCCESS && (nameUniq.status == RCC_SUCCESS || (overwrite && isNew)))
   {
      sucess = true;
      if (nameUniq.status != RCC_SUCCESS)
      {
         isNew = false;
         graphId = nameUniq.graphId;
      }
   }
   else
   {
      sucess = false;
      msg->setField(VID_RCC, accessRightStatus ? accessRightStatus : nameUniq.status);
   }

	// Create/update graph
	if (sucess)
	{
		DbgPrintf(5, _T("%s graph %d"), isNew ? _T("Creating") : _T("Updating"), graphId);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      flags = pRequest->getFieldAsUInt32(VID_FLAGS);
      name = pRequest->getFieldAsString(VID_NAME);
      config = pRequest->getFieldAsString(VID_GRAPH_CONFIG);
      filters = pRequest->getFieldAsString(VID_FILTER);
		if (DBBegin(hdb))
		{
         DB_STATEMENT hStmt;

			if (isNew)
			{
            hStmt = DBPrepare(hdb, _T("INSERT INTO graphs (flags,name,config,filters,graph_id,owner_id) VALUES (?,?,?,?,?,?)"));
			}
			else
			{
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graph_acl WHERE graph_id=%d"), graphId);
				sucess = DBQuery(hdb, szQuery);

            hStmt = DBPrepare(hdb, _T("UPDATE graphs SET flags=?,name=?,config=?,filters=?  WHERE graph_id=?"));
			}

			if(hStmt != NULL && sucess)
			{
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, 255);
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, config, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_TEXT, filters, DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, graphId);
            if(isNew)
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, userId);
            sucess = DBExecute(hStmt);
            DBFreeStatement(hStmt);
			}
			else
			{
            sucess = false;
			}

			if (sucess)
			{
				// Insert new ACL
				nACLSize = (int)pRequest->getFieldAsUInt32(VID_ACL_SIZE);
				for(i = 0, id = VID_GRAPH_ACL_BASE, sucess = true; i < nACLSize; i++)
				{
					graphUserId = pRequest->getFieldAsUInt32(id++);
					graphAccess = pRequest->getFieldAsUInt32(id++);
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO graph_acl (graph_id,user_id,user_rights) VALUES (%d,%d,%d)"),
					          graphId, graphUserId, graphAccess);
					if (!DBQuery(hdb, szQuery))
					{
						sucess = false;
						msg->setField(VID_RCC, RCC_DB_FAILURE);
						break;
					}
				}
			}
			else
			{
            msg->setField(VID_RCC, RCC_DB_FAILURE);
			}

			if (sucess)
			{
				DBCommit(hdb);
				msg->setField(VID_RCC, RCC_SUCCESS);
				msg->setField(VID_GRAPH_ID, graphId);

            //send notificaion
				NXCPMessage update;
				int dwId = VID_GRAPH_LIST_BASE;
            update.setCode(CMD_GRAPH_UPDATE);
            update.setField(dwId++, graphId);
            update.setField(dwId++, userId);
            update.setField(dwId++, flags);
            update.setField(dwId++, name);
            update.setField(dwId++, config);
            update.setField(dwId++, filters);

            int nACLSize;
            GRAPH_ACL_ENTRY *pACL = LoadGraphACL(hdb, 0, &nACLSize);
            if ((pACL != NULL) && (nACLSize > 0))
            {
               UINT32 *pdwUsers = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
               UINT32 *pdwRights = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
               // ACL for graph
               for(int j = 0; j < nACLSize; j++)
               {
                  pdwUsers[j] = pACL[j].dwUserId;
                  pdwRights[j] = pACL[j].dwAccess;
               }
               update.setField(dwId++, nACLSize);
               update.setFieldFromInt32Array(dwId++, nACLSize, pdwUsers);
               update.setFieldFromInt32Array(dwId++, nACLSize, pdwRights);

               free(pdwUsers);
               free(pdwRights);
            }
            else
            {
               update.setField(dwId++, (INT32)0);
               update.setFieldFromInt32Array(dwId++, 0, &graphId);
               update.setFieldFromInt32Array(dwId++, 0, &graphId);
            }
            update.setField(VID_NUM_GRAPHS, 1);

            NotifyClientGraphUpdate(&update, graphId);
			}
			else
			{
				DBRollback(hdb);
			}
			free(name);
			free(config);
			free(filters);
		}
		else
		{
			msg->setField(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
}

/**
 * Delete graph
 */
UINT32 DeleteGraph(UINT32 graphId, UINT32 userId)
{
	UINT32 owner;
	TCHAR szQuery[256];

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   UINT32 result = RCC_DB_FAILURE;

	_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT owner_id FROM graphs WHERE graph_id=%d"), graphId);
	DB_RESULT hResult = DBSelect(hdb, szQuery);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			owner = DBGetFieldULong(hResult, 0, 0);
         int aclSize;
			GRAPH_ACL_ENTRY *acl = LoadGraphACL(hdb, graphId, &aclSize);
			if (aclSize != -1)
			{
				if ((userId == 0) ||
				    (userId == owner) ||
				    CheckGraphAccess(acl, aclSize, graphId, userId, NXGRAPH_ACCESS_READ))
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graphs WHERE graph_id=%d"), graphId);
					if (DBQuery(hdb, szQuery))
					{
						_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graph_acl WHERE graph_id=%d"), graphId);
						DBQuery(hdb, szQuery);
						result = RCC_SUCCESS;
						NotifyClientSessions(NX_NOTIFY_GRAPHS_DELETED, graphId);
					}
				}
				else
				{
					result = RCC_ACCESS_DENIED;
				}
				safe_free(acl);
			}
		}
		else
		{
			result = RCC_INVALID_GRAPH_ID;
		}
		DBFreeResult(hResult);
	}

   DBConnectionPoolReleaseConnection(hdb);
}
