/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
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

#include "ups.h"
#include <setupapi.h>

/**
 * Get serial number of USB device
 * Buffer provided to this function should be 256 characters long
 */
static BOOL GetDeviceSerialNumber(HANDLE hDev, TCHAR *pszSerial)
{
#ifdef UNICODE
   return HidD_GetSerialNumberString(hDev, pszSerial, 256 * sizeof(WCHAR));
#else
   BOOL bRet = FALSE;
   WCHAR wszSerial[256];

   if (HidD_GetSerialNumberString(hDev, wszSerial, 256 * sizeof(WCHAR)))
   {
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                          wszSerial, -1, pszSerial, 256, NULL, NULL);
      pszSerial[255] = 0;
      bRet = TRUE;
   }
   return bRet;
#endif
}

/**
 * Find appropriate report for given usage
 */
static BYTE FindReport(HIDP_VALUE_CAPS *pCaps, DWORD dwCapSize, USAGE uPage, USAGE uUsage)
{
   DWORD i;

   for(i = 0; i < dwCapSize; i++)
   {
      if ((pCaps[i].UsagePage == uPage) &&
          ((pCaps[i].IsRange && (uUsage >= pCaps[i].Range.UsageMin) && (uUsage <= pCaps[i].Range.UsageMax)) ||
           ((!pCaps[i].IsRange) && (pCaps[i].NotRange.Usage == uUsage))))
      {
         return pCaps[i].ReportID;
      }
   }
   return 0;
}

/**
 * Constructor
 */
USBInterface::USBInterface(TCHAR *pszDevice) : UPSInterface(pszDevice)
{
   m_hDev = INVALID_HANDLE_VALUE;
   m_pPreparsedData = NULL;
   m_pFeatureValueCaps = NULL;
}

/**
 * Destructor
 */
USBInterface::~USBInterface()
{
   close();
}

/**
 * Open device
 */
