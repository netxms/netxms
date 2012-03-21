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
** File: zone.cpp
**
**/

#include "nxcore.h"


//
// Zone class default constructor
//

Zone::Zone()
     :NetObj()
{
   m_dwId = 0;
   m_zoneId = 0;
   _tcscpy(m_szName, _T("Default"));
   m_agentProxy = 0;
   m_snmpProxy = 0;
	m_icmpProxy = 0;
	m_idxNodeByAddr = new ObjectIndex;
	m_idxInterfaceByAddr = new ObjectIndex;
	m_idxSubnetByAddr = new ObjectIndex;
}


//
// Constructor for new zone object
//

Zone::Zone(DWORD zoneId, const TCHAR *name)
     :NetObj()
{
   m_dwId = 0;
   m_zoneId = zoneId;
   nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
   m_agentProxy = 0;
   m_snmpProxy = 0;
	m_icmpProxy = 0;
	m_idxNodeByAddr = new ObjectIndex;
	m_idxInterfaceByAddr = new ObjectIndex;
	m_idxSubnetByAddr = new ObjectIndex;
}


//
// Zone class destructor
//

Zone::~Zone()
{
	delete m_idxNodeByAddr;
	delete m_idxInterfaceByAddr;
	delete m_idxSubnetByAddr;
}


//
// Create object from database data
//

BOOL Zone::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT zone_guid,agent_proxy,snmp_proxy,icmp_proxy FROM zones WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      if (dwId == BUILTIN_OID_ZONE0)
      {
         m_zoneId = 0;
         return TRUE;
      }
      else
      {
			DbgPrintf(4, _T("Cannot load zone object %ld - missing record in \"zones\" table"), (long)m_dwId);
         return FALSE;
      }
   }

   m_zoneId = DBGetFieldULong(hResult, 0, 0);
   m_agentProxy = DBGetFieldULong(hResult, 0, 1);
   m_snmpProxy = DBGetFieldULong(hResult, 0, 2);
   m_icmpProxy = DBGetFieldULong(hResult, 0, 3);

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB();

   return TRUE;
}


//
// Save object to database
//

BOOL Zone::SaveToDB(DB_HANDLE hdb)
{
   BOOL bNewObject = TRUE;
   TCHAR szQuery[8192];
   DB_RESULT hResult;

   LockData();

   saveCommonProperties(hdb);
   
   // Check for object's existence in database
   _sntprintf(szQuery, 8192, _T("SELECT id FROM zones WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      _sntprintf(szQuery, 8192, _T("INSERT INTO zones (id,zone_guid,agent_proxy,snmp_proxy,icmp_proxy)")
                          _T(" VALUES (%d,%d,%d,%d,%d)"),
                 m_dwId, m_zoneId, m_agentProxy, m_snmpProxy, m_icmpProxy);
   else
      _sntprintf(szQuery, 8192, _T("UPDATE zones SET zone_guid=%d,agent_proxy=%d,")
                                _T("snmp_proxy=%d,icmp_proxy=%d WHERE id=%d"),
                 m_zoneId, m_agentProxy, m_snmpProxy, m_icmpProxy, m_dwId);
   DBQuery(hdb, szQuery);

   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_bIsModified = FALSE;
   UnlockData();
   return TRUE;
}


//
// Delete zone object from database
//

BOOL Zone::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM zones WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Create CSCP message with object's data
//

void Zone::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
   pMsg->SetVariable(VID_AGENT_PROXY, m_agentProxy);
   pMsg->SetVariable(VID_SNMP_PROXY, m_snmpProxy);
   pMsg->SetVariable(VID_ICMP_PROXY, m_icmpProxy);
}


//
// Modify object from message
//

DWORD Zone::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

	if (pRequest->IsVariableExist(VID_AGENT_PROXY))
		m_agentProxy = pRequest->GetVariableLong(VID_AGENT_PROXY);

	if (pRequest->IsVariableExist(VID_SNMP_PROXY))
		m_snmpProxy = pRequest->GetVariableLong(VID_SNMP_PROXY);

	if (pRequest->IsVariableExist(VID_ICMP_PROXY))
		m_icmpProxy = pRequest->GetVariableLong(VID_ICMP_PROXY);

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Update interface index
 */
void Zone::updateInterfaceIndex(DWORD oldIp, DWORD newIp, Interface *iface)
{
	m_idxInterfaceByAddr->remove(oldIp);
	m_idxInterfaceByAddr->put(newIp, iface);
}
