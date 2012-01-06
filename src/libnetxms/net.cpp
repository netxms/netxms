/* 
** NetXMS - Network Management System
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
** File: net.cpp
**
**/

#include "libnetxms.h"


/**
 * Helper fuction to create connected SocketConnection object.
 *
 * @param hostName host name or IP address
 * @param port port number
 * @param timeout connection timeout in milliseconds
 * @return connected SocketConnection object or NULL on connection failure
 */
SocketConnection *SocketConnection::createTCPConnection(const TCHAR *hostName, WORD port, DWORD timeout)
{
	SocketConnection *s = new SocketConnection;
	if (!s->connectTCP(hostName, port, timeout))
	{
		delete s;
		s = NULL;
	}
	return s;
}

/**
 * Default constructor
 */
SocketConnection::SocketConnection()
{
	m_dataPos = 0;
	m_data[0] = 0;
}

/**
 * Destructor
 */
SocketConnection::~SocketConnection()
{
	if (m_socket != INVALID_SOCKET)
		closesocket(m_socket);
}

/**
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address
 * @param port port number
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool SocketConnection::connectTCP(const TCHAR *hostName, unsigned short port, DWORD timeout)
{
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket != INVALID_SOCKET)
	{
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = ResolveHostName(hostName);

		if (ConnectEx(m_socket, (struct sockaddr*)&sa, sizeof(sa), (timeout != 0) ? timeout : 30000) < 0)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}

	return m_socket != INVALID_SOCKET;
}

/**
 * Check if given socket can be read
 */
bool SocketConnection::canRead(DWORD timeout)
{
	bool ret = false;
	struct timeval tv;
	fd_set readFdSet;

	FD_ZERO(&readFdSet);
	FD_SET(m_socket, &readFdSet);
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (select(SELECT_NFDS(m_socket + 1), &readFdSet, NULL, NULL, &tv) > 0)
	{
		ret = true;
	}

	return ret;
}

/**
 * Read data from socket
 */
int SocketConnection::read(char *pBuff, int nSize, DWORD timeout)
{
	return RecvEx(m_socket, pBuff, nSize, 0, timeout);
}

/**
 * Write data to socket
 */
int SocketConnection::write(const char *pBuff, int nSize)
{
	return SendEx(m_socket, pBuff, nSize, 0, NULL);
}

/**
 * Write line to socket (send CR/LF pair after data block
 */
bool SocketConnection::writeLine(const char *line)
{
	if (write(line, (int)strlen(line)) <= 0)
		return false;
	return write("\r\n", 2) > 0;
}

/**
 * Close socket
 */
void SocketConnection::disconnect()
{
	shutdown(m_socket, SHUT_RDWR);
	closesocket(m_socket);
	m_socket = INVALID_SOCKET;
}

/**
 * Wait for specific text in input stream. All data up to given text are discarded.
 */
bool SocketConnection::waitForText(const char *text, int timeout)
{
	int textLen = (int)strlen(text);
	int bufLen = (int)strlen(m_data);

	char *p = strstr(m_data, text);
	if (p != NULL)
	{
		int index = (int)(p - m_data);
		m_dataPos = bufLen - (index + textLen);
		memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos);
		return true;
	}

	m_dataPos = min(bufLen, textLen - 1);
	memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos);

	while(1)
	{
		if (!canRead(timeout))
			return false;

		int size = read(&m_data[m_dataPos], 4095 - m_dataPos);
		m_data[size + m_dataPos] = 0;
		bufLen = (int)strlen(m_data);

		p = strstr(m_data, text);
		if (p != NULL)
		{
			int index = (int)(p - m_data);
			m_dataPos = bufLen - (index + textLen);
			memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos);
			return true;
		}

		m_dataPos = min(bufLen, textLen - 1);
		memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos);
	}
}
