/* 
** NetXMS - Network Management System
** Driver for Allied Telesis switches
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: at.cpp
**/

#include "at.h"


/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("AT");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *AlliedTelesisDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *AlliedTelesisDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int AlliedTelesisDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.207.1"), 18) == 0) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool AlliedTelesisDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   TCHAR buffer[256];
   return (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.8.33.1.1.0"), NULL, 0, buffer, 256, 0) == SNMP_ERR_SUCCESS) ||
          (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.1.4.167.81.1.3.0"), NULL, 0, buffer, 256, 0) == SNMP_ERR_SUCCESS);
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes and driver's data from within this method.
 * Driver is responsible for destroying previously created data object.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData pointer to pointer to driver-specific data
 */
void AlliedTelesisDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData)
{
   TCHAR buffer[256];
   if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.207.1.4.167.81.1.3.0"), NULL, 0, buffer, 256, 0) == SNMP_ERR_SUCCESS)
   {
      attributes->set(_T(".alliedTelesis.isGS950"), _T("true"));
   }
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *AlliedTelesisDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, attributes, driverData, useAliases, useIfXTable);
   if (ifList != NULL)
   {
      bool isGS950 = attributes->getBoolean(_T(".alliedTelesis.isGS950"), false);

      // Find physical ports
      for(int i = 0; i < ifList->size(); i++)
      {
         InterfaceInfo *iface = ifList->get(i);
         if (isGS950)
         {
            // GS950 does not support atiIfExtnTable so we use ifConnectorPresent from ifXTable
            // to identify physical ports
            TCHAR oid[256];
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.31.1.1.1.17.%d"), iface->index); // ifConnectorPresent
            UINT32 cp;
            if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &cp, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
            {
               if (cp == 1)   // 1 == true
               {
                  iface->isPhysicalPort = true;
                  iface->slot = 0;
                  iface->port = iface->index;
               }
            }
         }
         else
         {
            SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

            TCHAR oid[256];
            _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.9.1.1.1.2.%d"), iface->index);  // module
            request->bindVariable(new SNMP_Variable(oid));
            _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.9.1.1.1.3.%d"), iface->index);  // port
            request->bindVariable(new SNMP_Variable(oid));

            SNMP_PDU *response;
            UINT32 rcc = snmp->doRequest(request, &response, SnmpGetDefaultTimeout(), 3);
	         delete request;
            if (rcc == SNMP_ERR_SUCCESS)
            {
               if ((response->getNumVariables() == 2) && 
                   (response->getVariable(0)->getType() != ASN_NO_SUCH_OBJECT) &&
                   (response->getVariable(0)->getType() != ASN_NO_SUCH_INSTANCE) &&
                   (response->getVariable(1)->getType() != ASN_NO_SUCH_OBJECT) &&
                   (response->getVariable(1)->getType() != ASN_NO_SUCH_INSTANCE))
               {
                  iface->slot = response->getVariable(0)->getValueAsInt();
                  iface->port = response->getVariable(1)->getValueAsInt();
                  iface->isPhysicalPort = true;
               }
               delete response;
            }
         }
      }
   }
	return ifList;
}

/**
 * Handler for VLAN enumeration
 */
static UINT32 HandlerVlanList(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

	VlanInfo *vlan = new VlanInfo(var->getName().getElement(var->getName().length() - 1), VLAN_PRM_SLOTPORT);

	TCHAR buffer[256];
	vlan->setName(var->getValueAsString(buffer, 256));
	vlanList->add(vlan);

   return SNMP_ERR_SUCCESS;
}

/**
 * Parse port list
 * Format of the input string would be like '1,2,5,7,12..15,18-22,26'
 */
static void ParsePortList(TCHAR *ports, VlanInfo *vlan, UINT32 slot)
{
   TCHAR *curr = ports, *next;

   do
   {
      next = _tcschr(curr, _T(','));
      if (next != NULL)
         *next = 0;

      TCHAR *ptr = _tcschr(curr, _T('-'));
      if (ptr == NULL)
         ptr = _tcschr(curr, _T('.'));
      if (ptr != NULL)
      {
         *ptr = 0;
         ptr++;
         if (*ptr == _T('.')) // handle n..m case
            ptr++;
         UINT32 start = _tcstoul(curr, NULL, 10);
         UINT32 end = _tcstoul(ptr, NULL, 10);
         for(UINT32 p = start; p <= end; p++)
            vlan->add(slot, p);
      }
      else
      {
         UINT32 port = _tcstoul(curr, NULL, 10);
         vlan->add(slot, port);
      }

      curr = next + 1;
   } while(next != NULL);
}

/**
 * Get VLANs 
 */
VlanList *AlliedTelesisDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   VlanList *list = NetworkDeviceDriver::getVlans(snmp, attributes, driverData);
   if ((list != NULL) && (list->size() > 0))
      return list;   // retrieved from standard MIBs

   if (list == NULL)
      list = new VlanList();

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.207.8.33.8.1.1.2"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
		goto failure;

   for(int i = 0; i < list->size(); i++)
   {
      VlanInfo *vlan = list->get(i);

      SNMP_PDU *request = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

      TCHAR oid[256];
      _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.8.1.1.5.%d"), vlan->getVlanId()); // tagged ports
      request->bindVariable(new SNMP_Variable(oid));
      _sntprintf(oid, 256, _T(".1.3.6.1.4.1.207.8.33.8.1.1.4.%d"), vlan->getVlanId()); // untagged ports
      request->bindVariable(new SNMP_Variable(oid));
      
      SNMP_PDU *response;
      if (snmp->doRequest(request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
      {
         if ((response->getNumVariables() == 2) && 
             (response->getVariable(0)->getType() != ASN_NO_SUCH_OBJECT) &&
             (response->getVariable(0)->getType() != ASN_NO_SUCH_INSTANCE) &&
             (response->getVariable(1)->getType() != ASN_NO_SUCH_OBJECT) &&
             (response->getVariable(1)->getType() != ASN_NO_SUCH_INSTANCE))
         {
            TCHAR buffer[1024];
            ParsePortList(response->getVariable(0)->getValueAsString(buffer, 1024), vlan, 1);
            ParsePortList(response->getVariable(1)->getValueAsString(buffer, 1024), vlan, 1);
         }
         delete response;
      }
      delete request;
   }

   return list;

failure:
	delete list;
	return NULL;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, AlliedTelesisDriver);

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
