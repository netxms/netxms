/*
** NetXMS UPS management subagent
** Copyright (C) 2025 Raden Solutions
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
** File: mec0003.cpp
**
**/

#include "ups.h"
#include <setupapi.h>

#define CMD_STATUS   0x03
#define CMD_RATINGS  0x0D
#define CMD_VENDOR   0x0C

/**
 * Parse VID:PID from device string
 */
static bool ParseDeviceString(const TCHAR *device, WORD *vendorId, WORD *productId)
{
   if (_tcsicmp(device, _T("ANY")) == 0)
   {
      *vendorId = 0x0001;
      *productId = 0x0000;
      return true;
   }

   TCHAR *end;
   *vendorId = static_cast<WORD>(_tcstol(device, &end, 16));
   if (*end != _T(':'))
      return false;

   *productId = static_cast<WORD>(_tcstol(end + 1, &end, 16));
   return *end == 0;
}

/**
 * Constructor
 */
MEC0003Interface::MEC0003Interface(const TCHAR *device) : UPSInterface(device)
{
   m_hDev = INVALID_HANDLE_VALUE;
   m_packs = 0;

   if (!ParseDeviceString(device, &m_vendorId, &m_productId))
   {
      m_vendorId = 0x0001;
      m_productId = 0x0000;
   }

   m_paramList[UPS_PARAM_MFG_DATE].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_SERIAL].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_BATTERY_LEVEL].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_EST_RUNTIME].flags |= UPF_NOT_SUPPORTED;
}

/**
 * Destructor
 */
MEC0003Interface::~MEC0003Interface()
{
   close();
}

/**
 * Query string descriptor using the given index
 */
bool MEC0003Interface::queryStringDescriptor(UCHAR index, char *buffer, size_t bufferSize)
{
   WCHAR wideBuffer[256];

   if (!HidD_GetIndexedString(m_hDev, index, wideBuffer, sizeof(wideBuffer)))
      return false;

   size_t len = wcslen(wideBuffer);
   if (len >= bufferSize)
      len = bufferSize - 1;

   for (size_t i = 0; i < len; i++)
      buffer[i] = static_cast<char>(wideBuffer[i]);
   buffer[len] = 0;

   return true;
}

/**
 * Calculate number of battery packs
 */
void MEC0003Interface::calculatePacks(double nominalVoltage, double actualVoltage)
{
   static double packs[] = { 120, 100, 80, 60, 48, 36, 30, 24, 18, 12, 8, 6, 4, 3, 2, 1, 0.5, 0 };
   for(int i = 0; packs[i] > 0; i++)
   {
      if (actualVoltage * packs[i] > nominalVoltage * 1.2)
         continue;
      if (actualVoltage * packs[i] < nominalVoltage * 0.8)
         break;

      m_packs = packs[i];
      break;
   }
   nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEC0003 interface initialization: packs=%0.1f"), m_packs);
}

/**
 * Open device
 */
bool MEC0003Interface::open()
{
   GUID hidGuid;
   HDEVINFO deviceInfo;
   SP_DEVICE_INTERFACE_DATA interfaceData;
   PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = nullptr;
   DWORD size;
   bool found = false;
   TCHAR errorText[256];

   HidD_GetHidGuid(&hidGuid);

   deviceInfo = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (deviceInfo == INVALID_HANDLE_VALUE)
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: SetupDiGetClassDevs failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
      return false;
   }

   interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

   for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfo, nullptr, &hidGuid, i, &interfaceData); i++)
   {
      SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, nullptr, 0, &size, nullptr);

      detailData = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(size));
      if (!detailData)
         continue;

      detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

      if (SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, size, nullptr, nullptr))
      {
         HANDLE hDevice = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
         );

         if (hDevice != INVALID_HANDLE_VALUE)
         {
            HIDD_ATTRIBUTES attrib;
            attrib.Size = sizeof(HIDD_ATTRIBUTES);

            if (HidD_GetAttributes(hDevice, &attrib))
            {
               nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: checking device VID:0x%04X PID:0x%04X"), attrib.VendorID, attrib.ProductID);

               if (attrib.VendorID == m_vendorId && attrib.ProductID == m_productId)
               {
                  m_hDev = hDevice;
                  found = true;
                  free(detailData);
                  nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEC0003: found matching device at %s"), detailData->DevicePath);
                  break;
               }
            }
            CloseHandle(hDevice);
         }
      }
      free(detailData);
   }

   SetupDiDestroyDeviceInfoList(deviceInfo);

   if (!found)
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEC0003: device not found (VID:0x%04X PID:0x%04X)"), m_vendorId, m_productId);
      return false;
   }

   char buffer[256];
   if (!queryStringDescriptor(CMD_RATINGS, buffer, 256))
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEC0003: failed to read ratings"));
      close();
      return false;
   }

   nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: received nominal values response \"%hs\""), buffer);

   if (buffer[0] == '#')
   {
      setConnected();

      buffer[16] = 0;
      double nominalVoltage = strtod(&buffer[11], nullptr);
      sprintf(m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].value, "%0.2f", nominalVoltage);
      m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].flags &= ~UPF_NULL_VALUE;

      if (queryStringDescriptor(CMD_STATUS, buffer, 256))
      {
         if (buffer[0] == '(')
         {
            buffer[32] = 0;
            calculatePacks(nominalVoltage, strtod(&buffer[28], nullptr));
         }
      }

      return true;
   }

   nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEC0003: invalid nominal values response \"%hs\""), buffer);
   close();
   return false;
}

