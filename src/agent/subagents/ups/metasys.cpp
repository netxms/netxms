/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
** Code is partially based on  Meta System driver for NUT:
**
** ! metasys.c - driver for Meta System UPS
** !
** ! Copyright (C) 2004 Fabio Di Niro <fabio.diniro@email.it>**
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
** File: metasys.cpp
**
**/

#include "ups.h"
#include <math.h>

/**
 * Commands
 */
#define MS_COMMAND_START_BYTE    ((BYTE)0x02)

#define MS_INFO                  ((BYTE)0x00)
#define MS_OUTPUT_DATA           ((BYTE)0x01)
#define MS_INPUT_DATA            ((BYTE)0x02)
#define MS_STATUS                ((BYTE)0x03)
#define MS_BATTERY_DATA          ((BYTE)0x04)
#define MS_HISTORY_DATA          ((BYTE)0x05)
#define MS_GET_SCHEDULING        ((BYTE)0x06)
#define MS_EVENT_LIST            ((BYTE)0x07)
#define MS_GET_TIMES_ON_BATTERY  ((BYTE)0x08)
#define MS_GET_NEUTRAL_SENSE     ((BYTE)0x09)
#define MS_SET_SCHEDULING        ((BYTE)0x0B)
#define MS_SET_NEUTRAL_SENSE     ((BYTE)0x0B)
#define MS_SET_TIMES_ON_BATTERY  ((BYTE)0x0C)
#define MS_SET_BUZZER_MUTE       ((BYTE)0x0D)
#define MS_SET_BATTERY_TEST      ((BYTE)0x0E)

/**
 * Meter map elements
 */
#define MAP_OUTPUT_VA      			23
#define MAP_INPUT_FREQUENCY         28
#define MAP_BATTERY_VOLTAGE         33
#define MAP_BATTERY_LEVEL           34
#define MAP_BATTERY_RUNTIME         35
#define MAP_INPUT_VOLTAGE           56
#define MAP_AMBIENT_TEMPERATURE     62
#define MAP_UPS_TEMPERATURE         63
#define MAP_MAX_OUTPUT_VA     		71
#define MAP_BATTERY_TEMPERATURE     77
#define MAP_OUTPUT_VOLTAGE          78

/**
 * Result formats
 */
#define FMT_FLOAT     0
#define FMT_DOUBLE      1
#define FMT_STRING      2
#define FMT_INT_MINUTES 3

/**
 * Calculate checksum for prepared message and add it to the end of buffer
 */
static void CalculateChecksum(BYTE *pBuffer)
{
	BYTE sum;
	int i, nLen;

   nLen = pBuffer[1];
	for(sum = 0, i = 0; i < nLen; i++)
		sum += pBuffer[i + 1];
   pBuffer[nLen + 1] = sum;
}

/**
 * Validate checksum of received packet
 */
static BOOL ValidateChecksum(BYTE *pBuffer)
{
   BYTE sum;
   int i, nLen;

   nLen = pBuffer[1];
	for(i = 0, sum = pBuffer[nLen + 1]; i < nLen; i++)
		sum -= pBuffer[i + 1];
   return sum == 0;
}

/**
 * Get value as long integer
 */
static INT32 GetLong(BYTE *pData)
{
#if WORDS_BIGENDIAN
	return (INT32)(((UINT32)pData[3] << 24) | ((UINT32)pData[2] << 16) |
                 ((UINT32)pData[1] << 8) | (UINT32)pData[0]);
#else
   return *((INT32 *)pData);
#endif
}

/**
 * Get value as float
 */
static float GetFloat(BYTE *data)
{
   return (float)((((int)data[0]) << 8) | (int)data[1]) / 10;
}

/**
 * Decode numerical value
 */
static void DecodeValue(BYTE *data, int format, char *value)
{
   switch(format)
   {
      case FMT_FLOAT:
         sprintf(value, "%0.1f", GetFloat(data));
         break;
      default:
         strcpy(value, "ERROR");
         break;
   }
}

/**
 * Constructor
 */
MetaSysInterface::MetaSysInterface(TCHAR *pszDevice) : SerialInterface(pszDevice)
{
	if (m_portSpeed == 0)
		m_portSpeed = 2400;
}

/**
 * Send read command to device
 */
BOOL MetaSysInterface::sendReadCommand(BYTE nCommand)
{
   BYTE packet[4];
   BOOL bRet;
   int nRetries = 5;

   packet[0] = MS_COMMAND_START_BYTE;
   packet[1] = 0x02; // length
   packet[2] = nCommand;
   CalculateChecksum(packet);
   do
   {
      bRet = m_serial.write((char *)packet, 4);
      nRetries--;
   } while((!bRet) && (nRetries > 0));
   return bRet;
}

/**
 * Receive data block from device, and check hat it is for right command
 */
