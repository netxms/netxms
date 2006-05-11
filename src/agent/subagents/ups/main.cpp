/*
** NetXMS UPS management subagent
** Copyright (C) 2006 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "ups.h"


//
// Static data
//

static UPSInterface *m_deviceInfo[MAX_UPS_DEVICES];


//
// Universal handler
//

static LONG H_UPSData(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   LONG nRet, nDev, nValue;
   double dValue;
   TCHAR *pErr, szArg[256];
   BOOL bFirstTry = TRUE;

   if (!NxGetParameterArg(pszParam, 1, szArg, 256))
      return SYSINFO_RC_UNSUPPORTED;

   nDev = _tcstol(szArg, &pErr, 0);
   if ((*pErr != 0) || (nDev < 0) || (nDev >= MAX_UPS_DEVICES))
      return SYSINFO_RC_UNSUPPORTED;

   if (m_deviceInfo[nDev] == NULL)
      return SYSINFO_RC_UNSUPPORTED;

   if (!m_deviceInfo[nDev]->IsConnected())
      if (!m_deviceInfo[nDev]->Open())
         return SYSINFO_RC_ERROR;

restart_query:
   switch(*((char *)pArg))
   {
      case 'B':   // Battery voltage
         nRet = m_deviceInfo[nDev]->GetBatteryVoltage(&dValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_double(pValue, dValue);
         break;
      case 'b':   // Nominal battery voltage
         nRet = m_deviceInfo[nDev]->GetNominalBatteryVoltage(&dValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_double(pValue, dValue);
         break;
      case 'E':   // Estimated runtime
         nRet = m_deviceInfo[nDev]->GetEstimatedRuntime(&nValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_int(pValue, nValue);
         break;
      case 'F':   // Firmware version
         nRet = m_deviceInfo[nDev]->GetFirmwareVersion(pValue);
         break;
      case 'f':   // Line frequency
         nRet = m_deviceInfo[nDev]->GetLineFrequency(&nValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_int(pValue, nValue);
         break;
      case 'I':   // Input voltage
         nRet = m_deviceInfo[nDev]->GetInputVoltage(&dValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_double(pValue, dValue);
         break;
      case 'L':   // Battery level
         nRet = m_deviceInfo[nDev]->GetBatteryLevel(&nValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_int(pValue, nValue);
         break;
      case 'l':   // Load
         nRet = m_deviceInfo[nDev]->GetPowerLoad(&nValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_int(pValue, nValue);
         break;
      case 'M':   // Model
         nRet = m_deviceInfo[nDev]->GetModel(pValue);
         break;
      case 'm':   // Manufacturing date
         nRet = m_deviceInfo[nDev]->GetMfgDate(pValue);
         break;
      case 'O':   // Output voltage
         nRet = m_deviceInfo[nDev]->GetOutputVoltage(&dValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_double(pValue, dValue);
         break;
      case 'S':   // Serial number
         nRet = m_deviceInfo[nDev]->GetSerialNumber(pValue);
         break;
      case 'T':   // Temperature
         nRet = m_deviceInfo[nDev]->GetTemperature(&nValue);
         if (nRet == SYSINFO_RC_SUCCESS)
            ret_int(pValue, nValue);
         break;
      default:
         nRet = SYSINFO_RC_UNSUPPORTED;
         break;
   }

   if ((nRet == SYSINFO_RC_ERROR) && bFirstTry)
   {
      // Try to re-connect in case of error
      m_deviceInfo[nDev]->Close();
      if (m_deviceInfo[nDev]->Open())
      {
         bFirstTry = FALSE;
         goto restart_query;
      }
   }
   return nRet;
}


//
// List configured devices
//

static LONG H_DeviceList(TCHAR *pszParam, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
{
   TCHAR szBuffer[256];
   int i;

   for(i = 0; i < MAX_UPS_DEVICES; i++)
      if (m_deviceInfo[i] != NULL)
      {
         _sntprintf(szBuffer, 256, _T("%d %s %s %s"), i,
                    m_deviceInfo[i]->Device(), m_deviceInfo[i]->Type(),
                    m_deviceInfo[i]->Name());
         NxAddResultString(pValue, szBuffer);
      }
   return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void UnloadHandler(void)
{
   int i;

   // Close all devices
   for(i = 0; i < MAX_UPS_DEVICES; i++)
      if (m_deviceInfo[i] != NULL)
      {
         m_deviceInfo[i]->Close();
         delete_and_null(m_deviceInfo[i]);
      }
}


//
// Add device from configuration file parameter
// Parameter value should be <device_id>:<port>:<protocol>
//

static BOOL AddDeviceFromConfig(TCHAR *pszStr)
{
   TCHAR *ptr, *eptr, *pszCurrField;
   TCHAR szPort[MAX_PATH], szName[MAX_DB_STRING] = _T("");
   int nState, nField, nDev, nPos, nProto;

   // Parse line
   pszCurrField = (TCHAR *)malloc(sizeof(TCHAR) * (_tcslen(pszStr) + 1));
   for(ptr = pszStr, nState = 0, nField = 0, nPos = 0;
       (nState != -1) && (nState != 255); ptr++)
   {
      switch(nState)
      {
         case 0:  // Normal text
            switch(*ptr)
            {
               case '\'':
                  nState = 1;
                  break;
               case '"':
                  nState = 2;
                  break;
               case ':':   // New field
               case 0:
                  pszCurrField[nPos] = 0;
                  switch(nField)
                  {
                     case 0:  // Device number
                        nDev = _tcstol(pszCurrField, &eptr, 0);
                        if ((*eptr != 0) || (nDev < 0) || (nDev >= MAX_UPS_DEVICES))
                           nState = 255;  // Error
                        break;
                     case 1:  // Serial port
                        nx_strncpy(szPort, pszCurrField, MAX_PATH);
                        break;
                     case 2:  // Protocol
                        if (!_tcsicmp(pszCurrField, _T("APC")))
                        {
                           nProto = UPS_PROTOCOL_APC;
                        }
                        else if (!_tcsicmp(pszCurrField, _T("BCMXCP")))
                        {
                           nProto = UPS_PROTOCOL_BCMXCP;
                        }
#ifdef _WIN32
                        else if (!_tcsicmp(pszCurrField, _T("USB")))
                        {
                           nProto = UPS_PROTOCOL_USB;
                        }
#endif
                        else
                        {
                           nState = 255;  // Error
                        }
                        break;
                     case 3:  // Name
                        nx_strncpy(szName, pszCurrField, MAX_DB_STRING);
                        break;
                     default:
                        nState = 255;  // Error
                        break;
                  }
                  nField++;
                  nPos = 0;
                  if ((nState != 255) && (*ptr == 0))
                     nState = -1;   // Finish
                  break;
               default:
                  pszCurrField[nPos++] = *ptr;
                  break;
            }
            break;
         case 1:  // Single quotes
            switch(*ptr)
            {
               case '\'':
                  nState = 0;
                  break;
               case 0:  // Unexpected end of line
                  nState = 255;
                  break;
               default:
                  pszCurrField[nPos++] = *ptr;
                  break;
            }
            break;
         case 2:  // Double quotes
            switch(*ptr)
            {
               case '"':
                  nState = 0;
                  break;
               case 0:  // Unexpected end of line
                  nState = 255;
                  break;
               default:
                  pszCurrField[nPos++] = *ptr;
                  break;
            }
            break;
         default:
            break;
      }
   }
   free(pszCurrField);

   // Add new device if parsing was successful
   if ((nState == -1) && (nField >= 3))
   {
      if (m_deviceInfo[nDev] != NULL)
         delete m_deviceInfo[nDev];
      switch(nProto)
      {
         case UPS_PROTOCOL_APC:
            m_deviceInfo[nDev] = new APCInterface(szPort);
            break;
         case UPS_PROTOCOL_BCMXCP:
            m_deviceInfo[nDev] = new BCMXCPInterface(szPort);
            break;
#ifdef _WIN32
         case UPS_PROTOCOL_USB:
            m_deviceInfo[nDev] = new USBInterface(szPort);
            break;
#endif
         default:
            break;
      }
      m_deviceInfo[nDev]->SetName(szName);
   }

   return ((nState == -1) && (nField >= 3));
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("UPS.BatteryLevel(*)"), H_UPSData, "L", DCI_DT_INT, _T("UPS {instance} battery charge level") },
   { _T("UPS.BatteryVoltage(*)"), H_UPSData, "B", DCI_DT_FLOAT, _T("UPS {instance} battery voltage") },
   { _T("UPS.EstimatedRuntime(*)"), H_UPSData, "E", DCI_DT_INT, _T("UPS {instance} estimated on-battery runtime (minutes)") },
   { _T("UPS.Firmware(*)"), H_UPSData, "F", DCI_DT_STRING, _T("UPS {instance} firmware version") },
   { _T("UPS.InputVoltage(*)"), H_UPSData, "I", DCI_DT_FLOAT, _T("UPS {instance} input line voltage") },
   { _T("UPS.LineFrequency(*)"), H_UPSData, "f", DCI_DT_INT, _T("UPS {instance} input line frequency") },
   { _T("UPS.Load(*)"), H_UPSData, "l", DCI_DT_INT, _T("UPS {instance} load") },
   { _T("UPS.MfgDate(*)"), H_UPSData, "m", DCI_DT_STRING, _T("UPS {instance} manufacturing date") },
   { _T("UPS.Model(*)"), H_UPSData, "M", DCI_DT_STRING, _T("UPS {instance} model") },
   { _T("UPS.NominalBatteryVoltage(*)"), H_UPSData, "b", DCI_DT_FLOAT, _T("UPS {instance} nominal battery voltage") },
   { _T("UPS.OutputVoltage(*)"), H_UPSData, "O", DCI_DT_FLOAT, _T("UPS {instance} output voltage") },
   { _T("UPS.SerialNumber(*)"), H_UPSData, "S", DCI_DT_STRING, _T("UPS {instance} serial number") },
   { _T("UPS.Temperature(*)"), H_UPSData, "T", DCI_DT_INT, _T("UPS {instance} temperature") }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { _T("UPS.DeviceList"), H_DeviceList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("UPS"), _T(NETXMS_VERSION_STRING),
   UnloadHandler, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
   0, NULL
};


//
// Configuration file template
//

static TCHAR *m_pszDeviceList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { _T("Device"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszDeviceList },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(APC)
{
   DWORD i, dwResult;

   memset(m_deviceInfo, 0, sizeof(UPSInterface *) * MAX_UPS_DEVICES);

   // Load configuration
   dwResult = NxLoadConfig(pszConfigFile, _T("UPS"), cfgTemplate, FALSE);
   if (dwResult == NXCFG_ERR_OK)
   {
      TCHAR *pItem, *pEnd;

      // Parse device list
      if (m_pszDeviceList != NULL)
      {
         for(pItem = m_pszDeviceList; *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            StrStrip(pItem);
            if (!AddDeviceFromConfig(pItem))
               NxWriteAgentLog(EVENTLOG_WARNING_TYPE,
                               _T("Unable to add UPS device from configuration file. ")
                               _T("Original configuration record: %s"), pItem);
         }
         free(m_pszDeviceList);

         // Open configured devices
         for(i = 0; i < MAX_UPS_DEVICES; i++)
         {
            if (m_deviceInfo[i] != NULL)
               if (!m_deviceInfo[i]->Open())
               {
                  NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("UPS: Unable to open device %d"), i);
               }
         }
      }
   }
   else
   {
      safe_free(m_pszDeviceList);
   }

   *ppInfo = &m_info;
   return dwResult == NXCFG_ERR_OK;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
