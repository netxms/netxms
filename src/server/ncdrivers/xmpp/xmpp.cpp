/*
** NetXMS - Network Management System
** Notification driver for XMPP protocol
** Copyright (C) 2022-2026 Raden Solutions
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
** Configuration (section [XMPP]):
**    Login      - JID (user@domain) or bare user name (domain is then taken from Server). Required.
**    Password   - account password, optionally encrypted with nxencpasswd using Login as key. Required.
**    Server     - XMPP server host to connect to; when empty, server is located via SRV records of JID domain.
**    Port       - server port (default 5222, or 5223 when TLSMode=TLS); used only when Server is set.
**    TLSMode    - STARTTLS (default; STARTTLS is mandatory, connection fails if server does not offer it),
**                 TLS (direct TLS connection), or NONE (plain text, TLS disabled).
**    VerifyPeer - verify server certificate (default yes).
**    CAFile     - path to CA certificate file for server certificate verification.
**
** The driver keeps a persistent XMPP session on a dedicated connection thread (libstrophe is not
** thread-safe, so all libstrophe calls are made from that thread). Channel worker thread hands
** messages over to the connection thread and waits until the message is written to the socket;
** when session is not available, send() reports a retry interval so that the server-side retry
** logic handles the outage.
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <strophe.h>

#define DEBUG_TAG _T("ncd.xmpp")

#define MAX_LOGIN 2047
#define MAX_SERVER_NAME 1023

/**
 * Retry interval (in seconds) reported to the server when message cannot be delivered
 */
#define RETRY_INTERVAL 30

/**
 * Time (in milliseconds) to wait for a message to be accepted by the connection thread and written to the socket
 */
#define SEND_TIMEOUT 60000

/**
 * Time (in milliseconds) to wait for a queued message to be written to the socket
 */
#define FLUSH_TIMEOUT 10000

/**
 * Time (in milliseconds) to wait for graceful stream close on shutdown
 */
#define DISCONNECT_TIMEOUT 3000

/**
 * Reconnect back-off limits (in seconds)
 */
#define MIN_RECONNECT_DELAY 5
#define MAX_RECONNECT_DELAY 300

/**
 * Namespaces
 */
#define NS_VERSION "jabber:iq:version"
#define NS_PING "urn:xmpp:ping"
#define NS_STANZAS "urn:ietf:params:xml:ns:xmpp-stanzas"

static const NCConfigurationTemplate s_config(false, true);

#if !HAVE_XMPP_STANZA_ADD_CHILD_EX

/**
 * Custom implementation of xmpp_stanza_add_child_ex for libstrophe versions before 0.10
 */
static inline int xmpp_stanza_add_child_ex(xmpp_stanza_t *stanza, xmpp_stanza_t *child, int do_clone)
{
   int rc = xmpp_stanza_add_child(stanza, child);
   if (!do_clone)
      xmpp_stanza_release(child);
   return rc;
}

#endif

class XmppDriver;
static void Logger(void *userdata, xmpp_log_level_t level, const char *area, const char *msg);

#if HAVE_XMPP_CONN_SET_CERTFAIL_HANDLER

/**
 * Server certificate verification failure handler. Certificate is always rejected (VerifyPeer=no
 * disables verification altogether), handler only reports details of the offending certificate.
 * libstrophe 0.13.0 crashes in this code path when no handler is installed.
 */
static int CertificateFailureHandler(const xmpp_tlscert_t *cert, const char *errorMessage)
{
   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("XMPP server certificate verification failed (%hs): subject=\"%hs\" issuer=\"%hs\" validity=\"%hs\" - \"%hs\" sha256=%hs"),
      CHECK_NULL_A(errorMessage),
      CHECK_NULL_A(xmpp_tlscert_get_string(cert, XMPP_CERT_SUBJECT)),
      CHECK_NULL_A(xmpp_tlscert_get_string(cert, XMPP_CERT_ISSUER)),
      CHECK_NULL_A(xmpp_tlscert_get_string(cert, XMPP_CERT_NOTBEFORE)),
      CHECK_NULL_A(xmpp_tlscert_get_string(cert, XMPP_CERT_NOTAFTER)),
      CHECK_NULL_A(xmpp_tlscert_get_string(cert, XMPP_CERT_FINGERPRINT_SHA256)));
   return 0;
}

#endif

/**
 * TLS mode
 */
