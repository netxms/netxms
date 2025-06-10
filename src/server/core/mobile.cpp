/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG_MOBILE   _T("mobile")

/**
 * Default constructor
 */
MobileDevice::MobileDevice() : super(Pollable::NONE)
{
   m_commProtocol[0] = 0;
	m_lastReportTime = 0;
	m_batteryLevel = -1;
	m_speed = -1;
	m_direction = -1;
	m_altitude = 0;
}

/**
 * Constructor for creating new mobile device object
 */
MobileDevice::MobileDevice(const TCHAR *name, const TCHAR *deviceId) : super(name, Pollable::NONE), m_deviceId(deviceId)
{
   m_commProtocol[0] = 0;
	m_lastReportTime = 0;
	m_batteryLevel = -1;
   m_speed = -1;
   m_direction = -1;
   m_altitude = 0;
}

/**
 * Destructor
 */
MobileDevice::~MobileDevice()
{
}

/**
 * Create object from database data
 */
bool MobileDevice::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT device_id,vendor,model,serial_number,os_name,os_version,user_id,battery_level,comm_protocol,speed,direction,altitude FROM mobile_devices WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == nullptr)
	{
	   DBFreeStatement(hStmt);
		return false;
	}

	m_deviceId = DBGetFieldAsSharedString(hResult, 0, 0);
	m_vendor = DBGetFieldAsSharedString(hResult, 0, 1);
	m_model = DBGetFieldAsSharedString(hResult, 0, 2);
	m_serialNumber = DBGetFieldAsSharedString(hResult, 0, 3);
	m_osName = DBGetFieldAsSharedString(hResult, 0, 4);
	m_osVersion = DBGetFieldAsSharedString(hResult, 0, 5);
	m_userId = DBGetFieldAsSharedString(hResult, 0, 6);
	m_batteryLevel = static_cast<int8_t>(DBGetFieldLong(hResult, 0, 7));
	DBGetField(hResult, 0, 8, m_commProtocol, 32);
   m_speed = static_cast<float>(DBGetFieldDouble(hResult, 0, 9));
   m_direction = static_cast<int16_t>(DBGetFieldLong(hResult, 0, 10));
   m_altitude = DBGetFieldLong(hResult, 0, 11);

	DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   return true;
}

/**
 * Save object to database
 */
bool MobileDevice::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = {
         _T("device_id") ,_T("vendor"), _T("model"), _T("serial_number"), _T("os_name"), _T("os_version"), _T("user_id"), _T("battery_level"),
         _T("comm_protocol"), _T("speed"), _T("direction"), _T("altitude"), nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("mobile_devices"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();

         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_deviceId, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_model, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_serialNumber, DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_osName, DB_BIND_STATIC);
         DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_osVersion, DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_userId, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_batteryLevel);
         DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_commProtocol, DB_BIND_STATIC);
         DBBind(hStmt, 10, DB_SQLTYPE_DOUBLE, m_speed);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_direction);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_altitude);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);

         unlockProperties();

         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete object from database
 */
bool MobileDevice::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM mobile_devices WHERE id=?"));
   return success;
}

/**
 * Create CSCP message with object's data
 */
void MobileDevice::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);

   msg->setField(VID_COMM_PROTOCOL, m_commProtocol);
	msg->setField(VID_DEVICE_ID, m_deviceId);
	msg->setField(VID_VENDOR, m_vendor);
	msg->setField(VID_MODEL, m_model);
	msg->setField(VID_SERIAL_NUMBER, m_serialNumber);
	msg->setField(VID_OS_NAME, m_osName);
	msg->setField(VID_OS_VERSION, m_osVersion);
	msg->setField(VID_USER_ID, m_userId);
	msg->setField(VID_BATTERY_LEVEL, static_cast<int16_t>(m_batteryLevel));
	msg->setFieldFromTime(VID_LAST_CHANGE_TIME, m_lastReportTime);
   msg->setField(VID_SPEED, m_speed);
   msg->setField(VID_DIRECTION, m_direction);
   msg->setField(VID_ALTITUDE, m_altitude);
}

/**
 * Modify object from message
 */
uint32_t MobileDevice::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Update system information from NXCP message
 */
void MobileDevice::updateSystemInfo(const MobileDeviceInfo& deviceInfo)
{
	lockProperties();
   m_lastReportTime = time(nullptr);
   _tcslcpy(m_commProtocol, deviceInfo.commProtocol, 32);
	m_vendor = deviceInfo.vendor;
	m_model = deviceInfo.model;
	m_serialNumber = deviceInfo.serialNumber;
	m_osName = deviceInfo.osName;
	m_osVersion = deviceInfo.osVersion;
	m_userId = deviceInfo.userId;

	setModified(MODIFY_OTHER | MODIFY_COMMON_PROPERTIES);
	unlockProperties();
}

/**
 * Update status from NXCP message
 */
