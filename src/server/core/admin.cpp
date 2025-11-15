/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <nms_users.h>

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
 * Execute server script
 */
static uint32_t ExecuteServerScript(const wchar_t *script, const StringList& args, ServerConsole *console, int32_t *result)
{
   uint32_t rcc = RCC_SUCCESS;
   bool libraryLocked = true;
   bool destroyCompiledScript = false;
   NXSL_Library *scriptLibrary = GetServerScriptLibrary();
   scriptLibrary->lock();

   NXSL_Program *compiledScript = scriptLibrary->findNxslProgram(script);
   if (compiledScript == nullptr)
   {
      scriptLibrary->unlock();
      libraryLocked = false;
      destroyCompiledScript = true;
      char *scriptSource;
      if ((scriptSource = LoadFileAsUTF8String(script)) != nullptr)
      {
         NXSL_CompilationDiagnostic diag;
         NXSL_ServerEnv env;
         wchar_t *wscript = WideStringFromUTF8String(scriptSource);
         compiledScript = NXSLCompile(wscript, &env, &diag);
         MemFree(wscript);
         MemFree(scriptSource);
         if (compiledScript == nullptr)
         {
            rcc = RCC_NXSL_COMPILATION_ERROR;
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteServerScript: Script compilation error: %s"), diag.errorText.cstr());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteServerScript: Script \"%s\" not found"), script);
         rcc = RCC_INVALID_SCRIPT_NAME;
      }
   }

   if (compiledScript != nullptr)
   {
      NXSL_ServerEnv *env = new NXSL_ServerEnv;
      env->setConsole(console);

      NXSL_VM *vm = new NXSL_VM(env);
      if (vm->load(compiledScript))
      {
         if (libraryLocked)
         {
            scriptLibrary->unlock();
            libraryLocked = false;
         }

         NXSL_Value *argv[32];
         int argc = 0;
         while((argc < 32) && (argc < args.size()))
         {
            argv[argc] = vm->createValue(args.get(argc));
            argc++;
         }

         if (vm->run(argc, argv))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteServerScript: Script finished with return value %s"), vm->getResult()->getValueAsCString());
            *result = vm->getResult()->getValueAsInt32();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteServerScript: Script finished with error: %s"), vm->getErrorText());
            rcc = RCC_NXSL_EXECUTION_ERROR;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteServerScript: VM creation failed: %s"), vm->getErrorText());
         rcc = RCC_NXSL_COMPILATION_ERROR;
      }
      delete vm;
      if (destroyCompiledScript)
         delete compiledScript;
   }

   if (libraryLocked)
      scriptLibrary->unlock();

   return rcc;
}

/**
 * Request processing thread
 */
static void ProcessingThread(SOCKET sock)
{
   bool isAuthenticated = false;
   bool requireAuthentication = true;
   bool isInitialized = false;

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

      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      if (request->getCode() == CMD_SET_DB_PASSWORD)
      {
         request->getFieldAsString(VID_PASSWORD, g_szDbPassword, MAX_PASSWORD);
         DecryptPassword(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);
         g_dbPasswordReady.set();
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else if (g_flags & AF_SERVER_INITIALIZED)
      {
         if (!isInitialized)
         {
            requireAuthentication = ConfigReadBoolean(_T("Server.Security.RestrictLocalConsoleAccess"), true);
            isInitialized = true;
         }

         if (request->getCode() == CMD_ADM_REQUEST)
         {
            uint32_t rcc;
            if (isAuthenticated || !requireAuthentication)
            {
               if (request->isFieldExist(VID_SCRIPT))
               {
                  TCHAR script[256];
                  request->getFieldAsString(VID_SCRIPT, script, 256);

                  StringList args(*request, VID_ACTION_ARG_BASE, VID_NUM_ARGS);
                  int executionResult = 0;
                  rcc = ExecuteServerScript(script, args, &console, &executionResult);
                  response.setField(VID_EXECUTION_RESULT, executionResult);
               }
               else
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
            }
            else
            {
               rcc = RCC_ACCESS_DENIED;
            }
            response.setField(VID_RCC, rcc);
         }
         else if (request->getCode() == CMD_LOGIN)
         {
            wchar_t loginName[MAX_USER_NAME], password[MAX_PASSWORD];
            request->getFieldAsString(VID_LOGIN_NAME, loginName, MAX_USER_NAME);
            request->getFieldAsString(VID_PASSWORD, password, MAX_PASSWORD);

            uint32_t userId, graceLogins;
            uint64_t systemRights;
            bool changePassword, intruderLockout, closeOtherSessions;
            uint32_t rcc = AuthenticateUser(loginName, password, 0, nullptr, nullptr, &userId, &systemRights, &changePassword, &intruderLockout, &closeOtherSessions, false, &graceLogins);
            if (rcc == RCC_SUCCESS)
            {
               if (systemRights & SYSTEM_ACCESS_SERVER_CONSOLE)
                  isAuthenticated = true;
               else
                  rcc = RCC_ACCESS_DENIED;
            }
            response.setField(VID_RCC, rcc);
         }
         else if (request->getCode() == CMD_GET_SERVER_INFO)
         {
            response.setField(VID_AUTH_TYPE, requireAuthentication);
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_NOT_IMPLEMENTED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_RESOURCE_NOT_AVAILABLE);
      }

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
