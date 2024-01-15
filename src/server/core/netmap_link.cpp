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
** File: netmap_link.cpp
**
**/

#include "nxcore.h"
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
   m_config = msg.getFieldAsString(baseId + 11);
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
   m_config = MemCopyString(src.m_config);
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
 * Update link from object link
 */
bool NetworkMapLink::update(const ObjLink& src, bool updateNames)
{
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

   StringBuffer config;
   if ((m_config == nullptr) || (*m_config == 0))
   {
      config.append(_T("<config>\n"));
      config.append(_T("   <routing>0</routing>\n"));
      config.append(_T("   <dciList length=\"0\"/>"));
   }
   else
   {
      config = m_config;
      config.replace(_T("</config>"), _T(""));  // remove trailing tag

      ssize_t startIndex = config.find(_T("<objectStatusList class=\"java.util.ArrayList\"/>"));
      if (startIndex != -1)
      {
         size_t endIndex = startIndex + 47;
         while((startIndex > 0) && _istspace(config.charAt(startIndex - 1)))
            startIndex--;
         while((endIndex < config.length()) && _istspace(config.charAt(endIndex)))
            endIndex++;
         config.removeRange(startIndex, endIndex - startIndex);
      }
      else
      {
         startIndex = config.find(_T("<objectStatusList"));
         if (startIndex != -1)
         {
            while((startIndex > 0) && _istspace(config.charAt(startIndex - 1)))
               startIndex--;
            ssize_t endIndex = config.find(_T("</objectStatusList>"));
            if (endIndex != -1)
            {
               endIndex += 19;
               while((endIndex < static_cast<ssize_t>(config.length())) && _istspace(config.charAt(endIndex)))
                  endIndex++;
               config.removeRange(startIndex, endIndex - startIndex);
            }
         }
      }
   }

   if (src.type == LINK_TYPE_VPN)
   {
      config.append(_T("   <style>3</style>\n")); //Dot lines for VPN
   }

   if (src.portIdCount > 0)
   {
      config.append(_T("\n   <objectStatusList class=\"java.util.ArrayList\">\n"));

      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(src.id1, OBJECT_NODE));
      if (node != nullptr)
      {
         for(int n = 0; n < src.portIdCount; n++)
         {
            shared_ptr<Interface> iface = node->findInterfaceByIndex(src.portIdArray1[n]);
            if (iface != nullptr)
            {
               config.append(_T("      <long>"));
               config.append(iface->getId());
               config.append(_T("</long>\n"));
            }
         }
      }

      node = static_pointer_cast<Node>(FindObjectById(src.id2, OBJECT_NODE));
      if (node != nullptr)
      {
         for(int n = 0; n < src.portIdCount; n++)
         {
            shared_ptr<Interface> iface = node->findInterfaceByIndex(src.portIdArray2[n]);
            if (iface != nullptr)
            {
               config.append(_T("      <long>"));
               config.append(iface->getId());
               config.append(_T("</long>\n"));
            }
         }
      }

      config.append(_T("   </objectStatusList>\n"));
   }
   else
   {
      config.append(_T("\n   <objectStatusList class=\"java.util.ArrayList\"/>\n"));
   }
   config.append(_T("</config>"));

   if (_tcscmp(CHECK_NULL_EX(m_config), config))
   {
      setConfig(config);
      modified = true;
   }

   return modified;
}

/**
 * Swap connector information
 */
void NetworkMapLink::swap()
{
   uint32_t te = m_element1;
   m_element1 = m_element2;
   m_element2 = te;

   TCHAR *tn = m_connectorName1;
   m_connectorName1 = m_connectorName2;
   m_connectorName2 = tn;
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
   msg->setField(baseId + 11, m_config);
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
   json_object_set_new(root, "config", json_string_t(m_config));
   return root;
}

/**
 * Get DCI list
 */
void NetworkMapLink::updateDciList(CountingHashSet<uint32_t>& dciSet, bool addItems) const
{
   if (m_config == nullptr)
      return;
   pugi::xml_document xml;
#ifdef UNICODE
   char *xmlSource = UTF8StringFromWideString(m_config);
#else
   char *xmlSource = MemCopyStringA(config);
#endif
   if (!xml.load_string(xmlSource))
   {
      nxlog_debug_tag(_T("netmap"), 8, _T("NetworkMapLink::getDciList(%d): Failed to load XML"), m_id); //TODO: make debug correct
      MemFree(xmlSource);
      return;
   }
   pugi::xml_node dciList = xml.child("dciList");
   for (pugi::xml_node element : dciList)
   {
      const char *tmp = element.child_value("dciId");
      if (addItems)
         dciSet.put(strtoll(tmp, nullptr, 0));
      else
         dciSet.remove(strtoll(tmp, nullptr, 0));
   }
   MemFree(xmlSource);
}

/**
 * Get config instance or create one
 */
