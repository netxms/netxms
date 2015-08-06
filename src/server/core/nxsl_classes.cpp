/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
 * setStatusCalculation(type, ...)
 */
NXSL_METHOD_DEFINITION(setStatusCalculation)
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
NXSL_METHOD_DEFINITION(setStatusPropagation)
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
   _tcscpy(m_name, _T("NetObj"));

   NXSL_REGISTER_METHOD(setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(setStatusPropagation, -1);
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *pValue = NULL;
   NetObj *object = (NetObj *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(object->getName());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(object->getId());
   }
   else if (!_tcscmp(pszAttr, _T("guid")))
   {
		TCHAR buffer[64];
      pValue = new NXSL_Value(object->getGuid().toString(buffer));
   }
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)object->Status());
   }
   else if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      TCHAR buffer[64];
      GetObjectIpAddress(object).toString(buffer);
      pValue = new NXSL_Value(buffer);
   }
   else if (!_tcscmp(pszAttr, _T("type")))
   {
      pValue = new NXSL_Value((LONG)object->getObjectClass());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(object->getComments());
   }
   else if (!_tcscmp(pszAttr, _T("country")))
   {
      pValue = new NXSL_Value(object->getPostalAddress()->getCountry());
   }
   else if (!_tcscmp(pszAttr, _T("city")))
   {
      pValue = new NXSL_Value(object->getPostalAddress()->getCity());
   }
   else if (!_tcscmp(pszAttr, _T("streetAddress")))
   {
      pValue = new NXSL_Value(object->getPostalAddress()->getStreetAddress());
   }
   else if (!_tcscmp(pszAttr, _T("postcode")))
   {
      pValue = new NXSL_Value(object->getPostalAddress()->getPostCode());
   }
	else
	{
		const TCHAR *attrValue = object->getCustomAttribute(pszAttr);
		if (attrValue != NULL)
		{
			pValue = new NXSL_Value(attrValue);
		}
	}
   return pValue;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_ZoneClass::NXSL_ZoneClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Zone"));

   NXSL_REGISTER_METHOD(setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(setStatusPropagation, -1);
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_ZoneClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *pValue = NULL;
   Zone *zone = (Zone *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("agentProxy")))
   {
      pValue = new NXSL_Value(zone->getAgentProxy());
   }
   else if (!_tcscmp(pszAttr, _T("city")))
   {
      pValue = new NXSL_Value(zone->getPostalAddress()->getCity());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(zone->getComments());
   }
   else if (!_tcscmp(pszAttr, _T("country")))
   {
      pValue = new NXSL_Value(zone->getPostalAddress()->getCountry());
   }
   else if (!_tcscmp(pszAttr, _T("guid")))
   {
		TCHAR buffer[64];
      pValue = new NXSL_Value(zone->getGuid().toString(buffer));
   }
   else if (!_tcscmp(pszAttr, _T("icmpProxy")))
   {
      pValue = new NXSL_Value(zone->getIcmpProxy());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(zone->getId());
   }
   else if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(zone->getName());
   }
   else if (!_tcscmp(pszAttr, _T("postcode")))
   {
      pValue = new NXSL_Value(zone->getPostalAddress()->getPostCode());
   }
   else if (!_tcscmp(pszAttr, _T("snmpProxy")))
   {
      pValue = new NXSL_Value(zone->getSnmpProxy());
   }
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)zone->Status());
   }
   else if (!_tcscmp(pszAttr, _T("streetAddress")))
   {
      pValue = new NXSL_Value(zone->getPostalAddress()->getStreetAddress());
   }
   else if (!_tcscmp(pszAttr, _T("zoneId")))
   {
      pValue = new NXSL_Value(zone->getZoneId());
   }
	else
	{
		const TCHAR *attrValue = zone->getCustomAttribute(pszAttr);
		if (attrValue != NULL)
		{
			pValue = new NXSL_Value(attrValue);
		}
	}
   return pValue;
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
NXSL_METHOD_DEFINITION(enableAgent)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_NXCP);
}

/**
 * enableConfigurationPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(enableConfigurationPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_CONF_POLL);
}

/**
 * enableIcmp(enabled) method
 */
NXSL_METHOD_DEFINITION(enableIcmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ICMP);
}

/**
 * enableSnmp(enabled) method
 */
NXSL_METHOD_DEFINITION(enableSnmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SNMP);
}

/**
 * enableStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(enableStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_STATUS_POLL);
}

/**
 * enableTopologyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(enableTopologyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_TOPOLOGY_POLL);
}

/**
 * NXSL class Node: constructor
 */
