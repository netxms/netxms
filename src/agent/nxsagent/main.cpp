/*
** NetXMS Session Agent
** Copyright (C) 2003-2016 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "nxsagent.h"

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * Connection port
 */
static UINT16 s_port = 28180;

/**
 * Socket
 */
static SOCKET s_socket = INVALID_SOCKET;

/**
 * Socket lock
 */
static MUTEX s_socketLock = MutexCreate();

/**
 * Protocol buffer
 */
static NXCP_BUFFER s_msgBuffer;

/**
 * Connect to master agent
 */
static bool ConnectToMasterAgent()
{
   _tprintf(_T("Connecting to master agent...\n"));
   s_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (s_socket == INVALID_SOCKET)
   {
      _tprintf(_T("Call to socket() failed\n"));
      return false;
   }

   // Fill in address structure
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = inet_addr("127.0.0.1");
   sa.sin_port = htons(s_port);

   // Connect to server
	if (ConnectEx(s_socket, (struct sockaddr *)&sa, sizeof(sa), 5000) == -1)
   {
      _tprintf(_T("Cannot establish connection with master agent\n"));
      closesocket(s_socket);
      s_socket = INVALID_SOCKET;
      return false;
   }
   
   return true;
}

/**
 * Send message to master agent
 */
static bool SendMsg(NXCPMessage *msg)
{
   if (s_socket == INVALID_SOCKET)
      return false;

   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = (SendEx(s_socket, rawMsg, ntohl(rawMsg->size), 0, s_socketLock) == ntohl(rawMsg->size));
   free(rawMsg);
   return success;
}

/**
 * Send login message
 */
static void Login()
{
   NXCPMessage msg;
   msg.setCode(CMD_LOGIN);

   DWORD sid;
   ProcessIdToSessionId(GetCurrentProcessId(), &sid);
   msg.setField(VID_SESSION_ID, (UINT32)sid);

   DWORD size;
   WTS_CONNECTSTATE_CLASS *state;
   INT16 sessionState = USER_SESSION_OTHER;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSConnectState, (LPTSTR *)&state, &size))
   {
      switch(*state)
      {
         case WTSActive:
            sessionState = USER_SESSION_ACTIVE;
            break;
         case WTSConnected:
            sessionState = USER_SESSION_CONNECTED;
            break;
         case WTSDisconnected:
            sessionState = USER_SESSION_DISCONNECTED;
            break;
         case WTSIdle:
            sessionState = USER_SESSION_IDLE;
            break;
      }
      WTSFreeMemory(state);
   }
   else
   {
      TCHAR buffer[1024];
      _tprintf(_T("WTSQuerySessionInformation(WTSConnectState) failed (%s)\n"), GetSystemErrorText(GetLastError(), buffer, 1024));
   }

   msg.setField(VID_SESSION_STATE, sessionState);

   TCHAR *sessionName;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSWinStationName, &sessionName, &size))
   {
      if (*sessionName != 0)
      {
         msg.setField(VID_NAME, sessionName);
      }
      else
      {
         TCHAR buffer[256];
         _sntprintf(buffer, 256, _T("%s-%d"), (sessionState == USER_SESSION_DISCONNECTED) ? _T("Disconnected") : ((sessionState == USER_SESSION_IDLE) ? _T("Idle") : _T("Session")), sid);
         msg.setField(VID_NAME, buffer);
      }
      WTSFreeMemory(sessionName);
   }
   else
   {
      TCHAR buffer[1024];
      _tprintf(_T("WTSQuerySessionInformation(WTSWinStationName) failed (%s)\n"), GetSystemErrorText(GetLastError(), buffer, 1024));
      msg.setField(VID_NAME, _T("Console")); // assume console if session name cannot be read
   }

   TCHAR userName[256];
   size = 256;
   if (GetUserName(userName, &size))
   {
      msg.setField(VID_USER_NAME, userName);
   }

   SendMsg(&msg);
}

/**
 * Process request from master agent
 */
static void ProcessRequest(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   switch(request->getCode())
   {
      case CMD_KEEPALIVE:
         msg.setField(VID_RCC, ERR_SUCCESS);
         break;
      case CMD_TAKE_SCREENSHOT:
         TakeScreenshot(&msg);
         break;
      default:
         msg.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
         break;
   }

   SendMsg(&msg);
}

/**
 * Message processing loop
 */
