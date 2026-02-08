/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: netmap_link.cpp
**
**/

#include "nxcore.h"
#include <netxms_maps.h>
#include <pugixml.h>

/**
 * Constructor
 */
NetworkMapLink::NetworkMapLink(uint32_t id, uint32_t e1, uint32_t e2, int type)
{
   m_id = id;
	m_element1 = e1;
	m_element2 = e2;
	m_interface1 = 0;
   m_interface2 = 0;
	m_type = type;
	m_name = nullptr;
	m_connectorName1 = nullptr;
	m_connectorName2 = nullptr;
	m_colorSource = MAP_LINK_COLOR_SOURCE_DEFAULT;
	m_color = 0;
	m_colorProvider = nullptr;
	m_config = nullptr;
	m_flags = 0;
}

/**
 * Constructor
 */
NetworkMapLink::NetworkMapLink(uint32_t id, uint32_t e1, uint32_t iface1, uint32_t e2, uint32_t iface2, int type)
{
   m_id = id;
   m_element1 = e1;
   m_element2 = e2;
   m_interface1 = iface1;
   m_interface2 = iface2;
   m_type = type;
   m_name = nullptr;
   m_connectorName1 = nullptr;
   m_connectorName2 = nullptr;
   m_colorSource = MAP_LINK_COLOR_SOURCE_DEFAULT;
   m_color = 0;
   m_colorProvider = nullptr;
   m_config = nullptr;
   m_flags = 0;
}

/**
 * Constuctor: create link object from NXCP message
 */
NetworkMapLink::NetworkMapLink(const NXCPMessage& msg, uint32_t baseId)
{
   m_id = msg.getFieldAsUInt32(baseId);
   m_name = msg.getFieldAsString(baseId + 1);
	m_type = msg.getFieldAsUInt16(baseId + 2);
	m_connectorName1 = msg.getFieldAsString(baseId + 3);
	m_connectorName2 = msg.getFieldAsString(baseId + 4);
	m_element1 = msg.getFieldAsUInt32(baseId + 5);
	m_element2 = msg.getFieldAsUInt32(baseId + 6);
	m_flags = msg.getFieldAsUInt32(baseId + 7);
   m_colorSource = static_cast<MapLinkColorSource>(msg.getFieldAsInt16(baseId + 8));
   m_color = msg.getFieldAsUInt32(baseId + 9);
   m_colorProvider = msg.getFieldAsString(baseId + 10);
   m_config = msg.getFieldAsUtf8String(baseId + 11);
   m_interface1 = msg.getFieldAsUInt32(baseId + 12);
   m_interface2 = msg.getFieldAsUInt32(baseId + 13);
}

/**
 * Map link copy constructor
 */
NetworkMapLink::NetworkMapLink(const NetworkMapLink& src)
{
   m_id = src.m_id;
   m_element1 = src.m_element1;
   m_element2 = src.m_element2;
   m_interface1 = src.m_interface1;
   m_interface2 = src.m_interface2;
   m_type = src.m_type;
   m_name = MemCopyString(src.m_name);
   m_connectorName1 = MemCopyString(src.m_connectorName1);
   m_connectorName2 = MemCopyString(src.m_connectorName2);
   m_colorSource = src.m_colorSource;
   m_color = src.m_color;
   m_colorProvider = MemCopyString(src.m_colorProvider);
   m_config = MemCopyStringA(src.m_config);
   m_flags = src.m_flags;
}

/**
 * Map link destructor
 */
NetworkMapLink::~NetworkMapLink()
{
	MemFree(m_name);
	MemFree(m_connectorName1);
	MemFree(m_connectorName2);
   MemFree(m_colorProvider);
	MemFree(m_config);
}

/**
 * Move config from another link object
 */
void NetworkMapLink::moveConfig(NetworkMapLink *other)
{
   MemFree(m_config);
   m_config = other->m_config;
   other->m_config = nullptr;
}

/**
 * Write XML to string
 */
class xml_string_writer : public pugi::xml_writer
{
public:
   StringBuffer result;

   virtual void write(const void* data, size_t size) override
   {
      result.appendUtf8String(static_cast<const char*>(data), size);
   }
};

/**
 * Update link from object link
 */
