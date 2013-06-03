/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#define TELNET_WILL 0xFB
#define TELNET_WONT 0xFC
#define TELNET_DO 0xFD
#define TELNET_DONT 0xFE
#define TELNET_IAC 0xFF
#define TELNET_GA 0xF9

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
bool SocketConnection::connectTCP(const TCHAR *hostName, WORD port, DWORD timeout)
{
   return connectTCP(ntohl(ResolveHostName(hostName)), port, timeout);
}

/**
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address in host byte order
 * @param port port number in host byte order
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool SocketConnection::connectTCP(DWORD ip, WORD port, DWORD timeout)
{
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket != INVALID_SOCKET)
	{
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(ip);

		if (ConnectEx(m_socket, (struct sockaddr*)&sa, sizeof(sa), (timeout != 0) ? timeout : 30000) < 0)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
      SetSocketNonBlocking(m_socket);
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
		memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos + 1);
		return true;
	}

	m_dataPos = min(bufLen, textLen - 1);
	memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos + 1);

	while(1)
	{
		if (!canRead(timeout))
		{
			return false;
		}

		int size = read(&m_data[m_dataPos], 4095 - m_dataPos);
      if ((size <= 0) && (WSAGetLastError() != WSAEWOULDBLOCK) && (WSAGetLastError() != WSAEINPROGRESS))
      {
         return false;
      }

		m_data[size + m_dataPos] = 0;
		bufLen = (int)strlen(m_data);

		p = strstr(m_data, text);
		if (p != NULL)
		{
			int index = (int)(p - m_data);
			m_dataPos = bufLen - (index + textLen);
			memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos + 1);
			return true;
		}

		m_dataPos = min(bufLen, textLen - 1);
		memmove(m_data, &m_data[bufLen - m_dataPos], m_dataPos);
	}
}

/*
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address
 * @param port port number in host byte order
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool TelnetConnection::connect(const TCHAR *hostName, WORD port, DWORD timeout)
{
   return connect(ntohl(ResolveHostName(hostName)), port, timeout);
}

/**
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address in host byte order
 * @param port port number in host byte order
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool TelnetConnection::connect(DWORD ip, WORD port, DWORD timeout)
{
   bool ret = SocketConnection::connectTCP(ip, port, timeout);

   if (ret)
   {
      // disable echo
      unsigned char out[3]; 
      out[0] = TELNET_IAC;
      out[1] = TELNET_WILL;
      out[2] = 0x01; // echo
      write((char *)out, 3);
   }

   return ret;
}

/**
 * Read data from socket
 */
int TelnetConnection::read(char *pBuff, int nSize, DWORD timeout)
{
retry:
	int bytesRead = RecvEx(m_socket, pBuff, nSize, 0, timeout);
   if (bytesRead > 0)
   {
      // process telnet control sequences
      for (int i = 0; i < bytesRead - 1; i++)
      {
         int skip = 0;
         switch ((unsigned char)pBuff[i])
         {
            case TELNET_IAC: // "Interpret as Command"
               {
                  unsigned char cmd = (unsigned char)pBuff[i + 1];

                  switch (cmd)
                  {
                     case TELNET_IAC:
                        // Duplicate IAC - data byte 0xFF, just deduplicate
                        skip = 1;
                        break;
                     case TELNET_WILL:
                     case TELNET_DO:
                     case TELNET_DONT:
                     case TELNET_WONT:
                        if ((i + 1) < bytesRead)
                        {
                           skip = 3;
                           if ((unsigned char)pBuff[i + 2] == TELNET_GA)
                           {
                              pBuff[i + 1] = cmd == TELNET_DO ? TELNET_WILL : TELNET_DO;
                           }
                           else
                           {
                              pBuff[i + 1] = cmd == TELNET_DO ? TELNET_WONT : TELNET_DONT;
                           }
                           write(pBuff + i, 3);
                        }
                        break;
                     default:
                        skip = 2; // skip IAC + unhandled command
                  }
               }
               break;
            case 0:
               // "No Operation", skip
               skip = 1;
         }

         if (skip > 0)
         {
            memmove(pBuff + i, pBuff + i + skip, bytesRead - i - 1);
            bytesRead -= skip;
            i--;
         }
      }

      // Only control sequence was received, read more data
      if (bytesRead == 0)
         goto retry;
   }

   return bytesRead;
}


/**
 * Read line from socket
 */
int TelnetConnection::readLine(char *buffer, int size, DWORD timeout)
{
   int numOfChars = 0;
	int bytesRead = 0;
   while (true) {
      bytesRead = read(buffer + numOfChars, 1, timeout);
      if (bytesRead <= 0) {
         break;
      }

      if (buffer[numOfChars] == 0x0d || buffer[numOfChars] == 0x0a)
      {
         if (numOfChars == 0) {
            // ignore leading new line characters
         }
         else
         {
            // got complete string, return
            break;
         }
      }
      else
      {
         numOfChars++;
      }
   };

   buffer[numOfChars] = 0;
   return numOfChars;
}

/**
 * Helper fuction to create connected TelnetConnection object.
 *
 * @param hostName host name or IP address
 * @param port port number
 * @param timeout connection timeout in milliseconds
 * @return connected TelnetConnection object or NULL on connection failure
 */
TelnetConnection *TelnetConnection::createConnection(const TCHAR *hostName, WORD port, DWORD timeout)
{
	TelnetConnection *tc = new TelnetConnection();
	if (!tc->connect(hostName, port, timeout))
	{
		delete tc;
		tc = NULL;
	}

	return tc;
}
