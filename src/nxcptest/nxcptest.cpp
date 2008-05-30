/* $Id: nxscript.cpp 3272 2008-05-14 01:40:18Z alk $ */
/* 
** NetXMS - Network Management System
** NetXMS Scripting Host
** Copyright (C) 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxcptest.cpp
**
**/

#include "nxcptest.h"


//
// Help text
//

static char m_szHelpText[] =
   "NetXMS Communication Protocol Tester  Version " NETXMS_VERSION_STRING "\n"
   "Copyright (c) 2008 Victor Kirhenshtein\n\n"
   "Usage:\n"
   "   nxcptest [options] host\n\n"
   "Where valid options are:\n"
   "   -h         : Show this help\n"
	"   -p port    : Port to connect (default 4700)\n"
   "   -v         : Show version and exit\n"
   "\n";


//
// Read XML document from stdin
//

static BOOL ReadXML(String &xml)
{
	char buffer[8192];

	xml = "";
	while(1)
	{
		fgets(buffer, 8192, stdin);
		if (feof(stdin))
			return FALSE;
		if (!strcmp(buffer, "%%\n"))
			break;
		xml += buffer;
	}
	return TRUE;
}


//
// Do protocol excahnge
//

static void Exchange(SOCKET hSocket)
{
	String xml;
	CSCPMessage *msg;
	CSCP_MESSAGE *rawMsg;

	printf("Enter XML messages separated by %%%% string\n");
	while(1)
	{
		if (!ReadXML(xml))
			break;
		msg = new CSCPMessage(xml);
		rawMsg = msg->CreateMessage();
		delete msg;
		SendEx(hSocket, rawMsg, ntohl(rawMsg->dwSize), 0);
		free(rawMsg);
	}
}


//
// main()
//

int main(int argc, char *argv[])
{
   struct sockaddr_in sa;
	int port = 4700, ch;
	SOCKET hSocket;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "hp:v")) != -1)
   {
      switch(ch)
      {
         case 'h':
				printf(m_szHelpText);
            return 0;
         case 'v':
				printf("NetXMS Communication Protocol Tester  Version " NETXMS_VERSION_STRING "\n"
				       "Copyright (c) 2008 Victor Kirhenshtein\n\n");
            return 0;
			case 'p':
				port = strtol(optarg, NULL, 0);
				break;
         case '?':
            return 1;
         default:
            break;
      }
   }

   if (argc - optind < 1)
   {
      printf("Required arguments missing\n");
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;

   if (WSAStartup(2, &wsaData) != 0)
   {
      _tprintf(_T("Unable to initialize Windows sockets\n"));
      return 4;
   }
#endif

   // Create socket
   hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (hSocket == -1)
   {
      printf("Call to socket() failed");
      goto connect_cleanup;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = ResolveHostName(argv[optind]);
   sa.sin_port = htons((WORD)port);
	if ((sa.sin_addr.s_addr == INADDR_ANY) || (sa.sin_addr.s_addr == INADDR_NONE))
	{
		printf("Cannot resolve host name or invalid IP address\n");
		goto connect_cleanup;
	}

   // Connect to server
   if (connect(hSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      printf("Cannot establish connection with %s:%d\n", argv[optind], port);
      goto connect_cleanup;
   }

	Exchange(hSocket);

connect_cleanup:
	if (hSocket != -1)
		closesocket(hSocket);
   return 0;
}