bool NetworkMapLink::update(const ObjLink& src, bool updateNames)
{
   if (isExcludeFromAutoUpdate())
      return false;

   bool modified = false;

   if (m_interface1 != src.iface1)
   {
      m_interface1 = src.iface1;
      modified = true;
   }

   if (m_interface2 != src.iface2)
   {
      m_interface2 = src.iface2;
      modified = true;
   }

   if (updateNames && _tcscmp(CHECK_NULL_EX(m_name), src.name))
   {
      setName(src.name);
      modified = true;
   }

   if (updateNames && _tcscmp(CHECK_NULL_EX(m_connectorName1), src.port1))
   {
      setConnector1Name(src.port1);
      modified = true;
   }

   if (updateNames && _tcscmp(CHECK_NULL_EX(m_connectorName2), src.port2))
   {
      setConnector2Name(src.port2);
      modified = true;
   }

   json_t *json;
   if ((m_config == nullptr) || (*m_config == 0))
   {
      json = json_object();
      json_object_set_new(json, "dciList", json_array());
   }
   else
   {
      json_error_t error;
      json = json_loads(m_config, 0, &error);
      if (json == nullptr)
      {
         nxlog_debug_tag(_T("netmap"), 6, _T("NetworkMapLink::update(%u): Failed to load JSON"), m_id);
         json = json_object();
         json_object_set_new(json, "routing", json_integer(0));
         json_object_set_new(json, "dciList", json_array());
      }
   }

   bool configModified = false;
   if (src.type == LINK_TYPE_VPN && json_object_get_uint32(json, "style", 0) != 3)
   {
      json_object_set_new(json, "style", json_integer(3));
      configModified = true;
   }

   json_t *statusList = json_object_get(json, "objectStatusList");
   if (json_array_size(statusList) != 0)
   {
      json_array_clear(statusList);
      configModified = true;
   }

   if (configModified)
   {
      char *s = json_dumps(json, JSON_COMPACT);
      setConfig(s);
      modified = true;
   }
   json_decref(json);

   return modified;
}

/**
 * Fill NXCP message
 */
void NetworkMapLink::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, getName());
	msg->setField(baseId + 2, static_cast<int16_t>(m_type));
   msg->setField(baseId + 3, m_element1);
   msg->setField(baseId + 4, m_element2);
	msg->setField(baseId + 5, getConnector1Name());
	msg->setField(baseId + 6, getConnector2Name());
   msg->setField(baseId + 7, m_flags);
   msg->setField(baseId + 8, static_cast<int16_t>(m_colorSource));
   msg->setField(baseId + 9, m_color);
   msg->setField(baseId + 10, getColorProvider());
   msg->setFieldFromUtf8String(baseId + 11, m_config);
   msg->setField(baseId + 12, m_interface1);
   msg->setField(baseId + 13, m_interface2);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapLink::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "element1", json_integer(m_element1));
   json_object_set_new(root, "element2", json_integer(m_element2));
   json_object_set_new(root, "interface1", json_integer(m_interface1));
   json_object_set_new(root, "interface2", json_integer(m_interface2));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "connectorName1", json_string_t(m_connectorName1));
   json_object_set_new(root, "connectorName2", json_string_t(m_connectorName2));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "colorSource", json_integer(static_cast<int32_t>(m_colorSource)));
   json_object_set_new(root, "color", json_integer(m_color));
   json_object_set_new(root, "colorProvider", json_string_t(m_colorProvider));
   json_object_set_new(root, "config", json_string(m_config));
   return root;
}

/**
 * Update list of DCIs referenced by owning map
 */
void NetworkMapLink::updateDciList(CountingHashSet<uint32_t>& dciSet, bool addItems) const
{
   if (m_config == nullptr)
      return;
   nxlog_debug_tag(_T("netmap"), 7, _T("NetworkMapLink::getDciList(%u): link configuration: %hs"), m_id, m_config);
   json_error_t error;
   json_t *json = json_loads(m_config, 0, &error);
   if (json == nullptr)
   {
      nxlog_debug_tag(_T("netmap"), 6, _T("NetworkMapLink::getDciList(%d): Failed to load json (%hs)"), m_id, error.text);
      return;
   }
   void *it = json_object_iter(json_object_get(json, "dciList"));
   while(it != nullptr)
   {
      json_t *value = json_object_iter_value(it);
      uint32_t id = json_object_get_uint32(value, "dciId", 0);
      if (addItems)
         dciSet.put(id);
      else
         dciSet.remove(id);
      it = json_object_iter_next(json, it);
   }
   json_decref(json);
}

/**
 * Update list of objects referenced by owning map
 */
