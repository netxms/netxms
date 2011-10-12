/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: transport.cpp
**
**/

#include "libnxsnmp.h"


//
// Report to SNMP error mapping
//

static struct
{
	const TCHAR *oid;
	DWORD errorCode;
} m_oidToErrorMap[] =
{
	{ _T(".1.3.6.1.6.3.15.1.1.1.0"), SNMP_ERR_UNSUPP_SEC_LEVEL },
	{ _T(".1.3.6.1.6.3.15.1.1.2.0"), SNMP_ERR_TIME_WINDOW },
	{ _T(".1.3.6.1.6.3.15.1.1.3.0"), SNMP_ERR_SEC_NAME },
	{ _T(".1.3.6.1.6.3.15.1.1.4.0"), SNMP_ERR_ENGINE_ID },
	{ _T(".1.3.6.1.6.3.15.1.1.5.0"), SNMP_ERR_AUTH_FAILURE },
	{ _T(".1.3.6.1.6.3.15.1.1.6.0"), SNMP_ERR_DECRYPTION },
	{ NULL, 0 }
};


//
// Constructor
//

SNMP_Transport::SNMP_Transport()
{
	m_authoritativeEngine = NULL;
	m_contextEngine = NULL;
	m_securityContext = NULL;
	m_enableEngineIdAutoupdate = false;
	m_snmpVersion = SNMP_VERSION_2C;
}


//
// Destructor
//

SNMP_Transport::~SNMP_Transport()
{
	delete m_authoritativeEngine;
	delete m_contextEngine;
	delete m_securityContext;
}


//
// Set security context
//

void SNMP_Transport::setSecurityContext(SNMP_SecurityContext *ctx)
{
	delete m_securityContext;
	m_securityContext = ctx;
}


//
// Send a request and wait for respone
// with respect for timeouts and retransmissions
//

