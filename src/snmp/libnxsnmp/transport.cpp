/*
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
	const uint32_t oid[12];
	size_t oidLen;
	uint32_t errorCode;
} s_oidToErrorMap[] =
{
	{ { 1, 3, 6, 1, 6, 3, 11, 2, 1, 3 }, 10, SNMP_ERR_ENGINE_ID },
	{ { 1, 3, 6, 1, 6, 3, 11, 2, 1, 3, 0 }, 11, SNMP_ERR_ENGINE_ID },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 1, 0 }, 11, SNMP_ERR_UNSUPP_SEC_LEVEL },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 2, 0 }, 11, SNMP_ERR_TIME_WINDOW },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 3, 0 }, 11, SNMP_ERR_SEC_NAME },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 4, 0 }, 11, SNMP_ERR_ENGINE_ID },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 5, 0 }, 11, SNMP_ERR_AUTH_FAILURE },
	{ { 1, 3, 6, 1, 6, 3, 15, 1, 1, 6, 0 }, 11, SNMP_ERR_DECRYPTION },
	{ { 0 }, 0, 0 }
};

/**
 * Default retry count
 */
static int s_defaultRetryCount = 3;

/**
 * Set default retry count
 */
void LIBNXSNMP_EXPORTABLE SnmpSetDefaultRetryCount(int numRetries)
{
   s_defaultRetryCount = (numRetries >= 1) ? numRetries : 1;
   nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 4, _T("SNMP default number of retries set to %d"), s_defaultRetryCount);
}

/**
 * Get default retry count
 */
int LIBNXSNMP_EXPORTABLE SnmpGetDefaultRetryCount()
{
   return s_defaultRetryCount;
}

/**
 * Create new SNMP transport.
 */
SNMP_Transport::SNMP_Transport()
{
	m_authoritativeEngine = nullptr;
	m_contextEngine = nullptr;
	m_securityContext = nullptr;
	m_enableEngineIdAutoupdate = false;
	m_updatePeerOnRecv = false;
	m_reliable = false;
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
   m_authoritativeEngine = ((ctx != nullptr) && (ctx->getAuthoritativeEngine().getIdLen() > 0)) ? new SNMP_Engine(ctx->getAuthoritativeEngine()) : nullptr;
   delete_and_null(m_contextEngine);
}

/**
 * Perform engine ID discovery.
 * Authoritative engine ID should be cached in transport object after successful operation and
 * context engine ID updated in original request PDU.
 */
