/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2015 Victor Kirhenshtein
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
** File: megatec.cpp
**
**/

#include "ups.h"

/**
 * Constructor
 */
MegatecInterface::MegatecInterface(const TCHAR *device) : SerialInterface(device)
{
	if (m_portSpeed == 0)
		m_portSpeed = 2400;
   m_packs = 0;

   m_paramList[UPS_PARAM_MFG_DATE].dwFlags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_SERIAL].dwFlags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_BATTERY_LEVEL].dwFlags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_EST_RUNTIME].dwFlags |= UPF_NOT_SUPPORTED;
}

/**
 * Open device
 */
BOOL MegatecInterface::open()
{
   if (!SerialInterface::open())
      return FALSE;

   m_serial.setTimeout(1000);
   m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

   // Read nominal values
   m_serial.write("F\r", 2);
   char buffer[256];
   bool success = readLineFromSerial(buffer, 256, '\r');
   if (success && (buffer[0] == '#'))
   {
      setConnected();
		
      buffer[16] = 0;
      double nominalVoltage = strtod(&buffer[11], NULL);
      sprintf(m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].szValue, "%0.2f", nominalVoltage);
      m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].dwFlags &= ~UPF_NULL_VALUE;
      
      m_serial.write("Q1\r", 3);
      if (readLineFromSerial(buffer, 256, '\r'))
      {
         if (buffer[0] == '(')
         {
            buffer[32] = 0;
            calculatePacks(nominalVoltage, strtod(&buffer[28], NULL));
         }
      }
   }
   else
   {
      success = false;
   }

   return success;
}

/**
 * Calculate number of battery packs
 */
void MegatecInterface::calculatePacks(double nominalVoltage, double actualVoltage)
{
   static double packs[] = { 120, 100, 80, 60, 48, 36, 30, 24, 18, 12, 8, 6, 4, 3, 2, 1, 0.5, 0 };
   for(int i = 0; packs[i] > 0; i++)
   {
      if (actualVoltage * packs[i] > nominalVoltage * 1.2)
         continue;
      if (actualVoltage * packs[i] < nominalVoltage * 0.8)
         break;

      m_packs = packs[i];
      break;
   }
   AgentWriteDebugLog(4, _T("UPS: MEGATEC interface init: packs=%0.1f"), m_packs);
}

/**
 * Validate connection
 */
BOOL MegatecInterface::validateConnection()
{
   m_serial.write("F\r", 2);
   char buffer[256];
   bool success = readLineFromSerial(buffer, 256, '\r');
   return success && (buffer[0] == '#');
}

/**
 * Query static data
 */
void MegatecInterface::queryStaticData()
{
   m_serial.write("I\r", 2);
   
   char buffer[256];
   if (readLineFromSerial(buffer, 256, '\r'))
   {
      if (buffer[0] == '#')
      {
         buffer[27] = 0;
         StrStripA(&buffer[17]);
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, buffer);

         StrStripA(&buffer[28]);
         strcpy(m_paramList[UPS_PARAM_FIRMWARE].szValue, buffer);

         m_paramList[UPS_PARAM_MODEL].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         m_paramList[UPS_PARAM_FIRMWARE].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      }
      else
      {
         m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NOT_SUPPORTED;
         m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NOT_SUPPORTED;
      m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NOT_SUPPORTED;
   }
}

/**
 * Query dynamic data
 */
void MegatecInterface::queryDynamicData()
{
   static int paramIndex[] = { UPS_PARAM_INPUT_VOLTAGE, -1, UPS_PARAM_OUTPUT_VOLTAGE, UPS_PARAM_LOAD, UPS_PARAM_LINE_FREQ, UPS_PARAM_BATTERY_VOLTAGE, UPS_PARAM_TEMP };
   m_serial.write("Q1\r", 3);
   
   char buffer[256];
   if (readLineFromSerial(buffer, 256, '\r'))
   {
      if (buffer[0] == '(')
      {
         const char *curr = &buffer[1];
         for(int i = 0; i < 7; i++)
         {
            char buffer[64];
            curr = ExtractWordA(curr, buffer);
            int index = paramIndex[i];
            if (index == -1)
               continue;
            strcpy(m_paramList[index].szValue, buffer);
            m_paramList[index].dwFlags &= ~UPF_NULL_VALUE;
         }
         
         // curr should point to bit flags now
         strcpy(m_paramList[UPS_PARAM_ONLINE_STATUS].szValue, curr[0] == '1' ? "1" : "0");
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags &= ~UPF_NULL_VALUE;

         // recalculate battery voltage for online devices
         if ((curr[4] == '0') && (m_packs > 0))
         {
            sprintf(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].szValue, "%0.2f", strtod(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].szValue, NULL) * m_packs);
         }
      }
      else
      {
         for(int i = 0; i < 7; i++)
         {
            if (paramIndex[i] != -1)
               m_paramList[paramIndex[i]].dwFlags |= UPF_NULL_VALUE;
         }
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      for(int i = 0; i < 7; i++)
      {
         if (paramIndex[i] != -1)
            m_paramList[paramIndex[i]].dwFlags |= UPF_NULL_VALUE;
      }
      m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
   }
}
