/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: template.cpp
**
**/

#include "nms_core.h"


//
// Template object constructor
//

Template::Template()
{
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
}


//
// Destructor
//

Template::~Template()
{
   DestroyItems();
}


//
// Destroy all related data collection items
//

void Template::DestroyItems(void)
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      delete m_ppItems[i];
   safe_free(m_ppItems);
   m_dwNumItems = 0;
   m_ppItems = NULL;
}


//
// Create object from database data
//

BOOL Template::CreateFromDB(DWORD dwId)
{
   return FALSE;
}


//
// Save object to database
//

BOOL Template::SaveToDB(void)
{
   return FALSE;
}


//
// Delete object from database
//

BOOL Template::DeleteFromDB(void)
{
   char szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      if (Type() == OBJECT_TEMPLATE)
      {
         sprintf(szQuery, "DELETE FROM dct WHERE template_id=%ld", m_dwId);
         QueueSQLRequest(szQuery);
      }
      sprintf(szQuery, "DELETE FROM items WHERE node_id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "UPDATE items SET template_id=0 WHERE template_id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Load data collection items from database
//

void Template::LoadItemsFromDB(void)
{
   char szQuery[256];
   DB_RESULT hResult;

   sprintf(szQuery, "SELECT item_id,name,source,datatype,polling_interval,retention_time,"
                    "status,delta_calculation,transformation,template_id,description "
                    "FROM items WHERE node_id=%d", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);

   if (hResult != 0)
   {
      int i, iRows;

      iRows = DBGetNumRows(hResult);
      if (iRows > 0)
      {
         m_dwNumItems = iRows;
         m_ppItems = (DCItem **)malloc(sizeof(DCItem *) * iRows);
         for(i = 0; i < iRows; i++)
            m_ppItems[i] = new DCItem(hResult, i, this);
      }
      DBFreeResult(hResult);
   }
}


//
// Add item to node
//

BOOL Template::AddItem(DCItem *pItem)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == pItem->Id())
         break;   // Item with specified id already exist
   
   if (i == m_dwNumItems)     // Add new item
   {
      m_dwNumItems++;
      m_ppItems = (DCItem **)realloc(m_ppItems, sizeof(DCItem *) * m_dwNumItems);
      m_ppItems[i] = pItem;
      m_ppItems[i]->SetLastPollTime(0);    // Cause item to be polled immediatelly
      m_ppItems[i]->SetStatus(ITEM_STATUS_ACTIVE);
      m_ppItems[i]->SetBusyFlag(FALSE);
      Modify();
      bResult = TRUE;
   }

   Unlock();
   return bResult;
}


//
// Delete item from node
//

BOOL Template::DeleteItem(DWORD dwItemId)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         // Destroy item
         m_ppItems[i]->DeleteFromDB();
         delete m_ppItems[i];
         m_dwNumItems--;
         memmove(&m_ppItems[i], &m_ppItems[i + 1], sizeof(DCItem *) * (m_dwNumItems - i));
         bResult = TRUE;
         break;
      }

   Unlock();
   return bResult;
}


//
// Modify data collection item from CSCP message
//

BOOL Template::UpdateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                          DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i;
   BOOL bResult = FALSE;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         m_ppItems[i]->UpdateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
         bResult = TRUE;
         m_bIsModified = TRUE;
         break;
      }

   Unlock();
   return bResult;
}


//
// Lock data collection items list
//

BOOL Template::LockDCIList(DWORD dwSessionId)
{
   BOOL bSuccess = FALSE;

   Lock();
   if (m_dwDCILockStatus == INVALID_INDEX)
   {
      m_dwDCILockStatus = dwSessionId;
      bSuccess = TRUE;
   }
   Unlock();
   return bSuccess;
}


//
// Unlock data collection items list
//

BOOL Template::UnlockDCIList(DWORD dwSessionId)
{
   BOOL bSuccess = FALSE;

   Lock();
   if (m_dwDCILockStatus == dwSessionId)
   {
      m_dwDCILockStatus = INVALID_INDEX;
      bSuccess = TRUE;
   }
   Unlock();
   return bSuccess;
}


//
// Send DCI list to client
//

void Template::SendItemsToClient(ClientSession *pSession, DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_NODE_DCI);

   // Walk through items list
   for(i = 0; i < m_dwNumItems; i++)
   {
      m_ppItems[i]->CreateMessage(&msg);
      pSession->SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-list indicator
   msg.SetCode(CMD_NODE_DCI_LIST_END);
   pSession->SendMessage(&msg);
}


//
// Get DCI item's type
//

int Template::GetItemType(DWORD dwItemId)
{
   DWORD i;
   int iType = -1;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         iType = m_ppItems[i]->DataType();
         break;
      }

   Unlock();
   return iType;
}


//
// Get item by it's id
//

const DCItem *Template::GetItemById(DWORD dwItemId)
{
   DWORD i;
   DCItem *pItem = NULL;

   Lock();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         pItem = m_ppItems[i];
         break;
      }

   Unlock();
   return pItem;
}
