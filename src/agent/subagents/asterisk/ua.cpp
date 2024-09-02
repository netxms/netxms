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
** File: ua.cpp
**/

#include "asterisk.h"
#include <netxms-version.h>
#include <eXosip2/eXosip.h>

/**
 * SIP status codes
 */
static CodeLookupElement s_sipStatusCodes[] =
{
   { 100, _T("Trying") },
   { 180, _T("Ringing") },
   { 181, _T("Call is Being Forwarded") },
   { 182, _T("Queued") },
   { 183, _T("Session Progress") },
   { 199, _T(" Early Dialog Terminated") },
   { 200, _T("OK") },
   { 202, _T("Accepted") },
   { 204, _T("No Notification") },
   { 300, _T("Multiple Choices") },
   { 301, _T("Moved Permanently") },
   { 302, _T("Moved Temporarily") },
   { 305, _T("Use Proxy") },
   { 380, _T("Alternative Service") },
   { 400, _T("Bad Request") },
   { 401, _T("Unauthorized") },
   { 402, _T("Payment Required") },
   { 403, _T("Forbidden") },
   { 404, _T("Not Found") },
   { 405, _T("Method Not Allowed") },
   { 406, _T("Not Acceptable") },
   { 407, _T("Proxy Authentication Required") },
   { 408, _T("Request Timeout") },
   { 409, _T("Conflict") },
   { 410, _T("Gone") },
   { 411, _T("Length Required") },
   { 412, _T("Conditional Request Failed") },
   { 413, _T("Request Entity Too Large") },
   { 414, _T("Request-URI Too Long") },
   { 415, _T("Unsupported Media Type") },
   { 416, _T("Unsupported URI Scheme") },
   { 417, _T("Unknown Resource-Priority") },
   { 420, _T("Bad Extension") },
   { 421, _T("Extension Required") },
   { 422, _T("Session Interval Too Small") },
   { 423, _T("Interval Too Brief") },
   { 424, _T("Bad Location Information") },
   { 428, _T("Use Identity Header") },
   { 429, _T("Provide Referrer Identity") },
   { 430, _T("Flow Failed") },
   { 433, _T("Anonymity Disallowed") },
   { 436, _T("Bad Identity-Info") },
   { 437, _T("Unsupported Certificate") },
   { 438, _T("Invalid Identity Header") },
   { 439, _T("First Hop Lacks Outbound Support") },
   { 440, _T("Max-Breadth Exceeded") },
   { 469, _T("Bad Info Package") },
   { 470, _T("Consent Needed") },
   { 480, _T("Temporarily Unavailable") },
   { 481, _T("Call/Transaction Does Not Exist") },
   { 482, _T("Loop Detected") },
   { 483, _T("Too Many Hops") },
   { 484, _T("Address Incomplete") },
   { 485, _T("Ambiguous") },
   { 486, _T("Busy Here") },
   { 487, _T("Request Terminated") },
   { 488, _T("Not Acceptable Here") },
   { 489, _T("Bad Event") },
   { 491, _T("Request Pending") },
   { 493, _T("Undecipherable") },
   { 494, _T("Security Agreement Required") },
   { 500, _T("Internal Server Error") },
   { 501, _T("Not Implemented") },
   { 502, _T("Bad Gateway") },
   { 503, _T("Service Unavailable") },
   { 504, _T("Server Time-out") },
   { 505, _T("Version Not Supported") },
   { 513, _T("Message Too Large") },
   { 555, _T("Push Notification Service Not Supported") },
   { 580, _T("Precondition Failure") },
   { 600, _T("Busy Everywhere") },
   { 603, _T("Decline") },
   { 604, _T("Does Not Exist Anywhere") },
   { 606, _T("Not Acceptable") },
   { 607, _T("Unwanted") },
   { 608, _T("Rejected") },
   { 0, nullptr }
};

/**
 * Destroy exosip context
 */
static inline void eXosip_free(struct eXosip_t *ctx)
{
   eXosip_quit(ctx);
   osip_free(ctx);
}

/**
 * Do SIP client registration
 */
