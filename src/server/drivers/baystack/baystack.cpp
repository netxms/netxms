/* 
** NetXMS - Network Management System
** Driver for Avaya ERS switches (except ERS 8xxx) (former Nortel/Bay Networks BayStack)
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
** File: baystack.cpp
**/

#include <nddrv.h>


//
// Static data
//

static TCHAR s_driverName[] = _T("BAYSTACK");
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;


//
// Driver's class
//

class BayStackDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

	virtual bool isDeviceSupported(const TCHAR *oid);
};


//
// Get name
//

const TCHAR *BayStackDriver::getName()
{
	return s_driverName;
}


//
// Get version
//

const TCHAR *BayStackDriver::getVersion()
{
	return s_driverVersion;
}


//
// Check if given OID is supported
//

bool BayStackDriver::isDeviceSupported(const TCHAR *oid)
{
	return _tcsncmp(oid, _T(".1.3.6.1.4.1.45.3"), 17) == 0;
}


//
// Driver entry point
//

DECLARE_NDD_ENTRY_POINT(s_driverName, BayStackDriver);


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
