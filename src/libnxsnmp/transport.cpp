/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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

int SNMP_Transport::RecvData(DWORD dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize)
{
   if (dwTimeout != INFINITE)
   {
      fd_set rdfs;
      struct timeval tv;
#ifndef _WIN32
      int iErr;
      QWORD qwTime;

      do
      {
#endif
         FD_ZERO(&rdfs);
         FD_SET(m_hSocket, &rdfs);
         tv.tv_sec = dwTimeout / 1000;
         tv.tv_usec = (dwTimeout % 1000) * 1000;
#ifdef _WIN32
         if (select(m_hSocket + 1, &rdfs, NULL, NULL, &tv) <= 0)
            return 0;
#else
         qwTime = GetCurrentTimeMs();
         if ((iErr = select(m_hSocket + 1, &rdfs, NULL, NULL, &tv)) <= 0)
         {
            if (((iErr == -1) && (errno != EINTR)) ||
                (iErr == 0))
            {
               return 0;
            }
         }
         qwTime = GetCurrentTimeMs() - qwTime;  // Elapsed time
         dwTimeout -= min(((DWORD)qwTime), dwTimeout);
      } while(iErr < 0);
#endif
   }
   return recvfrom(m_hSocket, (char *)&m_pBuffer[m_dwBufferPos + m_dwBytesInBuffer],
                   m_dwBufferSize - (m_dwBufferPos + m_dwBytesInBuffer), 0,
                   pSender, piAddrSize);
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

int SNMP_Transport::Read(SNMP_PDU **ppData, DWORD dwTimeout, 
                         struct sockaddr *pSender, socklen_t *piAddrSize)
{
   int iBytes;
   DWORD dwPDULength;

   if (m_dwBytesInBuffer < 2)
   {
      iBytes = RecvData(dwTimeout, pSender, piAddrSize);
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
      iBytes = RecvData(dwTimeout, pSender, piAddrSize);
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

int SNMP_Transport::Send(SNMP_PDU *pPDU, struct sockaddr *pRcpt, socklen_t iAddrSize)
{
   BYTE *pBuffer;
   DWORD dwSize;
   int nBytes = 0;

   dwSize = pPDU->Encode(&pBuffer);
   if (dwSize != 0)
   {
      if (pRcpt == NULL)
         nBytes = send(m_hSocket, (char *)pBuffer, dwSize, 0);
      else
         nBytes = sendto(m_hSocket, (char *)pBuffer, dwSize, 0, pRcpt, iAddrSize);
      free(pBuffer);
   }

   return nBytes;
}


//
// Create UDP transport connected to given host
// Will try to resolve host name if it's not null, otherwise
// IP address will be used
//

DWORD SNMP_Transport::CreateUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr, WORD wPort)
{
   struct sockaddr_in addr;
   DWORD dwResult;

   // Fill in remote address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(wPort);

   // Resolve hostname
   if (pszHostName != NULL)
   {
      struct hostent *hs;

      hs = gethostbyname(pszHostName);
      if (hs != NULL)
      {
         memcpy(&addr.sin_addr.s_addr, hs->h_addr, sizeof(DWORD));
      }
      else
      {
         addr.sin_addr.s_addr = inet_addr(pszHostName);
      }
   }
   else
   {
      addr.sin_addr.s_addr = dwHostAddr;
   }

   // Create and connect socket
   if ((addr.sin_addr.s_addr != INADDR_ANY) && 
       (addr.sin_addr.s_addr != INADDR_NONE))
   {
      m_hSocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (m_hSocket != -1)
      {
         // Pseudo-connect socket
         if (connect(m_hSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == 0)
         {
            dwResult = SNMP_ERR_SUCCESS;
         }
         else
         {
            closesocket(m_hSocket);
            m_hSocket = -1;
            dwResult = SNMP_ERR_SOCKET;
         }
      }
      else
      {
         dwResult = SNMP_ERR_SOCKET;
      }
   }
   else
   {
      dwResult = SNMP_ERR_HOSTNAME;
   }

   return dwResult;
}


//
// Send a request and wait for respone
// with respect for timeouts and retransmissions
//

DWORD SNMP_Transport::DoRequest(SNMP_PDU *pRequest, SNMP_PDU **ppResponce, 
                                DWORD dwTimeout, DWORD dwNumRetries)
{
   DWORD dwResult = SNMP_ERR_SUCCESS;
   int iBytes;

   if ((pRequest == NULL) || (ppResponce == NULL) || (dwNumRetries == 0))
      return SNMP_ERR_PARAM;

   *ppResponce = NULL;
   if (Send(pRequest) <= 0)
      return SNMP_ERR_COMM;

   while(dwNumRetries-- > 0)
   {
      iBytes = Read(ppResponce, dwTimeout);
      if (iBytes > 0)
      {
         if (*ppResponce != NULL)
         {
            if ((*ppResponce)->GetRequestId() == pRequest->GetRequestId())
               break;
            dwResult = SNMP_ERR_TIMEOUT;
         }
         else
         {
            dwResult = SNMP_ERR_PARSE;
         }
      }
      else
      {
         dwResult = (iBytes == 0) ? SNMP_ERR_TIMEOUT : SNMP_ERR_COMM;
      }
   }

   return dwResult;
}
