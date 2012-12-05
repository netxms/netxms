/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: mobile.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
MobileDevice::MobileDevice()
{
	m_lastReportTime = 0;
	m_deviceId = NULL;
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_osName = NULL;
	m_osVersion = NULL;
	m_userId = NULL;
}

/**
 * Constructor for creating new mobile device object
 */
MobileDevice::MobileDevice(const TCHAR *deviceId)
{
	m_lastReportTime = 0;
	m_deviceId = _tcsdup(deviceId);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_osName = NULL;
	m_osVersion = NULL;
	m_userId = NULL;
}

/**
 * Destructor
 */
MobileDevice::~MobileDevice()
{
	safe_free(m_deviceId);
	safe_free(m_vendor);
	safe_free(m_model);
	safe_free(m_serialNumber);
	safe_free(m_osName);
	safe_free(m_osVersion);
	safe_free(m_userId);
}
