/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: nxsl_classes.cpp
**
**/

#include "nxcore.h"

/**
 * clearGeoLocation()
 */
NXSL_METHOD_DEFINITION(NetObj, clearGeoLocation)
{
   ((NetObj *)object->getData())->setGeoLocation(GeoLocation());
   *result = new NXSL_Value;
   return 0;
}

/**
 * setGeoLocation(loc)
 */
NXSL_METHOD_DEFINITION(NetObj, setGeoLocation)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *o = argv[0]->getValueAsObject();
   if (_tcscmp(o->getClass()->getName(), g_nxslGeoLocationClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   GeoLocation *gl = (GeoLocation *)o->getData();
   ((NetObj *)object->getData())->setGeoLocation(*gl);
   *result = new NXSL_Value;
   return 0;
}

/**
 * setMapImage(image)
 */
NXSL_METHOD_DEFINITION(NetObj, setMapImage)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid FROM images WHERE upper(guid)=upper(?) OR upper(name)=upper(?)"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uuid guid = DBGetFieldGUID(hResult, 0, 0);
            ((NetObj *)object->getData())->setMapImage(guid);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   *result = new NXSL_Value(success ? 1 : 0);
   return 0;
}

/**
 * setStatusCalculation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusCalculation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = (NetObj *)object->getData();
   switch(method)
   {
      case SA_CALCULATE_DEFAULT:
      case SA_CALCULATE_MOST_CRITICAL:
         netobj->setStatusCalculation(method);
         break;
      case SA_CALCULATE_SINGLE_THRESHOLD:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32());
         break;
      case SA_CALCULATE_MULTIPLE_THRESHOLDS:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = new NXSL_Value(success);
   return 0;
}

/**
 * setStatusPropagation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusPropagation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = (NetObj *)object->getData();
   switch(method)
   {
      case SA_PROPAGATE_DEFAULT:
      case SA_PROPAGATE_UNCHANGED:
         netobj->setStatusPropagation(method);
         break;
      case SA_PROPAGATE_FIXED:
      case SA_PROPAGATE_RELATIVE:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32());
         break;
      case SA_PROPAGATE_TRANSLATED:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = new NXSL_Value(success);
   return 0;
}

/**
 * NXSL class NetObj: constructor
 */
