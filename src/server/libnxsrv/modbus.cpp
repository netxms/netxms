/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

/**
 * Get text message from MODBUS operation completion status
 */
const TCHAR LIBNXSRV_EXPORTABLE *GetModbusStatusText(ModbusOperationStatus status)
{
   static const TCHAR *texts[] = { _T("Success"), _T("Communication failure"), _T("Protocol failure"), _T("Timeout") };
   int index = static_cast<int>(status);
   return ((index >= 0) && (index < sizeof(texts) / sizeof(texts[0]))) ? texts[index] : _T("Unknown");
}

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
ModbusOperationStatus ModbusDirectTransport::checkConnection()
{
   return ModbusCheckConnection(m_ipAddress, m_port, m_unitId) ? MODBUS_STATUS_SUCCESS : MODBUS_STATUS_COMM_FAILURE;
}

/**
 * Direct transport - read device identification
 */
ModbusOperationStatus ModbusDirectTransport::readDeviceIdentification(ModbusDeviceIdentification *deviceId)
{
   memset(deviceId, 0, sizeof(ModbusDeviceIdentification));
   return (ModbusOperationStatus)ModbusExecute(m_ipAddress, m_port, m_unitId,
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
               size_t chars = mb_to_wchar(reinterpret_cast<char*>(curr + 2), objectLength, objectValue, 256);
               objectValue[chars] = 0;
            }

            curr += objectLength + 2;
         }

         return MODBUS_STATUS_SUCCESS;
      }, MODBUS_STATUS_COMM_FAILURE);
}

/**
 * Direct transport - read holding registers
 */