enum class TLSMode
{
   NONE,       // TLS disabled
   STARTTLS,   // Mandatory STARTTLS
   DIRECT      // Direct TLS connection
};

/**
 * Connection state
 */
enum class ConnectionState
{
   DISCONNECTED,
   CONNECTING,
   CONNECTED
};

/**
 * Message handed over from channel worker thread to connection thread
 */
struct SendRequest
{
   char *recipient;
   char *body;
   int result;
   Condition completed;

   SendRequest(const char *_recipient, const char *_body) : completed(true)
   {
      recipient = MemCopyStringA(_recipient);
      body = MemCopyStringA(_body);
      result = RETRY_INTERVAL;
   }

   ~SendRequest()
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
   char m_login[MAX_LOGIN];
   char m_password[MAX_PASSWORD];
   char m_server[MAX_SERVER_NAME];
   char m_caFile[MAX_PATH];
   uint16_t m_port;
   TLSMode m_tlsMode;
   bool m_verifyPeer;

   // Accessed only from connection thread
   xmpp_log_t m_logger;
   xmpp_ctx_t *m_context;
   xmpp_conn_t *m_connection;
   bool m_sessionEstablished;
   bool m_tlsStartFailed;

   volatile ConnectionState m_state;
   volatile bool m_shutdownFlag;
   Condition m_shutdownCondition;
   THREAD m_connectionManagerThread;

   // Channel worker thread places at most one request here (send() calls are serialized by the server)
   Mutex m_requestLock;
   shared_ptr<SendRequest> m_pendingRequest;

   XmppDriver();

   void connectionManager();
   bool runConnection();
   void processPendingRequest();
   void failPendingRequest(int result);
   int sendMessage(const SendRequest& request);
   void waitForShutdown(uint32_t timeout);

   void connectionHandler(xmpp_conn_event_t status, int error, xmpp_stream_error_t *streamError);
   void iqHandler(xmpp_stanza_t *stanza);
   void messageHandler(xmpp_stanza_t *stanza);
   void presenceHandler(xmpp_stanza_t *stanza);

   friend void Logger(void *userdata, xmpp_log_level_t level, const char *area, const char *msg);

   static void ConnectionHandlerCallback(xmpp_conn_t *conn, xmpp_conn_event_t status, int error, xmpp_stream_error_t *streamError, void *userdata)
   {
      static_cast<XmppDriver*>(userdata)->connectionHandler(status, error, streamError);
   }

   static int IqHandlerCallback(xmpp_conn_t *conn, xmpp_stanza_t *stanza, void *userdata)
   {
      static_cast<XmppDriver*>(userdata)->iqHandler(stanza);
      return 1;
   }

   static int MessageHandlerCallback(xmpp_conn_t *conn, xmpp_stanza_t *stanza, void *userdata)
   {
      static_cast<XmppDriver*>(userdata)->messageHandler(stanza);
      return 1;
   }

   static int PresenceHandlerCallback(xmpp_conn_t *conn, xmpp_stanza_t *stanza, void *userdata)
   {
      static_cast<XmppDriver*>(userdata)->presenceHandler(stanza);
      return 1;
   }

public:
   virtual ~XmppDriver();

   virtual int send(const NotificationContext& context) override;
   virtual bool checkHealth() override { return m_state == ConnectionState::CONNECTED; }

   static XmppDriver *createInstance(Config *config);
};

/**
 * libstrophe logger. Library errors (TLS, SASL, socket failures) are written to the server log;
 * library debug output includes raw stream content (and therefore SASL credentials), so it is
 * emitted only at debug level 9. Debug message reporting TLS start failure is used to work around
 * libstrophe 0.13.x bug (see runConnection()).
 */
static void Logger(void *userdata, xmpp_log_level_t level, const char *area, const char *msg)
{
   switch (level)
   {
      case XMPP_LEVEL_ERROR:
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("libstrophe [%hs]: %hs"), area, msg);
         break;
      case XMPP_LEVEL_WARN:
         nxlog_debug_tag(DEBUG_TAG, 4, _T("libstrophe [%hs]: %hs"), area, msg);
         break;
      case XMPP_LEVEL_INFO:
         nxlog_debug_tag(DEBUG_TAG, 6, _T("libstrophe [%hs]: %hs"), area, msg);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 9, _T("libstrophe [%hs]: %hs"), area, msg);
         if (!strcmp(area, "conn") && !strncmp(msg, "Couldn't start TLS", 18))
            static_cast<XmppDriver*>(userdata)->m_tlsStartFailed = true;
         break;
   }
}

