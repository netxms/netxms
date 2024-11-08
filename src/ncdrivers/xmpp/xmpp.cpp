/*
** NetXMS - Network Management System
** Notification driver for XMPP protocol
** Copyright (C) 2022-2024 Raden Solutions
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
** File: xmpp.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <strophe.h>
#include <nxqueue.h>

#define DEBUG_TAG _T("ncd.xmpp")

#define MAX_LOGIN 2047
#define MAX_SERVER_NAME 1023

static const NCConfigurationTemplate s_config(false, true);

#if !HAVE_XMPP_STANZA_ADD_CHILD_EX

/**
 * Custom implementation of xmpp_stanza_add_child_ex for older libstrophe versions
 */
static inline int xmpp_stanza_add_child_ex(xmpp_stanza_t *stanza, xmpp_stanza_t *child, int do_clone)
{
   int rc = xmpp_stanza_add_child(stanza, child);
   if (!do_clone)
      xmpp_stanza_release(child);
   return rc;
}

#endif

/**
 * Logger
 */
static void Logger(void* const userdata, const xmpp_log_level_t level, const char* const area, const char* const msg)
{
   switch (level)
   {
      case XMPP_LEVEL_ERROR:
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%hs %hs"), area, msg);
         break;
      case XMPP_LEVEL_WARN:
         nxlog_debug_tag(DEBUG_TAG, 6, _T("%hs %hs"), area, msg);
         break;
      case XMPP_LEVEL_INFO:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("%hs %hs"), area, msg);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 6, _T("%hs"), msg);
         break;
   }
}

static const xmpp_log_t s_logger = { Logger, nullptr };

struct MessageInfo
{
   char* recipient;
   char* body;

   MessageInfo(const TCHAR* _recipient, const TCHAR* _body)
   {
      recipient = UTF8StringFromWideString(_recipient);
      body = UTF8StringFromWideString(_body);
   }

   ~MessageInfo()
   {
      MemFree(recipient);
      MemFree(body);
   }
};

/**
 * XMPP driver class
 */
class XmppDriver : public NCDriver
{
private:
   bool m_connected;
   THREAD m_connectionManagerThread;

   char m_login[MAX_LOGIN];
   char m_password[MAX_PASSWORD];
   char m_server[MAX_SERVER_NAME];
   unsigned short m_port;

   bool m_shutdownFlag;

   ObjectQueue<MessageInfo> m_sendQueue;

   XmppDriver();
   void connectionManager();
   static void runConnectionHandler(xmpp_conn_t* const conn, const xmpp_conn_event_t status,
                                    const int error, xmpp_stream_error_t* const stream_error,
                                    void* const userdata)
   {
      auto container = static_cast<std::pair<XmppDriver*, xmpp_ctx_t*>*>(userdata);
      container->first->connectionHandler(conn, status, error, stream_error, container->second);
   }
   void connectionHandler(xmpp_conn_t* const conn, const xmpp_conn_event_t status,
                          const int error, xmpp_stream_error_t* const stream_error,
                          xmpp_ctx_t* ctx);

   void checkSendQueue(xmpp_ctx_t* context, xmpp_conn_t* connection);

public:
   ~XmppDriver();

   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;

   static XmppDriver* createInstance(Config* config);
};

/**
 * Create driver instance
 */
XmppDriver::XmppDriver()
   : m_sendQueue(16, Ownership::True)
{
   m_connected = false;
   m_connectionManagerThread = INVALID_THREAD_HANDLE;

   m_port = 0;

   m_shutdownFlag = false;
}

/**
 * Stop XMPP connector
 */
XmppDriver::~XmppDriver()
{
   m_shutdownFlag = true;
   ThreadJoin(m_connectionManagerThread);
}