Config *NetworkMapLinkNXSLContainer::getConfigInstance()
{
   if (m_config == nullptr)
   {
      m_config = new Config();
#ifdef UNICODE
      char *xml = UTF8StringFromWideString(m_link->getConfig());
#else
      char *xml = UTF8StringFromMBString(m_link->getConfig());
#endif
      if (!m_config->loadXmlConfigFromMemory(xml, strlen(xml), nullptr, "config", false))
      {
         static const char *defaultConfig =
               "<config>\n" \
               "   <routing>0</routing>\n" \
               "   <dciList length=\"0\"/>\n" \
               "   <objectStatusList class=\"java.util.ArrayList\"/>\n"
               "</config>";
         m_config->loadXmlConfigFromMemory(defaultConfig, strlen(defaultConfig), nullptr, "config", false);
      }
      MemFree(xml);
   }
   return m_config;
}

/**
 * Get color objects array
 */
NXSL_Value *NetworkMapLinkNXSLContainer::getColorObjects(NXSL_VM *vm)
{
   Config *config = getConfigInstance();
   NXSL_Array *array = new NXSL_Array(vm);
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/objectStatusList"), _T("*"));
   for(int i = 0; entries != nullptr && i < entries->size(); i++)
   {
      array->append(vm->createValue(entries->get(i)->getValueAsUInt()));
   }
   return vm->createValue(array);
}

/**
 * Get link data source array
 */
NXSL_Value *NetworkMapLinkNXSLContainer::getDataSource(NXSL_VM *vm)
{
   Config *config = getConfigInstance();
   NXSL_Array *array = new NXSL_Array(vm);
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/dciList"), _T("*"));
   for(int i = 0; entries != nullptr && i < entries->size(); i++)
   {
      LinkDataSouce *dataSolurce = new LinkDataSouce(entries->get(i));
      array->append(vm->createValue(vm->createObject(&g_nxslLinkDataSourceClass, dataSolurce)));
   }
   return vm->createValue(array);
}

/**
 * Update existing or add new data source entry
 *
 * @param dci dci to add
 * @param format format that should be used to display data
 */
void NetworkMapLinkNXSLContainer::updateDataSource(const shared_ptr<DCObjectInfo> &dci, const TCHAR *format)
{
   Config *config = getConfigInstance();
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/dciList"), _T("*"));
   bool found = false;
   for(int i = 0; entries != nullptr && i < entries->size(); i++)
   {
      LinkDataSouce dataSolurce(entries->get(i));
      if(dataSolurce.getDciId() == dci->getId())
      {
         found = true;
         if (!dataSolurce.getFormat().equals(format))
         {
            entries->get(i)->getSubEntries(_T("formatString"))->get(0)->setValue(format);
            setModified();
         }
      }
   }

   if (!found)
   {
      ConfigEntry *dciList = config->getEntry(_T("/dciList"));
      uint32_t currLen = dciList->getAttributeAsInt(_T("length"));
      dciList->setAttribute(_T("length"), ++currLen);
      ConfigEntry *newDciEntry = dciList->createEntry(_T("dci"));
      newDciEntry->setAttribute(_T("nodeId"), dci->getOwnerId());
      newDciEntry->setAttribute(_T("dciId"), dci->getId());
      ConfigEntry *newEntry = newDciEntry->createEntry(_T("type"));
      newEntry->setValue(_T("1"));
      newEntry = newDciEntry->createEntry(_T("name"));
      newEntry->setValue(_T(""));
      newEntry = newDciEntry->createEntry(_T("instance"));
      newEntry->setValue(_T(""));
      newEntry = newDciEntry->createEntry(_T("column"));
      newEntry->setValue(_T(""));
      newEntry = newDciEntry->createEntry(_T("formatString"));
      newEntry->setValue(format);
      setModified();
   }
}

/**
 * Clear data shource list
 */
void NetworkMapLinkNXSLContainer::clearDataSource()
{
   Config *config = getConfigInstance();
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/dciList"), _T("*"));
   if (entries != nullptr || entries->size() != 0)
   {
      ConfigEntry *entry = config->getEntry(_T("/dciList"));
      ConfigEntry *parent = entry->getParent();
      parent->unlinkEntry(entry);
      delete entry;
      entry = parent->createEntry(_T("dciList"));
      entry->setAttribute(_T("length"), 0);
      setModified();
   }
}

/**
 * Remove data source from list by index
 *
 * @param index index of data source to remove
 */
void NetworkMapLinkNXSLContainer::removeDataSource(uint32_t index)
{
   Config *config = getConfigInstance();
   unique_ptr<ObjectArray<ConfigEntry>> entries = config->getSubEntries(_T("/dciList"), _T("*"));
   if ((entries != nullptr) && (static_cast<uint32_t>(entries->size()) > index))
   {
      ConfigEntry *dciList = config->getEntry(_T("/dciList"));
      uint32_t currLen = dciList->getAttributeAsInt(_T("length"));
      dciList->setAttribute(_T("length"), --currLen);
      config->getEntry(_T("/dciList"))->unlinkEntry(entries->get(index));
      delete entries->get(index);
      setModified();
   }
}

/**
 * Set routing algorithm
 *
 * @param algorithm new algorithm should be between 0 and 3, other values are ignored
 */
