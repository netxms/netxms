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
** File: netmap_element.cpp
**
**/

#include "nxcore.h"

/**************************
 * Network Map Element
 **************************/

/**
 * Generic element default constructor
 */
NetworkMapElement::NetworkMapElement(UINT32 id, UINT32 flags)
{
	m_id = id;
	m_type = MAP_ELEMENT_GENERIC;
	m_posX = 0;
	m_posY = 0;
	m_flags = flags;
}

/**
 * Generic element config constructor
 */
NetworkMapElement::NetworkMapElement(UINT32 id, Config *config, UINT32 flags)
{
	m_id = id;
	m_type = config->getValueAsInt(_T("/type"), MAP_ELEMENT_GENERIC);
	m_posX = config->getValueAsInt(_T("/posX"), 0);
	m_posY = config->getValueAsInt(_T("/posY"), 0);
	m_flags = flags;
}

/**
 * Generic element NXCP constructor
 */
NetworkMapElement::NetworkMapElement(NXCPMessage *msg, UINT32 baseId)
{
	m_id = msg->getFieldAsUInt32(baseId);
	m_type = (LONG)msg->getFieldAsUInt16(baseId + 1);
	m_posX = (LONG)msg->getFieldAsUInt32(baseId + 2);
	m_posY = (LONG)msg->getFieldAsUInt32(baseId + 3);
	m_flags = (LONG)msg->getFieldAsUInt32(baseId + 4);
}

/**
 * Generic element destructor
 */
NetworkMapElement::~NetworkMapElement()
{
}

/**
 * Update element's persistent configuration
 */
void NetworkMapElement::updateConfig(Config *config)
{
	config->setValue(_T("/type"), m_type);
	config->setValue(_T("/posX"), m_posX);
	config->setValue(_T("/posY"), m_posY);
}

/**
 * Fill NXCP message with element's data
 */
void NetworkMapElement::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	msg->setField(baseId, m_id);
	msg->setField(baseId + 1, (WORD)m_type);
	msg->setField(baseId + 2, (UINT32)m_posX);
	msg->setField(baseId + 3, (UINT32)m_posY);
	msg->setField(baseId + 4, m_flags);
}

/**
 * Set element's position
 */
void NetworkMapElement::setPosition(LONG x, LONG y)
{
	m_posX = x;
	m_posY = y;
}

/**
 * Serialize object to JSON
 */
json_t *NetworkMapElement::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "posX", json_integer(m_posX));
   json_object_set_new(root, "posY", json_integer(m_posY));
   json_object_set_new(root, "flags", json_integer(m_flags));
   return root;
}

/**********************
 * Network Map Object
 **********************/

/**
 * Object element default constructor
 */
NetworkMapObject::NetworkMapObject(UINT32 id, UINT32 objectId, UINT32 flags) : NetworkMapElement(id, flags)
{
	m_type = MAP_ELEMENT_OBJECT;
	m_objectId = objectId;
}

/**
 * Object element config constructor
 */
NetworkMapObject::NetworkMapObject(UINT32 id, Config *config, UINT32 flags) : NetworkMapElement(id, config, flags)
{
	m_objectId = config->getValueAsUInt(_T("/objectId"), 0);
}

/**
 * Object element NXCP constructor
 */
NetworkMapObject::NetworkMapObject(NXCPMessage *msg, UINT32 baseId) : NetworkMapElement(msg, baseId)
{
	m_objectId = msg->getFieldAsUInt32(baseId + 10);
}

/**
 * Object element destructor
 */
NetworkMapObject::~NetworkMapObject()
{
}

/**
 * Update element's persistent configuration
 */
void NetworkMapObject::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/objectId"), m_objectId);
}

/**
 * Fill NXCP message with element's data
 */
void NetworkMapObject::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->setField(baseId + 10, m_objectId);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapObject::toJson() const
{
   json_t *root = NetworkMapElement::toJson();
   json_object_set_new(root, "objectId", json_integer(m_objectId));
   return root;
}

