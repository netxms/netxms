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

/**
 * Parse VID/PID from device instance ID
 * Device instance ID format: USB\VID_xxxx&PID_xxxx\...
 */
static bool ParseVidPid(const TCHAR *deviceId, UINT16 *vid, UINT16 *pid)
{
   *vid = 0;
   *pid = 0;

   const TCHAR *vidPos = _tcsstr(deviceId, _T("VID_"));
   const TCHAR *pidPos = _tcsstr(deviceId, _T("PID_"));

   if (vidPos != nullptr)
   {
      vidPos += 4;
      TCHAR *endPtr;
      *vid = (UINT16)_tcstoul(vidPos, &endPtr, 16);
   }

   if (pidPos != nullptr)
   {
      pidPos += 4;
      TCHAR *endPtr;
      *pid = (UINT16)_tcstoul(pidPos, &endPtr, 16);
   }

   return (*vid != 0);
}

/**
 * Parse parameter value (support both hex and decimal format)
 */
static UINT16 ParseParameterValue(const TCHAR *param)
{
   if (param == nullptr || param[0] == 0)
      return 0;

   TCHAR *endPtr;
   if (_tcsncmp(param, _T("0x"), 2) == 0 || _tcsncmp(param, _T("0X"), 2) == 0)
   {
      return (UINT16)_tcstoul(param + 2, &endPtr, 16);
   }
   else
   {
      return (UINT16)_tcstoul(param, &endPtr, 10);
   }
}

/**
 * Handler for USB.ConnectedCount metric
 */
LONG H_UsbConnectedCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR vidParam[256], pidParam[256];
   AgentGetParameterArg(cmd, 1, vidParam, 256);
   AgentGetParameterArg(cmd, 2, pidParam, 256);

   UINT16 targetVid = ParseParameterValue(vidParam);
   UINT16 targetPid = ParseParameterValue(pidParam);

   if (targetVid == 0)
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   bool matchPid = (targetPid != 0);

   GUID usbGuid = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
   
   HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&usbGuid, _T("USB"), nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (deviceInfoSet == INVALID_HANDLE_VALUE)
   {
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

      UINT16 vid, pid;
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
