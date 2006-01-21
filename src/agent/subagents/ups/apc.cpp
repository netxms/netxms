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
** File: apc.cpp
**
**/

#include "ups.h"


//
// Open device
//

BOOL APCInterface::Open(void)
{
   char szLine[256];
   BOOL bRet;

   if (!UPSInterface::Open())
      return FALSE;

   m_serial.SetTimeout(1000);
   m_serial.Set(2400, 8, NOPARITY, ONESTOPBIT);

   // Turn on "smart" mode
   m_serial.Write("Y", 1);
   bRet = ReadLineFromSerial(szLine, 256);
   if (bRet && !strcmp(szLine, "SM"))
   {
      bRet = TRUE;
   }
   else
   {
      bRet = FALSE;
   }

   return bRet;
}


//
// Get UPS model
//

LONG APCInterface::GetModel(TCHAR *pszBuffer)
{
   m_serial.Write("\x01", 1);
   return ReadLineFromSerial(pszBuffer, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}


//
// Get UPS firmware version
//

LONG APCInterface::GetFirmwareVersion(TCHAR *pszBuffer)
{
   m_serial.Write("V", 1);
   return ReadLineFromSerial(pszBuffer, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}


//
// Get manufacturing date
//

LONG APCInterface::GetMfgDate(TCHAR *pszBuffer)
{
   m_serial.Write("m", 1);
   return ReadLineFromSerial(pszBuffer, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}


//
// Get serial number
//

LONG APCInterface::GetSerialNumber(TCHAR *pszBuffer)
{
   m_serial.Write("n", 1);
   return ReadLineFromSerial(pszBuffer, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_UNSUPPORTED;
}


//
// Get temperature inside UPS
//

LONG APCInterface::GetTemperature(LONG *pnTemp)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("C", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      pErr = strchr(szLine, '.');
      if (pErr != NULL)
         *pErr = 0;
      *pnTemp = strtol(szLine, &pErr, 10);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get battery voltage
//

LONG APCInterface::GetBatteryVoltage(double *pdVoltage)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("B", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      *pdVoltage = strtod(szLine, &pErr);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get nominal battery voltage
//

LONG APCInterface::GetNominalBatteryVoltage(double *pdVoltage)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("g", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      *pdVoltage = strtod(szLine, &pErr);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get battery level (in percents)
//

LONG APCInterface::GetBatteryLevel(LONG *pnLevel)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("f", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      pErr = strchr(szLine, '.');
      if (pErr != NULL)
         *pErr = 0;
      *pnLevel = strtol(szLine, &pErr, 10);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get input line voltage
//

LONG APCInterface::GetInputVoltage(double *pdVoltage)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("L", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      *pdVoltage = strtod(szLine, &pErr);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get output voltage
//

LONG APCInterface::GetOutputVoltage(double *pdVoltage)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("O", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      *pdVoltage = strtod(szLine, &pErr);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get line frequency (Hz)
//

LONG APCInterface::GetLineFrequency(LONG *pnFrequency)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("F", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      pErr = strchr(szLine, '.');
      if (pErr != NULL)
         *pErr = 0;
      *pnFrequency = strtol(szLine, &pErr, 10);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get UPS power load (in percents)
//

LONG APCInterface::GetPowerLoad(LONG *pnLoad)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("P", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      pErr = strchr(szLine, '.');
      if (pErr != NULL)
         *pErr = 0;
      *pnLoad = strtol(szLine, &pErr, 10);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}


//
// Get estimated on-battery runtime
//

LONG APCInterface::GetEstimatedRuntime(LONG *pnMinutes)
{
   char *pErr, szLine[256];
   LONG nRet = SYSINFO_RC_UNSUPPORTED;

   m_serial.Write("j", 1);
   if (ReadLineFromSerial(szLine, 256))
   {
      pErr = strchr(szLine, ':');
      if (pErr != NULL)
         *pErr = 0;
      *pnMinutes = strtol(szLine, &pErr, 10);
      nRet = ((*pErr == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR);
   }
   return nRet;
}
