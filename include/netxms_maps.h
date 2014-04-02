/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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


/**
 * Constants
 */

#define MAX_CONNECTOR_NAME		128
#define MAX_PORT_COUNT			16
#define MAX_BEND_POINTS       16


/**
 * User access rights
 */

#define MAP_ACCESS_READ       0x0001
#define MAP_ACCESS_WRITE      0x0002
#define MAP_ACCESS_ACL        0x0004
#define MAP_ACCESS_DELETE     0x0008


/**
 * Object link types
 */

#define LINK_TYPE_NORMAL      0
#define LINK_TYPE_VPN         1
#define LINK_TYPE_MULTILINK   2


/**
 * Link between objects
 */
class ObjLink
{
public:
   UINT32 id1;
   UINT32 id2;
   LONG type;
	TCHAR port1[MAX_CONNECTOR_NAME];
	TCHAR port2[MAX_CONNECTOR_NAME];
	int portIdCount;
	UINT32 portIdArray1[MAX_PORT_COUNT];
	UINT32 portIdArray2[MAX_PORT_COUNT];
	TCHAR *config;
	UINT32 flags;

   ObjLink();
   ObjLink(UINT32 id1, UINT32 id2, LONG type, TCHAR* port1, TCHAR* port2, int portIdCount, UINT32* portIdArray1, UINT32* portIdArray2, TCHAR* config, UINT32 flags);
   ObjLink(ObjLink* old);
   ~ObjLink();
 };


/**
 * Connected object list - used as source for nxSubmap::DoLayout
 */
class LIBNXMAP_EXPORTABLE nxmap_ObjList
{
protected:
   IntegerArray<UINT32> m_objectList;
   ObjectArray<ObjLink> m_linkList;

public:
   nxmap_ObjList();
   nxmap_ObjList(nxmap_ObjList *old);
   nxmap_ObjList(CSCPMessage *pMsg);
   ~nxmap_ObjList();

   void addObject(UINT32 id);
   void linkObjects(UINT32 id1, UINT32 id2);
   void linkObjectsEx(UINT32 id1, UINT32 id2, const TCHAR *port1, const TCHAR *port2, UINT32 portId1, UINT32 portId2);
   void removeObject(UINT32 id);
   void clear();

   UINT32 getNumObjects() { return m_objectList.size(); }
   IntegerArray<UINT32> *getObjects() { return &m_objectList; }
   UINT32 getNumLinks() { return m_linkList.size(); }
   ObjectArray<ObjLink> *getLinks() { return &m_linkList; }

	void createMessage(CSCPMessage *pMsg);

	bool isLinkExist(UINT32 objectId1, UINT32 objectId2);
	bool isObjectExist(UINT32 objectId);
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
 * Possible flag values for NetworMapElements
 */
#define AUTO_GENERATED           1

/**
 * Generic map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapElement
{
protected:
	UINT32 m_id;
	LONG m_type;
	LONG m_posX;
	LONG m_posY;
	UINT32 m_flags;

public:
	NetworkMapElement(UINT32 id, UINT32 flags = 0);
	NetworkMapElement(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapElement(CSCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapElement();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, UINT32 baseId);

	UINT32 getId() { return m_id; }
	LONG getType() { return m_type; }
	LONG getPosX() { return m_posX; }
	LONG getPosY() { return m_posY; }
	UINT32 getFlags() { return m_flags; }

	void setPosition(LONG x, LONG y);
};

/**
 * Object map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapObject : public NetworkMapElement
{
protected:
	UINT32 m_objectId;

public:
	NetworkMapObject(UINT32 id, UINT32 objectId, UINT32 flags = 0);
	NetworkMapObject(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapObject(CSCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapObject();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, UINT32 baseId);

	UINT32 getObjectId() { return m_objectId; }
};

/**
 * Decoration map element
 */
class LIBNXMAP_EXPORTABLE NetworkMapDecoration : public NetworkMapElement
{
protected:
	LONG m_decorationType;
	UINT32 m_color;
	TCHAR *m_title;
	LONG m_width;
	LONG m_height;

public:
	NetworkMapDecoration(UINT32 id, LONG decorationType, UINT32 flags = 0);
	NetworkMapDecoration(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapDecoration(CSCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapDecoration();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(CSCPMessage *msg, UINT32 baseId);

	LONG getDecorationType() { return m_decorationType; }
	UINT32 getColor() { return m_color; }
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
	UINT32 m_element1;
	UINT32 m_element2;
	int m_type;
	TCHAR *m_name;
	TCHAR *m_connectorName1;
	TCHAR *m_connectorName2;
	UINT32 m_color;
	UINT32 m_statusObject;
	int m_routing;
	UINT32 m_bendPoints[MAX_BEND_POINTS * 2];
	UINT32 m_flags;
	TCHAR *m_config;

public:
	NetworkMapLink(UINT32 e1, UINT32 e2, int type);
	NetworkMapLink(CSCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapLink();

	void fillMessage(CSCPMessage *msg, UINT32 baseId);

	UINT32 getElement1() { return m_element1; }
	UINT32 getElement2() { return m_element2; }

	const TCHAR *getName() { return CHECK_NULL_EX(m_name); }
	const TCHAR *getConnector1Name() { return CHECK_NULL_EX(m_connectorName1); }
	const TCHAR *getConnector2Name() { return CHECK_NULL_EX(m_connectorName2); }
	int getType() { return m_type; }
	UINT32 getColor() { return m_color; }
	UINT32 getStatusObject() { return m_statusObject; }
	int getRouting() { return m_routing; }
	TCHAR *getBendPoints(TCHAR *buffer);
	UINT32 getFlags() { return m_flags; }
	const TCHAR *getConfig() { return CHECK_NULL_EX(m_config); }
	bool checkFlagSet(UINT32 flags) { return (m_flags & flags); }

	void setName(const TCHAR *name);
	void setConnector1Name(const TCHAR *name);
	void setConnector2Name(const TCHAR *name);
	void setColor(UINT32 color) { m_color = color; }
	void setStatusObject(UINT32 object) { m_statusObject = object; }
	void setRouting(int routing) { m_routing = routing; }
	void parseBendPoints(const TCHAR *data);
	void setFlags(UINT32 flags) { m_flags = flags; }
	void setConfig(const TCHAR *name);
};


#endif
