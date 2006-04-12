/* 
** NetXMS - Network Management System
** Network Maps Library
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: submap.cpp
**
**/

#include "libnxmap.h"


//
// Default submap object constructor
//

nxSubmap::nxSubmap()
{
   CommonInit();
}


//
// Create default submap for given object
//

nxSubmap::nxSubmap(DWORD dwObjectId)
{
   CommonInit();
   m_dwId = dwObjectId;
}


//
// Create submap object from NXCP message
//

nxSubmap::nxSubmap(CSCPMessage *pMsg)
{
   CommonInit();
   ModifyFromMessage(pMsg);
}


//
// Common initialization code
//

void nxSubmap::CommonInit(void)
{
   m_dwId = 0;
   m_dwAttr = 0;
   m_dwNumObjects = 0;
   m_pObjectList = NULL;
   m_dwNumLinks = 0;
   m_pLinkList = NULL;
}


//
// Destructor
//

nxSubmap::~nxSubmap()
{
   safe_free(m_pObjectList);
   safe_free(m_pLinkList);
}


//
// Fill NXCP message with submap data
//

void nxSubmap::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, j, *pdwList;

   pMsg->SetVariable(VID_OBJECT_ID, m_dwId);
   pMsg->SetVariable(VID_SUBMAP_ATTR, m_dwAttr);
   pMsg->SetVariable(VID_NUM_OBJECTS, m_dwNumObjects);
   if (m_dwNumObjects > 0)
   {
      pdwList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumObjects * 3);
      for(i = 0, j = 0; i < m_dwNumObjects; i++)
      {
         pdwList[j++] = m_pObjectList[i].dwId;
         pdwList[j++] = m_pObjectList[i].x;
         pdwList[j++] = m_pObjectList[i].y;
      }
      pMsg->SetVariableToInt32Array(VID_OBJECT_LIST, m_dwNumObjects * 3, pdwList);
      free(pdwList);
   }
   
   pMsg->SetVariable(VID_NUM_LINKS, m_dwNumLinks);
   if (m_dwNumLinks > 0)
   {
      pdwList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumLinks * 2);
      for(i = 0, j = 0; i < m_dwNumLinks; i++)
      {
         pdwList[j++] = m_pLinkList[i].dwId1;
         pdwList[j++] = m_pLinkList[i].dwId2;
      }
      pMsg->SetVariableToInt32Array(VID_LINK_LIST, m_dwNumLinks * 2, pdwList);
      free(pdwList);
   }
}


//
// Modify submap object from NXCP message
//

void nxSubmap::ModifyFromMessage(CSCPMessage *pMsg)
{
   DWORD i, j, *pdwList;

   safe_free_and_null(m_pObjectList);
   safe_free_and_null(m_pLinkList);

   m_dwId = pMsg->GetVariableLong(VID_OBJECT_ID);
   m_dwAttr = pMsg->GetVariableLong(VID_SUBMAP_ATTR);

   m_dwNumObjects = pMsg->GetVariableLong(VID_NUM_OBJECTS);
   if (m_dwNumObjects > 0)
   {
      m_pObjectList = (MAP_OBJECT *)realloc(m_pObjectList, sizeof(MAP_OBJECT) * m_dwNumObjects);
      pdwList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumObjects * 3);
      pMsg->GetVariableInt32Array(VID_OBJECT_LIST, m_dwNumObjects * 3, pdwList);
      for(i = 0, j = 0; i < m_dwNumObjects; i++)
      {
         m_pObjectList[i].dwId = pdwList[j++];
         m_pObjectList[i].x = pdwList[j++];
         m_pObjectList[i].y = pdwList[j++];
      }
      free(pdwList);
   }
   else
   {
      safe_free_and_null(m_pObjectList);
   }

   m_dwNumLinks = pMsg->GetVariableLong(VID_NUM_LINKS);
   if (m_dwNumLinks > 0)
   {
      m_pLinkList = (OBJLINK *)realloc(m_pLinkList, sizeof(OBJLINK) * m_dwNumLinks);
      pdwList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumLinks * 2);
      pMsg->GetVariableInt32Array(VID_LINK_LIST, m_dwNumLinks * 2, pdwList);
      for(i = 0, j = 0; i < m_dwNumLinks; i++)
      {
         m_pLinkList[i].dwId1 = pdwList[j++];
         m_pLinkList[i].dwId2 = pdwList[j++];
      }
      free(pdwList);
   }
   else
   {
      safe_free_and_null(m_pLinkList);
   }
}


//
// Set object's state
//

void nxSubmap::SetObjectState(DWORD dwObjectId, DWORD dwState)
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectList[i].dwId == dwObjectId)
      {
         m_pObjectList[i].dwState = dwState;
         break;
      }
}


