/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: transport.cpp
**
**/

#include "libnxsnmp.h"


//
// Constants
//

#define DEFAULT_BUFFER_SIZE      32768


//
// Send a request and wait for respone
// with respect for timeouts and retransmissions
//

DWORD SNMP_Transport::DoRequest(SNMP_PDU *pRequest, SNMP_PDU **ppResponse, 
                                DWORD dwTimeout, DWORD dwNumRetries)
{
   DWORD dwResult = SNMP_ERR_SUCCESS;
   int iBytes;

   if ((pRequest == NULL) || (ppResponse == NULL) || (dwNumRetries == 0))
      return SNMP_ERR_PARAM;

   *ppResponse = NULL;

   while(dwNumRetries-- > 0)
   {
      if (Send(pRequest) <= 0)
      {
         dwResult = SNMP_ERR_COMM;
         break;
      }

      iBytes = Read(ppResponse, dwTimeout);
      if (iBytes > 0)
      {
         if (*ppResponse != NULL)
         {
            if ((*ppResponse)->GetRequestId() == pRequest->GetRequestId())
               break;
            dwResult = SNMP_ERR_TIMEOUT;
         }
         else
         {
            dwResult = SNMP_ERR_PARSE;
            break;
         }
      }
      else
      {
         dwResult = (iBytes == 0) ? SNMP_ERR_TIMEOUT : SNMP_ERR_COMM;
      }
   }

   return dwResult;
}


//
// SNMP_UDPTransport default constructor
//

SNMP_UDPTransport::SNMP_UDPTransport()
                  :SNMP_Transport()
{
   m_hSocket = -1;
   m_dwBufferSize = DEFAULT_BUFFER_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
}


//
// Create SNMP_UDPTransport for existing socket
//

SNMP_UDPTransport::SNMP_UDPTransport(SOCKET hSocket)
                  :SNMP_Transport()
{
   m_hSocket = hSocket;
   m_dwBufferSize = DEFAULT_BUFFER_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
}


//
// Create SNMP_UDPTransport transport connected to given host
// Will try to resolve host name if it's not null, otherwise
// IP address will be used
//

DWORD SNMP_UDPTransport::CreateUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr, WORD wPort)
{
   struct sockaddr_in addr;
   DWORD dwResult;

#ifdef UNICODE
   char szHostName[256];

	if (pszHostName != NULL)
	{
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
								  pszHostName, -1, szHostName, 256, NULL, NULL);
	}
#define HOSTNAME_VAR szHostName
#else
#define HOSTNAME_VAR pszHostName
#endif

   // Fill in remote address structure
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(wPort);

   // Resolve hostname
   if (pszHostName != NULL)
   {
      struct hostent *hs;

      hs = gethostbyname(HOSTNAME_VAR);
      if (hs != NULL)
      {
         memcpy(&addr.sin_addr.s_addr, hs->h_addr, sizeof(DWORD));
      }
      else
      {
         addr.sin_addr.s_addr = inet_addr(HOSTNAME_VAR);
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

#undef HOSTNAME_VAR


//
// Destructor for SNMP_Transport
//

SNMP_UDPTransport::~SNMP_UDPTransport()
{
   safe_free(m_pBuffer);
   if (m_hSocket != -1)
      closesocket(m_hSocket);
}


//
// Clear buffer
//

void SNMP_UDPTransport::ClearBuffer(void)
{
   m_dwBytesInBuffer = 0;
   m_dwBufferPos = 0;
}


//
// Receive data from socket
//

int SNMP_UDPTransport::RecvData(DWORD dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize)
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

DWORD SNMP_UDPTransport::PreParsePDU(void)
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

int SNMP_UDPTransport::Read(SNMP_PDU **ppData, DWORD dwTimeout, 
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

int SNMP_UDPTransport::Send(SNMP_PDU *pPDU)
{
   BYTE *pBuffer;
   DWORD dwSize;
   int nBytes = 0;

   dwSize = pPDU->Encode(&pBuffer);
   if (dwSize != 0)
   {
      nBytes = send(m_hSocket, (char *)pBuffer, dwSize, 0);
      free(pBuffer);
   }

   return nBytes;
}
