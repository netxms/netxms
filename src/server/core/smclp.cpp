/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: smclp.cpp
**
**/

#include "nxcore.h"


/**
 * Constructor
 */
SMCLP_Connection::SMCLP_Connection(const TCHAR *host, WORD port, int protocol)
{
	m_host = _tcsdup(host);
	m_port = htons(port);
	m_protocol = protocol;
	m_timeout = 5000;
	m_conn = NULL;
}

/**
 * Destructor
 */
SMCLP_Connection::~SMCLP_Connection()
{
	disconnect();
	safe_free(m_host);
}

/**
 * Connect to target
 */
bool SMCLP_Connection::connect(const TCHAR *login, const TCHAR *password)
{
	bool success = false;

	if (m_conn != NULL)
		delete m_conn;

	m_conn = SocketConnection::createTCPConnection(m_host, m_port, 30000);
	if (m_conn != NULL)
	{
		if (m_conn->waitForText("username:", m_timeout))
		{
//			m_conn->writeLine(login);
			if (m_conn->waitForText("password:", m_timeout))
			{
//				m_conn->writeLine(password);
				if (m_conn->waitForText("-> ", m_timeout))
				{
					success = true;
				}
			}
		}

		if (!success)
		{
			m_conn->disconnect();
			delete_and_null(m_conn);
		}
	}

	return success;
}

/**
 * Disconnect from target
 */
void SMCLP_Connection::disconnect()
{
	if (m_conn != NULL)
	{
		m_conn->disconnect();
		delete m_conn;
	}
}
