/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: main.cpp
**
**/

#include "ntcb.h"
#include <netxms-version.h>
#include <socket_listener.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("NTCB", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Static data
 */
static int32_t s_maxDeviceSessions = 256;
static SharedHashMap<session_id_t, NTCBDeviceSession> s_sessions;
static RWLock s_sessionListLock;
static session_id_t *s_freeList = nullptr;
static size_t s_freePos = 0;
static uint16_t s_listenerPort = 2000;

/**
 * Register new session in list
 */
static bool RegisterSession(const shared_ptr<NTCBDeviceSession>& session)
{
   bool success;
   s_sessionListLock.writeLock();
   if (s_freePos < static_cast<size_t>(s_maxDeviceSessions))
   {
      session->setId(s_freeList[s_freePos++]);
      s_sessions.set(session->getId(), session);
      success = true;
   }
   else
   {
      success = false;
   }
   s_sessionListLock.unlock();

   if (success)
      nxlog_debug_tag(DEBUG_TAG_NTCB, 3, _T("Device session with ID %d registered"), session->getId());
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_NTCB, _T("Too many device sessions open - unable to accept new device connection"));

   return success;
}

/**
 * Unregister session
 */
void UnregisterNTCBDeviceSession(session_id_t id)
{
   s_sessionListLock.writeLock();
   if (s_sessions.contains(id))
   {
      s_sessions.remove(id);
      s_freeList[--s_freePos] = id;
   }
   s_sessionListLock.unlock();
   nxlog_debug_tag(DEBUG_TAG_NTCB, 3, _T("Device session with ID %d unregistered"), id);
}

/**
 * Client listener class
 */
class NTCBListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   NTCBListener(uint16_t port) : StreamSocketListener(port) { setName(_T("NTCB")); }
};

/**
 * Listener stop condition
 */
bool NTCBListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming connection
 */
ConnectionProcessingResult NTCBListener::processConnection(SOCKET s, const InetAddress& peer)
{
   SetSocketNonBlocking(s);
   shared_ptr<NTCBDeviceSession> session = make_shared<NTCBDeviceSession>(s, peer);
   if (RegisterSession(session))
   {
      if (!session->start())
      {
         UnregisterNTCBDeviceSession(session->getId());
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_NTCB, _T("Cannot create device session service thread"));
      }
   }
   return CPR_BACKGROUND;
}

/**
 * Listener thread
 */
static void ListenerThread(uint16_t listenerPort)
{
   ThreadSetName("NTCBListener");
   NTCBListener listener(listenerPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
      return;

   listener.mainLoop();
   listener.shutdown();
}

/**
 * Listener thread handle
 */
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   s_listenerPort = static_cast<uint16_t>(config->getValueAsInt(_T("/NTCB/ListenerPort"), 2000));
   s_maxDeviceSessions = config->getValueAsInt(_T("/NTCB/MaxDeviceSessions"), 256);

   s_freeList = MemAllocArrayNoInit<session_id_t>(s_maxDeviceSessions);
   for(int i = 0; i < s_maxDeviceSessions; i++)
      s_freeList[i] = i;

   return true;
}

/**
 * Server start handler
 */
static void OnServerStart()
{
   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("Starting NTCB listener"));
   s_listenerThread = ThreadCreateEx(ListenerThread, s_listenerPort);
}

/**
 * Shutdown module
 */
static void ShutdownModule()
{
   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("Waiting for NTCB listener thread to stop"));
   ThreadJoin(s_listenerThread);

   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("Terminating active NTCB sessions"));
   s_sessionListLock.readLock();
   for(shared_ptr<NTCBDeviceSession> session : s_sessions)
      session->terminate();
   s_sessionListLock.unlock();

   while(true)
   {
      s_sessionListLock.readLock();
      if (s_sessions.size() == 0)
      {
         s_sessionListLock.unlock();
         break;
      }
      s_sessionListLock.unlock();
      ThreadSleepMs(500);
   }

   MemFree(s_freeList);
   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("NTCB module shutdown completed"));
}

/**
 * Process console commands
 */
static bool ConsoleCommandHandler(const TCHAR *command, ServerConsole *console)
{
   TCHAR buffer[256];
   const TCHAR *arg = ExtractWord(command, buffer);
   if (!IsCommand(_T("NTCB"), buffer, 4))
      return false;

   ExtractWord(arg, buffer);
   if (IsCommand(_T("SESSIONS"), buffer, 2))
   {
      console->print(_T(" ID  | Address         | Device\n"));
      console->print(_T("-----+-----------------+-----------------------------------------------\n"));
      s_sessionListLock.readLock();
      for(shared_ptr<NTCBDeviceSession> session : s_sessions)
      {
         shared_ptr<MobileDevice> device = session->getDevice();
         console->printf(_T(" %3d | %-15s | %s\n"), session->getId(), session->getAddress().toString().cstr(),
                  (device != nullptr) ? device->getName() : _T(""));
      }
      s_sessionListLock.unlock();
   }
   else
   {
      console->print(_T("Invalid NTCB module command\n"));
   }
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"NTCB");
   module->pfInitialize = InitModule;
   module->pfServerStarted = OnServerStart;
   module->pfShutdown = ShutdownModule;
   module->pfProcessServerConsoleCommand = ConsoleCommandHandler;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
