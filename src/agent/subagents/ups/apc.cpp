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

/**
 * Data types for CheckNA()
 */
#define TYPE_STRING  0
#define TYPE_LONG    1
#define TYPE_DOUBLE  2

/**
 * Check if UPS reports parameter value as NA and set parameter's
 * flags acordingly. 
 */
static void CheckNA(UPS_PARAMETER *p, int nType)
{
   LONG nVal;
   double dVal;
   char *pErr;

   if (!strcmp(p->szValue, "NA"))
   {
      p->dwFlags |= UPF_NOT_SUPPORTED;
   }
   else
   {
      p->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      switch(nType)
      {
         case TYPE_LONG:
            nVal = strtol(p->szValue, &pErr, 10);
            if (*pErr == 0)
            {
               sprintf(p->szValue, "%d", nVal);
            }
            else
            {
               p->dwFlags |= UPF_NULL_VALUE;
            }
            break;
         case TYPE_DOUBLE:
            dVal = strtod(p->szValue, &pErr);
            if (*pErr == 0)
            {
               sprintf(p->szValue, "%f", dVal);
            }
            else
            {
               p->dwFlags |= UPF_NULL_VALUE;
            }
            break;
         default:
            break;
      }
   }
}

/**
 * Constructor
 */
APCInterface::APCInterface(TCHAR *pszDevice) : SerialInterface(pszDevice)
{
	if (m_portSpeed == 0)
		m_portSpeed = 2400;
}

/**
 * Query parameter from device
 */
void APCInterface::queryParameter(const char *pszRq, UPS_PARAMETER *p, int nType, int chSep)
{
   char *pch;

   m_serial.write(pszRq, 1);
   if (readLineFromSerial(p->szValue, MAX_RESULT_LENGTH))
   {
      if (chSep != -1)
      {
         pch = strchr(p->szValue, chSep);
         if (pch != NULL)
            *pch = 0;
      }
      CheckNA(p, nType);
   }
   else
   {
      p->dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Open device
 */
BOOL APCInterface::open()
{
   char szLine[256];
   BOOL bRet;

   if (!SerialInterface::open())
      return FALSE;

   m_serial.setTimeout(1000);
   m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

   // Turn on "smart" mode
   m_serial.write("Y", 1);
   bRet = readLineFromSerial(szLine, 256);
   if (bRet && !strcmp(szLine, "SM"))
   {
      bRet = TRUE;
      setConnected();
		
		// Query model and set as device name
		char buffer[MAX_RESULT_LENGTH];
   	m_serial.write("\x01", 1);
		if (readLineFromSerial(buffer, MAX_RESULT_LENGTH))
		{
			StrStripA(buffer);
			setName(buffer);
		}
   }
   else
   {
      bRet = FALSE;
   }

   return bRet;
}

/**
 * Validate connection
 */
BOOL APCInterface::validateConnection()
{
   BOOL bRet;
   char szLine[256];

   m_serial.write("Y", 1);
   bRet = readLineFromSerial(szLine, 256);
   return (bRet && !strcmp(szLine, "SM"));
}

/**
 * Get UPS model
 */
void APCInterface::queryModel()
{
   queryParameter("\x01", &m_paramList[UPS_PARAM_MODEL], TYPE_STRING, -1);
}

/**
 * Get UPS firmware version
 */
void APCInterface::queryFirmwareVersion()
{
   char szRev[256], szVer[256];

   m_serial.write("V", 1);
   if (!readLineFromSerial(szVer, 256))
   {
      szVer[0] = 0;
   }

   m_serial.write("b", 1);
   if (!readLineFromSerial(szRev, 256))
   {
      szRev[0] = 0;
   }
      
   if ((szVer[0] != 0) || (szRev[0] != 0))
   {
      snprintf(m_paramList[UPS_PARAM_FIRMWARE].szValue, MAX_RESULT_LENGTH, "%s%s%s", szVer,
               ((szVer[0] != 0) && (szRev[0] != 0)) ? " " : "", szRev);
      m_paramList[UPS_PARAM_FIRMWARE].dwFlags &= ~UPF_NULL_VALUE;
   }
   else
   {
      m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get manufacturing date
 */
void APCInterface::queryMfgDate()
{
   queryParameter("m", &m_paramList[UPS_PARAM_MFG_DATE], TYPE_STRING, -1);
}

/**
 * Get serial number
 */
void APCInterface::querySerialNumber()
{
   queryParameter("n", &m_paramList[UPS_PARAM_SERIAL], TYPE_STRING, -1);
}

/**
 * Get temperature inside UPS
 */
void APCInterface::queryTemperature()
{
   queryParameter("C", &m_paramList[UPS_PARAM_TEMP], TYPE_LONG, '.');
}

/**
 * Get battery voltage
 */
void APCInterface::queryBatteryVoltage()
{
   queryParameter("B", &m_paramList[UPS_PARAM_BATTERY_VOLTAGE], TYPE_DOUBLE, -1);
}

/**
 * Get nominal battery voltage
 */
void APCInterface::queryNominalBatteryVoltage()
{
   queryParameter("g", &m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE], TYPE_DOUBLE, -1);
}

/**
 * Get battery level (in percents)
 */
void APCInterface::queryBatteryLevel()
{
   queryParameter("f", &m_paramList[UPS_PARAM_BATTERY_LEVEL], TYPE_LONG, '.');
}

/**
 * Get input line voltage
 */
void APCInterface::queryInputVoltage()
{
   queryParameter("L", &m_paramList[UPS_PARAM_INPUT_VOLTAGE], TYPE_DOUBLE, -1);
}

/**
 * Get output voltage
 */
void APCInterface::queryOutputVoltage()
{
   queryParameter("O", &m_paramList[UPS_PARAM_OUTPUT_VOLTAGE], TYPE_DOUBLE, -1);
}


//
// Get line frequency (Hz)
//

void APCInterface::queryLineFrequency()
{
   queryParameter("F", &m_paramList[UPS_PARAM_LINE_FREQ], TYPE_LONG, '.');
}


//
// Get UPS power load (in percents)
//

void APCInterface::queryPowerLoad()
{
   queryParameter("P", &m_paramList[UPS_PARAM_LOAD], TYPE_LONG, '.');
}


//
// Get estimated on-battery runtime
//

void APCInterface::queryEstimatedRuntime()
{
   queryParameter("j", &m_paramList[UPS_PARAM_EST_RUNTIME], TYPE_LONG, ':');
}


//
// Check if UPS is online or on battery power
//

void APCInterface::queryOnlineStatus()
{
   char *eptr, szLine[MAX_RESULT_LENGTH];
   DWORD dwFlags;

   m_serial.write("Q", 1);
   if (readLineFromSerial(szLine, MAX_RESULT_LENGTH))
   {
      if (strcmp(szLine, "NA"))
      {
         dwFlags = strtoul(szLine, &eptr, 16);
         if (*eptr == 0)
         {
            m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[1] = 0;
            if (dwFlags & 0x08)  // online
            {
               m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '0';
            }
            else if (dwFlags & 0x10)   // on battery
            {
               if (dwFlags & 0x40)  // battery low
                  m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '2';
               else
                  m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '1';
            }
            m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags &= ~(UPF_NULL_VALUE | UPF_NOT_SUPPORTED);
         }
         else
         {
            m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
         }
      }
      else
      {
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
   }
}
