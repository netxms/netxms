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
#define MAX_LINK_NAME         64
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
#define LINK_TYPE_NORMAL     	  0
#define LINK_TYPE_VPN        	  1
#define LINK_TYPE_MULTILINK     2
#define LINK_TYPE_AGENT_TUNNEL  3
#define LINK_TYPE_AGENT_PROXY   4
#define LINK_TYPE_SSH_PROXY     5
#define LINK_TYPE_SNMP_PROXY    6
#define LINK_TYPE_ICMP_PROXY    7
#define LINK_TYPE_SENSOR_PROXY  8
#define LINK_TYPE_ZONE_PROXY    9

/**
 * Link between objects
 */
struct NXCORE_EXPORTABLE ObjLink
{
   uint32_t object1;
   uint32_t object2;
   uint32_t iface1;
   uint32_t iface2;
   int type;
	TCHAR port1[MAX_CONNECTOR_NAME];
	TCHAR port2[MAX_CONNECTOR_NAME];
	uint32_t flags;
	MutableString name;

   ObjLink();
   ObjLink(const ObjLink& src);

   ObjLink& operator=(const ObjLink& src);

   void update(const ObjLink& src);
   void swap();

   bool equals(const ObjLink& other)
   {
      return (type == other.type) &&
             (((object1 == other.object1) && (iface1 == other.iface1) && (object2 == other.object2) && (iface2 == other.iface2)) ||
              ((object1 == other.object2) && (iface1 == other.iface2) && (object2 == other.object1) && (iface2 == other.iface1)));
   }
 };

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE ObjectArray<ObjLink>;
#endif

/**
 * Connected object list
 */
class NXCORE_EXPORTABLE NetworkMapObjectList
{
protected:
   IntegerArray<uint32_t> m_objectList;
   ObjectArray<ObjLink> m_linkList;
   bool m_allowDuplicateLinks;

public:
   NetworkMapObjectList();
   NetworkMapObjectList(const NetworkMapObjectList& src);

   void merge(const NetworkMapObjectList& src);

   void addObject(uint32_t object);
   void linkObjects(uint32_t object1, uint32_t iface1, const TCHAR *port1, uint32_t object2, uint32_t iface2, const TCHAR *port2, const TCHAR *linkName, int linkType);
   void linkObjects(uint32_t object1, uint32_t object2, int linkType = LINK_TYPE_NORMAL, const TCHAR *linkName = nullptr, const TCHAR *port1 = nullptr, const TCHAR *port2 = nullptr)
   {
      linkObjects(object1, 0, port1, object2, 0, port2, linkName, linkType);
   }
   void linkObjects(uint32_t object1, const Interface *iface1, uint32_t object2, const Interface *iface2, const TCHAR *linkName = nullptr, int linkType = LINK_TYPE_NORMAL)
   {
      linkObjects(
         object1, (iface1 != nullptr) ? iface1->getId() : 0, nullptr,
         object2, (iface2 != nullptr) ? iface2->getId() : 0, nullptr,
         linkName, linkType);
   }
   void removeObject(uint32_t id);
   void clear();
   void filterObjects(bool (*filter)(uint32_t, void *), void *context);
   template<typename C> void filterObjects(bool (*filter)(uint32_t, C *), C *context) { filterObjects(reinterpret_cast<bool (*)(uint32_t, void *)>(filter), context); }
   unique_ptr<ObjectArray<IntegerArray<uint32_t>>> getUnconnectedSets();

   int getNumObjects() const { return m_objectList.size(); }
   const IntegerArray<uint32_t>& getObjects() const { return m_objectList; }
   int getNumLinks() const { return m_linkList.size(); }
   const ObjectArray<ObjLink>& getLinks() const { return m_linkList; }

	void createMessage(NXCPMessage *pMsg);

   bool isObjectExist(uint32_t objectId) const;
	bool isLinkExist(const ObjLink& prototype) const { return getLink(prototype) != nullptr; }
   bool isLinkExist(uint32_t object1, uint32_t iface1, uint32_t object2, uint32_t iface2, int linkType) const
   {
      ObjLink prototype;
      prototype.object1 = object1;
      prototype.iface1 = iface1;
      prototype.object2 = object2;
      prototype.iface2 = iface2;
      prototype.type = linkType;
      return isLinkExist(prototype);
   }
	ObjLink *getLink(const ObjLink& prototype) const;
   ObjLink *getLink(uint32_t object1, uint32_t object2, int linkType) const
   {
      ObjLink prototype;
      prototype.object1 = object1;
      prototype.object2 = object2;
      prototype.type = linkType;
      return getLink(prototype);
   }

