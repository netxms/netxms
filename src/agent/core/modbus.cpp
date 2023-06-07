/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Raden Solutions
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
** File: modbus.cpp
**
**/

#include "nxagentd.h"

#if WITH_MODBUS

#define DEBUG_TAG _T("modbus")

#if HAVE_MODBUS_MODBUS_H
#include <modbus/modbus.h>
#elif HAVE_MODBUS_H
#include <modbus.h>
#endif

/**
 * Common handler for MODBUS.* metrics
 */
static LONG ReadMODBUSMetric(const TCHAR *metric, std::function<int32_t (modbus_t*, const char*, uint16_t, int32_t, int32_t)> reader)
{
   if (!(g_dwFlags & AF_ENABLE_MODBUS_PROXY))
      return SYSINFO_RC_ACCESS_DENIED;

   InetAddress addr = AgentGetMetricArgAsInetAddress(metric, 1);
   uint16_t port;
   int32_t unitId, modbusAddress;
   if ((!addr.isValidUnicast() && !addr.isLoopback()) ||
       !AgentGetMetricArgAsUInt16(metric, 2, &port, MODBUS_TCP_DEFAULT_PORT) ||
       !AgentGetMetricArgAsInt32(metric, 3, &unitId, 1) ||
       !AgentGetMetricArgAsInt32(metric, 4, &modbusAddress, 0))
      return SYSINFO_RC_UNSUPPORTED;
   if ((unitId < 1) || (unitId > 255) || (modbusAddress < 0))
      return SYSINFO_RC_UNSUPPORTED;

   return MODBUSExecute(addr, port, unitId,
      [reader, modbusAddress] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId) -> int32_t
      {
         return reader(mb, addrText, port, unitId, modbusAddress);
      });
}

/**
 * Handler for metric MODBUS.HoldingRegister
 */
LONG H_MODBUSHoldingRegister(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR conversion[64];
   if (!AgentGetMetricArg(metric, 5, conversion, 64))
      return SYSINFO_RC_UNSUPPORTED;

   return ReadMODBUSMetric(metric,
      [conversion, value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         if (MODBUSReadHoldingRegisters(mb, modbusAddress, conversion, value) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("MODBUSReadHoldingRegisters(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }
         return SYSINFO_RC_SUCCESS;
      });
}

/**
 * Handler for metric MODBUS.InputRegister
 */
LONG H_MODBUSInputRegister(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR conversion[64];
   if (!AgentGetMetricArg(metric, 5, conversion, 64))
      return SYSINFO_RC_UNSUPPORTED;

   return ReadMODBUSMetric(metric,
      [conversion, value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         if (MODBUSReadInputRegisters(mb, modbusAddress, conversion, value) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("MODBUSReadInputRegisters(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }
         return SYSINFO_RC_SUCCESS;
      });
}

/**
 * Handler for metric MODBUS.Coil
 */
LONG H_MODBUSCoil(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return ReadMODBUSMetric(metric,
      [value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         uint8_t coil;
         if (modbus_read_bits(mb, modbusAddress, 1, &coil) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_bits(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }

         ret_boolean(value, coil ? true : false);
         return SYSINFO_RC_SUCCESS;
      });
}

/**
 * Handler for metric MODBUS.DiscreteInput
 */
LONG H_MODBUSDiscreteInput(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return ReadMODBUSMetric(metric,
      [value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         uint8_t coil;
         if (modbus_read_input_bits(mb, modbusAddress, 1, &coil) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_input_bits(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }

         ret_boolean(value, coil ? true : false);
         return SYSINFO_RC_SUCCESS;
      });
}

/**
 * Handler for MODBUS.DeviceIdentification list
 */
LONG H_MODBUSDeviceIdentification(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   if (!(g_dwFlags & AF_ENABLE_MODBUS_PROXY))
      return SYSINFO_RC_ACCESS_DENIED;

   InetAddress addr = AgentGetMetricArgAsInetAddress(metric, 1);
   uint16_t port;
   int32_t unitId;
   if ((!addr.isValidUnicast() && !addr.isLoopback()) ||
       !AgentGetMetricArgAsUInt16(metric, 2, &port, MODBUS_TCP_DEFAULT_PORT) ||
       !AgentGetMetricArgAsInt32(metric, 3, &unitId, 1))
      return SYSINFO_RC_UNSUPPORTED;
   if ((unitId < 1) || (unitId > 255))
      return SYSINFO_RC_UNSUPPORTED;

   return MODBUSExecute(addr, port, unitId,
      [value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId) -> int32_t
      {
         modbus_device_id_response_t deviceId;
         int rc = modbus_read_device_id(mb, MODBUS_READ_DEVICE_ID_REGULAR, 0, &deviceId);
         if (rc < 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): request on REGULAR level failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            rc = modbus_read_device_id(mb, MODBUS_READ_DEVICE_ID_BASIC, 0, &deviceId);
            if (rc < 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): request on BASIC level failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
               return SYSINFO_RC_ERROR;
            }
         }

         nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_device_id(%hs:%d:%d): %d objects received"), addrText, port, unitId, deviceId.object_count);

         uint8_t *curr = deviceId.objects;
         for(int i = 0; i < deviceId.object_count; i++)
         {
            static const TCHAR *standardNames[] = { _T("VendorName"), _T("ProductCode"), _T("Revision"), _T("VendorURL"), _T("ProductName"), _T("ModelName"), _T("UserApplicationName") };

            int objectId = static_cast<int>(curr[0]);
            int objectLength = static_cast<int>(curr[1]);

            TCHAR objectValue[256];
            size_t chars = mb_to_wchar(reinterpret_cast<char*>(curr + 2), objectLength, objectValue, 256);
            objectValue[chars] = 0;

            TCHAR element[256];
            if (objectId < sizeof(standardNames) / sizeof(const TCHAR*))
               _sntprintf(element, 256, _T("%s: %s"), standardNames[objectId], objectValue);
            else
               _sntprintf(element, 256, _T("0x%02X: %s"), objectId, objectValue);
            value->add(element);

            curr += objectLength + 2;
         }

         return SYSINFO_RC_SUCCESS;
      });
}

#endif   /* WITH_MODBUS */
