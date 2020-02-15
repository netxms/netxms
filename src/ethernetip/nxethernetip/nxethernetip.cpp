/* 
** nxethernetip - command line tool for Ethernet/IP
** Copyright (C) 2004-2020 Victor Kirhenshtein
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
** File: nxethernetip.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <ethernet_ip.h>

NETXMS_EXECUTABLE_HEADER(nxethernetip)

/**
 * Static data
 */
static uint16_t s_port = ETHERNET_IP_DEFAULT_PORT;
static uint32_t s_timeout = 5000;
static SOCKET s_socket = INVALID_SOCKET;
static EtherNetIP_MessageReceiver *s_receiver = nullptr;

/**
 * Dump raw bytes
 */
static void DumpBytes(const uint8_t *data, size_t length)
{
   TCHAR textForm[32], buffer[64];
   for(size_t i = 0; i < length; i += 16)
   {
      const uint8_t *block = data + i;
      size_t blockSize = MIN(16, length - i);
      BinToStrEx(block, blockSize, buffer, _T(' '), 16 - blockSize);
      size_t j;
      for(j = 0; j < blockSize; j++)
      {
         uint8_t b = block[j];
         textForm[j] = ((b >= ' ') && (b < 127)) ? (TCHAR)b : _T('.');
      }
      textForm[j] = 0;
      _tprintf(_T("   %04X | %s | %s\n"), i, buffer, textForm);
   }
}

/**
 * Connect to device
 */
static bool Connect(const char *hostname)
{
   InetAddress addr = InetAddress::resolveHostName(hostname);
   if (addr.getFamily() == AF_UNSPEC)
   {
      _tprintf(_T("Cannot resolve host name %hs\n"), hostname);
      return false;
   }

   s_socket = ConnectToHost(addr, s_port, s_timeout);
   if (s_socket == INVALID_SOCKET)
   {
      TCHAR ipAddrText[64], errorText[256];
      _tprintf(_T("Cannot connected to %s:%u (%s)\n"), addr.toString(ipAddrText), s_port, GetLastSocketErrorText(errorText, 256));
      return false;
   }

   TCHAR buffer[64];
   _tprintf(_T("Connected to %s:%u\n"), addr.toString(buffer), s_port);
   return true;
}

/**
 * Generic function for sending list type command
 */
