/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: snmpproxy.cpp
**
**/

#include "libnxsrv.h"

/**
 * Constructor
 */
SNMP_ProxyTransport::SNMP_ProxyTransport(AgentConnection *conn, const InetAddress& ipAddr, UINT16 port)
{
   m_reliable = true;   // no need for retries on server side, agent will do retry if needed
	m_agentConnection = conn;
	m_ipAddr = ipAddr;
	m_port = port;
	m_response = NULL;
	m_waitForResponse = true;
}

/**
 * Destructor
 */
SNMP_ProxyTransport::~SNMP_ProxyTransport()
{
	m_agentConnection->decRefCount();
	delete m_response;
}

/**
 * Send PDU
 */
int SNMP_ProxyTransport::sendMessage(SNMP_PDU *pdu)
{
   int nRet = -1;
	NXCPMessage msg(m_agentConnection->getProtocolVersion());

   BYTE *pBuffer;
   size_t size = pdu->encode(&pBuffer, m_securityContext);
   if (size != 0)
   {
		msg.setCode(CMD_SNMP_REQUEST);
		msg.setField(VID_IP_ADDRESS, m_ipAddr);
		msg.setField(VID_PORT, m_port);
		msg.setField(VID_PDU_SIZE, (UINT32)size);
		msg.setField(VID_PDU, pBuffer, size);
      free(pBuffer);

      if (m_waitForResponse)
      {
         m_response = m_agentConnection->customRequest(&msg);
         if (m_response != NULL)
         {
            nRet = 1;
         }
      }
      else
      {
         m_agentConnection->sendMessage(&msg);
      }
   }

   return nRet;
}

/**
 * Receive PDU
 */
int SNMP_ProxyTransport::readMessage(SNMP_PDU **ppData, UINT32 dwTimeout,
                                     struct sockaddr *pSender, socklen_t *piAddrSize,
                                     SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
	int nRet;
	BYTE *pBuffer;
	UINT32 dwSize;

	if (m_response == NULL)
		return -1;

	UINT32 rcc = m_response->getFieldAsUInt32(VID_RCC);
	if (rcc == ERR_SUCCESS)
	{
		dwSize = m_response->getFieldAsUInt32(VID_PDU_SIZE);
		pBuffer = (BYTE *)malloc(dwSize);
		m_response->getFieldAsBinary(VID_PDU, pBuffer, dwSize);

		if (contextFinder != NULL)
			setSecurityContext(contextFinder(pSender, *piAddrSize));

		*ppData = new SNMP_PDU;
		if (!(*ppData)->parse(pBuffer, dwSize, m_securityContext, m_enableEngineIdAutoupdate))
		{
			delete *ppData;
			*ppData = NULL;
		}
		free(pBuffer);
		nRet = (int)dwSize;
	}
	else if (rcc == ERR_REQUEST_TIMEOUT)
	{
	   nRet = 0;
	}
	else
	{
		nRet = -1;
	}

	delete_and_null(m_response);
	return nRet;
}

/**
 * Get peer IP address
 */
InetAddress SNMP_ProxyTransport::getPeerIpAddress()
{
   return m_ipAddr;
}

/**
 * Get port number
 */
UINT16 SNMP_ProxyTransport::getPort()
{
   return m_port;
}

/**
 * Check if this transport is a proxy transport
 */
bool SNMP_ProxyTransport::isProxyTransport()
{
   return true;
}
