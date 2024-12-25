/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: tftp.cpp
**
**/

#include "libnxagent.h"
#include <fstream>

/**
 * Block size
 */
#define BLOCK_SIZE 512

/**
 * Timeout
 */
#define TIMEOUT   1000

/**
 * TFTP commands
 */
enum Command
{
   TFTP_RRQ = 1,
   TFTP_WRQ = 2,
   TFTP_DATA = 3,
   TFTP_ACK = 4,
   TFTP_ERROR = 5
};

#pragma pack(1)

/**
 * Packet structure
 */
struct Packet
{
   uint16_t command;
   union
   {
      uint16_t blockNumber;
      uint16_t errorCode;
      char fileName[2];
   };
   char data[BLOCK_SIZE];
};

#pragma pack()

/**
 * Buffer class for memory stream implementation
 */
class membuf : public std::basic_streambuf<char>
{
public:
   membuf(const BYTE *data, size_t size)
   {
      setg((char*)data, (char*)data, (char*)data + size);
   }
};

/**
 * Simple memory stream implementation
 */
class memstream : public std::istream
{
private:
   membuf _buffer;

public:
   memstream(const BYTE *data, size_t size) : std::istream(&_buffer), _buffer(data, size)
   {
      rdbuf(&_buffer);
   }
};

/**
 * Buffer for ByteStream wrapper
 */
class bsbuf : public std::basic_streambuf<char>
{
private:
   ByteStream *_bs;

protected:
   virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override
   {
      _bs->write(s, n);
      return n;
   }

   virtual int_type overflow(int_type c) override
   {
      _bs->write(static_cast<BYTE>(c));
      return c;
   }

public:
   bsbuf(ByteStream *bs)
   {
      _bs = bs;
   }
};

/**
 * Wrapper around ByteStream that provides STL interface for writing
 */
class bstream : public std::ostream
{
private:
   bsbuf _buffer;

public:
   bstream(ByteStream *bs) : std::ostream(&_buffer), _buffer(bs) { }
};

/**
 * Prepare socket for transfer
 */
static SOCKET PrepareSocket(const InetAddress& addr)
{
   SOCKET s = CreateSocket(addr.getFamily(), SOCK_DGRAM, 0);
   if (s == INVALID_SOCKET)
      return INVALID_SOCKET;

   SockAddrBuffer localAddr;
   memset(&localAddr, 0, sizeof(SockAddrBuffer));
   if (addr.getFamily() == AF_INET)
   {
      localAddr.sa4.sin_family = AF_INET;
   }
#ifdef WITH_IPV6
   else
   {
      localAddr.sa6.sin6_family = AF_INET6;
   }
#endif

   // We use bind() because response will be sent from different source port
   if (bind(s, reinterpret_cast<struct sockaddr*>(&localAddr), SA_LEN(reinterpret_cast<struct sockaddr*>(&localAddr))) != 0)
   {
      closesocket(s);
      return INVALID_SOCKET;
   }

   SetSocketNonBlocking(s);
   return s;
}

