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

   if (!strcmp(p->value, "NA"))
   {
      p->flags |= UPF_NOT_SUPPORTED;
   }
   else
   {
      p->flags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      switch(nType)
      {
         case TYPE_LONG:
            nVal = strtol(p->value, &pErr, 10);
            if (*pErr == 0)
            {
               sprintf(p->value, "%d", nVal);
            }
            else
            {
               p->flags |= UPF_NULL_VALUE;
            }
            break;
         case TYPE_DOUBLE:
            dVal = strtod(p->value, &pErr);
            if (*pErr == 0)
            {
               sprintf(p->value, "%f", dVal);
            }
            else
            {
               p->flags |= UPF_NULL_VALUE;
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
APCInterface::APCInterface(const TCHAR *device) : SerialInterface(device)
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
   if (readLineFromSerial(p->value, MAX_RESULT_LENGTH))
   {
      if (chSep != -1)
      {
         pch = strchr(p->value, chSep);
         if (pch != NULL)
            *pch = 0;
      }
      CheckNA(p, nType);
   }
   else
   {
      p->flags |= UPF_NULL_VALUE;
   }
}

/**
 * Open device
 */
bool APCInterface::open()
{
   if (!SerialInterface::open())
      return false;

   m_serial.setTimeout(1000);
   m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

   // Turn on "smart" mode
   m_serial.write("Y", 1);
   char response[MAX_RESULT_LENGTH];
   bool success = readLineFromSerial(response, MAX_RESULT_LENGTH);
   if (success && !strcmp(response, "SM"))
   {
      setConnected();
		
		// Query model and set as device name
   	m_serial.write("\x01", 1);
		if (readLineFromSerial(response, MAX_RESULT_LENGTH))
		{
			setName(TrimA(response));
		}
   }
   else
   {
      success = false;
   }

   return success;
}

/**
 * Validate connection
 */
bool APCInterface::validateConnection()
{
   m_serial.write("Y", 1);
   char response[256];
   bool success = readLineFromSerial(response, 256);
   return (success && !strcmp(response, "SM"));
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
   m_serial.write("V", 1);
   char version[256];
   if (!readLineFromSerial(version, 256))
   {
      version[0] = 0;
   }

   m_serial.write("b", 1);
   char revision[256];
   if (!readLineFromSerial(revision, 256))
   {
      revision[0] = 0;
   }
      
   if ((version[0] != 0) || (revision[0] != 0))
   {
      snprintf(m_paramList[UPS_PARAM_FIRMWARE].value, MAX_RESULT_LENGTH, "%s%s%s", version,
               ((version[0] != 0) && (revision[0] != 0)) ? " " : "", revision);
      m_paramList[UPS_PARAM_FIRMWARE].flags &= ~UPF_NULL_VALUE;
   }
   else
   {
      m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NULL_VALUE;
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

/**
 * Get line frequency (Hz)
 */
void APCInterface::queryLineFrequency()
{
   queryParameter("F", &m_paramList[UPS_PARAM_LINE_FREQ], TYPE_LONG, '.');
}

/**
 * Get UPS power load (in percents)
 */
void APCInterface::queryPowerLoad()
{
   queryParameter("P", &m_paramList[UPS_PARAM_LOAD], TYPE_LONG, '.');
}

/**
 * Get estimated on-battery runtime
 */
void APCInterface::queryEstimatedRuntime()
{
   queryParameter("j", &m_paramList[UPS_PARAM_EST_RUNTIME], TYPE_LONG, ':');
}

/**
 * Check if UPS is online or on battery power
 */
void APCInterface::queryOnlineStatus()
{
   char *eptr, szLine[MAX_RESULT_LENGTH];
   DWORD flags;

   m_serial.write("Q", 1);
   if (readLineFromSerial(szLine, MAX_RESULT_LENGTH))
   {
      if (strcmp(szLine, "NA"))
      {
         flags = strtoul(szLine, &eptr, 16);
         if (*eptr == 0)
         {
            m_paramList[UPS_PARAM_ONLINE_STATUS].value[1] = 0;
            if (flags & 0x08)  // online
            {
               m_paramList[UPS_PARAM_ONLINE_STATUS].value[0] = '0';
            }
            else if (flags & 0x10)   // on battery
            {
               if (flags & 0x40)  // battery low
                  m_paramList[UPS_PARAM_ONLINE_STATUS].value[0] = '2';
               else
                  m_paramList[UPS_PARAM_ONLINE_STATUS].value[0] = '1';
            }
            m_paramList[UPS_PARAM_ONLINE_STATUS].flags &= ~(UPF_NULL_VALUE | UPF_NOT_SUPPORTED);
         }
         else
         {
            m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
         }
      }
      else
      {
         m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
   }
}
