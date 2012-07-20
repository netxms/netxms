/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Implementation of "NetXMS object" class
//

NXSL_NetObjClass::NXSL_NetObjClass()
                 :NXSL_Class()
{
   _tcscpy(m_szName, _T("NetObj"));
}

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


//
// Implementation of "NetXMS node" class
//

NXSL_NodeClass::NXSL_NodeClass()
               :NXSL_Class()
{
   _tcscpy(m_szName, _T("Node"));
}

NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Node *pNode;
   NXSL_Value *pValue = NULL;
   TCHAR szBuffer[256];

   pNode = (Node *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(pNode->Name());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(pNode->Id());
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
      pValue = new NXSL_Value((LONG)pNode->Status());
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
   else if (!_tcscmp(pszAttr, _T("isSNMP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_SNMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isBridge")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isRouter")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_ROUTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPrinter")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_PRINTER) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isCDP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_CDP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isSONMP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_SONMP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isLLDP")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_LLDP) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPAE")) || !_tcscmp(pszAttr, _T("is802_1x")))
   {
      pValue = new NXSL_Value((LONG)((pNode->getFlags() & NF_IS_8021X) ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("snmpVersion")))
   {
      pValue = new NXSL_Value((LONG)pNode->getSNMPVersion());
   }
   else if (!_tcscmp(pszAttr, _T("snmpOID")))
   {
      pValue = new NXSL_Value(pNode->getSNMPObjectId());
   }
   else if (!_tcscmp(pszAttr, _T("agentVersion")))
   {
      pValue = new NXSL_Value(pNode->getAgentVersion());
   }
   else if (!_tcscmp(pszAttr, _T("platformName")))
   {
      pValue = new NXSL_Value(pNode->getPlatformName());
   }
   else if (!_tcscmp(pszAttr, _T("snmpSysName")))
   {
      pValue = new NXSL_Value(pNode->getSysName());
   }
   else if (!_tcscmp(pszAttr, _T("sysDescription")))
   {
      pValue = new NXSL_Value(pNode->getSysDescription());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(pNode->getComments());
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


//
// Implementation of "NetXMS interface" class
//

NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("Interface"));
}

NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Interface *iface;
   NXSL_Value *pValue = NULL;

   iface = (Interface *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      pValue = new NXSL_Value(iface->Name());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      pValue = new NXSL_Value(iface->Id());
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
      pValue = new NXSL_Value((LONG)iface->Status());
   }
   else if (!_tcscmp(pszAttr, _T("macAddr")))
   {
		TCHAR buffer[256];
		pValue = new NXSL_Value(BinToStr(iface->getMacAddr(), MAC_ADDR_LENGTH, buffer));
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
   else if (!_tcscmp(pszAttr, _T("isLoopback")))
   {
		pValue = new NXSL_Value((LONG)(iface->isLoopback() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isPhysicalPort")))
   {
		pValue = new NXSL_Value((LONG)(iface->isPhysicalPort() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isManuallyCreated")))
   {
		pValue = new NXSL_Value((LONG)(iface->isManuallyCreated() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("isExcludedFromTopology")))
   {
      pValue = new NXSL_Value((LONG)(iface->isExcludedFromTopology() ? 1 : 0));
   }
   else if (!_tcscmp(pszAttr, _T("description")))
   {
      pValue = new NXSL_Value(iface->getDescription());
   }
   else if (!_tcscmp(pszAttr, _T("comments")))
   {
      pValue = new NXSL_Value(iface->getComments());
   }
   else if (!_tcscmp(pszAttr, _T("ifIndex")))
   {
		pValue = new NXSL_Value(iface->getIfIndex());
   }
   else if (!_tcscmp(pszAttr, _T("ifType")))
   {
		pValue = new NXSL_Value(iface->getIfType());
   }
   else if (!_tcscmp(pszAttr, _T("bridgePortNumber")))
   {
		pValue = new NXSL_Value(iface->getBridgePortNumber());
   }
   else if (!_tcscmp(pszAttr, _T("adminState")))
   {
		pValue = new NXSL_Value((LONG)iface->getAdminState());
   }
   else if (!_tcscmp(pszAttr, _T("operState")))
   {
		pValue = new NXSL_Value((LONG)iface->getOperState());
   }
   else if (!_tcscmp(pszAttr, _T("dot1xPaeAuthState")))
   {
		pValue = new NXSL_Value((LONG)iface->getDot1xPaeAuthState());
   }
   else if (!_tcscmp(pszAttr, _T("dot1xBackendAuthState")))
   {
		pValue = new NXSL_Value((LONG)iface->getDot1xBackendAuthState());
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
   else if (!_tcscmp(pszAttr, _T("peerNode")))
   {
		Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if (peerNode != NULL)
		{
			if (g_dwFlags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				if ((parentNode != NULL) && (peerNode->IsTrustedNode(parentNode->Id())))
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
					if (peerNode->IsTrustedNode(parentNode->Id()))
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
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=%p peerNode=%p"), parentNode, peerNode);
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


//
// Implementation of "NetXMS event" class
//

NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   _tcscpy(m_szName, _T("Event"));
}

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
		DWORD i;

		for(i = 0; i < event->getParametersCount(); i++)
			array->set((int)(i + 1), new NXSL_Value(event->getParameter(i)));
      value = new NXSL_Value(array);
   }
   return value;
}


//
// Implementation of "DCI" class
//

NXSL_DciClass::NXSL_DciClass()
              :NXSL_Class()
{
   _tcscpy(m_szName, _T("DCI"));
}

NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   DCItem *dci;
   NXSL_Value *value = NULL;

   dci = (DCItem *)object->getData();
   if (!_tcscmp(attr, _T("id")))
   {
		value = new NXSL_Value(dci->getId());
   }
   else if (!_tcscmp(attr, _T("name")))
   {
		value = new NXSL_Value(dci->getName());
   }
   else if (!_tcscmp(attr, _T("description")))
   {
		value = new NXSL_Value(dci->getDescription());
   }
   else if (!_tcscmp(attr, _T("origin")))
   {
		value = new NXSL_Value((LONG)dci->getDataSource());
   }
   else if (!_tcscmp(attr, _T("dataType")))
   {
		value = new NXSL_Value((LONG)dci->getDataType());
   }
   else if (!_tcscmp(attr, _T("status")))
   {
		value = new NXSL_Value((LONG)dci->getStatus());
   }
   else if (!_tcscmp(attr, _T("errorCount")))
   {
		value = new NXSL_Value(dci->getErrorCount());
   }
   else if (!_tcscmp(attr, _T("lastPollTime")))
   {
		value = new NXSL_Value((INT64)dci->getLastPollTime());
   }
   else if (!_tcscmp(attr, _T("systemTag")))
   {
		value = new NXSL_Value(dci->getSystemTag());
   }
   return value;
}

//
// Implementation of "SNMP_Transport" class
//

NXSL_SNMPTransportClass::NXSL_SNMPTransportClass() : NXSL_Class()
{
	_tcscpy(m_szName, _T("SNMP_Transport"));
}

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

void NXSL_SNMPTransportClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Transport *)object->getData();
}


//
// Implementation of "SNMP_VarBind" class
//

NXSL_SNMPVarBindClass::NXSL_SNMPVarBindClass() : NXSL_Class()
{
	_tcscpy(m_szName, _T("SNMP_VarBind"));
}

NXSL_Value *NXSL_SNMPVarBindClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
	NXSL_Value *value = NULL;
	SNMP_Variable *t;
	TCHAR strValue[1024];

	t = (SNMP_Variable *)object->getData();
	if (!_tcscmp(attr, _T("type")))
	{
		value = new NXSL_Value((DWORD)t->GetType());
	}
	else if (!_tcscmp(attr, _T("name")))
	{
		value = new NXSL_Value(t->GetName()->getValueAsText());
	}
	else if (!_tcscmp(attr, _T("value")))
	{
		value = new NXSL_Value(t->GetValueAsString(strValue, 1024));
	}
	else if (!_tcscmp(attr, _T("printableValue")))
	{
		bool convToHex = true;
		t->getValueAsPrintableString(strValue, 1024, &convToHex);
		value = new NXSL_Value(t->GetValueAsString(strValue, 1024));
	}
	else if (!_tcscmp(attr, _T("valueAsIp")))
	{
		t->GetValueAsIPAddr(strValue);
		value = new NXSL_Value(strValue);
	}
	else if (!_tcscmp(attr, _T("valueAsMac")))
	{
		t->GetValueAsMACAddr(strValue);
		value = new NXSL_Value(strValue);
	}

	return value;
}

void NXSL_SNMPVarBindClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Variable *)object->getData();
}


//
// Class objects
//

NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_EventClass g_nxslEventClass;
NXSL_DciClass g_nxslDciClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
