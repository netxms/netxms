/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2022-2023 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published
 ** by the Free Software Foundation; either version 3 of the License, or
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

#include "libnxagent.h"

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
LONG LIBNXAGENT_EXPORTABLE MODBUSExecute(const InetAddress& addr, uint16_t port, int32_t unitId, std::function<int32_t (modbus_t*, const char*, uint16_t, int32_t)> callback)
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
 * Internal data conversion types
 */
enum class ConversionType
{
   STRING,
   INT16,
   UINT16,
   INT32,
   UINT32,
   INT64,
   UINT64,
   FLOAT_ABCD,
   FLOAT_CDAB,
   FLOAT_BADC,
   FLOAT_DCBA
};

/**
 * Read registers from device and convert data according to given specification. Possible specifications:
 *    int16       : 16 bit signed integer
 *    uint16      : 16 bit unsigned integer
 *    int32       : 32 bit signed integer (will read 2 registers)
 *    uint32      : 32 bit unsigned integer (will read 2 registers)
 *    int64       : 64 bit signed integer (will read 4 registers)
 *    uint64      : 64 bit unsigned integer (will read 4 registers)
 *    float-abcd  : 4 byte floating point number, ABCD byte order
 *    float-cdab  : 4 byte floating point number, CDAB byte order
 *    float-badc  : 4 byte floating point number, BADC byte order
 *    float-dcba  : 4 byte floating point number, DCBA byte order
 *    string-N    : string of N characters (will read (N + 1) / 2 registers)
 *    string-N-CP : string of N characters encoded using codepage CP (will read (N + 1) / 2 registers)
 *
 * For string conversion, supplied value buffer should be at least N + 2 character long. For other conversions,
 * buffer should be at least 32 characters long.
 *
 * Returns number of registers read or -1 on error.
 */
