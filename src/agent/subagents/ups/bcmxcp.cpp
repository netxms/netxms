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
** File: bcmxcp.cpp
**
**/

#include "ups.h"


//
// Commands
//

#define PW_COMMAND_START_BYTE    ((BYTE)0xAB)

#define PW_ID_BLOCK_REQ 	      ((BYTE)0x31)


//
// Calculate checksum for prepared message and add it to the end of buffer
//

static void CalculateChecksum(BYTE *pBuffer)
{
	BYTE sum;
	int i, nLen;

   nLen = pBuffer[1] + 2;
	for(sum = 0, i = 0; i < nLen; i++)
		sum -= pBuffer[i];
   pBuffer[nLen] = sum;
}


//
// Validate checksum of received packet
//

static BOOL ValidateChecksum(BYTE *pBuffer)
{
   BYTE sum;
   int i, nLen;

   nLen = pBuffer[2] + 5;
	for(i = 0, sum = 0; i < nLen; i++)
		sum += pBuffer[i];
   return sum == 0;
}


//
// Send read command to device
//

BOOL BCMXCPInterface::SendReadCommand(BYTE nCommand)
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
      bRet = m_serial.Write((char *)packet, 4);
      nRetries--;
   } while((!bRet) && (nRetries > 0));
   return bRet;
}


//
// Receive data block from device, and check hat it is for right command
//

int BCMXCPInterface::RecvData(int nCommand)
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
			if (m_serial.Read((char *)packet, 1) != 1)
            return -1;  // Read error
			nCount++;
		} while((packet[0] != PW_COMMAND_START_BYTE) && (nCount < 128));
		
		if (nCount == 128)
			return -1;  // Didn't get command start byte

		// Read block number byte
      if (m_serial.Read((char *)&packet[1], 1) != 1)
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
      if (m_serial.Read((char *)&packet[2], 1) != 1)
         return -1;  // Read error
		nLength = packet[2];
		if (nLength < 1) 
			return -1;  // Invalid data length

		// Read sequence byte
      if (m_serial.Read((char *)&packet[3], 1) != 1)
         return -1;  // Read error
		if ((packet[3] & 0x80) == 0x80)
         bEndBlock = TRUE;
		if ((packet[3] & 0x07) != (nPrevSequence + 1))
			return -1;  // Invalid sequence number
      nPrevSequence = packet[3];

		// Read data
      for(nRead = 0; nRead < nLength; nRead += nBytes)
      {
         nBytes = m_serial.Read((char *)&packet[nRead + 4], nLength - nRead);
         if (nBytes <= 0)
            return -1;  // Read error
      }

		// Read the checksum byte
      if (m_serial.Read((char *)&packet[nLength + 4], 1) != 1)
         return -1;  // Read error

		// Validate packet's checksum
		if (!ValidateChecksum(packet))
   		return -1;  // Bad checksum

		memcpy(&m_data[nTotalBytes], &packet[4], nLength);
		nTotalBytes += nLength;
	}
	return nTotalBytes;
}


//
// Open device
//

BOOL BCMXCPInterface::Open(void)
{
   BOOL bRet = FALSE;
   TCHAR szModel[256];

   if (SerialInterface::Open())
   {
      m_serial.SetTimeout(1000);
      m_serial.Set(19200, 8, NOPARITY, ONESTOPBIT);

      // Send two escapes
      m_serial.Write("\x1D\x1D", 2);

      if (GetModel(szModel) == SYSINFO_RC_SUCCESS)
      {
         bRet = TRUE;
         SetConnected();
         SetName(szModel);
      }
   }
   return bRet;
}


//
// Get UPS model
//

LONG BCMXCPInterface::GetModel(TCHAR *pszBuffer)
{
   LONG nRet = SYSINFO_RC_ERROR;
   int nBytes, nPos;

   if (SendReadCommand(PW_ID_BLOCK_REQ))
   {
      nBytes = RecvData(PW_ID_BLOCK_REQ);
      if (nBytes > 0)
      {
         nPos = m_data[0] * 2 + 1;
         nPos += (m_data[nPos] == 0) ? 5 : 3;
         if ((nPos < nBytes) && (nPos + m_data[nPos] <= nBytes))
         {
            memcpy(pszBuffer, &m_data[nPos + 1], m_data[nPos]);
            pszBuffer[m_data[nPos]] = 0;
            StrStrip(pszBuffer);
            nRet = SYSINFO_RC_SUCCESS;
         }
         else
         {
            nRet = SYSINFO_RC_UNSUPPORTED;
         }
      }
   }
   return nRet;
}