/**
 * Constructor
 */
XmppDriver::XmppDriver() : m_shutdownCondition(true), m_requestLock(MutexType::FAST)
{
   m_login[0] = 0;
   m_password[0] = 0;
   m_server[0] = 0;
   m_caFile[0] = 0;
   m_port = 0;
   m_tlsMode = TLSMode::STARTTLS;
   m_verifyPeer = true;
   m_logger.handler = Logger;
   m_logger.userdata = this;
   m_context = nullptr;
   m_connection = nullptr;
   m_sessionEstablished = false;
   m_tlsStartFailed = false;
   m_state = ConnectionState::DISCONNECTED;
   m_shutdownFlag = false;
   m_connectionManagerThread = INVALID_THREAD_HANDLE;
}

/**
 * Destructor - stops connection thread (closing XMPP session gracefully)
 */
XmppDriver::~XmppDriver()
{
   m_shutdownFlag = true;
   m_shutdownCondition.set();
   ThreadJoin(m_connectionManagerThread);
}

/**
 * Create driver instance
 */
XmppDriver *XmppDriver::createInstance(Config *config)
{
   XmppDriver *driver = new XmppDriver();

   TCHAR tlsMode[16] = _T("STARTTLS");
   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("CAFile"), CT_MB_STRING, 0, 0, MAX_PATH, 0, driver->m_caFile },
      { _T("Login"), CT_UTF8_STRING, 0, 0, MAX_LOGIN, 0, driver->m_login },
      { _T("Password"), CT_UTF8_STRING, 0, 0, MAX_PASSWORD, 0, driver->m_password },
      { _T("Port"), CT_WORD, 0, 0, 0, 0, &driver->m_port },
      { _T("Server"), CT_UTF8_STRING, 0, 0, MAX_SERVER_NAME, 0, driver->m_server },
      { _T("TLSMode"), CT_STRING, 0, 0, 16, 0, tlsMode },
      { _T("VerifyPeer"), CT_BOOLEAN, 0, 0, 1, 0, &driver->m_verifyPeer },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
   };
   if (!config->parseTemplate(_T("XMPP"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot parse driver configuration"));
      delete driver;
      return nullptr;
   }

   if (driver->m_login[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Login is not configured"));
      delete driver;
      return nullptr;
   }

   if (driver->m_password[0] != 0)
      DecryptPasswordA(driver->m_login, driver->m_password, driver->m_password, MAX_PASSWORD);

   if (strchr(driver->m_login, '@') == nullptr)
   {
      if (driver->m_server[0] == 0)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Login \"%hs\" is not a full JID and Server is not configured"), driver->m_login);
         delete driver;
         return nullptr;
      }
      if (strlen(driver->m_login) + strlen(driver->m_server) + 1 >= MAX_LOGIN)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Login name too long"));
         delete driver;
         return nullptr;
      }
      strlcat(driver->m_login, "@", MAX_LOGIN);
      strlcat(driver->m_login, driver->m_server, MAX_LOGIN);
   }

   if (!_tcsicmp(tlsMode, _T("STARTTLS")))
   {
      driver->m_tlsMode = TLSMode::STARTTLS;
   }
   else if (!_tcsicmp(tlsMode, _T("TLS")))
   {
      driver->m_tlsMode = TLSMode::DIRECT;
   }
   else if (!_tcsicmp(tlsMode, _T("NONE")))
   {
      driver->m_tlsMode = TLSMode::NONE;
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("TLS is disabled, credentials and messages will be sent in plain text"));
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Invalid TLS mode \"%s\" (valid values are STARTTLS, TLS, NONE)"), tlsMode);
      delete driver;
      return nullptr;
   }

   if (driver->m_port == 0)
      driver->m_port = (driver->m_tlsMode == TLSMode::DIRECT) ? 5223 : 5222;

   nxlog_debug_tag(DEBUG_TAG, 3, _T("XMPP driver initialized: login=%hs server=%hs port=%u tlsMode=%s verifyPeer=%s"),
      driver->m_login, (driver->m_server[0] != 0) ? driver->m_server : "(SRV lookup)", driver->m_port, tlsMode, BooleanToString(driver->m_verifyPeer));

   // Library initialization is process-wide; matching xmpp_shutdown() is intentionally not called
   // because driver module stays loaded for the whole server lifetime
   static Mutex s_initLock(MutexType::FAST);
   static bool s_initialized = false;
   s_initLock.lock();
   if (!s_initialized)
   {
      xmpp_initialize();
      s_initialized = true;
   }
   s_initLock.unlock();

   // Report "connecting" from the start so that messages submitted before the first connection attempt
   // completes are held for delivery instead of being rejected immediately
   driver->m_state = ConnectionState::CONNECTING;
   driver->m_connectionManagerThread = ThreadCreateEx(driver, &XmppDriver::connectionManager);
   return driver;
}