int MetaSysInterface::recvData(int nCommand)
{
   BYTE packet[256];
   int nCount, nLength, nBytes, nRead;

   memset(m_data, 0, BCMXCP_BUFFER_SIZE);
   nCount = 0;
   do
   {
		// Read MS_COMMAND_START_BYTE byte
		if (m_serial.read((char *)packet, 1) != 1)
         return -1;  // Read error
		nCount++;
	} while((packet[0] != MS_COMMAND_START_BYTE) && (nCount < 256));
	
	if (nCount == 256)
		return -1;  // Didn't get command start byte

	// Read data lenght byte
   if (m_serial.read((char *)&packet[1], 1) != 1)
      return -1;  // Read error
	nLength = packet[1];
	if (nLength < 2) 
		return -1;  // Invalid data length

	// Read data
   for(nRead = 0; nRead < nLength; nRead += nBytes)
   {
      nBytes = m_serial.read((char *)&packet[nRead + 2], nLength - nRead);
      if (nBytes <= 0)
         return -1;  // Read error
   }

   // Check if packet is a responce to right command
	if (nCommand != packet[2])
		return -1;  // Invalid command

	// Validate packet's checksum
	if (!ValidateChecksum(packet))
		return -1;  // Bad checksum

	memcpy(m_data, &packet[2], nLength - 1);
	return nLength - 1;
}

/**
 * Open device
 */
BOOL MetaSysInterface::open()
{
   BOOL bRet = FALSE;

   if (SerialInterface::open())
   {
      m_serial.setTimeout(1000);
      m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

      // Send 100 zeroes to reset UPS serial interface
      char zeroes[100];
      memset(zeroes, 0, 100);
      m_serial.write(zeroes, 100);

      // Read UPS information
      if (sendReadCommand(MS_INFO))
      {
         int nBytes = recvData(MS_INFO);
         if (nBytes > 0)
         {
            parseModelId();

            memset(m_paramList[UPS_PARAM_SERIAL].szValue, 0, 13);
            memcpy(m_paramList[UPS_PARAM_SERIAL].szValue, m_data + 7, min(12, nBytes - 7));
            StrStripA(m_paramList[UPS_PARAM_SERIAL].szValue);

            sprintf(m_paramList[UPS_PARAM_FIRMWARE].szValue, "%d.%02d", m_data[5], m_data[6]);

            m_paramList[UPS_PARAM_MODEL].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
            m_paramList[UPS_PARAM_SERIAL].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
            m_paramList[UPS_PARAM_FIRMWARE].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);

            bRet = TRUE;
            setConnected();
         }
      }
   }
   return bRet;
}

/**
 * Parse UPS model ID
 */
void MetaSysInterface::parseModelId()
{
   // Model name hardcoded into protocol
   int id = m_data[1] * 10 + m_data[2];
   switch (id) 
   {        
      case 11:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (1 board)");
         break;
      case 12:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (2 boards)");
         break;
      case 13:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (3 boards)");
         break;
      case 14:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (4 boards)");
         break;
      case 21:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO Network 750/1000");
         break;        
      case 22:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO Network 1050/1500");
         break;        
      case 23:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO Network 1500/2000");
         break;        
      case 24:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO Network 1800/2500");
         break;        
      case 25:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO Network 2100/3000");
         break;        
      case 31:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 308");
         break;        
      case 32:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 311");
         break;        
      case 44:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (4 boards)/2");
         break;
      case 45:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (5 boards)/2");
         break;
      case 46:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (6 boards)/2");
         break;
      case 47:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (7 boards)/2");
         break;
      case 48:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Line (8 boards)/2");
         break;
      case 51:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Millennium 810");
         break;
      case 52:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF Millennium 820");
         break;
      case 61:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 910");
         break;
      case 62:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 920");
         break;
      case 63:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 930");
         break;
      case 64:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 940");
         break;
      case 74:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 940/2");
         break;
      case 75:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 950/2");
         break;
      case 76:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 960/2");
         break;
      case 77:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 970/2");
         break;
      case 78:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "HF TOP Line 980/2");
         break;
      case 81:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 508");
         break;
      case 82:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 511");
         break;
      case 83:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 516");
         break;
      case 84:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 519");
         break;
      case 85:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 522");
         break;
      case 91:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 305 / Harviot 530 SX");
         break;
      case 92:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ORDINATORE 2");
         break;
      case 93:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Harviot 730 SX");
         break;
      case 101:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 308 SX / SX Interactive / Ordinatore");
         break;
      case 102:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ECO 311 SX / SX Interactive");
         break;
      case 111:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 800 / BI-TWICE 800");
         break;
      case 112:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 1600");
         break;
      case 121:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 1000 / BI-TWICE 1000");
         break;
      case 122:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 2000");
         break;
      case 131:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 1250 / BI-TWICE 1250");
         break;
      case 132:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "ally HF 2500");
         break;
      case 141:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 1250");
         break;
      case 142:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 2500");
         break;
      case 143:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 3750");
         break;
      case 144:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 5000");
         break;
      case 154:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 5000/2");
         break;
      case 155:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 6250/2");
         break;
      case 156:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 7500/2");
         break;
      case 157:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 8750/2");
         break;
      case 158:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "Megaline 10000/2");
         break;
      default:
         strcpy(m_paramList[UPS_PARAM_MODEL].szValue, "unknown");
         break;
   } 
}

