/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("snmp.proxy")

/**
 * Constructor
 */
SNMP_ProxyTransport::SNMP_ProxyTransport(const shared_ptr<AgentConnection>& conn, const InetAddress& ipAddr, uint16_t port) : m_agentConnection(conn), m_ipAddr(ipAddr)
{
   m_reliable = true;   // no need for retries on server side, agent will do retry if needed
	m_port = port;
	m_response = nullptr;
	m_waitForResponse = true;
	ipAddr.toString(m_debugId);
}

/**
 * Destructor
 */
SNMP_ProxyTransport::~SNMP_ProxyTransport()
{
	delete m_response;
}

/**
 * Send PDU
 */
int SNMP_ProxyTransport::sendMessage(SNMP_PDU *pdu, uint32_t timeout)
{
   int nRet = -1;
   SNMP_PDUBuffer encodedPDU;
   size_t size = pdu->encode(&encodedPDU, m_securityContext);
   if (size != 0)
   {
      NXCPMessage msg(CMD_SNMP_REQUEST, 0, m_agentConnection->getProtocolVersion());
		msg.setField(VID_IP_ADDRESS, m_ipAddr);
		msg.setField(VID_PORT, m_port);
		msg.setField(VID_PDU_SIZE, static_cast<uint32_t>(size));
		msg.setField(VID_PDU, encodedPDU.buffer(), size);
		msg.setField(VID_TIMEOUT, timeout);

      if (m_waitForResponse)
      {
         m_response = m_agentConnection->customRequest(&msg);
         if (m_response != nullptr)
         {
            nRet = 1;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SNMP_ProxyTransport::sendMessage(%s): request timeout"), m_debugId);
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
int SNMP_ProxyTransport::readMessage(SNMP_PDU **pdu, uint32_t timeout, struct sockaddr *sender,
         socklen_t *addrSize, SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
	if (m_response == nullptr)
		return -1;

	int rc;
	uint32_t rcc = m_response->getFieldAsUInt32(VID_RCC);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("SNMP_ProxyTransport::readMessage(%s): rcc=%u"), m_debugId, rcc);
	if (rcc == ERR_SUCCESS)
	{
	   size_t size;
		const BYTE *encodedPDU = m_response->getBinaryFieldPtr(VID_PDU, &size);
		if (encodedPDU != nullptr)
		{
         if (contextFinder != nullptr)
            setSecurityContext(contextFinder(sender, *addrSize));

         *pdu = new SNMP_PDU;
         if ((*pdu)->parse(encodedPDU, size, m_securityContext, m_enableEngineIdAutoupdate))
         {
            rc = static_cast<int>(size);
         }
         else
         {
            delete *pdu;
            *pdu = nullptr;
            rc = -1; // Malformed PDU
         }
		}
		else
		{
		   rc = -1; // Malformed agent response
		}
	}
	else if (rcc == ERR_REQUEST_TIMEOUT)
	{
	   rc = 0;
	}
	else
	{
		rc = -1;
	}

	delete_and_null(m_response);
	return rc;
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
uint16_t SNMP_ProxyTransport::getPort()
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
