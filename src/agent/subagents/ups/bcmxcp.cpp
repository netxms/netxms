/*
** NetXMS UPS management subagent
** Copyright (C) 2006-2014 Victor Kirhenshtein
** Code is partially based on BCMXCP driver for NUT:
**
** ! bcmxcp.c - driver for powerware UPS
** !
** ! Total rewrite of bcmxcp.c (nut ver-1.4.3)
** ! * Copyright (c) 2002, Martin Schroeder *
** ! * emes -at- geomer.de *
** ! * All rights reserved.*
** !
** ! Copyright (C) 2004 Kjell Claesson <kjell.claesson-at-telia.com>
** ! and Tore +rpetveit <tore-at-orpetveit.net>
** !
** ! Thanks to Tore +rpetveit <tore-at-orpetveit.net> that sent me the
** ! manuals for bcm/xcp.
** !
** ! And to Fabio Di Niro <fabio.diniro@email.it> and his metasys module.
** ! It influenced the layout of this driver.
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
** File: bcmxcp.cpp
**
**/

#include "ups.h"
#include <math.h>

/**
 * Commands
 */
#define PW_COMMAND_START_BYTE    ((BYTE)0xAB)

#define PW_ID_BLOCK_REQ 	      ((BYTE)0x31)
#define PW_STATUS_REQ 		      ((BYTE)0x33)
#define PW_METER_BLOCK_REQ	      ((BYTE)0x34)
#define PW_CUR_ALARM_REQ	      ((BYTE)0x35)
#define PW_CONFIG_BLOCK_REQ	   ((BYTE)0x36)
#define PW_BAT_TEST_REQ		      ((BYTE)0x3B)
#define PW_LIMIT_BLOCK_REQ	      ((BYTE)0x3C)
#define PW_TEST_RESULT_REQ	      ((BYTE)0x3F)
#define PW_COMMAND_LIST_REQ	   ((BYTE)0x40)
#define PW_OUT_MON_BLOCK_REQ	   ((BYTE)0x41)
#define PW_COM_CAP_REQ		      ((BYTE)0x42)
#define PW_UPS_TOPO_DATA_REQ	   ((BYTE)0x43)

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
#define FMT_INTEGER     0
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

   nLen = pBuffer[1] + 2;
	for(sum = 0, i = 0; i < nLen; i++)
		sum -= pBuffer[i];
   pBuffer[nLen] = sum;
}

/**
 * Validate checksum of received packet
 */
static BOOL ValidateChecksum(BYTE *pBuffer)
{
   BYTE sum;
   int i, nLen;

   nLen = pBuffer[2] + 5;
	for(i = 0, sum = 0; i < nLen; i++)
		sum += pBuffer[i];
   return sum == 0;
}

/**
 * Get value as long integer
 */
static LONG GetLong(BYTE *pData)
{
#if WORDS_BIGENDIAN
	return (LONG)(((DWORD)pData[3] << 24) | ((DWORD)pData[2] << 16) |
                 ((DWORD)pData[1] << 8) | (DWORD)pData[0]);
#else
   return *((LONG *)pData);
#endif
}

/**
 * Get value as float
 */
static float GetFloat(BYTE *pData)
{
#if WORDS_BIGENDIAN
   DWORD dwTemp;

	dwTemp = ((DWORD)pData[3] << 24) | ((DWORD)pData[2] << 16) |
            ((DWORD)pData[1] << 8) | (DWORD)pData[0];
   return *((float *)&dwTemp);
#else
   return *((float *)pData);
#endif
}

/**
 * Decode numerical value
 */