	void setAllowDuplicateLinks(bool allowDuplicateLinks) { m_allowDuplicateLinks = allowDuplicateLinks; }
	bool isAllowDuplicateLinks() const { return m_allowDuplicateLinks; }

	void dumpToLog() const;
};

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE shared_ptr<NetworkMapObjectList>;
#endif

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
#define AUTO_GENERATED           0x1
#define EXCLUDE_FROM_AUTO_UPDATE  0x2

/**
 * Generic map element
 */
class NetworkMapElement
{
protected:
   uint32_t m_id;
	int32_t m_type;
   int32_t m_posX;
   int32_t m_posY;
   uint32_t m_flags;

   NetworkMapElement(const NetworkMapElement& src);

public:
	NetworkMapElement(uint32_t id, uint32_t flags = 0);
	NetworkMapElement(uint32_t id, json_t *config, uint32_t flags = 0);
	NetworkMapElement(const NXCPMessage& msg, uint32_t baseId);
	virtual ~NetworkMapElement();

   virtual void updateInternalFields(NetworkMapElement *e);
	virtual void updateConfig(json_t *config);
	virtual void fillMessage(NXCPMessage *msg, uint32_t baseId);
	virtual json_t *toJson() const;
   virtual NetworkMapElement *clone() const;

   uint32_t getId() const { return m_id; }
   int32_t getType() const { return m_type; }
   int32_t getPosX() const { return m_posX; }
   int32_t getPosY() const { return m_posY; }
   uint32_t getFlags() const { return m_flags; }

	void setPosition(int32_t x, int32_t y);
};

/**
 * Object map element
 */
class NetworkMapObject : public NetworkMapElement
{
protected:
	uint32_t m_objectId;
   uint32_t m_width;
   uint32_t m_height;

   NetworkMapObject(const NetworkMapObject& src);

public:
	NetworkMapObject(uint32_t id, uint32_t objectId, uint32_t flags = 0);
	NetworkMapObject(uint32_t id, json_t *config, uint32_t flags = 0);
	NetworkMapObject(const NXCPMessage& msg, uint32_t baseId);
	virtual ~NetworkMapObject();

	virtual void updateConfig(json_t *config) override;
	virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) override;
   virtual json_t *toJson() const override;
   virtual NetworkMapElement *clone() const override;

	uint32_t getObjectId() const { return m_objectId; }
};

/**
 * Decoration map element
 */
class NetworkMapDecoration : public NetworkMapElement
{
protected:
   int32_t m_decorationType;
	UINT32 m_color;
	TCHAR *m_title;
   int32_t m_width;
   int32_t m_height;

   NetworkMapDecoration(const NetworkMapDecoration& src);

public:
	NetworkMapDecoration(uint32_t id, LONG decorationType, uint32_t flags = 0);
	NetworkMapDecoration(uint32_t id, json_t *config, uint32_t flags = 0);
	NetworkMapDecoration(const NXCPMessage& msg, uint32_t baseId);
	virtual ~NetworkMapDecoration();

	virtual void updateConfig(json_t *config) override;
	virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) override;
   virtual json_t *toJson() const override;
   virtual NetworkMapElement *clone() const override;

   int32_t getDecorationType() const { return m_decorationType; }
   uint32_t getColor() const { return m_color; }
	const TCHAR *getTitle() const { return CHECK_NULL_EX(m_title); }

   int32_t getWidth() const { return m_width; }
   int32_t getHeight() const { return m_height; }
};

/**
 * Network map text box
 */
class NetworkMapTextBox : public NetworkMapElement
{
protected:
   TCHAR *m_config;

   NetworkMapTextBox(const NetworkMapTextBox& src);

public:
   NetworkMapTextBox(uint32_t id, json_t *config, uint32_t flags);
   NetworkMapTextBox(const NXCPMessage& msg, uint32_t baseId);
   virtual ~NetworkMapTextBox();

   virtual void updateConfig(json_t *config) override;
   virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) override;
   virtual json_t *toJson() const override;
   virtual NetworkMapElement *clone() const override;

   const TCHAR *getConfig() const { return m_config; }
};

