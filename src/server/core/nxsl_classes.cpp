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
 * NXSL class NetObj: constructor
 */
NXSL_NetObjClass::NXSL_NetObjClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("NetObj"));
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   NetObj *object;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   object = (NetObj *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(object->Name());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(object->Id());
   }
   else if (!_tcscmp(pszAttr, _T("guid")))
   {
		uuid_t guid;
		object->getGuid(guid);
		TCHAR buffer[128];
		pValue = new NXSL_Value(uuid_to_string(guid, buffer));
   }
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)object->Status());
   }
   else if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      IpToStr(object->IpAddr(), szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!_tcscmp(pszAttr, _T("type")))
   {
      pValue = new NXSL_Value((LONG)object->Type());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(object->getComments());
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
   _tcscpy(m_szName, _T("Node"));

   NXSL_REGISTER_METHOD(enableAgent, 1);
   NXSL_REGISTER_METHOD(enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(enableIcmp, 1);
   NXSL_REGISTER_METHOD(enableSnmp, 1);
   NXSL_REGISTER_METHOD(enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(enableTopologyPolling, 1);
}

/**
 * NXSL class Node: get attribute
 */
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Node *pNode;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   pNode = (Node *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("agentVersion")))
   {
      pValue = new NXSL_Value(pNode->getAgentVersion());
   }
   else if (!_tcscmp(pszAttr, _T("bootTime")))
   {
      pValue = new NXSL_Value((INT64)pNode->getBootTime());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(pNode->getComments());
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
		uuid_t guid;
		pNode->getGuid(guid);
		TCHAR buffer[128];
		pValue = new NXSL_Value(uuid_to_string(guid, buffer));
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(pNode->Id());
   }
   else if (!_tcscmp(pszAttr, _T("ipAddr")))
   {
      IpToStr(pNode->IpAddr(), szBuffer);
      pValue = new NXSL_Value(szBuffer);
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
      pValue = new NXSL_Value(pNode->Name());
   }
   else if (!_tcscmp(pszAttr, _T("platformName")))
   {
      pValue = new NXSL_Value(pNode->getPlatformName());
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
   else if (!_tcscmp(pszAttr, _T("sysDescription")))
   {
      pValue = new NXSL_Value(pNode->getSysDescription());
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
   _tcscpy(m_szName, _T("Interface"));
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
		uuid_t guid;
		iface->getGuid(guid);
		TCHAR buffer[128];
		pValue = new NXSL_Value(uuid_to_string(guid, buffer));
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(iface->Id());
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
		TCHAR buffer[256];
      IpToStr(iface->IpAddr(), buffer);
      pValue = new NXSL_Value(buffer);
   }
   else if (!_tcscmp(pszAttr, _T("ipNetMask")))
   {
		TCHAR buffer[256];
		IpToStr(iface->getIpNetMask(), buffer);
      pValue = new NXSL_Value(buffer);
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
   else if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(iface->Name());
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
			if (g_dwFlags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				Node *peerNode = peerIface->getParentNode();
				if ((parentNode != NULL) && (peerNode != NULL))
				{
					if (peerNode->isTrustedNode(parentNode->Id()))
					{
						pValue = new NXSL_Value(new NXSL_Object(&g_nxslInterfaceClass, peerIface));
					}
					else
					{
						// No access, return null
						pValue = new NXSL_Value;
						DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): access denied for node %s [%d]"),
									 iface->Name(), iface->Id(), peerNode->Name(), peerNode->Id());
					}
				}
				else
				{
					pValue = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=%p peerNode=%p"), iface->Name(), iface->Id(), parentNode, peerNode);
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
			if (g_dwFlags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				if ((parentNode != NULL) && (peerNode->isTrustedNode(parentNode->Id())))
				{
					pValue = new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, peerNode));
				}
				else
				{
					// No access, return null
					pValue = new NXSL_Value;
					DbgPrintf(4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
					          iface->Name(), iface->Id(), peerNode->Name(), peerNode->Id());
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
   else if (!_tcscmp(pszAttr, _T("status")))
   {
      pValue = new NXSL_Value((LONG)iface->Status());
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
 * NXSL class Event: constructor
 */
NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("Event"));
}

/**
 * NXSL class Event: get attribute
 */
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Event *event;
   NXSL_Value *value = NULL;

   event = (Event *)pObject->getData();
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
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("DCI"));
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   DCObject *dci;
   NXSL_Value *value = NULL;

   dci = (DCObject *)object->getData();
   if (!_tcscmp(attr, _T("dataType")) && (dci->getType() == DCO_TYPE_ITEM))
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
	_tcscpy(m_szName, _T("SNMP_Transport"));
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
	_tcscpy(m_szName, _T("SNMP_VarBind"));
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
		value = new NXSL_Value((UINT32)t->GetType());
	}
	else if (!_tcscmp(attr, _T("name")))
	{
		value = new NXSL_Value(t->GetName()->getValueAsText());
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
		t->GetValueAsIPAddr(strValue);
		value = new NXSL_Value(strValue);
	}
	else if (!_tcscmp(attr, _T("valueAsMac")))
	{
   	TCHAR strValue[128];
		t->GetValueAsMACAddr(strValue);
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
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_EventClass g_nxslEventClass;
NXSL_DciClass g_nxslDciClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
