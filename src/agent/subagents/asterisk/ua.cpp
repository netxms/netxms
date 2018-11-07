/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: ua.cpp
**/

#include "asterisk.h"
#include <eXosip2/eXosip.h>

/**
 * Do SIP client registration
 */
static INT32 RegisterSIPClient(const char *login, const char *password, const char *domain, const char *proxy, int *status)
{
   *status = 400; // default status for client-side errors

   char uri[256];
   snprintf(uri, 256, "sip:%s@%s", login, domain);

   struct eXosip_t *ctx = eXosip_malloc();
   if (ctx == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_malloc() failed"), uri, proxy);
      return -1;
   }

   if (eXosip_init(ctx) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_init() failed"), uri, proxy);
      return -1;
   }

   eXosip_set_user_agent(ctx, "NetXMS/" NETXMS_BUILD_TAG_A);

   // open a UDP socket
   if (eXosip_listen_addr(ctx, IPPROTO_UDP, NULL, 0, AF_INET, 0) != 0)
   {
      eXosip_quit(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_listen_addr() failed"), uri, proxy);
      return -1;
   }

   eXosip_lock(ctx);

   eXosip_add_authentication_info(ctx, login, login, password, NULL, NULL);

   osip_message_t *regmsg = NULL;
   int registrationId = eXosip_register_build_initial_register(ctx, uri, proxy, uri, 300, &regmsg);
   if (registrationId < 0)
   {
      eXosip_unlock(ctx);
      eXosip_quit(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_register_build_initial_register() failed"), uri, proxy);
      return -1;
   }

   INT64 startTime = GetCurrentTimeMs();
   int rc = eXosip_register_send_register(ctx, registrationId, regmsg);
   eXosip_unlock(ctx);

   if (rc != 0)
   {
      eXosip_quit(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_register_send_register() failed"), uri, proxy);
      return -1;
   }

   bool success = false;
   INT32 elapsedTime;
   while(true)
   {
      eXosip_event_t *evt = eXosip_event_wait(ctx, 0, 100);

      eXosip_execute(ctx);
      eXosip_automatic_action(ctx);

      if (evt == NULL)
         continue;

      if ((evt->type == EXOSIP_REGISTRATION_FAILURE) && (evt->response != NULL) &&
          ((evt->response->status_code == 401) || (evt->response->status_code == 407)))
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration failed with status code %03d (will retry)"),
                  uri, proxy, evt->response->status_code);
         eXosip_default_action(ctx, evt);
         eXosip_event_free(evt);
         *status = evt->response->status_code;
         continue;
      }

      if (evt->type == EXOSIP_REGISTRATION_SUCCESS)
      {
         elapsedTime = static_cast<INT32>(GetCurrentTimeMs() - startTime);
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration successful"), uri, proxy);
         success = true;
         eXosip_event_free(evt);
         *status = 200;
         break;
      }

      if (evt->type != EXOSIP_MESSAGE_PROCEEDING)
      {
         int statusCode = (evt->response != NULL) ? evt->response->status_code : 408;
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration failed with status code %03d (will not retry)"), uri, proxy, statusCode);
         eXosip_event_free(evt);
         *status = statusCode;
         break;
      }

      eXosip_event_free(evt);
   }

   eXosip_quit(ctx);
   return success ? elapsedTime : -1;
}

/**
 * Handler for Asterisk.SIP.TestRegistration parameter
 */
LONG H_SIPTestRegistration(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(3);

   char login[128], password[128], domain[128];
   GET_ARGUMENT_A(1, login, 128);
   GET_ARGUMENT_A(2, password, 128);
   GET_ARGUMENT_A(3, domain, 128);

   int status;
   char proxy[256] = "sip:";
   sys->getIpAddress().toStringA(&proxy[4]);
   ret_int64(value, RegisterSIPClient(login, password, domain, proxy, &status));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Registration test constructor
 */
SIPRegistrationTest::SIPRegistrationTest(ConfigEntry *config, const char *defaultProxy)
{
   m_name = _tcsdup(config->getName());
#ifdef UNICODE
   m_login = UTF8StringFromWideString(config->getSubEntryValue(L"Login", 0, L"netxms"));
   m_password = UTF8StringFromWideString(config->getSubEntryValue(L"Password", 0, L"netxms"));
   m_domain = UTF8StringFromWideString(config->getSubEntryValue(L"Domain", 0, L""));
   const WCHAR *proxy = config->getSubEntryValue(L"Proxy", 0, NULL);
   m_proxy = (proxy != NULL) ? UTF8StringFromWideString(proxy) : strdup(defaultProxy);
#else
   m_login = strdup(config->getSubEntryValue("Login", 0, "netxms"));
   m_password = strdup(config->getSubEntryValue("Password", 0, "netxms"));
   m_domain = strdup(config->getSubEntryValue("Domain", 0, ""));
   m_proxy = strdup(config->getSubEntryValue("Proxy", 0, defaultProxy));
#endif
   m_interval = config->getSubEntryValueAsUInt(_T("Interval"), 0, 300) * 1000;
   m_lastRunTime = 0;
   m_elapsedTime = 0;
   m_status = 500;
   m_mutex = MutexCreate();
   m_stop = false;
}

/**
 * Registration test destructor
 */
SIPRegistrationTest::~SIPRegistrationTest()
{
   MemFree(m_domain);
   MemFree(m_login);
   MemFree(m_name);
   MemFree(m_password);
   MemFree(m_proxy);
   MutexDestroy(m_mutex);
}

/**
 * Run test
 */
void SIPRegistrationTest::run()
{
   if (m_stop)
      return;

   int status;
   INT32 elapsed = RegisterSIPClient(m_login, m_password, m_domain, m_proxy, &status);
   MutexLock(m_mutex);
   m_lastRunTime = time(NULL);
   m_elapsedTime = elapsed;
   m_status = status;
   MutexUnlock(m_mutex);
   nxlog_debug_tag(DEBUG_TAG, 7, _T("SIP registration test for %hs@%hs via %hs completed (status=%03d elapsed=%d)"),
            m_login, m_domain, m_proxy, status, elapsed);

   // Re-schedule
   if (!m_stop)
      ThreadPoolScheduleRelative(g_asteriskThreadPool, m_interval, this, &SIPRegistrationTest::run);
}

/**
 * Start tests
 */
void SIPRegistrationTest::start()
{
   ThreadPoolScheduleRelative(g_asteriskThreadPool, m_interval, this, &SIPRegistrationTest::run);
}

/**
 * Stop tests
 */
void SIPRegistrationTest::stop()
{
   m_stop = true;
}

/**
 * Handler for Asterisk.SIP.RegistrationTests list
 */
LONG H_SIPRegistrationTestList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   ObjectArray<SIPRegistrationTest> *tests = sys->getRegistrationTests();
   for(int i = 0; i < tests->size(); i++)
      value->add(tests->get(i)->getName());
   delete tests;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.SIP.RegistrationTests table
 */
LONG H_SIPRegistrationTestTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("LOGIN"), DCI_DT_STRING, _T("Login"));
   value->addColumn(_T("DOMAIN"), DCI_DT_STRING, _T("Domain"));
   value->addColumn(_T("PROXY"), DCI_DT_STRING, _T("Proxy"));
   value->addColumn(_T("INTERVAL"), DCI_DT_INT, _T("Proxy"));
   value->addColumn(_T("STATUS"), DCI_DT_INT, _T("Status"));
   value->addColumn(_T("ELAPSED_TIME"), DCI_DT_INT, _T("Elapsed Time"));
   value->addColumn(_T("TIMESTAMP"), DCI_DT_INT64, _T("Timestamp"));

   ObjectArray<SIPRegistrationTest> *tests = sys->getRegistrationTests();
   for(int i = 0; i < tests->size(); i++)
   {
      SIPRegistrationTest *t = tests->get(i);
      value->addRow();
      value->set(0, t->getName());
      value->set(1, t->getLogin());
      value->set(2, t->getDomain());
      value->set(3, t->getProxy());
      value->set(4, t->getInterval());
      value->set(5, t->getStatus());
      value->set(6, t->getElapsedTime());
      value->set(7, static_cast<INT64>(t->getLastRunTime()));
   }
   delete tests;
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.SIP.RegistrationTest.* parameters
 */
LONG H_SIPRegistrationTestData(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   TCHAR name[128];
   GET_ARGUMENT(1, name, 128);

   SIPRegistrationTest *test = sys->getRegistartionTest(name);
   if (test == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   if (test->getLastRunTime() == 0)
      return SYSINFO_RC_ERROR;   // Data not collected yet

   switch(*arg)
   {
      case 'E':
         ret_int(value, test->getElapsedTime());
         break;
      case 'S':
         ret_int(value, test->getStatus());
         break;
      case 'T':
         ret_int64(value, test->getLastRunTime());
         break;
   }

   return SYSINFO_RC_SUCCESS;
}
