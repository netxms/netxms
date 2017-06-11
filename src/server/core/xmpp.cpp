/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

#include "nxcore.h"

#if XMPP_SUPPORTED

#include <strophe.h>

/**
 * Logger
 */
static void Logger(void * const userdata, const xmpp_log_level_t level, const char * const area, const char * const msg)
{
   switch(level)
   {
      case XMPP_LEVEL_ERROR:
         nxlog_write(MSG_XMPP_ERROR, NXLOG_ERROR, "mm", area, msg);
         break;
      case XMPP_LEVEL_WARN:
         nxlog_write(MSG_XMPP_WARNING, NXLOG_WARNING, "mm", area, msg);
         break;
      case XMPP_LEVEL_INFO:
         nxlog_write(MSG_XMPP_INFO, NXLOG_INFO, "mm", area, msg);
         break;
      default:
         DbgPrintf(6, _T("XMPP: %hs"), msg);
         break;
   }
}

/**
 * Logger definition
 */
static const xmpp_log_t s_logger = { Logger, NULL };

/**
 * Version request handler
 */
static int VersionHandler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
   xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
   DbgPrintf(5, _T("XMPP: Received version request from %hs"), xmpp_stanza_get_attribute(stanza, "from"));

   xmpp_stanza_t *reply = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(reply, "iq");
   xmpp_stanza_set_type(reply, "result");
   xmpp_stanza_set_id(reply, xmpp_stanza_get_id(stanza));
   xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

   xmpp_stanza_t *query = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(query, "query");
   const char *ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
   if (ns != NULL)
   {
      xmpp_stanza_set_ns(query, ns);
   }

   xmpp_stanza_t *name = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(name, "name");
   xmpp_stanza_add_child_ex(query, name, FALSE);

   xmpp_stanza_t *text = xmpp_stanza_new(ctx);
   xmpp_stanza_set_text(text, "NetXMS Server");
   xmpp_stanza_add_child_ex(name, text, FALSE);

   xmpp_stanza_t *version = xmpp_stanza_new(ctx);
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
 * Presence handler
 */
static int PresenceHandler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

   const char *type = xmpp_stanza_get_attribute(stanza, "type");
	if ((type == NULL) || strcmp(type, "subscribe"))
      return 1;

   const char *requestor = xmpp_stanza_get_attribute(stanza, "from");
   DbgPrintf(4, _T("XMPP: presence subscribe request from %hs"), requestor);

   xmpp_stanza_t *reply = xmpp_stanza_new(ctx);
   xmpp_stanza_set_name(reply, "presence");
	xmpp_stanza_set_attribute(reply, "to", requestor);
   if (AuthenticateUserForXMPPSubscription(requestor))
   {
	   xmpp_stanza_set_attribute(reply, "type", "subscribed");
   }
   else
   {
	   xmpp_stanza_set_attribute(reply, "type", "unsubscribed");
   }

   xmpp_send(conn, reply);
   xmpp_stanza_release(reply);
   return 1;
}

/**
 * Incoming message handler
 */
static int MessageHandler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

	if (!xmpp_stanza_get_child_by_name(stanza, "body"))
      return 1;
   const char *type = xmpp_stanza_get_attribute(stanza, "type");
	if ((type != NULL) && !strcmp(type, "error"))
      return 1;

   const char *requestor = xmpp_stanza_get_attribute(stanza, "from");
	char *intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));
   DbgPrintf(6, _T("XMPP: Incoming message from %hs: %hs"), requestor, intext);

   if (AuthenticateUserForXMPPCommands(requestor))
   {
#ifdef UNICODE
      WCHAR *cmd = WideStringFromUTF8String(intext);
#else
      char *cmd = strdup(intext);
#endif
      TCHAR *eol = _tcschr(cmd, _T('\n'));
      if (eol != NULL)
         *eol = 0;

      struct __console_ctx console;
      console.hSocket = -1;
	   console.socketMutex = MutexCreate();
      console.pMsg = NULL;
	   console.session = NULL;
      console.output = new String();
      ProcessConsoleCommand(cmd, &console);
      free(cmd);
      MutexDestroy(console.socketMutex);

      if (!console.output->isEmpty())
      {
	      xmpp_stanza_t *reply = xmpp_stanza_new(ctx);
	      xmpp_stanza_set_name(reply, "message");
	      xmpp_stanza_set_type(reply, (xmpp_stanza_get_type(stanza) != NULL) ? xmpp_stanza_get_type(stanza) : "chat");
	      xmpp_stanza_set_attribute(reply, "to", requestor);

	      xmpp_stanza_t *body = xmpp_stanza_new(ctx);
	      xmpp_stanza_set_name(body, "body");

	      xmpp_stanza_t *text = xmpp_stanza_new(ctx);
         char *response = console.output->getUTF8String();
	      xmpp_stanza_set_text(text, response);
         free(response);
	      xmpp_stanza_add_child_ex(body, text, FALSE);
	      xmpp_stanza_add_child_ex(reply, body, FALSE);

	      xmpp_send(conn, reply);
	      xmpp_stanza_release(reply);
      }
      delete console.output;
   }
   else
   {
      DbgPrintf(6, _T("XMPP: %hs is not authorized for XMPP commands"), requestor);
   }
   xmpp_free(ctx, intext);
	return 1;
}

