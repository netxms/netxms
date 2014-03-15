/* 
** NetXMS - Network Management System
** Driver for HP ProCurve switches
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
** File: procurve.h
**
**/

#ifndef _procurve_h_
#define _procurve_h_

#include <nddrv.h>


/**
 * Driver's class
 */
class ProCurveDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual int isPotentialDevice(const TCHAR *oid);
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
	virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
};

#endif