/**
 * Driver send method. Hands message over to connection thread and waits for completion.
 */
int XmppDriver::send(const NotificationContext& context)
{
   if (m_state == ConnectionState::DISCONNECTED)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("XMPP session is not available, message to %hs will be retried"), context.recipient);
      return RETRY_INTERVAL;
   }

   shared_ptr<SendRequest> request = make_shared<SendRequest>(context.recipient, context.body);
   m_requestLock.lock();
   m_pendingRequest = request;
   m_requestLock.unlock();

   if (!request->completed.wait(SEND_TIMEOUT))
   {
      m_requestLock.lock();
      if (m_pendingRequest == request)
         m_pendingRequest.reset();
      m_requestLock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Timeout waiting for XMPP session, message to %hs will be retried"), context.recipient);
      return RETRY_INTERVAL;
   }

   return request->result;
}

/**
 * Connection manager thread: maintains XMPP session with reconnect back-off
 */
void XmppDriver::connectionManager()
{
   m_context = xmpp_ctx_new(nullptr, &m_logger);
   if (m_context == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create libstrophe context"));
      m_state = ConnectionState::DISCONNECTED;
      while (!m_shutdownFlag)
      {
         failPendingRequest(-1);
         m_shutdownCondition.wait(1000);
      }
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("XMPP connection manager started"));

   uint32_t reconnectDelay = MIN_RECONNECT_DELAY;
   while (!m_shutdownFlag)
   {
      if (runConnection())
         reconnectDelay = MIN_RECONNECT_DELAY;

      if (m_shutdownFlag)
         break;

      nxlog_debug_tag(DEBUG_TAG, 4, _T("Next XMPP connection attempt in %u seconds"), reconnectDelay);
      waitForShutdown(reconnectDelay * 1000);
      reconnectDelay = std::min(reconnectDelay * 2, static_cast<uint32_t>(MAX_RECONNECT_DELAY));
   }

   failPendingRequest(-1);
   xmpp_ctx_free(m_context);
   m_context = nullptr;

   nxlog_debug_tag(DEBUG_TAG, 2, _T("XMPP connection manager stopped"));
}

/**
 * Wait for shutdown for given time, failing any message request that arrives meanwhile
 */
void XmppDriver::waitForShutdown(uint32_t timeout)
{
   int64_t deadline = GetCurrentTimeMs() + timeout;
   while (!m_shutdownFlag)
   {
      failPendingRequest(RETRY_INTERVAL);
      int64_t remaining = deadline - GetCurrentTimeMs();
      if (remaining <= 0)
         break;
      m_shutdownCondition.wait(static_cast<uint32_t>(std::min(remaining, static_cast<int64_t>(1000))));
   }
}

/**
 * Run single XMPP session: connect, pump event loop until disconnect or shutdown, release connection.
 * Returns true if session was successfully established at some point.
 */
bool XmppDriver::runConnection()
{
   xmpp_conn_t *conn = xmpp_conn_new(m_context);
   if (conn == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create libstrophe connection object"));
      return false;
   }

   xmpp_conn_set_jid(conn, m_login);
   xmpp_conn_set_pass(conn, m_password);

   long flags = 0;
   switch (m_tlsMode)
   {
      case TLSMode::NONE:
         flags |= XMPP_CONN_FLAG_DISABLE_TLS;
         break;
      case TLSMode::STARTTLS:
         flags |= XMPP_CONN_FLAG_MANDATORY_TLS;
         break;
      case TLSMode::DIRECT:
         flags |= XMPP_CONN_FLAG_LEGACY_SSL;
         break;
   }
   if (!m_verifyPeer)
      flags |= XMPP_CONN_FLAG_TRUST_TLS;
   xmpp_conn_set_flags(conn, flags);

#if HAVE_XMPP_CONN_SET_CERTFAIL_HANDLER
   xmpp_conn_set_certfail_handler(conn, CertificateFailureHandler);
#endif

#if HAVE_XMPP_CONN_SET_CAFILE
   if (m_caFile[0] != 0)
      xmpp_conn_set_cafile(conn, m_caFile);
#else
   if (m_caFile[0] != 0)
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("CAFile is ignored (requires libstrophe 0.11 or later)"));
#endif

   // TCP keepalive so that sessions dropped by NAT or firewall are detected without waiting for next send
