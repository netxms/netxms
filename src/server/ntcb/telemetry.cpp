/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
** File: session.cpp
**
**/

#include "ntcb.h"

static struct TelemetryField s_telemetryFields[255] =
{
   { 4, TelemetryDataType::U32, nullptr, nullptr },
   { 2, TelemetryDataType::U16, nullptr, nullptr },
   { 4, TelemetryDataType::U32, nullptr, nullptr },
   { 1, TelemetryDataType::U8, nullptr, nullptr },
};

/**
 * Convert value of given field to string
 */
static TCHAR *TelemetryFieldToString(const TelemetryField *field, TelemetryValue value, TCHAR *buffer)
{
   switch(field->dataType)
   {
      case TelemetryDataType::I8:
         _sntprintf(buffer, 64, _T("%d"), static_cast<int>(value.i8));
         break;
      case TelemetryDataType::I16:
         _sntprintf(buffer, 64, _T("%d"), static_cast<int>(value.i16));
         break;
      case TelemetryDataType::I32:
         _sntprintf(buffer, 64, _T("%d"), value.i32);
         break;
      case TelemetryDataType::I64:
         _sntprintf(buffer, 64, INT64_FMT, value.i64);
         break;
      case TelemetryDataType::U8:
         _sntprintf(buffer, 64, _T("%u"), static_cast<unsigned int>(value.u8));
         break;
      case TelemetryDataType::U16:
         _sntprintf(buffer, 64, _T("%u"), static_cast<unsigned int>(value.u16));
         break;
      case TelemetryDataType::U32:
         _sntprintf(buffer, 64, _T("%u"), value.u32);
         break;
      case TelemetryDataType::U64:
         _sntprintf(buffer, 64, UINT64_FMT, value.u64);
         break;
      case TelemetryDataType::F32:
         _sntprintf(buffer, 64, _T("%f"), value.f32);
         break;
      case TelemetryDataType::F64:
         _sntprintf(buffer, 64, _T("%f"), value.f64);
         break;
      default:
         BinToStr(value.raw, field->size, buffer);
         break;
   }
   return buffer;
}

/**
 * Push DCI value
 */
static void PushData(shared_ptr<MobileDevice> device, TelemetryField *field, TelemetryValue value)
{
   shared_ptr<DCObject> dci = device->getDCObjectByName(field->name, 0);
   if ((dci == nullptr) || (dci->getDataSource() != DS_PUSH_AGENT) || (dci->getType() != DCO_TYPE_ITEM))
   {
      nxlog_debug_tag(DEBUG_TAG_NTCB, 5, _T("DCI %s on device %s [%u] %s"), field->name, device->getName(), device->getId(),
               (dci == nullptr) ? _T("does not exist") : _T("is of incompatible type"));
      return;
   }

   TCHAR buffer[MAX_RESULT_LENGTH];
   device->processNewDCValue(dci, time(nullptr), TelemetryFieldToString(field, value, buffer));
}

/**
 * Read single FLEX telemetry record
 */
bool NTCBDeviceSession::readTelemetryRecord()
{
   int fieldIndex = 0;
   int maskByte;
   uint8_t mask = 0x80;
   while(fieldIndex < m_flexFieldCount)
   {
      if (m_flexFieldMask[maskByte] & mask)
      {
         TelemetryField *field = &s_telemetryFields[fieldIndex];
         TelemetryValue value;
         if (!m_socket.readFully(&value, field->size, g_ntcbSocketTimeout))
         {
            debugPrintf(5, _T("readTelemetryRecord: failed to read field #%d"), fieldIndex + 1);
            return false;
         }

         TCHAR stringValue[64];
         debugPrintf(7, _T("Telemetry record #%d = %s"), fieldIndex + 1, TelemetryFieldToString(field, value, stringValue));

         if (field->name != nullptr)
         {
            ThreadPoolExecute(g_mobileThreadPool, PushData, m_device, field, value);
         }
         else if (field->handler != nullptr)
         {
            ThreadPoolExecute(g_mobileThreadPool, field->handler, field->dataType, value, m_device);
         }
      }

      fieldIndex++;
      mask >>= 1;
      if (mask == 0)
      {
         mask = 0x80;
         maskByte++;
      }
   }
   return true;
}