uint32_t SNMP_Transport::doEngineIdDiscovery(SNMP_PDU *originalRequest, uint32_t timeout, int numRetries)
{
   SNMP_PDU discoveryRequest(SNMP_GET_REQUEST, originalRequest->getRequestId(), SNMP_VERSION_3);
   discoveryRequest.bindVariable(new SNMP_Variable(_T(".1.3.6.1.6.3.10.2.1.1.0")));    // snmpEngineID
   SNMP_PDU *response = nullptr;
   uint32_t rc = doRequest(&discoveryRequest, &response, timeout, numRetries, true);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   if (response->getContextEngineIdLength() > 0)
      originalRequest->setContextEngineId(response->getContextEngineId(), response->getContextEngineIdLength());
   else if (response->getAuthoritativeEngine().getIdLen() != 0)
      originalRequest->setContextEngineId(response->getAuthoritativeEngine().getId(), response->getAuthoritativeEngine().getIdLen());

   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Send a request and wait for response with respect for timeouts and retransmissions
 */
uint32_t SNMP_Transport::doRequest(SNMP_PDU *request, SNMP_PDU **response)
{
   return doRequest(request, response, SnmpGetDefaultTimeout(), s_defaultRetryCount, false);
}

/**
 * Send a request and wait for response with respect for timeouts and retransmissions
 */
uint32_t SNMP_Transport::doRequest(SNMP_PDU *request, SNMP_PDU **response, uint32_t timeout, int numRetries, bool engineIdDiscoveryOnly)
{
   if ((request == nullptr) || (response == nullptr) || (numRetries <= 0))
      return SNMP_ERR_PARAM;

   *response = nullptr;

	// Create dummy context
	if (m_securityContext == nullptr)
		m_securityContext = new SNMP_SecurityContext();

	// Update SNMP V3 request with cached context engine id
	if (request->getVersion() == SNMP_VERSION_3)
	{
	   if ((m_authoritativeEngine == nullptr) && (request->getCommand() != SNMP_GET_REQUEST))
	   {
	      uint32_t rc = doEngineIdDiscovery(request, timeout, numRetries);
	      if ((rc != SNMP_ERR_SUCCESS) || engineIdDiscoveryOnly)
	         return rc;
	   }
	   else if ((request->getContextEngineIdLength() == 0) && (m_contextEngine != nullptr))
		{
			request->setContextEngineId(m_contextEngine->getId(), m_contextEngine->getIdLen());
		}
	}

   uint32_t rc;
	if (m_reliable)
	   numRetries = 1;   // Don't do retry on reliable transport
   while(--numRetries >= 0)
   {
		int timeSyncRetries = 3;

retry:
		rc = SNMP_ERR_SUCCESS;
      if (sendMessage(request, timeout) <= 0)
      {
         rc = SNMP_ERR_COMM;
         break;
      }

      uint32_t remainingWaitTime = timeout;

retry_wait:
		delete *response;
		*response = nullptr;
		int64_t startTime = GetCurrentTimeMs();
      int bytes = readMessage(response, remainingWaitTime);
      if (bytes > 0)
      {
         if (*response != nullptr)
         {
            if (!m_codepage.isNull())
               (*response)->setCodepage(m_codepage);
            if (request->getVersion() == SNMP_VERSION_3)
            {
               if ((*response)->getMessageId() == request->getMessageId())
               {
                  // Cache authoritative engine ID
                  if ((m_authoritativeEngine == nullptr) && ((*response)->getAuthoritativeEngine().getIdLen() != 0))
                  {
                     m_authoritativeEngine = new SNMP_Engine((*response)->getAuthoritativeEngine());
                     m_securityContext->setAuthoritativeEngine(*m_authoritativeEngine);
                  }

                  // Cache context engine ID
                  if (((m_contextEngine == nullptr) || (m_contextEngine->getIdLen() == 0)) && ((*response)->getContextEngineIdLength() != 0))
                  {
                     delete m_contextEngine;
                     m_contextEngine = new SNMP_Engine((*response)->getContextEngineId(), (*response)->getContextEngineIdLength());
                  }

                  if ((*response)->getCommand() == SNMP_REPORT)
                  {
                     rc = SNMP_ERR_AGENT;
                     SNMP_Variable *var = (*response)->getVariable(0);
                     if (var != nullptr)
                     {
                        const SNMP_ObjectId& oid = var->getName();
                        for(int i = 0; s_oidToErrorMap[i].oidLen != 0; i++)
                        {
                           if (oid.compare(s_oidToErrorMap[i].oid, s_oidToErrorMap[i].oidLen) == OID_EQUAL)
                           {
                              rc = s_oidToErrorMap[i].errorCode;
                              break;
                           }
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
                        {
                           if (engineIdDiscoveryOnly)
                           {
                              rc = SNMP_ERR_SUCCESS;
                              break;
                           }
                           goto retry;
                        }
                     }
                     else if (rc == SNMP_ERR_TIME_WINDOW)
                     {
                        // Update cached authoritative engine with new boots and time
                        assert(m_authoritativeEngine != nullptr);
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
               else  // message ID do not match
               {
                  uint32_t elapsedTime = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
                  if (elapsedTime < remainingWaitTime)
                  {
                     remainingWaitTime -= elapsedTime;
                     goto retry_wait;
                  }
                  rc = SNMP_ERR_TIMEOUT;
               }
            }
            else  // not SNMPv3
            {
               if ((*response)->getRequestId() == request->getRequestId())
                  break;

               uint32_t elapsedTime = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
               if (elapsedTime < remainingWaitTime)
               {
                  remainingWaitTime -= elapsedTime;
                  goto retry_wait;
               }
               rc = SNMP_ERR_TIMEOUT;
            }
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
 * Send TRAP or INFORM REQUEST message
 */
uint32_t SNMP_Transport::sendTrap(SNMP_PDU *trap, uint32_t timeout, int numRetries)
{
   if ((trap == nullptr) || (numRetries <= 0))
      return SNMP_ERR_PARAM;

   if (trap->getCommand() == SNMP_INFORM_REQUEST)
   {
      SNMP_PDU *response = nullptr;
      uint32_t rc = doRequest(trap, &response, timeout, numRetries);
      delete response;
      return rc;
   }

   if (trap->getCommand() != SNMP_TRAP)
      return SNMP_ERR_PARAM;

   // Create dummy context
   if (m_securityContext == nullptr)
      m_securityContext = new SNMP_SecurityContext();

   // Do engine ID discovery if needed
   // Update SNMP V3 request with cached context engine id
   if (trap->getVersion() == SNMP_VERSION_3)
   {
      if (m_authoritativeEngine == nullptr)
      {
         uint32_t rc = doEngineIdDiscovery(trap, timeout, numRetries);
         if (rc != SNMP_ERR_SUCCESS)
            return rc;
      }
      else if ((trap->getContextEngineIdLength() == 0) && (m_contextEngine != nullptr))
      {
         trap->setContextEngineId(m_contextEngine->getId(), m_contextEngine->getIdLen());
      }
   }

   return (sendMessage(trap, timeout) > 0) ? SNMP_ERR_SUCCESS : SNMP_ERR_COMM;
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
   m_pBuffer = (BYTE *)MemAlloc(m_dwBufferSize);
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
   m_pBuffer = (BYTE *)MemAlloc(m_dwBufferSize);
	m_connected = false;
}

/**
 * Create SNMP_UDPTransport transport connected to given host using host name
 */
uint32_t SNMP_UDPTransport::createUDPTransport(const TCHAR *hostName, uint16_t port)
{
   return createUDPTransport(InetAddress::resolveHostName(hostName), port);
}

/**
 * Create SNMP_UDPTransport transport connected to given host
 */
uint32_t SNMP_UDPTransport::createUDPTransport(const InetAddress& hostAddr, uint16_t port)
{
   if (!hostAddr.isValid())
      return SNMP_ERR_HOSTNAME;

   m_port = port;
   hostAddr.fillSockAddr(&m_peerAddr, port);

   uint32_t result;

   // Create and connect socket
   m_hSocket = CreateSocket(hostAddr.getFamily(), SOCK_DGRAM, 0);
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
         result = SNMP_ERR_SUCCESS;
      }
      else
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
         result = SNMP_ERR_SOCKET;
      }
   }
   else
   {
      result = SNMP_ERR_SOCKET;
   }
   return result;
}

#undef HOSTNAME_VAR

/**
 * Destructor for UDP transport
 */
SNMP_UDPTransport::~SNMP_UDPTransport()
{
   MemFree(m_pBuffer);
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
   SockAddrBuffer srcAddrBuffer;

retry_wait:
   if (dwTimeout != INFINITE)
   {
      if (!SocketCanRead(m_hSocket, dwTimeout))
         return 0;
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
   const BYTE *pbCurrPos;

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
int SNMP_UDPTransport::readMessage(SNMP_PDU **pdu, uint32_t timeout, struct sockaddr *sender,
         socklen_t *addrSize, SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
   if (m_dwBytesInBuffer < 2)
   {
      int bytes = recvData(timeout, sender, addrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

   size_t pduLength = preParsePDU();
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
      int bytes = recvData(timeout, sender, addrSize);
      if (bytes <= 0)
      {
         clearBuffer();
         return bytes;
      }
      m_dwBytesInBuffer += bytes;
   }

	// Change security context if needed
	if (contextFinder != nullptr)
	{
		setSecurityContext(contextFinder(sender, *addrSize));
	}

   // Create new PDU object and remove parsed data from buffer
   *pdu = new SNMP_PDU;
   if (!(*pdu)->parse(&m_pBuffer[m_dwBufferPos], pduLength, m_securityContext, m_enableEngineIdAutoupdate))
   {
      delete *pdu;
      *pdu = nullptr;
   }
   m_dwBytesInBuffer -= pduLength;
   if (m_dwBytesInBuffer == 0)
      m_dwBufferPos = 0;

   return (int)pduLength;
}

/**
 * Send PDU to socket
 */
int SNMP_UDPTransport::sendMessage(SNMP_PDU *pdu, uint32_t timeout)
{
   int bytes = 0;
   BYTE *buffer;
   size_t size = pdu->encode(&buffer, m_securityContext);
   if (size != 0)
   {
      bytes = sendto(m_hSocket, (char *)buffer, (int)size, 0, (struct sockaddr *)&m_peerAddr, SA_LEN((struct sockaddr *)&m_peerAddr));
      MemFree(buffer);
   }
   return bytes;
}

/**
 * Get peer IPv4 address (in host byte order)
 */
InetAddress SNMP_UDPTransport::getPeerIpAddress()
{
   return InetAddress::createFromSockaddr((struct sockaddr *)&m_peerAddr);
}

/**
 * Get port number
 */
uint16_t SNMP_UDPTransport::getPort()
{
   return m_port;
}

/**
 * Check if transport is a proxy transport
 */
bool SNMP_UDPTransport::isProxyTransport()
{
   return false;
}
