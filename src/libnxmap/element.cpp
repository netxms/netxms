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


//
// Generic element default constructor
//

NetworkMapElement::NetworkMapElement()
{
	m_type = MAP_ELEMENT_GENERIC;
	m_posX = 0;
	m_posY = 0;
}


//
// Generic element config constructor
//

NetworkMapElement::NetworkMapElement(Config *config)
{
	m_type = config->getValueInt(_T("/type"), MAP_ELEMENT_GENERIC);
	m_posX = config->getValueInt(_T("/posX"), 0);
	m_posY = config->getValueInt(_T("/posY"), 0);
}


//
// Generic element destructor
//

NetworkMapElement::~NetworkMapElement()
{
}


//
// Update element's persistent configuration
//

void NetworkMapElement::updateConfig(Config *config)
{
	config->setValue(_T("/type"), m_type);
	config->setValue(_T("/posX"), m_posX);
	config->setValue(_T("/posY"), m_posY);
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

NetworkMapObject::NetworkMapObject(DWORD objectId) : NetworkMapElement()
{
	m_type = MAP_ELEMENT_OBJECT;
	m_objectId = objectId;
}


//
// Object element config constructor
//

NetworkMapObject::NetworkMapObject(Config *config) : NetworkMapElement(config)
{
	m_objectId = config->getValueUInt(_T("/objectId"), 0);
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
// Decoration element default constructor
//

NetworkMapDecoration::NetworkMapDecoration(LONG decorationType) : NetworkMapElement()
{
	m_type = MAP_ELEMENT_DECORATION;
	m_decorationType = decorationType;
	m_color = 0;
	m_title = NULL;
}


//
// Decoration element config constructor
//

NetworkMapDecoration::NetworkMapDecoration(Config *config) : NetworkMapElement(config)
{
	m_decorationType = config->getValueInt(_T("/decorationType"), 0);
	m_color = config->getValueInt(_T("/color"), 0);
	m_title = _tcsdup(config->getValue(_T("/title"), _T("")));
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
}
