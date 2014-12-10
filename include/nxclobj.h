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
class LIBNXCLIENT_EXPORTABLE AbstractObject : public RefCountObject
{
protected:
   UINT32 m_id;
   uuid_t m_guid;
   int m_class;
   TCHAR m_name[MAX_OBJECT_NAME];
   int m_status;
   InetAddress m_primaryIP;
   TCHAR *m_comments;
   StringMap m_customAttributes;
   UINT32 m_submapId;
   IntegerArray<UINT32> *m_parents;
   IntegerArray<UINT32> *m_children;
   GeoLocation m_geoLocation;

public:
   AbstractObject(NXCPMessage *msg);
   virtual ~AbstractObject();

   UINT32 getId() { return m_id; }
   int getObjectClass() { return m_class; }
   const TCHAR *getName() { return m_name; }
   const InetAddress& getPrimaryIP() { return m_primaryIP; }
   const TCHAR *getComments() { return m_comments; }
   const TCHAR *getCustomAttribute(const TCHAR *name) { return m_customAttributes.get(name); }
   UINT32 getSubmapId() { return m_submapId; }
   const GeoLocation& getGeoLocation() { return m_geoLocation; }
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