void NetworkMapLink::updateColorSourceObjectList(CountingHashSet<uint32_t>& objectSet, bool addItems) const
{
   if ((m_config == nullptr) || (m_colorSource != MAP_LINK_COLOR_SOURCE_OBJECT_STATUS))
      return;

   pugi::xml_document xml;
   json_error_t error;
   json_t *json = json_loads(m_config, 0, &error);
   if (json == nullptr)
   {
      nxlog_debug_tag(_T("netmap"), 6, _T("NetworkMapLink::updateColorSourceObjectList(%d): Failed to load json (%hs)"), m_id, error.text);
      return;
   }

   json_t *array = json_object_get(json, "objectStatusList");
   if (json_is_array(array))
   {
      size_t i;
      json_t *m;
      json_array_foreach(array, i, m)
      {
         uint32_t id = static_cast<uint32_t>(json_integer_value(m));
         if (addItems)
            objectSet.put(id);
         else
            objectSet.remove(id);
      }
   }
   json_decref(json);
}

/**
 * Get config instance or create one
 */
json_t *NetworkMapLinkContainer::getConfigInstance()
{
   if (m_config == nullptr)
   {
      json_error_t error;
      m_config = json_loads(m_link->getConfig(), 0, &error);
      if (m_config == nullptr)
      {
         if (*m_link->getConfig() != 0)
            nxlog_debug_tag(_T("netmap"), 6, _T("Cannot parse response JSON: %s"), m_config);
         m_config = json_object();
         json_object_set_new(m_config, "routing", json_integer(0));
         json_object_set_new(m_config, "dciList", json_array());
         json_object_set_new(m_config, "objectStatusList", json_array());
      }
   }
   return m_config;
}

/**
 * Get color objects array
 */
NXSL_Value *NetworkMapLinkContainer::getColorObjects(NXSL_VM *vm)
{
   json_t *config = getConfigInstance();
   NXSL_Array *array = new NXSL_Array(vm);
   json_t *elements = json_object_get(config, "objectStatusList");
   if (json_is_array(elements))
   {
      size_t i;
      json_t *m;
      json_array_foreach(elements, i, m)
      {
         array->append(vm->createValue(static_cast<uint32_t>(json_integer_value(m))));
      }
   }
   return vm->createValue(array);
}

/**
 * Get link data source array
 */
NXSL_Value *NetworkMapLinkContainer::getDataSource(NXSL_VM *vm)
{
   NXSL_Array *array = new NXSL_Array(vm);
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (json_is_array(dciList))
   {
      size_t i;
      json_t *m;
      json_array_foreach(dciList, i, m)
      {
         LinkDataSouce *dataSolurce = new LinkDataSouce(m);
         array->append(vm->createValue(vm->createObject(&g_nxslLinkDataSourceClass, dataSolurce)));
      }
   }

   return vm->createValue(array);
}

/**
 * Get link data source array
 */
unique_ptr<ObjectArray<LinkDataSouce>> NetworkMapLinkContainer::getDataSource()
{
   unique_ptr<ObjectArray<LinkDataSouce>> array(new ObjectArray<LinkDataSouce>(0, 8, Ownership::True));
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (json_is_array(dciList))
   {
      size_t i;
      json_t *m;
      json_array_foreach(dciList, i, m)
      {
         array->add(new LinkDataSouce(m));
      }
   }

   return array;
}

/**
 * Update data source location on link
 *
 * @param dci to change location
 * @param location new location
 */
void NetworkMapLinkContainer::updateDataSourceLocation(const shared_ptr<DCObjectInfo> &dci, LinkDataLocation location)
{
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (!json_is_array(dciList))
   {
      return;
   }

   size_t i;
   json_t *m;
   json_array_foreach(dciList, i, m)
   {
      LinkDataSouce dataSource(m);
      if (dataSource.getDciId() == dci->getId())
      {
         json_object_set_new(m, "location", json_string(LinkLocationToString(location)));
         setModified();
         break;
      }
   }
}


/**
 * Add new system data source entry alphabetically sorted
 *
 * @param dci dci to add
 * @param format format that should be used to display data
 * @param location data location on the link
 */
