/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
#define NDDRV_API_VERSION           3

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
 * Port numbering schemes
 */
enum
{
	NDD_PN_UNKNOWN = 0, // port layout not known to driver
	NDD_PN_CUSTOM = 1,  // custom layout, driver defines location of each port
	NDD_PN_LR_UD = 2,   // left-to-right, then up-down:
	                    //  1 2 3 4
	                    //  5 6 7 8
	NDD_PN_LR_DU = 3,   // left-to-right, then down-up:
	                    //  5 6 7 8
	                    //  1 2 3 4
	NDD_PN_UD_LR = 4,   // up-down, then left-right:
	                    //  1 3 5 7
	                    //  2 4 6 8
	NDD_PN_DU_LR = 5    // down-up, then left-right:
	                    //  2 4 6 8
	                    //  1 3 5 7
};

/**
 * Modules orientation on the switch
 */
enum
{
	NDD_ORIENTATION_HORIZONTAL = 0,
	NDD_ORIENTATION_VERTICAL = 1
};

/**
 * Module layout definition
 */
typedef struct __ndd_module_layout
{
	int rows;					// number of port rows on the module
	int numberingScheme;		// port numbering scheme
	int columns;            // number of columns for custom layout
	WORD portRows[256];     // row numbers for ports
	WORD portColumns[256];  // column numbers for ports
} NDD_MODULE_LAYOUT;

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
	virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, void **driverData);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int useAliases, bool useIfXTable);
	virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes, void *driverData);
	virtual int getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, void *driverData);
	virtual void getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, void *driverData, int module, NDD_MODULE_LAYOUT *layout);
	virtual void destroyDriverData(void *driverData);
};

#endif   /* _nddrv_h_ */