NXSL_NetObjClass::NXSL_NetObjClass() : NXSL_Class()
{
   setName(_T("NetObj"));

   NXSL_REGISTER_METHOD(NetObj, clearGeoLocation, 0);
   NXSL_REGISTER_METHOD(NetObj, setGeoLocation, 1);
   NXSL_REGISTER_METHOD(NetObj, setMapImage, 1);
   NXSL_REGISTER_METHOD(NetObj, setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(NetObj, setStatusPropagation, -1);
}

/**
 * Object creation handler
 */
void NXSL_NetObjClass::onObjectCreate(NXSL_Object *object)
{
   ((NetObj *)object->getData())->incRefCount();
}

/**
 * Object destruction handler
 */
void NXSL_NetObjClass::onObjectDelete(NXSL_Object *object)
{
   ((NetObj *)object->getData())->decRefCount();
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *_object, const TCHAR *attr)
{
   NXSL_Value *value = NULL;
   NetObj *object = (NetObj *)_object->getData();
   if (!_tcscmp(attr, _T("alarms")))
   {
      ObjectArray<Alarm> *alarms = GetAlarms(object->getId(), true);
      alarms->setOwner(false);
      NXSL_Array *array = new NXSL_Array;
      for(int i = 0; i < alarms->size(); i++)
         array->append(new NXSL_Value(new NXSL_Object(&g_nxslAlarmClass, alarms->get(i))));
      value = new NXSL_Value(array);
      delete alarms;
   }
   else if (!_tcscmp(attr, _T("city")))
   {
      value = new NXSL_Value(object->getPostalAddress()->getCity());
   }
   else if (!_tcscmp(attr, _T("comments")))
   {
      value = new NXSL_Value(object->getComments());
   }
   else if (!_tcscmp(attr, _T("country")))
   {
      value = new NXSL_Value(object->getPostalAddress()->getCountry());
   }
   else if (!_tcscmp(attr, _T("customAttributes")))
   {
      value = object->getCustomAttributesForNXSL();
   }
   else if (!_tcscmp(attr, _T("geolocation")))
   {
      value = NXSL_GeoLocationClass::createObject(object->getGeoLocation());
   }
   else if (!_tcscmp(attr, _T("guid")))
   {
      TCHAR buffer[64];
      value = new NXSL_Value(object->getGuid().toString(buffer));
   }
   else if (!_tcscmp(attr, _T("id")))
   {
      value = new NXSL_Value(object->getId());
   }
   else if (!_tcscmp(attr, _T("ipAddr")))
   {
      TCHAR buffer[64];
      GetObjectIpAddress(object).toString(buffer);
      value = new NXSL_Value(buffer);
   }
   else if (!_tcscmp(attr, _T("mapImage")))
   {
      TCHAR buffer[64];
      value = new NXSL_Value(object->getMapImage().toString(buffer));
   }
   else if (!_tcscmp(attr, _T("name")))
   {
      value = new NXSL_Value(object->getName());
   }
   else if (!_tcscmp(attr, _T("postcode")))
   {
      value = new NXSL_Value(object->getPostalAddress()->getPostCode());
   }
   else if (!_tcscmp(attr, _T("status")))
   {
      value = new NXSL_Value((LONG)object->getStatus());
   }
   else if (!_tcscmp(attr, _T("streetAddress")))
   {
      value = new NXSL_Value(object->getPostalAddress()->getStreetAddress());
   }
   else if (!_tcscmp(attr, _T("type")))
   {
      value = new NXSL_Value((LONG)object->getObjectClass());
   }
	else
	{
		value = object->getCustomAttributeForNXSL(attr);
	}
   return value;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_ZoneClass::NXSL_ZoneClass() : NXSL_NetObjClass()
{
   setName(_T("Zone"));
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_ZoneClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Zone *zone = (Zone *)object->getData();
   if (!_tcscmp(attr, _T("proxyNode")))
   {
      Node *node = (Node *)FindObjectById(zone->getProxyNodeId(), OBJECT_NODE);
      value = (node != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node)) : new NXSL_Value();
   }
   else if (!_tcscmp(attr, _T("proxyNodeId")))
   {
      value = new NXSL_Value(zone->getProxyNodeId());
   }
   else if (!_tcscmp(attr, _T("uin")))
   {
      value = new NXSL_Value(zone->getUIN());
   }
   return value;
}

/**
 * Generic implementation for flag changing methods
 */
static int ChangeFlagMethod(NXSL_Object *object, NXSL_Value *arg, NXSL_Value **result, UINT32 flag)
{
   if (!arg->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Node *node = (Node *)object->getData();
   if (arg->getValueAsInt32())
      node->clearFlag(flag);
   else
      node->setFlag(flag);

   *result = new NXSL_Value;
   return 0;
}

/**
 * enableAgent(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableAgent)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_NXCP);
}

/**
 * enableConfigurationPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableConfigurationPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_CONF_POLL);
}

/**
 * enableDiscoveryPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableDiscoveryPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_DISCOVERY_POLL);
}

/**
 * enableIcmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableIcmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ICMP);
}

/**
 * enableRoutingTablePolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableRoutingTablePolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ROUTE_POLL);
}

/**
 * enableSnmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSnmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SNMP);
}

/**
 * enableStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_STATUS_POLL);
}

/**
 * enableTopologyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableTopologyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_TOPOLOGY_POLL);
}

/**
 * NXSL class Node: constructor
 */
NXSL_NodeClass::NXSL_NodeClass() : NXSL_NetObjClass()
{
   setName(_T("Node"));

   NXSL_REGISTER_METHOD(Node, enableAgent, 1);
   NXSL_REGISTER_METHOD(Node, enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableDiscoveryPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableIcmp, 1);
   NXSL_REGISTER_METHOD(Node, enableRoutingTablePolling, 1);
   NXSL_REGISTER_METHOD(Node, enableSnmp, 1);
   NXSL_REGISTER_METHOD(Node, enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableTopologyPolling, 1);
}

/**
 * NXSL class Node: get attribute
 */
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Node *node = (Node *)object->getData();
   if (!_tcscmp(attr, _T("agentVersion")))
   {
      value = new NXSL_Value(node->getAgentVersion());
   }
   else if (!_tcscmp(attr, _T("bootTime")))
   {
      value = new NXSL_Value((INT64)node->getBootTime());
   }
   else if (!_tcscmp(attr, _T("bridgeBaseAddress")))
   {
      TCHAR buffer[64];
      value = new NXSL_Value(BinToStr(node->getBridgeId(), MAC_ADDR_LENGTH, buffer));
   }
   else if (!_tcscmp(attr, _T("components")))
   {
      value = new NXSL_Value(new NXSL_Object(&g_nxslComponentClass, node->getComponents()->getRoot()));
   }
   else if (!_tcscmp(attr, _T("driver")))
   {
      value = new NXSL_Value(node->getDriverName());
   }
   else if (!_tcscmp(attr, _T("flags")))
   {
		value = new NXSL_Value(node->getFlags());
   }
   else if (!_tcscmp(attr, _T("isAgent")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_NATIVE_AGENT) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isBridge")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isCDP")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_CDP) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isInMaintenanceMode")))
   {
      value = new NXSL_Value((LONG)(node->isInMaintenanceMode() ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isLLDP")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_LLDP) ? 1 : 0));
   }
	else if (!_tcscmp(attr, _T("isLocalMgmt")) || !_tcscmp(attr, _T("isLocalManagement")))
	{
		value = new NXSL_Value((LONG)((node->isLocalManagement()) ? 1 : 0));
	}
   else if (!_tcscmp(attr, _T("isPAE")) || !_tcscmp(attr, _T("is802_1x")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_8021X) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isPrinter")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_PRINTER) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isRouter")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_ROUTER) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isSMCLP")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_SMCLP) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isSNMP")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_SNMP) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isSONMP")))
   {
      value = new NXSL_Value((LONG)((node->getFlags() & NF_IS_SONMP) ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("lastAgentCommTime")))
   {
      value = new NXSL_Value((INT64)node->getLastAgentCommTime());
   }
   else if (!_tcscmp(attr, _T("platformName")))
   {
      value = new NXSL_Value(node->getPlatformName());
   }
   else if (!_tcscmp(attr, _T("rack")))
   {
      Rack *rack = (Rack *)FindObjectById(node->getRackId(), OBJECT_RACK);
      if (rack != NULL)
      {
         value = rack->createNXSLObject();
      }
      else
      {
         value = new NXSL_Value;
      }
   }
   else if (!_tcscmp(attr, _T("rackId")))
   {
      value = new NXSL_Value(node->getRackId());
   }
   else if (!_tcscmp(attr, _T("rackHeight")))
   {
      value = new NXSL_Value(node->getRackHeight());
   }
   else if (!_tcscmp(attr, _T("rackPosition")))
   {
      value = new NXSL_Value(node->getRackPosition());
   }
   else if (!_tcscmp(attr, _T("runtimeFlags")))
   {
      value = new NXSL_Value(node->getRuntimeFlags());
   }
   else if (!_tcscmp(attr, _T("snmpOID")))
   {
      value = new NXSL_Value(node->getSNMPObjectId());
   }
   else if (!_tcscmp(attr, _T("snmpSysContact")))
   {
      value = new NXSL_Value(node->getSysContact());
   }
   else if (!_tcscmp(attr, _T("snmpSysLocation")))
   {
      value = new NXSL_Value(node->getSysLocation());
   }
   else if (!_tcscmp(attr, _T("snmpSysName")))
   {
      value = new NXSL_Value(node->getSysName());
   }
   else if (!_tcscmp(attr, _T("snmpVersion")))
   {
      value = new NXSL_Value((LONG)node->getSNMPVersion());
   }
   else if (!_tcscmp(attr, _T("subType")))
   {
      value = new NXSL_Value(node->getSubType());
   }
   else if (!_tcscmp(attr, _T("sysDescription")))
   {
      value = new NXSL_Value(node->getSysDescription());
   }
   else if (!_tcscmp(attr, _T("type")))
   {
      value = new NXSL_Value((INT32)node->getType());
   }
   else if (!_tcscmp(attr, _T("zone")))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(node->getZoneUIN());
		   if (zone != NULL)
		   {
			   value = new NXSL_Value(new NXSL_Object(&g_nxslZoneClass, zone));
		   }
		   else
		   {
			   value = new NXSL_Value;
		   }
	   }
	   else
	   {
		   value = new NXSL_Value;
	   }
	}
   else if (!_tcscmp(attr, _T("zoneUIN")))
	{
      value = new NXSL_Value(node->getZoneUIN());
   }
   return value;
}

