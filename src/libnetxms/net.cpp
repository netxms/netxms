/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
SocketConnection *SocketConnection::createTCPConnection(const TCHAR *hostName, uint16_t port, uint32_t timeout)
{
	SocketConnection *s = new SocketConnection;
	if (!s->connectTCP(hostName, port, timeout))
	{
		delete_and_null(s);
	}
	return s;
}

/**
 * Default constructor
 */
SocketConnection::SocketConnection()
{
   memset(m_data, 0, sizeof(m_data));
	m_dataSize = 0;
	m_dataReadPos = 0;
	m_socket = INVALID_SOCKET;
}

/**
 * Create connection object for existing socket
 */
SocketConnection::SocketConnection(SOCKET s)
{
   memset(m_data, 0, sizeof(m_data));
   m_dataSize = 0;
   m_dataReadPos = 0;
   m_socket = s;
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
bool SocketConnection::connectTCP(const TCHAR *hostName, uint16_t port, uint32_t timeout)
{
   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast() && !addr.isLoopback())
      return false;
   return connectTCP(addr, port, timeout);
}

/**
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address in host byte order
 * @param port port number in host byte order
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool SocketConnection::connectTCP(const InetAddress& ip, uint16_t port, uint32_t timeout)
{
	m_socket = ConnectToHost(ip, port, (timeout != 0) ? timeout : 30000);
	return m_socket != INVALID_SOCKET;
}

/**
 * Check if given socket can be read
 */
bool SocketConnection::canRead(uint32_t timeout)
{
   return (m_dataSize > 0) ? true : SocketCanRead(m_socket, timeout);
}

/**
 * Read data from socket
 */
ssize_t SocketConnection::readFromSocket(void *buffer, size_t size, uint32_t timeout)
{
   return RecvEx(m_socket, buffer, size, 0, timeout);
}

/**
 * Read data from internal buffer or socket
 */
ssize_t SocketConnection::read(void *buffer, size_t size, uint32_t timeout)
{
   if (m_dataSize > 0)
   {
      ssize_t bytes = std::min(m_dataSize, size);
      memcpy(buffer, &m_data[m_dataReadPos], bytes);
      m_dataSize -= bytes;
      if (m_dataSize > 0)
         m_dataReadPos += bytes;
      else
         m_dataReadPos = 0;
      return bytes;
   }

   if (size >= sizeof(m_data))
      return readFromSocket(buffer, size, timeout);

	ssize_t bytes = readFromSocket(m_data, sizeof(m_data), timeout);
	if (bytes <= 0)
	   return bytes;

	if (bytes <= static_cast<ssize_t>(size))
	{
	   memcpy(buffer, m_data, bytes);
	}
	else
	{
      memcpy(buffer, m_data, size);
      m_dataReadPos = size;
      m_dataSize = bytes - size;
      bytes = size;
	}
	return bytes;
}

/**
 * Read exact number of bytes from socket
 */
bool SocketConnection::readFully(void *buffer, size_t size, uint32_t timeout)
{
   BYTE *p = static_cast<BYTE*>(buffer);
   while(size > 0)
   {
      ssize_t bytes = read(p, size, timeout);
      if (bytes <= 0)
         return false;
      size -= bytes;
      p += bytes;
   }
   return true;
}

/**
 * Skip given number of bytes
 */
bool SocketConnection::skip(size_t bytes, uint32_t timeout)
{
   char buffer[1024];
   while(bytes > 0)
   {
      if (!readFully(buffer, std::min(bytes, static_cast<size_t>(1024)), timeout))
         return false;
      bytes -= std::min(bytes, static_cast<size_t>(1024));
   }
   return true;
}

/**
 * Skip bytes with given value (stops at first other byte)
 */
bool SocketConnection::skipBytes(BYTE value, uint32_t timeout)
{
   if (m_dataSize > 0)
   {
      BYTE *p = reinterpret_cast<BYTE*>(&m_data[m_dataReadPos]);
      while((*p == value) && (m_dataSize > 0))
      {
         m_dataSize--;
         m_dataReadPos++;
         p++;
      }
      if (m_dataSize > 0)
         return true;   // Still some data in buffer
   }

   while (m_dataSize == 0)
   {
      ssize_t bytes = RecvEx(m_socket, m_data, sizeof(m_data), 0, timeout);
      if (bytes <= 0)
      {
         if ((bytes == -1) && ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS)))
            continue;
         return false;
      }

      m_dataSize = bytes;
      m_dataReadPos = 0;

      BYTE *p = reinterpret_cast<BYTE*>(m_data);
      while((*p == value) && (m_dataSize > 0))
      {
         m_dataSize--;
         m_dataReadPos++;
         p++;
      }
   }
   return true;
}

/**
 * Write data to socket
 */
ssize_t SocketConnection::write(const void *buffer, size_t size)
{
	return SendEx(m_socket, buffer, size, 0, nullptr);
}

/**
 * Write line to socket (send CR/LF pair after data block
 */
