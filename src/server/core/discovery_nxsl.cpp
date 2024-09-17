/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: discovery_nxsl.cpp
**
**/

#include "nxcore.h"
#include <nxcore_discovery.h>

/**
 * Implementation of NXSL class "DiscoveredInterface" - constructor
 */
NXSL_DiscoveredInterfaceClass::NXSL_DiscoveredInterfaceClass() : NXSL_Class()
{
   setName(_T("DiscoveredInterface"));
}

/**
 * Implementation of NXSL class "DiscoveredInterface" - get attribute
 */
NXSL_Value *NXSL_DiscoveredInterfaceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   InterfaceInfo *iface = static_cast<InterfaceInfo*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("alias"))
   {
      value = vm->createValue(iface->alias);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("chassis"))
   {
      value = vm->createValue(iface->location.chassis);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(iface->description);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("index"))
   {
      value = vm->createValue(iface->index);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddressList"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < iface->ipAddrList.size(); i++)
      {
         a->append(NXSL_InetAddressClass::createObject(vm, iface->ipAddrList.get(i)));
      }
      value = vm->createValue(a);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isPhysicalPort"))
   {
      value = vm->createValue(iface->isPhysicalPort);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddr"))
   {
      TCHAR buffer[64];
      value = vm->createValue(MACToStr(iface->macAddr, buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("module"))
   {
      value = vm->createValue(iface->location.module);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("mtu"))
   {
      value = vm->createValue(iface->mtu);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(iface->name);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pic"))
   {
      value = vm->createValue(iface->location.pic);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("port"))
   {
      value = vm->createValue(iface->location.port);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("speed"))
   {
      value = vm->createValue(iface->speed);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(iface->type);
   }
   return value;
}

/**
 * "DiscoveredInterface" class object
 */
NXSL_DiscoveredInterfaceClass g_nxslDiscoveredInterfaceClass;

/**
 * Implementation of NXSL class "DiscoveredNode" - constructor
 */
NXSL_DiscoveredNodeClass::NXSL_DiscoveredNodeClass() : NXSL_Class()
{
   setName(_T("DiscoveredNode"));
}

/**
 * Implementation of NXSL class "DiscoveredNode" - get attribute
 */
NXSL_Value *NXSL_DiscoveredNodeClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   DiscoveryFilterData *data = static_cast<DiscoveryFilterData*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("agentVersion"))
   {
      value = vm->createValue(data->agentVersion);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dnsName"))
   {
      if (!data->dnsNameResolved)
      {
         if (IsZoningEnabled() && (data->zoneUIN != 0))
         {
            shared_ptr<Zone> zone = FindZoneByUIN(data->zoneUIN);
            if (zone != nullptr)
            {
               shared_ptr<AgentConnectionEx> conn = zone->acquireConnectionToProxy();
               if (conn != nullptr)
               {
                  conn->getHostByAddr(data->ipAddr, data->dnsName, MAX_DNS_NAME);
               }
            }
         }
         else
         {
            data->ipAddr.getHostByAddr(data->dnsName, MAX_DNS_NAME);
         }
         data->dnsNameResolved = true;
      }
      value = (data->dnsName[0] != 0) ? vm->createValue(data->dnsName) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interfaces"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      if (data->ifList != nullptr)
      {
         for(int i = 0; i < data->ifList->size(); i++)
            array->append(vm->createValue(vm->createObject(&g_nxslDiscoveredInterfaceClass, data->ifList->get(i))));
      }
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddr"))
   {
      TCHAR buffer[64];
      value = vm->createValue(data->ipAddr.toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddress"))
   {
      value = NXSL_InetAddressClass::createObject(vm, data->ipAddr);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAgent"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_AGENT) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isBridge"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_BRIDGE) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isCDP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_CDP) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLLDP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_LLDP) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isPrinter"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_PRINTER) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isRouter"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_ROUTER) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSNMP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_SNMP) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSONMP"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_SONMP) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSSH"))
   {
      value = vm->createValue((LONG)((data->flags & NNF_IS_SSH) ? 1 : 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("netMask"))
   {
      value = vm->createValue(data->ipAddr.getMaskBits());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("platformName"))
   {
      value = vm->createValue(data->platform);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpOID"))
   {
      value = vm->createValue(data->snmpObjectId.toString());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpVersion"))
   {
      value = vm->createValue((LONG)data->snmpVersion);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("subnet"))
   {
      TCHAR buffer[64];
      value = vm->createValue(data->ipAddr.getSubnetAddress().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zone"))
   {
      shared_ptr<Zone> zone = FindZoneByUIN(data->zoneUIN);
      value = (zone != nullptr) ? zone->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
   {
      value = vm->createValue(data->zoneUIN);
   }
   return value;
}

/**
 * "DiscoveredNode" class object
 */
NXSL_DiscoveredNodeClass g_nxslDiscoveredNodeClass;