/**
 * Interface::setExcludeFromTopology(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, setExcludeFromTopology)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = (Interface *)object->getData();
   iface->setExcludeFromTopology(argv[0]->getValueAsInt32() != 0);
   *result = new NXSL_Value;
   return 0;
}

/**
 * Interface::setExpectedState(state) method
 */
NXSL_METHOD_DEFINITION(Interface, setExpectedState)
{
   int state;
   if (argv[0]->isInteger())
   {
      state = argv[0]->getValueAsInt32();
   }
   else if (argv[0]->isString())
   {
      static const TCHAR *stateNames[] = { _T("UP"), _T("DOWN"), _T("IGNORE"), NULL };
      const TCHAR *name = argv[0]->getValueAsCString();
      for(state = 0; stateNames[state] != NULL; state++)
         if (!_tcsicmp(stateNames[state], name))
            break;
   }
   else
   {
      return NXSL_ERR_NOT_STRING;
   }

   if ((state >= 0) && (state <= 2))
      ((Interface *)object->getData())->setExpectedState(state);

   *result = new NXSL_Value;
   return 0;
}

/**
 * NXSL class Interface: constructor
 */
NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_NetObjClass()
{
   setName(_T("Interface"));

   NXSL_REGISTER_METHOD(Interface, setExcludeFromTopology, 1);
   NXSL_REGISTER_METHOD(Interface, setExpectedState, 1);
}

/**
 * NXSL class Interface: get attribute
 */
NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Interface *iface = (Interface *)object->getData();
   if (!_tcscmp(attr, _T("adminState")))
   {
		value = new NXSL_Value((LONG)iface->getAdminState());
   }
   else if (!_tcscmp(attr, _T("alias")))
   {
      value = new NXSL_Value(iface->getAlias());
   }
   else if (!_tcscmp(attr, _T("bridgePortNumber")))
   {
		value = new NXSL_Value(iface->getBridgePortNumber());
   }
   else if (!_tcscmp(attr, _T("description")))
   {
      value = new NXSL_Value(iface->getDescription());
   }
   else if (!_tcscmp(attr, _T("dot1xBackendAuthState")))
   {
		value = new NXSL_Value((LONG)iface->getDot1xBackendAuthState());
   }
   else if (!_tcscmp(attr, _T("dot1xPaeAuthState")))
   {
		value = new NXSL_Value((LONG)iface->getDot1xPaeAuthState());
   }
   else if (!_tcscmp(attr, _T("expectedState")))
   {
		value = new NXSL_Value((iface->getFlags() & IF_EXPECTED_STATE_MASK) >> 28);
   }
   else if (!_tcscmp(attr, _T("flags")))
   {
		value = new NXSL_Value(iface->getFlags());
   }
   else if (!_tcscmp(attr, _T("ifIndex")))
   {
		value = new NXSL_Value(iface->getIfIndex());
   }
   else if (!_tcscmp(attr, _T("ifType")))
   {
		value = new NXSL_Value(iface->getIfType());
   }
   else if (!_tcscmp(attr, _T("ipAddressList")))
   {
      const InetAddressList *addrList = iface->getIpAddressList();
      NXSL_Array *a = new NXSL_Array();
      for(int i = 0; i < addrList->size(); i++)
      {
         a->append(new NXSL_Value(NXSL_InetAddressClass::createObject(addrList->get(i))));
      }
      value = new NXSL_Value(a);
   }
   else if (!_tcscmp(attr, _T("ipNetMask")))
   {
      value = new NXSL_Value(iface->getIpAddressList()->getFirstUnicastAddress().getMaskBits());
   }
   else if (!_tcscmp(attr, _T("isExcludedFromTopology")))
   {
      value = new NXSL_Value((LONG)(iface->isExcludedFromTopology() ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isLoopback")))
   {
		value = new NXSL_Value((LONG)(iface->isLoopback() ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isManuallyCreated")))
   {
		value = new NXSL_Value((LONG)(iface->isManuallyCreated() ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("isPhysicalPort")))
   {
		value = new NXSL_Value((LONG)(iface->isPhysicalPort() ? 1 : 0));
   }
   else if (!_tcscmp(attr, _T("macAddr")))
   {
		TCHAR buffer[256];
		value = new NXSL_Value(BinToStr(iface->getMacAddr(), MAC_ADDR_LENGTH, buffer));
   }
   else if (!_tcscmp(attr, _T("mtu")))
   {
      value = new NXSL_Value(iface->getMTU());
   }
   else if (!_tcscmp(attr, _T("node")))
	{
		Node *parentNode = iface->getParentNode();
		if (parentNode != NULL)
		{
			value = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, parentNode));
		}
		else
		{
			value = new NXSL_Value;
		}
	}
   else if (!_tcscmp(attr, _T("operState")))
   {
		value = new NXSL_Value((LONG)iface->getOperState());
   }
   else if (!_tcscmp(attr, _T("peerInterface")))
   {
		Interface *peerIface = (Interface *)FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
		if (peerIface != NULL)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				Node *peerNode = peerIface->getParentNode();
				if ((parentNode != NULL) && (peerNode != NULL))
				{
					if (peerNode->isTrustedNode(parentNode->getId()))
					{
						value = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, peerIface));
					}
					else
					{
						// No access, return null
						value = new NXSL_Value;
						DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): access denied for node %s [%d]"),
									 iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
					}
				}
				else
				{
					value = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=%p peerNode=%p"), iface->getName(), iface->getId(), parentNode, peerNode);
				}
			}
			else
			{
				value = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, peerIface));
			}
		}
		else
		{
			value = new NXSL_Value;
		}
   }
   else if (!_tcscmp(attr, _T("peerNode")))
   {
		Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if (peerNode != NULL)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				if ((parentNode != NULL) && (peerNode->isTrustedNode(parentNode->getId())))
				{
					value = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, peerNode));
				}
				else
				{
					// No access, return null
					value = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
					          iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
				}
			}
			else
			{
				value = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, peerNode));
			}
		}
		else
		{
			value = new NXSL_Value;
		}
   }
   else if (!_tcscmp(attr, _T("port")))
   {
      value = new NXSL_Value(iface->getPortNumber());
   }
   else if (!_tcscmp(attr, _T("slot")))
   {
      value = new NXSL_Value(iface->getSlotNumber());
   }
   else if (!_tcscmp(attr, _T("speed")))
   {
      value = new NXSL_Value(iface->getSpeed());
   }
   else if (!_tcscmp(attr, _T("zone")))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(iface->getZoneUIN());
		   if (zone != NULL)
		   {
			   value = new NXSL_Value(new NXSL_Object(&g_nxslZoneClass, zone));
		   }
		   else
		   {
			   value = new NXSL_Value;
		   }
	   }
	   else
	   {
		   value = new NXSL_Value;
	   }
	}
   else if (!_tcscmp(attr, _T("zoneUIN")))
	{
      value = new NXSL_Value(iface->getZoneUIN());
   }
   return value;
}

/**
 * NXSL class Mobile Device: constructor
 */
NXSL_MobileDeviceClass::NXSL_MobileDeviceClass() : NXSL_NetObjClass()
{
   setName(_T("MobileDevice"));
}

/**
 * NXSL class Mobile Device: get attribute
 */