#if HAVE_XMPP_CONN_SET_SOCKOPT_CALLBACK
   xmpp_conn_set_sockopt_callback(conn, xmpp_sockopt_cb_keepalive);
#else
   xmpp_conn_set_keepalive(conn, 60, 30);
#endif

   m_sessionEstablished = false;
   m_tlsStartFailed = false;
   m_state = ConnectionState::CONNECTING;
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Connecting to XMPP server %hs"), (m_server[0] != 0) ? m_server : "(SRV lookup)");

   int rc = xmpp_connect_client(conn, (m_server[0] != 0) ? m_server : nullptr, (m_server[0] != 0) ? m_port : 0, ConnectionHandlerCallback, this);
   if (rc != XMPP_EOK)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot initiate connection to XMPP server (libstrophe error %d)"), rc);
      m_state = ConnectionState::DISCONNECTED;
      xmpp_conn_release(conn);
      return false;
   }

   m_connection = conn;
   while (!m_shutdownFlag && (m_state != ConnectionState::DISCONNECTED))
   {
      processPendingRequest();
      xmpp_run_once(m_context, 100);

      // libstrophe 0.13.x leaves TLS I/O interface installed after failed TLS negotiation and crashes
      // on the next write attempt (fixed upstream in 0.14.0), so the connection must be released
      // without running event loop again
      if (m_tlsStartFailed)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Connection to XMPP server failed (TLS negotiation error)"));
         m_state = ConnectionState::DISCONNECTED;   // suppress duplicate report from disconnect callback
         break;
      }
   }

   if (!m_tlsStartFailed && (m_state != ConnectionState::DISCONNECTED))
   {
      // Shutdown requested while session is active - close stream gracefully
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Closing XMPP session"));
      xmpp_disconnect(conn);
      int64_t deadline = GetCurrentTimeMs() + DISCONNECT_TIMEOUT;
      while ((m_state != ConnectionState::DISCONNECTED) && (GetCurrentTimeMs() < deadline))
         xmpp_run_once(m_context, 100);
   }

   m_connection = nullptr;
   xmpp_conn_release(conn);   // forces socket close and DISCONNECT callback if stream was not closed gracefully
   m_state = ConnectionState::DISCONNECTED;
   return m_sessionEstablished;
}

/**
 * Process message request from channel worker thread, if any. Request is left pending
 * while connection is being established so that message is delivered as soon as session is up.
 */
void XmppDriver::processPendingRequest()
{
   if (m_state == ConnectionState::CONNECTING)
      return;

   m_requestLock.lock();
   shared_ptr<SendRequest> request = std::move(m_pendingRequest);
   m_requestLock.unlock();
   if (request == nullptr)
      return;

   request->result = (m_state == ConnectionState::CONNECTED) ? sendMessage(*request) : RETRY_INTERVAL;
   request->completed.set();
}

/**
 * Complete pending message request (if any) with given result without sending
 */
void XmppDriver::failPendingRequest(int result)
{
   m_requestLock.lock();
   shared_ptr<SendRequest> request = std::move(m_pendingRequest);
   m_requestLock.unlock();
   if (request != nullptr)
   {
      request->result = result;
      request->completed.set();
   }
}

/**
 * Send message stanza and wait until it is written to the socket (requires active session)
 */