NXSL_NodeClass::NXSL_NodeClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Node"));

   NXSL_REGISTER_METHOD(enableAgent, 1);
   NXSL_REGISTER_METHOD(enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(enableIcmp, 1);
   NXSL_REGISTER_METHOD(enableSnmp, 1);
   NXSL_REGISTER_METHOD(enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(enableTopologyPolling, 1);
   NXSL_REGISTER_METHOD(setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(setStatusPropagation, -1);
}

/**
 * NXSL class Node: get attribute
 */
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Node *pNode;
   NXSL_Value *pValue = NULL;

   pNode = (Node *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("agentVersion")))
   {
      pValue = new NXSL_Value(pNode->getAgentVersion());
   }
   else if (!_tcscmp(pszAttr, _T("bootTime")))
   {
      pValue = new NXSL_Value((INT64)pNode->getBootTime());
   }
   else if (!_tcscmp(pszAttr, _T("city")))
   {
      pValue = new NXSL_Value(pNode->getPostalAddress()->getCity());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(pNode->getComments());
   }
   else if (!_tcscmp(pszAttr, _T("country")))
   {
      pValue = new NXSL_Value(pNode->getPostalAddress()->getCountry());
   }
   else if (!_tcscmp(pszAttr, _T("driver")))
   {
      pValue = new NXSL_Value(pNode->getDriverName());
   }
   else if (!_tcscmp(pszAttr, _T("flags")))
   {
		pValue = new NXSL_Value(pNode->getFlags());
   }
   else if (!_tcscmp(pszAttr, _T("guid")))
   {
		TCHAR buffer[64];
      pValue = new NXSL_Value(pNode->getGuid().toString(buffer));
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(pNode->getId());
   }
   else if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      TCHAR buffer[64];
      pNode->getIpAddress().toString(buffer);
      pValue = new NXSL_Value(buffer);
   }
   else if (!_tcscmp(pszAttr, _T("isAgent")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_NATIVE_AGENT) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isBridge")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isCDP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_CDP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isLLDP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_LLDP) ? 1 : 0));
   }
	else if (!_tcscmp(pszAttr, _T("isLocalMgmt")) || !_tcscmp(pszAttr, _T("isLocalManagement")))
	{
		pValue = new NXSL_Value((LONG)((pNode->isLocalManagement()) ? 1 : 0));
	}
   else if (!_tcscmp(pszAttr, _T("isPAE")) || !_tcscmp(pszAttr, _T("is802_1x")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_8021X) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPrinter")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_PRINTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isRouter")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_ROUTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSMCLP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_SMCLP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSNMP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_SNMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSONMP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_SONMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(pNode->getName());
   }
   else if (!_tcscmp(pszAttr, _T("platformName")))
   {
      pValue = new NXSL_Value(pNode->getPlatformName());
   }
   else if (!_tcscmp(pszAttr, _T("postcode")))
   {
      pValue = new NXSL_Value(pNode->getPostalAddress()->getPostCode());
   }
   else if (!_tcscmp(pszAttr, _T("runtimeFlags")))
   {
      pValue = new NXSL_Value(pNode->getRuntimeFlags());
   }
   else if (!_tcscmp(pszAttr, _T("snmpOID")))
   {
      pValue = new NXSL_Value(pNode->getSNMPObjectId());
   }
   else if (!_tcscmp(pszAttr, _T("snmpSysName")))
   {
      pValue = new NXSL_Value(pNode->getSysName());
   }
   else if (!_tcscmp(pszAttr, _T("snmpVersion")))
   {
      pValue = new NXSL_Value((LONG)pNode->getSNMPVersion());
   }
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)pNode->Status());
   }
   else if (!_tcscmp(pszAttr, _T("streetAddress")))
   {
      pValue = new NXSL_Value(pNode->getPostalAddress()->getStreetAddress());
   }
   else if (!_tcscmp(pszAttr, _T("sysDescription")))
   {
      pValue = new NXSL_Value(pNode->getSysDescription());
   }
   else if (!_tcscmp(pszAttr, _T("zone")))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByGUID(pNode->getZoneId());
		   if (zone != NULL)
		   {
			   pValue = new NXSL_Value(new NXSL_Object(&g_nxslZoneClass, zone));
		   }
		   else
		   {
			   pValue = new NXSL_Value;
		   }
	   }
	   else
	   {
		   pValue = new NXSL_Value;
	   }
	}
   else if (!_tcscmp(pszAttr, _T("zoneId")))
	{
      pValue = new NXSL_Value(pNode->getZoneId());
   }
	else
	{
		const TCHAR *attrValue = pNode->getCustomAttribute(pszAttr);
		if (attrValue != NULL)
		{
			pValue = new NXSL_Value(attrValue);
		}
	}
   return pValue;
}