/**
 * Common base class for all DCI based elements
 */
class NetworkMapDCIElement : public NetworkMapElement
{
protected:
   String m_config;

   NetworkMapDCIElement(const NetworkMapDCIElement& src) : NetworkMapElement(src), m_config(src.m_config) {}

public:
   NetworkMapDCIElement(uint32_t id, json_t *config, uint32_t flags) : NetworkMapElement(id, config, flags), m_config(json_object_get_string_utf8(config, "DCIList", ""), "utf8") {}
   NetworkMapDCIElement(const NXCPMessage& msg, uint32_t baseId) : NetworkMapElement(msg, baseId), m_config(msg.getFieldAsString(baseId + 10), -1, Ownership::True) {}
   virtual ~NetworkMapDCIElement() = default;

   virtual void updateConfig(json_t *config) override;
   virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) override;
   virtual json_t *toJson() const override;

   const String& getConfig() const { return m_config; }
   void updateDciList(CountingHashSet<uint32_t> *dciSet, bool addItems) const;
};

/**
 * DCI map container
 */
class NetworkMapDCIContainer : public NetworkMapDCIElement
{
protected:
	NetworkMapDCIContainer(const NetworkMapDCIContainer& src) : NetworkMapDCIElement(src) {}

public:
	NetworkMapDCIContainer(uint32_t id, json_t *config, uint32_t flags) : NetworkMapDCIElement(id, config, flags) {}
	NetworkMapDCIContainer(const NXCPMessage& msg, uint32_t baseId) : NetworkMapDCIElement(msg, baseId) {}
	virtual ~NetworkMapDCIContainer() = default;

   virtual NetworkMapElement *clone() const override;
};

/**
 * DCI map image
 */
class NetworkMapDCIImage : public NetworkMapDCIElement
{
protected:
   NetworkMapDCIImage(const NetworkMapDCIImage& src) : NetworkMapDCIElement(src) {}


public:
   NetworkMapDCIImage(uint32_t id, json_t *config, uint32_t flags = 0) : NetworkMapDCIElement(id, config, flags) {}
   NetworkMapDCIImage(const NXCPMessage& msg, uint32_t baseId) : NetworkMapDCIElement(msg, baseId) {}
   virtual ~NetworkMapDCIImage() = default;

   virtual NetworkMapElement *clone() const override;
};

/**
 * Network map link color source
 */
enum MapLinkColorSource
{
   MAP_LINK_COLOR_SOURCE_UNDEFINED = -1,
   MAP_LINK_COLOR_SOURCE_DEFAULT = 0,
   MAP_LINK_COLOR_SOURCE_OBJECT_STATUS = 1,
   MAP_LINK_COLOR_SOURCE_CUSTOM_COLOR = 2,
   MAP_LINK_COLOR_SOURCE_SCRIPT = 3,
   MAP_LINK_COLOR_SOURCE_LINK_UTILIZATION = 4,
   MAP_LINK_COLOR_SOURCE_INTERFACE_STATUS = 5
};

/**
 * Network map link style
 */
enum class MapLinkStyle
{
   Default = 0,
   Solid = 1,
   Dash = 2,
   Dot = 3,
   DashDot = 4,
   DashDotDot = 5
};

/**
 * Link on map
 */
class NetworkMapLink
{
protected:
   uint32_t m_id;
	uint32_t m_element1;
	uint32_t m_element2;
	uint32_t m_interface1;
   uint32_t m_interface2;
	int m_type;
	TCHAR *m_name;
	TCHAR *m_connectorName1;
	TCHAR *m_connectorName2;
	uint32_t m_flags;
	MapLinkColorSource m_colorSource;
	uint32_t m_color;
	TCHAR *m_colorProvider;
	char *m_config;

public:
	NetworkMapLink(uint32_t id, uint32_t e1, uint32_t e2, int type);
   NetworkMapLink(uint32_t id, uint32_t e1, uint32_t iface1, uint32_t e2, uint32_t iface2, int type);
	NetworkMapLink(const NXCPMessage& msg, uint32_t baseId);
   NetworkMapLink(const NetworkMapLink& src);
	~NetworkMapLink();

	void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   json_t *toJson() const;