/**
 * Close device
 */
void MEC0003Interface::close()
{
   if (m_hDev != INVALID_HANDLE_VALUE)
   {
      CloseHandle(m_hDev);
      m_hDev = INVALID_HANDLE_VALUE;
   }
   UPSInterface::close();
}

/**
 * Validate connection
 */
bool MEC0003Interface::validateConnection()
{
   char buffer[256];
   bool success = queryStringDescriptor(CMD_RATINGS, buffer, 256);
   if (success)
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: received nominal values response \"%hs\""), buffer);
   return success && (buffer[0] == '#');
}

/**
 * Query static data
 */
void MEC0003Interface::queryStaticData()
{
   char buffer[256];
   if (queryStringDescriptor(CMD_VENDOR, buffer, 256))
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: received info response \"%hs\""), buffer);
      if (buffer[0] == '#')
      {
         buffer[27] = 0;
         TrimA(&buffer[17]);
         strcpy(m_paramList[UPS_PARAM_MODEL].value, &buffer[17]);

         TrimA(&buffer[28]);
         strcpy(m_paramList[UPS_PARAM_FIRMWARE].value, &buffer[28]);

         m_paramList[UPS_PARAM_MODEL].flags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         m_paramList[UPS_PARAM_FIRMWARE].flags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      }
      else
      {
         m_paramList[UPS_PARAM_MODEL].flags |= UPF_NOT_SUPPORTED;
         m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_MODEL].flags |= UPF_NOT_SUPPORTED;
      m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NOT_SUPPORTED;
   }
}

/**
 * Query dynamic data
 */
void MEC0003Interface::queryDynamicData()
{
   static int paramIndex[] = { UPS_PARAM_INPUT_VOLTAGE, -1, UPS_PARAM_OUTPUT_VOLTAGE, UPS_PARAM_LOAD, UPS_PARAM_LINE_FREQ, UPS_PARAM_BATTERY_VOLTAGE, UPS_PARAM_TEMP };

   char buffer[256];
   if (queryStringDescriptor(CMD_STATUS, buffer, 256))
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: received status response \"%hs\""), buffer);
      if (buffer[0] == '(')
      {
         const char *curr = &buffer[1];
         for(int i = 0; i < 7; i++)
         {
            char value[64];
            curr = ExtractWordA(curr, value);
            int index = paramIndex[i];
            if (index == -1)
               continue;

            char *p = value;
            while(*p == '0')
               p++;
            if (*p == 0)
               p--;
            strcpy(m_paramList[index].value, p);
            m_paramList[index].flags &= ~UPF_NULL_VALUE;
         }

         while(isspace(*curr))
            curr++;
         strcpy(m_paramList[UPS_PARAM_ONLINE_STATUS].value, curr[0] == '1' ? (curr[1] == '1' ? "2" : "1") : "0");
         m_paramList[UPS_PARAM_ONLINE_STATUS].flags &= ~UPF_NULL_VALUE;
         nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEC0003: status bits = %hs, online = %hs"), curr, m_paramList[UPS_PARAM_ONLINE_STATUS].value);

         if ((curr[4] == '0') && (m_packs > 0))
         {
            sprintf(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].value, "%0.2f", strtod(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].value, nullptr) * m_packs);
         }
      }
      else
      {
         for(int i = 0; i < 7; i++)
         {
            if (paramIndex[i] != -1)
               m_paramList[paramIndex[i]].flags |= UPF_NULL_VALUE;
         }
         m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      for(int i = 0; i < 7; i++)
      {
         if (paramIndex[i] != -1)
            m_paramList[paramIndex[i]].flags |= UPF_NULL_VALUE;
      }
      m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
   }
}
