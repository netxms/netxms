/*
 ** NetXMS - Network Management System
 ** Driver for TP-Link switches
 **
 ** Licensed under MIT lisence, as stated by the original
 ** author: https://dev.raden.solutions/issues/779#note-4
 **
 ** Copyright (C) 2015 Procyshin Dmitriy
 ** Copyleft (L) 2023 Anatoly Rudnev

 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy
 of
 ** this software and associated documentation files (the “Software”), to deal in
 ** the Software without restriction, including without limitation the rights to
 ** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 ** of the Software, and to permit persons to whom the Software is furnished to
 do
 ** so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in
 all
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
 ** File: tplink.h
 **
 **/

#ifndef _tplink_h_
#define _tplink_h_

#include <nddrv.h>

/**
 * Driver's class
 */
class TPLinkDriver : public NetworkDeviceDriver
{
public:
   virtual const TCHAR* getName() override;
   virtual const TCHAR* getVersion() override;

   virtual int isPotentialDevice(const SNMP_ObjectId& oid) override;
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid) override;
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo) override;
   virtual InterfaceList* getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable) override;
   virtual bool isValidLldpRemLocalPortNum(const NObject *node, DriverData *driverData) override;
   virtual VlanList* getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData) override;
};

#endif