void NetworkMapLinkContainer::addSystemDataSource(const shared_ptr<DCObjectInfo> &dci, const wchar_t *format, LinkDataLocation location)
{
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (!json_is_array(dciList))
   {
      dciList = json_array();
      json_object_set_new(getConfigInstance(), "dciList", dciList);
   }

   size_t i;
   json_t *m;
   json_array_foreach(dciList, i, m)
   {
      LinkDataSouce dataSource(m);
      if (dataSource.isSystem())
      {
         if (wcscmp(format, dataSource.getFormat()) < 0)
            break;
      }
   }

   json_t *dciElement = json_object();
   json_object_set_new(dciElement, "nodeId", json_integer(dci->getOwnerId()));
   json_object_set_new(dciElement, "dciId", json_integer(dci->getId()));
   json_object_set_new(dciElement, "type", json_integer(1));
   json_object_set_new(dciElement, "location", json_string(LinkLocationToString(location)));
   json_object_set_new(dciElement, "system", json_boolean(true));
   json_object_set_new(dciElement, "formatString", json_string_t(format));
   json_array_insert_new(dciList, i, dciElement);
   setModified();
}


/**
 * Update existing or add new data source entry
 *
 * @param dci dci to add
 * @param format format that should be used to display data
 * @param location data location on the link
 */
void NetworkMapLinkContainer::updateDataSource(const shared_ptr<DCObjectInfo> &dci, const wchar_t *format, LinkDataLocation location)
{
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (!json_is_array(dciList))
   {
      dciList = json_array();
      json_object_set_new(getConfigInstance(), "dciList", dciList);
   }

   size_t i;
   json_t *m;
   json_array_foreach(dciList, i, m)
   {
      LinkDataSouce dataSource(m);
      if (dataSource.getDciId() == dci->getId())
      {
         if (!dataSource.getFormat().equals(format))
         {
            json_t *formatString = json_object_get(m, "formatString");
            json_object_update_new(formatString, json_string_t(format));
            setModified();
         }
         if (dataSource.getLocation() != location)
         {
            json_object_set_new(m, "location", json_string(LinkLocationToString(location)));

            setModified();
         }
         if (dataSource.isSystem())
         {
            json_object_set_new(m, "system", json_boolean(false));
            setModified();
         }
         return;
      }
   }

   // DCI not found, add new entry
   json_t *dciElement = json_object();
   json_object_set_new(dciElement, "nodeId", json_integer(dci->getOwnerId()));
   json_object_set_new(dciElement, "dciId", json_integer(dci->getId()));
   json_object_set_new(dciElement, "type", json_integer(1));
   json_object_set_new(dciElement, "location", json_string(LinkLocationToString(location)));
   json_object_set_new(dciElement, "system", json_boolean(false));
   json_object_set_new(dciElement, "formatString", json_string_t(format));
   json_array_append_new(dciList, dciElement);
   setModified();
}

/**
 * Clear data shource list
 */
void NetworkMapLinkContainer::clearDataSource()
{
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if ((json_array_size(dciList) > 0) && (json_array_clear(dciList) == 0))
      setModified();
}

/**
 * Remove data source from list by index
 *
 * @param index index of data source to remove
 */
void NetworkMapLinkContainer::removeDataSource(uint32_t index)
{
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   if (json_array_remove(dciList, index) == 0)
      setModified();
}

/**
 * Remove data source from list by index
 *
 * @param index index of data source to remove
 */
void NetworkMapLinkContainer::clearSystemDataSource()
{
   bool modified = false;
   json_t *dciList = json_object_get(getConfigInstance(), "dciList");
   size_t i;
   json_t *m;
   json_array_foreach(dciList, i, m)
   {
      LinkDataSouce dataSource(m);
      if (dataSource.isSystem())
      {
         json_array_remove(dciList, i);
         i--;
         modified = true;
      }
   }

   if (modified)
      setModified();
}

/**
 * Set routing algorithm
 *
 * @param algorithm new algorithm should be between 0 and 3, other values are ignored
 */
void NetworkMapLinkContainer::setRoutingAlgorithm(uint32_t algorithm)
{
   if (algorithm > 3)
      return;

   json_t *config = getConfigInstance();

   if (json_object_get_uint32(config, "routing", 0) != algorithm)
   {
      json_object_set_new(config, "routing", json_integer(algorithm));
      setModified();
   }
}

/**
 * Set width
 *
 * @param width new width should be bigger or equals to 0
 */
void NetworkMapLinkContainer::setWidth(uint32_t width)
{
   json_t *config = getConfigInstance();
   if (json_object_get_uint32(config, "width", 0) != width)
   {
      json_object_set_new(config, "width", json_integer(width));
      setModified();
   }
}

/**
 * Set routing algorithm
 *
 * @param style new style should be between 0 and 5, other values are ignored
 */
void NetworkMapLinkContainer::setStyle(uint32_t style)
{
   if (style > ((uint32_t)MapLinkStyle::DashDotDot))
      return;

   json_t *config = getConfigInstance();
   if (json_object_get_uint32(config, "style", 0) != style)
   {
      json_object_set_new(config, "style", json_integer(style));
      setModified();
   }
}

