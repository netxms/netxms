/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: ndd.cpp
**/

#include "libnxsrv.h"
#include <nddrv.h>


//
// Constructor
//

NetworkDeviceDriver::NetworkDeviceDriver()
{
}


//
// Destructor
//

NetworkDeviceDriver::~NetworkDeviceDriver()
{
}

/**
 * Get driver name
 */
const TCHAR *NetworkDeviceDriver::getName()
{
	return _T("GENERIC");
}

/**
 * Get driver version
 */
const TCHAR *NetworkDeviceDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device is supported by driver
 *
 * @param oid Device OID
 */
bool NetworkDeviceDriver::isDeviceSupported(const TCHAR *oid)
{
	return true;
}


/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
void NetworkDeviceDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes)
{
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *NetworkDeviceDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes)
{
	return NULL;
}