   uint32_t getId() const { return m_id; }
   uint32_t getElement1() const { return m_element1; }
   uint32_t getElement2() const { return m_element2; }
   uint32_t getInterface1() const { return m_interface1; }
   uint32_t getInterface2() const { return m_interface2; }

	const TCHAR *getName() const { return CHECK_NULL_EX(m_name); }
	const TCHAR *getConnector1Name() const { return CHECK_NULL_EX(m_connectorName1); }
	const TCHAR *getConnector2Name() const { return CHECK_NULL_EX(m_connectorName2); }
	int getType() const { return m_type; }
	uint32_t getFlags() const { return m_flags; }
	bool checkFlagSet(uint32_t flag) const { return (m_flags & flag) != 0; }
	bool isExcludeFromAutoUpdate() { return (m_flags & EXCLUDE_FROM_AUTO_UPDATE) != 0; }
	MapLinkColorSource getColorSource() const { return m_colorSource; }
	uint32_t getColor() const { return m_color; }
   const TCHAR *getColorProvider() const { return CHECK_NULL_EX(m_colorProvider); }
   const char *getConfig() const { return CHECK_NULL_EX_A(m_config); }
   void updateDciList(CountingHashSet<uint32_t>& dciSet, bool addItems) const;
   void updateColorSourceObjectList(CountingHashSet<uint32_t>& dciSet, bool addItems) const;

   bool update(const ObjLink& src, bool updateNames);

	void setName(const TCHAR *name)
	{
	   MemFree(m_name);
	   m_name = MemCopyString(name);
	}
	void setConnectedElements(uint32_t e1, uint32_t e2)
	{
	   m_element1 = e1;
	   m_element2 = e2;
	}
   void setConnectedElements(uint32_t e1, uint32_t iface1, uint32_t e2, uint32_t iface2)
   {
      m_element1 = e1;
      m_element2 = e2;
      m_interface1 = iface1;
      m_interface2 = iface2;
   }
   void setConnectedInterfaces(uint32_t iface1, uint32_t iface2)
   {
      m_interface1 = iface1;
      m_interface2 = iface2;
   }
   void setInterface1(uint32_t iface1)
   {
      m_interface1 = iface1;
   }
   void setInterface2(uint32_t iface2)
   {
      m_interface2 = iface2;
   }
	void setConnector1Name(const TCHAR *name)
	{
	   MemFree(m_connectorName1);
	   m_connectorName1 = MemCopyString(name);
	}
	void setConnector2Name(const TCHAR *name)
	{
	   MemFree(m_connectorName2);
	   m_connectorName2 = MemCopyString(name);
	}
	void setFlags(uint32_t flags)
	{
	   m_flags = flags;
	}
   void setColorSource(MapLinkColorSource colorSource) { m_colorSource = colorSource; }
   void setColor(uint32_t color)
   {
      m_color = color;
   }
   void setColorProvider(const TCHAR *colorProvider)
   {
      MemFree(m_colorProvider);
      m_colorProvider = MemCopyString(colorProvider);
   }
	void setConfig(char *config)
	{
	   MemFree(m_config);
	   m_config = config;
	}
	void moveConfig(NetworkMapLink *other);

	void swap()
	{
	   std::swap(m_element1, m_element2);
      std::swap(m_interface1, m_interface2);
      std::swap(m_connectorName1, m_connectorName2);
	}
};


/**
 * Link data shource location
 */
enum class LinkDataLocation
{
   CENTER = 0,
   OBJECT1 = 1,
   OBJECT2 = 2
};

#define LINK_DATA_LOCATION_CENTER "CENTER"
#define LINK_DATA_LOCATION_OBJECT1 "OBJECT1"
#define LINK_DATA_LOCATION_OBJECT2 "OBJECT2"

/**
 * Convert network map link data location to string
 */
static inline const char *LinkLocationToString(LinkDataLocation l)
{
   switch (l)
   {
      case LinkDataLocation::CENTER:
         return LINK_DATA_LOCATION_CENTER;
      case LinkDataLocation::OBJECT1:
         return LINK_DATA_LOCATION_OBJECT1;
      case LinkDataLocation::OBJECT2:
         return LINK_DATA_LOCATION_OBJECT2;
   }
   return LINK_DATA_LOCATION_CENTER;
}

/**
 * Convert network map link data location string to enum
 */