BOOL USBInterface::open()
{
   GUID hidGuid;
   HDEVINFO hInfo;
   SP_DEVICE_INTERFACE_DATA data;
   SP_DEVICE_INTERFACE_DETAIL_DATA *pDetails;
   int nIndex;
   DWORD dwLen, dwTemp;
   WORD wSize;
   BOOL bSuccess = FALSE;
   TCHAR errorText[256];

   HidD_GetHidGuid(&hidGuid);
   hInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_INTERFACEDEVICE | DIGCF_PRESENT);
   if (hInfo != INVALID_HANDLE_VALUE)
   {
      for(nIndex = 0; ; nIndex++)
      {
         data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
         if (!SetupDiEnumDeviceInterfaces(hInfo, NULL, &hidGuid, nIndex, &data))
         {
            AgentWriteDebugLog(7, _T("UPS: SetupDiEnumDeviceInterfaces failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
            break;
         }

         SetupDiGetDeviceInterfaceDetail(hInfo, &data, NULL, 0, &dwLen, NULL);
         pDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(dwLen);
         pDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
         if (!SetupDiGetDeviceInterfaceDetail(hInfo, &data, pDetails, dwLen, &dwTemp, NULL))
         {
            AgentWriteDebugLog(7, _T("UPS: SetupDiGetDeviceInterfaceDetail failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
            free(pDetails);
            continue;
         }

         // Open USB device
         m_hDev = CreateFile(pDetails->DevicePath, GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
         if (m_hDev == INVALID_HANDLE_VALUE)
         {
            AgentWriteDebugLog(7, _T("UPS: CreateFile failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
            free(pDetails);
            continue;
         }
         free(pDetails);

         // Get preparsed data of a top-level HID collection
         if (!HidD_GetPreparsedData(m_hDev, &m_pPreparsedData))
         {
            AgentWriteDebugLog(7, _T("UPS: HidD_GetPreparsedData failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
            CloseHandle(m_hDev);
            m_hDev = INVALID_HANDLE_VALUE;
            continue;
         }

         // Get device capabilities
         if (HidP_GetCaps(m_pPreparsedData, &m_deviceCaps) == HIDP_STATUS_SUCCESS)
         {
            TCHAR szSerial[256];

            // Get device serial number
            if (GetDeviceSerialNumber(m_hDev, szSerial))
            {
               AgentWriteDebugLog(7, _T("UPS: read serial number \"%s\""), szSerial);
            }
            else
            {
               AgentWriteDebugLog(7, _T("UPS: GetDeviceSerialNumber failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
               szSerial[0] = 0;
            }

            // Check if device is a UPS and that serial number matched
            if ((m_deviceCaps.UsagePage == 0x84) &&
                (m_deviceCaps.Usage == 0x04) &&
                ((!_tcsicmp(m_pszDevice, szSerial)) ||
                 (!_tcsicmp(m_pszDevice, _T("ANY")))))
            {
               // Get value capabilities
               wSize = m_deviceCaps.NumberFeatureValueCaps;
               m_pFeatureValueCaps = (HIDP_VALUE_CAPS *)malloc(sizeof(HIDP_VALUE_CAPS) * wSize);
               if (HidP_GetValueCaps(HidP_Feature, m_pFeatureValueCaps, &wSize, m_pPreparsedData) == HIDP_STATUS_SUCCESS)
               {
                  AgentWriteDebugLog(7, _T("UPS: found matching device"));
                  bSuccess = TRUE;
                  break;
               }
               else
               {
                  AgentWriteDebugLog(7, _T("UPS: HidP_GetValueCaps failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
               }
               safe_free_and_null(m_pFeatureValueCaps);
            }
            else
            {
               AgentWriteDebugLog(7, _T("UPS: device not matched (page=0x%02X usage=0x%02X serial=%s)"), m_deviceCaps.UsagePage, m_deviceCaps.Usage, szSerial);
            }
         }
         else
         {
            AgentWriteDebugLog(7, _T("UPS: HidP_GetCaps failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
         }

         CloseHandle(m_hDev);
         m_hDev = INVALID_HANDLE_VALUE;
      }
      SetupDiDestroyDeviceInfoList(hInfo);
   }
   else
   {
      AgentWriteDebugLog(7, _T("UPS: SetupDiGetClassDevs failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 256));
   }
   if (bSuccess)
      setConnected();
   return bSuccess;
}

/**
 * Close device
 */
void USBInterface::close()
{
   if (m_hDev != INVALID_HANDLE_VALUE)
   {
      CloseHandle(m_hDev);
      m_hDev = INVALID_HANDLE_VALUE;
   }
   if (m_pPreparsedData != NULL)
   {
      HidD_FreePreparsedData(m_pPreparsedData);
      m_pPreparsedData = NULL;
   }
   safe_free_and_null(m_pFeatureValueCaps);
   UPSInterface::close();
}

/**
 * Validate connection
 */
BOOL USBInterface::validateConnection()
{
   WCHAR wszSerial[256];
   return HidD_GetSerialNumberString(m_hDev, wszSerial, 256 * sizeof(WCHAR));
}

/**
 * Read integer value from device
 */
LONG USBInterface::readInt(USAGE uPage, USAGE uUsage, LONG *pnValue)
{
   BYTE bReportID, *pReport;
   DWORD dwTemp;
   LONG nRet;

   bReportID = FindReport(m_pFeatureValueCaps,
                          m_deviceCaps.NumberFeatureValueCaps, uPage, uUsage);
   if (bReportID != 0)
   {
      pReport = (BYTE *)malloc(m_deviceCaps.FeatureReportByteLength);
      *pReport = bReportID;
      if (HidD_GetFeature(m_hDev, pReport, m_deviceCaps.FeatureReportByteLength))
      {
         if (HidP_GetUsageValue(HidP_Feature, uPage, 0, uUsage,
                                &dwTemp, m_pPreparsedData, (char *)pReport,
                                m_deviceCaps.FeatureReportByteLength) == HIDP_STATUS_SUCCESS)
         {
            *pnValue = (LONG)dwTemp;
            nRet = SYSINFO_RC_SUCCESS;
         }
         else
         {
            nRet = SYSINFO_RC_ERROR;
         }
      }
      else
      {
         nRet = SYSINFO_RC_ERROR;
      }
   }
   else
   {
      nRet = SYSINFO_RC_UNSUPPORTED;
   }
   return nRet;
}

/**
 * Read indexed string value from device
 */
LONG USBInterface::readIndexedString(USAGE uPage, USAGE uUsage, char *pszBuffer, int nBufLen)
{
   LONG nRet, nIndex;

   nRet = readInt(uPage, uUsage, &nIndex);
   if (nRet == SYSINFO_RC_SUCCESS)
   {
      WCHAR wszTemp[256];

      if (HidD_GetIndexedString(m_hDev, nIndex, wszTemp, 512))
      {
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             wszTemp, -1, pszBuffer, nBufLen, NULL, NULL);
         pszBuffer[nBufLen - 1] = 0;
      }
      else
      {
         nRet = SYSINFO_RC_ERROR;
      }
   }
   return nRet;
}

/**
 * Read string parameter
 */
void USBInterface::readStringParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam)
{
   LONG nRet;

   nRet = readIndexedString(nPage, nUsage, pParam->szValue, MAX_RESULT_LENGTH);
   switch(nRet)
   {
      case SYSINFO_RC_SUCCESS:
         pParam->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         break;
      case SYSINFO_RC_ERROR:
         pParam->dwFlags |= UPF_NULL_VALUE;
         break;
      case SYSINFO_RC_UNSUPPORTED:
         pParam->dwFlags |= UPF_NOT_SUPPORTED;
         break;
   }
}

/**
 * Read integer parameter
 */
void USBInterface::readIntParam(USAGE nPage, USAGE nUsage, UPS_PARAMETER *pParam, int nDiv, BOOL bDouble)
{
   LONG nRet, nValue;
   double dValue;

   nRet = readInt(nPage, nUsage, &nValue);
   switch(nRet)
   {
      case SYSINFO_RC_SUCCESS:
         pParam->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         if (bDouble)
         {
            dValue = (nDiv != 0) ? (double)nValue / nDiv : nValue;
            sprintf(pParam->szValue, "%f", dValue);
         }
         else
         {
            if (nDiv != 0)
               nValue /= nDiv;
            sprintf(pParam->szValue, "%d", nValue);
         }
         break;
      case SYSINFO_RC_ERROR:
         pParam->dwFlags |= UPF_NULL_VALUE;
         break;
      case SYSINFO_RC_UNSUPPORTED:
         pParam->dwFlags |= UPF_NOT_SUPPORTED;
         break;
   }
}

/**
 * Get model name
 */
void USBInterface::queryModel()
{
   readStringParam(0x84, 0xFE, &m_paramList[UPS_PARAM_MODEL]);
}

/**
 * Get serial number
 */
void USBInterface::querySerialNumber()
{
   readStringParam(0x84, 0xFF, &m_paramList[UPS_PARAM_SERIAL]);
}

/**
 * Get battery voltage
 */
void USBInterface::queryBatteryVoltage()
{
   readIntParam(0x84, 0x30, &m_paramList[UPS_PARAM_BATTERY_VOLTAGE], 100, TRUE);
}

/**
 * Get nominal battery voltage
 */
void USBInterface::queryNominalBatteryVoltage()
{
   readIntParam(0x84, 0x40, &m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE], 100, TRUE);
}

/**
 * Battery level
 */
void USBInterface::queryBatteryLevel()
{
   readIntParam(0x85, 0x66, &m_paramList[UPS_PARAM_BATTERY_LEVEL], 0, FALSE);
}

/**
 * UPS load (in percents)
 */
void USBInterface::queryPowerLoad()
{
   readIntParam(0x84, 0x35, &m_paramList[UPS_PARAM_LOAD], 10, FALSE);
}

/**
 * Estimated on-battery run time
 */
void USBInterface::queryEstimatedRuntime()
{
   readIntParam(0x85, 0x68, &m_paramList[UPS_PARAM_EST_RUNTIME], 60, FALSE);
}

/**
 * Get UPS online status
 */
void USBInterface::queryOnlineStatus()
{
   LONG nRet, nACPresent, nBelowLimit;

   nRet = readInt(0x85, 0xD0, &nACPresent);
   switch(nRet)
   {
      case SYSINFO_RC_SUCCESS:
         if (nACPresent == 0)
         {
            // If on battery power, check battery status
            nRet = readInt(0x85, 0x42, &nBelowLimit);
            if (nRet == SYSINFO_RC_SUCCESS)
            {
               m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = nBelowLimit ? '2' : '1';
            }
            else
            {
               m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '1';
            }
         }
         else
         {
            m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '0';
         }
         m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[1] = 0;
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         break;
      case SYSINFO_RC_ERROR:
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
         break;
      case SYSINFO_RC_UNSUPPORTED:
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NOT_SUPPORTED;
         break;
   }
}
