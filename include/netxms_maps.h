/* 
** NetXMS - Network Management System
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
** File: netxms_maps.h
**
**/

#ifndef _netxms_maps_h_
#define _netxms_maps_h_

#ifdef _WIN32
#ifdef LIBNXMAP_EXPORTS
#define LIBNXMAP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXMAP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXMAP_EXPORTABLE
#endif

#include <nxconfig.h>


//
// Constants
//

#define MAX_CONNECTOR_NAME		128
#define MAX_PORT_COUNT			16
#define MAX_BEND_POINTS       16


//
// User access rights
//

#define MAP_ACCESS_READ       0x0001
#define MAP_ACCESS_WRITE      0x0002
#define MAP_ACCESS_ACL        0x0004
#define MAP_ACCESS_DELETE     0x0008


//
// Object link types
//

#define LINK_TYPE_NORMAL      0
#define LINK_TYPE_VPN         1
#define LINK_TYPE_MULTILINK   2


/**
 * Link between objects
 */
struct OBJLINK
{
   DWORD dwId1;
   DWORD dwId2;
   LONG nType;
	TCHAR szPort1[MAX_CONNECTOR_NAME];
	TCHAR szPort2[MAX_CONNECTOR_NAME];
	int portIdCount;
	DWORD portId1[MAX_PORT_COUNT];
	DWORD portId2[MAX_PORT_COUNT];
};

/**
 * Connected object list - used as source for nxSubmap::DoLayout
 */
class LIBNXMAP_EXPORTABLE nxmap_ObjList
{
protected:
   DWORD m_dwNumObjects;
	DWORD m_allocatedObjects;
   DWORD *m_pdwObjectList;
   DWORD m_dwNumLinks;
	DWORD m_allocatedLinks;
   OBJLINK *m_pLinkList;

public:
   nxmap_ObjList();
   nxmap_ObjList(nxmap_ObjList *pSrc);
   nxmap_ObjList(CSCPMessage *pMsg);
   ~nxmap_ObjList();

   void addObject(DWORD dwId);
   void linkObjects(DWORD dwId1, DWORD dwId2);
   void linkObjectsEx(DWORD dwId1, DWORD dwId2, const TCHAR *pszPort1, const TCHAR *pszPort2, DWORD portId1, DWORD portId2);
   void clear();

   DWORD getNumObjects() { return m_dwNumObjects; }
   DWORD *getObjects() { return m_pdwObjectList; }
   DWORD getNumLinks() { return m_dwNumLinks; }
   OBJLINK *getLinks() { return m_pLinkList; }

	void createMessage(CSCPMessage *pMsg);

	bool isLinkExist(DWORD objectId1, DWORD objectId2);
	bool isObjectExist(DWORD objectId);
};

/**
 * Map element types
 */
#define MAP_ELEMENT_GENERIC      0
#define MAP_ELEMENT_OBJECT       1
#define MAP_ELEMENT_DECORATION   2

/**
 * Decoration types
 */
#define MAP_DECORATION_GROUP_BOX 0
#define MAP_DECORATION_IMAGE     1

/**
 * Routing modes for connections
 */
#define ROUTING_DEFAULT          0
#define ROUTING_DIRECT           1
#define ROUTING_MANHATTAN        2
#define ROUTING_BENDPOINTS       3

/**
 * Generic map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapElement
{
protected:
	DWORD m_id;
	LONG m_type;
	LONG m_posX;
	LONG m_posY;

public:
	NetworkMapElement(DWORD id);
	NetworkMapElement(DWORD id, Config *config);
	NetworkMapElement(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapElement();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getId() { return m_id; }
	LONG getType() { return m_type; }
	LONG getPosX() { return m_posX; }
	LONG getPosY() { return m_posY; }

	void setPosition(LONG x, LONG y);
};

/**
 * Object map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapObject : public NetworkMapElement
{
protected:
	DWORD m_objectId;

public:
	NetworkMapObject(DWORD id, DWORD objectId);
	NetworkMapObject(DWORD id, Config *config);
	NetworkMapObject(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapObject();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getObjectId() { return m_objectId; }
};

/**
 * Decoration map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapDecoration : public NetworkMapElement
{
protected:
	LONG m_decorationType;
	DWORD m_color;
	TCHAR *m_title;
	LONG m_width;
	LONG m_height;

public:
	NetworkMapDecoration(DWORD id, LONG decorationType);
	NetworkMapDecoration(DWORD id, Config *config);
	NetworkMapDecoration(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapDecoration();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, DWORD baseId);

	LONG getDecorationType() { return m_decorationType; }
	DWORD getColor() { return m_color; }
	const TCHAR *getTitle() { return CHECK_NULL_EX(m_title); }

	LONG getWidth() { return m_width; }
	LONG getHeight() { return m_height; }
};

/**
 * Link on map
 */
class LIBNXMAP_EXPORTABLE NetworkMapLink
{
protected:
	DWORD m_element1;
	DWORD m_element2;
	int m_type;
	TCHAR *m_name;
	TCHAR *m_connectorName1;
	TCHAR *m_connectorName2;
	DWORD m_color;
	DWORD m_statusObject;
	int m_routing;
	DWORD m_bendPoints[MAX_BEND_POINTS * 2];

public:
	NetworkMapLink(DWORD e1, DWORD e2, int type);
	NetworkMapLink(CSCPMessage *msg, DWORD baseId);
	virtual ~NetworkMapLink();

	void fillMessage(CSCPMessage *msg, DWORD baseId);

	DWORD getElement1() { return m_element1; }
	DWORD getElement2() { return m_element2; }

	const TCHAR *getName() { return CHECK_NULL_EX(m_name); }
	const TCHAR *getConnector1Name() { return CHECK_NULL_EX(m_connectorName1); }
	const TCHAR *getConnector2Name() { return CHECK_NULL_EX(m_connectorName2); }
	int getType() { return m_type; }
	DWORD getColor() { return m_color; }
	DWORD getStatusObject() { return m_statusObject; }
	int getRouting() { return m_routing; }
	TCHAR *getBendPoints(TCHAR *buffer);

	void setName(const TCHAR *name);
	void setConnector1Name(const TCHAR *name);
	void setConnector2Name(const TCHAR *name);
	void setColor(DWORD color) { m_color = color; }
	void setStatusObject(DWORD object) { m_statusObject = object; }
	void setRouting(int routing) { m_routing = routing; }
	void parseBendPoints(const TCHAR *data);
};


#endif
