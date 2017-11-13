/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: netmap_objlist.cpp
**
**/

#include "nxcore.h"

/**
 * Create empty object link
 */
ObjLink::ObjLink()
{
   id1 = 0;
   id2 = 0;
   type = LINK_TYPE_NORMAL;
   port1[0] = 0;
   port2[0] = 0;
	portIdCount = 0;
	flags = 0;
}

/**
 * Object link copy constructor
 */
ObjLink::ObjLink(const ObjLink *src)
{
   id1 = src->id1;
   id2 = src->id2;
   type = src->type;
   _tcscpy(port1, src->port1);
	_tcscpy(port2, src->port2);
	portIdCount = src->portIdCount;

	for(int i = 0; i < portIdCount; i++)
	{
      this->portIdArray1[i] = src->portIdArray1[i];
      this->portIdArray2[i] = src->portIdArray2[i];
	}

	flags = src->flags;
}

/**
 * Create empty object list
 */
NetworkMapObjectList::NetworkMapObjectList()
{
   m_objectList = new IntegerArray<UINT32>(16, 16);
   m_linkList = new ObjectArray<ObjLink>(16, 16, true);
}

/**
 * Copy constructor
 */
NetworkMapObjectList::NetworkMapObjectList(NetworkMapObjectList *src)
{
   int i;

   m_objectList = new IntegerArray<UINT32>(src->m_objectList->size(), 16);
   for(i = 0; i < src->m_objectList->size(); i++)
      m_objectList->add(src->m_objectList->get(i));

   m_linkList = new ObjectArray<ObjLink>(src->m_linkList->size(), 16, true);
	for(i = 0; i < src->m_linkList->size(); i++)
      m_linkList->add(new ObjLink(src->m_linkList->get(i)));
}

/**
 * Destructor
 */
NetworkMapObjectList::~NetworkMapObjectList()
{
   delete m_objectList;
   delete m_linkList;
}

/**
 * Merge two lists
 */
void NetworkMapObjectList::merge(const NetworkMapObjectList *src)
{
   int i;

   for(i = 0; i < src->m_objectList->size(); i++)
   {
      if (!isObjectExist(src->m_objectList->get(i)))
         m_objectList->add(src->m_objectList->get(i));
   }
   for(i = 0; i < src->m_linkList->size(); i++)
   {
      ObjLink *l = src->m_linkList->get(i);
      if (!isLinkExist(l->id1, l->id2))
         m_linkList->add(new ObjLink(l));
   }
}

/**
 * Clear list
 */
void NetworkMapObjectList::clear()
{
   m_linkList->clear();
   m_objectList->clear();
}

/**
 * Compare object IDs
 */
static int CompareObjectId(const void *e1, const void *e2)
{
   UINT32 id1 = *((UINT32 *)e1);
   UINT32 id2 = *((UINT32 *)e2);
   return (id1 < id2) ? -1 : ((id1 > id2) ? 1 : 0);
}

/**
 * Add object to list
 */
void NetworkMapObjectList::addObject(UINT32 id)
{
   if (!isObjectExist(id))
   {
      m_objectList->add(id);
      m_objectList->sort(CompareObjectId);
   }
}

/**
 * Remove object from list
 */
void NetworkMapObjectList::removeObject(UINT32 id)
{
   m_objectList->remove(id);

   for(int i = 0; i < m_linkList->size(); i++)
   {
      if ((m_linkList->get(i)->id1 == id) || (m_linkList->get(i)->id2 == id))
      {
         m_linkList->remove(i);
         i--;
      }
   }
}

/**
 * Link two objects
 */
void NetworkMapObjectList::linkObjects(UINT32 id1, UINT32 id2, int linkType, const TCHAR *linkName)
{
   bool linkExists = false;
   if ((m_objectList->indexOf(id1) != -1) && (m_objectList->indexOf(id2) != -1))  // if both objects exist
   {
      // Check for duplicate links
      for(int i = 0; i < m_linkList->size(); i++)
      {
         if (((m_linkList->get(i)->id1 == id1) && (m_linkList->get(i)->id2 == id2)) ||
             ((m_linkList->get(i)->id2 == id1) && (m_linkList->get(i)->id1 == id2)))
         {
            linkExists = true;
            break;
         }
      }
      if (!linkExists)
      {
         ObjLink *link = new ObjLink();
         link->id1 = id1;
         link->id2 = id2;
         link->type = linkType;
			m_linkList->add(link);
      }
   }
}

/**
 * Update port names on connector
 */
static void UpdatePortNames(ObjLink *link, const TCHAR *port1, const TCHAR *port2)
{
	_tcslcat(link->port1, _T(", "), MAX_CONNECTOR_NAME);
	_tcslcat(link->port1, port1, MAX_CONNECTOR_NAME);
	_tcslcat(link->port2, _T(", "), MAX_CONNECTOR_NAME);
	_tcslcat(link->port2, port2, MAX_CONNECTOR_NAME);
}