XmppDriver* XmppDriver::createInstance(Config* config)
{
   XmppDriver* driver = new XmppDriver();

#ifdef UNICODE
   wchar_to_utf8(config->getValue(_T("/XMPP/Login"), _T("netxms@localhost")), -1, driver->m_login, MAX_LOGIN);
   wchar_to_utf8(config->getValue(_T("/XMPP/Password"), _T("netxms")), -1, driver->m_password, MAX_PASSWORD);
   wchar_to_utf8(config->getValue(_T("/XMPP/Server"), _T("")), -1, driver->m_server, MAX_SERVER_NAME);
#else
   mb_to_utf8(config->getValue(_T("/XMPP/Login"), _T("netxms@localhost")), -1, driver->m_login, MAX_LOGIN);
   mb_to_utf8(config->getValue(_T("/XMPP/Password"), _T("netxms")), -1, driver->m_password, MAX_PASSWORD);
   mb_to_utf8(config->getValue(_T("/XMPP/Server"), _T("")), -1, driver->m_server, MAX_SERVER_NAME);
#endif

   if (strchr(driver->m_login, '@') == nullptr)
   {
      if (strlen(driver->m_server) + strlen(driver->m_login) + 1 >= MAX_LOGIN)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Failed to create XMPP notification driver: login name too long"));
         return nullptr;
      }
      strlcat(driver->m_login, "@", MAX_LOGIN - strlen(driver->m_login));
      strlcat(driver->m_login, driver->m_server, MAX_LOGIN - strlen(driver->m_login));
   }

   driver->m_port = config->getValueAsUInt(_T("/XMPP/Port"), 5222);

   driver->m_connectionManagerThread = ThreadCreateEx(driver, &XmppDriver::connectionManager);
   return driver;
}

/**
 * Driver send method
 */
int XmppDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   m_sendQueue.put(new MessageInfo(recipient, body));
   return 0;
}

/**
 * Version request handler
 */
static int VersionHandler(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
   xmpp_ctx_t* ctx = static_cast<xmpp_ctx_t*>(userdata);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Received version request from %hs"), xmpp_stanza_get_attribute(stanza, "from"));

   xmpp_stanza_t* reply = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(reply, "iq");
   xmpp_stanza_set_type(reply, "result");
   xmpp_stanza_set_id(reply, xmpp_stanza_get_id(stanza));
   xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

   xmpp_stanza_t* query = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(query, "query");
   const char* ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
   if (ns != nullptr)
   {
      xmpp_stanza_set_ns(query, ns);
   }

   xmpp_stanza_t* name = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(name, "name");
   xmpp_stanza_add_child_ex(query, name, FALSE);

   xmpp_stanza_t* text = xmpp_stanza_new(ctx);
   xmpp_stanza_set_text(text, "NetXMS Server");
   xmpp_stanza_add_child_ex(name, text, FALSE);

   xmpp_stanza_t* version = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(version, "version");
   xmpp_stanza_add_child_ex(query, version, FALSE);

   text = xmpp_stanza_new(ctx);
   xmpp_stanza_set_text(text, NETXMS_VERSION_STRING_A);
   xmpp_stanza_add_child_ex(version, text, FALSE);

   xmpp_stanza_add_child_ex(reply, query, FALSE);

   xmpp_send(conn, reply);
   xmpp_stanza_release(reply);
   return 1;
}

/**
 * Incoming message handler
 */
static int MessageHandler(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
   xmpp_ctx_t* ctx = (xmpp_ctx_t*)userdata;

   if (xmpp_stanza_get_child_by_name(stanza, "body") == nullptr)
      return 1;
   const char* type = xmpp_stanza_get_attribute(stanza, "type");
   if ((type != nullptr) && !strcmp(type, "error"))
      return 1;

   char* intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Incoming message from %hs: %hs"), xmpp_stanza_get_attribute(stanza, "from"), intext);

   xmpp_free(ctx, intext);
   return 1;
}

/**
 * Presence handler
 */
static int PresenceHandler(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
   xmpp_ctx_t* ctx = (xmpp_ctx_t*)userdata;

   const char* type = xmpp_stanza_get_attribute(stanza, "type");
   if ((type == nullptr) || strcmp(type, "subscribe"))
      return 1;

   const char* requestor = xmpp_stanza_get_attribute(stanza, "from");
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Presence subscribe request from %hs"), requestor);

   xmpp_stanza_t* reply = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(reply, "presence");
   xmpp_stanza_set_attribute(reply, "to", requestor);
   xmpp_stanza_set_attribute(reply, "type", "subscribed");

   xmpp_send(conn, reply);
   xmpp_stanza_release(reply);
   return 1;
}