/**************************
 * Network Map Decoration
 **************************/

/**
 * Decoration element default constructor
 */
NetworkMapDecoration::NetworkMapDecoration(UINT32 id, LONG decorationType, UINT32 flags) : NetworkMapElement(id, flags)
{
	m_type = MAP_ELEMENT_DECORATION;
	m_decorationType = decorationType;
	m_color = 0;
	m_title = NULL;
	m_width = 50;
	m_height = 20;
}

/**
 * Decoration element config constructor
 */
NetworkMapDecoration::NetworkMapDecoration(UINT32 id, Config *config, UINT32 flags) : NetworkMapElement(id, config, flags)
{
	m_decorationType = config->getValueAsInt(_T("/decorationType"), 0);
	m_color = config->getValueAsUInt(_T("/color"), 0);
	m_title = _tcsdup(config->getValue(_T("/title"), _T("")));
	m_width = config->getValueAsInt(_T("/width"), 0);
	m_height = config->getValueAsInt(_T("/height"), 0);
}

/**
 * Decoration element NXCP constructor
 */
NetworkMapDecoration::NetworkMapDecoration(NXCPMessage *msg, UINT32 baseId) : NetworkMapElement(msg, baseId)
{
	m_decorationType = (LONG)msg->getFieldAsUInt32(baseId + 10);
	m_color = msg->getFieldAsUInt32(baseId + 11);
	m_title = msg->getFieldAsString(baseId + 12);
	m_width = (LONG)msg->getFieldAsUInt32(baseId + 13);
	m_height = (LONG)msg->getFieldAsUInt32(baseId + 14);
}

/**
 * Decoration element destructor
 */
NetworkMapDecoration::~NetworkMapDecoration()
{
	free(m_title);
}

/**
 * Update decoration element's persistent configuration
 */
void NetworkMapDecoration::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/decorationType"), m_decorationType);
	config->setValue(_T("/color"), m_color);
	config->setValue(_T("/title"), CHECK_NULL_EX(m_title));
	config->setValue(_T("/width"), m_width);
	config->setValue(_T("/height"), m_height);
}

/**
 * Fill NXCP message with element's data
 */
void NetworkMapDecoration::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->setField(baseId + 10, (UINT32)m_decorationType);
	msg->setField(baseId + 11, m_color);
	msg->setField(baseId + 12, CHECK_NULL_EX(m_title));
	msg->setField(baseId + 13, (UINT32)m_width);
	msg->setField(baseId + 14, (UINT32)m_height);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapDecoration::toJson() const
{
   json_t *root = NetworkMapElement::toJson();
   json_object_set_new(root, "decorationType", json_integer(m_decorationType));
   json_object_set_new(root, "color", json_integer(m_color));
   json_object_set_new(root, "title", json_string_t(m_title));
   json_object_set_new(root, "width", json_integer(m_width));
   json_object_set_new(root, "height", json_integer(m_height));
   return root;
}

/*****************************
 * Network Map DCI Container
 *****************************/

/**
 * DCI container default constructor
 */
NetworkMapDCIContainer::NetworkMapDCIContainer(UINT32 id, TCHAR* xmlDCIList, UINT32 flags) : NetworkMapElement(id, flags)
{
	m_type = MAP_ELEMENT_DCI_CONTAINER;
   m_xmlDCIList = _tcsdup(xmlDCIList);
}

/**
 * DCI container config constructor
 */
NetworkMapDCIContainer::NetworkMapDCIContainer(UINT32 id, Config *config, UINT32 flags) : NetworkMapElement(id, config, flags)
{
   m_xmlDCIList = _tcsdup(config->getValue(_T("/DCIList"), _T("")));
}

/**
 * DCI container NXCP constructor
 */
NetworkMapDCIContainer::NetworkMapDCIContainer(NXCPMessage *msg, UINT32 baseId) : NetworkMapElement(msg, baseId)
{
   m_xmlDCIList = msg->getFieldAsString(baseId + 10);
}

