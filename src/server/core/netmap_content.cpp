/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Raden Solutions
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
** File: netmap_content.cpp
**
**/

#include "nxcore.h"
#include <netxms_maps.h>

/**
 * Create empty map content
 */
NetworkMapContent::NetworkMapContent() : m_elements(0, 128, Ownership::True), m_links(0, 128, Ownership::True)
{
}

/**
 * Create copy of map content
 */
NetworkMapContent::NetworkMapContent(const NetworkMapContent& src) :
   m_elements(src.m_elements.size(), 128, Ownership::True), m_links(src.m_links.size(), 128, Ownership::True)
{
   for(int i = 0; i < src.m_elements.size(); i++)
      m_elements.add(src.m_elements.get(i)->clone());
   for(int i = 0; i < src.m_links.size(); i++)
      m_links.add(new NetworkMapLink(*src.m_links.get(i)));
}

/**
 * Get object ID from map element ID
 * Assumes that object data already locked
 */
uint32_t NetworkMapContent::objectIdFromElementId(uint32_t eid) const
{
   for(int i = 0; i < m_elements.size(); i++)
   {
      NetworkMapElement *e = m_elements.get(i);
      if (e->getId() == eid)
      {
         if (e->getType() == MAP_ELEMENT_OBJECT)
         {
            return static_cast<NetworkMapObject*>(e)->getObjectId();
         }
         else
         {
            return 0;
         }
      }
   }
   return 0;
}

/**
 * Get map element ID from object ID
 * Assumes that object data already locked
 */
uint32_t NetworkMapContent::elementIdFromObjectId(uint32_t oid) const
{
   for(int i = 0; i < m_elements.size(); i++)
   {
      NetworkMapElement *e = m_elements.get(i);
      if ((e->getType() == MAP_ELEMENT_OBJECT) && (static_cast<NetworkMapObject*>(e)->getObjectId() == oid))
      {
         return e->getId();
      }
   }
   return 0;
}
