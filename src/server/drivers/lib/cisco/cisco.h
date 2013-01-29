/* 
** NetXMS - Network Management System
** Generic driver for Cisco devices
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: cisco.h
**
**/

#ifndef _cisco_h_
#define _cisco_h_

#include <nddrv.h>

#ifdef _WIN32
#ifdef CISCO_EXPORTS
#define CISCO_EXPORTABLE __declspec(dllexport)
#else
#define CISCO_EXPORTABLE __declspec(dllimport)
#endif
#else
#define CISCO_EXPORTABLE
#endif


/**
 * Driver's class
 */
class CISCO_EXPORTABLE CiscoDeviceDriver : public NetworkDeviceDriver
{
public:
	virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes, void *driverData);
};

#endif
