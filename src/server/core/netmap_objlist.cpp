/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
   iface1 = 0;
   iface2 = 0;
   type = LINK_TYPE_NORMAL;
   port1[0] = 0;
   port2[0] = 0;
	portIdCount = 0;
	flags = 0;
}

/**
 * Object link copy constructor
 */
ObjLink::ObjLink(const ObjLink& src) : name(src.name)
{
   id1 = src.id1;
   id2 = src.id2;
   iface1 = src.iface1;
   iface2 = src.iface2;
   type = src.type;
   _tcscpy(port1, src.port1);
	_tcscpy(port2, src.port2);
	portIdCount = src.portIdCount;
	memcpy(portIdArray1, src.portIdArray1, portIdCount * sizeof(uint32_t));
   memcpy(portIdArray2, src.portIdArray2, portIdCount * sizeof(uint32_t));
	flags = src.flags;
}

/**
 * Object link assignment operator
 */
ObjLink& ObjLink::operator=(const ObjLink& src)
{
   id1 = src.id1;
   id2 = src.id2;
   iface1 = src.iface1;
   iface2 = src.iface2;
   type = src.type;
   name = src.name;
   _tcscpy(port1, src.port1);
   _tcscpy(port2, src.port2);
   portIdCount = src.portIdCount;
   memcpy(portIdArray1, src.portIdArray1, portIdCount * sizeof(uint32_t));
   memcpy(portIdArray2, src.portIdArray2, portIdCount * sizeof(uint32_t));
   flags = src.flags;
   return *this;
}

/**
 * Update link from source
 */
void ObjLink::update(const ObjLink& src)
{
   name = src.name;
   type = src.type;
   flags = src.flags;
   portIdCount = src.portIdCount;
   bool swapped = ((id1 == src.id2) && (id2 == src.id1));
   if (swapped)
   {
      iface1 = src.iface2;
      iface2 = src.iface1;
      _tcscpy(port2, src.port1);
      _tcscpy(port1, src.port2);
      memcpy(portIdArray2, src.portIdArray1, portIdCount * sizeof(uint32_t));
      memcpy(portIdArray1, src.portIdArray2, portIdCount * sizeof(uint32_t));
   }
   else
   {
      iface1 = src.iface1;
      iface2 = src.iface2;
      _tcscpy(port1, src.port1);
      _tcscpy(port2, src.port2);
      memcpy(portIdArray1, src.portIdArray1, portIdCount * sizeof(uint32_t));
      memcpy(portIdArray2, src.portIdArray2, portIdCount * sizeof(uint32_t));
   }
}

/**
 * Create empty object list
 */
NetworkMapObjectList::NetworkMapObjectList() : m_objectList(64, 64), m_linkList(64, 64, Ownership::True)
{
   m_allowDuplicateLinks = false;
}

/**
 * Copy constructor
 */
NetworkMapObjectList::NetworkMapObjectList(const NetworkMapObjectList& src) : m_objectList(src.m_objectList), m_linkList(src.m_linkList.size(), 64, Ownership::True)
{
   m_allowDuplicateLinks = src.m_allowDuplicateLinks;
	for(int i = 0; i < src.m_linkList.size(); i++)
      m_linkList.add(new ObjLink(*src.m_linkList.get(i)));
}

/**
 * Merge two lists
 */
void NetworkMapObjectList::merge(const NetworkMapObjectList& src)
{
   if (src.isAllowDuplicateLinks())
      m_allowDuplicateLinks = true;

   for(int i = 0; i < src.m_objectList.size(); i++)
      addObject(src.m_objectList.get(i));

   for(int i = 0; i < src.m_linkList.size(); i++)
   {
      ObjLink *srcLink = src.m_linkList.get(i);
      ObjLink *currLink = getLink(srcLink->id1, srcLink->id2, srcLink->type);
      if (currLink != nullptr)
      {
         if (m_allowDuplicateLinks)
            m_linkList.add(new ObjLink(*srcLink));
         else
            currLink->update(*srcLink);
      }
      else
      {
         m_linkList.add(new ObjLink(*srcLink));
      }
   }
}

/**
 * Clear list
 */
void NetworkMapObjectList::clear()
{
   m_linkList.clear();
   m_objectList.clear();
}