static int32_t RegisterSIPClient(const char *login, const char *password, const char *domain, const char *proxy, int32_t timeout, int *status)
{
   *status = 400; // default status for client-side errors

   char uri[256];
   snprintf(uri, 256, "sip:%s@%s", login, domain);
   nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): starting registration (timeout %d seconds)"), uri, proxy, timeout);

   struct eXosip_t *ctx = eXosip_malloc();
   if (ctx == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_malloc() failed"), uri, proxy);
      return -1;
   }

   if (eXosip_init(ctx) != 0)
   {
      osip_free(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_init() failed"), uri, proxy);
      return -1;
   }

   eXosip_set_user_agent(ctx, "NetXMS/" NETXMS_BUILD_TAG_A);

   // open a UDP socket
   if (eXosip_listen_addr(ctx, IPPROTO_UDP, nullptr, 0, AF_INET, 0) != 0)
   {
      eXosip_free(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_listen_addr() failed"), uri, proxy);
      return -1;
   }

   eXosip_lock(ctx);

   eXosip_add_authentication_info(ctx, login, login, password, nullptr, nullptr);

   osip_message_t *regmsg = nullptr;
   int registrationId = eXosip_register_build_initial_register(ctx, uri, proxy, uri, timeout, &regmsg);
   if (registrationId < 0)
   {
      eXosip_unlock(ctx);
      eXosip_free(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_register_build_initial_register() failed"), uri, proxy);
      return -1;
   }

   int64_t startTime = GetMonotonicClockTime();
   int rc = eXosip_register_send_register(ctx, registrationId, regmsg);
   eXosip_unlock(ctx);

   if (rc != 0)
   {
      eXosip_free(ctx);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RegisterSIPClient(%hs via %hs): call to eXosip_register_send_register() failed"), uri, proxy);
      return -1;
   }

   bool success = false;
   int32_t elapsedTime;
   while(true)
   {
      eXosip_event_t *evt = eXosip_event_wait(ctx, 0, 100);

      eXosip_execute(ctx);
      eXosip_automatic_action(ctx);

      if (evt == nullptr)
         continue;

      nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): event %d"), uri, proxy, static_cast<int>(evt->type));

      if ((evt->type == EXOSIP_REGISTRATION_FAILURE) && (evt->response != nullptr) &&
          ((evt->response->status_code == 401) || (evt->response->status_code == 407)))
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration failed with status code %03d (will retry)"),
                  uri, proxy, evt->response->status_code);
         eXosip_default_action(ctx, evt);
         eXosip_event_free(evt);
         continue;
      }

      if (evt->type == EXOSIP_REGISTRATION_SUCCESS)
      {
         elapsedTime = static_cast<int32_t>(GetMonotonicClockTime() - startTime);
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration successful"), uri, proxy);
         success = true;
         eXosip_event_free(evt);
         *status = 200;
         break;
      }

      if (evt->type == EXOSIP_REGISTRATION_FAILURE)
      {
         int statusCode = (evt->response != nullptr) ? evt->response->status_code : 408;
         nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): registration failed with status code %03d (will not retry)"), uri, proxy, statusCode);
         eXosip_event_free(evt);
         *status = statusCode;
         break;
      }

      if ((evt->type == EXOSIP_MESSAGE_NEW) && (evt->request != nullptr))
            nxlog_debug_tag(DEBUG_TAG, 8, _T("RegisterSIPClient(%hs via %hs): received %hs message"), uri, proxy, osip_message_get_method(evt->request));
      eXosip_default_action(ctx, evt);
      eXosip_event_free(evt);
   }

   eXosip_free(ctx);
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
   ret_int64(value, RegisterSIPClient(login, password, domain, proxy, 5, &status));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Registration test constructor
 */
SIPRegistrationTest::SIPRegistrationTest(ConfigEntry *config, const char *defaultProxy) : m_mutex(MutexType::FAST)
{
   m_name = MemCopyString(config->getName());
#ifdef UNICODE
   m_login = UTF8StringFromWideString(config->getSubEntryValue(L"Login", 0, L"netxms"));
   m_password = UTF8StringFromWideString(config->getSubEntryValue(L"Password", 0, L"netxms"));
   m_domain = UTF8StringFromWideString(config->getSubEntryValue(L"Domain", 0, L""));
   const WCHAR *proxy = config->getSubEntryValue(L"Proxy", 0, nullptr);
   if (proxy != nullptr)
   {
      if (!wcsncmp(proxy, L"sip:", 4))
      {
         m_proxy = UTF8StringFromWideString(proxy);
      }
      else
      {
         size_t len = wchar_utf8len(proxy, -1) + 5;
         m_proxy = MemAllocStringA(len);
         memcpy(m_proxy, "sip:", 4);
         wchar_to_utf8(proxy, -1, &m_proxy[4], len);
      }
   }
   else
   {
      m_proxy = MemCopyStringA(defaultProxy);
   }
#else
   m_login = MemCopyStringA(config->getSubEntryValue("Login", 0, "netxms"));
   m_password = MemCopyStringA(config->getSubEntryValue("Password", 0, "netxms"));
   m_domain = MemCopyStringA(config->getSubEntryValue("Domain", 0, ""));
   const char *proxy = config->getSubEntryValue("Proxy", 0, nullptr);
   if (proxy != nullptr)
   {
      if (!strncmp(proxy, "sip:", 4))
      {
         m_proxy = MemCopyStringA(proxy);
      }
      else
      {
         m_proxy = MemAllocStringA(strlen(proxy) + 5);
         memcpy(m_proxy, "sip:", 4);
         strcpy(&m_proxy[4], proxy);
      }
   }
   else
   {
      m_proxy = MemCopyStringA(defaultProxy);
   }
#endif
   m_timeout = config->getSubEntryValueAsUInt(_T("Timeout"), 0, 30);
   m_interval = config->getSubEntryValueAsUInt(_T("Interval"), 0, 300) * 1000;
   m_lastRunTime = 0;
   m_elapsedTime = 0;
   m_status = 500;
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
}

/**
 * Run test
 */
void SIPRegistrationTest::run()
{
   if (m_stop)
      return;

   int status;
   int32_t elapsed = RegisterSIPClient(m_login, m_password, m_domain, m_proxy, m_timeout, &status);
   m_mutex.lock();
   m_lastRunTime = time(nullptr);
   m_elapsedTime = elapsed;
   m_status = status;
   m_mutex.unlock();
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
   // First run in 5 seconds, then repeat with configured intervals
   ThreadPoolScheduleRelative(g_asteriskThreadPool, 5, this, &SIPRegistrationTest::run);
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
   value->addColumn(_T("STATUS_TEXT"), DCI_DT_STRING, _T("Status Text"));
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
      value->set(6, CodeToText(t->getStatus(), s_sipStatusCodes));
      value->set(7, t->getElapsedTime());
      value->set(8, static_cast<INT64>(t->getLastRunTime()));
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
   if (test == nullptr)
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
      case 's':
         ret_string(value, CodeToText(test->getStatus(), s_sipStatusCodes));
         break;
      case 'T':
         ret_int64(value, test->getLastRunTime());
         break;
   }

   return SYSINFO_RC_SUCCESS;
}
