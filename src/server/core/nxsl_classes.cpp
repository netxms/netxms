/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
	else
	{
		const TCHAR *attrValue = object->GetCustomAttribute(pszAttr);
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
	else
	{
		const TCHAR *attrValue = pNode->GetCustomAttribute(pszAttr);
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

NXSL_EventClass::NXSL_EventClass()
                :NXSL_Class()
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
// Class objects
//

NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_EventClass g_nxslEventClass;
NXSL_DciClass g_nxslDciClass;
