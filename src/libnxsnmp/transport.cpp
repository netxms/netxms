/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: transport.cpp
**
**/

#include "libnxsnmp.h"


//
// Constants
//

#define DEFAULT_BUFFER_SIZE      32768


//
// SNMP_Transport default constructor
//

SNMP_Transport::SNMP_Transport()
{
   m_hSocket = -1;
   m_dwBufferSize = DEFAULT_BUFFER_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
}


//
// Create SNMP_Transport for existing socket
//

SNMP_Transport::SNMP_Transport(SOCKET hSocket)
{
   m_hSocket = hSocket;
   m_dwBufferSize = DEFAULT_BUFFER_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
}


//
// Destructor for SNMP_Transport
//

SNMP_Transport::~SNMP_Transport()
{
   safe_free(m_pBuffer);
   if (m_hSocket != -1)
      closesocket(m_hSocket);
}


//
// Clear buffer
//

void SNMP_Transport::ClearBuffer(void)
{
   m_dwBytesInBuffer = 0;
   m_dwBufferPos = 0;
}


//
// Receive data from socket
//

int SNMP_Transport::RecvData(void)
{
   return recv(m_hSocket, (char *)&m_pBuffer[m_dwBufferPos + m_dwBytesInBuffer],
               m_dwBufferSize - (m_dwBufferPos + m_dwBytesInBuffer), 0);
}


//
// Pre-parse PDU
//

DWORD SNMP_Transport::PreParsePDU(void)
{
   DWORD dwType, dwLength, dwIdLength;
   BYTE *pbCurrPos;

   if (!BER_DecodeIdentifier(&m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer, 
                             &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return 0;
   if (dwType != ASN_SEQUENCE)
      return 0;   // Packet should start with SEQUENCE

   return dwLength + dwIdLength;
}


//
// Read PDU from socket
//

int SNMP_Transport::Read(SNMP_PDU **ppData)
{
   int iBytes;
   DWORD dwPDULength;

   if (m_dwBytesInBuffer < 2)
   {
      iBytes = RecvData();
      if (iBytes <= 0)
      {
         ClearBuffer();
         return iBytes;
      }
      m_dwBytesInBuffer += iBytes;
   }

   dwPDULength = PreParsePDU();
   if (dwPDULength == 0)
   {
      // Clear buffer
      ClearBuffer();
      return 0;
   }

   // Move existing data to the beginning of buffer if there are not enough space at the end
   if (dwPDULength > m_dwBufferSize - m_dwBufferPos)
   {
      memmove(m_pBuffer, &m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer);
      m_dwBufferPos = 0;
   }

   // Read entire PDU into buffer
   while(m_dwBytesInBuffer < dwPDULength)
   {
      iBytes = RecvData();
      if (iBytes <= 0)
      {
         ClearBuffer();
         return iBytes;
      }
      m_dwBytesInBuffer += iBytes;
   }

   // Create new PDU object and remove parsed data from buffer
   *ppData = new SNMP_PDU;
   if (!(*ppData)->Parse(&m_pBuffer[m_dwBufferPos], dwPDULength))
   {
      delete *ppData;
      *ppData = NULL;
   }
   m_dwBytesInBuffer -= dwPDULength;
   if (m_dwBytesInBuffer == 0)
      m_dwBufferPos = 0;

   return dwPDULength;
}


//
// Send PDU to socket
//

int SNMP_Transport::Send(SNMP_PDU *pPDU)
{
   return 0;
}