/**
 * DCI container destructor
 */
NetworkMapDCIContainer::~NetworkMapDCIContainer()
{
   free(m_xmlDCIList);
}

/**
 * Update container's persistent configuration
 */
void NetworkMapDCIContainer::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/DCIList"), m_xmlDCIList);
}

/**
 * Fill NXCP message with container's data
 */
void NetworkMapDCIContainer::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->setField(baseId + 10, m_xmlDCIList);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapDCIContainer::toJson() const
{
   json_t *root = NetworkMapElement::toJson();
   json_object_set_new(root, "xmlDCIList", json_string_t(m_xmlDCIList));
   return root;
}

/**************************
 * Network Map DCI Image
 **************************/

/**
 * DCI image default constructor
 */
NetworkMapDCIImage::NetworkMapDCIImage(UINT32 id, TCHAR* config, UINT32 flags) : NetworkMapElement(id, flags)
{
	m_type = MAP_ELEMENT_DCI_IMAGE;
   m_config = _tcsdup(config);
}

/**
 * DCI image config constructor
 */
NetworkMapDCIImage::NetworkMapDCIImage(UINT32 id, Config *config, UINT32 flags) : NetworkMapElement(id, config, flags)
{
   m_config = _tcsdup(config->getValue(_T("/DCIList"), _T("")));
}

/**
 * DCI image NXCP constructor
 */
NetworkMapDCIImage::NetworkMapDCIImage(NXCPMessage *msg, UINT32 baseId) : NetworkMapElement(msg, baseId)
{
   m_config = msg->getFieldAsString(baseId + 10);
}

/**
 * DCI image destructor
 */
NetworkMapDCIImage::~NetworkMapDCIImage()
{
   free(m_config);
}

/**
 * Update image's persistent configuration
 */
void NetworkMapDCIImage::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/DCIList"), m_config);
}

/**
 * Fill NXCP message with container's data
 */
void NetworkMapDCIImage::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->setField(baseId + 10, m_config);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapDCIImage::toJson() const
{
   json_t *root = NetworkMapElement::toJson();
   json_object_set_new(root, "config", json_string_t(m_config));
   return root;
}

/**************************
 * Network Map Text Box
 **************************/

/**
 * Text Box default constructor
 */
NetworkMapTextBox::NetworkMapTextBox(UINT32 id, TCHAR* config, UINT32 flags) : NetworkMapElement(id, flags)
{
   m_type = MAP_ELEMENT_TEXT_BOX;
   m_config = _tcsdup(config);
}

/**
 * Text Box config constructor
 */
NetworkMapTextBox::NetworkMapTextBox(UINT32 id, Config *config, UINT32 flags) : NetworkMapElement(id, config, flags)
{
   m_config = _tcsdup(config->getValue(_T("/TextBox"), _T("")));
}

/**
 * DCI image NXCP constructor
 */
NetworkMapTextBox::NetworkMapTextBox(NXCPMessage *msg, UINT32 baseId) : NetworkMapElement(msg, baseId)
{
   m_config = msg->getFieldAsString(baseId + 10);
}

/**
 * DCI image destructor
 */
NetworkMapTextBox::~NetworkMapTextBox()
{
   free(m_config);
}

/**
 * Update image's persistent configuration
 */
void NetworkMapTextBox::updateConfig(Config *config)
{
   NetworkMapElement::updateConfig(config);
   config->setValue(_T("/TextBox"), m_config);
}

/**
 * Fill NXCP message with container's data
 */
void NetworkMapTextBox::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   NetworkMapElement::fillMessage(msg, baseId);
   msg->setField(baseId + 10, m_config);
}

/**
 * Serialize to JSON
 */
json_t *NetworkMapTextBox::toJson() const
{
   json_t *root = NetworkMapElement::toJson();
   json_object_set_new(root, "config", json_string_t(m_config));
   return root;
}
