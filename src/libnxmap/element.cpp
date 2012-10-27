/* 
** NetXMS - Network Management System
** Network Maps Library
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
** File: element.cpp
**
**/

#include "libnxmap.h"

/**
 * Generic element default constructor
 */
NetworkMapElement::NetworkMapElement(DWORD id)
{
	m_id = id;
	m_type = MAP_ELEMENT_GENERIC;
	m_posX = 0;
	m_posY = 0;
}

/**
 * Generic element config constructor
 */
NetworkMapElement::NetworkMapElement(DWORD id, Config *config)
{
	m_id = id;
	m_type = config->getValueInt(_T("/type"), MAP_ELEMENT_GENERIC);
	m_posX = config->getValueInt(_T("/posX"), 0);
	m_posY = config->getValueInt(_T("/posY"), 0);
}

/**
 * Generic element NXCP constructor
 */
NetworkMapElement::NetworkMapElement(CSCPMessage *msg, DWORD baseId)
{
	m_id = msg->GetVariableLong(baseId);
	m_type = (LONG)msg->GetVariableShort(baseId + 1);
	m_posX = (LONG)msg->GetVariableLong(baseId + 2);
	m_posY = (LONG)msg->GetVariableLong(baseId + 3);
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


//
// Fill NXCP message with element's data
//

void NetworkMapElement::fillMessage(CSCPMessage *msg, DWORD baseId)
{
	msg->SetVariable(baseId, m_id);
	msg->SetVariable(baseId + 1, (WORD)m_type);
	msg->SetVariable(baseId + 2, (DWORD)m_posX);
	msg->SetVariable(baseId + 3, (DWORD)m_posY);
}


//
// Set element's position
//

void NetworkMapElement::setPosition(LONG x, LONG y)
{
	m_posX = x;
	m_posY = y;
}


//
// Object element default constructor
//

NetworkMapObject::NetworkMapObject(DWORD id, DWORD objectId) : NetworkMapElement(id)
{
	m_type = MAP_ELEMENT_OBJECT;
	m_objectId = objectId;
}


//
// Object element config constructor
//

NetworkMapObject::NetworkMapObject(DWORD id, Config *config) : NetworkMapElement(id, config)
{
	m_objectId = config->getValueUInt(_T("/objectId"), 0);
}


//
// Object element NXCP constructor
//

NetworkMapObject::NetworkMapObject(CSCPMessage *msg, DWORD baseId) : NetworkMapElement(msg, baseId)
{
	m_objectId = msg->GetVariableLong(baseId + 10);
}


//
// Object element destructor
//

NetworkMapObject::~NetworkMapObject()
{
}


//
// Update element's persistent configuration
//

void NetworkMapObject::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/objectId"), m_objectId);
}


//
// Fill NXCP message with element's data
//

void NetworkMapObject::fillMessage(CSCPMessage *msg, DWORD baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->SetVariable(baseId + 10, m_objectId);
}


//
// Decoration element default constructor
//

NetworkMapDecoration::NetworkMapDecoration(DWORD id, LONG decorationType) : NetworkMapElement(id)
{
	m_type = MAP_ELEMENT_DECORATION;
	m_decorationType = decorationType;
	m_color = 0;
	m_title = NULL;
	m_width = 50;
	m_height = 20;
}


//
// Decoration element config constructor
//

NetworkMapDecoration::NetworkMapDecoration(DWORD id, Config *config) : NetworkMapElement(id, config)
{
	m_decorationType = config->getValueInt(_T("/decorationType"), 0);
	m_color = config->getValueUInt(_T("/color"), 0);
	m_title = _tcsdup(config->getValue(_T("/title"), _T("")));
	m_width = config->getValueInt(_T("/width"), 0);
	m_height = config->getValueInt(_T("/height"), 0);
}


//
// Decoration element NXCP constructor
//

NetworkMapDecoration::NetworkMapDecoration(CSCPMessage *msg, DWORD baseId) : NetworkMapElement(msg, baseId)
{
	m_decorationType = (LONG)msg->GetVariableLong(baseId + 10);
	m_color = msg->GetVariableLong(baseId + 11);
	m_title = msg->GetVariableStr(baseId + 12);
	m_width = (LONG)msg->GetVariableLong(baseId + 13);
	m_height = (LONG)msg->GetVariableLong(baseId + 14);
}


//
// Decoration element destructor
//

NetworkMapDecoration::~NetworkMapDecoration()
{
	safe_free(m_title);
}


//
// Update decoration element's persistent configuration
//

void NetworkMapDecoration::updateConfig(Config *config)
{
	NetworkMapElement::updateConfig(config);
	config->setValue(_T("/decorationType"), m_decorationType);
	config->setValue(_T("/color"), m_color);
	config->setValue(_T("/title"), CHECK_NULL_EX(m_title));
	config->setValue(_T("/width"), m_width);
	config->setValue(_T("/height"), m_height);
}


//
// Fill NXCP message with element's data
//

void NetworkMapDecoration::fillMessage(CSCPMessage *msg, DWORD baseId)
{
	NetworkMapElement::fillMessage(msg, baseId);
	msg->SetVariable(baseId + 10, (DWORD)m_decorationType);
	msg->SetVariable(baseId + 11, m_color);
	msg->SetVariable(baseId + 12, CHECK_NULL_EX(m_title));
	msg->SetVariable(baseId + 13, (DWORD)m_width);
	msg->SetVariable(baseId + 14, (DWORD)m_height);
}
