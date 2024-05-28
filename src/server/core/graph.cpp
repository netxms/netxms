/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
#include <nms_users.h>

#define DEBUG_TAG _T("graphs")

/**
 * Graph ACL entry
 */
struct GRAPH_ACL_ENTRY
{
   uint32_t graphId;
   uint32_t userId;
   uint32_t access;
};

/**
 * Process result set from ACL loading
 */
static StructArray<GRAPH_ACL_ENTRY> *ProcessGraphACLResultSet(DB_RESULT hResult)
{
   if (hResult == nullptr)
      return nullptr;

   int count = DBGetNumRows(hResult);
   auto acl = new StructArray<GRAPH_ACL_ENTRY>(count);
   for(int i = 0; i < count; i++)
   {
      GRAPH_ACL_ENTRY *e = acl->addPlaceholder();
      e->graphId = DBGetFieldULong(hResult, i, 0);
      e->userId = DBGetFieldULong(hResult, i, 1);
      e->access = DBGetFieldULong(hResult, i, 2);
   }
   return acl;
}

/**
 * Load graph's ACL
 */
static StructArray<GRAPH_ACL_ENTRY> *LoadGraphACL(DB_HANDLE hdb, uint32_t graphId)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT graph_id,user_id,user_rights FROM graph_acl WHERE graph_id=?"));
   if (hStmt == nullptr)
      return nullptr;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, graphId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   StructArray<GRAPH_ACL_ENTRY> *acl = ProcessGraphACLResultSet(hResult);
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return acl;
}

/*
 * Load all graph ACLs
 */
static StructArray<GRAPH_ACL_ENTRY> *LoadAllGraphACL(DB_HANDLE hdb)
{
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT graph_id,user_id,user_rights FROM graph_acl"));
   StructArray<GRAPH_ACL_ENTRY> *acl = ProcessGraphACLResultSet(hResult);
   DBFreeResult(hResult);
   return acl;
}

/**
 * Check access to the graph
 */
static bool CheckGraphAccess(StructArray<GRAPH_ACL_ENTRY> *acl, uint32_t graphId, uint32_t userId, uint32_t requiredAccess)
{
   for(int i = 0; i < acl->size(); i++)
   {
      GRAPH_ACL_ENTRY *e = acl->get(i);
      if (e->graphId == graphId)
      {
         if ((e->userId == userId) || ((e->userId & GROUP_FLAG) && CheckUserMembership(userId, e->userId)))
         {
            if ((e->access & requiredAccess) == requiredAccess)
               return true;
         }
      }
   }
   return false;
}

/**
 * Get graph owner
 */
static uint32_t GetGraphOwner(DB_HANDLE hdb, uint32_t graphId, uint32_t *ownerId)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT owner_id FROM graphs WHERE graph_id=?"));
   if (hStmt == nullptr)
      return RCC_DB_FAILURE;

   uint32_t rcc;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, graphId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         *ownerId = DBGetFieldULong(hResult, 0, 0);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_INVALID_GRAPH_ID;
      }
      DBFreeResult(hResult);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBFreeStatement(hStmt);
   return rcc;
}

/**
 * Check access to the graph
 */
