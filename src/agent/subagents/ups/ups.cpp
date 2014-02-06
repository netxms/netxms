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
** File: ups.cpp
**
**/

#include "ups.h"

/**
 * Abstract class constructor
 */
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

/**
 * Destructor
 */
UPSInterface::~UPSInterface()
{
   ConditionSet(m_condStop);
   ThreadJoin(m_thCommThread);
   safe_free(m_pszDevice);
   safe_free(m_pszName);
   MutexDestroy(m_mutex);
   ConditionDestroy(m_condStop);
}

/**
 * Set name
 */
void UPSInterface::setName(const char *pszName)
{
   safe_free(m_pszName);
   if (pszName[0] == 0)
   {
      TCHAR szBuffer[MAX_DB_STRING];

      _sntprintf(szBuffer, MAX_DB_STRING, _T("%s-%s"), getType(), m_pszDevice);
      m_pszName = _tcsdup(szBuffer);
   }
   else
   {
#ifdef UNICODE
      m_pszName = WideStringFromMBString(pszName);
#else
      m_pszName = strdup(pszName);
#endif
   }
}


//
// Set name
//

#ifdef UNICODE

void UPSInterface::setName(const WCHAR *pszName)
{
   if (pszName[0] == 0)
   {
		setName("");
   }
   else
   {
	   safe_free(m_pszName);
      m_pszName = _tcsdup(pszName);
   }
}

#endif

/**
 * Open communication to UPS
 */
BOOL UPSInterface::open()
{
   return FALSE;
}

/**
 * Close communications with UPS
 */
void UPSInterface::close()
{
   m_bIsConnected = FALSE;
}

/**
 * Validate connection to the UPS
 */
BOOL UPSInterface::validateConnection()
{
   return FALSE;
}

/**
 * Get parameter's value
 */
LONG UPSInterface::getParameter(int nParam, TCHAR *pszValue)
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
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_paramList[nParam].szValue, -1, pszValue, MAX_RESULT_LENGTH);
#else
      strcpy(pszValue, m_paramList[nParam].szValue);
#endif
      nRet = SYSINFO_RC_SUCCESS;
   }

   MutexUnlock(m_mutex);
   return nRet;
}

/**
 * Parameter query methods need to be overriden
 */
void UPSInterface::queryModel()
{
   m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryFirmwareVersion()
{
   m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryMfgDate()
{
   m_paramList[UPS_PARAM_MFG_DATE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::querySerialNumber()
{
   m_paramList[UPS_PARAM_SERIAL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryTemperature()
{
   m_paramList[UPS_PARAM_TEMP].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryBatteryVoltage()
{
   m_paramList[UPS_PARAM_BATTERY_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryNominalBatteryVoltage()
{
   m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryBatteryLevel()
{
   m_paramList[UPS_PARAM_BATTERY_LEVEL].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryInputVoltage()
{
   m_paramList[UPS_PARAM_INPUT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryOutputVoltage()
{
   m_paramList[UPS_PARAM_OUTPUT_VOLTAGE].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryLineFrequency()
{
   m_paramList[UPS_PARAM_LINE_FREQ].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryPowerLoad()
{
   m_paramList[UPS_PARAM_LOAD].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryEstimatedRuntime()
{
   m_paramList[UPS_PARAM_EST_RUNTIME].dwFlags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryOnlineStatus()
{
   m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NOT_SUPPORTED;
}

/**
 * Communication thread starter
 */
THREAD_RESULT THREAD_CALL UPSInterface::commThreadStarter(void *pArg)
{
   ((UPSInterface *)pArg)->commThread();
   return THREAD_OK;
}

/**
 * Start communication thread
 */
void UPSInterface::startCommunication()
{
   m_thCommThread = ThreadCreateEx(commThreadStarter, 0, this);
}

/**
 * Communication thread
 */
void UPSInterface::commThread()
{
   int nIteration;

   // Try to open device immediatelly after start
   if (open())
   {
      AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("UPS: Established communication with device #%d \"%s\""), m_nIndex, m_pszName);

      // Open successfully, query all parameters
      MutexLock(m_mutex);
      queryModel();
      queryFirmwareVersion();
      queryMfgDate();
      querySerialNumber();
      queryTemperature();
      queryBatteryVoltage();
      queryNominalBatteryVoltage();
      queryBatteryLevel();
      queryInputVoltage();
      queryOutputVoltage();
      queryLineFrequency();
      queryPowerLoad();
      queryEstimatedRuntime();
      queryOnlineStatus();
      MutexUnlock(m_mutex);

      AgentWriteDebugLog(5, _T("UPS: initial poll finished for device #%d \"%s\""), m_nIndex, m_pszName);
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
         if (open())
         {
            AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("UPS: Established communication with device #%d \"%s\""), m_nIndex, m_pszName);
            nIteration = 100;    // Force all parameters to be polled
         }
      }
      else  // Validate connection
      {
         if (!validateConnection())
         {
            // Try to reconnect
            close();
            if (open())
            {
               nIteration = 100;    // Force all parameters to be polled
            }
            else
            {
               AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("UPS: Lost communication with device #%d \"%s\""), m_nIndex, m_pszName);
            }
         }
      }

      // query parameters if we are connected
      if (m_bIsConnected)
      {
         MutexLock(m_mutex);

         // Check rarely changing parameters only every 100th poll
         if (nIteration == 100)
         {
            nIteration = 0;
            queryModel();
            queryFirmwareVersion();
            queryMfgDate();
            querySerialNumber();
         }

         queryTemperature();
         queryBatteryVoltage();
         queryNominalBatteryVoltage();
         queryBatteryLevel();
         queryInputVoltage();
         queryOutputVoltage();
         queryLineFrequency();
         queryPowerLoad();
         queryEstimatedRuntime();
         queryOnlineStatus();

         MutexUnlock(m_mutex);

         AgentWriteDebugLog(9, _T("UPS: poll finished for device #%d \"%s\""), m_nIndex, m_pszName);
      }
   }
}
