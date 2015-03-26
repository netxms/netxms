/* 
** NetXMS - Network Management System
** Driver for Qtech OLT switches
**
** Licensed under MIT lisence, as stated by the original
** author: https://dev.raden.solutions/issues/779#note-4
** 
** Copyright (C) 2015 Procyshin Dmitriy
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy of
** this software and associated documentation files (the “Software”), to deal in
** the Software without restriction, including without limitation the rights to
** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
** of the Software, and to permit persons to whom the Software is furnished to do
** so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
** File: qtech-olt.h
**
**/

#ifndef _qtecholt_h_
#define _qtecholt_h_

#include <nddrv.h>

/**
 * Driver's class
 */
class QtechOLTDriver : public NetworkDeviceDriver
{
public:
	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();
	virtual int isPotentialDevice(const TCHAR *oid);
	virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
	virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable);
   virtual void getInterfaceState(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, UINT32 ifIndex, InterfaceAdminState *adminState, InterfaceOperState *operState);
};

#endif