uint32_t CheckGraphAccess(uint32_t graphId, uint32_t userId, uint32_t requiredAccess)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   uint32_t ownerId;
   uint32_t rcc = GetGraphOwner(hdb, graphId, &ownerId);
   if ((rcc == RCC_SUCCESS) && (userId != 0) && (userId != ownerId))
   {
      StructArray<GRAPH_ACL_ENTRY> *acl = LoadGraphACL(hdb, graphId);
      if (acl != nullptr)
      {
         if (!CheckGraphAccess(acl, graphId, userId, requiredAccess))
         {
            rcc = RCC_ACCESS_DENIED;
         }
         delete acl;
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Check if graph name already exist
 */
static std::pair<uint32_t, uint32_t> IsGraphNameExists(const TCHAR *graphName, UINT32 flags)
{
   DB_RESULT hResult;
   std::pair<uint32_t, uint32_t> result(0, 0);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Check existence and access rights
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT graph_id FROM graphs WHERE name=? AND flags=?")); // check unique only thought same type of graph
   if(hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, graphName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, flags);
      hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            result.first = DBGetFieldULong(hResult, 0, 0);
            result.second = RCC_OBJECT_ALREADY_EXISTS;
         }
         else
         {
            result.second = RCC_SUCCESS;
         }
         DBFreeResult(hResult);
      }
      else
      {
         result.second = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      result.second = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Fills message with available graph list
 */
void FillGraphListMsg(NXCPMessage *msg, uint32_t userId, bool templateGraphs, uint32_t graphId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	StructArray<GRAPH_ACL_ENTRY> *acl = LoadAllGraphACL(hdb);
	if (acl != nullptr)
	{
	   DB_RESULT hResult;
	   if (graphId != 0)
	   {
	      TCHAR query[256];
	      _sntprintf(query, 256, _T("SELECT graph_id,owner_id,flags,name,config,filters FROM graphs WHERE graph_id=%u"), graphId);
	      hResult = DBSelect(hdb, query);
	   }
	   else
	   {
	      hResult = DBSelect(hdb, _T("SELECT graph_id,owner_id,flags,name,config,filters FROM graphs"));
	   }
		if (hResult != nullptr)
		{
		   uint32_t *pdwUsers = MemAllocArrayNoInit<uint32_t>(acl->size());
			uint32_t *pdwRights = MemAllocArrayNoInit<uint32_t>(acl->size());
			int nRows = DBGetNumRows(hResult);
		   uint32_t dwId = VID_GRAPH_LIST_BASE;
         uint32_t numGraphs = 0;
			for(int i = 0; i < nRows; i++)
			{
			   uint32_t graphId = DBGetFieldULong(hResult, i, 0);
				uint32_t dwOwner = DBGetFieldULong(hResult, i, 1);
				uint32_t flags = DBGetFieldULong(hResult, i, 2);
				if (((userId == 0) || (userId == dwOwner) || CheckGraphAccess(acl, graphId, userId, NXGRAPH_ACCESS_READ)) && (((GRAPH_FLAG_TEMPLATE & flags) != 0) == templateGraphs))
				{
					msg->setField(dwId++, graphId);
					msg->setField(dwId++, dwOwner);
               msg->setField(dwId++, flags);

					TCHAR *value = DBGetField(hResult, i, 3, nullptr, 0);
               msg->setField(dwId++, CHECK_NULL_EX(value));
               MemFree(value);

					value = DBGetField(hResult, i, 4, nullptr, 0);
               msg->setField(dwId++, CHECK_NULL_EX(value));
               MemFree(value);

					value = DBGetField(hResult, i, 5, nullptr, 0);
               msg->setField(dwId++, CHECK_NULL_EX(value));
               MemFree(value);

					// ACL for graph
               uint32_t graphACLSize = 0;
					for(int j = 0; j < acl->size(); j++)
					{
					   GRAPH_ACL_ENTRY *e = acl->get(j);
						if (e->graphId == graphId)
						{
							pdwUsers[graphACLSize] = e->userId;
							pdwRights[graphACLSize] = e->access;
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
			MemFree(pdwUsers);
			MemFree(pdwRights);

			msg->setField(VID_NUM_GRAPHS, numGraphs);
			msg->setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg->setField(VID_RCC, RCC_DB_FAILURE);
		}
		delete acl;
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
uint32_t SaveGraph(const NXCPMessage& request, uint32_t userId, uint32_t *assignedId)
{
   uint32_t graphId = request.getFieldAsUInt32(VID_GRAPH_ID);
   TCHAR graphName[256];
	request.getFieldAsString(VID_NAME, graphName, 256);
	bool overwrite = request.getFieldAsBoolean(VID_OVERWRITE);

   std::pair<uint32_t, uint32_t> nameCheckResult = IsGraphNameExists(graphName, request.getFieldAsUInt32(VID_FLAGS));
   if (nameCheckResult.first == graphId)
   {
      nameCheckResult.second = RCC_SUCCESS;
   }

   bool isNew;
   uint32_t accessRightStatus;
	if (graphId == 0)
	{
		graphId = (nameCheckResult.first != 0) ? nameCheckResult.first : CreateUniqueId(IDG_GRAPH);
		isNew = true;
		accessRightStatus = RCC_SUCCESS;
	}
	else
	{
	   accessRightStatus = CheckGraphAccess(graphId, userId, NXGRAPH_ACCESS_WRITE);
		isNew = false;
	}

   uint32_t rcc;
   if ((accessRightStatus == RCC_SUCCESS) && ((nameCheckResult.second == RCC_SUCCESS) || overwrite))
   {
      rcc = RCC_SUCCESS;
      if (nameCheckResult.second != RCC_SUCCESS)
      {
         isNew = false;
         graphId = nameCheckResult.first;
      }
   }
   else
   {
      rcc = accessRightStatus ? accessRightStatus : nameCheckResult.second;
   }

	// Create/update graph
	if (rcc == RCC_SUCCESS)
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("SaveGraph: %s graph %u"), isNew ? _T("Creating") : _T("Updating"), graphId);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      uint32_t flags = request.getFieldAsUInt32(VID_FLAGS);
      TCHAR *name = request.getFieldAsString(VID_NAME);
      TCHAR *config = request.getFieldAsString(VID_GRAPH_CONFIG);
      TCHAR *filters = request.getFieldAsString(VID_FILTER);
		if (DBBegin(hdb))
		{
         DB_STATEMENT hStmt;

			if (isNew)
			{
            hStmt = DBPrepare(hdb, _T("INSERT INTO graphs (flags,name,config,filters,graph_id,owner_id) VALUES (?,?,?,?,?,?)"));
			}
			else
			{
				if (ExecuteQueryOnObject(hdb, graphId,_T("DELETE FROM graph_acl WHERE graph_id=?")))
				   hStmt = DBPrepare(hdb, _T("UPDATE graphs SET flags=?,name=?,config=?,filters=? WHERE graph_id=?"));
				else
				   hStmt = nullptr;
			}

			if (hStmt != nullptr)
			{
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, flags);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, config, DB_BIND_STATIC);
            DBBind(hStmt, 4, DB_SQLTYPE_TEXT, filters, DB_BIND_STATIC);
            DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, graphId);
            if (isNew)
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, userId);
            if (!DBExecute(hStmt))
               rcc = RCC_DB_FAILURE;
            DBFreeStatement(hStmt);
			}
			else
			{
            rcc = RCC_DB_FAILURE;
			}

		   if (rcc == RCC_SUCCESS)
			{
				// Insert new ACL
				int aclSize = request.getFieldAsInt32(VID_ACL_SIZE);
				if (aclSize > 0)
				{
				   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO graph_acl (graph_id,user_id,user_rights) VALUES (?,?,?)"), aclSize > 1);
				   if (hStmt != nullptr)
				   {
				      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, graphId);
                  uint32_t fieldId = VID_GRAPH_ACL_BASE;
                  for(int i = 0; i < aclSize; i++)
                  {
                     DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, request.getFieldAsUInt32(fieldId++)); // user ID
                     DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, request.getFieldAsUInt32(fieldId++)); // access rights
                     if (!DBExecute(hStmt))
                     {
                        rcc = RCC_DB_FAILURE;
                        break;
                     }
                  }
                  DBFreeStatement(hStmt);
				   }
				   else
				   {
		            rcc = RCC_DB_FAILURE;
				   }
				}
			}
			else
			{
            rcc = RCC_DB_FAILURE;
			}

         if (rcc == RCC_SUCCESS)
			{
				DBCommit(hdb);
				*assignedId = graphId;

            // send notificaion
				NXCPMessage update;
				uint32_t fieldId = VID_GRAPH_LIST_BASE;
            update.setCode(CMD_GRAPH_UPDATE);
            update.setField(fieldId++, graphId);
            update.setField(fieldId++, userId);
            update.setField(fieldId++, flags);
            update.setField(fieldId++, name);
            update.setField(fieldId++, config);
            update.setField(fieldId++, filters);

            StructArray<GRAPH_ACL_ENTRY> *acl = LoadGraphACL(hdb, graphId);
            if ((acl != nullptr) && (acl->size() > 0))
            {
               uint32_t *users = static_cast<uint32_t*>(MemAllocLocal(acl->size() * sizeof(uint32_t)));
               uint32_t *accessRights = static_cast<uint32_t*>(MemAllocLocal(acl->size() * sizeof(uint32_t)));

               for(int j = 0; j < acl->size(); j++)
               {
                  users[j] = acl->get(j)->userId;
                  accessRights[j] = acl->get(j)->access;
               }
               update.setField(fieldId++, acl->size());
               update.setFieldFromInt32Array(fieldId++, acl->size(), users);
               update.setFieldFromInt32Array(fieldId++, acl->size(), accessRights);

               MemFreeLocal(pdwUsers);
               MemFreeLocal(pdwRights);
            }
            else
            {
               update.setField(fieldId++, (INT32)0);
               update.setFieldFromInt32Array(fieldId++, 0, &graphId);
               update.setFieldFromInt32Array(fieldId++, 0, &graphId);
            }
            update.setField(VID_NUM_GRAPHS, 1);
            delete acl;

            NotifyClientsOnGraphUpdate(update, graphId);
			}
			else
			{
				DBRollback(hdb);
			}
			MemFree(name);
			MemFree(config);
			MemFree(filters);
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
	return rcc;
}

/**
 * Delete graph
 */
uint32_t DeleteGraph(uint32_t graphId, uint32_t userId)
{
   uint32_t rcc = CheckGraphAccess(graphId, userId, NXGRAPH_ACCESS_WRITE);
   if (rcc == RCC_SUCCESS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (ExecuteQueryOnObject(hdb, graphId, _T("DELETE FROM graphs WHERE graph_id=?")))
      {
         ExecuteQueryOnObject(hdb, graphId, _T("DELETE FROM graph_acl WHERE graph_id=?"));
         NotifyClientSessions(NX_NOTIFY_GRAPHS_DELETED, graphId);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("DeleteGraph: predefined graph %u deleted"), graphId);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   return rcc;
}