static void ProcessMessages()
{
   NXCPEncryptionContext *dummyCtx = NULL;
   NXCPInitBuffer(&s_msgBuffer);
   UINT32 rawMsgSize = 65536;
   NXCP_MESSAGE *rawMsg = (NXCP_MESSAGE *)malloc(rawMsgSize);
   while(true)
   {
      int err = RecvNXCPMessageEx(s_socket, &rawMsg, &s_msgBuffer, &rawMsgSize, &dummyCtx, NULL, 900000, 4 * 1024 * 1024);
      if (err <= 0)
         break;

      // Check if message is too large
      if (err == 1)
         continue;

      // Check for decryption failure
      if (err == 2)
         continue;

      // Check for timeout
      if (err == 3)
      {
         _tprintf(_T("Socket read timeout\n"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(rawMsg->size) != err)
      {
         _tprintf(_T("Actual message size doesn't match wSize value (%d,%d)\n"), err, ntohl(rawMsg->size));
         continue;   // Bad packet, wait for next
      }

      UINT16 flags = ntohs(rawMsg->flags);
      if (!(flags & MF_BINARY))
      {
         NXCPMessage *msg = NXCPMessage::deserialize(rawMsg);
         if (msg != NULL)
         {
            TCHAR msgCodeName[256];
            _tprintf(_T("Received message %s\n"), NXCPMessageCodeName(msg->getCode(), msgCodeName));
            ProcessRequest(msg);
            delete msg;
         }
         else
         {
            _tprintf(_T("Message deserialization error\n"));
         }
      }
   }
   free(rawMsg);
}

#ifdef _WIN32

/**
 * Window proc for event handling window
 */
static LRESULT CALLBACK EventHandlerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
      case WM_WTSSESSION_CHANGE:
         _tprintf(_T(">> session change: %d\n"), (int)wParam);
         if ((wParam == WTS_CONSOLE_CONNECT) || (wParam == WTS_CONSOLE_DISCONNECT) ||
             (wParam == WTS_REMOTE_CONNECT) || (wParam == WTS_REMOTE_DISCONNECT))
         {
            Login();
         }
         break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

/**
 * Event handling thread
 */
static THREAD_RESULT THREAD_CALL EventHandler(void *arg)
{
   HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = EventHandlerWndProc;
	wc.hInstance = hInstance;
	wc.cbWndExtra = 0;
	wc.lpszClassName = _T("NetXMS_SessionAgent_Wnd");
	if (RegisterClass(&wc) == 0)
   {
      _tprintf(_T("Call to RegisterClass() failed\n"));
      return THREAD_OK;
   }

	HWND hWnd = CreateWindow(_T("NetXMS_SessionAgent_Wnd"), _T("NetXMS Session Agent"), 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hWnd == NULL)
	{
      _tprintf(_T("Cannot create window: %s"), GetSystemErrorText(GetLastError(), NULL, 0));
		return THREAD_OK;
	}

   WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION);

	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

   return THREAD_OK;
}

/**
 * Get our own console window handle (an alternative to Microsoft's GetConsoleWindow)
 */
static HWND GetConsoleHWND()
{
   DWORD cpid = GetCurrentProcessId();
	HWND hWnd = NULL;
   while(true)
   {
	   hWnd = FindWindowEx(NULL, hWnd, _T("ConsoleWindowClass"), NULL);
      if (hWnd == NULL)
         break;

   	DWORD wpid;
      GetWindowThreadProcessId(hWnd, &wpid);
	   if (cpid == wpid)
         break;
   }

	return hWnd;
}

#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   bool hideConsole = false;

   int ch;
   while((ch = getopt(argc, argv, "c:Hv")) != -1)
   {
		switch(ch)
		{
         case 'c':   // config
            break;
         case 'H':   // hide console
            hideConsole = true;
            break;
		   case 'v':   // version
            _tprintf(_T("NetXMS Session Agent Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING _T("\n"));
			   exit(0);
			   break;
		   case '?':
			   return 3;
		}
   }

#ifdef _WIN32
   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      _tprintf(_T("WSAStartup() failed"));
      return 1;
   }

   ThreadCreate(EventHandler, 0, NULL);

   if (hideConsole)
   {
      HWND hWnd = GetConsoleHWND();
      if (hWnd != NULL)
         ShowWindow(hWnd, SW_HIDE);
   }
#endif

   while(true)
   {
      if (!ConnectToMasterAgent())
      {
         ThreadSleep(30);
         continue;
      }

      _tprintf(_T("*** Connected to master agent ***\n"));
      Login();
      ProcessMessages();
   }
   return 0;
}