DWORD SNMP_Transport::doRequest(SNMP_PDU *request, SNMP_PDU **response, 
                                DWORD timeout, int numRetries)
{
   DWORD rc;
   int bytes;

   if ((request == NULL) || (response == NULL) || (numRetries == 0))
      return SNMP_ERR_PARAM;

   *response = NULL;

	// Create dummy context
	if (m_securityContext == NULL)
		m_securityContext = new SNMP_SecurityContext();

	// Update SNMP V3 request with cached context engine id
	if (request->getVersion() == SNMP_VERSION_3)
	{
		if ((request->getContextEngineIdLength() == 0) && (m_contextEngine != NULL))
		{
			request->setContextEngineId(m_contextEngine->getId(), m_contextEngine->getIdLen());
		}
	}

   while(numRetries-- >= 0)
   {
		int timeSyncRetries = 3;

retry:
		rc = SNMP_ERR_SUCCESS;
      if (sendMessage(request) <= 0)
      {
         rc = SNMP_ERR_COMM;
         break;
      }

      bytes = readMessage(response, timeout);
      if (bytes > 0)
      {
         if (*response != NULL)
         {
				if (request->getVersion() == SNMP_VERSION_3)
				{
					if ((*response)->getMessageId() == request->getMessageId())
					{
						// Cache authoritative engine ID
						if ((m_authoritativeEngine == NULL) && ((*response)->getAuthoritativeEngine().getIdLen() != 0))
						{
							m_authoritativeEngine = new SNMP_Engine((*response)->getAuthoritativeEngine());
							m_securityContext->setAuthoritativeEngine(*m_authoritativeEngine);
						}

						// Cache context engine ID
						if ((m_contextEngine == NULL) && ((*response)->getContextEngineIdLength() != 0))
						{
							m_contextEngine = new SNMP_Engine((*response)->getContextEngineId(), (*response)->getContextEngineIdLength());
						}

						if ((*response)->getCommand() == SNMP_REPORT)
						{
		               SNMP_Variable *var = (*response)->getVariable(0);
							const TCHAR *oid = var->GetName()->GetValueAsText();
							rc = SNMP_ERR_AGENT;
							for(int i = 0; m_oidToErrorMap[i].oid != NULL; i++)
							{
								if (!_tcscmp(oid, m_oidToErrorMap[i].oid))
								{
									rc = m_oidToErrorMap[i].errorCode;
									break;
								}
							}

							// Engine ID discovery - if request contains empty engine ID,
							// replace it with correct one and retry
							if (rc == SNMP_ERR_ENGINE_ID)
							{
								bool canRetry = false;

								if (request->getContextEngineIdLength() == 0)
								{
									request->setContextEngineId((*response)->getContextEngineId(), (*response)->getContextEngineIdLength());
									canRetry = true;
								}
								if (m_securityContext->getAuthoritativeEngine().getIdLen() == 0)
								{
									m_securityContext->setAuthoritativeEngine((*response)->getAuthoritativeEngine());
									canRetry = true;
								}
								if (canRetry)
									goto retry;
							}
							else if (rc == SNMP_ERR_TIME_WINDOW)
							{
								// Update cached authoritative engine with new boots and time
								if ((timeSyncRetries > 0) &&
								    (((*response)->getAuthoritativeEngine().getBoots() != m_authoritativeEngine->getBoots()) ||
								     ((*response)->getAuthoritativeEngine().getTime() != m_authoritativeEngine->getTime())))
								{
									m_authoritativeEngine->setBoots((*response)->getAuthoritativeEngine().getBoots());
									m_authoritativeEngine->setTime((*response)->getAuthoritativeEngine().getTime());
									m_securityContext->setAuthoritativeEngine(*m_authoritativeEngine);
									timeSyncRetries--;
									goto retry;
								}
							}
						}
						else if ((*response)->getCommand() != SNMP_RESPONSE)
						{
							rc = SNMP_ERR_BAD_RESPONSE;
						}
						break;
					}
				}
				else
				{
					if ((*response)->getRequestId() == request->getRequestId())
						break;
				}
            rc = SNMP_ERR_TIMEOUT;
         }
         else
         {
            rc = SNMP_ERR_PARSE;
            break;
         }
      }
      else
      {
         rc = (bytes == 0) ? SNMP_ERR_TIMEOUT : SNMP_ERR_COMM;
      }
   }

	if (rc != SNMP_ERR_SUCCESS)
		delete_and_null(*response);
   return rc;
}


//
// SNMP_UDPTransport default constructor
//

SNMP_UDPTransport::SNMP_UDPTransport()
                  :SNMP_Transport()
{
   m_hSocket = -1;
   m_dwBufferSize = SNMP_DEFAULT_MSG_MAX_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
	m_connected = false;
}


//
// Create SNMP_UDPTransport for existing socket
//

SNMP_UDPTransport::SNMP_UDPTransport(SOCKET hSocket)
                  :SNMP_Transport()
{
   m_hSocket = hSocket;
   m_dwBufferSize = SNMP_DEFAULT_MSG_MAX_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
	m_connected = false;
}


//
// Create SNMP_UDPTransport transport connected to given host
// Will try to resolve host name if it's not null, otherwise
// IP address will be used
//

