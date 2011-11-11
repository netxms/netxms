/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2009 Victor Kirhenshtein
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
** File: ups.cpp
**
**/

#include "ups.h"


//
// Abstract class constructor
//

UPSInterface::UPSInterface(TCHAR *pszDevice)
{
   int i;

   m_pszName = NULL;
   m_pszDevice = _tcsdup(pszDevice);
   m_bIsConnected = FALSE;
   memset(m_paramList, 0, sizeof(UPS_PARAMETER) * UPS_PARAM_COUNT);
   for(i = 0; i < UPS_PARAM_COUNT; i++)
      m_paramList[i].dwFlags |= UPF_NULL_VALUE;
   m_mutex = MutexCreate();
   m_condStop = ConditionCreate(TRUE);
   m_thCommThread = INVALID_THREAD_HANDLE;
}


//
// Destructor
//

UPSInterface::~UPSInterface()
{
   ConditionSet(m_condStop);
   ThreadJoin(m_thCommThread);
   safe_free(m_pszDevice);
   safe_free(m_pszName);
   MutexDestroy(m_mutex);
   ConditionDestroy(m_condStop);
}


//
// Set name
//

void UPSInterface::SetName(TCHAR *pszName)
{
   safe_free(m_pszName);
   if (pszName[0] == 0)
   {
      TCHAR szBuffer[MAX_DB_STRING];

      _sntprintf(szBuffer, MAX_DB_STRING, _T("%s-%s"), Type(), m_pszDevice);
      m_pszName = _tcsdup(szBuffer);
   }
   else
   {
      m_pszName = _tcsdup(pszName);
   }
}


//
// Open communication to UPS
//

BOOL UPSInterface::Open(void)
{
   return FALSE;
}


//
// Close communications with UPS
//

void UPSInterface::Close(void)
{
   m_bIsConnected = FALSE;
}


//
// Validate connection to the UPS
//

BOOL UPSInterface::ValidateConnection(void)
{
   return FALSE;
}


//
// Get parameter's value
//

LONG UPSInterface::GetParameter(int nParam, TCHAR *pszValue)
{
   LONG nRet;

   if ((nParam < 0) || (nParam >= UPS_PARAM_COUNT))
      return SYSINFO_RC_UNSUPPORTED;

   MutexLock(m_mutex);

   if (m_paramList[nParam].dwFlags & UPF_NOT_SUPPORTED)
   {
      nRet = SYSINFO_RC_UNSUPPORTED;
   }
   else if (m_paramList[nParam].dwFlags & UPF_NULL_VALUE)
   {
      nRet = SYSINFO_RC_ERROR;
   }
   else
   {
      _tcscpy(pszValue, m_paramList[nParam].szValue);
      nRet = SYSINFO_RC_SUCCESS;
   }

   MutexUnlock(m_mutex);
   return nRet;
}


//
// Parameter query methods need to be overriden
//

void UPSInterface::QueryModel(void)
{
   m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryFirmwareVersion(void)
{
   m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryMfgDate(void)
{
   m_paramList[UPS_PARAM_MFG_DATE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QuerySerialNumber(void)
{
   m_paramList[UPS_PARAM_SERIAL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryTemperature(void)
{
   m_paramList[UPS_PARAM_TEMP].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryBatteryVoltage(void)
{
   m_paramList[UPS_PARAM_BATTERY_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryNominalBatteryVoltage(void)
{
   m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryBatteryLevel(void)
{
   m_paramList[UPS_PARAM_BATTERY_LEVEL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryInputVoltage(void)
{
   m_paramList[UPS_PARAM_INPUT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryOutputVoltage(void)
{
   m_paramList[UPS_PARAM_OUTPUT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryLineFrequency(void)
{
   m_paramList[UPS_PARAM_LINE_FREQ].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryPowerLoad(void)
{
   m_paramList[UPS_PARAM_LOAD].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryEstimatedRuntime(void)
{
   m_paramList[UPS_PARAM_EST_RUNTIME].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::QueryOnlineStatus(void)
{
   m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NOT_SUPPORTED;
}


//
// Communication thread starter
//

THREAD_RESULT THREAD_CALL UPSInterface::CommThreadStarter(void *pArg)
{
   ((UPSInterface *)pArg)->CommThread();
   return THREAD_OK;
}


//
// Start communication thread
//

void UPSInterface::StartCommunication(void)
{
   m_thCommThread = ThreadCreateEx(CommThreadStarter, 0, this);
}


//
// Communication thread
//

void UPSInterface::CommThread(void)
{
   int nIteration;

   // Try to open device immediatelly after start
   if (Open())
   {
      AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("UPS: Established communication with device #%d \"%s\""), m_nIndex, m_pszName);

      // Open successfully, query all parameters
      MutexLock(m_mutex);
      QueryModel();
      QueryFirmwareVersion();
      QueryMfgDate();
      QuerySerialNumber();
      QueryTemperature();
      QueryBatteryVoltage();
      QueryNominalBatteryVoltage();
      QueryBatteryLevel();
      QueryInputVoltage();
      QueryOutputVoltage();
      QueryLineFrequency();
      QueryPowerLoad();
      QueryEstimatedRuntime();
      QueryOnlineStatus();
      MutexUnlock(m_mutex);
   }
   else
   {
      AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("UPS: Cannot establish communication with device #%d \"%s\""), m_nIndex, m_pszName);
   }

   for(nIteration = 0; ; nIteration++)
   {
      if (ConditionWait(m_condStop, 10000))
         break;

      // Try to connect to device if it is not connected
      if (!m_bIsConnected)
      {
         if (Open())
         {
            AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("UPS: Established communication with device #%d \"%s\""), m_nIndex, m_pszName);
            nIteration = 100;    // Force all parameters to be polled
         }
      }
      else  // Validate connection
      {
         if (!ValidateConnection())
         {
            // Try to reconnect
            Close();
            if (Open())
            {
               nIteration = 100;    // Force all parameters to be polled
            }
            else
            {
               AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("UPS: Lost communication with device #%d \"%s\""), m_nIndex, m_pszName);
            }
         }
      }

      // Query parameters if we are connected
      if (m_bIsConnected)
      {
         MutexLock(m_mutex);

         // Check rarely changing parameters only every 100th poll
         if (nIteration == 100)
         {
            nIteration = 0;
            QueryModel();
            QueryFirmwareVersion();
            QueryMfgDate();
            QuerySerialNumber();
         }

         QueryTemperature();
         QueryBatteryVoltage();
         QueryNominalBatteryVoltage();
         QueryBatteryLevel();
         QueryInputVoltage();
         QueryOutputVoltage();
         QueryLineFrequency();
         QueryPowerLoad();
         QueryEstimatedRuntime();
         QueryOnlineStatus();

         MutexUnlock(m_mutex);
      }
   }
}