/**
 * NXSL class Interface: constructor
 */
NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Interface"));

   NXSL_REGISTER_METHOD(setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(setStatusPropagation, -1);
}

/**
 * NXSL class Interface: get attribute
 */
NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Interface *iface;
   NXSL_Value *pValue = NULL;

   iface = (Interface *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("adminState")))
   {
		pValue = new NXSL_Value((LONG)iface->getAdminState());
   }
   else if (!_tcscmp(pszAttr, _T("alias")))
   {
      pValue = new NXSL_Value(iface->getAlias());
   }
   else if (!_tcscmp(pszAttr, _T("bridgePortNumber")))
   {
		pValue = new NXSL_Value(iface->getBridgePortNumber());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(iface->getComments());
   }
   else if (!_tcscmp(pszAttr, _T("description")))
   {
      pValue = new NXSL_Value(iface->getDescription());
   }
   else if (!_tcscmp(pszAttr, _T("dot1xBackendAuthState")))
   {
		pValue = new NXSL_Value((LONG)iface->getDot1xBackendAuthState());
   }
   else if (!_tcscmp(pszAttr, _T("dot1xPaeAuthState")))
   {
		pValue = new NXSL_Value((LONG)iface->getDot1xPaeAuthState());
   }
   else if (!_tcscmp(pszAttr, _T("expectedState")))
   {
		pValue = new NXSL_Value((iface->getFlags() & IF_EXPECTED_STATE_MASK) >> 28);
   }
   else if (!_tcscmp(pszAttr, _T("flags")))
   {
		pValue = new NXSL_Value(iface->getFlags());
   }
   else if (!_tcscmp(pszAttr, _T("guid")))
   {
		TCHAR buffer[64];
      pValue = new NXSL_Value(iface->getGuid().toString(buffer));
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(iface->getId());
   }
   else if (!_tcscmp(pszAttr, _T("ifIndex")))
   {
		pValue = new NXSL_Value(iface->getIfIndex());
   }
   else if (!_tcscmp(pszAttr, _T("ifType")))
   {
		pValue = new NXSL_Value(iface->getIfType());
   }
   else if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      TCHAR buffer[64];
      iface->getIpAddressList()->getFirstUnicastAddress().toString(buffer);
      pValue = new NXSL_Value(buffer);
   }
   else if (!_tcscmp(pszAttr, _T("ipNetMask")))
   {
      pValue = new NXSL_Value(iface->getIpAddressList()->getFirstUnicastAddress().getMaskBits());
   }
   else if (!_tcscmp(pszAttr, _T("isExcludedFromTopology")))
   {
      pValue = new NXSL_Value((LONG)(iface->isExcludedFromTopology() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isLoopback")))
   {
		pValue = new NXSL_Value((LONG)(iface->isLoopback() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isManuallyCreated")))
   {
		pValue = new NXSL_Value((LONG)(iface->isManuallyCreated() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPhysicalPort")))
   {
		pValue = new NXSL_Value((LONG)(iface->isPhysicalPort() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("macAddr")))
   {
		TCHAR buffer[256];
		pValue = new NXSL_Value(BinToStr(iface->getMacAddr(), MAC_ADDR_LENGTH, buffer));
   }
   else if (!_tcscmp(pszAttr, _T("mtu")))
   {
      pValue = new NXSL_Value(iface->getMTU());
   }
   else if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(iface->getName());
   }
   else if (!_tcscmp(pszAttr, _T("node")))
	{
		Node *parentNode = iface->getParentNode();
		if (parentNode != NULL)
		{
			pValue = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, parentNode));
		}
		else
		{
			pValue = new NXSL_Value;
		}
	}
   else if (!_tcscmp(pszAttr, _T("operState")))
   {
		pValue = new NXSL_Value((LONG)iface->getOperState());
   }
   else if (!_tcscmp(pszAttr, _T("peerInterface")))
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
						pValue = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, peerIface));
					}
					else
					{
						// No access, return null
						pValue = new NXSL_Value;
						DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): access denied for node %s [%d]"),
									 iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
					}
				}
				else
				{
					pValue = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=%p peerNode=%p"), iface->getName(), iface->getId(), parentNode, peerNode);
				}
			}
			else
			{
				pValue = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, peerIface));
			}
		}
		else
		{
			pValue = new NXSL_Value;
		}
   }
   else if (!_tcscmp(pszAttr, _T("peerNode")))
   {
		Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if (peerNode != NULL)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				if ((parentNode != NULL) && (peerNode->isTrustedNode(parentNode->getId())))
				{
					pValue = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, peerNode));
				}
				else
				{
					// No access, return null
					pValue = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
					          iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
				}
			}
			else
			{
				pValue = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, peerNode));
			}
		}
		else
		{
			pValue = new NXSL_Value;
		}
   }
   else if (!_tcscmp(pszAttr, _T("port")))
   {
      pValue = new NXSL_Value(iface->getPortNumber());
   }
   else if (!_tcscmp(pszAttr, _T("slot")))
   {
      pValue = new NXSL_Value(iface->getSlotNumber());
   }
   else if (!_tcscmp(pszAttr, _T("speed")))
   {
      pValue = new NXSL_Value(iface->getSpeed());
   }
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)iface->Status());
   }
   else if (!_tcscmp(pszAttr, _T("zone")))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByGUID(iface->getZoneId());
		   if (zone != NULL)
		   {
			   pValue = new NXSL_Value(new NXSL_Object(&g_nxslZoneClass, zone));
		   }
		   else
		   {
			   pValue = new NXSL_Value;
		   }
	   }
	   else
	   {
		   pValue = new NXSL_Value;
	   }
	}
   else if (!_tcscmp(pszAttr, _T("zoneId")))
	{
      pValue = new NXSL_Value(iface->getZoneId());
   }
	else
	{
		const TCHAR *attrValue = iface->getCustomAttribute(pszAttr);
		if (attrValue != NULL)
		{
			pValue = new NXSL_Value(attrValue);
		}
	}
   return pValue;
}

