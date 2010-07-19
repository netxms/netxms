/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
// Implementation of "NetXMS node" class
//

NXSL_NodeClass::NXSL_NodeClass()
               :NXSL_Class()
{
   strcpy(m_szName, "NetXMS_Node");
}

NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Node *pNode;
   NXSL_Value *pValue = NULL;
   char szBuffer[256];

   pNode = (Node *)pObject->getData();
   if (!strcmp(pszAttr, "name"))
   {
      pValue = new NXSL_Value(pNode->Name());
   }
   else if (!strcmp(pszAttr, "id"))
   {
      pValue = new NXSL_Value(pNode->Id());
   }
   else if (!strcmp(pszAttr, "status"))
   {
      pValue = new NXSL_Value((LONG)pNode->Status());
   }
   else if (!strcmp(pszAttr, "ipAddr"))
   {
      IpToStr(pNode->IpAddr(), szBuffer);
      pValue = new NXSL_Value(szBuffer);
   }
   else if (!strcmp(pszAttr, "isAgent"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_NATIVE_AGENT) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSNMP"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isBridge"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isRouter"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isPrinter"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isCDP"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_CDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isSONMP"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_SONMP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "isLLDP"))
   {
      pValue = new NXSL_Value((LONG)((pNode->Flags() & NF_IS_LLDP) ? 1 : 0));
   }
   else if (!strcmp(pszAttr, "snmpVersion"))
   {
      pValue = new NXSL_Value((LONG)pNode->GetSNMPVersion());
   }
   else if (!strcmp(pszAttr, "snmpOID"))
   {
      pValue = new NXSL_Value(pNode->GetSNMPObjectId());
   }
   else if (!strcmp(pszAttr, "agentVersion"))
   {
      pValue = new NXSL_Value(pNode->GetAgentVersion());
   }
   else if (!strcmp(pszAttr, "platformName"))
   {
      pValue = new NXSL_Value(pNode->GetPlatformName());
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
   strcpy(m_szName, "NetXMS_Event");
}

NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   Event *event;
   NXSL_Value *value = NULL;

   event = (Event *)pObject->getData();
   if (!strcmp(pszAttr, "code"))
   {
      value = new NXSL_Value(event->getCode());
   }
   else if (!strcmp(pszAttr, "name"))
   {
		value = new NXSL_Value(event->getName());
   }
   else if (!strcmp(pszAttr, "id"))
   {
      value = new NXSL_Value(event->getId());
   }
   else if (!strcmp(pszAttr, "severity"))
   {
      value = new NXSL_Value(event->getSeverity());
   }
   else if (!strcmp(pszAttr, "timestamp"))
   {
      value = new NXSL_Value((INT64)event->getTimeStamp());
   }
   else if (!strcmp(pszAttr, "message"))
   {
      value = new NXSL_Value(event->getMessage());
   }
   else if (!strcmp(pszAttr, "customMessage"))
   {
		value = new NXSL_Value(event->getCustomMessage());
   }
   else if (!strcmp(pszAttr, "userTag"))
   {
      value = new NXSL_Value(event->getUserTag());
   }
   else if (!strcmp(pszAttr, "parameters"))
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
   strcpy(m_szName, "DCI");
}

NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const TCHAR *attr)
{
   DCItem *dci;
   NXSL_Value *value = NULL;

   dci = (DCItem *)object->getData();
   if (!strcmp(attr, "id"))
   {
		value = new NXSL_Value(dci->getId());
   }
   else if (!strcmp(attr, "name"))
   {
		value = new NXSL_Value(dci->getName());
   }
   else if (!strcmp(attr, "description"))
   {
		value = new NXSL_Value(dci->getDescription());
   }
   else if (!strcmp(attr, "origin"))
   {
		value = new NXSL_Value((LONG)dci->getDataSource());
   }
   else if (!strcmp(attr, "dataType"))
   {
		value = new NXSL_Value((LONG)dci->getDataType());
   }
   else if (!strcmp(attr, "status"))
   {
		value = new NXSL_Value((LONG)dci->getStatus());
   }
   else if (!strcmp(attr, "errorCount"))
   {
		value = new NXSL_Value(dci->getErrorCount());
   }
   else if (!strcmp(attr, "lastPollTime"))
   {
		value = new NXSL_Value((INT64)dci->getLastPollTime());
   }
   else if (!strcmp(attr, "systemTag"))
   {
		value = new NXSL_Value(dci->getSystemTag());
   }
   return value;
}


//
// Class objects
//

NXSL_NodeClass g_nxslNodeClass;
NXSL_EventClass g_nxslEventClass;
NXSL_DciClass g_nxslDciClass;