NXSL_Value *NXSL_MobileDeviceClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   MobileDevice *mobDevice = (MobileDevice *)object->getData();
   if (!_tcscmp(attr, _T("deviceId")))
   {
		value = new NXSL_Value(mobDevice->getDeviceId());
   }
   else if (!_tcscmp(attr, _T("vendor")))
   {
      value = new NXSL_Value(mobDevice->getVendor());
   }
   else if (!_tcscmp(attr, _T("model")))
   {
      value = new NXSL_Value(mobDevice->getModel());
   }
   else if (!_tcscmp(attr, _T("serialNumber")))
   {
      value = new NXSL_Value(mobDevice->getSerialNumber());
   }
   else if (!_tcscmp(attr, _T("osName")))
   {
      value = new NXSL_Value(mobDevice->getOsName());
   }
   else if (!_tcscmp(attr, _T("osVersion")))
   {
      value = new NXSL_Value(mobDevice->getOsVersion());
   }
   else if (!_tcscmp(attr, _T("userId")))
   {
      value = new NXSL_Value(mobDevice->getUserId());
   }
   else if (!_tcscmp(attr, _T("batteryLevel")))
   {
      value = new NXSL_Value(mobDevice->getBatteryLevel());
   }

   return value;
}

/**
 * NXSL class "Chassis" constructor
 */
NXSL_ChassisClass::NXSL_ChassisClass() : NXSL_NetObjClass()
{
   setName(_T("Chassis"));
}

/**
 * NXSL class "Chassis" attributes
 */
NXSL_Value *NXSL_ChassisClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Chassis *chassis = (Chassis *)object->getData();
   if (!_tcscmp(attr, _T("controller")))
   {
      Node *node = (Node *)FindObjectById(chassis->getControllerId(), OBJECT_NODE);
      if (node != NULL)
      {
         value = node->createNXSLObject();
      }
      else
      {
         value = new NXSL_Value;
      }
   }
   else if (!_tcscmp(attr, _T("controllerId")))
   {
      value = new NXSL_Value(chassis->getControllerId());
   }
   else if (!_tcscmp(attr, _T("flags")))
   {
      value = new NXSL_Value(chassis->getFlags());
   }
   else if (!_tcscmp(attr, _T("rack")))
   {
      Rack *rack = (Rack *)FindObjectById(chassis->getRackId(), OBJECT_RACK);
      if (rack != NULL)
      {
         value = rack->createNXSLObject();
      }
      else
      {
         value = new NXSL_Value;
      }
   }
   else if (!_tcscmp(attr, _T("rackId")))
   {
      value = new NXSL_Value(chassis->getRackId());
   }
   else if (!_tcscmp(attr, _T("rackHeight")))
   {
      value = new NXSL_Value(chassis->getRackHeight());
   }
   else if (!_tcscmp(attr, _T("rackPosition")))
   {
      value = new NXSL_Value(chassis->getRackPosition());
   }
   return value;
}

/**
 * Cluster::getResourceOwner() method
 */
NXSL_METHOD_DEFINITION(Cluster, getResourceOwner)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT32 ownerId = ((Cluster *)object->getData())->getResourceOwner(argv[0]->getValueAsCString());
   if (ownerId != 0)
   {
      NetObj *object = FindObjectById(ownerId);
      *result = (object != NULL) ? object->createNXSLObject() : new NXSL_Value();
   }
   else
   {
      *result = new NXSL_Value();
   }
   return 0;
}

/**
 * NXSL class "Cluster" constructor
 */
NXSL_ClusterClass::NXSL_ClusterClass() : NXSL_NetObjClass()
{
   setName(_T("Cluster"));

   NXSL_REGISTER_METHOD(Cluster, getResourceOwner, 1);
}

/**
 * NXSL class "Cluster" attributes
 */
NXSL_Value *NXSL_ClusterClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Cluster *cluster = (Cluster *)object->getData();
   if (!_tcscmp(attr, _T("nodes")))
   {
      value = new NXSL_Value(cluster->getNodesForNXSL());
   }
   else if (!_tcscmp(attr, _T("zone")))
   {
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(cluster->getZoneUIN());
         if (zone != NULL)
         {
            value = zone->createNXSLObject();
         }
         else
         {
            value = new NXSL_Value;
         }
      }
      else
      {
         value = new NXSL_Value;
      }
   }
   else if (!_tcscmp(attr, _T("zoneUIN")))
   {
      value = new NXSL_Value(cluster->getZoneUIN());
   }
   return value;
}

/**
 * Container::setAutoBindMode() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindMode)
{
   if (!argv[0]->isInteger() || !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ((Container *)object->getData())->setAutoBindMode(argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
   *result = new NXSL_Value();
   return 0;
}

/**
 * Container::setAutoBindScript() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindScript)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   ((Container *)object->getData())->setAutoBindFilter(argv[0]->getValueAsCString());
   *result = new NXSL_Value();
   return 0;
}

/**
 * NXSL class "Container" constructor
 */
NXSL_ContainerClass::NXSL_ContainerClass() : NXSL_NetObjClass()
{
   setName(_T("Container"));

   NXSL_REGISTER_METHOD(Container, setAutoBindMode, 2);
   NXSL_REGISTER_METHOD(Container, setAutoBindScript, 1);
}

/**
 * NXSL class "Cluster" attributes
 */
NXSL_Value *NXSL_ContainerClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   Container *container = (Container *)object->getData();
   if (!_tcscmp(attr, _T("autoBindScript")))
   {
      const TCHAR *script = container->getAutoBindScriptSource();
      value = new NXSL_Value(CHECK_NULL_EX(script));
   }
   else if (!_tcscmp(attr, _T("isAutoBindEnabled")))
   {
      value = new NXSL_Value(container->isAutoBindEnabled() ? 1 : 0);
   }
   else if (!_tcscmp(attr, _T("isAutoUnbindEnabled")))
   {
      value = new NXSL_Value(container->isAutoUnbindEnabled() ? 1 : 0);
   }
   return value;
}

