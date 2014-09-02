/*
** NetXMS Session Agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
static CSCP_BUFFER s_msgBuffer;

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
static bool SendMsg(CSCPMessage *msg)
{
   if (s_socket == INVALID_SOCKET)
      return false;

   CSCP_MESSAGE *rawMsg = msg->createMessage();
   bool success = (SendEx(s_socket, rawMsg, ntohl(rawMsg->dwSize), 0, s_socketLock) == ntohl(rawMsg->dwSize));
   free(rawMsg);
   return success;
}

/**
 * Send login message
 */
static void Login()
{
   CSCPMessage msg;
   msg.SetCode(CMD_LOGIN);

   DWORD sid;
   ProcessIdToSessionId(GetCurrentProcessId(), &sid);
   msg.SetVariable(VID_SESSION_ID, (UINT32)sid);

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

   msg.SetVariable(VID_SESSION_STATE, sessionState);

   TCHAR *sessionName;
   if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSWinStationName, &sessionName, &size))
   {
      if (*sessionName != 0)
      {
         msg.SetVariable(VID_NAME, sessionName);
      }
      else
      {
         TCHAR buffer[256];
         _sntprintf(buffer, 256, _T("%s-%d"), (sessionState == USER_SESSION_DISCONNECTED) ? _T("Disconnected") : ((sessionState == USER_SESSION_IDLE) ? _T("Idle") : _T("Session")), sid);
         msg.SetVariable(VID_NAME, buffer);
      }
      WTSFreeMemory(sessionName);
   }

   TCHAR userName[256];
   size = 256;
   if (GetUserName(userName, &size))
   {
      msg.SetVariable(VID_USER_NAME, userName);
   }

   SendMsg(&msg);
}

/**
 * Process request from master agent
 */
static void ProcessRequest(CSCPMessage *request)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   switch(request->GetCode())
   {
      case CMD_KEEPALIVE:
         msg.SetVariable(VID_RCC, ERR_SUCCESS);
         break;
      case CMD_TAKE_SCREENSHOT:
         TakeScreenshot(&msg);
         break;
      default:
         msg.SetVariable(VID_RCC, ERR_UNKNOWN_COMMAND);
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
   RecvNXCPMessage(0, NULL, &s_msgBuffer, 0, NULL, NULL, 0);
   UINT32 rawMsgSize = 65536;
   CSCP_MESSAGE *rawMsg = (CSCP_MESSAGE *)malloc(rawMsgSize);
   while(1)
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
         _tprintf(_T("Socket read timeout"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(rawMsg->dwSize) != err)
      {
         _tprintf(_T("Actual message size doesn't match wSize value (%d,%d)"), err, ntohl(rawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      UINT16 flags = ntohs(rawMsg->wFlags);
      if (!(flags & MF_BINARY))
      {
         CSCPMessage *msg = new CSCPMessage(rawMsg);
         TCHAR msgCodeName[256];
         _tprintf(_T("Received message %s\n"), NXCPMessageCodeName(msg->GetCode(), msgCodeName));
         ProcessRequest(msg);
         delete msg;
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
         _tprintf(_T(">> session change: %d\n"), wParam);
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

#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
#ifdef _WIN32
   WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      _tprintf(_T("WSAStartup() failed"));
      return 1;
   }

   ThreadCreate(EventHandler, 0, NULL);
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
