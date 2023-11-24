/*
** NetXMS - Network Management System
** Driver for ELTEX switches
**
** Licensed under MIT lisence, as stated by the original
** author: https://dev.raden.solutions/issues/779#note-4
** 
** Copyright (c) 2015 Procyshin Dmitriy
** Copyright (c) 2023 Raden Solutions
** Copyleft (l) 2023 Anatoly Rudnev 
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
** File: eltex.cpp
**/

#include "eltex.h"
#include <nms_util.h>
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *EltexDriver::getName()
{
	return _T("ELTEX");
}

/**
 * Get driver version
 */
const TCHAR *EltexDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int EltexDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcsncmp(oid, _T(".1.3.6.1.4.1.35265.1"),20) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool EltexDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *EltexDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;


   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      
      if (iface->type == 6)
      {
         iface->location.chassis = 1;
         iface->isPhysicalPort = true;
         iface->location.module = 0;
         iface->location.port = iface->index;
         iface->bridgePort = iface->location.port;
      }
    }

   return ifList;
}


/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool EltexDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Eltex Ltd."));

   TCHAR buffer[256];

 /*
 *  !!! A T T E N T I O N !!!! 
 *  !!!  Eltex stores important informatiom in a Radlan tree !!!
 */


   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.89.53.14.1.4.1")));  // Hardware Version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.89.53.14.1.2.1")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.89.53.14.1.5.1")));  // Serial number




   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
        _tcslcpy(hwInfo->productName,v->getValueAsString(buffer, 256),128);      
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      delete response;
   }
   return true;
}

/**
 * Get orientation of the modules in the device
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int EltexDriver::getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NDD_ORIENTATION_HORIZONTAL;
}

/**
 * Get port layout of given module
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void EltexDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
   layout->numberingScheme = NDD_PN_LR_UD;
   layout->rows = 2;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(EltexDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
