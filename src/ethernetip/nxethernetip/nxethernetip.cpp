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
static uint32_t s_timeout = 3;
static SOCKET s_socket = INVALID_SOCKET;
static EthernetIP_MessageReceiver *s_receiver = nullptr;

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

   s_socket = ConnectToHost(addr, s_port, s_timeout * 1000);
   if (s_socket == INVALID_SOCKET)
      return false;

   TCHAR buffer[64];
   _tprintf(_T("Connected to %s:%u\n"), addr.toString(buffer), s_port);
   return true;
}

/**
 * List identity
 */
static bool ListIdentity()
{
   CIP_Message request(CIP_LIST_IDENTITY, 0);
   int bytes = request.getSize();
   if (SendEx(s_socket, request.getBytes(), bytes, 0, nullptr) != bytes)
   {
      TCHAR buffer[1024];
      _tprintf(_T("Request sending failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return false;
   }

   CIP_Message *response = s_receiver->readMessage(s_timeout);
   if (response == nullptr)
   {
      _tprintf(_T("Request timeout\n"));
      return false;
   }

   TCHAR *buffer = MemAllocString(response->getSize() * 2 + 1);
   _tprintf(_T("Raw message: %s\n"), BinToStr(response->getBytes(), response->getSize(), buffer));
   MemFree(buffer);

   if (response->getCommand() != CIP_LIST_IDENTITY)
   {
      _tprintf(_T("Invalid response (command code 0x%04X)\n"), response->getCommand());
      delete response;
      return false;
   }

   _tprintf(_T("%d items in response message\n\n"), response->getItemCount());

   CPF_Item item;
   while(response->nextItem(&item))
   {
      if (item.type != 0x0C)
         continue;

      _tprintf(_T("Supported protocol version: %u"), response->readDataAsUInt16(item.offset));
      _tprintf(_T("Device IP adsress: %s"), InetAddress(response->readDataAsUInt32(item.offset + 6)).toString().cstr());
      _tprintf(_T("Vendor ID: %u"), response->readDataAsUInt16(item.offset + 18));
      _tprintf(_T("Device type: %u"), response->readDataAsUInt16(item.offset + 20));
      _tprintf(_T("Product code: %u"), response->readDataAsUInt16(item.offset + 22));

      TCHAR productName[256];
      if (response->readDataAsLengthPrefixString(item.offset + 32, buffer, 256))
      {
         _tprintf(_T("Product name: %s"), productName);
      }
   }

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
                     _T("   -h           : Display help and exit\n")
                     _T("   -p <port>    : TCP port (default is 44818)\n")
                     _T("   -w <seconds> : Request timeout (default is 3 seconds)\n")
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
            if ((*eptr != 0) || (value > 60) || (value == 0))
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

   s_receiver = new EthernetIP_MessageReceiver(s_socket);

   const char *command = argv[optind + 1];
   if (!stricmp(command, "ListIdentity"))
   {
      if (!ListIdentity())
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
