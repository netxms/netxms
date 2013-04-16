/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
SNMP_ProxyTransport::SNMP_ProxyTransport(AgentConnection *pConn, DWORD dwIpAddr, WORD wPort)
{
	m_pAgentConnection = pConn;
	m_dwIpAddr = dwIpAddr;
	m_wPort = wPort;
	m_pResponse = NULL;
}

/**
 * Destructor
 */
SNMP_ProxyTransport::~SNMP_ProxyTransport()
{
	delete m_pAgentConnection;
	delete m_pResponse;
}

/**
 * Send PDU
 */
int SNMP_ProxyTransport::sendMessage(SNMP_PDU *pdu)
{
   BYTE *pBuffer;
   DWORD dwSize;
   int nRet = -1;
	CSCPMessage msg(m_pAgentConnection->getProtocolVersion());

   dwSize = pdu->encode(&pBuffer, m_securityContext);
   if (dwSize != 0)
   {
		msg.SetCode(CMD_SNMP_REQUEST);
		msg.SetVariable(VID_IP_ADDRESS, m_dwIpAddr);
		msg.SetVariable(VID_PORT, m_wPort);
		msg.SetVariable(VID_PDU_SIZE, dwSize);
		msg.SetVariable(VID_PDU, pBuffer, dwSize);
      free(pBuffer);

		m_pResponse = m_pAgentConnection->customRequest(&msg);
		if (m_pResponse != NULL)
		{
			nRet = 1;
		}
   }

   return nRet;
}

/**
 * Receive PDU
 */
int SNMP_ProxyTransport::readMessage(SNMP_PDU **ppData, DWORD dwTimeout, 
                                     struct sockaddr *pSender, socklen_t *piAddrSize,
                                     SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t))
{
	int nRet;
	BYTE *pBuffer;
	DWORD dwSize;

	if (m_pResponse == NULL)
		return -1;

	if (m_pResponse->GetVariableLong(VID_RCC) == ERR_SUCCESS)
	{
		dwSize = m_pResponse->GetVariableLong(VID_PDU_SIZE);
		pBuffer = (BYTE *)malloc(dwSize);
		m_pResponse->GetVariableBinary(VID_PDU, pBuffer, dwSize);

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
	else
	{
		nRet = -1;
	}

	delete_and_null(m_pResponse);
	return nRet;
}

/**
 * Get peer IPv4 address (in host byte order)
 */
DWORD SNMP_ProxyTransport::getPeerIpAddress()
{
   return m_dwIpAddr;
}