int XmppDriver::sendMessage(const SendRequest& request)
{
   xmpp_stanza_t *msg = xmpp_stanza_new(m_context);
   xmpp_stanza_set_name(msg, "message");
   xmpp_stanza_set_type(msg, "chat");
   xmpp_stanza_set_attribute(msg, "to", request.recipient);

   char *id = xmpp_uuid_gen(m_context);
   if (id != nullptr)
   {
      xmpp_stanza_set_id(msg, id);
      xmpp_free(m_context, id);
   }

   xmpp_stanza_t *body = xmpp_stanza_new(m_context);
   xmpp_stanza_set_name(body, "body");

   xmpp_stanza_t *text = xmpp_stanza_new(m_context);
   xmpp_stanza_set_text(text, request.body);
   xmpp_stanza_add_child_ex(body, text, FALSE);
   xmpp_stanza_add_child_ex(msg, body, FALSE);

   xmpp_send(m_connection, msg);
   xmpp_stanza_release(msg);

#if HAVE_XMPP_CONN_SEND_QUEUE_LEN
   int64_t deadline = GetCurrentTimeMs() + FLUSH_TIMEOUT;
   while ((m_state == ConnectionState::CONNECTED) && (xmpp_conn_send_queue_len(m_connection) > 0) && (GetCurrentTimeMs() < deadline))
      xmpp_run_once(m_context, 50);

   if (m_state != ConnectionState::CONNECTED)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("XMPP session lost while sending message to %hs, message will be retried"), request.recipient);
      return RETRY_INTERVAL;
   }
   if (xmpp_conn_send_queue_len(m_connection) > 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Timeout writing message to %hs into XMPP stream, message will be retried"), request.recipient);
      return RETRY_INTERVAL;
   }
#else
   xmpp_run_once(m_context, 0);
   if (m_state != ConnectionState::CONNECTED)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("XMPP session lost while sending message to %hs, message will be retried"), request.recipient);
      return RETRY_INTERVAL;
   }
#endif

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Message sent to %hs"), request.recipient);
   return 0;
}

/**
 * Connection event handler
 */
void XmppDriver::connectionHandler(xmpp_conn_event_t status, int error, xmpp_stream_error_t *streamError)
{
   switch (status)
   {
      case XMPP_CONN_CONNECT:
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Connected to XMPP server as %hs"), xmpp_conn_get_bound_jid(m_connection));

         xmpp_handler_add(m_connection, IqHandlerCallback, nullptr, "iq", nullptr, this);
         xmpp_handler_add(m_connection, MessageHandlerCallback, nullptr, "message", nullptr, this);
         xmpp_handler_add(m_connection, PresenceHandlerCallback, nullptr, "presence", nullptr, this);

         // Send initial <presence/> so that we appear online to contacts
         {
            xmpp_stanza_t *presence = xmpp_stanza_new(m_context);
            xmpp_stanza_set_name(presence, "presence");
            xmpp_send(m_connection, presence);
            xmpp_stanza_release(presence);
         }

         m_sessionEstablished = true;
         m_state = ConnectionState::CONNECTED;
         break;
      case XMPP_CONN_RAW_CONNECT:
         break;   // not applicable to client connections
      default:   // XMPP_CONN_DISCONNECT or XMPP_CONN_FAIL
         if (m_state == ConnectionState::DISCONNECTED)
            break;   // already reported (library may report failure and disconnect separately)

         {
            StringBuffer details;
            if (error != 0)
               details.appendFormattedString(_T("; error code %d"), error);
            if (streamError != nullptr)
            {
               if (streamError->text != nullptr)
                  details.appendFormattedString(_T("; stream error: %hs"), streamError->text);
               else
                  details.appendFormattedString(_T("; stream error type %d"), static_cast<int>(streamError->type));
            }
            if (m_shutdownFlag)
               nxlog_debug_tag(DEBUG_TAG, 4, _T("XMPP session closed%s"), details.cstr());
            else if (m_state == ConnectionState::CONNECTING)
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Connection to XMPP server failed%s"), details.cstr());
            else
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Disconnected from XMPP server%s"), details.cstr());
         }
         m_state = ConnectionState::DISCONNECTED;
         break;
   }
}

/**
 * IQ stanza handler. Answers version and ping requests; any other request is rejected
 * with service-unavailable error as required by RFC 6120 section 8.2.3.
 */
