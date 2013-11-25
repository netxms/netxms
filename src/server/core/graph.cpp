/*
** NetXMS - Network Management System
** NXCP API
** Copyright (C) 2003-2010 Victor Kirhenshtein
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

//
// Load graph's ACL - load for all graphs if dwGraphId is 0
//

GRAPH_ACL_ENTRY *LoadGraphACL(UINT32 dwGraphId, int *pnACLSize)
{
   int i, nSize;
   GRAPH_ACL_ENTRY *pACL = NULL;
   DB_RESULT hResult;

   if (dwGraphId == 0)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT graph_id,user_id,user_rights FROM graph_acl"));
   }
   else
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT graph_id,user_id,user_rights FROM graph_acl WHERE graph_id=%d"), dwGraphId);
      hResult = DBSelect(g_hCoreDB, szQuery);
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

//
// Check access to the graph
//

BOOL CheckGraphAccess(GRAPH_ACL_ENTRY *pACL, int nACLSize, UINT32 dwGraphId,
                             UINT32 dwUserId, UINT32 dwDesiredAccess)
{
   int i;

   for(i = 0; i < nACLSize; i++)
   {
      if (pACL[i].dwGraphId == dwGraphId)
      {
         if ((pACL[i].dwUserId == dwUserId) ||
             ((pACL[i].dwUserId & GROUP_FLAG) && CheckUserMembership(dwUserId, pACL[i].dwUserId)))
         {
            if ((pACL[i].dwAccess & dwDesiredAccess) == dwDesiredAccess)
               return TRUE;
         }
      }
   }
   return FALSE;
}

int getAccessCehckResult(UINT32 dwGraphId,UINT32 m_dwUserId)
{
   // Check existence and access rights
   TCHAR szQuery[16384];
   DB_RESULT hResult;
   UINT32 dwOwner;
   GRAPH_ACL_ENTRY *pACL = NULL;
   int nACLSize;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT owner_id FROM graphs WHERE graph_id=%d"), dwGraphId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         dwOwner = DBGetFieldULong(hResult, 0, 0);
         pACL = LoadGraphACL(dwGraphId, &nACLSize);
         if (nACLSize != -1)
         {
            if ((m_dwUserId == 0) ||
                (m_dwUserId == dwOwner) ||
                CheckGraphAccess(pACL, nACLSize, dwGraphId, m_dwUserId, NXGRAPH_ACCESS_WRITE))
            {
               return RCC_SUCCESS;
            }
            else
            {
               return RCC_ACCESS_DENIED;
            }
            safe_free(pACL);
         }
         else
         {
            return RCC_DB_FAILURE;
         }
      }
      else
      {
         return RCC_INVALID_GRAPH_ID;
      }
      DBFreeResult(hResult);
   }
   else
   {
      return RCC_DB_FAILURE;
   }
};

/*
 *
 */

GRAPH_ACL_AND_ID checkNameExistsAndGetID(TCHAR *dwGraphName)
{
   CSCPMessage msg;
   UINT32 dwGraphId;
   TCHAR szQuery[16384];
   GRAPH_ACL_ENTRY *pACL = NULL;
   int nACLSize;
   DB_RESULT hResult;
   GRAPH_ACL_AND_ID result;
   result.dwGraphId = 0;

   // Check existence and access rights
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT graph_id FROM graphs WHERE name='%s'"), dwGraphName);
   hResult = DBSelect(g_hCoreDB, szQuery);

   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         dwGraphId = DBGetFieldULong(hResult, 0, 0);
         pACL = LoadGraphACL(dwGraphId, &nACLSize);
         if (nACLSize != -1)
         {
            result.dwGraphId = dwGraphId;
            result.status = RCC_OBJECT_ALREADY_EXISTS;
         }
         else
         {
            result.status = RCC_DB_FAILURE;
         }
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
