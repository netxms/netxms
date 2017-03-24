/* 
** NetXMS - Network Management System
** NetXMS Message Bus Library
** Copyright (C) 2009-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: filter.cpp
**
**/

#include "libnxmb.h"


//
// Basic filter class implementation
//

NXMBFilter::NXMBFilter()
{
}

NXMBFilter::~NXMBFilter()
{
}

bool NXMBFilter::isAllowed(NXMBMessage &msg)
{ 
	return true;
}

bool NXMBFilter::isOwnedByDispatcher()
{
	return true;
}

/**
 * Filter by message type implementation
 */
NXMBTypeFilter::NXMBTypeFilter()
               :NXMBFilter()
{
}

NXMBTypeFilter::~NXMBTypeFilter()
{
}

bool NXMBTypeFilter::isAllowed(NXMBMessage &msg)
{ 
	return m_types.get(msg.getType()) != NULL;
}

void NXMBTypeFilter::addMessageType(const TCHAR *type)
{ 
	m_types.set(type, _T("*"));
}

void NXMBTypeFilter::removeMessageType(const TCHAR *type)
{ 
	m_types.remove(type);
}