//
// Set object's state
//

DWORD nxSubmap::GetObjectState(DWORD dwObjectId)
{
   DWORD i, dwState = 0;

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectList[i].dwId == dwObjectId)
      {
         dwState = m_pObjectList[i].dwState;
         break;
      }
   return dwState;
}


//
// Get index of object with given ID
//

DWORD nxSubmap::GetObjectIndex(DWORD dwObjectId)
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectList[i].dwId == dwObjectId)
         return i;
   return INVALID_INDEX;
}


//
// Returns position of given object or (-1,-1) if object's position is undefined
//

POINT nxSubmap::GetObjectPosition(DWORD dwObjectId)
{
   DWORD i;
   POINT pt;

   pt.x = -1;
   pt.y = -1;
   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectList[i].dwId == dwObjectId)
      {
         pt.x = m_pObjectList[i].x;
         pt.y = m_pObjectList[i].y;
         break;
      }
   return pt;
}


//
// Returns position of given object or (-1,-1) if object's position is undefined
//

POINT nxSubmap::GetObjectPositionByIndex(DWORD dwIndex)
{
   POINT pt;

   if (dwIndex < m_dwNumObjects)
   {
      pt.x = m_pObjectList[dwIndex].x;
      pt.y = m_pObjectList[dwIndex].y;
   }
   else
   {
      pt.x = -1;
      pt.y = -1;
   }
   return pt;
}


//
// Set object's position
//

void nxSubmap::SetObjectPosition(DWORD dwObjectId, int x, int y)
{
   DWORD i;

   for(i = 0; i < m_dwNumObjects; i++)
      if (m_pObjectList[i].dwId == dwObjectId)
      {
         m_pObjectList[i].x = x;
         m_pObjectList[i].y = y;
         break;
      }
   if (i == m_dwNumObjects)
   {
      // New element
      m_dwNumObjects++;
      m_pObjectList = (MAP_OBJECT *)realloc(m_pObjectList, m_dwNumObjects * sizeof(MAP_OBJECT));
      m_pObjectList[i].dwId = dwObjectId;
      m_pObjectList[i].x = x;
      m_pObjectList[i].y = y;
      m_pObjectList[i].dwState = 0;
   }
}


//
// Set object's position using index
//

void nxSubmap::SetObjectPositionByIndex(DWORD dwIndex, int x, int y)
{
   m_pObjectList[dwIndex].x = x;
   m_pObjectList[dwIndex].y = y;
}


//
// Get minimal required size
//

POINT nxSubmap::GetMinSize(void)
{
   POINT pt;
   DWORD i;

   pt.x = 0;
   pt.y = 0;

   for(i = 0; i < m_dwNumObjects; i++)
   {
      if (m_pObjectList[i].x > pt.x)
         pt.x = m_pObjectList[i].x;
      if (m_pObjectList[i].y > pt.y)
         pt.y = m_pObjectList[i].y;
   }

   pt.x += MAP_OBJECT_SIZE_X + MAP_RIGHT_MARGIN;
   pt.y += MAP_OBJECT_SIZE_Y + MAP_TEXT_BOX_HEIGHT + MAP_BOTTOM_MARGIN;

   return pt;
}


//
// Layout objects on map
//

void nxSubmap::DoLayout(DWORD dwNumObjects, DWORD *pdwObjectList,
                        DWORD dwNumLinks, OBJLINK *pLinkList,
                        int nIdealX, int nIdealY)
{
   DWORD i;
   int x, y;

   safe_free(m_pLinkList);
   m_dwNumLinks = dwNumLinks;
   m_pLinkList = (pLinkList != NULL) ? (OBJLINK *)nx_memdup(pLinkList, sizeof(OBJLINK) * dwNumLinks) : NULL;

   safe_free_and_null(m_pObjectList);
   m_dwNumObjects = 0;

   for(i = 0, x = MAP_LEFT_MARGIN, y = MAP_TOP_MARGIN; i < dwNumObjects; i++)
   {
      SetObjectPosition(pdwObjectList[i], x, y);
      x += MAP_OBJECT_SIZE_X + MAP_OBJECT_INTERVAL_X;
      if (x >= nIdealX - MAP_OBJECT_SIZE_X - MAP_OBJECT_INTERVAL_X / 2)
      {
         x = MAP_LEFT_MARGIN;
         y += MAP_OBJECT_SIZE_Y + MAP_TEXT_BOX_HEIGHT + MAP_OBJECT_INTERVAL_Y;
      }
   }
   
   m_dwAttr |= SUBMAP_ATTR_LAYOUT_COMPLETED;
}
