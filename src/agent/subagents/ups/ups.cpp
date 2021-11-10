/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2021 Victor Kirhenshtein
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
UPSInterface::UPSInterface(const TCHAR *device) : m_condStop(true)
{
   m_id = 0;
   m_name = nullptr;
   m_device = MemCopyString(device);
   m_isConnected = false;
   memset(m_paramList, 0, sizeof(UPS_PARAMETER) * UPS_PARAM_COUNT);
   for(int i = 0; i < UPS_PARAM_COUNT; i++)
      m_paramList[i].flags |= UPF_NULL_VALUE;
   m_commThread = INVALID_THREAD_HANDLE;
}

/**
 * Destructor
 */
UPSInterface::~UPSInterface()
{
   m_condStop.set();
   ThreadJoin(m_commThread);
   MemFree(m_device);
   MemFree(m_name);
}

/**
 * Set name
 */
void UPSInterface::setName(const char *pszName)
{
   MemFree(m_name);
   if (pszName[0] == 0)
   {
      TCHAR szBuffer[MAX_DB_STRING];

      _sntprintf(szBuffer, MAX_DB_STRING, _T("%s-%s"), getType(), m_device);
      m_name = _tcsdup(szBuffer);
   }
   else
   {
#ifdef UNICODE
      m_name = WideStringFromMBString(pszName);
#else
      m_name = MemCopyStringA(pszName);
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
	   MemFree(m_name);
      m_name = _tcsdup(pszName);
   }
}

#endif

/**
 * Open communication to UPS
 */
bool UPSInterface::open()
{
   return false;
}

/**
 * Close communications with UPS
 */
void UPSInterface::close()
{
   m_isConnected = false;
}

/**
 * Validate connection to the UPS
 */
bool UPSInterface::validateConnection()
{
   return false;
}

/**
 * Get parameter's value
 */
LONG UPSInterface::getParameter(int parameterIndex, TCHAR *value)
{
   if ((parameterIndex < 0) || (parameterIndex >= UPS_PARAM_COUNT))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc;
   m_mutex.lock();
   if (m_paramList[parameterIndex].flags & UPF_NOT_SUPPORTED)
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }
   else if (m_paramList[parameterIndex].flags & UPF_NULL_VALUE)
   {
      rc = SYSINFO_RC_ERROR;
   }
   else
   {
#ifdef UNICODE
		mb_to_wchar(m_paramList[parameterIndex].value, -1, value, MAX_RESULT_LENGTH);
#else
      strcpy(value, m_paramList[parameterIndex].value);
#endif
      rc = SYSINFO_RC_SUCCESS;
   }
   m_mutex.unlock();
   return rc;
}

/**
 * Parameter query methods need to be overriden
 */
void UPSInterface::queryModel()
{
   m_paramList[UPS_PARAM_MODEL].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryFirmwareVersion()
{
   m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryMfgDate()
{
   m_paramList[UPS_PARAM_MFG_DATE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::querySerialNumber()
{
   m_paramList[UPS_PARAM_SERIAL].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryTemperature()
{
   m_paramList[UPS_PARAM_TEMP].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryBatteryVoltage()
{
   m_paramList[UPS_PARAM_BATTERY_VOLTAGE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryNominalBatteryVoltage()
{
   m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryBatteryLevel()
{
   m_paramList[UPS_PARAM_BATTERY_LEVEL].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryInputVoltage()
{
   m_paramList[UPS_PARAM_INPUT_VOLTAGE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryOutputVoltage()
{
   m_paramList[UPS_PARAM_OUTPUT_VOLTAGE].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryLineFrequency()
{
   m_paramList[UPS_PARAM_LINE_FREQ].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryPowerLoad()
{
   m_paramList[UPS_PARAM_LOAD].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryEstimatedRuntime()
{
   m_paramList[UPS_PARAM_EST_RUNTIME].flags |= UPF_NOT_SUPPORTED;
}

void UPSInterface::queryOnlineStatus()
{
   m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NOT_SUPPORTED;
}

/**
 * Query static data
 */
void UPSInterface::queryStaticData()
{
   queryModel();
   queryFirmwareVersion();
   queryMfgDate();
   querySerialNumber();
}

/**
 * Query dynamic data
 */
void UPSInterface::queryDynamicData()
{
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
}

/**
 * Start communication thread
 */
void UPSInterface::startCommunication()
{
   m_commThread = ThreadCreateEx(this, &UPSInterface::commThread);
}

/**
 * Communication thread
 */
void UPSInterface::commThread()
{
   int nIteration;

   // Try to open device immediately after start
   if (open())
   {
      nxlog_write_tag(NXLOG_INFO, UPS_DEBUG_TAG, _T("Established communication with device #%d \"%s\""), m_id, m_name);

      // Open successfully, query all parameters
      m_mutex.lock();
      queryStaticData();
      queryDynamicData();
      m_mutex.unlock();

      nxlog_debug_tag(UPS_DEBUG_TAG, 5, _T("Initial poll finished for device #%d \"%s\""), m_id, m_name);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, UPS_DEBUG_TAG, _T("Cannot establish communication with device #%d \"%s\""), m_id, m_name);
   }

   for(nIteration = 0; ; nIteration++)
   {
      if (m_condStop.wait(10000))
         break;

      // Try to connect to device if it is not connected
      if (!m_isConnected)
      {
         if (open())
         {
            nxlog_write_tag(NXLOG_INFO, UPS_DEBUG_TAG, _T("Established communication with device #%d \"%s\""), m_id, m_name);
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
               nxlog_write_tag(NXLOG_WARNING, UPS_DEBUG_TAG, _T("Lost communication with device #%d \"%s\""), m_id, m_name);
            }
         }
      }

      // query parameters if we are connected
      if (m_isConnected)
      {
         m_mutex.lock();

         // Check rarely changing parameters only every 100th poll
         if (nIteration == 100)
         {
            nIteration = 0;
            queryStaticData();
         }
         queryDynamicData();
         
         m_mutex.unlock();
         nxlog_debug_tag(UPS_DEBUG_TAG, 9, _T("Poll finished for device #%d \"%s\""), m_id, m_name);
      }
   }
}
