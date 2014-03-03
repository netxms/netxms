/* 
** NetXMS - Network Management System
** Network Maps Library
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
** File: objlist.cpp
**
**/

#include "libnxmap.h"


//
// Constructors
//

nxmap_ObjList::nxmap_ObjList()
{
   m_dwNumObjects = 0;
	m_allocatedObjects = 0;
   m_pdwObjectList = NULL;
   m_dwNumLinks = 0;
	m_allocatedLinks = 0;
   m_pLinkList = NULL;
}

nxmap_ObjList::nxmap_ObjList(CSCPMessage *pMsg)
{
	UINT32 i, dwId;

   m_dwNumObjects = pMsg->GetVariableLong(VID_NUM_OBJECTS);
	m_allocatedObjects = m_dwNumObjects;
   m_pdwObjectList = (UINT32 *)malloc(m_dwNumObjects * sizeof(UINT32));
	pMsg->GetVariableInt32Array(VID_OBJECT_LIST, m_dwNumObjects, m_pdwObjectList);
   m_dwNumLinks = pMsg->GetVariableLong(VID_NUM_LINKS);
	m_allocatedLinks = m_dwNumLinks;
   m_pLinkList = (OBJLINK *)malloc(m_dwNumLinks * sizeof(OBJLINK));
	for(i = 0, dwId = VID_OBJECT_LINKS_BASE; i < m_dwNumLinks; i++, dwId += 5)
	{
		m_pLinkList[i].dwId1 = pMsg->GetVariableLong(dwId++);
		m_pLinkList[i].dwId2 = pMsg->GetVariableLong(dwId++);
		m_pLinkList[i].nType = (int)pMsg->GetVariableShort(dwId++);
		pMsg->GetVariableStr(dwId++, m_pLinkList[i].szPort1, MAX_CONNECTOR_NAME);
		pMsg->GetVariableStr(dwId++, m_pLinkList[i].szPort2, MAX_CONNECTOR_NAME);
	}
}

nxmap_ObjList::nxmap_ObjList(nxmap_ObjList *pSrc)
{
   m_dwNumObjects = pSrc->m_dwNumObjects;
	m_allocatedObjects = m_dwNumObjects;
   m_pdwObjectList = (UINT32 *)nx_memdup(pSrc->m_pdwObjectList, sizeof(UINT32) * m_dwNumObjects);
   m_dwNumLinks = pSrc->m_dwNumLinks;
	m_allocatedLinks = m_dwNumLinks;
   m_pLinkList = (OBJLINK *)nx_memdup(pSrc->m_pLinkList, sizeof(OBJLINK) * m_dwNumLinks);
}

/**
 * Destructor
 */
nxmap_ObjList::~nxmap_ObjList()
{
   safe_free(m_pdwObjectList);
   safe_free(m_pLinkList);
}

/**
 * Clear list
 */
void nxmap_ObjList::clear()
{
   safe_free_and_null(m_pdwObjectList);
   safe_free_and_null(m_pLinkList);
   m_dwNumObjects = 0;
	m_allocatedObjects = 0;
   m_dwNumLinks = 0;
	m_allocatedLinks = 0;
}

/**
 * Add object to list
 */
void nxmap_ObjList::addObject(UINT32 id)
{
   UINT32 i;

   for(i = 0; i < m_dwNumObjects; i++)
   {
      if (m_pdwObjectList[i] == id)
         break;
   }

   if (i == m_dwNumObjects)
   {
		if (m_dwNumObjects == m_allocatedObjects)
		{
			m_allocatedObjects += 64;
	      m_pdwObjectList = (UINT32 *)realloc(m_pdwObjectList, sizeof(UINT32) * m_allocatedObjects);
		}
      m_pdwObjectList[m_dwNumObjects++] = id;
   }
}

/**
 * Remove object from list
 */