DWORD SNMP_UDPTransport::createUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr, WORD wPort)
{
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
   memset(&m_peerAddr, 0, sizeof(struct sockaddr_in));
   m_peerAddr.sin_family = AF_INET;
   m_peerAddr.sin_port = htons(wPort);

   // Resolve hostname
   if (pszHostName != NULL)
   {
      struct hostent *hs;

      hs = gethostbyname(HOSTNAME_VAR);
      if (hs != NULL)
      {
         memcpy(&m_peerAddr.sin_addr.s_addr, hs->h_addr, sizeof(DWORD));
      }
      else
      {
         m_peerAddr.sin_addr.s_addr = inet_addr(HOSTNAME_VAR);
      }
   }
   else
   {
      m_peerAddr.sin_addr.s_addr = dwHostAddr;
   }

   // Create and connect socket
   if ((m_peerAddr.sin_addr.s_addr != INADDR_ANY) && 
       (m_peerAddr.sin_addr.s_addr != INADDR_NONE))
   {
      m_hSocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (m_hSocket != -1)
      {
			struct sockaddr_in localAddr;

			memset(&localAddr, 0, sizeof(struct sockaddr_in));
			localAddr.sin_family = AF_INET;
			localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

         // We use bind() and later sendto() instead of connect()
			// to handle cases with some strange devices responding to SNMP
			// requests from random port insted of 161 where request was sent
			// (currently only one such device known is AS/400)
         if (bind(m_hSocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) == 0)
         {
				// Set non-blocking mode
#ifdef _WIN32
				u_long one = 1;
				ioctlsocket(m_hSocket, FIONBIO, &one);
#endif
				m_connected = true;
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

void SNMP_UDPTransport::clearBuffer()
{
   m_dwBytesInBuffer = 0;
   m_dwBufferPos = 0;
}


//
// Receive data from socket
//

int SNMP_UDPTransport::recvData(DWORD dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize)
{
   fd_set rdfs;
   struct timeval tv;
	struct sockaddr_in srcAddrBuffer;
	socklen_t srcAddrLenBuffer = sizeof(struct sockaddr_in);

retry_wait:
   if (dwTimeout != INFINITE)
   {
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
         if (select(1, &rdfs, NULL, NULL, &tv) <= 0)
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

	struct sockaddr *senderAddr = (pSender != NULL) ? pSender : (struct sockaddr *)&srcAddrBuffer;
	socklen_t *senderAddrLen = (piAddrSize != NULL) ? piAddrSize : &srcAddrLenBuffer;
   int rc = recvfrom(m_hSocket, (char *)&m_pBuffer[m_dwBufferPos + m_dwBytesInBuffer],
                     m_dwBufferSize - (m_dwBufferPos + m_dwBytesInBuffer), 0,
                     senderAddr, senderAddrLen);

	// Validate sender's address if socket is connected
	if ((rc >= 0) && m_connected)
	{
		// Packet from wrong address, ignore it
		if (((struct sockaddr_in *)senderAddr)->sin_addr.s_addr != m_peerAddr.sin_addr.s_addr)
		{
			goto retry_wait;
		}
	}
	return rc;
}


//
// Pre-parse PDU
//

DWORD SNMP_UDPTransport::preParsePDU()
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

int SNMP_UDPTransport::readMessage(SNMP_PDU **ppData, DWORD dwTimeout, 
                                   struct sockaddr *pSender, socklen_t *piAddrSize,
                                   SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
   int iBytes;
   DWORD dwPDULength;

   if (m_dwBytesInBuffer < 2)
   {
      iBytes = recvData(dwTimeout, pSender, piAddrSize);
      if (iBytes <= 0)
      {
         clearBuffer();
         return iBytes;
      }
      m_dwBytesInBuffer += iBytes;
   }

   dwPDULength = preParsePDU();
   if (dwPDULength == 0)
   {
      // Clear buffer
      clearBuffer();
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
      iBytes = recvData(dwTimeout, pSender, piAddrSize);
      if (iBytes <= 0)
      {
         clearBuffer();
         return iBytes;
      }
      m_dwBytesInBuffer += iBytes;
   }

	// Change security context if needed
	if (contextFinder != NULL)
	{
		setSecurityContext(contextFinder(pSender, *piAddrSize));
	}

   // Create new PDU object and remove parsed data from buffer
   *ppData = new SNMP_PDU;
   if (!(*ppData)->parse(&m_pBuffer[m_dwBufferPos], dwPDULength, m_securityContext, m_enableEngineIdAutoupdate))
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

int SNMP_UDPTransport::sendMessage(SNMP_PDU *pPDU)
{
   BYTE *pBuffer;
   DWORD dwSize;
   int nBytes = 0;

   dwSize = pPDU->encode(&pBuffer, m_securityContext);
   if (dwSize != 0)
   {
      nBytes = sendto(m_hSocket, (char *)pBuffer, dwSize, 0, (struct sockaddr *)&m_peerAddr, sizeof(struct sockaddr_in));
      free(pBuffer);
   }

   return nBytes;
}