void XmppDriver::iqHandler(xmpp_stanza_t *stanza)
{
   const char *type = xmpp_stanza_get_type(stanza);
   if ((type == nullptr) || (strcmp(type, "get") && strcmp(type, "set")))
      return;

   const char *from = xmpp_stanza_get_attribute(stanza, "from");
   const char *id = xmpp_stanza_get_id(stanza);
   xmpp_stanza_t *payload = xmpp_stanza_get_children(stanza);
   const char *ns = (payload != nullptr) ? xmpp_stanza_get_ns(payload) : nullptr;

   xmpp_stanza_t *reply = xmpp_stanza_new(m_context);
   xmpp_stanza_set_name(reply, "iq");
   if (id != nullptr)
      xmpp_stanza_set_id(reply, id);
   if (from != nullptr)
      xmpp_stanza_set_attribute(reply, "to", from);

   if (!strcmp(type, "get") && (ns != nullptr) && !strcmp(ns, NS_VERSION))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Version request from %hs"), CHECK_NULL_A(from));
      xmpp_stanza_set_type(reply, "result");

      xmpp_stanza_t *query = xmpp_stanza_new(m_context);
      xmpp_stanza_set_name(query, "query");
      xmpp_stanza_set_ns(query, NS_VERSION);

      xmpp_stanza_t *name = xmpp_stanza_new(m_context);
      xmpp_stanza_set_name(name, "name");
      xmpp_stanza_t *text = xmpp_stanza_new(m_context);
      xmpp_stanza_set_text(text, "NetXMS Server");
      xmpp_stanza_add_child_ex(name, text, FALSE);
      xmpp_stanza_add_child_ex(query, name, FALSE);

      xmpp_stanza_t *version = xmpp_stanza_new(m_context);
      xmpp_stanza_set_name(version, "version");
      text = xmpp_stanza_new(m_context);
      xmpp_stanza_set_text(text, NETXMS_VERSION_STRING_A);
      xmpp_stanza_add_child_ex(version, text, FALSE);
      xmpp_stanza_add_child_ex(query, version, FALSE);

      xmpp_stanza_add_child_ex(reply, query, FALSE);
   }
   else if (!strcmp(type, "get") && (ns != nullptr) && !strcmp(ns, NS_PING))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Ping from %hs"), CHECK_NULL_A(from));
      xmpp_stanza_set_type(reply, "result");
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Unsupported IQ %hs request from %hs (namespace %hs)"), type, CHECK_NULL_A(from), CHECK_NULL_A(ns));
      xmpp_stanza_set_type(reply, "error");

      xmpp_stanza_t *error = xmpp_stanza_new(m_context);
      xmpp_stanza_set_name(error, "error");
      xmpp_stanza_set_type(error, "cancel");

      xmpp_stanza_t *condition = xmpp_stanza_new(m_context);
      xmpp_stanza_set_name(condition, "service-unavailable");
      xmpp_stanza_set_ns(condition, NS_STANZAS);
      xmpp_stanza_add_child_ex(error, condition, FALSE);

      xmpp_stanza_add_child_ex(reply, error, FALSE);
   }

   xmpp_send(m_connection, reply);
   xmpp_stanza_release(reply);
}

/**
 * Incoming message handler
 */
void XmppDriver::messageHandler(xmpp_stanza_t *stanza)
{
   const char *type = xmpp_stanza_get_type(stanza);
   if ((type != nullptr) && !strcmp(type, "error"))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Error message received from %hs"), CHECK_NULL_A(xmpp_stanza_get_attribute(stanza, "from")));
      return;
   }

   xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, "body");
   if (body == nullptr)
      return;

   char *text = xmpp_stanza_get_text(body);
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Incoming message from %hs: %hs"), CHECK_NULL_A(xmpp_stanza_get_attribute(stanza, "from")), CHECK_NULL_A(text));
   xmpp_free(m_context, text);
}

/**
 * Presence handler. Subscription requests are accepted automatically so that any user
 * of the XMPP server can add notification bot to the roster and see its presence.
 */
void XmppDriver::presenceHandler(xmpp_stanza_t *stanza)
{
   const char *type = xmpp_stanza_get_type(stanza);
   if ((type == nullptr) || strcmp(type, "subscribe"))
      return;

   const char *requestor = xmpp_stanza_get_attribute(stanza, "from");
   if (requestor == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Presence subscription request from %hs accepted"), requestor);

   xmpp_stanza_t *reply = xmpp_stanza_new(m_context);
   xmpp_stanza_set_name(reply, "presence");
   xmpp_stanza_set_attribute(reply, "to", requestor);
   xmpp_stanza_set_type(reply, "subscribed");
   xmpp_send(m_connection, reply);
   xmpp_stanza_release(reply);
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(XMPP, &s_config)
{
   return XmppDriver::createInstance(config);
}