/**
 * Event::setMessage() method
 */
NXSL_METHOD_DEFINITION(setMessage)
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
NXSL_METHOD_DEFINITION(setSeverity)
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
NXSL_METHOD_DEFINITION(setUserTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = (Event *)object->getData();
   event->setUserTag(argv[0]->getValueAsCString());
   *result = new NXSL_Value();
   return 0;
}

/**
 * NXSL class Event: constructor
 */
NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Event"));

   NXSL_REGISTER_METHOD(setMessage, 1);
   NXSL_REGISTER_METHOD(setSeverity, 1);
   NXSL_REGISTER_METHOD(setUserTag, 1);
}

/**
 * NXSL class Event: get attribute
 */
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *value = NULL;

   Event *event = (Event *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("code")))
   {
      value = new NXSL_Value(event->getCode());
   }
   else if (!_tcscmp(pszAttr, _T("name")))
   {
		value = new NXSL_Value(event->getName());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      value = new NXSL_Value(event->getId());
   }
   else if (!_tcscmp(pszAttr, _T("severity")))
   {
      value = new NXSL_Value(event->getSeverity());
   }
   else if (!_tcscmp(pszAttr, _T("timestamp")))
   {
      value = new NXSL_Value((INT64)event->getTimeStamp());
   }
   else if (!_tcscmp(pszAttr, _T("message")))
   {
      value = new NXSL_Value(event->getMessage());
   }
   else if (!_tcscmp(pszAttr, _T("customMessage")))
   {
		value = new NXSL_Value(event->getCustomMessage());
   }
   else if (!_tcscmp(pszAttr, _T("userTag")))
   {
      value = new NXSL_Value(event->getUserTag());
   }
   else if (!_tcscmp(pszAttr, _T("parameters")))
   {
		NXSL_Array *array = new NXSL_Array;
		UINT32 i;

		for(i = 0; i < event->getParametersCount(); i++)
			array->set((int)(i + 1), new NXSL_Value(event->getParameter(i)));
      value = new NXSL_Value(array);
   }
   return value;
}

/**
 * Alarm::acknowledge() method
 */
NXSL_METHOD_DEFINITION(acknowledge)
{
   NXC_ALARM *alarm = (NXC_ALARM *)object->getData();
   *result = new NXSL_Value(AckAlarmById(alarm->dwAlarmId, NULL, false, 0));
   return 0;
}

/**
 * Alarm::resolve() method
 */
NXSL_METHOD_DEFINITION(resolve)
{
   NXC_ALARM *alarm = (NXC_ALARM *)object->getData();
   *result = new NXSL_Value(ResolveAlarmById(alarm->dwAlarmId, NULL, false));
   return 0;
}