static CIP_Message *SendListCommand(CIP_Command command)
{
   CIP_Message request(command, 0);
   size_t bytes = request.getSize();
   if (SendEx(s_socket, request.getBytes(), bytes, 0, nullptr) != bytes)
   {
      TCHAR buffer[1024];
      _tprintf(_T("Request sending failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return nullptr;
   }

   CIP_Message *response = s_receiver->readMessage(s_timeout);
   if (response == nullptr)
   {
      _tprintf(_T("Request timeout\n"));
      return nullptr;
   }

   _tprintf(_T("Raw message:\n"));
   DumpBytes(response->getBytes(), response->getSize());
   _tprintf(_T("\n"));

   if (response->getCommand() != command)
   {
      _tprintf(_T("Invalid response (command code 0x%04X)\n"), response->getCommand());
      delete response;
      return nullptr;
   }

   _tprintf(_T("Status: %02X (%s)\n"), response->getStatus(), EtherNetIP_StatusTextFromCode(response->getStatus()));
   if (response->getStatus() != EIP_STATUS_SUCCESS)
   {
      delete response;
      return nullptr;
   }

   _tprintf(_T("%d item%s in response message\n\n"), response->getItemCount(), response->getItemCount() == 1 ? _T("") : _T("s"));
   return response;
}

/**
 * List identity
 */
static bool ListIdentity()
{
   CIP_Message *response = SendListCommand(CIP_LIST_IDENTITY);
   if (response == nullptr)
      return false;

   CPF_Item item;
   while(response->nextItem(&item))
   {
      if (item.type != 0x0C)
         continue;

      _tprintf(_T("Protocol version.....: %u\n"), response->readDataAsUInt16(item.offset));
      _tprintf(_T("Device IP address....: %s\n"), response->readDataAsInetAddress(item.offset + 6).toString().cstr());
      _tprintf(_T("Vendor...............: %u (%s)\n"), response->readDataAsUInt16(item.offset + 18), CIP_VendorNameFromCode(response->readDataAsUInt16(item.offset + 18)));
      _tprintf(_T("Device type..........: %u (%s)\n"), response->readDataAsUInt16(item.offset + 20), CIP_DeviceTypeNameFromCode(response->readDataAsUInt16(item.offset + 20)));

      TCHAR productName[256] = _T("");
      if (response->readDataAsLengthPrefixString(item.offset + 32, productName, 256))
      {
         _tprintf(_T("Product name.........: %s\n"), productName);
      }
      _tprintf(_T("Product code.........: %u\n"), response->readDataAsUInt16(item.offset + 22));
      _tprintf(_T("Product revision.....: %u.%u\n"), response->readDataAsUInt8(item.offset + 24), response->readDataAsUInt8(item.offset + 25));
      _tprintf(_T("Serial number........: %08X\n"), response->readDataAsUInt32(item.offset + 28));

      _tprintf(_T("Status...............: %04X\n"), response->readDataAsUInt16(item.offset + 26));
      uint8_t state = response->readDataAsUInt8(item.offset + 33 + _tcslen(productName));
      _tprintf(_T("State................: %u (%s)\n"), state, CIP_DeviceStateTextFromCode(state));
   }

   delete response;
   return true;
}

/**
 * List interfaces
 */
static bool ListInterfaces()
{
   CIP_Message *response = SendListCommand(CIP_LIST_INTERFACES);
   if (response == nullptr)
      return false;

   delete response;
   return true;
}

/**
 * List services
 */
static bool ListServices()
{
   CIP_Message *response = SendListCommand(CIP_LIST_SERVICES);
   if (response == nullptr)
      return false;

   delete response;
   return true;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   bool start = true;
   int exitCode = 0;
   uint32_t value;
   char *eptr;
   int ch;
	while((ch = getopt(argc, argv, "hp:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxethernetip [<options>] <host> <command>\n")
                     _T("Valid options are:\n")
                     _T("   -h                : Display help and exit\n")
                     _T("   -p <port>         : TCP port (default is 44818)\n")
                     _T("   -w <milliseconds> : Request timeout (default is 5000 milliseconds)\n")
                     _T("\n"));
            start = false;
            break;
         case 'p':   // Port number
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 65535) || (value == 0))
            {
               _tprintf(_T("Invalid port number %hs\n"), optarg);
               start = false;
               exitCode = 1;
            }
            else
            {
               s_port = static_cast<uint16_t>(value);
            }
            break;
         case 'w':   // Timeout
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 60000) || (value == 0))
            {
               _tprintf(_T("Invalid timeout value %hs\n"), optarg);
               start = false;
               exitCode = 1;
            }
            else
            {
               s_timeout = value;
            }
            break;
         case '?':
            exitCode = 1;
            start = false;
            break;
         default:
            break;
      }
   }

	if (!start)
	   return exitCode;

   if (argc - optind < 2)
   {
      _tprintf(_T("Required argument(s) missing.\nUse nxethernetip -h to get complete command line syntax.\n"));
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   if (!Connect(argv[optind]))
      return 2;

   s_receiver = new EtherNetIP_MessageReceiver(s_socket);

   const char *command = argv[optind + 1];
   if (!stricmp(command, "ListIdentity"))
   {
      if (!ListIdentity())
         exitCode = 4;
   }
   else if (!stricmp(command, "ListInterfaces"))
   {
      if (!ListInterfaces())
         exitCode = 4;
   }
   else if (!stricmp(command, "ListServices"))
   {
      if (!ListServices())
         exitCode = 4;
   }
   else
   {
      _tprintf(_T("Invalid command %hs\n"), command);
      exitCode = 3;
   }

   delete s_receiver;
   shutdown(s_socket, SHUT_RDWR);
   closesocket(s_socket);

   return exitCode;
}