static void DecodeValue(BYTE *pData, int nDataFmt, int nOutputFmt, char *pszValue)
{
   LONG nValue;
   double dValue;
   int nInputFmt;

	if ((nDataFmt == 0xF0) || (nDataFmt == 0xE2))
   {
      nValue = GetLong(pData);
      nInputFmt = FMT_INTEGER;
	}
	else if ((nDataFmt & 0xF0) == 0xF0)
   {
		// Fixed point integer
		dValue = GetLong(pData) / ldexp((double)1, nDataFmt & 0x0F);
      nInputFmt = FMT_DOUBLE;
	}
	else if (nDataFmt <= 0x97)
   {
		// Floating point
		dValue = GetFloat(pData);
      nInputFmt = FMT_DOUBLE;
	}
	else if (nDataFmt == 0xE0)
   {
		// Date
      nInputFmt = FMT_INTEGER;
	}
	else if (nDataFmt == 0xE1)
   {
		// Time
      nInputFmt = FMT_INTEGER;
	}
	else
   {
      // Unknown format
      nValue = 0;
      nInputFmt = FMT_INTEGER;
	}

   // Convert value in input format to requested format
   switch(nInputFmt)
   {
      case FMT_INTEGER:
         dValue = nValue;
         break;
      case FMT_DOUBLE:
         nValue = (LONG)dValue;
         break;
   }

   switch(nOutputFmt)
   {
      case FMT_INTEGER:
         sprintf(pszValue, "%d", nValue);
         break;
      case FMT_INT_MINUTES:
         sprintf(pszValue, "%d", nValue / 60);
         break;
      case FMT_DOUBLE:
         sprintf(pszValue, "%f", dValue);
         break;
      default:
         strcpy(pszValue, "ERROR");
         break;
   }
}

/**
 * Constructor
 */
BCMXCPInterface::BCMXCPInterface(TCHAR *pszDevice) : SerialInterface(pszDevice)
{
	if (m_portSpeed == 0)
		m_portSpeed = 19200;
}

/**
 * Send read command to device
 */