static inline LinkDataLocation LinkLocationFromString(const char *location)
{
   if (!stricmp(location, LINK_DATA_LOCATION_CENTER))
   {
      return LinkDataLocation::CENTER;
   }
   if (!stricmp(location, LINK_DATA_LOCATION_OBJECT1))
   {
      return LinkDataLocation::OBJECT1;
   }
   if (!stricmp(location, LINK_DATA_LOCATION_OBJECT2))
   {
      return LinkDataLocation::OBJECT2;
   }
   return LinkDataLocation::CENTER;
}

/**
 * Convert network map link data location int to enum
 */
static inline LinkDataLocation LinkLocationFromInt(uint32_t location)
{
   switch (location)
   {
      case 0:
         return LinkDataLocation::CENTER;
      case 1:
         return LinkDataLocation::OBJECT1;
      case 2:
         return LinkDataLocation::OBJECT2;
   }
   return LinkDataLocation::CENTER;
}

/**
 * Convert network map link data location int to enum
 */
static inline uint32_t IntFromLinkLocation(LinkDataLocation location)
{
   switch (location)
   {
      case LinkDataLocation::CENTER:
         return 0;
      case LinkDataLocation::OBJECT1:
         return 1;
      case LinkDataLocation::OBJECT2:
         return 2;
   }
   return 0;
}

/**
 * Link data shource provider
 * Used also for NXSL
 */
class LinkDataSouce
{
private:
   uint32_t m_nodeId;
   uint32_t m_dciId;
   String m_format;
   String m_name;
   bool m_system;
   LinkDataLocation m_location;


public:
   LinkDataSouce(json_t *config);

   uint32_t getNodeId() const { return m_nodeId; }
   uint32_t getDciId() const { return m_dciId; }
   const String& getName() const { return m_name; }
   const String& getFormat() const { return m_format; }
   LinkDataLocation getLocation() const { return m_location; }

   bool isSystem() const { return m_system; }
};

/**
 * Network map link container
 * Provides fields and updates fields
 * Used also for NXSL
 */
class NetworkMapLinkContainer
{
protected:
   NetworkMapLink *m_link;
   bool m_modified;
   json_t *m_config;

   json_t *getConfigInstance();

public:
   NetworkMapLinkContainer(const NetworkMapLink &link)
   {
     m_link = new NetworkMapLink(link);
     m_modified = false;
     m_config = nullptr;
   }

   ~NetworkMapLinkContainer()
   {
      delete m_link;
      json_decref(m_config);
   }

   NetworkMapLink *get() { return m_link; }
   NetworkMapLink *take()
   {
      NetworkMapLink *link = m_link;
      m_link = nullptr;
      return link;
   }

   NXSL_Value *getColorObjects(NXSL_VM *vm);
   bool isUsingActiveThresholds() { return json_object_get_boolean(getConfigInstance(), "useActiveThresholds", false); }
   bool useInterfaceUtilization() { return json_object_get_boolean(getConfigInstance(), "useInterfaceUtilization", false); }
   uint32_t getRouting() { return json_object_get_int32(getConfigInstance(), "routing", 0); }
   uint32_t getWidth() { return json_object_get_int32(getConfigInstance(), "width", 0); }
   uint32_t getStyle() { return json_object_get_int32(getConfigInstance(), "style", 0); }
   NXSL_Value *getDataSource(NXSL_VM *vm);
   unique_ptr<ObjectArray<LinkDataSouce>> getDataSource();
   void updateConfig();

   void updateDataSource(const shared_ptr<DCObjectInfo> &dci, const wchar_t *format, LinkDataLocation location);
   void addSystemDataSource(const shared_ptr<DCObjectInfo> &dci, const wchar_t *format, LinkDataLocation location);
   void updateDataSourceLocation(const shared_ptr<DCObjectInfo> &dci, LinkDataLocation location);
   void clearDataSource();
   void clearSystemDataSource();
   void removeDataSource(uint32_t index);
   void setRoutingAlgorithm(uint32_t algorithm);
   void setWidth(uint32_t width);
   void setStyle(uint32_t style);

   void setColorSource(MapLinkColorSource source);
   void setColorSourceToObjectStatus(const IntegerArray<uint32_t>& objects, bool useThresholds, bool useLinkUtilization);
   void setColorSourceToScript(const TCHAR *scriptName);
   void setColorSourceToCustomColor(uint32_t newColor);

   void setModified() { m_modified = true; }
   bool isModified() const { return m_modified; }
};

#endif
