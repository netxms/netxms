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
** File: mobile.cpp
**/

#include "nxcore.h"

/**
 * Default constructor
 */
MobileDevice::MobileDevice() : DataCollectionTarget()
{
	m_lastReportTime = 0;
	m_deviceId = NULL;
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_osName = NULL;
	m_osVersion = NULL;
	m_userId = NULL;
	m_batteryLevel = -1;
}

/**
 * Constructor for creating new mobile device object
 */
MobileDevice::MobileDevice(const TCHAR *name, const TCHAR *deviceId) : DataCollectionTarget(name)
{
	m_lastReportTime = 0;
	m_deviceId = _tcsdup(deviceId);
	m_vendor = NULL;
	m_model = NULL;
	m_serialNumber = NULL;
	m_osName = NULL;
	m_osVersion = NULL;
	m_userId = NULL;
	m_batteryLevel = -1;
}

/**
 * Destructor
 */
MobileDevice::~MobileDevice()
{
	safe_free(m_deviceId);
	safe_free(m_vendor);
	safe_free(m_model);
	safe_free(m_serialNumber);
	safe_free(m_osName);
	safe_free(m_osVersion);
	safe_free(m_userId);
}

/**
 * Create object from database data
 */
BOOL MobileDevice::CreateFromDB(DWORD dwId)
{
   m_dwId = dwId;

   if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for cluster object %d"), dwId);
      return FALSE;
   }

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT device_id,vendor,model,serial_number,os_name,os_version,user_id,battery_level FROM mobile_devices WHERE id=%d"), (int)m_dwId);
	DB_RESULT hResult = DBSelect(g_hCoreDB, query);
	if (hResult == NULL)
		return FALSE;

	m_deviceId = DBGetField(hResult, 0, 0, NULL, 0);
	m_vendor = DBGetField(hResult, 0, 1, NULL, 0);
	m_model = DBGetField(hResult, 0, 2, NULL, 0);
	m_serialNumber = DBGetField(hResult, 0, 3, NULL, 0);
	m_osName = DBGetField(hResult, 0, 4, NULL, 0);
	m_osVersion = DBGetField(hResult, 0, 5, NULL, 0);
	m_userId = DBGetField(hResult, 0, 6, NULL, 0);
	m_batteryLevel = DBGetFieldLong(hResult, 0, 7);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB();
   loadItemsFromDB();
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB())
         return FALSE;

   return TRUE;
}

/**
 * Save object to database
 */
BOOL MobileDevice::SaveToDB(DB_HANDLE hdb)
{
   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   BOOL bResult;
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("mobile_devices"), _T("id"), m_dwId))
		hStmt = DBPrepare(hdb, _T("UPDATE mobile_devices SET device_id=?,vendor=?,model=?,serial_number=?,os_name=?,os_version=?,user_id=?,battery_level=? WHERE id=?"));
	else
		hStmt = DBPrepare(hdb, _T("INSERT INTO mobile_devices (device_id,vendor,model,serial_number,os_name,os_version,user_id,battery_level,id) VALUES (?,?,?,?,?,?,?,?,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_deviceId), DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_vendor), DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_model), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_serialNumber), DB_BIND_STATIC);
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_osName), DB_BIND_STATIC);
		DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_osVersion), DB_BIND_STATIC);
		DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(m_userId), DB_BIND_STATIC);
		DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_batteryLevel);
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_dwId);

		bResult = DBExecute(hStmt);

		DBFreeStatement(hStmt);
	}
	else
	{
		bResult = FALSE;
	}

   // Save data collection items
   if (bResult)
   {
		lockDciAccess();
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();
   }

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (bResult)
		m_bIsModified = FALSE;
   UnlockData();

   return bResult;
}

/**
 * Delete object from database
 */
BOOL MobileDevice::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = DataCollectionTarget::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM mobile_devices WHERE id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}

/**
 * Create CSCP message with object's data
 */
void MobileDevice::CreateMessage(CSCPMessage *msg)
{
   DataCollectionTarget::CreateMessage(msg);
	msg->SetVariable(VID_DEVICE_ID, CHECK_NULL_EX(m_deviceId));
	msg->SetVariable(VID_VENDOR, CHECK_NULL_EX(m_vendor));
	msg->SetVariable(VID_MODEL, CHECK_NULL_EX(m_model));
	msg->SetVariable(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_model));
	msg->SetVariable(VID_OS_NAME, CHECK_NULL_EX(m_model));
	msg->SetVariable(VID_OS_VERSION, CHECK_NULL_EX(m_model));
	msg->SetVariable(VID_USER_ID, CHECK_NULL_EX(m_userId));
	msg->SetVariable(VID_BATTERY_LEVEL, (DWORD)m_batteryLevel);
}

/**
 * Modify object from message
 */
DWORD MobileDevice::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return DataCollectionTarget::ModifyFromMessage(pRequest, TRUE);
}
