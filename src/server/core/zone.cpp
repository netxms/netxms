/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: zone.cpp
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
   m_dwZoneGUID = 0;
   m_dwControllerIpAddr = 0;
   _tcscpy(m_szName, _T("Zone 0"));
   m_pszDescription = _tcsdup(_T(""));
   m_dwAddrListSize = 0;
   m_pdwIpAddrList = NULL;
   m_iZoneType = ZONE_TYPE_ACTIVE;
}


//
// Zone class destructor
//

Zone::~Zone()
{
   safe_free(m_pszDescription);
   safe_free(m_pdwIpAddrList);
}


//
// Create object from database data
//

BOOL Zone::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD i;

   m_dwId = dwId;

   if (!LoadCommonProperties())
      return FALSE;

   _stprintf(szQuery, _T("SELECT zone_guid,zone_type,controller_ip,"
                         "description FROM zones WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      if (dwId == BUILTIN_OID_ZONE0)
      {
         m_dwZoneGUID = 0;
         m_iZoneType = ZONE_TYPE_ACTIVE;
         safe_free(m_pszDescription);
         m_pszDescription = _tcsdup(_T("Built-in default zone object"));
         return TRUE;
      }
      else
      {
         return FALSE;
      }
   }

   safe_free(m_pszDescription);

   m_dwZoneGUID = DBGetFieldULong(hResult, 0, 0);
   m_iZoneType = DBGetFieldLong(hResult, 0, 1);
   m_dwControllerIpAddr = DBGetFieldIPAddr(hResult, 0, 2);
   m_pszDescription = _tcsdup(DBGetField(hResult, 0, 3));
   DecodeSQLString(m_pszDescription);

   DBFreeResult(hResult);

   // Load IP address list
   if (m_iZoneType == ZONE_TYPE_PASSIVE)
   {
      _stprintf(szQuery, _T("SELECT ip_addr FROM zone_ip_addr_list WHERE zone_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         m_dwAddrListSize = DBGetNumRows(hResult);
         m_pdwIpAddrList = (DWORD *)malloc(sizeof(DWORD) * m_dwAddrListSize);
         for(i = 0; i < m_dwAddrListSize; i++)
            m_pdwIpAddrList[i] = DBGetFieldIPAddr(hResult, i, 0);
         DBFreeResult(hResult);
      }
   }

   // Load access list
   LoadACLFromDB();

   return TRUE;
}


//
// Save object to database
//

BOOL Zone::SaveToDB(DB_HANDLE hdb)
{
   BOOL bNewObject = TRUE;
   TCHAR *pszEscDescr, szIpAddr[16], szQuery[8192];
   DB_RESULT hResult;
   DWORD i;

   LockData();

   SaveCommonProperties(hdb);
   
   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM zones WHERE id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   pszEscDescr = EncodeSQLString(m_pszDescription);
   if (bNewObject)
      _sntprintf(szQuery, 8192, "INSERT INTO zones (id,zone_guid,zone_type,controller_ip,"
                          "description) VALUES (%d,%d,%d,'%s','%s')",
                 m_dwId, m_dwZoneGUID, m_iZoneType,
                 IpToStr(m_dwControllerIpAddr, szIpAddr), pszEscDescr);
   else
      _sntprintf(szQuery, 8192, "UPDATE zones SET zone_guid=%d,zone_type=%d,"
                                "controller_ip='%s',description='%s' WHERE id=%d",
                 m_dwZoneGUID, m_iZoneType,
                 IpToStr(m_dwControllerIpAddr, szIpAddr), pszEscDescr, m_dwId);
   free(pszEscDescr);
   DBQuery(hdb, szQuery);

   // Save ip address list
   _stprintf(szQuery, _T("DELETE FROM zone_ip_addr_list WHERE zone_id=%d"), m_dwId);
   DBQuery(hdb, szQuery);
   if (m_iZoneType == ZONE_TYPE_PASSIVE)
   {
      for(i = 0; i < m_dwAddrListSize; i++)
      {
         _stprintf(szQuery, _T("INSERT INTO zone_ip_addr_list (zone_id,ip_addr) VALUES (%d,'%s')"),
                   m_dwId, IpToStr(m_pdwIpAddrList[i], szIpAddr));
         DBQuery(hdb, szQuery);
      }
   }
   
   SaveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_bIsModified = FALSE;
   UnlockData();
   return TRUE;
}


//
// Delete zone object from database
//

BOOL Zone::DeleteFromDB(void)
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _stprintf(szQuery, _T("DELETE FROM zones WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _stprintf(szQuery, _T("DELETE FROM zone_ip_addr_list WHERE zone_id=%d"), m_dwId);
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
   pMsg->SetVariable(VID_ZONE_GUID, m_dwZoneGUID);
   pMsg->SetVariable(VID_ZONE_TYPE, (WORD)m_iZoneType);
   pMsg->SetVariable(VID_CONTROLLER_IP_ADDR, m_dwControllerIpAddr);
   pMsg->SetVariable(VID_ADDR_LIST_SIZE, m_dwAddrListSize);
   pMsg->SetVariableToInt32Array(VID_IP_ADDR_LIST, m_dwAddrListSize, m_pdwIpAddrList);
   pMsg->SetVariable(VID_DESCRIPTION, m_pszDescription);
}


//
// Modify object from message
//

DWORD Zone::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change description
   if (pRequest->IsVariableExist(VID_DESCRIPTION))
   {
      safe_free(m_pszDescription);
      m_pszDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
   }

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}
