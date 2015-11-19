/*
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Report to SNMP error mapping
 */
static struct
{
	const TCHAR *oid;
	UINT32 errorCode;
} s_oidToErrorMap[] =
{
	{ _T(".1.3.6.1.6.3.11.2.1.3"), SNMP_ERR_ENGINE_ID },
	{ _T(".1.3.6.1.6.3.11.2.1.3.0"), SNMP_ERR_ENGINE_ID },
	{ _T(".1.3.6.1.6.3.15.1.1.1.0"), SNMP_ERR_UNSUPP_SEC_LEVEL },
	{ _T(".1.3.6.1.6.3.15.1.1.2.0"), SNMP_ERR_TIME_WINDOW },
	{ _T(".1.3.6.1.6.3.15.1.1.3.0"), SNMP_ERR_SEC_NAME },
	{ _T(".1.3.6.1.6.3.15.1.1.4.0"), SNMP_ERR_ENGINE_ID },
	{ _T(".1.3.6.1.6.3.15.1.1.5.0"), SNMP_ERR_AUTH_FAILURE },
	{ _T(".1.3.6.1.6.3.15.1.1.6.0"), SNMP_ERR_DECRYPTION },
	{ NULL, 0 }
};

/**
 * Create new SNMP transport.
 */
SNMP_Transport::SNMP_Transport()
{
	m_authoritativeEngine = NULL;
	m_contextEngine = NULL;
	m_securityContext = NULL;
	m_enableEngineIdAutoupdate = false;
	m_updatePeerOnRecv = false;
	m_snmpVersion = SNMP_VERSION_2C;
}

/**
 * Destructor
 */
SNMP_Transport::~SNMP_Transport()
{
	delete m_authoritativeEngine;
	delete m_contextEngine;
	delete m_securityContext;
}

/**
 * Set security context. Previous security context will be destroyed.
 *
 * @param ctx new security context
 */
void SNMP_Transport::setSecurityContext(SNMP_SecurityContext *ctx)
{
	delete m_securityContext;
	m_securityContext = ctx;
   delete m_authoritativeEngine;
   m_authoritativeEngine = ((m_securityContext != NULL) && (m_securityContext->getAuthoritativeEngine().getIdLen() > 0)) ? new SNMP_Engine(&m_securityContext->getAuthoritativeEngine()) : NULL;
}

/**
 * Send a request and wait for respone with respect for timeouts and retransmissions
 */
