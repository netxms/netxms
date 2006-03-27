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
   m_dwPosListSize = 0;
   m_pPosList = NULL;
}


//
// Destructor
//

nxSubmap::~nxSubmap()
{
   safe_free(m_pPosList);
}


//
// Fill NXCP message with submap data
//

void nxSubmap::CreateMessage(CSCPMessage *pMsg)
{
}


//
// Modify submap object from NXCP message
//

void nxSubmap::ModifyFromMessage(CSCPMessage *pMsg)
{
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
   for(i = 0; i < m_dwPosListSize; i++)
      if (m_pPosList[i].dwId == dwObjectId)
      {
         pt.x = m_pPosList[i].x;
         pt.y = m_pPosList[i].y;
         break;
      }
   return pt;
}


//
// Set object's position
//

void nxSubmap::SetObjectPosition(DWORD dwObjectId, int x, int y)
{
   DWORD i;

   for(i = 0; i < m_dwPosListSize; i++)
      if (m_pPosList[i].dwId == dwObjectId)
      {
         m_pPosList[i].x = x;
         m_pPosList[i].y = y;
         break;
      }
   if (i == m_dwPosListSize)
   {
      // New element
      m_dwPosListSize++;
      m_pPosList = (OBJPOS *)realloc(m_pPosList, m_dwPosListSize * sizeof(OBJPOS));
      m_pPosList[i].dwId = dwObjectId;
      m_pPosList[i].x = x;
      m_pPosList[i].y = y;
   }
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

   safe_free_and_null(m_pPosList);
   m_dwPosListSize = 0;

   for(i = 0, x = MAP_LEFT_MARGIN, y = MAP_TOP_MARGIN; i < dwNumObjects; i++)
   {
      SetObjectPosition(pdwObjectList[i], x, y);
      x += MAP_OBJECT_SIZE_X + MAP_OBJECT_INTERVAL;
      if (x >= nIdealX - MAP_OBJECT_SIZE_X - MAP_OBJECT_INTERVAL / 2)
      {
         x = MAP_LEFT_MARGIN;
         y += MAP_OBJECT_SIZE_X + MAP_TEXT_BOX_HEIGHT + MAP_OBJECT_INTERVAL;
      }
   }
   
   m_dwAttr |= SUBMAP_ATTR_LAYOUT_COMPLETED;
}
