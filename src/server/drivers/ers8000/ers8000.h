/* 
** NetXMS - Network Management System
** Driver for Avaya ERS 8xxx switches (former Nortel/Bay Networks Passport)
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: ers8000.h
**
**/

#ifndef _ers8000_h_
#define _ers8000_h_

#include <nddrv.h>


/**
 * Driver's class
 */
class PassportDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual bool isDeviceSupported(const TCHAR *oid);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes);
};

/**
 * Functions
 */
void GetVLANInterfaces(SNMP_Transport *pTransport, InterfaceList *pIfList);

#endif
