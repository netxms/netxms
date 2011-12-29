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
** File: nddrv.h
**
**/

#ifndef _nddrv_h_
#define _nddrv_h_

#include <nms_common.h>
#include <nxsrvapi.h>


/**
 *API version
 */
#define NDDRV_API_VERSION           1

/**
 * Driver header
 */
#ifdef _WIN32
#define __NDD_EXPORT __declspec(dllexport)
#else
#define __NDD_EXPORT
#endif

#define DECLARE_NDD_ENTRY_POINT(name, implClass) \
extern "C" int __NDD_EXPORT nddAPIVersion; \
extern "C" const TCHAR __NDD_EXPORT *nddName; \
int __NDD_EXPORT nddAPIVersion = NDDRV_API_VERSION; \
const TCHAR __NDD_EXPORT *nddName = name; \
extern "C" NetworkDeviceDriver __NDD_EXPORT *nddCreateInstance() { return new implClass; }


/**
 * Base class for device drivers
 */
class LIBNXSRV_EXPORTABLE NetworkDeviceDriver
{
public:
	NetworkDeviceDriver();
	virtual ~NetworkDeviceDriver();

	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual int isPotentialDevice(const TCHAR *oid);
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
	virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, int useAliases, bool useIfXTable);
	virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes);
};


#endif   /* _nddrv_h_ */