/**
 * Write file to server via TFTP
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(const BYTE *data, size_t size, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   memstream s(data, size);
   return TFTPWrite(&s, remoteFileName, addr, port, progressCallback);
}

/**
 * Write file to server via TFTP
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(const TCHAR *fileName, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   std::ifstream s;
#ifdef UNICODE
   char fn[MAX_PATH];
   WideCharToMultiByteSysLocale(fileName, fn, MAX_PATH);
   s.open(fn, std::ios::binary);
#else
   s.open(fileName, std::ios::binary);
#endif
   if (s.fail())
      return TFTP_FILE_READ_ERROR;

   TFTPError rc = TFTPWrite(&s, (remoteFileName != nullptr) ? remoteFileName : GetCleanFileName(fileName), addr, port, progressCallback);

   s.close();
   return rc;
}

/**
 * Write file to server via TFTP
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(std::istream *stream, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   SOCKET s = PrepareSocket(addr);
   if (s == INVALID_SOCKET)
      return TFTP_SOCKET_ERROR;

   SockAddrBuffer sa;
   addr.fillSockAddr(&sa, port);

   Packet request, response;

   request.command = htons(TFTP_WRQ);
#ifdef UNICODE
   wchar_to_utf8(remoteFileName, -1, request.fileName, BLOCK_SIZE - 8);
#else
   strlcpy(request.fileName, remoteFileName, BLOCK_SIZE - 8);
#endif
   size_t flen = strlen(request.fileName);
   memcpy(reinterpret_cast<char*>(&request) + flen + 3, "octet\0" "512", 10);

   bool connected = false;
   int retryCount = 5;
   while(!connected && (retryCount-- > 0))
   {
      if (sendto(s, reinterpret_cast<char*>(&request), flen + 13, 0, reinterpret_cast<struct sockaddr*>(&sa), SA_LEN(reinterpret_cast<struct sockaddr*>(&sa))) == -1)
      {
         closesocket(s);
         return TFTP_SOCKET_ERROR;
      }

      if (!SocketCanRead(s, TIMEOUT))
         continue;

      socklen_t alen = sizeof(sa);
      int bytes = recvfrom(s, reinterpret_cast<char*>(&response), sizeof(response), 0, reinterpret_cast<struct sockaddr*>(&sa), &alen);
      if (bytes == -1)
      {
         closesocket(s);
         return TFTP_SOCKET_ERROR;
      }

      if (bytes < 4)
         continue;   // Packet is too small

      if ((ntohs(response.command) == TFTP_ACK) && (response.blockNumber == 0))
      {
         connected = true;
      }
      else if (ntohs(response.command) == TFTP_ERROR)
      {
         closesocket(s);
         return static_cast<TFTPError>(static_cast<int>(TFTP_PROTOCOL_ERROR) + static_cast<int>(ntohs(response.errorCode)));
      }
   }
   if (!connected)
   {
      closesocket(s);
      return TFTP_TIMEOUT;
   }

   request.command = htons(TFTP_DATA);

   size_t bytesTotal = 0;
   uint16_t blockNumber = 1;
   while(!stream->eof())
   {
      request.blockNumber = htons(blockNumber);

      stream->read(request.data, BLOCK_SIZE);
      if (stream->bad())
      {
         closesocket(s);
         return TFTP_FILE_READ_ERROR;
      }
      size_t blockSize = static_cast<size_t>(stream->gcount());

      retryCount = 5;
      bool sent = false;
      while(!sent && (retryCount-- > 0))
      {
         if (sendto(s, reinterpret_cast<char*>(&request), blockSize + 4, 0, reinterpret_cast<struct sockaddr*>(&sa), SA_LEN(reinterpret_cast<struct sockaddr*>(&sa))) == -1)
         {
            closesocket(s);
            return TFTP_SOCKET_ERROR;
         }

         if (!SocketCanRead(s, TIMEOUT))
            continue;

         int bytes = recv(s, reinterpret_cast<char*>(&response), sizeof(response), 0);
         if (bytes >= 4)
         {
            if ((ntohs(response.command) == TFTP_ACK) && (ntohs(response.blockNumber) == blockNumber))
            {
               sent = true;
            }
            else if (ntohs(response.command) == TFTP_ERROR)
            {
               closesocket(s);
               return static_cast<TFTPError>(static_cast<int>(TFTP_PROTOCOL_ERROR) + static_cast<int>(ntohs(response.errorCode)));
            }
         }
      }
      if (!sent)
      {
         closesocket(s);
         return TFTP_TIMEOUT;
      }

      blockNumber++;
      if (progressCallback != nullptr)
      {
         bytesTotal += blockSize;
         progressCallback(bytesTotal);
      }
   }

   closesocket(s);
   return TFTP_SUCCESS;
}

/**
 * Read remote file from TFTP server into given local file
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(const TCHAR *fileName, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   std::ofstream s;
#ifdef UNICODE
   char fn[MAX_PATH];
   WideCharToMultiByteSysLocale(fileName, fn, MAX_PATH);
   s.open(fn, std::ios::binary);
#else
   s.open(fileName, std::ios::binary);
#endif
   if (s.fail())
      return TFTP_FILE_WRITE_ERROR;

   TFTPError rc = TFTPRead(&s, (remoteFileName != nullptr) ? remoteFileName : GetCleanFileName(fileName), addr, port, progressCallback);

   s.close();
   return rc;
}

/**
 * Read remote file from TFTP server into byte stream
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(ByteStream *output, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   bstream s(output);
   return TFTPRead(&s, remoteFileName, addr, port, progressCallback);
}

/**
 * Read remote file from TFTP server into given output stream
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(std::ostream *stream, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port, std::function<void (size_t)> progressCallback)
{
   SOCKET s = PrepareSocket(addr);
   if (s == INVALID_SOCKET)
      return TFTP_SOCKET_ERROR;

   SockAddrBuffer sa;
   addr.fillSockAddr(&sa, port);

   Packet request, response;

   request.command = htons(TFTP_RRQ);
#ifdef UNICODE
   wchar_to_utf8(remoteFileName, -1, request.fileName, BLOCK_SIZE - 8);
#else
   strlcpy(request.fileName, remoteFileName, BLOCK_SIZE - 8);
#endif
   size_t flen = strlen(request.fileName);
   memcpy(reinterpret_cast<char*>(&request) + flen + 3, "octet\0" "512", 10);

   bool connected = false;
   int retryCount = 5;
   int bytes;
   while(!connected && (retryCount-- > 0))
   {
      if (sendto(s, reinterpret_cast<char*>(&request), flen + 13, 0, reinterpret_cast<struct sockaddr*>(&sa), SA_LEN(reinterpret_cast<struct sockaddr*>(&sa))) == -1)
      {
         closesocket(s);
         return TFTP_SOCKET_ERROR;
      }

      if (!SocketCanRead(s, TIMEOUT))
         continue;

      socklen_t alen = sizeof(sa);
      bytes = recvfrom(s, reinterpret_cast<char*>(&response), sizeof(response), 0, reinterpret_cast<struct sockaddr*>(&sa), &alen);
      if (bytes == -1)
      {
         closesocket(s);
         return TFTP_SOCKET_ERROR;
      }

      if (bytes < 4)
         continue;   // Packet is too small

      if ((ntohs(response.command) == TFTP_DATA) && (ntohs(response.blockNumber) == 1))
      {
         connected = true;
      }
      else if (ntohs(response.command) == TFTP_ERROR)
      {
         closesocket(s);
         return static_cast<TFTPError>(static_cast<int>(TFTP_PROTOCOL_ERROR) + static_cast<int>(ntohs(response.errorCode)));
      }
   }
   if (!connected)
   {
      closesocket(s);
      return TFTP_TIMEOUT;
   }

   size_t bytesTotal = bytes - 4;
   if (progressCallback != nullptr)
      progressCallback(bytesTotal);

   request.command = htons(TFTP_ACK);

   uint16_t blockNumber = 1;
   do
   {
      stream->write(response.data, bytes - 4);
      if (stream->bad())
      {
         closesocket(s);
         return TFTP_FILE_WRITE_ERROR;
      }

      request.blockNumber = htons(blockNumber);

      retryCount = 5;
      bool nextBlock = false;
      while(!nextBlock && (retryCount-- > 0))
      {
         if (sendto(s, reinterpret_cast<char*>(&request), 4, 0, reinterpret_cast<struct sockaddr*>(&sa), SA_LEN(reinterpret_cast<struct sockaddr*>(&sa))) == -1)
         {
            closesocket(s);
            return TFTP_SOCKET_ERROR;
         }

         if (bytes < BLOCK_SIZE + 4)
         {
            // Last block received
            nextBlock = true;
            break;
         }

         if (!SocketCanRead(s, TIMEOUT))
            continue;

         bytes = recv(s, reinterpret_cast<char*>(&response), sizeof(response), 0);
         if (bytes >= 4)
         {
            if ((ntohs(response.command) == TFTP_DATA) && (ntohs(response.blockNumber) == blockNumber + 1))
            {
               nextBlock = true;
            }
            else if (ntohs(response.command) == TFTP_ERROR)
            {
               closesocket(s);
               return static_cast<TFTPError>(static_cast<int>(TFTP_PROTOCOL_ERROR) + static_cast<int>(ntohs(response.errorCode)));
            }
         }
      }
      if (!nextBlock)
      {
         closesocket(s);
         return TFTP_TIMEOUT;
      }

      blockNumber++;
      if (progressCallback != nullptr)
      {
         bytesTotal += bytes - 4;
         progressCallback(bytesTotal);
      }
   }
   while(bytes == BLOCK_SIZE + 4);

   // Write final block to file
   if (bytes > 4)
   {
      stream->write(response.data, bytes - 4);
      if (stream->bad())
      {
         closesocket(s);
         return TFTP_FILE_WRITE_ERROR;
      }
   }

   closesocket(s);
   return TFTP_SUCCESS;
}

/**
 * Get error message from error code
 */
const TCHAR LIBNXAGENT_EXPORTABLE *TFTPErrorMessage(TFTPError code)
{
   static const TCHAR *messages[] =
   {
       _T("Success"),
       _T("File read error"),
       _T("File write error"),
       _T("Socket error"),
       _T("Timeout"),
       _T("Protocol error"),
       _T("File not found"),
       _T("Access violation"),
       _T("Disk full"),
       _T("Illegal operation"),
       _T("Unknown transfer ID"),
       _T("File already exists"),
       _T("No such user")
   };
   return ((int)code >= 0) && ((int)code < sizeof(messages) / sizeof(TCHAR*)) ? messages[code] : _T("Unknown error");
}
