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

   // Check existence and access rights
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT graph_id FROM graphs WHERE name=%s"), 
              (const TCHAR *)DBPrepareString(g_hCoreDB, graphName));
   hResult = DBSelect(g_hCoreDB, szQuery);

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
   return result;
}