/**
 * Event::setMessage() method
 */
NXSL_METHOD_DEFINITION(Event, setMessage)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = (Event *)object->getData();
   event->setMessage(argv[0]->getValueAsCString());
   *result = new NXSL_Value();
   return 0;
}

/**
 * Event::setSeverity() method
 */
NXSL_METHOD_DEFINITION(Event, setSeverity)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_STRING;

   int s = argv[0]->getValueAsInt32();
   if ((s >= SEVERITY_NORMAL) && (s <= SEVERITY_CRITICAL))
   {
      Event *event = (Event *)object->getData();
      event->setSeverity(s);
   }
   *result = new NXSL_Value();
   return 0;
}

/**
 * Event::setUserTag() method
 */
NXSL_METHOD_DEFINITION(Event, setUserTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = (Event *)object->getData();
   event->setUserTag(argv[0]->getValueAsCString());
   *result = new NXSL_Value();
   return 0;
}

/**
 * Event::toJson() method
 */
NXSL_METHOD_DEFINITION(Event, toJson)
{
   Event *event = (Event *)object->getData();
   *result = new NXSL_Value(event->createJson());
   return 0;
}

/**
 * NXSL class Event: constructor
 */
NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   setName(_T("Event"));

   NXSL_REGISTER_METHOD(Event, setMessage, 1);
   NXSL_REGISTER_METHOD(Event, setSeverity, 1);
   NXSL_REGISTER_METHOD(Event, setUserTag, 1);
   NXSL_REGISTER_METHOD(Event, toJson, 0);
}

/**
 * NXSL class Event: get attribute
 */
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *pObject, const TCHAR *attr)
{
   NXSL_Value *value = NULL;

   Event *event = (Event *)pObject->getData();
   if (!_tcscmp(attr, _T("code")))
   {
      value = new NXSL_Value(event->getCode());
   }
   else if (!_tcscmp(attr, _T("customMessage")))
   {
      value = new NXSL_Value(event->getCustomMessage());
   }
   else if (!_tcscmp(attr, _T("id")))
   {
      value = new NXSL_Value(event->getId());
   }
   else if (!_tcscmp(attr, _T("message")))
   {
      value = new NXSL_Value(event->getMessage());
   }
   else if (!_tcscmp(attr, _T("name")))
   {
		value = new NXSL_Value(event->getName());
   }
   else if (!_tcscmp(attr, _T("parameters")))
   {
      NXSL_Array *array = new NXSL_Array;
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, new NXSL_Value(event->getParameter(i)));
      value = new NXSL_Value(array);
   }
   else if (!_tcscmp(attr, _T("severity")))
   {
      value = new NXSL_Value(event->getSeverity());
   }
   else if (!_tcscmp(attr, _T("source")))
   {
      NetObj *object = FindObjectById(event->getSourceId());
      value = (object != NULL) ? object->createNXSLObject() : new NXSL_Value();
   }
   else if (!_tcscmp(attr, _T("sourceId")))
   {
      value = new NXSL_Value(event->getSourceId());
   }
   else if (!_tcscmp(attr, _T("timestamp")))
   {
      value = new NXSL_Value((INT64)event->getTimeStamp());
   }
   else if (!_tcscmp(attr, _T("userTag")))
   {
      value = new NXSL_Value(event->getUserTag());
   }
   else
   {
      if (attr[0] == _T('$'))
      {
         // Try to find parameter with given index
         TCHAR *eptr;
         int index = _tcstol(&attr[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *s = event->getParameter(index - 1);
            if (s != NULL)
            {
               value = new NXSL_Value(s);
            }
         }
      }

      // Try to find named parameter with given name
      if (value == NULL)
      {
         const TCHAR *s = event->getNamedParameter(attr);
         if (s != NULL)
         {
            value = new NXSL_Value(s);
         }
      }
   }
   return value;
}

/**
 * Alarm::acknowledge() method
 */
NXSL_METHOD_DEFINITION(Alarm, acknowledge)
{
   Alarm *alarm = (Alarm *)object->getData();
   *result = new NXSL_Value(AckAlarmById(alarm->getAlarmId(), NULL, false, 0));
   return 0;
}

/**
 * Alarm::resolve() method
 */
NXSL_METHOD_DEFINITION(Alarm, resolve)
{
   Alarm *alarm = (Alarm *)object->getData();
   *result = new NXSL_Value(ResolveAlarmById(alarm->getAlarmId(), NULL, false));
   return 0;
}

/**
 * Alarm::terminate() method
 */
