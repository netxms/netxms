/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: admin.cpp
**
**/

#include "nxcore.h"
#include <socket_listener.h>
#include <local_admin.h>

#define DEBUG_TAG _T("localadmin")

/**
 * Max message size
 */
#define MAX_MSG_SIZE       65536

/**
 * DB password set condition
 */
extern Condition g_dbPasswordReady;

/**
 * Request processing thread
 */
static void ProcessingThread(SOCKET sock)
{
   bool isAuthenticated = false;
   bool requireAuthentication = ConfigReadBoolean(_T("Server.Security.RestrictLocalConsoleAccess"), true);
   SocketConsole console(sock);
   SocketMessageReceiver receiver(sock, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *request = receiver.readMessage(INFINITE, &result);

      // Receive error
      if (request == nullptr)
      {
         if (result == MSGRECV_CLOSED)
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Local administration interface connection closed"));
         else
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Local administration interface message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (IsShutdownInProgress())
      {
         delete request;
         break;
      }

      uint32_t rcc;
      if (request->getCode() == CMD_ADM_REQUEST)
      {
         if (isAuthenticated || !requireAuthentication)
         {
            TCHAR command[256];
            request->getFieldAsString(VID_COMMAND, command, 256);

            int exitCode = ProcessConsoleCommand(command, &console);
            switch(exitCode)
            {
               case CMD_EXIT_SHUTDOWN:
                  InitiateShutdown(ShutdownReason::FROM_REMOTE_CONSOLE);
                  break;
               case CMD_EXIT_CLOSE_SESSION:
                  delete request;
                  goto close_session;
               default:
                  break;
            }
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_ACCESS_DENIED;
         }
      }
      else if (request->getCode() == CMD_LOGIN)
      {
         TCHAR loginName[MAX_USER_NAME], password[MAX_PASSWORD];
         request->getFieldAsString(VID_LOGIN_NAME, loginName, MAX_USER_NAME);
         request->getFieldAsString(VID_PASSWORD, password, MAX_PASSWORD);

         uint32_t userId, graceLogins;
         uint64_t systemRights;
         bool changePassword, intruderLockout, closeOtherSessions;
         rcc = AuthenticateUser(loginName, password, 0, nullptr, nullptr, &userId, &systemRights, &changePassword, &intruderLockout, &closeOtherSessions, false, &graceLogins);
         if (rcc == RCC_SUCCESS)
         {
            if (systemRights & SYSTEM_ACCESS_SERVER_CONSOLE)
               isAuthenticated = true;
            else
               rcc = RCC_ACCESS_DENIED;
         }
      }
      else if (request->getCode() == CMD_SET_DB_PASSWORD)
      {
         request->getFieldAsString(VID_PASSWORD, g_szDbPassword, MAX_PASSWORD);
         DecryptPassword(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);
         g_dbPasswordReady.set();
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_NOT_IMPLEMENTED;
      }

      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      response.setField(VID_RCC, rcc);
      NXCP_MESSAGE *rawMsgOut = response.serialize();
		SendEx(sock, rawMsgOut, ntohl(rawMsgOut->size), 0, console.getMutex());
      MemFree(rawMsgOut);
      delete request;
   }

close_session:
   shutdown(sock, SHUT_RDWR);
   closesocket(sock);
}

/**
 * Client listener class
 */
class LocalAdminListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   LocalAdminListener() : StreamSocketListener(LOCAL_ADMIN_PORT) { setName(_T("LocalAdmin")); }
};

/**
 * Listener stop condition
 */
bool LocalAdminListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming connection
 */
ConnectionProcessingResult LocalAdminListener::processConnection(SOCKET s, const InetAddress& peer)
{
   ThreadCreate(ProcessingThread, s);
   return CPR_BACKGROUND;
}

/**
 * Local administrative interface listener thread
 */
void LocalAdminListenerThread()
{
   ThreadSetName("LocalAdminLsnr");

   LocalAdminListener listener;
   listener.setListenAddress(_T("127.0.0.1"));
   if (!listener.initialize())
      return;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Local administration interface listener initialized"));

   listener.mainLoop();
   listener.shutdown();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Local administration interface listener stopped"));
}