BOOL BCMXCPInterface::sendReadCommand(BYTE nCommand)
{
   BYTE packet[4];
   BOOL bRet;
   int nRetries = 3;

   packet[0] = PW_COMMAND_START_BYTE;
   packet[1] = 0x01;
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
int BCMXCPInterface::recvData(int nCommand)
{
   BYTE packet[128], nPrevSequence = 0;
   BOOL bEndBlock = FALSE;
   int nCount, nBlock, nLength, nBytes, nRead, nTotalBytes = 0;

   memset(m_data, 0, BCMXCP_BUFFER_SIZE);
	while(!bEndBlock)
   {
      nCount = 0;
      do
      {
			// Read PW_COMMAND_START_BYTE byte
			if (m_serial.read((char *)packet, 1) != 1)
            return -1;  // Read error
			nCount++;
		} while((packet[0] != PW_COMMAND_START_BYTE) && (nCount < 128));
		
		if (nCount == 128)
			return -1;  // Didn't get command start byte

		// Read block number byte
      if (m_serial.read((char *)&packet[1], 1) != 1)
         return -1;  // Read error
		nBlock = packet[1];

      // Check if block is a responce to right command
		if (nCommand <= 0x43)
      {
			if ((nCommand - 0x30) != nBlock)
				return -1;  // Invalid block
		}
		else if (nCommand >= 0x89)
      {
			if (((nCommand == 0xA0) && (nBlock != 0x01)) ||
			    ((nCommand != 0xA0) && (nBlock != 0x09)))
				return -1;  // Invalid block
		}

		// Read data lenght byte
      if (m_serial.read((char *)&packet[2], 1) != 1)
         return -1;  // Read error
		nLength = packet[2];
		if (nLength < 1) 
			return -1;  // Invalid data length

		// Read sequence byte
      if (m_serial.read((char *)&packet[3], 1) != 1)
         return -1;  // Read error
		if ((packet[3] & 0x80) == 0x80)
         bEndBlock = TRUE;
		if ((packet[3] & 0x07) != (nPrevSequence + 1))
			return -1;  // Invalid sequence number
      nPrevSequence = packet[3];

		// Read data
      for(nRead = 0; nRead < nLength; nRead += nBytes)
      {
         nBytes = m_serial.read((char *)&packet[nRead + 4], nLength - nRead);
         if (nBytes <= 0)
            return -1;  // Read error
      }

		// Read the checksum byte
      if (m_serial.read((char *)&packet[nLength + 4], 1) != 1)
         return -1;  // Read error

		// Validate packet's checksum
		if (!ValidateChecksum(packet))
   		return -1;  // Bad checksum

		memcpy(&m_data[nTotalBytes], &packet[4], nLength);
		nTotalBytes += nLength;
	}
	return nTotalBytes;
}

/**
 * Open device
 */
BOOL BCMXCPInterface::open()
{
   BOOL bRet = FALSE;
   char szBuffer[256];
   int i, nBytes, nPos, nLen, nOffset;

   if (SerialInterface::open())
   {
      m_serial.setTimeout(1000);
      m_serial.set(m_portSpeed, m_dataBits, m_parity, m_stopBits);

      // Send two escapes
      m_serial.write("\x1D\x1D", 2);

      // Read UPS ID block
      if (sendReadCommand(PW_ID_BLOCK_REQ))
      {
         nBytes = recvData(PW_ID_BLOCK_REQ);
         if (nBytes > 0)
         {
            // Skip to model name
            nPos = m_data[0] * 2 + 1;
            nPos += (m_data[nPos] == 0) ? 5 : 3;
            nLen = min(m_data[nPos], 255);
            if ((nPos < nBytes) && (nPos + nLen <= nBytes))
            {
               memcpy(szBuffer, &m_data[nPos + 1], nLen);
               szBuffer[nLen] = 0;
               StrStripA(szBuffer);
               setName(szBuffer);
            }

            // Read meter map
            memset(m_map, 0, sizeof(BCMXCP_METER_MAP_ENTRY) * BCMXCP_MAP_SIZE);
            nPos += m_data[nPos] + 1;
            nLen = m_data[nPos++];
            for(i = 0, nOffset = 0; (i < nLen) && (i < BCMXCP_MAP_SIZE); i++, nPos++)
            {
               m_map[i].nFormat = m_data[nPos];
               if (m_map[i].nFormat != 0)
               {
                  m_map[i].nOffset = nOffset;
                  nOffset += 4;
               }
            }

            bRet = TRUE;
            setConnected();
         }
      }
   }
   return bRet;
}

/**
 * Validate connection to the device
 */
BOOL BCMXCPInterface::validateConnection()
{
   if (sendReadCommand(PW_ID_BLOCK_REQ))
   {
      if (recvData(PW_ID_BLOCK_REQ) > 0)
         return TRUE;
   }
   return FALSE;
}

/**
 * Read parameter from UPS
 */
void BCMXCPInterface::readParameter(int nIndex, int nFormat, UPS_PARAMETER *pParam)
{
   int nBytes;

   if ((nIndex >= BCMXCP_MAP_SIZE) || (m_map[nIndex].nFormat == 0))
   {
      pParam->dwFlags |= UPF_NOT_SUPPORTED;
   }
   else
   {
      if (sendReadCommand(PW_METER_BLOCK_REQ))
      {
         nBytes = recvData(PW_METER_BLOCK_REQ);
         if (nBytes > 0)
         {
            if (m_map[nIndex].nOffset < nBytes)
            {
               DecodeValue(&m_data[m_map[nIndex].nOffset],
                           m_map[nIndex].nFormat, nFormat, pParam->szValue);
               pParam->dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
            }
            else
            {
               pParam->dwFlags |= UPF_NOT_SUPPORTED;
            }
         }
         else
         {
            pParam->dwFlags |= UPF_NULL_VALUE;
         }
      }
      else
      {
         pParam->dwFlags |= UPF_NULL_VALUE;
      }
   }
}

/**
 * Get UPS model
 */
void BCMXCPInterface::queryModel()
{
   int nBytes, nPos;

   if (sendReadCommand(PW_ID_BLOCK_REQ))
   {
      nBytes = recvData(PW_ID_BLOCK_REQ);
      if (nBytes > 0)
      {
         nPos = m_data[0] * 2 + 1;
         nPos += (m_data[nPos] == 0) ? 5 : 3;
         if ((nPos < nBytes) && (nPos + m_data[nPos] <= nBytes))
         {
            memcpy(m_paramList[UPS_PARAM_MODEL].szValue, &m_data[nPos + 1], m_data[nPos]);
            m_paramList[UPS_PARAM_MODEL].szValue[m_data[nPos]] = 0;
            StrStripA(m_paramList[UPS_PARAM_MODEL].szValue);
            m_paramList[UPS_PARAM_MODEL].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
         }
         else
         {
            m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NOT_SUPPORTED;
         }
      }
      else
      {
         m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_MODEL].dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get battery level (in percents)
 */
void BCMXCPInterface::queryBatteryLevel()
{
   readParameter(MAP_BATTERY_LEVEL, FMT_INTEGER, &m_paramList[UPS_PARAM_BATTERY_LEVEL]);
}

/**
 * Get battery voltage
 */
void BCMXCPInterface::queryBatteryVoltage()
{
   readParameter(MAP_BATTERY_VOLTAGE, FMT_DOUBLE, &m_paramList[UPS_PARAM_BATTERY_VOLTAGE]);
}

/**
 * Get input line voltage
 */
void BCMXCPInterface::queryInputVoltage()
{
   readParameter(MAP_INPUT_VOLTAGE, FMT_DOUBLE, &m_paramList[UPS_PARAM_INPUT_VOLTAGE]);
}

/**
 * Get output voltage
 */
void BCMXCPInterface::queryOutputVoltage()
{
   readParameter(MAP_OUTPUT_VOLTAGE, FMT_DOUBLE, &m_paramList[UPS_PARAM_OUTPUT_VOLTAGE]);
}

/**
 * Get estimated runtime (in minutes)
 */
void BCMXCPInterface::queryEstimatedRuntime()
{
   readParameter(MAP_BATTERY_RUNTIME, FMT_INT_MINUTES, &m_paramList[UPS_PARAM_EST_RUNTIME]);
}

/**
 * Get temperature inside UPS
 */
void BCMXCPInterface::queryTemperature()
{
   readParameter(MAP_AMBIENT_TEMPERATURE, FMT_INTEGER, &m_paramList[UPS_PARAM_TEMP]);
}

/**
 * Get line frequency (Hz)
 */
void BCMXCPInterface::queryLineFrequency()
{
   readParameter(MAP_INPUT_FREQUENCY, FMT_INTEGER, &m_paramList[UPS_PARAM_LINE_FREQ]);
}

/**
 * Get firmware version
 */
void BCMXCPInterface::queryFirmwareVersion()
{
   int i, nLen, nBytes, nPos;

   if (sendReadCommand(PW_ID_BLOCK_REQ))
   {
      nBytes = recvData(PW_ID_BLOCK_REQ);
      if (nBytes > 0)
      {
         nLen = m_data[0];
         for(i = 0, nPos = 1; i < nLen; i++, nPos += 2)
         {
            if ((m_data[nPos + 1] != 0) || (m_data[nPos] != 0))
            {
               sprintf(m_paramList[UPS_PARAM_FIRMWARE].szValue, "%d.%02d", m_data[nPos + 1], m_data[nPos]);
               m_paramList[UPS_PARAM_FIRMWARE].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
               break;
            }
         }
         if (i == nLen)
         {
            m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NOT_SUPPORTED;
         }
      }
      else
      {
         m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NULL_VALUE;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_FIRMWARE].dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get unit serial number
 */
void BCMXCPInterface::querySerialNumber()
{
   int nBytes;

   if (sendReadCommand(PW_ID_BLOCK_REQ))
   {
      nBytes = recvData(PW_ID_BLOCK_REQ);
      if (nBytes >= 80)    // Serial number offset + length
      {
         memcpy(m_paramList[UPS_PARAM_SERIAL].szValue, &m_data[64], 16);
         if (m_paramList[UPS_PARAM_SERIAL].szValue[0] == 0)
         {
            strcpy(m_paramList[UPS_PARAM_SERIAL].szValue, "UNSET");
         }
         else
         {
            m_paramList[UPS_PARAM_SERIAL].szValue[16] = 0;
            StrStripA(m_paramList[UPS_PARAM_SERIAL].szValue);
         }
         m_paramList[UPS_PARAM_SERIAL].dwFlags &= ~(UPF_NOT_SUPPORTED | UPF_NULL_VALUE);
      }
      else
      {
         m_paramList[UPS_PARAM_SERIAL].dwFlags |= (nBytes == -1) ? UPF_NULL_VALUE : UPF_NOT_SUPPORTED;
      }
   }
   else
   {
      m_paramList[UPS_PARAM_SERIAL].dwFlags |= UPF_NULL_VALUE;
   }
}

/**
 * Get UPS online status
 */
void BCMXCPInterface::queryOnlineStatus()
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
void BCMXCPInterface::queryPowerLoad()
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
