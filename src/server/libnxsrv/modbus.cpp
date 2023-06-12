/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: modbus.cpp
**/

#include "libnxsrv.h"

#if WITH_MODBUS

#define DEBUG_TAG _T("modbus")

#if HAVE_MODBUS_MODBUS_H
#include <modbus/modbus.h>
#elif HAVE_MODBUS_H
#include <modbus.h>
#endif

/**
 * Direct transport - check connection
 */
MODBUS_OperationStatus MODBUS_DirectTransport::checkConnection()
{
   return MODBUSCheckConnection(m_address, m_port, m_unitId) ? MODBUS_STATUS_SUCCESS : MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Direct transport - read device identification
 */
MODBUS_OperationStatus MODBUS_DirectTransport::readDeviceIdentification(MODBUS_DeviceIdentification *deviceId)
{
   memset(deviceId, 0, sizeof(MODBUS_DeviceIdentification));
   return (MODBUS_OperationStatus)MODBUSExecute(m_address, m_port, m_unitId,
      [deviceId] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         modbus_device_id_response_t d;
         int rc = modbus_read_device_id(mb, MODBUS_READ_DEVICE_ID_REGULAR, 0, &d);
         if (rc < 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): request on REGULAR level failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
            rc = modbus_read_device_id(mb, MODBUS_READ_DEVICE_ID_BASIC, 0, &d);
            if (rc < 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): request on BASIC level failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
               return MODBUS_STATUS_PROTOCOL_ERROR;
            }
         }

         nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): %d objects received"), ipAddrText, port, unitId, d.object_count);

         uint8_t *curr = d.objects;
         for(int i = 0; i < d.object_count; i++)
         {
            int objectId = static_cast<int>(curr[0]);
            int objectLength = static_cast<int>(curr[1]);

            TCHAR *objectValue;
            switch(objectId)
            {
               case 0:
                  objectValue = deviceId->vendorName;
                  break;
               case 1:
                  objectValue = deviceId->productCode;
                  break;
               case 2:
                  objectValue = deviceId->revision;
                  break;
               case 3:
                  objectValue = deviceId->vendorURL;
                  break;
               case 4:
                  objectValue = deviceId->productName;
                  break;
               case 5:
                  objectValue = deviceId->modelName;
                  break;
               case 6:
                  objectValue = deviceId->userApplicationName;
                  break;
               default:
                  objectValue = nullptr;
                  break;
            }
            if (objectValue != nullptr)
            {
#ifdef UNICODE
               size_t chars = mb_to_wchar(reinterpret_cast<char*>(curr + 2), objectLength, objectValue, 256);
               objectValue[chars] = 0;
#else
               memcpy(objectValue, curr + 2, objectLength);
               objectValue[objectLength] = 0;
#endif
            }

            curr += objectLength + 2;
         }

         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

/**
 * Direct transport - read holding registers
 */
MODBUS_OperationStatus MODBUS_DirectTransport::readHoldingRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return (MODBUS_OperationStatus)MODBUSExecute(m_address, m_port, m_unitId,
      [address, conversion, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         if (MODBUSReadHoldingRegisters(mb, address, conversion, value) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("MODBUSReadHoldingRegisters(%hs:%d:%d) failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
            return MODBUS_STATUS_PROTOCOL_ERROR;
         }
         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

/**
 * Direct transport - read input registers
 */
MODBUS_OperationStatus MODBUS_DirectTransport::readInputRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return (MODBUS_OperationStatus)MODBUSExecute(m_address, m_port, m_unitId,
      [address, conversion, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         if (MODBUSReadInputRegisters(mb, address, conversion, value) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("MODBUSReadInputRegisters(%hs:%d:%d) failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
            return MODBUS_STATUS_PROTOCOL_ERROR;
         }
         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

/**
 * Direct transport - read coil
 */
MODBUS_OperationStatus MODBUS_DirectTransport::readCoil(int address, TCHAR *value)
{
   return (MODBUS_OperationStatus)MODBUSExecute(m_address, m_port, m_unitId,
      [address, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         uint8_t coil;
         if (modbus_read_bits(mb, address, 1, &coil) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_bits(%hs:%d:%d) failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
            return MODBUS_STATUS_PROTOCOL_ERROR;
         }

         ret_boolean(value, coil ? true : false);
         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

/**
 * Direct transport - read discrete input
 */
MODBUS_OperationStatus MODBUS_DirectTransport::readDiscreteInput(int address, TCHAR *value)
{
   return (MODBUS_OperationStatus)MODBUSExecute(m_address, m_port, m_unitId,
      [address, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         uint8_t input;
         if (modbus_read_input_bits(mb, address, 1, &input) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_input_bits(%hs:%d:%d) failed (%hs)"), ipAddrText, port, unitId, modbus_strerror(errno));
            return MODBUS_STATUS_PROTOCOL_ERROR;
         }

         ret_boolean(value, input ? true : false);
         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

#else /* WITH_MODBUS */

/**
 * Dummy transport - check connection
 */
MODBUS_OperationStatus MODBUS_DummyTransport::checkConnection()
{
   return MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Dummy transport - read device identification
 */
MODBUS_OperationStatus MODBUS_DummyTransport::readDeviceIdentification(MODBUS_DeviceIdentification *deviceId)
{
   return MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Dummy transport - read holding registers
 */
MODBUS_OperationStatus MODBUS_DummyTransport::readHoldingRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Dummy transport - read input registers
 */
MODBUS_OperationStatus MODBUS_DummyTransport::readInputRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Dummy transport - read coil
 */
MODBUS_OperationStatus MODBUS_DummyTransport::readCoil(int address, TCHAR *value)
{
   return MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Dummy transport - read discrete input
 */
MODBUS_OperationStatus MODBUS_DummyTransport::readDiscreteInput(int address, TCHAR *value)
{
   return MODBUS_STATUS_COMM_FAILURE;
}

#endif   /* WITH_MODBUS */
