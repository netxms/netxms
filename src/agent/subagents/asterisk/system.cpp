/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: system.cpp
**
**/

#include "asterisk.h"

/**
 * Create from configuration entry
 */
AsteriskSystem *AsteriskSystem::createFromConfig(ConfigEntry *config, bool defaultSystem)
{
   AsteriskSystem *as = new AsteriskSystem(defaultSystem ? _T("LOCAL") : config->getName());

   as->m_ipAddress = InetAddress::resolveHostName(config->getSubEntryValue(_T("Hostname"), 0, _T("127.0.0.1")));
   if (!as->m_ipAddress.isValidUnicast() && !as->m_ipAddress.isLoopback())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid or unknown hostname for system %s"), config->getName());
      delete as;
      return NULL;
   }

   int port = config->getSubEntryValueAsInt(_T("Port"), 0, 5038);
   if ((port < 1) || (port > 65535))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Invalid port number for system %s"), config->getName());
      delete as;
      return NULL;
   }
   as->m_port = (UINT16)port;

   as->m_login = UTF8StringFromTString(config->getSubEntryValue(_T("Login"), 0, _T("root")));
   as->m_password = UTF8StringFromTString(config->getSubEntryValue(_T("Password"), 0, _T("")));

   ConfigEntry *registrationsRoot = config->findEntry(_T("SIPRegistrationTests"));
   if (registrationsRoot != NULL)
   {
      unique_ptr<ObjectArray<ConfigEntry>> registrations = registrationsRoot->getSubEntries(NULL);
      for(int i = 0; i < registrations->size(); i++)
      {
         char defaultProxy[128] = "sip:";
         as->m_ipAddress.toStringA(&defaultProxy[4]);
         SIPRegistrationTest *r = new SIPRegistrationTest(registrations->get(i), defaultProxy);
         as->m_registrationTests.set(r->getName(), r);
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Added SIP registration test %s (%hs@%hs via %hs every %d seconds)"),
                  r->getName(), r->getLogin(), r->getDomain(), r->getProxy(), r->getInterval() / 1000);
      }
   }

   return as;
}

/**
 * Constructor
 */
AsteriskSystem::AsteriskSystem(const TCHAR *name) :
         m_requestCompletion(false), m_eventListeners(0, 16, Ownership::False), m_eventListenersLock(MutexType::FAST),
         m_peerEventCounters(Ownership::True), m_eventCounterLock(MutexType::FAST), m_rtcpData(Ownership::True),
         m_peerRTCPStatistic(Ownership::True), m_rtcpLock(MutexType::FAST), m_registrationTests(Ownership::True)

{
   m_name = MemCopyString(name);
   m_port = 5038;
   m_login = nullptr;
   m_password = nullptr;
   m_connectorThread = INVALID_THREAD_HANDLE;
   m_socket = INVALID_SOCKET;
   m_requestId = 1;
   m_activeRequestId = 0;
   m_amiSessionReady = false;
   m_resetSession = false;
   m_amiTimeout = 2000;
   memset(&m_globalEventCounters, 0, sizeof(EventCounters));
}

/**
 * Destructor
 */
AsteriskSystem::~AsteriskSystem()
{
   MemFree(m_name);
   MemFree(m_login);
   MemFree(m_password);
}

/**
 * Add AMI event listener
 */
void AsteriskSystem::addEventListener(AmiEventListener *listener)
{
   m_eventListenersLock.lock();
   if (m_eventListeners.indexOf(listener) == -1)
      m_eventListeners.add(listener);
   m_eventListenersLock.unlock();
}

/**
 * Remove AMI event listener
 */
void AsteriskSystem::removeEventListener(AmiEventListener *listener)
{
   m_eventListenersLock.lock();
   m_eventListeners.remove(listener);
   m_eventListenersLock.unlock();
}

/**
 * Get tag value as parameter value from request without parameters
 */
LONG AsteriskSystem::readSingleTag(const char *rqname, const char *tag, TCHAR *value)
{
   shared_ptr<AmiMessage> response = sendRequest(make_shared<AmiMessage>(rqname));
   if (response == nullptr)
      return SYSINFO_RC_ERROR;

   if (!response->isSuccess())
   {
      const char *reason = response->getTag("Message");
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Request \"%hs\" to %s failed (%hs)"), rqname, m_name, (reason != NULL) ? reason : "Unknown reason");
      return SYSINFO_RC_ERROR;
   }

   LONG rc;
   const char *v = response->getTag(tag);
   if (v != nullptr)
   {
      ret_mbstring(value, v);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }
   return rc;
}

/**
 * Read table (as sequence of event messages)
 */
SharedObjectArray<AmiMessage> *AsteriskSystem::readTable(const char *rqname)
{
   auto messages = new SharedObjectArray<AmiMessage>();

   shared_ptr<AmiMessage> response = sendRequest(make_shared<AmiMessage>(rqname), messages);
   if (response == nullptr)
   {
      delete messages;
      return nullptr;
   }

   if (!response->isSuccess())
   {
      const char *reason = response->getTag("Message");
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Request \"%hs\" to %s failed (%hs)"), rqname, m_name, (reason != NULL) ? reason : "Unknown reason");
      delete messages;
      return nullptr;
   }

   return messages;
}

/**
 * Execute CLI command
 */
StringList *AsteriskSystem::executeCommand(const char *command)
{
   auto request = make_shared<AmiMessage>("Command");
   request->setTag("Command", command);
   shared_ptr<AmiMessage> response = sendRequest(request);
   if (response == nullptr)
      return nullptr;

   if (!response->isSuccess() || (response->getData() == nullptr))
   {
      const char *reason = response->getTag("Message");
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Request \"Command\" to %s failed (%hs)"), m_name, (reason != NULL) ? reason : "Unknown reason");
      return nullptr;
   }
   StringList *output = response->acquireData();
   return output;
}
