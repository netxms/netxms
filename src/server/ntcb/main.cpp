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
   NTCBDeviceSession *session = new NTCBDeviceSession(s, peer);
   if (!session->start())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_NTCB, _T("Cannot create device session service thread"));
      delete session;
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
   nxlog_debug_tag(DEBUG_TAG_NTCB, 2, _T("NTCB listener thread stopped"));
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