/**
 * Set color source to specific value
 */
void NetworkMapLinkContainer::setColorSource(MapLinkColorSource source)
{
   if (m_link->getColorSource() != source)
   {
      m_link->setColorSource(source);
      setModified();
   }
}

/**
 * Set color source to "object status"
 *
 * @param objects list of objects' identifiers
 * @param useThresholds true if active thresholds should be used for color calculation
 * @param useLinkUtilization true if interface utilization should be used for color calculation
 */
void NetworkMapLinkContainer::setColorSourceToObjectStatus(const IntegerArray<uint32_t>& objects, bool useThresholds, bool useLinkUtilization)
{
   if (m_link->getColorSource() != MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
   {
      m_link->setColorSource(MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS);
      setModified();
   }

   json_t *config = getConfigInstance();
   json_t *elements = json_object_get(config, "objectStatusList");
   if (!json_is_array(elements))
   {
      elements =  json_array();
      json_object_set_new(config, "objectStatusList", elements);
   }

   size_t i;
   json_t *m;
   json_array_foreach(elements, i, m)
   {
      uint32_t id = static_cast<uint32_t>(json_integer_value(m));
      bool found = false;
      for (int j = 0; j < objects.size(); j++)
      {
         if (id == objects.get(j))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         json_array_remove(elements, i);
         i--;
         setModified();
      }
   }

   for (int j = 0; j < objects.size(); j++)
   {
      bool found = false;

      size_t i;
      json_t *m;
      json_array_foreach(elements, i, m)
      {
         uint32_t id = static_cast<uint32_t>(json_integer_value(m));
         if (id == objects.get(j))
         {
            found = true;
            break;
         }
      }

      if (!found)
      {
         json_array_append_new(elements, json_integer(objects.get(j)));
         setModified();
      }
   }

   if (json_object_get_boolean(config, "useActiveThresholds", false) != useThresholds)
   {
      json_object_set_new(config, "useActiveThresholds", json_boolean(useThresholds));
      setModified();
   }

   if (json_object_get_boolean(config, "useInterfaceUtilization", false) != useLinkUtilization)
   {
      json_object_set_new(config, "useInterfaceUtilization", json_boolean(useLinkUtilization));
      setModified();
   }
}

/**
 * Set color source to script
 *
 * @param scriptName new name of the script
 */
void NetworkMapLinkContainer::setColorSourceToScript(const TCHAR *scriptName)
{
   if (m_link->getColorSource() != MapLinkColorSource::MAP_LINK_COLOR_SOURCE_SCRIPT)
   {
      m_link->setColorSource(MapLinkColorSource::MAP_LINK_COLOR_SOURCE_SCRIPT);
      setModified();
   }

   const TCHAR *newScriptName = scriptName == nullptr ? _T("") : scriptName;
   if (_tcscmp(m_link->getColorProvider(), newScriptName))
   {
      m_link->setColorProvider(newScriptName);
      setModified();
   }
}

/**
 * Set color source to custom color
 *
 * @param newColor new color Java formatted
 */
void NetworkMapLinkContainer::setColorSourceToCustomColor(uint32_t newColor)
{
   if (m_link->getColorSource() != MapLinkColorSource::MAP_LINK_COLOR_SOURCE_CUSTOM_COLOR)
   {
      m_link->setColorSource(MapLinkColorSource::MAP_LINK_COLOR_SOURCE_CUSTOM_COLOR);
      setModified();
   }

   if (m_link->getColor() != newColor)
   {
      m_link->setColor(newColor);
      setModified();
   }
}

/**
 * Update link config form this current class modified config
 */
void NetworkMapLinkContainer::updateConfig()
{
   if (m_config != nullptr)
   {
      char *s = json_dumps(m_config, JSON_COMPACT);
      m_link->setConfig(s);
   }
}

/**
 * Link data source constructor
 */
LinkDataSouce::LinkDataSouce(json_t *config) : m_format(json_object_get_string_utf8(config, "formatString", ""), "utf8"), m_name(json_object_get_string_utf8(config, "name", ""), "utf8")
{
   m_nodeId = json_object_get_uint32(config, "nodeId", 0);
   m_dciId = json_object_get_uint32(config, "dciId", 0);
   m_system = json_object_get_boolean(config, "system", false);
   m_location = LinkLocationFromString(json_object_get_string_utf8(config, "location", ""));
}