void nxmap_ObjList::removeObject(UINT32 id)
{
   UINT32 i;

   for(i = 0; i < m_dwNumObjects; i++)
   {
      if (m_pdwObjectList[i] == id)
      {
         m_dwNumObjects--;
         memmove(&m_pdwObjectList[i], &m_pdwObjectList[i + 1], (m_dwNumObjects - i) * sizeof(UINT32));
         break;
      }
   }

   for(i = 0; i < m_dwNumLinks; i++)
   {
      if ((m_pLinkList[i].dwId1 == id) || (m_pLinkList[i].dwId2 == id))
      {
         m_dwNumLinks--;
         memmove(&m_pLinkList[i], &m_pLinkList[i + 1], (m_dwNumLinks - i) * sizeof(UINT32));
         i--;
      }
   }
}

/**
 * Link two objects
 */
void nxmap_ObjList::linkObjects(UINT32 id1, UINT32 id2)
{
   UINT32 i;
   int nCount;

   // Validate object IDs
   for(i = 0, nCount = 0; (i < m_dwNumObjects) && (nCount < 2); i++)
   {
      if ((m_pdwObjectList[i] == id1) ||
          (m_pdwObjectList[i] == id2))
         nCount++;
   }

   if (nCount == 2)  // Both objects exist?
   {
      // Check for duplicate links
      for(i = 0; i < m_dwNumLinks; i++)
      {
         if (((m_pLinkList[i].dwId1 == id1) && (m_pLinkList[i].dwId2 == id2)) ||
             ((m_pLinkList[i].dwId2 == id1) && (m_pLinkList[i].dwId1 == id2)))
            break;
      }
      if (i == m_dwNumLinks)
      {
			if (m_dwNumLinks == m_allocatedLinks)
			{
				m_allocatedLinks += 64;
	         m_pLinkList = (OBJLINK *)realloc(m_pLinkList, sizeof(OBJLINK) * m_allocatedLinks);
			}
         m_pLinkList[m_dwNumLinks].dwId1 = id1;
         m_pLinkList[m_dwNumLinks].dwId2 = id2;
         m_pLinkList[m_dwNumLinks].nType = LINK_TYPE_NORMAL;
			m_pLinkList[m_dwNumLinks].szPort1[0] = 0;
			m_pLinkList[m_dwNumLinks].szPort2[0] = 0;
         m_dwNumLinks++;
      }
   }
}

/**
 * Update port names on connector
 */
static void UpdatePortNames(OBJLINK *link, const TCHAR *port1, const TCHAR *port2)
{
	_tcscat_s(link->szPort1, MAX_CONNECTOR_NAME, _T(", "));
	_tcscat_s(link->szPort1, MAX_CONNECTOR_NAME, port1);
	_tcscat_s(link->szPort2, MAX_CONNECTOR_NAME, _T(", "));
	_tcscat_s(link->szPort2, MAX_CONNECTOR_NAME, port2);
}

/**
 * Link two objects with named links
 */