/**
 * Validate connection to the device
 */
BOOL MetaSysInterface::validateConnection()
{
   if (sendReadCommand(MS_INFO))
   {
      if (recvData(MS_INFO) > 0)
         return TRUE;
   }
   return FALSE;
}

/**
 * Read parameter from UPS
 */
void MetaSysInterface::readParameter(int command, int offset, int format, UPS_PARAMETER *param)
{
   if (sendReadCommand(command))
   {
      int nBytes = recvData(command);
      if (nBytes > 0)
      {
         if (offset < nBytes)
         {
            DecodeValue(&m_data[offset], format, param->szValue);
            param->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         }
         else
         {
            param->dwFlags |= UPF_NOT_SUPPORTED;
         }
      }
      else
      {
         param->dwFlags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      param->dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get UPS model
 */
void MetaSysInterface::queryModel()
{
   // read on open, nothing to do here
}

/**
 * Get battery voltage
 */
void MetaSysInterface::queryBatteryVoltage()
{
   readParameter(MS_BATTERY_DATA, FMT_FLOAT, 1, &m_paramList[UPS_PARAM_BATTERY_LEVEL]);
}

/**
 * Get input line voltage
 */
void MetaSysInterface::queryInputVoltage()
{
   readParameter(MAP_INPUT_VOLTAGE, FMT_DOUBLE, &m_paramList[UPS_PARAM_INPUT_VOLTAGE]);
}

/**
 * Get output voltage
 */
void MetaSysInterface::queryOutputVoltage()
{
   readParameter(MAP_OUTPUT_VOLTAGE, FMT_DOUBLE, &m_paramList[UPS_PARAM_OUTPUT_VOLTAGE]);
}

/**
 * Get estimated runtime (in minutes)
 */
void MetaSysInterface::queryEstimatedRuntime()
{
   readParameter(MAP_BATTERY_RUNTIME, FMT_INT_MINUTES, &m_paramList[UPS_PARAM_EST_RUNTIME]);
}

/**
 * Get temperature inside UPS
 */
void MetaSysInterface::queryTemperature()
{
   readParameter(MAP_AMBIENT_TEMPERATURE, FMT_INTEGER, &m_paramList[UPS_PARAM_TEMP]);
}

/**
 * Get line frequency (Hz)
 */
void MetaSysInterface::queryLineFrequency()
{
   readParameter(MAP_INPUT_FREQUENCY, FMT_INTEGER, &m_paramList[UPS_PARAM_LINE_FREQ]);
}

/**
 * Get firmware version
 */
void MetaSysInterface::queryFirmwareVersion()
{
   // read on open, nothing to do here
}

/**
 * Get unit serial number
 */
void MetaSysInterface::querySerialNumber()
{
   // read on open, nothing to do here
}

/**
 * Get UPS online status
 */
void MetaSysInterface::queryOnlineStatus()
{
   int nBytes;

   if (sendReadCommand(PW_STATUS_REQ))
   {
      nBytes = recvData(PW_STATUS_REQ);
      if (nBytes > 0)
      {
         switch(m_data[0])
         {
            case 0x50:  // Online
               m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '0';
               break;
            case 0xF0:  // On battery
               if (m_data[1] & 0x20)
                  m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '2';   // Low battery
               else
                  m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '1';
               break;
            default:    // Unknown status, assume OK
               m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[0] = '0';
               break;
         }
         m_paramList[UPS_PARAM_ONLINE_STATUS].szValue[1] = 0;
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      }
      else
      {
         m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_ONLINE_STATUS].dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get UPS load
 */
void MetaSysInterface::queryPowerLoad()
{
   UPS_PARAMETER upsCurrOutput, upsMaxOutput;
   int nCurrOutput, nMaxOutput;

   readParameter(MAP_OUTPUT_VA, FMT_INTEGER, &upsCurrOutput);
   readParameter(MAP_MAX_OUTPUT_VA, FMT_INTEGER, &upsMaxOutput);
   m_paramList[UPS_PARAM_LOAD].dwFlags = upsCurrOutput.dwFlags | upsMaxOutput.dwFlags;
   if ((m_paramList[UPS_PARAM_LOAD].dwFlags & (UPF_NOT_SUPPORTED | UPF_NULL_VALUE)) == 0)
   {
      nCurrOutput = atoi(upsCurrOutput.szValue);
      nMaxOutput = atoi(upsMaxOutput.szValue);
      if ((nMaxOutput > 0) && (nMaxOutput >= nCurrOutput))
      {
         sprintf(m_paramList[UPS_PARAM_LOAD].szValue, "%d", nCurrOutput * 100 / nMaxOutput);
      }
      else
      {
         m_paramList[UPS_PARAM_LOAD].dwFlags |= UPF_NULL_VALUE;
      }
   }
}
