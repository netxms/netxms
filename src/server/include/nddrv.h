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
#define NDDRV_API_VERSION           4

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
 * Cluster modes
 */
enum
{
   CLUSTER_MODE_UNKNOWN = -1,
   CLUSTER_MODE_STANDALONE = 0,
   CLUSTER_MODE_ACTIVE = 1,
   CLUSTER_MODE_STANDBY = 2
};


/**
 * Access point state
 */
enum AccessPointState
{
   AP_ADOPTED = 0,
   AP_UNADOPTED = 1,
   AP_DOWN = 2
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
 * Radio interface information
 */
struct RadioInterfaceInfo
{
	int index;
	TCHAR name[64];
	BYTE macAddr[MAC_ADDR_LENGTH];
   UINT32 channel;
   INT32 powerDBm;
   INT32 powerMW;
};

/**
 * Wireless access point information
 */
class LIBNXSRV_EXPORTABLE AccessPointInfo
{
private:
   BYTE m_macAddr[MAC_ADDR_LENGTH];
   UINT32 m_ipAddr;
   AccessPointState m_state;
   TCHAR *m_name;
   TCHAR *m_vendor;
   TCHAR *m_model;
   TCHAR *m_serial;
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;

public:
   AccessPointInfo(BYTE *macAddr, UINT32 ipAddr, AccessPointState state, const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial);
   ~AccessPointInfo();

	void addRadioInterface(RadioInterfaceInfo *iface);

	BYTE *getMacAddr() { return m_macAddr; }
   UINT32 getIpAddr() { return m_ipAddr; }
	AccessPointState getState() { return m_state; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getVendor() { return m_vendor; }
	const TCHAR *getModel() { return m_model; }
	const TCHAR *getSerial() { return m_serial; }
	ObjectArray<RadioInterfaceInfo> *getRadioInterfaces() { return m_radioInterfaces; }
};

/**
 * Wireless station AP match policy
 */
#define AP_MATCH_BY_RFINDEX   0
#define AP_MATCH_BY_BSSID     1

/**
 * Wireless station information
 */
struct WirelessStationInfo
{
	// This part filled by driver
   BYTE macAddr[MAC_ADDR_LENGTH];
	UINT32 ipAddr;	// IP address, must be in host byte order
	int rfIndex;	// radio interface index
   BYTE bssid[MAC_ADDR_LENGTH];
   short apMatchPolicy;
	TCHAR ssid[MAX_OBJECT_NAME];
   int vlan;
   int signalStrength;
   UINT32 txRate;
   UINT32 rxRate;

	// This part filled by core
	UINT32 apObjectId;
	UINT32 nodeId;
   TCHAR rfName[MAX_OBJECT_NAME];
};

/**
 * Base class for driver data
 */
class LIBNXSRV_EXPORTABLE DriverData
{
public:
   DriverData();
   virtual ~DriverData();
};

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

   virtual const TCHAR *getCustomTestOID();
   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
   virtual VlanList *getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual int getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual void getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout);
   virtual bool isPerVlanFdbSupported();
   virtual int getClusterMode(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual bool isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData);
};

#endif   /* _nddrv_h_ */