/**
 * Alarm::terminate() method
 */
NXSL_METHOD_DEFINITION(terminate)
{
   NXC_ALARM *alarm = (NXC_ALARM *)object->getData();
   *result = new NXSL_Value(ResolveAlarmById(alarm->dwAlarmId, NULL, true));
   return 0;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmClass::NXSL_AlarmClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("Alarm"));

   NXSL_REGISTER_METHOD(acknowledge, 0);
   NXSL_REGISTER_METHOD(resolve, 0);
   NXSL_REGISTER_METHOD(terminate, 0);
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmClass::onObjectDelete(NXSL_Object *object)
{
   free(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NXSL_Value *value = NULL;
   NXC_ALARM *alarm = (NXC_ALARM *)pObject->getData();

   if (!_tcscmp(pszAttr, _T("ackBy")))
   {
      value = new NXSL_Value(alarm->dwAckByUser);
   }
   else if (!_tcscmp(pszAttr, _T("creationTime")))
   {
      value = new NXSL_Value(alarm->dwCreationTime);
   }
   else if (!_tcscmp(pszAttr, _T("eventCode")))
   {
      value = new NXSL_Value(alarm->dwSourceEventCode);
   }
   else if (!_tcscmp(pszAttr, _T("eventId")))
   {
      value = new NXSL_Value(alarm->qwSourceEventId);
   }
   else if (!_tcscmp(pszAttr, _T("helpdeskReference")))
   {
      value = new NXSL_Value(alarm->szHelpDeskRef);
   }
   else if (!_tcscmp(pszAttr, _T("helpdeskState")))
   {
      value = new NXSL_Value(alarm->nHelpDeskState);
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      value = new NXSL_Value(alarm->dwAlarmId);
   }
   else if (!_tcscmp(pszAttr, _T("key")))
   {
      value = new NXSL_Value(alarm->szKey);
   }
   else if (!_tcscmp(pszAttr, _T("lastChangeTime")))
   {
      value = new NXSL_Value(alarm->dwLastChangeTime);
   }
   else if (!_tcscmp(pszAttr, _T("message")))
   {
      value = new NXSL_Value(alarm->szMessage);
   }
   else if (!_tcscmp(pszAttr, _T("originalSeverity")))
   {
      value = new NXSL_Value(alarm->nOriginalSeverity);
   }
   else if (!_tcscmp(pszAttr, _T("repeatCount")))
   {
      value = new NXSL_Value(alarm->dwRepeatCount);
   }
   else if (!_tcscmp(pszAttr, _T("resolvedBy")))
   {
      value = new NXSL_Value(alarm->dwResolvedByUser);
   }
   else if (!_tcscmp(pszAttr, _T("severity")))
   {
      value = new NXSL_Value(alarm->nCurrentSeverity);
   }
   else if (!_tcscmp(pszAttr, _T("sourceObject")))
   {
      value = new NXSL_Value(alarm->dwSourceObject);
   }
   else if (!_tcscmp(pszAttr, _T("state")))
   {
      value = new NXSL_Value(alarm->nState);
   }
   return value;
}

/**
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   _tcscpy(m_name, _T("DCI"));
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   DCObject *dci;
   NXSL_Value *value = NULL;

   dci = (DCObject *)object->getData();
   if (!_tcscmp(attr, _T("comments")))
   {
		value = new NXSL_Value(dci->getComments());
   }
   else if (!_tcscmp(attr, _T("dataType")) && (dci->getType() == DCO_TYPE_ITEM))
   {
		value = new NXSL_Value((LONG)((DCItem *)dci)->getDataType());
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
		value = new NXSL_Value(((DCItem *)dci)->getInstance());
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
		value = new NXSL_Value((LONG)dci->getDataSource());
   }
   else if (!_tcscmp(attr, _T("status")))
   {
		value = new NXSL_Value((LONG)dci->getStatus());
   }
   else if (!_tcscmp(attr, _T("systemTag")))
   {
		value = new NXSL_Value(dci->getSystemTag());
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
	_tcscpy(m_name, _T("SNMP_Transport"));
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
	_tcscpy(m_name, _T("SNMP_VarBind"));
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
		value = new NXSL_Value(t->getName()->getValueAsText());
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
 * Class objects
 */
NXSL_AlarmClass g_nxslAlarmClass;
NXSL_DciClass g_nxslDciClass;
NXSL_EventClass g_nxslEventClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_ZoneClass g_nxslZoneClass;