UINT32 SNMP_Transport::doRequest(SNMP_PDU *request, SNMP_PDU **response, UINT32 timeout, int numRetries)
{
   UINT32 rc;
   int bytes;

   if ((request == NULL) || (response == NULL) || (numRetries <= 0))
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

		delete *response;
		*response = NULL;
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
                  if (((m_contextEngine == NULL) || (m_contextEngine->getIdLen() == 0)) && ((*response)->getContextEngineIdLength() != 0))
						{
                     delete m_contextEngine;
							m_contextEngine = new SNMP_Engine((*response)->getContextEngineId(), (*response)->getContextEngineIdLength());
						}

						if ((*response)->getCommand() == SNMP_REPORT)
						{
		               SNMP_Variable *var = (*response)->getVariable(0);
							const TCHAR *oid = var->getName()->getValueAsText();
							rc = SNMP_ERR_AGENT;
							for(int i = 0; s_oidToErrorMap[i].oid != NULL; i++)
							{
								if (!_tcscmp(oid, s_oidToErrorMap[i].oid))
								{
									rc = s_oidToErrorMap[i].errorCode;
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
								   // Use provided context engine ID if set in response
								   // Use authoritative engine ID if response has no context engine id
								   if ((*response)->getContextEngineIdLength() > 0)
								      request->setContextEngineId((*response)->getContextEngineId(), (*response)->getContextEngineIdLength());
								   else if ((*response)->getAuthoritativeEngine().getIdLen() != 0)
								      request->setContextEngineId((*response)->getAuthoritativeEngine().getId(), (*response)->getAuthoritativeEngine().getIdLen());
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

/**
 * SNMP_UDPTransport default constructor
 */
SNMP_UDPTransport::SNMP_UDPTransport() : SNMP_Transport()
{
   m_port = SNMP_DEFAULT_PORT;
   m_hSocket = -1;
   m_dwBufferSize = SNMP_DEFAULT_MSG_MAX_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
	m_connected = false;
}

/**
 * Create SNMP_UDPTransport for existing socket
 */
SNMP_UDPTransport::SNMP_UDPTransport(SOCKET hSocket) : SNMP_Transport()
{
   m_port = SNMP_DEFAULT_PORT;
   m_hSocket = hSocket;
   m_dwBufferSize = SNMP_DEFAULT_MSG_MAX_SIZE;
   m_dwBufferPos = 0;
   m_dwBytesInBuffer = 0;
   m_pBuffer = (BYTE *)malloc(m_dwBufferSize);
	m_connected = false;
}

/**
 * Create SNMP_UDPTransport transport connected to given host using host name
 */
UINT32 SNMP_UDPTransport::createUDPTransport(const TCHAR *hostName, UINT16 port)
{
   return createUDPTransport(InetAddress::resolveHostName(hostName), port);
}

/**
 * Create SNMP_UDPTransport transport connected to given host
 */
UINT32 SNMP_UDPTransport::createUDPTransport(const InetAddress& hostAddr, UINT16 port)
{
   if (!hostAddr.isValid())
      return SNMP_ERR_HOSTNAME;

   m_port = port;
   hostAddr.fillSockAddr(&m_peerAddr, port);

   UINT32 dwResult;

   // Create and connect socket
   m_hSocket = socket(hostAddr.getFamily(), SOCK_DGRAM, 0);
   if (m_hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer localAddr;

		memset(&localAddr, 0, sizeof(SockAddrBuffer));
      if (hostAddr.getFamily() == AF_INET)
      {
		   localAddr.sa4.sin_family = AF_INET;
		   localAddr.sa4.sin_addr.s_addr = htonl(INADDR_ANY);
      }
#ifdef WITH_IPV6
      else
      {
		   localAddr.sa6.sin6_family = AF_INET6;
      }
#endif

      // We use bind() and later sendto() instead of connect()
		// to handle cases with some strange devices responding to SNMP
		// requests from random port insted of 161 where request was sent
		// (currently only one such device known is AS/400)
      if (bind(m_hSocket, (struct sockaddr *)&localAddr, SA_LEN((struct sockaddr *)&localAddr)) == 0)
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
   return dwResult;
}

#undef HOSTNAME_VAR

/**
 * Destructor for UDP transport
 */
SNMP_UDPTransport::~SNMP_UDPTransport()
{
   safe_free(m_pBuffer);
   if (m_hSocket != -1)
      closesocket(m_hSocket);
}

/**
 * Clear buffer
 */
void SNMP_UDPTransport::clearBuffer()
{
   m_dwBytesInBuffer = 0;
   m_dwBufferPos = 0;
}

/**
 * Receive data from socket
 */
int SNMP_UDPTransport::recvData(UINT32 dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize)
{
   fd_set rdfs;
   struct timeval tv;
   SockAddrBuffer srcAddrBuffer;

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
            if (((iErr == -1) && (errno != EINTR)) || (iErr == 0))
            {
               return 0;
            }
         }
         qwTime = GetCurrentTimeMs() - qwTime;  // Elapsed time
         dwTimeout -= min(((UINT32)qwTime), dwTimeout);
      } while(iErr < 0);
#endif
   }

	struct sockaddr *senderAddr = (pSender != NULL) ? pSender : (struct sockaddr *)&srcAddrBuffer;
   socklen_t srcAddrLenBuffer = (piAddrSize != NULL) ? *piAddrSize : sizeof(srcAddrBuffer);
   int rc = recvfrom(m_hSocket, (char *)&m_pBuffer[m_dwBufferPos + m_dwBytesInBuffer],
                     (int)(m_dwBufferSize - (m_dwBufferPos + m_dwBytesInBuffer)), 0,
                     senderAddr, &srcAddrLenBuffer);

	// Validate sender's address if socket is connected
	if ((rc >= 0) && m_connected)
	{
		// Packet from wrong address, ignore it
      if (!SocketAddressEquals(senderAddr, (struct sockaddr *)&m_peerAddr))
		{
			goto retry_wait;
		}
	}

	if (piAddrSize != NULL)
   {
      *piAddrSize = srcAddrLenBuffer;
   }

	// Update peer address
	if ((rc >= 0) && m_updatePeerOnRecv)
	{
		memcpy(&m_peerAddr, senderAddr, SA_LEN(senderAddr));
	}

	return rc;
}

/**
 * Pre-parse PDU
 */
size_t SNMP_UDPTransport::preParsePDU()
{
   UINT32 dwType;
   size_t dwLength, dwIdLength;
   BYTE *pbCurrPos;

   if (!BER_DecodeIdentifier(&m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer,
                             &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return 0;
   if (dwType != ASN_SEQUENCE)
      return 0;   // Packet should start with SEQUENCE

   return dwLength + dwIdLength;
}

/**
 * Read PDU from socket
 */
int SNMP_UDPTransport::readMessage(SNMP_PDU **ppData, UINT32 dwTimeout,
                                   struct sockaddr *pSender, socklen_t *piAddrSize,
                                   SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
   int bytes;
   size_t pduLength;

   if (m_dwBytesInBuffer < 2)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   pduLength = preParsePDU();
   if (pduLength == 0)
   {
      // Clear buffer
      clearBuffer();
      return 0;
   }

   // Move existing data to the beginning of buffer if there are not enough space at the end
   if (pduLength > m_dwBufferSize - m_dwBufferPos)
   {
      memmove(m_pBuffer, &m_pBuffer[m_dwBufferPos], m_dwBytesInBuffer);
      m_dwBufferPos = 0;
   }

   // Read entire PDU into buffer
   while(m_dwBytesInBuffer < pduLength)
   {
      bytes = recvData(dwTimeout, pSender, piAddrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

	// Change security context if needed
	if (contextFinder != NULL)
	{
		setSecurityContext(contextFinder(pSender, *piAddrSize));
	}

   // Create new PDU object and remove parsed data from buffer
   *ppData = new SNMP_PDU;
   if (!(*ppData)->parse(&m_pBuffer[m_dwBufferPos], pduLength, m_securityContext, m_enableEngineIdAutoupdate))
   {
      delete *ppData;
      *ppData = NULL;
   }
   m_dwBytesInBuffer -= pduLength;
   if (m_dwBytesInBuffer == 0)
      m_dwBufferPos = 0;

   return (int)pduLength;
}

/**
 * Send PDU to socket
 */
int SNMP_UDPTransport::sendMessage(SNMP_PDU *pPDU)
{
   BYTE *pBuffer;
   int nBytes = 0;

   size_t size = pPDU->encode(&pBuffer, m_securityContext);
   if (size != 0)
   {
      nBytes = sendto(m_hSocket, (char *)pBuffer, (int)size, 0, (struct sockaddr *)&m_peerAddr, SA_LEN((struct sockaddr *)&m_peerAddr));
      free(pBuffer);
   }

   return nBytes;
}

/**
 * Get peer IPv4 address (in host byte order)
 */
InetAddress SNMP_UDPTransport::getPeerIpAddress()
{
   return InetAddress::createFromSockaddr((struct sockaddr *)&m_peerAddr);
}