/**
 * Filter objects using provided filter. Any object for which filter function will return false will be removed.
 */
void NetworkMapObjectList::filterObjects(bool (*filter)(uint32_t, void *), void *context)
{
   for(int i = 0; i < m_objectList.size(); i++)
   {
      if (!filter(m_objectList.get(i), context))
      {
         m_objectList.remove(i);
         i--;
      }
   }
}

/**
 * Add object to list
 */
void NetworkMapObjectList::addObject(uint32_t id)
{
   if (!isObjectExist(id))
   {
      m_objectList.add(id);
      m_objectList.sortAscending();
   }
}

/**
 * Remove object from list
 */
void NetworkMapObjectList::removeObject(uint32_t id)
{
   m_objectList.remove(m_objectList.indexOf(id));

   for(int i = 0; i < m_linkList.size(); i++)
   {
      if ((m_linkList.get(i)->id1 == id) || (m_linkList.get(i)->id2 == id))
      {
         m_linkList.remove(i);
         i--;
      }
   }
}

/**
 * Link two objects
 */
void NetworkMapObjectList::linkObjects(uint32_t id1, uint32_t id2, int linkType, const TCHAR *linkName, const TCHAR *port1, const TCHAR *port2)
{
   if ((m_objectList.indexOf(id1) == -1) || (m_objectList.indexOf(id2) == -1))
      return;  // both objects should exist

   // Check for duplicate links
   bool swappedSides = false;
   ObjLink *link = nullptr;
   for(int i = 0; i < m_linkList.size(); i++)
   {
      ObjLink *curr = m_linkList.get(i);
      if (((curr->id1 == id1) && (curr->id2 == id2)) ||
          ((curr->id2 == id1) && (curr->id1 == id2)))
      {
         if (m_allowDuplicateLinks)
         {
            if (curr->type == linkType)
            {
               swappedSides = (curr->id1 == id2);
               link = curr;
               break;
            }
         }
         else
         {
            swappedSides = (curr->id1 == id2);
            link = curr;
            break;
         }
      }
   }

   if (link == nullptr)
   {
      link = new ObjLink();
      link->id1 = id1;
      link->id2 = id2;
      link->type = linkType;
      m_linkList.add(link);
   }

   if (linkName != nullptr)
      link->name = linkName;
   if (port1 != nullptr)
      _tcslcpy(swappedSides ? link->port2 : link->port1, port1, MAX_CONNECTOR_NAME);
   if (port2 != nullptr)
      _tcslcpy(swappedSides ? link->port1 : link->port2, port2, MAX_CONNECTOR_NAME);
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
void NetworkMapObjectList::linkObjectsEx(uint32_t id1, uint32_t id2, const TCHAR *port1, const TCHAR *port2, uint32_t portId1, uint32_t portId2, const TCHAR *name)
{
   if ((m_objectList.indexOf(id1) == -1) || (m_objectList.indexOf(id2) == -1))
      return;  // both objects should exist

   if (id1 == id2)
      return;  // Link should not link the same object

   // Check for duplicate links
   bool linkExists = false;
   for(int i = 0; i < m_linkList.size(); i++)
   {
      if ((m_linkList.get(i)->id1 == id1) && (m_linkList.get(i)->id2 == id2))
      {
         int j;
         for(j = 0; j < m_linkList.get(i)->portIdCount; j++)
         {
            if ((m_linkList.get(i)->portIdArray1[j] == portId1) && (m_linkList.get(i)->portIdArray2[j] == portId2))
            {
               m_linkList.get(i)->name = name;
               linkExists = true;
               break;
            }
         }
         if (!linkExists && (m_linkList.get(i)->portIdCount < MAX_PORT_COUNT))
         {
            m_linkList.get(i)->portIdArray1[j] = portId1;
            m_linkList.get(i)->portIdArray2[j] = portId2;
            m_linkList.get(i)->portIdCount++;
            UpdatePortNames(m_linkList.get(i), port1, port2);
            m_linkList.get(i)->type = LINK_TYPE_MULTILINK;
            m_linkList.get(i)->name = name;
            linkExists = true;
         }
         break;
      }
      if ((m_linkList.get(i)->id1 == id2) && (m_linkList.get(i)->id2 == id1))
      {
         int j;
         for(j = 0; j < m_linkList.get(i)->portIdCount; j++)
         {
            if ((m_linkList.get(i)->portIdArray1[j] == portId2) && (m_linkList.get(i)->portIdArray2[j] == portId1))
            {
               m_linkList.get(i)->name = name;
               linkExists = true;
               break;
            }
         }
         if (!linkExists && (m_linkList.get(i)->portIdCount < MAX_PORT_COUNT))
         {
            m_linkList.get(i)->portIdArray1[j] = portId2;
            m_linkList.get(i)->portIdArray2[j] = portId1;
            m_linkList.get(i)->portIdCount++;
            UpdatePortNames(m_linkList.get(i), port2, port1);
            m_linkList.get(i)->type = LINK_TYPE_MULTILINK;
            m_linkList.get(i)->name = name;
            linkExists = true;
         }
         break;
      }
   }

   if (!linkExists)
   {
      ObjLink* obj = new ObjLink();
      obj->id1 = id1;
      obj->iface1 = portId1;
      obj->id2 = id2;
      obj->iface2 = portId2;
      obj->type = LINK_TYPE_NORMAL;
      obj->portIdCount = 1;
      obj->portIdArray1[0] = portId1;
      obj->portIdArray2[0] = portId2;
      obj->name = name;
      _tcslcpy(obj->port1, port1, MAX_CONNECTOR_NAME);
      _tcslcpy(obj->port2, port2, MAX_CONNECTOR_NAME);
      m_linkList.add(obj);
   }
}

/**
 * Create NXCP message
 */
void NetworkMapObjectList::createMessage(NXCPMessage *msg)
{
	// Object list
	msg->setField(VID_NUM_OBJECTS, m_objectList.size());
	if (m_objectList.size() > 0)
		msg->setFieldFromInt32Array(VID_OBJECT_LIST, &m_objectList);

	// Links between objects
	msg->setField(VID_NUM_LINKS, m_linkList.size());
	uint32_t fieldId = VID_OBJECT_LINKS_BASE;
	for(int i = 0; i < m_linkList.size(); i++, fieldId += 3)
	{
      ObjLink *l = m_linkList.get(i);
		msg->setField(fieldId++, l->id1);
		msg->setField(fieldId++, l->id2);
		msg->setField(fieldId++, (WORD)l->type);
		msg->setField(fieldId++, l->port1);
		msg->setField(fieldId++, l->port2);
		msg->setField(fieldId++, l->name);
		msg->setField(fieldId++, m_linkList.get(i)->flags);
	}
}

/**
 * Check if link between two given objects exist
 */
bool NetworkMapObjectList::isLinkExist(uint32_t objectId1, uint32_t objectId2, int type) const
{
   for(int i = 0; i < m_linkList.size(); i++)
   {
      ObjLink *l = m_linkList.get(i);
		if ((l->id1 == objectId1) && (l->id2 == objectId2) && ((type == -1) || (l->type == type)))
			return true;
	}
	return false;
}

/**
 * Get link between two given objects if it exists
 */
ObjLink *NetworkMapObjectList::getLink(uint32_t objectId1, uint32_t objectId2, int linkType)
{
   for(int i = 0; i < m_linkList.size(); i++)
   {
      ObjLink *l = m_linkList.get(i);
      if (((l->id1 == objectId1) && (l->id2 == objectId2)) ||
         (((l->id1 == objectId2) && (l->id2 == objectId1)) &&
           (l->type == linkType)))
         return l;
   }
   return nullptr;
}

/**
 * Compare object IDs
 */
static int CompareObjectId(const void *e1, const void *e2)
{
   uint32_t id1 = *static_cast<const uint32_t*>(e1);
   uint32_t id2 = *static_cast<const uint32_t*>(e2);
   return (id1 < id2) ? -1 : ((id1 > id2) ? 1 : 0);
}

/**
 * Check if given object exist
 */
bool NetworkMapObjectList::isObjectExist(uint32_t objectId) const
{
   return bsearch(&objectId, m_objectList.getBuffer(), m_objectList.size(), sizeof(uint32_t), CompareObjectId) != nullptr;
}