static int MODBUSReadRegisters(modbus_t *mb, bool input, int address, const TCHAR *conversion, TCHAR *value)
{
   ConversionType type;
   int nr;
   char codepage[32];
   if (!_tcsnicmp(conversion, _T("string-"), 7))
   {
      const TCHAR *p = _tcschr(&conversion[7], _T('-'));
      if (p != nullptr)
      {
         TCHAR temp[64];
         size_t l = p - conversion - 7;
         _tcsncpy(temp, &conversion[7], l);
         temp[l] = 0;
         nr = (_tcstol(temp, nullptr, 10) + 1) / 2;
#ifdef UNICODE
         wchar_to_mb(p + 1, -1, codepage, 32);
#else
         strlcpy(codepage, p + 1, 32);
#endif
      }
      else
      {
         codepage[0] = 0;
         nr = (_tcstol(&conversion[7], nullptr, 10) + 1) / 2;
      }
      if (nr <= 0)
         return -1;
      type = ConversionType::STRING;
   }
   else if (!_tcsicmp(conversion, _T("int16")) || !_tcsicmp(conversion, _T("uint16")))
   {
      nr = 1;
      type = ((*conversion == 'u') || (*conversion == 'U')) ? ConversionType::UINT16 : ConversionType::INT16;
   }
   else if (!_tcsicmp(conversion, _T("int32")) || !_tcsicmp(conversion, _T("uint32")))
   {
      nr = 2;
      type = ((*conversion == 'u') || (*conversion == 'U')) ? ConversionType::UINT32 : ConversionType::INT32;
   }
   else if (!_tcsicmp(conversion, _T("int64")) || !_tcsicmp(conversion, _T("uint64")))
   {
      nr = 4;
      type = ((*conversion == 'u') || (*conversion == 'U')) ? ConversionType::UINT64 : ConversionType::INT64;
   }
   else if (!_tcsicmp(conversion, _T("float-abcd")))
   {
      nr = 2;
      type = ConversionType::FLOAT_ABCD;
   }
   else if (!_tcsicmp(conversion, _T("float-cdab")))
   {
      nr = 2;
      type = ConversionType::FLOAT_CDAB;
   }
   else if (!_tcsicmp(conversion, _T("float-badc")))
   {
      nr = 2;
      type = ConversionType::FLOAT_BADC;
   }
   else if (!_tcsicmp(conversion, _T("float-dcba")))
   {
      nr = 2;
      type = ConversionType::FLOAT_DCBA;
   }
   else
   {
      return -1;  // invalid conversion
   }

   int rc;
   if (type == ConversionType::STRING)
   {
      char *text = static_cast<char*>(MemAllocLocal(nr * sizeof(uint16_t) + 1));
      memset(text, 0, nr * sizeof(uint16_t) + 1);
      rc = input ? modbus_read_input_registers_as_string(mb, address, nr, text) : modbus_read_registers_as_string(mb, address, nr, text);
      if (rc >= 0)
      {
#ifdef UNICODE
         mbcp_to_wchar(text, -1, value, nr * 2 + 1, (codepage[0] != 0) ? codepage : nullptr);
#else
         strlcpy(value, text, nr * 2 + 1);
#endif
      }
      MemFreeLocal(text);
   }
   else
   {
      uint16_t *rawData = static_cast<uint16_t*>(MemAllocLocal(nr * sizeof(uint16_t)));
      rc = input ? modbus_read_input_registers(mb, address, nr, rawData) : modbus_read_registers(mb, address, nr, rawData);
      if (rc == nr)
      {
         switch(type)
         {
            case ConversionType::FLOAT_ABCD:
               _sntprintf(value, 32, _T("%f"), modbus_get_float_abcd(rawData));
               break;
            case ConversionType::FLOAT_BADC:
               _sntprintf(value, 32, _T("%f"), modbus_get_float_badc(rawData));
               break;
            case ConversionType::FLOAT_CDAB:
               _sntprintf(value, 32, _T("%f"), modbus_get_float_cdab(rawData));
               break;
            case ConversionType::FLOAT_DCBA:
               _sntprintf(value, 32, _T("%f"), modbus_get_float_dcba(rawData));
               break;
            case ConversionType::INT16:
               IntegerToString(static_cast<int16_t>(*rawData), value);
               break;
            case ConversionType::INT32:
               IntegerToString(MODBUS_GET_INT32_FROM_INT16(rawData, 0), value);
               break;
            case ConversionType::INT64:
               IntegerToString(MODBUS_GET_INT64_FROM_INT16(rawData, 0), value);
               break;
            case ConversionType::UINT16:
               IntegerToString(*rawData, value);
               break;
            case ConversionType::UINT32:
               IntegerToString(MODBUS_GET_UINT32_FROM_INT16(rawData, 0), value);
               break;
            case ConversionType::UINT64:
               IntegerToString(MODBUS_GET_UINT64_FROM_INT16(rawData, 0), value);
               break;
            default:
               value[0] = 0;
               break;
         }
      }
      MemFreeLocal(rawData);
   }

   return rc;
}

/**
 * Convenience wrapper for common MODBUSReadRegisters: read holding registers
 */
int LIBNXAGENT_EXPORTABLE MODBUSReadHoldingRegisters(modbus_t *mb, int address, const TCHAR *conversion, TCHAR *value)
{
   return MODBUSReadRegisters(mb, false, address, ((conversion != nullptr) && (*conversion != 0)) ? conversion : _T("uint16"), value);
}

/**
 * Convenience wrapper for common MODBUSReadRegisters: read input registers
 */
int LIBNXAGENT_EXPORTABLE MODBUSReadInputRegisters(modbus_t *mb, int address, const TCHAR *conversion, TCHAR *value)
{
   return MODBUSReadRegisters(mb, true, address, ((conversion != nullptr) && (*conversion != 0)) ? conversion : _T("uint16"), value);
}

#endif   /* WITH_MODBUS */