void nxmap_ObjList::linkObjectsEx(UINT32 id1, UINT32 id2, const TCHAR *port1, const TCHAR *port2, UINT32 portId1, UINT32 portId2)
{
   UINT32 i;
   int nCount;

   // Validate object IDs
   for(i = 0, nCount = 0; (i < m_dwNumObjects) && (nCount < 2); i++)
   {
      if ((m_pdwObjectList[i] == id1) ||
          (m_pdwObjectList[i] == id2))
         nCount++;
   }

   if (nCount == 2)  // Both objects exist?
   {
      // Check for duplicate links
      for(i = 0; i < m_dwNumLinks; i++)
      {
			if ((m_pLinkList[i].dwId1 == id1) && (m_pLinkList[i].dwId2 == id2))
			{
				int j;
				for(j = 0; j < m_pLinkList[i].portIdCount; j++)
				{
					// assume point-to-point interfaces, therefore "or" is enough
					if ((m_pLinkList[i].portId1[j] == portId1) || (m_pLinkList[i].portId2[j] == portId2))
						break;
				}
				if ((j == m_pLinkList[i].portIdCount) && (j < MAX_PORT_COUNT))
				{
					m_pLinkList[i].portId1[j] = portId1;
					m_pLinkList[i].portId2[j] = portId2;
					m_pLinkList[i].portIdCount++;
					UpdatePortNames(&m_pLinkList[i], port1, port2);
					m_pLinkList[i].nType = LINK_TYPE_MULTILINK;
				}
				break;
			}
			if ((m_pLinkList[i].dwId1 == id2) && (m_pLinkList[i].dwId2 == id1))
			{
				int j;
				for(j = 0; j < m_pLinkList[i].portIdCount; j++)
				{
					// assume point-to-point interfaces, therefore or is enough
					if ((m_pLinkList[i].portId1[j] == portId2) || (m_pLinkList[i].portId2[j] == portId1))
						break;
				}
				if ((j == m_pLinkList[i].portIdCount) && (j < MAX_PORT_COUNT))
				{
					m_pLinkList[i].portId1[j] = portId2;
					m_pLinkList[i].portId2[j] = portId1;
					m_pLinkList[i].portIdCount++;
					UpdatePortNames(&m_pLinkList[i], port2, port1);
					m_pLinkList[i].nType = LINK_TYPE_MULTILINK;
				}
				break;
			}
      }
      if (i == m_dwNumLinks)
      {
			if (m_dwNumLinks == m_allocatedLinks)
			{
				m_allocatedLinks += 64;
	         m_pLinkList = (OBJLINK *)realloc(m_pLinkList, sizeof(OBJLINK) * m_allocatedLinks);
			}
         m_pLinkList[m_dwNumLinks].dwId1 = id1;
         m_pLinkList[m_dwNumLinks].dwId2 = id2;
         m_pLinkList[m_dwNumLinks].nType = LINK_TYPE_NORMAL;
			m_pLinkList[m_dwNumLinks].portIdCount = 1;
			m_pLinkList[m_dwNumLinks].portId1[0] = portId1;
			m_pLinkList[m_dwNumLinks].portId2[0] = portId2;
			nx_strncpy(m_pLinkList[m_dwNumLinks].szPort1, port1, MAX_CONNECTOR_NAME);
			nx_strncpy(m_pLinkList[m_dwNumLinks].szPort2, port2, MAX_CONNECTOR_NAME);
         m_dwNumLinks++;
      }
   }
}

/**
 * Create NXCP message
 */
void nxmap_ObjList::createMessage(CSCPMessage *pMsg)
{
	UINT32 i, dwId;

	// Object list
	pMsg->SetVariable(VID_NUM_OBJECTS, m_dwNumObjects);
	if (m_dwNumObjects > 0)
		pMsg->SetVariableToInt32Array(VID_OBJECT_LIST, m_dwNumObjects, m_pdwObjectList);

	// Links between objects
	pMsg->SetVariable(VID_NUM_LINKS, m_dwNumLinks);
	for(i = 0, dwId = VID_OBJECT_LINKS_BASE; i < m_dwNumLinks; i++, dwId += 5)
	{
		pMsg->SetVariable(dwId++, m_pLinkList[i].dwId1);
		pMsg->SetVariable(dwId++, m_pLinkList[i].dwId2);
		pMsg->SetVariable(dwId++, (WORD)m_pLinkList[i].nType);
		pMsg->SetVariable(dwId++, m_pLinkList[i].szPort1);
		pMsg->SetVariable(dwId++, m_pLinkList[i].szPort2);
	}
}

/**
 * Check if link between two given objects exist
 */
bool nxmap_ObjList::isLinkExist(UINT32 objectId1, UINT32 objectId2)
{
   for(UINT32 i = 0; i < m_dwNumLinks; i++)
   {
		if ((m_pLinkList[i].dwId1 == objectId1) && (m_pLinkList[i].dwId2 == objectId2))
			return true;
	}
	return false;
}

/**
 * Check if given object exist
 */
bool nxmap_ObjList::isObjectExist(UINT32 objectId)
{
	for(UINT32 i = 0; i < m_dwNumObjects; i++)
	{
		if (m_pdwObjectList[i] == objectId)
			return true;
	}
	return false;
}
