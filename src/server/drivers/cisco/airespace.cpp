/*
** NetXMS - Network Management System
** Drivers for Cisco devices
** Copyright (C) 2013-2026 Raden Solutions
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
** File: airespace.cpp
**
** Access point enumeration via AIRESPACE-WIRELESS-MIB (bsnAPTable / bsnAPIfTable),
** shared by AireOS (CISCO-WLC) and IOS-XE (CISCO-C9800) wireless controller drivers.
**/

#include "cisco.h"

/**
 * Convert CISCO-LWAPP-TC-MIB CLApIfType (with optional cLApDot11XorRadioBand) and AIRESPACE bsnAPIfType to radio band
 */
static RadioBand RadioBandFromInterfaceTypes(const SNMP_Variable *lwappType, const SNMP_Variable *xorBand, const SNMP_Variable *bsnType)
{
   if ((lwappType != nullptr) && (lwappType->getType() == ASN_INTEGER))
   {
      switch(lwappType->getValueAsInt())
      {
         case 1:  // dot11bg
            return RADIO_BAND_2_4_GHZ;
         case 2:  // dot11a
            return RADIO_BAND_5_GHZ;
         case 6:  // dot11_6ghz
            return RADIO_BAND_6_GHZ;
         case 7:  // dot11_xor_5_6ghz
            if ((xorBand != nullptr) && (xorBand->getType() == ASN_INTEGER))
            {
               switch(xorBand->getValueAsInt())
               {
                  case 1:
                     return RADIO_BAND_2_4_GHZ;
                  case 2:
                     return RADIO_BAND_5_GHZ;
                  case 3:
                     return RADIO_BAND_6_GHZ;
               }
            }
            break;
      }
   }

   if ((bsnType != nullptr) && (bsnType->getType() == ASN_INTEGER))
   {
      switch(bsnType->getValueAsInt())
      {
         case 1:  // dot11b (implies b/g)
            return RADIO_BAND_2_4_GHZ;
         case 2:  // dot11a
            return RADIO_BAND_5_GHZ;
      }
   }

   return RADIO_BAND_UNKNOWN;
}

/**
 * Check if SNMP variable holds an actual value (not NoSuchObject/NoSuchInstance/EndOfMibView)
 */
static inline bool HasValue(const SNMP_Variable *v)
{
   return (v != nullptr) && (v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE) && (v->getType() != ASN_END_OF_MIBVIEW);
}

/**
 * Handler for access point enumeration (walk over bsnAPEthernetMacAddress).
 * Row index is bsnAPDot3MacAddress (base radio MAC), value is AP system (Ethernet) MAC
 * which is also the index of cLApTable in CISCO-LWAPP-AP-MIB.
 */