/**
 * Link two objects with named links
 */
void NetworkMapObjectList::linkObjectsEx(UINT32 id1, UINT32 id2, const TCHAR *port1, const TCHAR *port2, UINT32 portId1, UINT32 portId2)
{
   bool linkExists = false;
   if ((m_objectList->indexOf(id1) != -1) && (m_objectList->indexOf(id2) != -1))  // if both objects exist
   {
      // Check for duplicate links
      for(int i = 0; i < m_linkList->size(); i++)
      {
			if ((m_linkList->get(i)->id1 == id1) && (m_linkList->get(i)->id2 == id2))
			{
				int j;
				for(j = 0; j < m_linkList->get(i)->portIdCount; j++)
				{
					// assume point-to-point interfaces, therefore "or" is enough
					if ((m_linkList->get(i)->portIdArray1[j] == portId1) || (m_linkList->get(i)->portIdArray2[j] == portId2))
               {
                  linkExists = true;
                  break;
               }
				}
				if (!linkExists && (m_linkList->get(i)->portIdCount < MAX_PORT_COUNT))
				{
					m_linkList->get(i)->portIdArray1[j] = portId1;
					m_linkList->get(i)->portIdArray2[j] = portId2;
					m_linkList->get(i)->portIdCount++;
					UpdatePortNames(m_linkList->get(i), port1, port2);
					m_linkList->get(i)->type = LINK_TYPE_MULTILINK;
               linkExists = true;
				}
				break;
			}
			if ((m_linkList->get(i)->id1 == id2) && (m_linkList->get(i)->id2 == id1))
			{
				int j;
				for(j = 0; j < m_linkList->get(i)->portIdCount; j++)
				{
					// assume point-to-point interfaces, therefore or is enough
					if ((m_linkList->get(i)->portIdArray1[j] == portId2) || (m_linkList->get(i)->portIdArray2[j] == portId1))
					{
                  linkExists = true;
                  break;
               }
				}
				if (!linkExists && (m_linkList->get(i)->portIdCount < MAX_PORT_COUNT))
				{
					m_linkList->get(i)->portIdArray1[j] = portId2;
					m_linkList->get(i)->portIdArray2[j] = portId1;
					m_linkList->get(i)->portIdCount++;
					UpdatePortNames(m_linkList->get(i), port2, port1);
					m_linkList->get(i)->type = LINK_TYPE_MULTILINK;
               linkExists = true;
				}
				break;
			}
      }
      if (!linkExists)
      {
         ObjLink* obj = new ObjLink();
         obj->id1 = id1;
         obj->id2 = id2;
         obj->type = LINK_TYPE_NORMAL;
			obj->portIdCount = 1;
			obj->portIdArray1[0] = portId1;
			obj->portIdArray2[0] = portId2;
			nx_strncpy(obj->port1, port1, MAX_CONNECTOR_NAME);
			nx_strncpy(obj->port2, port2, MAX_CONNECTOR_NAME);
			m_linkList->add(obj);
      }
   }
}

/**
 * Create NXCP message
 */
void NetworkMapObjectList::createMessage(NXCPMessage *msg)
{
	// Object list
	msg->setField(VID_NUM_OBJECTS, m_objectList->size());
	if (m_objectList->size() > 0)
		msg->setFieldFromInt32Array(VID_OBJECT_LIST, m_objectList);

	// Links between objects
	msg->setField(VID_NUM_LINKS, m_linkList->size());
   UINT32 dwId = VID_OBJECT_LINKS_BASE;
	for(int i = 0; i < m_linkList->size(); i++, dwId += 3)
	{
      ObjLink *l = m_linkList->get(i);
		msg->setField(dwId++, l->id1);
		msg->setField(dwId++, l->id2);
		msg->setField(dwId++, (WORD)l->type);
		msg->setField(dwId++, l->port1);
		msg->setField(dwId++, l->port2);
		msg->setField(dwId++, _T(""));
		msg->setField(dwId++, m_linkList->get(i)->flags);
	}
}

/**
 * Check if link between two given objects exist
 */
bool NetworkMapObjectList::isLinkExist(UINT32 objectId1, UINT32 objectId2) const
{
   for(int i = 0; i < m_linkList->size(); i++)
   {
      ObjLink *l = m_linkList->get(i);
		if ((l->id1 == objectId1) && (l->id2 == objectId2))
			return true;
	}
	return false;
}

/**
 * Check if given object exist
 */
bool NetworkMapObjectList::isObjectExist(UINT32 objectId) const
{
   return bsearch(&objectId, m_objectList->getBuffer(), m_objectList->size(), sizeof(UINT32), CompareObjectId) != NULL;
}
