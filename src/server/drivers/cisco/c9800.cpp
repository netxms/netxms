/*
** NetXMS - Network Management System
** Driver for Cisco Catalyst 9800 (IOS-XE) wireless controllers
** Copyright (C) 2026 Raden Solutions
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
** File: c9800.cpp
**/

#include "cisco.h"
#include <map>

#define DEBUG_TAG DEBUG_TAG_CISCO _T(".c9800")

/**
 * Get driver name
 */
const TCHAR *CiscoC9800Driver::getName()
{
   return _T("CISCO-C9800");
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int CiscoC9800Driver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 9, 1 }) ? 200 : 0;
}

/**
 * Read sysDescr from device
 */
static bool ReadSysDescr(SNMP_Transport *snmp, TCHAR *buffer, size_t size)
{
   return SnmpGetEx(snmp, { 1, 3, 6, 1, 2, 1, 1, 1, 0 }, buffer, size, SG_STRING_RESULT) == SNMP_ERR_SUCCESS;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool CiscoC9800Driver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   // sysDescr on Catalyst 9800 looks like "Cisco IOS Software [IOSXE], C9800 Software (C9800_IOSXE-K9), Version 17.15.4d, RELEASE SOFTWARE (fc4)..."
   // (or "C9800-CL Software", "C9800-L Software")
   TCHAR sysDescr[1024];
   if (!ReadSysDescr(snmp, sysDescr, sizeof(sysDescr)))
      return false;
   nxlog_debug_tag(DEBUG_TAG, 5, _T("sysDescr=\"%s\""), sysDescr);
   return (_tcsstr(sysDescr, _T("Cisco IOS")) != nullptr) && (_tcsstr(sysDescr, _T("C9800")) != nullptr);
}

/**
 * Get device virtualization type.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param vtype pointer to virtualization type enum to fill
 * @return true if virtualization type is known
 */
bool CiscoC9800Driver::getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype)
{
   TCHAR sysDescr[1024];
   if (!ReadSysDescr(snmp, sysDescr, sizeof(sysDescr)))
      return false;
   *vtype = (_tcsstr(sysDescr, _T("C9800-CL")) != nullptr) ? VTYPE_FULL : VTYPE_NONE;
   return true;
}

/**
 * Get cluster mode for device (standalone / active / standby) from CISCO-RF-MIB (HA SSO)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
int CiscoC9800Driver::getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 9, 9, 176, 1, 1, 2, 0 }));  // cRFStatusUnitState
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 9, 9, 176, 1, 1, 4, 0 }));  // cRFStatusPeerUnitState

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return CLUSTER_MODE_STANDALONE;

   int mode = CLUSTER_MODE_STANDALONE;
   if (response->getNumVariables() == 2)
   {
      const SNMP_Variable *unitState = response->getVariable(0);
      const SNMP_Variable *peerState = response->getVariable(1);
      if ((unitState->getType() == ASN_INTEGER) && (peerState->getType() == ASN_INTEGER))
      {
         // RFState: notKnown(1), disabled(2), initialization(3), negotiation(4), standbyCold(5) ... standbyHot(9),
         // activeFast(10), activeDrain(11), activePreconfig(12), activePostconfig(13), active(14), activeExtraload(15), activeHandback(16)
         int unit = unitState->getValueAsInt();
         int peer = peerState->getValueAsInt();
         nxlog_debug_tag(DEBUG_TAG, 6, _T("cRFStatusUnitState=%d cRFStatusPeerUnitState=%d"), unit, peer);
         if (peer > 2)   // peer unit present
         {
            if (unit >= 10)
               mode = CLUSTER_MODE_ACTIVE;
            else if (unit >= 3)
               mode = CLUSTER_MODE_STANDBY;
         }
      }
   }
   delete response;
   return mode;
}

/**
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool CiscoC9800Driver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Get access points
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<AccessPointInfo> *CiscoC9800Driver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return AirespaceGetAccessPoints(snmp);
}

/**
 * Get access point state
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param apIndex access point index
 * @param macAdddr access point MAC address
 * @param ipAddr access point IP address
 * @param radioInterfaces list of radio interfaces for this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState CiscoC9800Driver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
      uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   return AirespaceGetAccessPointState(snmp, apIndex, macAddr, radioInterfaces, DEBUG_TAG);
}

/**
 * Radio slot map: key is base radio MAC (as in bsnAPIfTable index) packed into uint64, value is slot -> bsnAPIfType
 */
typedef std::map<uint64_t, std::map<uint32_t, int>> RadioSlotMap;

/**
 * Pack MAC address bytes into integer key
 */
static inline uint64_t MacKey(const BYTE *mac)
{
   uint64_t key = 0;
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      key = (key << 8) | mac[i];
   return key;
}

/**
 * Handler for bsnAPIfType walk (index: bsnAPDot3MacAddress + bsnAPIfSlotId)
 */