ModbusOperationStatus ModbusDirectTransport::readHoldingRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return (ModbusOperationStatus)ModbusExecute(m_ipAddress, m_port, m_unitId,
      [address, conversion, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         if (ModbusReadHoldingRegisters(mb, address, conversion, value, MAX_RESULT_LENGTH) < 1)
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
ModbusOperationStatus ModbusDirectTransport::readInputRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   return (ModbusOperationStatus)ModbusExecute(m_ipAddress, m_port, m_unitId,
      [address, conversion, value] (modbus_t *mb, const char *ipAddrText, uint16_t port, int32_t unitId) -> int32_t
      {
         if (ModbusReadInputRegisters(mb, address, conversion, value, MAX_RESULT_LENGTH) < 1)
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
ModbusOperationStatus ModbusDirectTransport::readCoil(int address, TCHAR *value)
{
   return (ModbusOperationStatus)ModbusExecute(m_ipAddress, m_port, m_unitId,
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
ModbusOperationStatus ModbusDirectTransport::readDiscreteInput(int address, TCHAR *value)
{
   return (ModbusOperationStatus)ModbusExecute(m_ipAddress, m_port, m_unitId,
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

/**
 * Direct transport - check connection
 */
ModbusOperationStatus ModbusProxyTransport::checkConnection()
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.ConnectionStatus(%s,%u,%d)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId);

   TCHAR result[MAX_RESULT_LENGTH];
   uint32_t rcc = m_agentConnection->getParameter(metric, result, MAX_RESULT_LENGTH);
   switch(rcc)
   {
      case ERR_SUCCESS:
         return !_tcscmp(result, _T("1")) ? MODBUS_STATUS_SUCCESS : MODBUS_STATUS_COMM_FAILURE;
      case ERR_REQUEST_TIMEOUT:
         return MODBUS_STATUS_TIMEOUT;
      default:
         return MODBUS_STATUS_COMM_FAILURE;
   }
}

/**
 * Direct transport - read device identification
 */
ModbusOperationStatus ModbusProxyTransport::readDeviceIdentification(ModbusDeviceIdentification *deviceId)
{
   memset(deviceId, 0, sizeof(ModbusDeviceIdentification));

   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.DeviceIdentification(%s,%u,%d,false)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId);

   StringList *objects;
   uint32_t rcc = m_agentConnection->getList(metric, &objects);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ModbusProxyTransport::readDeviceIdentification(%s, %u, %d): request failed (agent error code %d)"), ipAddrText, m_port, m_unitId, rcc);
      return (rcc == ERR_REQUEST_TIMEOUT) ? MODBUS_STATUS_TIMEOUT : MODBUS_STATUS_COMM_FAILURE;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, _T("ModbusProxyTransport::readDeviceIdentification(%s, %u, %d): %d objects successfully retrieved"), ipAddrText, m_port, m_unitId, objects->size());
   for(int i = 0; i < objects->size(); i++)
   {
      const TCHAR *object = objects->get(i);
      const TCHAR *value = _tcschr(object, _T('='));
      if (value == nullptr)
         continue;
      value++;

      TCHAR *eptr;
      int objectId = _tcstol(object, &eptr, 10);
      if (*eptr != _T('='))
         continue;

      switch(objectId)
      {
         case 0:
            _tcslcpy(deviceId->vendorName, value, 256);
            break;
         case 1:
            _tcslcpy(deviceId->productCode, value, 256);
            break;
         case 2:
            _tcslcpy(deviceId->revision, value, 256);
            break;
         case 3:
            _tcslcpy(deviceId->vendorURL, value, 256);
            break;
         case 4:
            _tcslcpy(deviceId->productName, value, 256);
            break;
         case 5:
            _tcslcpy(deviceId->modelName, value, 256);
            break;
         case 6:
            _tcslcpy(deviceId->userApplicationName, value, 256);
            break;
      }
   }

   delete objects;
   return MODBUS_STATUS_SUCCESS;
}

/**
 * Proxy transport - generic reader for MODBUS metric
 */
ModbusOperationStatus ModbusProxyTransport::readMetricFromAgent(const TCHAR *metric, TCHAR *value)
{
   uint32_t rcc = m_agentConnection->getParameter(metric, value, MAX_RESULT_LENGTH);
   nxlog_debug_tag(DEBUG_TAG, 6, _T("ModbusProxyTransport::readMetricFromAgent: request for %s %s (agent status code %d)"), metric, (rcc == ERR_SUCCESS) ? _T("successful") : _T("failed"), rcc);
   switch(rcc)
   {
      case ERR_SUCCESS:
         return MODBUS_STATUS_SUCCESS;
      case ERR_REQUEST_TIMEOUT:
         return MODBUS_STATUS_TIMEOUT;
      default:
         return MODBUS_STATUS_COMM_FAILURE;
   }
}

/**
 * Proxy transport - read holding registers
 */
ModbusOperationStatus ModbusProxyTransport::readHoldingRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.HoldingRegister(%s,%u,%d,%d,%s)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId, address, conversion);
   return readMetricFromAgent(metric, value);
}

/**
 * Proxy transport - read input registers
 */
ModbusOperationStatus ModbusProxyTransport::readInputRegisters(int address, const TCHAR *conversion, TCHAR *value)
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.InputRegister(%s,%u,%d,%d,%s)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId, address, conversion);
   return readMetricFromAgent(metric, value);
}

/**
 * Proxy transport - read coil
 */
ModbusOperationStatus ModbusProxyTransport::readCoil(int address, TCHAR *value)
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.Coil(%s,%u,%d,%d)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId, address);
   return readMetricFromAgent(metric, value);
}

/**
 * Proxy transport - read discrete input
 */
ModbusOperationStatus ModbusProxyTransport::readDiscreteInput(int address, TCHAR *value)
{
   TCHAR metric[256], ipAddrText[64];
   _sntprintf(metric, 256, _T("Modbus.DiscreteInput(%s,%u,%d,%d)"), m_ipAddress.toString(ipAddrText), m_port, m_unitId, address);
   return readMetricFromAgent(metric, value);
}

#endif   /* WITH_MODBUS */