/**
 * Connection handler
 */
void XmppDriver::connectionHandler(xmpp_conn_t* const conn, const xmpp_conn_event_t status,
                                   const int error, xmpp_stream_error_t* const stream_error,
                                   xmpp_ctx_t* ctx)
{
   if (status == XMPP_CONN_CONNECT)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Connected to XMPP server"));

      xmpp_handler_add(conn, VersionHandler, "jabber:iq:version", "iq", nullptr, ctx);
      xmpp_handler_add(conn, MessageHandler, nullptr, "message", nullptr, ctx);
      xmpp_handler_add(conn, PresenceHandler, nullptr, "presence", nullptr, ctx);

      // Send initial <presence/> so that we appear online to contacts
      xmpp_stanza_t* presence = xmpp_stanza_new(ctx);
      xmpp_stanza_set_name(presence, "presence");
      xmpp_send(conn, presence);
      xmpp_stanza_release(presence);

      m_connected = true;
   }
   else
   {
      m_connected = false;
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Disconnected from XMPP server"));
      xmpp_stop(ctx);
   }
}

/**
 * Driver send method
 */
void XmppDriver::checkSendQueue(xmpp_ctx_t* context, xmpp_conn_t* connection)
{
   if (!m_connected)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("XMPP connection unavailable"));
      return;
   }

   MessageInfo* mi = m_sendQueue.get();
   while (mi != nullptr)
   {
      xmpp_stanza_t* msg = xmpp_stanza_new(context);
      xmpp_stanza_set_name(msg, "message");
      xmpp_stanza_set_type(msg, "chat");

      xmpp_stanza_set_attribute(msg, "to", mi->recipient);

      xmpp_stanza_t* msgBody = xmpp_stanza_new(context);
      xmpp_stanza_set_name(msgBody, "body");

      xmpp_stanza_t* text = xmpp_stanza_new(context);
      xmpp_stanza_set_text(text, mi->body);
      xmpp_stanza_add_child_ex(msgBody, text, FALSE);
      xmpp_stanza_add_child_ex(msg, msgBody, FALSE);

      xmpp_send(connection, msg);

      xmpp_stanza_release(msg);

      delete mi;
      mi = m_sendQueue.get();
   }
}

/**
 * XMPP thread
 */
void XmppDriver::connectionManager()
{
   xmpp_ctx_t* context = xmpp_ctx_new(nullptr, &s_logger);

   if (context != nullptr)
   {
      // outer loop - try to reconnect after disconnect
      time_t checkpoint = time(nullptr) - 30;
      do
      {
         if (time(nullptr) - checkpoint >= 30)
         {
            checkpoint = time(nullptr);
            xmpp_conn_t* connection = xmpp_conn_new(context);
            if (connection != nullptr)
            {
               xmpp_conn_set_jid(connection, m_login);
               xmpp_conn_set_pass(connection, m_password);
               std::pair<XmppDriver*, xmpp_ctx_t*> container(this, context);
               xmpp_connect_client(connection, (m_server[0] != 0) ? m_server : nullptr, (m_server[0] != 0) ? m_port : 0, runConnectionHandler, &container);

               while (m_connected)
               {
                  checkSendQueue(context, connection);
                  xmpp_run_once(context, 100);
               }
               xmpp_disconnect(connection);
               xmpp_conn_release(connection);
               nxlog_debug_tag(DEBUG_TAG, 6, _T("XMPP connection released"));
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("XMPP connection unavailable"));
            }
         }
         ThreadSleepMs(500);
      }
      while (!m_shutdownFlag);

      xmpp_ctx_free(context);
   }
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(XMPP, &s_config)
{
   return XmppDriver::createInstance(config);
}