/**
 * Connection status
 */
static bool s_xmppConnected = false;

/**
 * Connection handler
 */
static void ConnectionHandler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
                              const int error, xmpp_stream_error_t * const stream_error,
                              void * const userdata)
{
   xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

   if (status == XMPP_CONN_CONNECT)
   {
      DbgPrintf(3, _T("XMPP: connected"));

      xmpp_handler_add(conn, VersionHandler, "jabber:iq:version", "iq", NULL, ctx);
      xmpp_handler_add(conn, MessageHandler, NULL, "message", NULL, ctx);
      xmpp_handler_add(conn, PresenceHandler, NULL, "presence", NULL, ctx);

      // Send initial <presence/> so that we appear online to contacts
      xmpp_stanza_t *presence = xmpp_stanza_new(ctx);
      xmpp_stanza_set_name(presence, "presence");
      xmpp_send(conn, presence);
      xmpp_stanza_release(presence);

      s_xmppConnected = true;
   }
   else
   {
      s_xmppConnected = false;
      DbgPrintf(3, _T("XMPP: disconnected"));
      xmpp_stop(ctx);
   }
}

/**
 * XMPP context
 */
static xmpp_ctx_t *s_xmppContext = NULL;
static xmpp_conn_t *s_xmppConnection = NULL;
static MUTEX s_xmppMutex = MutexCreate();

/**
 * XMPP thread
 */
static THREAD_RESULT THREAD_CALL XMPPConnectionManager(void *arg)
{
   xmpp_initialize();

   s_xmppContext = xmpp_ctx_new(NULL, &s_logger);

   TCHAR tmpLogin[64];
   TCHAR tmpPassword[MAX_PASSWORD];

   ConfigReadStr(_T("XMPPLogin"), tmpLogin, 64, _T("netxms@localhost"));
   ConfigReadStr(_T("XMPPPassword"), tmpPassword, MAX_PASSWORD, _T("netxms"));
   DecryptPassword(tmpLogin, tmpPassword, tmpPassword, MAX_PASSWORD);

   char login[64], password[MAX_PASSWORD];
#ifdef UNICODE
   ConfigReadStrA(_T("XMPPLogin"), login, 64, "netxms@localhost");
   char *_tmpPassword = UTF8StringFromWideString(tmpPassword);
   strncpy(password, _tmpPassword, MAX_PASSWORD);
   safe_free(_tmpPassword);
#else
   strncpy(password, tmpPassword, MAX_PASSWORD);
   strncpy(login, tmpPassword, 64);
#endif // UNICODE
   DbgPrintf(1, _T("XMPP connection manager started"));

   // outer loop - try to reconnect after disconnect
   do
   {
      MutexLock(s_xmppMutex);
      s_xmppConnection = xmpp_conn_new(s_xmppContext);
      xmpp_conn_set_jid(s_xmppConnection, login);
      xmpp_conn_set_pass(s_xmppConnection, password);
      xmpp_connect_client(s_xmppConnection, NULL, 0, ConnectionHandler, s_xmppContext);
      MutexUnlock(s_xmppMutex);

      xmpp_set_loop_status(s_xmppContext, XMPP_LOOP_RUNNING);
      while(xmpp_get_loop_status(s_xmppContext) == XMPP_LOOP_RUNNING)
      {
         MutexLock(s_xmppMutex);
         xmpp_run_once(s_xmppContext, 100);
         MutexUnlock(s_xmppMutex);
      }
      MutexLock(s_xmppMutex);
      xmpp_conn_release(s_xmppConnection);
      s_xmppConnection = NULL;
      MutexUnlock(s_xmppMutex);
   } while(!SleepAndCheckForShutdown(30));

   xmpp_ctx_free(s_xmppContext);
   s_xmppContext = NULL;

   xmpp_shutdown();
   DbgPrintf(1, _T("XMPP connection manager stopped"));
   return THREAD_OK;
}