static uint32_t HandlerAccessPointList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<AccessPointInfo> *apList)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   if (nameLen < 12 + MAC_ADDR_LENGTH)
      return SNMP_ERR_SUCCESS;

   const uint32_t *radioMacIndex = name.value() + nameLen - MAC_ADDR_LENGTH;
   BYTE radioMac[MAC_ADDR_LENGTH];
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      radioMac[i] = static_cast<BYTE>(radioMacIndex[i]);

   MacAddress systemMac = var->getValueAsMACAddr();

   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(uint32_t));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid[11] = 19;  // bsnApIpAddress
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 6;   // bsnAPOperationStatus
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 3;   // bsnAPName
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 16;  // bsnAPModel
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[11] = 17;  // bsnAPSerialNumber
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   // bsnAPIfTable, indexed by radio MAC + slot
   oid[9] = 2;    // bsnAPIfTable
   oid[10] = 1;   // bsnAPIfEntry
   nameLen++;
   for(uint32_t slot = 0; slot < AIRESPACE_MAX_RADIO_SLOTS; slot++)
   {
      oid[nameLen - 1] = slot;
      oid[11] = 2;   // bsnAPIfType
      request.bindVariable(new SNMP_Variable(oid, nameLen));
      oid[11] = 4;   // bsnAPIfPhyChannelNumber
      request.bindVariable(new SNMP_Variable(oid, nameLen));
   }

   // cLApDot11IfTable (CISCO-LWAPP-AP-MIB), indexed by system MAC + slot
   static const uint32_t lwappDot11IfEntry[] = { 1, 3, 6, 1, 4, 1, 9, 9, 513, 1, 2, 1, 1 };
   uint32_t lwappOid[MAX_OID_LEN];
   memcpy(lwappOid, lwappDot11IfEntry, sizeof(lwappDot11IfEntry));
   size_t lwappOidLen = sizeof(lwappDot11IfEntry) / sizeof(uint32_t);
   lwappOidLen++; // column
   const BYTE *sysMacBytes = systemMac.value();
   for(int i = 0; i < MAC_ADDR_LENGTH; i++)
      lwappOid[lwappOidLen + i] = sysMacBytes[i];
   lwappOidLen += MAC_ADDR_LENGTH + 1;  // + slot
   for(uint32_t slot = 0; slot < AIRESPACE_MAX_RADIO_SLOTS; slot++)
   {
      lwappOid[lwappOidLen - 1] = slot;
      lwappOid[13] = 2;    // cLApDot11IfType
      request.bindVariable(new SNMP_Variable(lwappOid, lwappOidLen));
      lwappOid[13] = 27;   // cLApDot11XorRadioBand
      request.bindVariable(new SNMP_Variable(lwappOid, lwappOidLen));
   }

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc != SNMP_ERR_SUCCESS)
      return SNMP_ERR_SUCCESS;

   if (response->getNumVariables() == 5 + AIRESPACE_MAX_RADIO_SLOTS * 4)
   {
      TCHAR ipAddr[32], apName[MAX_OBJECT_NAME], model[MAX_OBJECT_NAME], serial[MAX_OBJECT_NAME];
      AccessPointInfo *ap = new AccessPointInfo(
            0,
            systemMac,
            InetAddress::parse(response->getVariable(0)->getValueAsString(ipAddr, 32)),
            (response->getVariable(1)->getValueAsInt() == 1) ? AP_UP : AP_UNPROVISIONED,
            response->getVariable(2)->getValueAsString(apName, MAX_OBJECT_NAME),
            _T("Cisco Systems Inc."),   // vendor
            response->getVariable(3)->getValueAsString(model, MAX_OBJECT_NAME),
            response->getVariable(4)->getValueAsString(serial, MAX_OBJECT_NAME));

      for(uint32_t slot = 0; slot < AIRESPACE_MAX_RADIO_SLOTS; slot++)
      {
         const SNMP_Variable *bsnType = response->getVariable(5 + slot * 2);
         const SNMP_Variable *bsnChannel = response->getVariable(6 + slot * 2);
         const SNMP_Variable *lwappType = response->getVariable(5 + AIRESPACE_MAX_RADIO_SLOTS * 2 + slot * 2);
         const SNMP_Variable *xorBand = response->getVariable(6 + AIRESPACE_MAX_RADIO_SLOTS * 2 + slot * 2);

         if (!HasValue(bsnType) && !HasValue(lwappType))
            continue;   // slot does not exist

         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         _sntprintf(radio.name, MAX_OBJECT_NAME, _T("slot%u"), slot);
         radio.index = slot;
         memcpy(radio.bssid, radioMac, MAC_ADDR_LENGTH);
         radio.band = RadioBandFromInterfaceTypes(HasValue(lwappType) ? lwappType : nullptr, HasValue(xorBand) ? xorBand : nullptr, HasValue(bsnType) ? bsnType : nullptr);
         if (HasValue(bsnChannel))
         {
            radio.channel = static_cast<uint16_t>(bsnChannel->getValueAsUInt());
            radio.frequency = WirelessChannelToFrequency(radio.band, radio.channel);
         }
         ap->addRadioInterface(radio);
      }

      apList->add(ap);
   }
   delete response;

   return SNMP_ERR_SUCCESS;
}

/**
 * Get access points from bsnAPTable / bsnAPIfTable
 */
ObjectArray<AccessPointInfo> *AirespaceGetAccessPoints(SNMP_Transport *snmp)
{
   auto apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 14179, 2, 2, 1, 1, 33 },  // bsnAPEthernetMacAddress
                HandlerAccessPointList, apList) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return nullptr;
   }
   return apList;
}

/**
 * Get access point state from bsnAPOperationStatus (bsnAPTable row is identified by base radio MAC)
 */
AccessPointState AirespaceGetAccessPointState(SNMP_Transport *snmp, uint32_t apIndex, const MacAddress& macAddr,
      const StructArray<RadioInterfaceInfo>& radioInterfaces, const TCHAR *debugTag)
{
   if (radioInterfaces.isEmpty())
      return AP_UNKNOWN;

   TCHAR oid[256], macAddrText[64];
   _sntprintf(oid, 256, _T(".1.3.6.1.4.1.14179.2.2.1.1.6.%s"),
            MacAddress(radioInterfaces.get(0)->bssid, MAC_ADDR_LENGTH).toString(macAddrText, MacAddressNotation::DECIMAL_DOT_SEPARATED));

   TCHAR buffer[32];
   uint32_t value;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &value, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(debugTag, 6, _T("Cannot get access point [index=%u, mac=%s] status from OID %s"), apIndex, macAddr.toString(buffer), oid);
      return AP_UNKNOWN;
   }

   nxlog_debug_tag(debugTag, 6, _T("Retrieved access point [index=%u, mac=%s] status %d"), apIndex, macAddr.toString(buffer), value);
   switch(value)
   {
      case 1:  // associated
         return AP_UP;
      case 2:  // disassociating
      case 3:  // downloading
         return AP_UNPROVISIONED;
      default:
         return AP_UNKNOWN;
   }
}
