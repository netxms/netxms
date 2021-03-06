/*
** NetXMS - Network Management System
** Client Library API
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: nxclobj.h
**
**/

#ifndef _nxclobj_h_
#define _nxclobj_h_

#include <geolocation.h>

/**
 * Generic object class
 */
class LIBNXCLIENT_EXPORTABLE AbstractObject
{
protected:
   uint32_t m_id;
   uuid_t m_guid;
   int m_class;
   TCHAR m_name[MAX_OBJECT_NAME];
   int m_status;
   InetAddress m_primaryIP;
   TCHAR *m_comments;
   StringMap m_customAttributes;
   uint32_t m_submapId;
   IntegerArray<uint32_t> *m_parents;
   IntegerArray<uint32_t> *m_children;
   GeoLocation m_geoLocation;

public:
   AbstractObject(NXCPMessage *msg);
   virtual ~AbstractObject();

   uint32_t getId() const { return m_id; }
   int getObjectClass() const { return m_class; }
   const TCHAR *getName() const { return m_name; }
   const InetAddress& getPrimaryIP() const { return m_primaryIP; }
   const TCHAR *getComments() const { return m_comments; }
   const TCHAR *getCustomAttribute(const TCHAR *name) const { return m_customAttributes.get(name); }
   uint32_t getSubmapId() const { return m_submapId; }
   const GeoLocation& getGeoLocation() const { return m_geoLocation; }
};

/**
 * Node object
 */
class LIBNXCLIENT_EXPORTABLE Node : public AbstractObject
{
protected:
   TCHAR *m_primaryHostname;

public:
   Node(NXCPMessage *msg);
   virtual ~Node();

   const TCHAR *getPrimaryHostname() { return m_primaryHostname; }
};

#endif
