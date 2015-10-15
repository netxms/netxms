/* 
** NetXMS - Network Management System
** NetXMS Message Bus Library
** Copyright (C) 2009 Victor Kirhenshtein
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
** File: message.cpp
**
**/

#include "libnxmb.h"

/**
 * Default constructor
 */
NXMBMessage::NXMBMessage()
{
	m_type = _tcsdup(_T("NONE"));
	m_senderId = _tcsdup(_T("UNKNOWN"));
}

/**
 * Create message with type and sender information
 */
NXMBMessage::NXMBMessage(const TCHAR *type, const TCHAR *senderId)
{
	m_type = _tcsdup(CHECK_NULL(type));
	m_senderId = _tcsdup(CHECK_NULL(senderId));
}

/**
 * Destructor
 */
NXMBMessage::~NXMBMessage()
{
	safe_free(m_type);
	safe_free(m_senderId);
}