bool SocketConnection::writeLine(const char *line)
{
	if (write(line, strlen(line)) <= 0)
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
 * Wait for specific pattern in input stream. All data up to given pattern is discarded.
 */
bool SocketConnection::waitForData(const void *pattern, size_t patternSize, uint32_t timeout)
{
   if (m_dataSize >= patternSize)
   {
      void *p = memmem(&m_data[m_dataReadPos], m_dataSize, pattern, patternSize);
      if (p != nullptr)
      {
         size_t index = static_cast<size_t>(static_cast<char*>(p) - m_data);
         size_t consumedBytes = index - m_dataReadPos + patternSize;
         m_dataSize -= consumedBytes;
         if (m_dataSize > 0)
            m_dataReadPos += consumedBytes;
         else
            m_dataReadPos = 0;
         return true;
      }

      // Discard all bytes in buffer except last size-1 bytes
      if (m_dataSize > patternSize - 1)
      {
         m_dataReadPos += (m_dataSize - patternSize - 1);
         m_dataSize = patternSize - 1;
      }
   }

   if ((m_dataSize > 0) && (m_dataReadPos > 0))
   {
      memmove(m_data, &m_data[m_dataReadPos], m_dataSize);
      m_dataReadPos = 0;
   }

	while (true)
	{
      ssize_t bytes = RecvEx(m_socket, &m_data[m_dataSize], sizeof(m_data) - m_dataSize, 0, timeout);
      if (bytes <= 0)
      {
         if ((bytes == -1) && ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS)))
            continue;
         return false;
      }

      m_dataSize += bytes;
      if (m_dataSize < patternSize)
         continue;

		void *p = memmem(m_data, m_dataSize, pattern, patternSize);
		if (p != nullptr)
		{
         size_t index = static_cast<size_t>(static_cast<char*>(p) - m_data);
         size_t consumedBytes = index + patternSize;
         m_dataSize -= consumedBytes;
         if (m_dataSize > 0)
            m_dataReadPos = consumedBytes;
         return true;
		}

		memmove(m_data, &m_data[m_dataSize - patternSize - 1], patternSize - 1);
      m_dataSize = patternSize - 1;
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
bool TelnetConnection::connect(const TCHAR *hostName, uint16_t port, uint32_t timeout)
{
   InetAddress addr = InetAddress::resolveHostName(hostName);
   if (!addr.isValidUnicast())
      return false;
   return connect(addr, port, timeout);
}

/**
 * Establish TCP connection to given host and port
 *
 * @param hostName host name or IP address in host byte order
 * @param port port number in host byte order
 * @param timeout connection timeout in milliseconds
 * @return true if connection attempt was successful
 */
bool TelnetConnection::connect(const InetAddress& ip, uint16_t port, uint32_t timeout)
{
   bool ret = SocketConnection::connectTCP(ip, port, timeout);

   if (ret)
   {
      // disable echo
      unsigned char out[3]; 
      out[0] = TELNET_IAC;
      out[1] = TELNET_WONT;
      out[2] = 0x01; // echo
      write((char *)out, 3);
   }

   return ret;
}

/**
 * Read data from socket
 */
ssize_t TelnetConnection::readFromSocket(void *buffer, size_t size, uint32_t timeout)
{
retry:
   ssize_t bytesRead = RecvEx(m_socket, buffer, size, 0, timeout);
   if (bytesRead > 0)
   {
      // process telnet control sequences
      for (ssize_t i = 0; i < bytesRead - 1; i++)
      {
         int skip = 0;
         switch (static_cast<BYTE*>(buffer)[i])
         {
            case TELNET_IAC: // "Interpret as Command"
               {
                  unsigned char cmd = static_cast<BYTE*>(buffer)[i + 1];

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
                           if (static_cast<BYTE*>(buffer)[i + 2] == TELNET_GA)
                           {
                              static_cast<BYTE*>(buffer)[i + 1] = (cmd == TELNET_DO) ? TELNET_WILL : TELNET_DO;
                           }
                           else
                           {
                              static_cast<BYTE*>(buffer)[i + 1] = (cmd == TELNET_DO) ? TELNET_WONT : TELNET_DONT;
                           }
                           write(static_cast<BYTE*>(buffer) + i, 3);
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
            memmove(static_cast<BYTE*>(buffer) + i, static_cast<BYTE*>(buffer) + i + skip, bytesRead - i - 1);
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
ssize_t TelnetConnection::readLine(char *buffer, size_t size, uint32_t timeout)
{
   ssize_t numOfChars = 0;
   ssize_t bytesRead = 0;
   while (true)
   {
      bytesRead = read(buffer + numOfChars, 1, timeout);
      if (bytesRead <= 0)
         break;

      if (buffer[numOfChars] == 0x0d || buffer[numOfChars] == 0x0a)
      {
         if (numOfChars == 0)
         {
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
TelnetConnection *TelnetConnection::createConnection(const TCHAR *hostName, uint16_t port, uint32_t timeout)
{
	TelnetConnection *tc = new TelnetConnection();
	if (!tc->connect(hostName, port, timeout))
	{
      delete_and_null(tc);
	}
	return tc;
}

/**
 * Helper fuction to create connected TelnetConnection object.
 *
 * @param ip IP address
 * @param port port number
 * @param timeout connection timeout in milliseconds
 * @return connected TelnetConnection object or NULL on connection failure
 */
TelnetConnection *TelnetConnection::createConnection(const InetAddress& ip, uint16_t port, uint32_t timeout)
{
   TelnetConnection *tc = new TelnetConnection();
   if (!tc->connect(ip, port, timeout))
   {
      delete_and_null(tc);
   }
   return tc;
}