static uint32_t HandlerRadioSlotList(SNMP_Variable *var, SNMP_Transport *snmp, RadioSlotMap *slots)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   if (nameLen < MAC_ADDR_LENGTH + 1)
      return SNMP_ERR_SUCCESS;

   const uint32_t *index = name.value() + nameLen - MAC_ADDR_LENGTH - 1;
   BYTE mac[MAC_ADDR_LENGTH];
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      mac[i] = static_cast<BYTE>(index[i]);
   (*slots)[MacKey(mac)][index[MAC_ADDR_LENGTH]] = var->getValueAsInt();
   return SNMP_ERR_SUCCESS;
}

/**
 * Resolve radio slot for client from cldcIfType (CLApIfType) using AP radio slot map
 */
static uint32_t ResolveRadioSlot(const RadioSlotMap& slots, const BYTE *radioMac, int clientIfType)
{
   // CLApIfType: dot11bg(1), dot11a(2), uwb(3), dot11abgn(4), rlan(5), dot11_6ghz(6), dot11_xor_5_6ghz(7)
   // bsnAPIfType: dot11b(1), dot11a(2), uwb(4)
   int wantedBsnType;
   uint32_t defaultSlot;
   switch(clientIfType)
   {
      case 1:
         wantedBsnType = 1;
         defaultSlot = 0;
         break;
      case 2:
         wantedBsnType = 2;
         defaultSlot = 1;
         break;
      case 6:
      case 7:
         wantedBsnType = -1;  // 6 GHz radios are not distinguishable via bsnAPIfType
         defaultSlot = 2;
         break;
      default:
         return 0;
   }

   auto it = slots.find(MacKey(radioMac));
   if ((it != slots.end()) && (wantedBsnType != -1))
   {
      for(auto slot : it->second)
      {
         if (slot.second == wantedBsnType)
            return slot.first;
      }
   }
   return defaultSlot;
}

/**
 * Context for wireless station enumeration
 */
struct WirelessStationListContext
{
   ObjectArray<WirelessStationInfo> *wsList;
   RadioSlotMap slots;
};

/**
 * Handler for wireless client enumeration (walk over cldcApMacAddress in CISCO-LWAPP-DOT11-CLIENT-MIB)
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, WirelessStationListContext *ctx)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   if (nameLen < 14 + MAC_ADDR_LENGTH)
      return SNMP_ERR_SUCCESS;

   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(uint32_t));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid[13] = 2;   // cldcClientStatus
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[13] = 9;   // cldcIfType
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[13] = 10;  // cldcClientIPAddress
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[13] = 13;  // cldcClientAccessVLAN
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[13] = 28;  // cldcClientSSID
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[13] = 50;  // cldcClientCurrentTxRate (Mbit/s)
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
      return SNMP_ERR_SUCCESS;

   if (response->getNumVariables() == 6)
   {
      // CLDot11ClientStatus: idle(1), aaaPending(2), authenticated(3), associated(4), powersave(5),
      // disassociated(6), tobedeleted(7), probing(8), excluded(9)
      int status = response->getVariable(0)->getValueAsInt();
      if ((status >= 1) && (status <= 5))
      {
         auto info = new WirelessStationInfo();

         const uint32_t *macIndex = name.value() + nameLen - MAC_ADDR_LENGTH;
         for(int i = 0; i < MAC_ADDR_LENGTH; i++)
            info->macAddr[i] = static_cast<BYTE>(macIndex[i]);

         var->getRawValue(info->bssid, MAC_ADDR_LENGTH);   // base radio MAC of AP
         info->apMatchPolicy = AP_MATCH_BY_BSSID;
         info->rfIndex = ResolveRadioSlot(ctx->slots, info->bssid, response->getVariable(1)->getValueAsInt());

         TCHAR ipAddr[32];
         info->ipAddr = InetAddress::parse(response->getVariable(2)->getValueAsString(ipAddr, 32));
         info->vlan = response->getVariable(3)->getValueAsInt();
         response->getVariable(4)->getValueAsString(info->ssid, MAX_SSID_LENGTH);
         info->txRate = response->getVariable(5)->getValueAsUInt() * 1000;  // kbps
         info->rxRate = info->txRate;

         ctx->wsList->add(info);
      }
   }
   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *CiscoC9800Driver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{

   WirelessStationListContext ctx;
   ctx.wsList = new ObjectArray<WirelessStationInfo>(0, 64, Ownership::True);

   // Radio slot types per AP are needed to map client's interface type to radio slot
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 14179, 2, 2, 2, 1, 2 }, HandlerRadioSlotList, &ctx.slots) != SNMP_ERR_SUCCESS)  // bsnAPIfType
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CiscoC9800Driver::getWirelessStations(%s): cannot read bsnAPIfTable"), node->getName());
   }

   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 9, 9, 599, 1, 3, 1, 1, 8 }, HandlerWirelessStationList, &ctx) != SNMP_ERR_SUCCESS)  // cldcApMacAddress
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CiscoC9800Driver::getWirelessStations(%s): cannot read cldcClientTable"), node->getName());
      delete ctx.wsList;
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("CiscoC9800Driver::getWirelessStations(%s): %d clients read"), node->getName(), ctx.wsList->size());
   return ctx.wsList;
}
