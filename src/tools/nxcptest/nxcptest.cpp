/* $Id$ */
/* 
** NetXMS - Network Management System
** NXCP testing tool
** Copyright (C) 2008 Victor Kirhenshtein
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
// Constants
//

#define RAW_MSG_SIZE    262144


//
// Help text
//

static TCHAR m_szHelpText[] =
   _T("NetXMS Communication Protocol Tester  Version ") NETXMS_VERSION_STRING _T("\n")
   _T("Copyright (c) 2008 Victor Kirhenshtein\n\n")
   _T("Usage:\n")
   _T("   nxcptest [options] host\n\n")
   _T("Where valid options are:\n")
   _T("   -h         : Show this help\n")
	_T("   -p port    : Port to connect (default 4700)\n")
   _T("   -v         : Show version and exit\n")
   _T("\n");


//
// Receiver thread
//

static THREAD_RESULT THREAD_CALL RecvThread(void *arg)
{
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
	CSCP_BUFFER *pMsgBuffer;
	NXCPEncryptionContext *pCtx = NULL;
   BYTE *pDecryptionBuffer = NULL;
   int iErr;
   char *xml;
   WORD wFlags;

   // Initialize raw message receiving function
	pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pMsgBuffer, 0, NULL, NULL, 0);

   pRawMsg = (CSCP_MESSAGE *)malloc(RAW_MSG_SIZE);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(RAW_MSG_SIZE);
#endif
   while(1)
   {
      if ((iErr = RecvNXCPMessage(CAST_FROM_POINTER(arg, SOCKET), pRawMsg, pMsgBuffer, RAW_MSG_SIZE,
                                  &pCtx, pDecryptionBuffer, INFINITE)) <= 0)
      {
         break;
      }

      // Check if message is too large
      if (iErr == 1)
		{
         printf("RECV ERROR: message is too large\n");
         continue;
		}

      // Check for decryption failure
      if (iErr == 2)
      {
         printf("RECV ERROR: decryption failure\n");
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         _tprintf(_T("RECV ERROR: Actual message size doesn't match wSize value (%d,%d)\n"), iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      wFlags = ntohs(pRawMsg->wFlags);
      if (wFlags & MF_BINARY)
      {
			printf("WARNING: Binary message received\n");
      }
      else
      {
         // Create message object from raw message
         pMsg = new CSCPMessage(pRawMsg);
			xml = pMsg->CreateXML();
			if (xml != NULL)
			{
				puts(xml);
				free(xml);
			}
      }
   }
   if (iErr < 0)
      _tprintf(_T("Session terminated (socket error %d)\n"), WSAGetLastError());
	else
		_tprintf(_T("Session closed\n"));
	free(pMsgBuffer);
	free(pRawMsg);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif

	return THREAD_OK;
}


//
// Read XML document from stdin
//

static BOOL ReadXML(String &xml)
{
	char buffer[8192];

	xml = _T("");
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

	ThreadCreate(RecvThread, 0, (void *)hSocket);
	_tprintf(_T("Enter XML messages separated by %%%% string\n"));
	while(1)
	{
		if (!ReadXML(xml))
			break;
		msg = new CSCPMessage(xml);
		rawMsg = msg->CreateMessage();
		delete msg;
		if ((DWORD)SendEx(hSocket, rawMsg, ntohl(rawMsg->dwSize), 0, NULL) != ntohl(rawMsg->dwSize))
		{
			_tprintf(_T("Error sending message\n"));
			free(rawMsg);
			break;
		}
		free(rawMsg);
	}
	shutdown(hSocket, 2);
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
				_tprintf(m_szHelpText);
            return 0;
         case 'v':
				_tprintf(_T("NetXMS Communication Protocol Tester  Version ") NETXMS_VERSION_STRING _T("\n")
				       _T("Copyright (c) 2008 Victor Kirhenshtein\n\n"));
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
   sa.sin_addr.s_addr = ResolveHostNameA(argv[optind]);
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