void NetworkMapLinkNXSLContainer::setRoutingAlgorithm(uint32_t algorithm)
{
   if (algorithm > 3)
      return;

   Config *config = getConfigInstance();
   if (config->getValueAsUInt(_T("/routing"), 0) != algorithm)
   {
      ConfigEntry *routing = config->getEntry(_T("/routing"));
      TCHAR buffer[64];
      _sntprintf(buffer, 64, _T("%u"), (unsigned int)algorithm);
      routing->setValue(buffer);
      setModified();
   }
}

/**
 * Set width
 *
 * @param width new width should be bigger or equals to 0
 */
void NetworkMapLinkNXSLContainer::setWidth(uint32_t width)
{
   Config *config = getConfigInstance();
   if (config->getValueAsUInt(_T("/width"), 0) != width)
   {
      ConfigEntry *routing = config->getEntry(_T("/width"));
      TCHAR buffer[64];
      _sntprintf(buffer, 64, _T("%u"), (unsigned int)width);
      routing->setValue(buffer);
      setModified();
   }
}


/**
 * Set routing algorithm
 *
 * @param style new style should be between 0 and 5, other values are ignored
 */
void NetworkMapLinkNXSLContainer::setStyle(uint32_t style)
{
   if (style > ((uint32_t)MapLinkStyle::DashDotDot))
      return;

   Config *config = getConfigInstance();
   if (config->getValueAsUInt(_T("/style"), 0) != style)
   {
      ConfigEntry *routing = config->getEntry(_T("/style"));
      TCHAR buffer[64];
      _sntprintf(buffer, 64, _T("%u"), (unsigned int)style);
      routing->setValue(buffer);
      setModified();
   }
}

/**
 * Set color source to specific value
 */
void NetworkMapLinkNXSLContainer::setColorSource(MapLinkColorSource source)
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
void NetworkMapLinkNXSLContainer::setColorSourceToObjectStatus(const IntegerArray<uint32_t>& objects, bool useThresholds, bool useLinkUtilization)
{
   if (m_link->getColorSource() != MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
   {
      m_link->setColorSource(MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS);
      setModified();
   }

   Config *config = getConfigInstance();
   unique_ptr<ObjectArray<ConfigEntry>> entryListToRemove = config->getSubEntries(_T("/objectStatusList"), _T("*"));
   for(int i = 0; entryListToRemove != nullptr && i < entryListToRemove->size(); i++)
   {
      const TCHAR* value = entryListToRemove->get(i)->getValue();
      uint32_t id = (value != nullptr) ? _tcstoul(value, nullptr, 0) : 0;
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
         ConfigEntry *parent = entryListToRemove->get(i)->getParent();
         if (parent == nullptr)  // root entry
            return;

         parent->unlinkEntry(entryListToRemove->get(i));
         delete entryListToRemove->get(i);
         setModified();
      }
   }

   unique_ptr<ObjectArray<ConfigEntry>> entryListToAdd = config->getSubEntries(_T("/objectStatusList"), _T("*"));
   for (int j = 0; j < objects.size(); j++)
   {
      bool found = false;
      for(int i = 0; entryListToAdd != nullptr && i < entryListToAdd->size(); i++)
      {
         const TCHAR* value = entryListToAdd->get(i)->getValue();
         uint32_t id = (value != nullptr) ? _tcstoul(value, nullptr, 0) : 0;
         if (id == objects.get(j))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         ConfigEntry *parent = config->getEntry(_T("/objectStatusList"));
         ConfigEntry *newConfigEntry = new ConfigEntry(_T("long"), parent, config, _T("<memory>"), 0, 0);
         TCHAR buffer[64];
         IntegerToString(objects.get(j), buffer);
         newConfigEntry->setValue(buffer);
         setModified();
      }
   }

   if (config->getValueAsBoolean(_T("/useActiveThresholds"), false) != useThresholds)
   {
      config->setValue(_T("/useActiveThresholds"), useThresholds ? _T("true") : _T("false"));
      setModified();
   }

   if (config->getValueAsBoolean(_T("/useInterfaceUtilization"), false) != useLinkUtilization)
   {
      config->setValue(_T("/useInterfaceUtilization"), useLinkUtilization ? _T("true") : _T("false"));
      setModified();
   }
}

/**
 * Set color source to script
 *
 * @param scriptName new name of the script
 */
void NetworkMapLinkNXSLContainer::setColorSourceToScript(const TCHAR *scriptName)
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
void NetworkMapLinkNXSLContainer::setColorSourceToCustomColor(uint32_t newColor)
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
void NetworkMapLinkNXSLContainer::updateConfig()
{
   if (m_config != nullptr)
   {
      m_link->setConfig(m_config->createXml());
   }
}

/**
 * Link data source constructor
 */
LinkDataSouce::LinkDataSouce(ConfigEntry *config) : m_format(config->getSubEntryValue(_T("formatString")))
{
   m_nodeId = config->getAttributeAsUInt(_T("nodeId"));
   m_dciId = config->getAttributeAsUInt(_T("dciId"));
}
