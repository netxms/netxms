/*
** Windows platform subagent - USB metrics
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
** File: usb.cpp
**
**/

#include "winnt_subagent.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <Usbiodef.h>

/**
 * Parse VID/PID from device instance ID
 * Device instance ID format: USB\VID_xxxx&PID_xxxx\...
 */
static bool ParseVidPid(const TCHAR *deviceId, uint16_t *vid, uint16_t *pid)
{
   *vid = 0;
   *pid = 0;

   const TCHAR *vidPos = _tcsstr(deviceId, _T("VID_"));
   const TCHAR *pidPos = _tcsstr(deviceId, _T("PID_"));

   if (vidPos != nullptr)
   {
      vidPos += 4;
      *vid = static_cast<uint16_t>(_tcstoul(vidPos, nullptr, 16));
   }

   if (pidPos != nullptr)
   {
      pidPos += 4;
      *pid = static_cast<uint16_t>(_tcstoul(pidPos, nullptr, 16));
   }

   return (*vid != 0);
}

/**
 * Handler for USB.ConnectedCount metric
 */
LONG H_UsbConnectedCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR vidParam[256], pidParam[256];
   AgentGetParameterArg(cmd, 1, vidParam, 256);
   AgentGetParameterArg(cmd, 2, pidParam, 256);

   uint16_t targetVid = static_cast<uint16_t>(_tcstoul(vidParam, nullptr, 16));
   uint16_t targetPid = static_cast<uint16_t>(_tcstoul(pidParam, nullptr, 16));
   if (targetVid == 0)
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   bool matchPid = (targetPid != 0);

   HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (deviceInfoSet == INVALID_HANDLE_VALUE)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to SetupDiGetClassDevs failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return SYSINFO_RC_ERROR;
   }

   int count = 0;
   SP_DEVINFO_DATA deviceInfoData;
   deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

   for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
   {
      TCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];
      CONFIGRET cr = CM_Get_Device_ID(deviceInfoData.DevInst, deviceInstanceId, MAX_DEVICE_ID_LEN, 0);
      if (cr != CR_SUCCESS)
         continue;

      uint16_t vid, pid;
      if (!ParseVidPid(deviceInstanceId, &vid, &pid))
         continue;

      if (vid == targetVid)
      {
         if (!matchPid || pid == targetPid)
         {
            count++;
         }
      }
   }

   SetupDiDestroyDeviceInfoList(deviceInfoSet);

   ret_int(value, count);
   return SYSINFO_RC_SUCCESS;
}