NXSL_METHOD_DEFINITION(Alarm, terminate)
{
   Alarm *alarm = (Alarm *)object->getData();
   *result = new NXSL_Value(ResolveAlarmById(alarm->getAlarmId(), NULL, true));
   return 0;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmClass::NXSL_AlarmClass() : NXSL_Class()
{
   setName(_T("Alarm"));

   NXSL_REGISTER_METHOD(Alarm, acknowledge, 0);
   NXSL_REGISTER_METHOD(Alarm, resolve, 0);
   NXSL_REGISTER_METHOD(Alarm, terminate, 0);
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmClass::onObjectDelete(NXSL_Object *object)
{
   delete (Alarm *)object->getData();
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmClass::getAttr(NXSL_Object *pObject, const TCHAR *attr)
{
   NXSL_Value *value = NULL;
   Alarm *alarm = (Alarm *)pObject->getData();

   if (!_tcscmp(attr, _T("ackBy")))
   {
      value = new NXSL_Value(alarm->getAckByUser());
   }
   else if (!_tcscmp(attr, _T("creationTime")))
   {
      value = new NXSL_Value((INT64)alarm->getCreationTime());
   }
   else if (!_tcscmp(attr, _T("dciId")))
   {
      value = new NXSL_Value(alarm->getDciId());
   }
   else if (!_tcscmp(attr, _T("eventCode")))
   {
      value = new NXSL_Value(alarm->getSourceEventCode());
   }
   else if (!_tcscmp(attr, _T("eventId")))
   {
      value = new NXSL_Value(alarm->getSourceEventId());
   }
   else if (!_tcscmp(attr, _T("helpdeskReference")))
   {
      value = new NXSL_Value(alarm->getHelpDeskRef());
   }
   else if (!_tcscmp(attr, _T("helpdeskState")))
   {
      value = new NXSL_Value(alarm->getHelpDeskState());
   }
   else if (!_tcscmp(attr, _T("id")))
   {
      value = new NXSL_Value(alarm->getAlarmId());
   }
   else if (!_tcscmp(attr, _T("key")))
   {
      value = new NXSL_Value(alarm->getKey());
   }
   else if (!_tcscmp(attr, _T("lastChangeTime")))
   {
      value = new NXSL_Value((INT64)alarm->getLastChangeTime());
   }
   else if (!_tcscmp(attr, _T("message")))
   {
      value = new NXSL_Value(alarm->getMessage());
   }
   else if (!_tcscmp(attr, _T("originalSeverity")))
   {
      value = new NXSL_Value(alarm->getOriginalSeverity());
   }
   else if (!_tcscmp(attr, _T("repeatCount")))
   {
      value = new NXSL_Value(alarm->getRepeatCount());
   }
   else if (!_tcscmp(attr, _T("resolvedBy")))
   {
      value = new NXSL_Value(alarm->getResolvedByUser());
   }
   else if (!_tcscmp(attr, _T("severity")))
   {
      value = new NXSL_Value(alarm->getCurrentSeverity());
   }
   else if (!_tcscmp(attr, _T("sourceObject")))
   {
      value = new NXSL_Value(alarm->getSourceObject());
   }
   else if (!_tcscmp(attr, _T("state")))
   {
      value = new NXSL_Value(alarm->getState());
   }
   return value;
}

/**
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   setName(_T("DCI"));
}

/**
 * Object destructor
 */
void NXSL_DciClass::onObjectDelete(NXSL_Object *object)
{
   delete (DCObjectInfo *)object->getData();
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   DCObjectInfo *dci;
   NXSL_Value *value = NULL;

   dci = (DCObjectInfo *)object->getData();
   if (!_tcscmp(attr, _T("comments")))
   {
		value = new NXSL_Value(dci->getComments());
   }
   else if (!_tcscmp(attr, _T("dataType")) && (dci->getType() == DCO_TYPE_ITEM))
   {
		value = new NXSL_Value(dci->getDataType());
   }
   else if (!_tcscmp(attr, _T("description")))
   {
		value = new NXSL_Value(dci->getDescription());
   }
   else if (!_tcscmp(attr, _T("errorCount")))
   {
		value = new NXSL_Value(dci->getErrorCount());
   }
   else if (!_tcscmp(attr, _T("id")))
   {
		value = new NXSL_Value(dci->getId());
   }
   else if ((dci->getType() == DCO_TYPE_ITEM) && !_tcscmp(attr, _T("instance")))
   {
		value = new NXSL_Value(dci->getInstance());
   }
   else if (!_tcscmp(attr, _T("lastPollTime")))
   {
		value = new NXSL_Value((INT64)dci->getLastPollTime());
   }
   else if (!_tcscmp(attr, _T("name")))
   {
		value = new NXSL_Value(dci->getName());
   }
   else if (!_tcscmp(attr, _T("origin")))
   {
		value = new NXSL_Value((LONG)dci->getOrigin());
   }
   else if (!_tcscmp(attr, _T("status")))
   {
		value = new NXSL_Value((LONG)dci->getStatus());
   }
   else if (!_tcscmp(attr, _T("systemTag")))
   {
		value = new NXSL_Value(dci->getSystemTag());
   }
   else if (!_tcscmp(attr, _T("template")))
   {
      if (dci->getTemplateId() != 0)
      {
         NetObj *object = FindObjectById(dci->getTemplateId());
         value = (object != NULL) ? object->createNXSLObject() : new NXSL_Value();
      }
      else
      {
         value = new NXSL_Value();
      }
   }
   else if (!_tcscmp(attr, _T("templateId")))
   {
      value = new NXSL_Value(dci->getTemplateId());
   }
   else if (!_tcscmp(attr, _T("templateItemId")))
   {
      value = new NXSL_Value(dci->getTemplateItemId());
   }
   else if (!_tcscmp(attr, _T("type")))
   {
		value = new NXSL_Value((LONG)dci->getType());
   }
   return value;
}

/**
 * Implementation of "SNMP_Transport" class: constructor
 */
NXSL_SNMPTransportClass::NXSL_SNMPTransportClass() : NXSL_Class()
{
	setName(_T("SNMP_Transport"));
}

/**
 * Implementation of "SNMP_Transport" class: get attribute
 */
NXSL_Value *NXSL_SNMPTransportClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
	NXSL_Value *value = NULL;
	SNMP_Transport *t;

	t = (SNMP_Transport*)object->getData();
	if (!_tcscmp(attr, _T("snmpVersion")))
	{
		const TCHAR *versions[] = { _T("1"), _T("2c"), _T("3") };
		value = new NXSL_Value((const TCHAR*)versions[t->getSnmpVersion()]);
	}

	return value;
}

/**
 * Implementation of "SNMP_Transport" class: NXSL object destructor
 */
void NXSL_SNMPTransportClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Transport *)object->getData();
}

