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

   m_paramList[UPS_PARAM_MFG_DATE].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_SERIAL].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_BATTERY_LEVEL].flags |= UPF_NOT_SUPPORTED;
   m_paramList[UPS_PARAM_EST_RUNTIME].flags |= UPF_NOT_SUPPORTED;
}

/**
 * Open device
 */
bool MegatecInterface::open()
{
   if (!SerialInterface::open())
      return false;

   m_serial.setTimeout(1000);
   m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

   // Read nominal values
   m_serial.write("F\r", 2);
   char buffer[256];
   bool success = readLineFromSerial(buffer, 256, '\r');
   if (success && (buffer[0] == '#'))
   {
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: received nominal values response \"%hs\""), buffer);

      setConnected();
		
      buffer[16] = 0;
      double nominalVoltage = strtod(&buffer[11], nullptr);
      sprintf(m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].value, "%0.2f", nominalVoltage);
      m_paramList[UPS_PARAM_NOMINAL_BATT_VOLTAGE].flags &= ~UPF_NULL_VALUE;
      
      m_serial.write("Q1\r", 3);
      if (readLineFromSerial(buffer, 256, '\r'))
      {
         if (buffer[0] == '(')
         {
            buffer[32] = 0;
            calculatePacks(nominalVoltage, strtod(&buffer[28], nullptr));
         }
      }
   }
   else
   {
      if (success)
         nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: invalid nominal values response \"%hs\""), buffer);
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
   nxlog_debug_tag(UPS_DEBUG_TAG, 4, _T("MEGATEC interface initialization: packs=%0.1f"), m_packs);
}

/**
 * Validate connection
 */
bool MegatecInterface::validateConnection()
{
   m_serial.write("F\r", 2);
   char buffer[256];
   bool success = readLineFromSerial(buffer, 256, '\r');
   if (success)
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: received nominal values response \"%hs\""), buffer);
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
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: received info response \"%hs\""), buffer);
      if (buffer[0] == '#')
      {
         buffer[27] = 0;
         TrimA(&buffer[17]);
         strcpy(m_paramList[UPS_PARAM_MODEL].value, &buffer[17]);

         TrimA(&buffer[28]);
         strcpy(m_paramList[UPS_PARAM_FIRMWARE].value, &buffer[28]);

         m_paramList[UPS_PARAM_MODEL].flags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         m_paramList[UPS_PARAM_FIRMWARE].flags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      }
      else
      {
         m_paramList[UPS_PARAM_MODEL].flags |= UPF_NOT_SUPPORTED;
         m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_MODEL].flags |= UPF_NOT_SUPPORTED;
      m_paramList[UPS_PARAM_FIRMWARE].flags |= UPF_NOT_SUPPORTED;
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
      nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: received status response \"%hs\""), buffer);
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

            char *p = buffer;
            while(*p == '0')
               p++;
            if (*p == 0)
               p--;
            strcpy(m_paramList[index].value, p);
            m_paramList[index].flags &= ~UPF_NULL_VALUE;
         }
         
         // curr should point to bit flags now
         while(isspace(*curr))
            curr++;
         strcpy(m_paramList[UPS_PARAM_ONLINE_STATUS].value, curr[0] == '1' ? (curr[1] == '1' ? "2" : "1") : "0");
         m_paramList[UPS_PARAM_ONLINE_STATUS].flags &= ~UPF_NULL_VALUE;
         nxlog_debug_tag(UPS_DEBUG_TAG, 7, _T("MEGATEC: status bits = %hs, online = %hs"), curr, m_paramList[UPS_PARAM_ONLINE_STATUS].value);

         // recalculate battery voltage for online devices
         if ((curr[4] == '0') && (m_packs > 0))
         {
            sprintf(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].value, "%0.2f", strtod(m_paramList[UPS_PARAM_BATTERY_VOLTAGE].value, nullptr) * m_packs);
         }
      }
      else
      {
         for(int i = 0; i < 7; i++)
         {
            if (paramIndex[i] != -1)
               m_paramList[paramIndex[i]].flags |= UPF_NULL_VALUE;
         }
         m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      for(int i = 0; i < 7; i++)
      {
         if (paramIndex[i] != -1)
            m_paramList[paramIndex[i]].flags |= UPF_NULL_VALUE;
      }
      m_paramList[UPS_PARAM_ONLINE_STATUS].flags |= UPF_NULL_VALUE;
   }
}
