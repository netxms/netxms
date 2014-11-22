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

/**
 * ObjLink class implementation
 */

/**
 * Constructors
 */

ObjLink::ObjLink()
{
   id1 = 0;
   id2 = 0;
   type = LINK_TYPE_NORMAL;
   port1[0] = 0;
   port2[0] = 0;
	portIdCount = 0;
	config = NULL;
	flags = 0;
}

ObjLink::ObjLink(UINT32 id1, UINT32 id2, LONG type, TCHAR* port1, TCHAR* port2, int portIdCount, UINT32* portIdArray1, UINT32* portIdArray2, TCHAR* config, UINT32 flags)
{
   this->id1 = id1;
   this->id2 = id2;
   this->type = type;
	_tcscpy(this->port1, port1);
	_tcscpy(this->port2, port2);
	this->portIdCount = portIdCount;

	for(int i = 0; i < portIdCount; i++)
	{
      this->portIdArray1[i] = portIdArray1[i];
      this->portIdArray2[i] = portIdArray2[i];
	}

   if(config != NULL)
      this->config = _tcsdup(config);
   else
      config = NULL;
	this->flags = flags;
}

ObjLink::ObjLink(ObjLink* old)
{
   id1 = old->id1;
   id2 = old->id2;
   type = old->type;
   _tcscpy(port1, old->port1);
	_tcscpy(port2, old->port2);
	portIdCount = old->portIdCount;

	for(int i = 0; i < portIdCount; i++)
	{
      this->portIdArray1[i] = old->portIdArray1[i];
      this->portIdArray2[i] = old->portIdArray2[i];
	}

   if(old->config != NULL)
      config = _tcsdup(old->config);
   else
      config = NULL;
	flags = old->flags;
}

ObjLink::~ObjLink()
{
   safe_free(config);
}

/**
 * nxmap_ObjList class implementation
 */

/**
 * Constructors
 */

nxmap_ObjList::nxmap_ObjList()
{
   m_objectList = new IntegerArray<UINT32>(16, 16);
   m_linkList = new ObjectArray<ObjLink>(16, 16, true);
}

nxmap_ObjList::nxmap_ObjList(NXCPMessage *msg)
{
   m_objectList = new IntegerArray<UINT32>(16, 16);
   m_linkList = new ObjectArray<ObjLink>(16, 16, true);

	msg->getFieldAsInt32Array(VID_OBJECT_LIST, m_objectList);

   int linksCount = msg->getFieldAsInt32(VID_NUM_LINKS);
	UINT32 dwId = VID_OBJECT_LINKS_BASE;
	for(int i = 0; i < linksCount; i++, dwId += 3)
	{
      ObjLink *obj = new ObjLink();
		obj->id1 = msg->getFieldAsUInt32(dwId++);
		obj->id2 = msg->getFieldAsUInt32(dwId++);
		obj->type = (int)msg->getFieldAsUInt16(dwId++);
		msg->getFieldAsString(dwId++, obj->port1, MAX_CONNECTOR_NAME);
		msg->getFieldAsString(dwId++, obj->port2, MAX_CONNECTOR_NAME);
		obj->config = msg->getFieldAsString(dwId++);
		obj->flags = msg->getFieldAsUInt32(dwId++);
		m_linkList->add(obj);
	}
}

/**
 * Copy constructor
 */
nxmap_ObjList::nxmap_ObjList(nxmap_ObjList *src)
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
nxmap_ObjList::~nxmap_ObjList()
{
   delete m_objectList;
   delete m_linkList;
}

/**
 * Clear list
 */
void nxmap_ObjList::clear()
{
   m_linkList->clear();
   m_objectList->clear();
}

/**
 * Add object to list
 */
void nxmap_ObjList::addObject(UINT32 id)
{
   if (m_objectList->indexOf(id) == -1)
   {
      m_objectList->add(id);
   }
}

/**
 * Remove object from list
 */
void nxmap_ObjList::removeObject(UINT32 id)
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
void nxmap_ObjList::linkObjects(UINT32 id1, UINT32 id2, int linkType, const TCHAR *linkName)
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
	_tcscat_s(link->port1, MAX_CONNECTOR_NAME, _T(", "));
	_tcscat_s(link->port1, MAX_CONNECTOR_NAME, port1);
	_tcscat_s(link->port2, MAX_CONNECTOR_NAME, _T(", "));
	_tcscat_s(link->port2, MAX_CONNECTOR_NAME, port2);
}

/**
 * Link two objects with named links
 */
void nxmap_ObjList::linkObjectsEx(UINT32 id1, UINT32 id2, const TCHAR *port1, const TCHAR *port2, UINT32 portId1, UINT32 portId2)
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
			obj->config = NULL;
			m_linkList->add(obj);
      }
   }
}

/**
 * Create NXCP message
 */
void nxmap_ObjList::createMessage(NXCPMessage *msg)
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
		msg->setField(dwId++, CHECK_NULL_EX(m_linkList->get(i)->config));
		msg->setField(dwId++, m_linkList->get(i)->flags);
	}
}

/**
 * Check if link between two given objects exist
 */
bool nxmap_ObjList::isLinkExist(UINT32 objectId1, UINT32 objectId2)
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
bool nxmap_ObjList::isObjectExist(UINT32 objectId)
{
   return !(m_objectList->indexOf(objectId) == -1);
}
