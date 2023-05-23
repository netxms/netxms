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
 * Execute provided callback within MODBUS session context
 */
static LONG MODBUSExecute(const InetAddress& addr, uint16_t port, int32_t unitId, std::function<int32_t (modbus_t*, const char*, uint16_t, int32_t)> callback)
{
   char ipAddrText[64];

   modbus_t *mb;
   if (addr.getFamily() == AF_INET)
   {
      mb = modbus_new_tcp(addr.toStringA(ipAddrText), port);
   }
   else
   {
      char portText[64];
      mb = modbus_new_tcp_pi(addr.toStringA(ipAddrText), IntegerToString(port, portText));
   }

   modbus_set_response_timeout(mb, 1, 0);

   if (modbus_connect(mb) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_connect(%hs:%d) failed (%hs)"), ipAddrText, port, modbus_strerror(errno));
      modbus_free(mb);
      return SYSINFO_RC_ERROR;
   }

   modbus_set_slave(mb, unitId);

   LONG exitCode = callback(mb, ipAddrText, port, unitId);

   modbus_close(mb);
   modbus_free(mb);
   return exitCode;
}

/**
 * Common handler for MODBUS.* metrics
 */
static LONG ReadMODBUSMetric(const TCHAR *metric, std::function<int32_t (modbus_t*, const char*, uint16_t, int32_t, int32_t)> reader)
{
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
   return ReadMODBUSMetric(metric,
      [value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         uint16_t r;
         if (modbus_read_registers(mb, modbusAddress, 1, &r) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_registers(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }

         ret_uint(value, r);
         return SYSINFO_RC_SUCCESS;
      });
}

/**
 * Handler for metric MODBUS.InputRegister
 */
LONG H_MODBUSInputRegister(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return ReadMODBUSMetric(metric,
      [value] (modbus_t *mb, const char *addrText, uint16_t port, int32_t unitId, int32_t modbusAddress) -> int32_t
      {
         uint16_t r;
         if (modbus_read_input_registers(mb, modbusAddress, 1, &r) < 1)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("modbus_read_input_registers(%hs:%d:%d) failed (%hs)"), addrText, port, unitId, modbus_strerror(errno));
            return SYSINFO_RC_ERROR;
         }

         ret_uint(value, r);
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

#endif
