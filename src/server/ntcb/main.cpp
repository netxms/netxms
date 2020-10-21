/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
static RWLOCK s_sessionListLock = nullptr;
static session_id_t *s_freeList = nullptr;
static size_t s_freePos = 0;

/**
 * Register new session in list
 */
static bool RegisterSession(const shared_ptr<NTCBDeviceSession>& session)
{
   bool success;
   RWLockWriteLock(s_sessionListLock);
   if (s_freePos < s_maxDeviceSessions)
   {
      session->setId(s_freeList[s_freePos++]);
      s_sessions.set(session->getId(), session);
      success = true;
   }
   else
   {
      success = false;
   }
   RWLockUnlock(s_sessionListLock);

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
   RWLockWriteLock(s_sessionListLock);
   if (s_sessions.contains(id))
   {
      s_sessions.remove(id);
      s_freeList[--s_freePos] = id;
   }
   RWLockUnlock(s_sessionListLock);
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
   uint16_t listenerPort = static_cast<uint16_t>(config->getValueAsInt(_T("/NTCB/ListenerPort"), 2000));
   s_maxDeviceSessions = config->getValueAsInt(_T("/NTCB/MaxDeviceSessions"), 256);

   s_sessionListLock = RWLockCreate();
   s_freeList = MemAllocArrayNoInit<session_id_t>(s_maxDeviceSessions);
   for(int i = 0; i < s_maxDeviceSessions; i++)
      s_freeList[i] = i;

   s_listenerThread = ThreadCreateEx(ListenerThread, listenerPort);
   return true;
}

/**
 * Shutdown module
 */
static void ShutdownModule()
{
   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("Waiting for NTCB listener thread to stop"));
   ThreadJoin(s_listenerThread);

   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("Terminating active NTCB sessions"));
   RWLockReadLock(s_sessionListLock);
   auto it = s_sessions.iterator();
   while(it->hasNext())
   {
      it->next()->get()->terminate();
   }
   delete it;
   RWLockUnlock(s_sessionListLock);

   while(true)
   {
      RWLockReadLock(s_sessionListLock);
      if (s_sessions.size() == 0)
      {
         RWLockUnlock(s_sessionListLock);
         break;
      }
      RWLockUnlock(s_sessionListLock);
      ThreadSleepMs(500);
   }

   MemFree(s_freeList);
   RWLockDestroy(s_sessionListLock);
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

   arg = ExtractWord(arg, buffer);
   if (IsCommand(_T("SESSIONS"), buffer, 2))
   {
      console->print(_T(" ID  | Address         | Device\n"));
      console->print(_T("-----+-----------------+-----------------------------------------------\n"));
      RWLockReadLock(s_sessionListLock);
      auto it = s_sessions.iterator();
      while(it->hasNext())
      {
         NTCBDeviceSession *session = it->next()->get();
         shared_ptr<MobileDevice> device = session->getDevice();
         console->printf(_T(" %3d | %-15s | %s\n"), session->getId(), session->getAddress().toString().cstr(),
                  (device != nullptr) ? device->getName() : _T(""));
      }
      delete it;
      RWLockUnlock(s_sessionListLock);
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
   _tcscpy(module->szName, _T("NTCB"));
   module->pfInitialize = InitModule;
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