/**
 * Outgoing message queue
 */
static Queue s_xmppMessageQueue;

/**
 * Message to send
 */
class XMPPMessage
{
private:
   char *m_rcpt;
   char *m_text;
   time_t m_timestamp;

public:

   XMPPMessage(const TCHAR *rcpt, const TCHAR *text)
   {
#ifdef UNICODE
      m_rcpt = UTF8StringFromWideString(rcpt);
      m_text = UTF8StringFromWideString(text);
#else
      m_rcpt = UTF8StringFromMBString(rcpt);
      m_text = UTF8StringFromMBString(text);
#endif
      m_timestamp = time(NULL);
   }

   ~XMPPMessage()
   {
      free(m_rcpt);
      free(m_text);
   }

   const char *getRecipient() { return m_rcpt; }
   const char *getText() { return m_text; }
   time_t getAge() { return time(NULL) - m_timestamp; }
};

/**
 * Message sender
 */
static THREAD_RESULT THREAD_CALL XMPPMessageSender(void *arg)
{
   DbgPrintf(1, _T("XMPP message sender started"));

   while(true)
   {
      XMPPMessage *m = (XMPPMessage *)s_xmppMessageQueue.getOrBlock();
      if (m == INVALID_POINTER_VALUE)
         break;

      MutexLock(s_xmppMutex);
      if ((s_xmppContext == NULL) || (s_xmppConnection == NULL) || !s_xmppConnected)
      {
         MutexUnlock(s_xmppMutex);
         if (m->getAge() < 3600)
         {
            s_xmppMessageQueue.insert(m);
            DbgPrintf(6, _T("XMPPMessageSender: XMPP connection unavailable, will retry in 30 seconds"));
            if (SleepAndCheckForShutdown(30))   // retry message sending in 30 seconds
               break;
         }
         else
         {
            DbgPrintf(6, _T("XMPPMessageSender: XMPP connection unavailable, dropping undelivered message to %hs"), m->getRecipient());
            delete m;
         }
         continue;
      }

      xmpp_stanza_t *msg = xmpp_stanza_new(s_xmppContext);
      xmpp_stanza_set_name(msg, "message");
      xmpp_stanza_set_type(msg, "chat");
      xmpp_stanza_set_attribute(msg, "to", m->getRecipient());

      xmpp_stanza_t *body = xmpp_stanza_new(s_xmppContext);
      xmpp_stanza_set_name(body, "body");

      xmpp_stanza_t *text = xmpp_stanza_new(s_xmppContext);
      xmpp_stanza_set_text(text, m->getText());
      xmpp_stanza_add_child_ex(body, text, FALSE);
      xmpp_stanza_add_child_ex(msg, body, FALSE);

      xmpp_send(s_xmppConnection, msg);
      xmpp_stanza_release(msg);

      MutexUnlock(s_xmppMutex);
      delete m;
   }

   DbgPrintf(1, _T("XMPP message sender stopped"));
   return THREAD_OK;
}

/**
 * XMPP threads
 */
static THREAD s_connManagerThread = INVALID_THREAD_HANDLE;
static THREAD s_msgSenderThread = INVALID_THREAD_HANDLE;

/**
 * Start XMPP connector
 */
void StartXMPPConnector()
{
   s_connManagerThread = ThreadCreateEx(XMPPConnectionManager, 0, NULL);
   s_msgSenderThread = ThreadCreateEx(XMPPMessageSender, 0, NULL);
}

/**
 * Stop XMPP connector
 */
void StopXMPPConnector()
{
   XMPPMessage *m;
   while((m = (XMPPMessage *)s_xmppMessageQueue.get()) != NULL)
      delete m;
   s_xmppMessageQueue.put(INVALID_POINTER_VALUE);

   MutexLock(s_xmppMutex);
   if (s_xmppContext != NULL)
   {
      if (s_xmppConnected)
         xmpp_disconnect(s_xmppConnection);
      else
         xmpp_stop(s_xmppContext);
   }
   MutexUnlock(s_xmppMutex);

   ThreadJoin(s_connManagerThread);
   ThreadJoin(s_msgSenderThread);
}

/**
 * Send message to XMPP recipient
 */
void SendXMPPMessage(const TCHAR *rcpt, const TCHAR *text)
{
   s_xmppMessageQueue.put(new XMPPMessage(rcpt, text));
}

#endif
