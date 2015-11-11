/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
bool MobileDevice::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   m_id = dwId;

   if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for mobile device object %d"), dwId);
      return false;
   }

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT device_id,vendor,model,serial_number,os_name,os_version,user_id,battery_level FROM mobile_devices WHERE id=%d"), (int)m_id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == NULL)
		return false;

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
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         return false;

   return true;
}

/**
 * Save object to database
 */
BOOL MobileDevice::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   BOOL bResult;
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("mobile_devices"), _T("id"), m_id))
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
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);

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
		lockDciAccess(false);
      for(int i = 0; i < m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();
   }

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (bResult)
		m_isModified = false;
   unlockProperties();

   return bResult;
}

/**
 * Delete object from database
 */
bool MobileDevice::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = DataCollectionTarget::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM mobile_devices WHERE id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void MobileDevice::fillMessageInternal(NXCPMessage *msg)
{
   DataCollectionTarget::fillMessageInternal(msg);

	msg->setField(VID_DEVICE_ID, CHECK_NULL_EX(m_deviceId));
	msg->setField(VID_VENDOR, CHECK_NULL_EX(m_vendor));
	msg->setField(VID_MODEL, CHECK_NULL_EX(m_model));
	msg->setField(VID_SERIAL_NUMBER, CHECK_NULL_EX(m_serialNumber));
	msg->setField(VID_OS_NAME, CHECK_NULL_EX(m_osName));
	msg->setField(VID_OS_VERSION, CHECK_NULL_EX(m_osVersion));
	msg->setField(VID_USER_ID, CHECK_NULL_EX(m_userId));
	msg->setField(VID_BATTERY_LEVEL, (UINT32)m_batteryLevel);
	msg->setField(VID_LAST_CHANGE_TIME, (QWORD)m_lastReportTime);
}

/**
 * Modify object from message
 */
UINT32 MobileDevice::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return DataCollectionTarget::modifyFromMessageInternal(pRequest);
}

/**
 * Update system information from NXCP message
 */
void MobileDevice::updateSystemInfo(NXCPMessage *msg)
{
	lockProperties();

	m_lastReportTime = time(NULL);

	safe_free(m_vendor);
	m_vendor = msg->getFieldAsString(VID_VENDOR);

	safe_free(m_model);
	m_model = msg->getFieldAsString(VID_MODEL);

	safe_free(m_serialNumber);
	m_serialNumber = msg->getFieldAsString(VID_SERIAL_NUMBER);

	safe_free(m_osName);
	m_osName = msg->getFieldAsString(VID_OS_NAME);

	safe_free(m_osVersion);
	m_osVersion = msg->getFieldAsString(VID_OS_VERSION);

	safe_free(m_userId);
	m_userId = msg->getFieldAsString(VID_USER_NAME);

	setModified();
	unlockProperties();
}

/**
 * Update status from NXCP message
 */
void MobileDevice::updateStatus(NXCPMessage *msg)
{
	lockProperties();

	m_lastReportTime = time(NULL);

   int type = msg->getFieldType(VID_BATTERY_LEVEL);
   if (type == NXCP_DT_INT32)
		m_batteryLevel = msg->getFieldAsInt32(VID_BATTERY_LEVEL);
   else if (type == NXCP_DT_INT16)
		m_batteryLevel = (int)msg->getFieldAsInt16(VID_BATTERY_LEVEL);
	else
		m_batteryLevel = -1;

	if (msg->isFieldExist(VID_GEOLOCATION_TYPE))
	{
		m_geoLocation = GeoLocation(*msg);
		addLocationToHistory();
   }

	if (msg->isFieldExist(VID_IP_ADDRESS))
		m_ipAddress = msg->getFieldAsInetAddress(VID_IP_ADDRESS);

	TCHAR temp[64];
	DbgPrintf(5, _T("Mobile device %s [%d] updated from agent (battery=%d addr=%s loc=[%s %s])"),
	          m_name, (int)m_id, m_batteryLevel, m_ipAddress.toString(temp),
				 m_geoLocation.getLatitudeAsString(), m_geoLocation.getLongitudeAsString());

	setModified();
	unlockProperties();
}

/**
 * Get value for server's internal parameter
 */
UINT32 MobileDevice::getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
	UINT32 rc = DataCollectionTarget::getInternalItem(param, bufSize, buffer);
	if (rc != DCE_NOT_SUPPORTED)
		return rc;
	rc = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("MobileDevice.BatteryLevel")))
   {
      _sntprintf(buffer, bufSize, _T("%d"), m_batteryLevel);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.DeviceId")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_deviceId), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.LastReportTime")))
   {
		_sntprintf(buffer, bufSize, INT64_FMT, (INT64)m_lastReportTime);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.Model")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_model), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.OS.Name")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_osName), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.OS.Version")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_osVersion), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.SerialNumber")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_serialNumber), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.Vendor")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_vendor), bufSize);
   }
   else if (!_tcsicmp(param, _T("MobileDevice.UserId")))
   {
		nx_strncpy(buffer, CHECK_NULL_EX(m_userId), bufSize);
   }
   else
   {
      rc = DCE_NOT_SUPPORTED;
   }

   return rc;
}

/**
 * Calculate compound status
 */
void MobileDevice::calculateCompoundStatus(BOOL bForcedRecalc)
{
   NetObj::calculateCompoundStatus(bForcedRecalc);

   // Assume normal status by default for mobile device
   if (m_iStatus == STATUS_UNKNOWN)
   {
      lockProperties();
      m_iStatus = STATUS_NORMAL;
      setModified();
      unlockProperties();
   }
}