void MobileDevice::updateStatus(const MobileDeviceStatus& status)
{
	lockProperties();

	if (status.timestamp != 0)
	{
	   if (status.timestamp > m_lastReportTime)
	      m_lastReportTime = status.timestamp;
	}
	else
	{
	   m_lastReportTime = time(nullptr);
	}

	_tcslcpy(m_commProtocol, status.commProtocol, 32);
	m_batteryLevel = status.batteryLevel;
   m_speed = status.speed;
   m_direction = status.direction;
	if (status.geoLocation.isValid())
	{
      m_altitude = status.altitude;
	   updateGeoLocation(status.geoLocation);
   }
	if (!m_ipAddress.equals(status.ipAddress))
	{
	   m_ipAddress = status.ipAddress;
      setModified(MODIFY_COMMON_PROPERTIES);
	}

	TCHAR temp[64];
	nxlog_debug_tag(DEBUG_TAG_MOBILE, 5, _T("Mobile device %s [%u] status update (battery=%d addr=%s timestamp=%u loc=[%s %s] accuracy=%d)"),
	      m_name, m_id, m_batteryLevel, m_ipAddress.toString(temp), static_cast<uint32_t>(status.geoLocation.getTimestamp()),
			status.geoLocation.getLatitudeAsString(), status.geoLocation.getLongitudeAsString(), status.geoLocation.getAccuracy());

	setModified(MODIFY_OTHER);
	unlockProperties();
}

/**
 * Get value for server's internal parameter
 */
DataCollectionError MobileDevice::getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size)
{
   DataCollectionError rc = super::getInternalMetric(name, buffer, size);
	if (rc != DCE_NOT_SUPPORTED)
		return rc;

	rc = DCE_SUCCESS;
   if (!_tcsicmp(name, _T("MobileDevice.Altitude")))
   {
      _sntprintf(buffer, size, _T("%d"), m_altitude);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.BatteryLevel")))
   {
      _sntprintf(buffer, size, _T("%d"), static_cast<int32_t>(m_batteryLevel));
   }
   else if (!_tcsicmp(name, _T("MobileDevice.CommProtocol")))
   {
      _tcslcpy(buffer, m_commProtocol, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.DeviceId")))
   {
		_tcslcpy(buffer, m_deviceId, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.Direction")))
   {
      _sntprintf(buffer, size, _T("%d"), static_cast<int32_t>(m_direction));
   }
   else if (!_tcsicmp(name, _T("MobileDevice.LastReportTime")))
   {
		_sntprintf(buffer, size, INT64_FMT, static_cast<int64_t>(m_lastReportTime));
   }
   else if (!_tcsicmp(name, _T("MobileDevice.Model")))
   {
		_tcslcpy(buffer, m_model, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.OS.Name")))
   {
		_tcslcpy(buffer, m_osName, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.OS.Version")))
   {
		_tcslcpy(buffer, m_osVersion, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.SerialNumber")))
   {
		_tcslcpy(buffer, m_serialNumber, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.Speed")))
   {
      _sntprintf(buffer, size, _T("%f"), m_speed);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.Vendor")))
   {
		_tcslcpy(buffer, m_vendor, size);
   }
   else if (!_tcsicmp(name, _T("MobileDevice.UserId")))
   {
		_tcslcpy(buffer, m_userId, size);
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
void MobileDevice::calculateCompoundStatus(bool forcedRecalc)
{
   NetObj::calculateCompoundStatus(forcedRecalc);

   // Assume normal status by default for mobile device
   lockProperties();
   if (m_status == STATUS_UNKNOWN)
   {
      m_status = STATUS_NORMAL;
      setModified(MODIFY_RUNTIME);
   }
   unlockProperties();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *MobileDevice::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslMobileDeviceClass, new shared_ptr<MobileDevice>(self())));
}

/**
 * Serialize object to JSON
 */
json_t *MobileDevice::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "altitude", json_integer(m_altitude));
   json_object_set_new(root, "batteryLevel", json_integer(m_batteryLevel));
   json_object_set_new(root, "commProtocol", json_string_t(m_commProtocol));
   json_object_set_new(root, "deviceId", json_string_t(m_deviceId));
   json_object_set_new(root, "direction", json_integer(m_direction));
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "lastReportTime", json_integer(m_lastReportTime));
   json_object_set_new(root, "model", json_string_t(m_model));
   json_object_set_new(root, "osName", json_string_t(m_osName));
   json_object_set_new(root, "osVersion", json_string_t(m_osVersion));
   json_object_set_new(root, "serialNumber", json_string_t(m_serialNumber));
   json_object_set_new(root, "speed", json_real(m_speed));
   json_object_set_new(root, "userId", json_string_t(m_userId));
   json_object_set_new(root, "vendor", json_string_t(m_vendor));
   unlockProperties();

   return root;
}
