/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: netobj.cpp
**
**/

#include "nms_core.h"


//
// NetObj class constructor
//

NetObj::NetObj()
{
   m_hMutex = MutexCreate();
   m_iStatus = STATUS_UNKNOWN;
   m_szName[0] = 0;
   m_bIsModified = FALSE;
   m_bIsDeleted = FALSE;
   m_dwIpAddr = 0;
   m_dwChildCount = 0;
   m_pChildList = NULL;
   m_dwParentCount = 0;
   m_pParentList = NULL;
}


//
// NetObj class destructor
//

NetObj::~NetObj()
{
   MutexDestroy(m_hMutex);
   if (m_pChildList != NULL)
      free(m_pChildList);
   if (m_pParentList != NULL)
      free(m_pParentList);
}


//
// Create object from database data
//

BOOL NetObj::CreateFromDB(DWORD dwId)
{
   return FALSE;     // Abstract objects cannot be loaded from database
}


//
// Save object to database
//

BOOL NetObj::SaveToDB(void)
{
   return FALSE;     // Abstract objects cannot be saved to database
}


//
// Delete object from database
//

BOOL NetObj::DeleteFromDB(void)
{
   return FALSE;     // Abstract objects cannot be deleted from database
}


//
// Add reference to the new child object
//

void NetObj::AddChild(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
         return;     // Already in the child list
   m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * (m_dwChildCount + 1));
   m_pChildList[m_dwChildCount++] = pObject;
   m_bIsModified = TRUE;
   Unlock();
}


//
// Add reference to parent object
//

void NetObj::AddParent(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
         return;     // Already in the parents list
   m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * (m_dwParentCount + 1));
   m_pParentList[m_dwParentCount++] = pObject;
   m_bIsModified = TRUE;
   Unlock();
}


//
// Delete reference to child object
//

void NetObj::DeleteChild(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i] == pObject)
         break;
   if (i == m_dwChildCount)   // No such object
      return;
   m_dwChildCount--;
   if (m_dwChildCount > 0)
   {
      memmove(&m_pChildList[i], &m_pChildList[i + 1], sizeof(NetObj *) * (m_dwChildCount - i));
      m_pChildList = (NetObj **)realloc(m_pChildList, sizeof(NetObj *) * m_dwChildCount);
   }
   else
   {
      free(m_pChildList);
      m_pChildList = NULL;
   }
   m_bIsModified = TRUE;
   Unlock();
}


//
// Delete reference to parent object
//

void NetObj::DeleteParent(NetObj *pObject)
{
   DWORD i;

   Lock();
   for(i = 0; i < m_dwParentCount; i++)
      if (m_pParentList[i] == pObject)
         break;
   if (i == m_dwParentCount)   // No such object
      return;
   m_dwParentCount--;
   if (m_dwParentCount > 0)
   {
      memmove(&m_pParentList[i], &m_pParentList[i + 1], sizeof(NetObj *) * (m_dwParentCount - i));
      m_pParentList = (NetObj **)realloc(m_pParentList, sizeof(NetObj *) * m_dwParentCount);
   }
   else
   {
      free(m_pParentList);
      m_pParentList = NULL;
   }
   m_bIsModified = TRUE;
   Unlock();
}


//
// Prepare object for deletion - remove all references, etc.
//

void NetObj::Delete(void)
{
   DWORD i;

   Lock();

   // Remove references to this object from parent objects
   for(i = 0; i < m_dwParentCount; i++)
      m_pParentList[i]->DeleteChild(this);
   free(m_pParentList);
   m_dwParentCount = 0;

   // Delete references to this object from child objects
   for(i = 0; i < m_dwChildCount; i++)
   {
      m_pChildList[i]->DeleteParent(this);
      if (m_pChildList[i]->IsOrphaned())
         m_pChildList[i]->Delete();
   }
   free(m_pChildList);
   m_dwChildCount = 0;

   m_bIsDeleted = TRUE;
   m_bIsModified = TRUE;
   Unlock();
}


//
// Print childs IDs
//

const char *NetObj::ChildList(char *szBuffer)
{
   DWORD i;
   char *pBuf = szBuffer;

   *pBuf = 0;
   for(i = 0, pBuf = szBuffer; i < m_dwChildCount; i++)
   {
      sprintf(pBuf, "%d ", m_pChildList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}


//
// Print parents IDs
//

const char *NetObj::ParentList(char *szBuffer)
{
   DWORD i;
   char *pBuf = szBuffer;

   *pBuf = 0;
   for(i = 0; i < m_dwParentCount; i++)
   {
      sprintf(pBuf, "%d ", m_pParentList[i]->Id());
      while(*pBuf)
         pBuf++;
   }
   if (pBuf != szBuffer)
      *(pBuf - 1) = 0;
   return szBuffer;
}


//
// Calculate status for compound object based on childs' status
//

void NetObj::CalculateCompoundStatus(void)
{
   DWORD i;
   int iSum, iCount, iOldStatus = m_iStatus;

   /* TODO: probably status calculation algorithm should be changed */
   for(i = 0, iSum = 0, iCount = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Status() < STATUS_UNKNOWN)
      {
         iSum += m_pChildList[i]->Status();
         iCount++;
      }
   if (iCount > 0)
      m_iStatus = iSum / iCount;
   else
      m_iStatus = STATUS_UNKNOWN;

   // Cause parent object(s) to recalculate it's status
   if (iOldStatus != m_iStatus)
   {
      for(i = 0; i < m_dwParentCount; i++)
         m_pParentList[i]->CalculateCompoundStatus();
      m_bIsModified = TRUE;
   }
}
