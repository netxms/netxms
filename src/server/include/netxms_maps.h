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
** File: netxms_maps.h
**
**/

#ifndef _netxms_maps_h_
#define _netxms_maps_h_

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
	UINT32 flags;

   ObjLink();
   ObjLink(const ObjLink *src);
 };

/**
 * Connected object list
 */
class NetworkMapObjectList
{
protected:
   IntegerArray<UINT32> *m_objectList;
   ObjectArray<ObjLink> *m_linkList;

public:
   NetworkMapObjectList();
   NetworkMapObjectList(NetworkMapObjectList *src);
   ~NetworkMapObjectList();

   void merge(const NetworkMapObjectList *src);

   void addObject(UINT32 id);
   void linkObjects(UINT32 id1, UINT32 id2, int linkType = LINK_TYPE_NORMAL, const TCHAR *linkName = NULL);
   void linkObjectsEx(UINT32 id1, UINT32 id2, const TCHAR *port1, const TCHAR *port2, UINT32 portId1, UINT32 portId2);
   void removeObject(UINT32 id);
   void clear();

   int getNumObjects() const { return m_objectList->size(); }
   IntegerArray<UINT32> *getObjects() { return m_objectList; }
   int getNumLinks() const { return m_linkList->size(); }
   ObjectArray<ObjLink> *getLinks() { return m_linkList; }

	void createMessage(NXCPMessage *pMsg);

	bool isLinkExist(UINT32 objectId1, UINT32 objectId2) const;
	bool isObjectExist(UINT32 objectId) const;
};

/**
 * Map element types
 */
#define MAP_ELEMENT_GENERIC         0
#define MAP_ELEMENT_OBJECT          1
#define MAP_ELEMENT_DECORATION      2
#define MAP_ELEMENT_DCI_CONTAINER   3
#define MAP_ELEMENT_DCI_IMAGE       4
#define MAP_ELEMENT_TEXT_BOX        5

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
class NetworkMapElement
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
	NetworkMapElement(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapElement();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
	virtual json_t *toJson() const;

	UINT32 getId() const { return m_id; }
	LONG getType() const { return m_type; }
	LONG getPosX() const { return m_posX; }
	LONG getPosY() const { return m_posY; }
	UINT32 getFlags() const { return m_flags; }

	void setPosition(LONG x, LONG y);
};

/**
 * Object map element
 */
class NetworkMapObject : public NetworkMapElement
{
protected:
	UINT32 m_objectId;

public:
	NetworkMapObject(UINT32 id, UINT32 objectId, UINT32 flags = 0);
	NetworkMapObject(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapObject(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapObject();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual json_t *toJson() const;

	UINT32 getObjectId() const { return m_objectId; }
};

/**
 * Decoration map element
 */
class NetworkMapDecoration : public NetworkMapElement
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
	NetworkMapDecoration(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapDecoration();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual json_t *toJson() const;

	LONG getDecorationType() const { return m_decorationType; }
	UINT32 getColor() const { return m_color; }
	const TCHAR *getTitle() const { return CHECK_NULL_EX(m_title); }

	LONG getWidth() const { return m_width; }
	LONG getHeight() const { return m_height; }
};

/**
 * DCI map conatainer
 */
class NetworkMapDCIContainer : public NetworkMapElement
{
protected:
	TCHAR *m_xmlDCIList;

public:
	NetworkMapDCIContainer(UINT32 id, TCHAR* objectDCIList, UINT32 flags = 0);
	NetworkMapDCIContainer(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapDCIContainer(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapDCIContainer();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual json_t *toJson() const;

	const TCHAR *getObjectDCIList() const { return m_xmlDCIList; }
};

/**
 * Network map text box
 */
class NetworkMapTextBox : public NetworkMapElement
{
protected:
	TCHAR *m_config;

public:
	NetworkMapTextBox(UINT32 id, TCHAR* objectDCIList, UINT32 flags = 0);
	NetworkMapTextBox(UINT32 id, Config *config, UINT32 flags = 0);
	NetworkMapTextBox(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapTextBox();

	virtual void updateConfig(Config *config);
	virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual json_t *toJson() const;

	TCHAR *getObjectDCIList() const { return m_config; }
};

/**
 * DCI map image
 */
class NetworkMapDCIImage : public NetworkMapElement
{
protected:
   TCHAR *m_config;

public:
   NetworkMapDCIImage(UINT32 id, TCHAR* objectDCIList, UINT32 flags = 0);
   NetworkMapDCIImage(UINT32 id, Config *config, UINT32 flags = 0);
   NetworkMapDCIImage(NXCPMessage *msg, UINT32 baseId);
   virtual ~NetworkMapDCIImage();

   virtual void updateConfig(Config *config);
   virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual json_t *toJson() const;

   TCHAR *getObjectDCIList() const { return m_config; }
};

/**
 * Link on map
 */
class NetworkMapLink
{
protected:
	UINT32 m_element1;
	UINT32 m_element2;
	int m_type;
	TCHAR *m_name;
	TCHAR *m_connectorName1;
	TCHAR *m_connectorName2;
	UINT32 m_flags;
	TCHAR *m_config;

public:
	NetworkMapLink(UINT32 e1, UINT32 e2, int type);
	NetworkMapLink(NXCPMessage *msg, UINT32 baseId);
	virtual ~NetworkMapLink();

	void fillMessage(NXCPMessage *msg, UINT32 baseId);
   json_t *toJson() const;

	UINT32 getElement1() const { return m_element1; }
	UINT32 getElement2() const { return m_element2; }

	const TCHAR *getName() const { return CHECK_NULL_EX(m_name); }
	const TCHAR *getConnector1Name() const { return CHECK_NULL_EX(m_connectorName1); }
	const TCHAR *getConnector2Name() const { return CHECK_NULL_EX(m_connectorName2); }
	int getType() const { return m_type; }
	UINT32 getFlags() const { return m_flags; }
	const TCHAR *getConfig() const { return CHECK_NULL_EX(m_config); }
	bool checkFlagSet(UINT32 flag) const { return (m_flags & flag) != 0; }

	void setName(const TCHAR *name);
	void setConnector1Name(const TCHAR *name);
	void setConnector2Name(const TCHAR *name);
	void setFlags(UINT32 flags) { m_flags = flags; }
	void setConfig(const TCHAR *name);
	void swap();
};

#endif
