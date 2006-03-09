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
** File: ups.cpp
**
**/

#include "ups.h"


//
// Abstract class constructor
//

UPSInterface::UPSInterface(TCHAR *pszDevice)
{
   m_pszName = NULL;
   m_pszDevice = _tcsdup(pszDevice);
   m_bIsConnected = FALSE;
}


//
// Destructor
//

UPSInterface::~UPSInterface()
{
   safe_free(m_pszDevice);
   safe_free(m_pszName);
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
// Methods need to be overriden
//

LONG UPSInterface::GetModel(TCHAR *pszBuffer)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetFirmwareVersion(TCHAR *pszBuffer)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetMfgDate(TCHAR *pszBuffer)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetSerialNumber(TCHAR *pszBuffer)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetTemperature(LONG *pnTemp)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetBatteryVoltage(double *pdVoltage)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetNominalBatteryVoltage(double *pdVoltage)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetBatteryLevel(LONG *pnLevel)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetInputVoltage(double *pdVoltage)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetOutputVoltage(double *pdVoltage)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetLineFrequency(LONG *pnFrequency)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetPowerLoad(LONG *pnLoad)
{
   return SYSINFO_RC_UNSUPPORTED;
}

LONG UPSInterface::GetEstimatedRuntime(LONG *pnMinutes)
{
   return SYSINFO_RC_UNSUPPORTED;
}