/**
 * NXSL class SNMP_VarBind: constructor
 */
NXSL_SNMPVarBindClass::NXSL_SNMPVarBindClass() : NXSL_Class()
{
	setName(_T("SNMP_VarBind"));
}

/**
 * NXSL class SNMP_VarBind: get attribute
 */
NXSL_Value *NXSL_SNMPVarBindClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
	NXSL_Value *value = NULL;
	SNMP_Variable *t = (SNMP_Variable *)object->getData();
	if (!_tcscmp(attr, _T("type")))
	{
		value = new NXSL_Value((UINT32)t->getType());
	}
	else if (!_tcscmp(attr, _T("name")))
	{
		value = new NXSL_Value(t->getName().toString());
	}
	else if (!_tcscmp(attr, _T("value")))
	{
   	TCHAR strValue[1024];
		value = new NXSL_Value(t->getValueAsString(strValue, 1024));
	}
	else if (!_tcscmp(attr, _T("printableValue")))
	{
   	TCHAR strValue[1024];
		bool convToHex = true;
		t->getValueAsPrintableString(strValue, 1024, &convToHex);
		value = new NXSL_Value(strValue);
	}
	else if (!_tcscmp(attr, _T("valueAsIp")))
	{
   	TCHAR strValue[128];
		t->getValueAsIPAddr(strValue);
		value = new NXSL_Value(strValue);
	}
	else if (!_tcscmp(attr, _T("valueAsMac")))
	{
   	TCHAR strValue[128];
		t->getValueAsMACAddr(strValue);
		value = new NXSL_Value(strValue);
	}

	return value;
}

/**
 * NXSL class SNMP_VarBind: NXSL object desctructor
 */
void NXSL_SNMPVarBindClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Variable *)object->getData();
}

/**
 * NXSL class ComponentClass: constructor
 */
NXSL_ComponentClass::NXSL_ComponentClass() : NXSL_Class()
{
   setName(_T("Component"));
}

/**
 * NXSL class ComponentClass: get attribute
 */
NXSL_Value *NXSL_ComponentClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   const UINT32 classCount = 12;
   static const TCHAR *className[classCount] = { _T(""), _T("other"), _T("unknown"), _T("chassis"), _T("backplane"),
                                                 _T("container"), _T("power supply"), _T("fan"), _T("sensor"),
                                                 _T("module"), _T("port"), _T("stack") };
   NXSL_Value *value = NULL;
   Component *component = (Component *)object->getData();
   if (!_tcscmp(attr, _T("class")))
   {
      if (component->getClass() >= classCount)
         value = new NXSL_Value(className[2]); // Unknown class
      else
         value = new NXSL_Value(className[component->getClass()]);
   }
   else if (!_tcscmp(attr, _T("children")))
   {
      value = new NXSL_Value(component->getChildrenForNXSL());
   }
   else if (!_tcscmp(attr, _T("firmware")))
   {
      value = new NXSL_Value(component->getFirmware());
   }
   else if (!_tcscmp(attr, _T("model")))
   {
      value = new NXSL_Value(component->getModel());
   }
   else if (!_tcscmp(attr, _T("name")))
   {
      value = new NXSL_Value(component->getName());
   }
   else if (!_tcscmp(attr, _T("serial")))
   {
      value = new NXSL_Value(component->getSerial());
   }
   else if (!_tcscmp(attr, _T("vendor")))
   {
      value = new NXSL_Value(component->getVendor());
   }
   return value;
}

/**
 * Class objects
 */
NXSL_AlarmClass g_nxslAlarmClass;
NXSL_ChassisClass g_nxslChassisClass;
NXSL_ClusterClass g_nxslClusterClass;
NXSL_ComponentClass g_nxslComponentClass;
NXSL_ContainerClass g_nxslContainerClass;
NXSL_DciClass g_nxslDciClass;
NXSL_EventClass g_nxslEventClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_ZoneClass g_nxslZoneClass;
