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
#include <netxms_maps.h>

/**
 * Create empty object link
 */
ObjLink::ObjLink()
{
   object1 = 0;
   object2 = 0;
   iface1 = 0;
   iface2 = 0;
   type = LINK_TYPE_NORMAL;
   port1[0] = 0;
   port2[0] = 0;
	flags = 0;
}

/**
 * Object link copy constructor
 */
ObjLink::ObjLink(const ObjLink& src) : name(src.name)
{
   object1 = src.object1;
   object2 = src.object2;
   iface1 = src.iface1;
   iface2 = src.iface2;
   type = src.type;
   _tcscpy(port1, src.port1);
	_tcscpy(port2, src.port2);
	flags = src.flags;
}

/**
 * Object link assignment operator
 */
ObjLink& ObjLink::operator=(const ObjLink& src)
{
   object1 = src.object1;
   object2 = src.object2;
   iface1 = src.iface1;
   iface2 = src.iface2;
   type = src.type;
   name = src.name;
   _tcscpy(port1, src.port1);
   _tcscpy(port2, src.port2);
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
   bool swapped = ((object1 == src.object2) && (object2 == src.object1));
   if (swapped)
   {
      iface1 = src.iface2;
      iface2 = src.iface1;
      _tcscpy(port2, src.port1);
      _tcscpy(port1, src.port2);
   }
   else
   {
      iface1 = src.iface1;
      iface2 = src.iface2;
      _tcscpy(port1, src.port1);
      _tcscpy(port2, src.port2);
   }
}

/**
 * Swap sides for a link
 */
void ObjLink::swap()
{
   std::swap(object1, object2);
   std::swap(iface1, iface2);

   TCHAR tmp[MAX_CONNECTOR_NAME];
   _tcscpy(tmp, port1);
   _tcscpy(port2, port1);
   _tcscpy(port1, tmp);
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
      ObjLink *currLink = getLink(srcLink->object1, srcLink->object2, srcLink->type);
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
      ObjLink *link = m_linkList.get(i);
      if ((link->object1 == id) || (link->object2 == id))
      {
         m_linkList.remove(i);
         i--;
      }
   }
}

/**
 * Link two objects
 */
void NetworkMapObjectList::linkObjects(uint32_t object1, uint32_t iface1, const TCHAR *port1, uint32_t object2, uint32_t iface2, const TCHAR *port2, const TCHAR *linkName, int linkType)
{
   if ((m_objectList.indexOf(object1) == -1) || (m_objectList.indexOf(object2) == -1))
      return;  // both objects should exist

   // Check for duplicate links
   bool swappedSides = false;
   ObjLink *link = nullptr;
   for(int i = 0; i < m_linkList.size(); i++)
   {
      ObjLink *curr = m_linkList.get(i);
      if (((curr->object1 == object1) && (curr->object2 == object2) && (curr->iface1 == iface1) && (curr->iface2 == iface2)) ||
          ((curr->object1 == object2) && (curr->object2 == object1) && (curr->iface1 == iface2) && (curr->iface2 == iface1)))
      {
         if (m_allowDuplicateLinks)
         {
            if (curr->type == linkType)
            {
               swappedSides = (curr->object1 == object2);
               link = curr;
               break;
            }
         }
         else
         {
            swappedSides = (curr->object1 == object2);
            link = curr;
            break;
         }
      }
   }

   if (link == nullptr)
   {
      link = new ObjLink();
      link->object1 = object1;
      link->iface1 = iface1;
      link->object2 = object2;
      link->iface2 = iface2;
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
	for(int i = 0; i < m_linkList.size(); i++, fieldId++)
	{
      ObjLink *l = m_linkList.get(i);
		msg->setField(fieldId++, l->object1);
		msg->setField(fieldId++, l->object2);
		msg->setField(fieldId++, static_cast<int16_t>(l->type));
		msg->setField(fieldId++, l->port1);
		msg->setField(fieldId++, l->port2);
		msg->setField(fieldId++, l->name);
		msg->setField(fieldId++, l->flags);
      msg->setField(fieldId++, l->iface1);
      msg->setField(fieldId++, l->iface2);
	}
}

/**
 * Get link between two given objects if it exists
 */
ObjLink *NetworkMapObjectList::getLink(const ObjLink& prototype) const
{
   for(int i = 0; i < m_linkList.size(); i++)
   {
      ObjLink *l = m_linkList.get(i);
      if (l->equals(prototype))
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

/**
 * Dump objects and links to log
 */
void NetworkMapObjectList::dumpToLog() const
{
   for(int i = 0; i < m_objectList.size(); i++)
   {
      uint32_t id = m_objectList.get(i);
      nxlog_debug_tag(_T("netmap"), 7, _T("   %s [%u]"), GetObjectName(id, _T("?")), id);
   }

   for(int i = 0; i < m_linkList.size(); i++)
   {
      const ObjLink *link = m_linkList.get(i);
      nxlog_debug_tag(_T("netmap"), 7, _T("   %s [%u] --- %s [%u] ====== %s [%u] --- %s [%u]"),
         GetObjectName(link->object1, _T("?")), link->object1,
         (link->iface1 != 0) ? GetObjectName(link->iface1, _T("?")) : _T("none"), link->iface1,
         (link->iface2 != 0) ? GetObjectName(link->iface2, _T("?")) : _T("none"), link->iface2,
         GetObjectName(link->object2, _T("?")), link->object2);
   }
}

/**
 * Get connected element sets
 */
unique_ptr<ObjectArray<IntegerArray<uint32_t>>> NetworkMapObjectList::getUnconnectedSets()
{
   auto sets = make_unique<ObjectArray<IntegerArray<uint32_t>>>(0, 16, Ownership::True);
   for(int i = 0; i < m_linkList.size(); i++)
   {
      const ObjLink *link = m_linkList.get(i);
      int setIndex = -1;
      for (int j = 0; j < sets->size(); j++)
      {
         IntegerArray<uint32_t> *set = sets->get(j);
         if (set->contains(link->object1) || set->contains(link->object2))
         {
            if (setIndex == -1)
            {
               set->add(link->object1);
               set->add(link->object2);
               setIndex = j;
            }
            else
            {
               //connect sets with same objects
               sets->get(setIndex)->addAll(set);
               sets->remove(j);
               j--;
               break;
            }
         }
      }
      if (setIndex == -1)
      {
         IntegerArray<uint32_t> *set = new IntegerArray<uint32_t>();
         set->add(link->object1);
         set->add(link->object2);
         sets->add(set);
      }
   }

   for(int i = 0; i < m_objectList.size(); i++)
   {
      uint32_t objectId = m_objectList.get(i);
      bool found = false;
      for (int j = 0; j < sets->size(); j++)
      {
         IntegerArray<uint32_t> *set = sets->get(j);
         if (set->contains(objectId))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         IntegerArray<uint32_t> *set = new IntegerArray<uint32_t>();
         set->add(objectId);
         sets->add(set);

      }
   }
   return sets;
}
