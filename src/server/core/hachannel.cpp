/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: hachannel.cpp
**
** HA cluster peer channel (doc/HA_Design.md sections 3.3, 5.3). Purpose-built
** TLS link between the two cluster nodes carrying change notifications,
** watermark acknowledgements and the handover notification. The channel is
** pure acceleration: the standby's applier reads committed journal rows from
** the database, so a lost or delayed channel message costs latency, never
** correctness (the periodic catch-up and the activation replay cover it).
**
** Authentication: mutual HMAC-SHA256 challenge-response over TLS, keyed with
** a cluster secret stored in the shared database (metadata entry) - access
** to the database is the cluster's actual trust anchor.
**
**/

#include "nxcore.h"
#include <nxcore_ha.h>
#include <socket_listener.h>
#include <agent_tunnel.h>

#define DEBUG_TAG L"ha.channel"

#define MAX_CHANNEL_MSG_SIZE  4194304

/**
 * Channel message codes (private namespace - these never leave the cluster
 * interconnect and are not part of the client-facing NXCP command space)
 */
#define HA_CMD_HELLO          0x1001   // VID_GUID, VID_SERVER_ID, VID_CHALLENGE
#define HA_CMD_AUTH           0x1002   // VID_SIGNATURE = HMAC(secret, peer challenge)
#define HA_CMD_KEEPALIVE      0x1003
#define HA_CMD_CHANGE_NOTIFY  0x1004   // VID_SEQUENCE_NUMBER = journal head hint
#define HA_CMD_WATERMARK      0x1005   // VID_SEQUENCE_NUMBER = peer applied watermark
#define HA_CMD_DEMOTED        0x1006   // active is releasing the lease (handover)
#define HA_CMD_DCI_DATA       0x1007   // VID_NUM_ELEMENTS + repeating groups (data collection feed)

/**
 * Data collection feed batching limits
 */
#define DATA_FEED_BATCH_SIZE     1000  // values per message
#define DATA_FEED_MAX_PENDING    10    // finalized messages queued for the sender thread

/**
 * Data collection feed flags (per-value)
 */
#define DATA_FEED_FLAG_STORED    0x01  // value was written to the history table (goes into the value cache)
#define DATA_FEED_FLAG_ANOMALY   0x02  // anomaly detector flagged this value

/**
 * Cluster secret (HMAC key) storage
 */
#define CLUSTER_SECRET_METADATA_ENTRY  L"HAClusterSecret"
#define CLUSTER_SECRET_LENGTH          32

/**
 * Configuration (set by HAChannelConfigure before start)
 */
static wchar_t s_peerAddress[256] = L"";
static uint16_t s_channelPort = 4704;
static uint16_t s_peerPort = 4704;

/**
 * Cluster secret
 */
static BYTE s_secret[CLUSTER_SECRET_LENGTH];
static bool s_secretLoaded = false;

/**
 * Shutdown flag
 */
static std::atomic<bool> s_shutdown(false);

/**
 * Journal apply handler (registered by the warm state sync layer via
 * HAChannelSetupSync; null = no warm state to update, applying is a no-op)
 */
static std::function<void(const std::vector<HAJournalEntry>&)> s_applyHandler = nullptr;

/**
 * Applier starting watermark: journal head captured before the warm state
 * load began, so every change the load could have missed gets re-applied
 * (application is idempotent). -1 = not set, fall back to the persisted
 * watermark.
 */
static int64_t s_initialWatermark = -1;

/**
 * Contiguous applied watermark of this node (exposed for status display and
 * for the activation-time final replay)
 */
static std::atomic<int64_t> s_appliedWatermark(0);

/**
 * Set when the applier thread must stop before the activation-time final
 * replay takes over journal application
 */
static std::atomic<bool> s_applierQuiesce(false);

/**
 * Journal head hint received from the active node (0 = no hint)
 */
static std::atomic<int64_t> s_headHint(0);

/**
 * Watermark reported by the peer (active side; used by graceful switchover)
 */
static std::atomic<int64_t> s_peerWatermark(0);

/**
 * Applier wakeup
 */
static Condition s_applierWakeup(false);

/**
 * Data collection feed state. The feed is fire-and-forget: values are batched
 * into an NXCP message under a fast lock; full batches queue for the sender
 * thread, which also flushes the current partial batch once per second. Any
 * form of backpressure (no authenticated peer, pending queue full) drops
 * values - the feed is pure cache pre-warming, never correctness.
 */
static bool s_dataFeedEnabled = false;               // [CLUSTER] EnableDataCollectionFeed
static std::atomic<bool> s_dataFeedActive(false);    // enabled && this node active && peer connected (maintained by sender thread)
static Mutex s_dataFeedLock(MutexType::FAST);
static NXCPMessage *s_dataFeedMessage = nullptr;     // current partial batch
static uint32_t s_dataFeedCount = 0;                 // values in current partial batch
static uint32_t s_dataFeedFieldId = 0;
static std::vector<NXCPMessage*> s_dataFeedPending;  // finalized batches awaiting the sender thread
static std::atomic<uint64_t> s_dataFeedValuesSent(0);
static std::atomic<uint64_t> s_dataFeedValuesDropped(0);
static std::atomic<uint64_t> s_dataFeedValuesApplied(0);
static std::atomic<uint64_t> s_dataFeedValuesDiscarded(0);

/**
 * Load (or bootstrap) the cluster secret from database metadata. If two nodes
 * bootstrap concurrently the last writer wins; the loser fails authentication
 * once and re-reads on the next connection attempt.
 */
static bool LoadClusterSecret()
{
   wchar_t buffer[CLUSTER_SECRET_LENGTH * 2 + 1];
   if (!MetaDataReadStr(CLUSTER_SECRET_METADATA_ENTRY, buffer, CLUSTER_SECRET_LENGTH * 2 + 1, L"") || (buffer[0] == 0))
   {
      BYTE secret[CLUSTER_SECRET_LENGTH];
      GenerateRandomBytes(secret, CLUSTER_SECRET_LENGTH);
      wchar_t text[CLUSTER_SECRET_LENGTH * 2 + 1];
      BinToStr(secret, CLUSTER_SECRET_LENGTH, text);
      MetaDataWriteStr(CLUSTER_SECRET_METADATA_ENTRY, text);
      if (!MetaDataReadStr(CLUSTER_SECRET_METADATA_ENTRY, buffer, CLUSTER_SECRET_LENGTH * 2 + 1, L"") || (buffer[0] == 0))
         return false;
      nxlog_debug_tag(DEBUG_TAG, 2, L"Cluster secret created");
   }
   StrToBin(buffer, s_secret, CLUSTER_SECRET_LENGTH);
   s_secretLoaded = true;
   return true;
}

#ifdef _WITH_ENCRYPTION

/**
 * Run TLS handshake with WANT_READ/WANT_WRITE retry on a non-blocking socket
 */
static bool DoTlsHandshake(SSL *ssl, SOCKET s)
{
   SetSocketNonBlocking(s);
   while(true)
   {
      int rc = SSL_do_handshake(ssl);
      if (rc == 1)
         return true;
      int sslErr = SSL_get_error(ssl, rc);
      if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
      {
         SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
         poller.add(s);
         if (poller.poll(10000) > 0)
            continue;
         nxlog_debug_tag(DEBUG_TAG, 4, L"TLS handshake timeout on peer channel connection");
      }
      else
      {
         char buffer[128];
         nxlog_debug_tag(DEBUG_TAG, 4, L"TLS handshake failed on peer channel connection (%hs)", ERR_error_string(ERR_get_error(), buffer));
         LogOpenSSLErrorStack(6);
      }
      return false;
   }
}

/**
 * Channel session - one authenticated TLS link to the peer. At most one
 * inbound (accepted) and one outbound (dialed) session exist at any time.
 */
class HAChannelSession
{
private:
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   Mutex m_sslLock;
   Mutex m_writeLock;
   bool m_outbound;
   std::atomic<bool> m_authenticated;
   std::atomic<bool> m_stopped;
   BYTE m_challenge[CLUSTER_SECRET_LENGTH];
   time_t m_lastMessageTime;
   THREAD m_thread;

   void readLoop();
   void processMessage(NXCPMessage *msg);

public:
   HAChannelSession(SOCKET socket, SSL_CTX *context, SSL *ssl, bool outbound);
   ~HAChannelSession();

   void start();
   void stop();

   bool sendMessage(const NXCPMessage& msg);
   bool isAuthenticated() const { return m_authenticated.load(); }
   bool isStopped() const { return m_stopped.load(); }
};

/**
 * Session slots
 */
static Mutex s_sessionLock(MutexType::FAST);
static HAChannelSession *s_inboundSession = nullptr;
static HAChannelSession *s_outboundSession = nullptr;

/**
 * Create session
 */
HAChannelSession::HAChannelSession(SOCKET socket, SSL_CTX *context, SSL *ssl, bool outbound) : m_sslLock(MutexType::FAST), m_writeLock(MutexType::FAST)
{
   m_socket = socket;
   m_context = context;
   m_ssl = ssl;
   m_outbound = outbound;
   m_authenticated = false;
   m_stopped = false;
   m_lastMessageTime = time(nullptr);
   m_thread = INVALID_THREAD_HANDLE;
   GenerateRandomBytes(m_challenge, CLUSTER_SECRET_LENGTH);
}

/**
 * Destroy session
 */
HAChannelSession::~HAChannelSession()
{
   stop();
   if (m_thread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_thread);
   if (m_ssl != nullptr)
      SSL_free(m_ssl);
   if (m_context != nullptr)
      SSL_CTX_free(m_context);
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
}

/**
 * Start session read thread and send HELLO
 */
void HAChannelSession::start()
{
   m_thread = ThreadCreateEx(this, &HAChannelSession::readLoop);

   NXCPMessage hello(HA_CMD_HELLO, 0, 5);   // protocol version 5
   HALeaseManager *manager = HAGetLeaseManager();
   if (manager != nullptr)
      hello.setField(VID_GUID, manager->getNodeGuid());
   hello.setField(VID_SERVER_ID, g_serverId);
   hello.setField(VID_CHALLENGE, m_challenge, CLUSTER_SECRET_LENGTH);
   sendMessage(hello);
}

/**
 * Stop session
 */
void HAChannelSession::stop()
{
   if (m_stopped.exchange(true))
      return;
   m_authenticated = false;
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
}

/**
 * Send message to peer (non-blocking socket: retry on WANT_READ/WANT_WRITE)
 */
bool HAChannelSession::sendMessage(const NXCPMessage& msg)
{
   if (m_stopped.load())
      return false;

   NXCP_MESSAGE *data = msg.serialize(false);
   size_t size = ntohl(data->size);

   LockGuard writeGuard(m_writeLock);
   int bytes;
   bool canRetry;
   do
   {
      canRetry = false;
      m_sslLock.lock();
      bytes = SSL_write(m_ssl, data, static_cast<int>(size));
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            m_sslLock.unlock();
            SocketPoller poller(err == SSL_ERROR_WANT_WRITE);
            poller.add(m_socket);
            if (poller.poll(10000) > 0)
               canRetry = true;
            m_sslLock.lock();
         }
      }
      m_sslLock.unlock();
   }
   while(canRetry);

   MemFree(data);
   return bytes == static_cast<int>(size);
}

/**
 * Session read loop
 */
void HAChannelSession::readLoop()
{
   ThreadSetName("HAChannel");
   TlsMessageReceiver receiver(m_socket, m_ssl, &m_sslLock, 8192, MAX_CHANNEL_MSG_SIZE);
   while(!m_stopped.load() && !s_shutdown.load())
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(5000, &result);
      if (msg != nullptr)
      {
         m_lastMessageTime = time(nullptr);
         processMessage(msg);
         delete msg;
      }
      else if (result == MSGRECV_TIMEOUT)
      {
         if (time(nullptr) - m_lastMessageTime > 30)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"Peer channel timeout (%s)", m_outbound ? L"outbound" : L"inbound");
            break;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"Peer channel closed (%s, %s)", m_outbound ? L"outbound" : L"inbound",
               AbstractMessageReceiver::resultToText(result));
         break;
      }
   }
   m_authenticated = false;
   m_stopped = true;
}

/**
 * Process message from peer
 */
void HAChannelSession::processMessage(NXCPMessage *msg)
{
   switch(msg->getCode())
   {
      case HA_CMD_HELLO:
      {
         uint64_t peerServerId = msg->getFieldAsUInt64(VID_SERVER_ID);
         if (peerServerId != g_serverId)
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Peer channel rejected: peer belongs to a different cluster (server ID " UINT64X_FMT(L"016") L")", peerServerId);
            stop();
            break;
         }
         HALeaseManager *manager = HAGetLeaseManager();
         if ((manager != nullptr) && msg->getFieldAsGUID(VID_GUID).equals(manager->getNodeGuid()))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Peer channel rejected: connected to self (check PeerAddress configuration)");
            stop();
            break;
         }
         BYTE peerChallenge[CLUSTER_SECRET_LENGTH];
         msg->getFieldAsBinary(VID_CHALLENGE, peerChallenge, CLUSTER_SECRET_LENGTH);
         BYTE signature[SHA256_DIGEST_SIZE];
         SignMessage(peerChallenge, CLUSTER_SECRET_LENGTH, s_secret, CLUSTER_SECRET_LENGTH, signature);
         NXCPMessage response(HA_CMD_AUTH, 0, 5);
         response.setField(VID_SIGNATURE, signature, SHA256_DIGEST_SIZE);
         sendMessage(response);
         break;
      }
      case HA_CMD_AUTH:
      {
         BYTE signature[SHA256_DIGEST_SIZE];
         msg->getFieldAsBinary(VID_SIGNATURE, signature, SHA256_DIGEST_SIZE);
         if (ValidateMessageSignature(m_challenge, CLUSTER_SECRET_LENGTH, s_secret, CLUSTER_SECRET_LENGTH, signature))
         {
            m_authenticated = true;
            nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Peer channel established (%s)", m_outbound ? L"outbound" : L"inbound");
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Peer channel authentication failed; re-reading cluster secret");
            LoadClusterSecret();   // this node may hold a stale secret from a concurrent bootstrap
            stop();
         }
         break;
      }
      case HA_CMD_CHANGE_NOTIFY:
      {
         int64_t head = msg->getFieldAsInt64(VID_SEQUENCE_NUMBER);
         int64_t current = s_headHint.load();
         while((current < head) && !s_headHint.compare_exchange_weak(current, head))
            ;
         s_applierWakeup.set();
         break;
      }
      case HA_CMD_WATERMARK:
         s_peerWatermark = msg->getFieldAsInt64(VID_SEQUENCE_NUMBER);
         break;
      case HA_CMD_DCI_DATA:
      {
         // Only a standby maintains warm state; drop batches that arrive
         // around a role change (the values are already in this node's
         // caches or will be reloaded from the database)
         HALeaseManager *manager = HAGetLeaseManager();
         if ((manager == nullptr) || (manager->getState() != HALeaseState::STANDBY))
            break;
         int count = msg->getFieldAsInt32(VID_NUM_ELEMENTS);
         uint32_t fieldId = VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < count; i++, fieldId += 10)
         {
            uint32_t ownerId = msg->getFieldAsUInt32(fieldId);
            uint32_t dciId = msg->getFieldAsUInt32(fieldId + 1);
            bool applied = false;
            shared_ptr<NetObj> object = FindObjectById(ownerId);
            if ((object != nullptr) && object->isDataCollectionTarget())
            {
               shared_ptr<DCObject> dci = static_cast<DataCollectionTarget*>(object.get())->getDCObjectById(dciId, 0, true);
               if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
               {
                  wchar_t rawValue[MAX_DB_STRING], transformedValue[MAX_DB_STRING];
                  msg->getFieldAsString(fieldId + 3, rawValue, MAX_DB_STRING);
                  msg->getFieldAsString(fieldId + 4, transformedValue, MAX_DB_STRING);
                  uint16_t flags = msg->getFieldAsUInt16(fieldId + 5);
                  static_cast<DCItem*>(dci.get())->feedValueFromPeer(Timestamp::fromMilliseconds(msg->getFieldAsInt64(fieldId + 2)),
                        rawValue, transformedValue, (flags & DATA_FEED_FLAG_STORED) != 0, (flags & DATA_FEED_FLAG_ANOMALY) != 0);
                  applied = true;
               }
            }
            if (applied)
               s_dataFeedValuesApplied++;
            else
               s_dataFeedValuesDiscarded++;
         }
         break;
      }
      case HA_CMD_DEMOTED:
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Peer node announced demotion");
         HALeaseManager *manager = HAGetLeaseManager();
         if (manager != nullptr)
            manager->wakeup();   // accelerate lease acquisition; the lease itself stays authoritative
         break;
      }
      case HA_CMD_KEEPALIVE:
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 4, L"Unexpected message 0x%04X on peer channel", msg->getCode());
         break;
   }
}

/**
 * Send message to all authenticated peer sessions
 */
static void SendToPeer(const NXCPMessage& msg)
{
   LockGuard lockGuard(s_sessionLock);
   if ((s_inboundSession != nullptr) && s_inboundSession->isAuthenticated())
      s_inboundSession->sendMessage(msg);
   if ((s_outboundSession != nullptr) && s_outboundSession->isAuthenticated())
      s_outboundSession->sendMessage(msg);
}

/**
 * Flush the data collection feed: finalize the current partial batch and
 * dispatch everything queued. Called by the sender thread once per cycle;
 * without an authenticated peer the batches are dropped (the feed never
 * buffers across an outage - the cache loader covers the gap at activation).
 */
static void FlushDataFeed(bool peerConnected)
{
   std::vector<NXCPMessage*> batch;
   s_dataFeedLock.lock();
   if (s_dataFeedMessage != nullptr)
   {
      s_dataFeedMessage->setField(VID_NUM_ELEMENTS, s_dataFeedCount);
      s_dataFeedPending.push_back(s_dataFeedMessage);
      s_dataFeedMessage = nullptr;
   }
   batch.swap(s_dataFeedPending);
   s_dataFeedLock.unlock();

   for(NXCPMessage *msg : batch)
   {
      uint32_t count = msg->getFieldAsUInt32(VID_NUM_ELEMENTS);
      if (peerConnected)
      {
         SendToPeer(*msg);
         s_dataFeedValuesSent += count;
      }
      else
      {
         s_dataFeedValuesDropped += count;
      }
      delete msg;
   }
}

/**
 * Channel listener
 */
class HAChannelListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer) override;
   virtual bool isStopConditionReached() override { return s_shutdown.load(); }

public:
   HAChannelListener(uint16_t port) : StreamSocketListener(port) { setName(_T("HAChannel")); }
};

/**
 * Process inbound connection: TLS accept, then session handshake
 */
ConnectionProcessingResult HAChannelListener::processConnection(SOCKET s, const InetAddress& peer)
{
   nxlog_debug_tag(DEBUG_TAG, 4, L"Inbound peer channel connection from %s", peer.toString().cstr());

   SSL_CTX *context = SSL_CTX_new(TLS_method());
   if (context == nullptr)
      return CPR_COMPLETED;
   if (!SetupServerTlsContext(context))
   {
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }

   SSL *ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }
   SSL_set_accept_state(ssl);
   SSL_set_fd(ssl, static_cast<int>(s));

   if (!DoTlsHandshake(ssl, s))
   {
      SSL_free(ssl);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }

   LockGuard lockGuard(s_sessionLock);
   if ((s_inboundSession != nullptr) && !s_inboundSession->isStopped())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Rejecting inbound peer channel connection (session already active)");
      SSL_free(ssl);
      SSL_CTX_free(context);
      return CPR_COMPLETED;
   }
   delete s_inboundSession;
   s_inboundSession = new HAChannelSession(s, context, ssl, false);
   s_inboundSession->start();
   return CPR_BACKGROUND;
}

/**
 * Listener thread
 */
static void ListenerThread()
{
   ThreadSetName("HAChListener");
   HAChannelListener listener(s_channelPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot initialize peer channel listener on port %u", s_channelPort);
      return;
   }
   nxlog_debug_tag(DEBUG_TAG, 2, L"Peer channel listener started on port %u", s_channelPort);
   listener.mainLoop();
   listener.shutdown();
}

/**
 * Dialer thread: while this node is not the lease holder, maintain an
 * outbound connection to the configured peer (the active never dials)
 */
static void DialerThread()
{
   ThreadSetName("HAChDialer");
   while(!s_shutdown.load())
   {
      HALeaseManager *manager = HAGetLeaseManager();
      bool wantConnection = (manager != nullptr) && (manager->getState() == HALeaseState::STANDBY);

      s_sessionLock.lock();
      bool connected = (s_outboundSession != nullptr) && !s_outboundSession->isStopped();
      s_sessionLock.unlock();

      if (wantConnection && !connected)
      {
         InetAddress addr = InetAddress::resolveHostName(s_peerAddress);
         if (addr.isValidUnicast() || addr.isLoopback())
         {
            SOCKET s = ConnectToHost(addr, s_peerPort, 5000);
            if (s != INVALID_SOCKET)
            {
               SSL_CTX *context = SSL_CTX_new(TLS_method());
               SSL *ssl = (context != nullptr) ? SSL_new(context) : nullptr;
               if (ssl != nullptr)
               {
                  SSL_set_connect_state(ssl);
                  SSL_set_fd(ssl, static_cast<int>(s));
                  if (DoTlsHandshake(ssl, s))
                  {
                     LockGuard lockGuard(s_sessionLock);
                     delete s_outboundSession;
                     s_outboundSession = new HAChannelSession(s, context, ssl, true);
                     s_outboundSession->start();
                     nxlog_debug_tag(DEBUG_TAG, 3, L"Outbound peer channel connection to %s:%u established", s_peerAddress, s_peerPort);
                  }
                  else
                  {
                     SSL_free(ssl);
                     SSL_CTX_free(context);
                     closesocket(s);
                  }
               }
               else
               {
                  if (context != nullptr)
                     SSL_CTX_free(context);
                  closesocket(s);
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, L"Cannot connect to peer %s:%u", s_peerAddress, s_peerPort);
            }
         }
      }
      else if (!wantConnection && connected)
      {
         LockGuard lockGuard(s_sessionLock);
         if (s_outboundSession != nullptr)
            s_outboundSession->stop();
      }

      ThreadSleep(5);
   }
}

/**
 * Keepalive / notification sender thread (all roles; only authenticated
 * sessions receive anything). Coalesces change notifications: at most one
 * CHANGE_NOTIFY per cycle carrying the current journal head.
 */
static void SenderThread()
{
   ThreadSetName("HAChSender");
   int64_t lastSentHead = 0;
   int keepaliveCountdown = 5;
   while(!s_shutdown.load())
   {
      ThreadSleep(1);
      if (s_shutdown.load())
         break;

      // Maintain data collection feed state and flush the accumulated batch
      bool peerConnected = HAChannelIsPeerConnected();
      HALeaseManager *manager = HAGetLeaseManager();
      s_dataFeedActive = s_dataFeedEnabled && peerConnected && (manager != nullptr) && (manager->getState() == HALeaseState::ACTIVE);
      FlushDataFeed(peerConnected);

      int64_t head = HAJournalGetHead();
      if (head > lastSentHead)
      {
         NXCPMessage msg(HA_CMD_CHANGE_NOTIFY, 0, 5);
         msg.setField(VID_SEQUENCE_NUMBER, head);
         SendToPeer(msg);
         lastSentHead = head;
         keepaliveCountdown = 5;
      }
      else if (--keepaliveCountdown <= 0)
      {
         NXCPMessage msg(HA_CMD_KEEPALIVE, 0, 5);
         SendToPeer(msg);
         keepaliveCountdown = 5;
      }
   }
}

#endif   /* _WITH_ENCRYPTION */

/**
 * Applier thread (standby side): apply committed journal entries above the
 * watermark in sequence order. Runs on notification hints and on a periodic
 * timer, so a lost channel message only costs latency. The watermark
 * advances over a contiguous prefix; a missing sequence number (journal row
 * allocated but transaction not yet committed, or rolled back) blocks
 * advancement for a bounded grace period, then is treated as rolled back.
 */
static void ApplierThread()
{
   ThreadSetName("HAApplier");

   int64_t watermark = (s_initialWatermark >= 0) ? s_initialWatermark : HAJournalReadWatermark();
   s_appliedWatermark = watermark;
   int64_t pendingGapSeq = 0;
   time_t pendingGapTime = 0;
   nxlog_debug_tag(DEBUG_TAG, 2, L"Journal applier started (watermark " INT64_FMTW L")", watermark);
   HAJournalSaveWatermark(watermark);   // supersede the previous incarnation's persisted value

   while(!s_shutdown.load() && !s_applierQuiesce.load())
   {
      s_applierWakeup.wait((s_headHint.load() > watermark) ? 1000 : 5000);
      if (s_shutdown.load() || s_applierQuiesce.load())
         break;

      // Only a standby maintains warm state; the active is the journal writer
      HALeaseManager *manager = HAGetLeaseManager();
      if ((manager == nullptr) || (manager->getState() != HALeaseState::STANDBY))
         continue;

      // A watermark below the age-based truncation point cannot be
      // reconciled by replay - the warm state is unrebuildable in place
      if (HAJournalGetTruncationPoint() > watermark)
         HASyncSetDirty(L"change journal truncated past this node's watermark");

      // Collect the contiguous committed prefix above the watermark
      int64_t newWatermark = watermark;
      bool blocked = false;
      std::vector<HAJournalEntry> batch;
      HAJournalReplay(watermark,
         [&newWatermark, &blocked, &pendingGapSeq, &pendingGapTime, &batch] (const HAJournalEntry& entry) -> void
         {
            if (blocked)
               return;
            if (entry.seq > newWatermark + 1)
            {
               // Gap: seq allocated but row not visible yet (uncommitted or rolled back)
               if (pendingGapSeq != newWatermark + 1)
               {
                  pendingGapSeq = newWatermark + 1;
                  pendingGapTime = time(nullptr);
                  blocked = true;
                  return;
               }
               if (time(nullptr) - pendingGapTime < 60)
               {
                  blocked = true;
                  return;
               }
               nxlog_debug_tag(DEBUG_TAG, 4, L"Journal gap at " INT64_FMTW L" aged out (rolled back transaction)", pendingGapSeq);
               pendingGapSeq = 0;
            }
            batch.push_back(entry);
            newWatermark = entry.seq;
         });

      if (!batch.empty() && (s_applyHandler != nullptr))
         s_applyHandler(batch);

      if (newWatermark > watermark)
      {
         watermark = newWatermark;
         s_appliedWatermark = watermark;
         if (pendingGapSeq <= watermark)
            pendingGapSeq = 0;
         HAJournalSaveWatermark(watermark);
#ifdef _WITH_ENCRYPTION
         NXCPMessage msg(HA_CMD_WATERMARK, 0, 5);
         msg.setField(VID_SEQUENCE_NUMBER, watermark);
         SendToPeer(msg);
#endif
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 2, L"Journal applier stopped");
}

/**
 * Threads
 */
static THREAD s_listenerThread = INVALID_THREAD_HANDLE;
static THREAD s_dialerThread = INVALID_THREAD_HANDLE;
static THREAD s_senderThread = INVALID_THREAD_HANDLE;
static THREAD s_applierThread = INVALID_THREAD_HANDLE;

/**
 * Set channel configuration (called by HAStartController before start)
 */
void HAChannelConfigure(const wchar_t *peerAddress, uint16_t listenPort, uint16_t peerPort, bool dataFeedEnabled)
{
   wcslcpy(s_peerAddress, peerAddress, 256);
   s_channelPort = listenPort;
   s_peerPort = peerPort;
   s_dataFeedEnabled = dataFeedEnabled;
}

/**
 * Feed a collected DCI value to the standby node (called by the data
 * collector for every processed value while this node is active). Values are
 * batched; a full batch queues for the sender thread, which also flushes
 * partial batches once per second.
 */
void HAChannelFeedDataValue(uint32_t ownerId, uint32_t dciId, Timestamp timestamp, const wchar_t *rawValue,
      const wchar_t *transformedValue, bool storedInDb, bool anomalyDetected)
{
   if (!s_dataFeedActive.load(std::memory_order_relaxed))
      return;

#ifdef _WITH_ENCRYPTION
   LockGuard lockGuard(s_dataFeedLock);
   if (s_dataFeedMessage == nullptr)
   {
      s_dataFeedMessage = new NXCPMessage(HA_CMD_DCI_DATA, 0, 5);
      s_dataFeedCount = 0;
      s_dataFeedFieldId = VID_ELEMENT_LIST_BASE;
   }
   s_dataFeedMessage->setField(s_dataFeedFieldId, ownerId);
   s_dataFeedMessage->setField(s_dataFeedFieldId + 1, dciId);
   s_dataFeedMessage->setField(s_dataFeedFieldId + 2, timestamp.asMilliseconds());
   s_dataFeedMessage->setField(s_dataFeedFieldId + 3, rawValue);
   s_dataFeedMessage->setField(s_dataFeedFieldId + 4, transformedValue);
   s_dataFeedMessage->setField(s_dataFeedFieldId + 5,
         static_cast<uint16_t>((storedInDb ? DATA_FEED_FLAG_STORED : 0) | (anomalyDetected ? DATA_FEED_FLAG_ANOMALY : 0)));
   s_dataFeedCount++;
   s_dataFeedFieldId += 10;
   if (s_dataFeedCount >= DATA_FEED_BATCH_SIZE)
   {
      s_dataFeedMessage->setField(VID_NUM_ELEMENTS, s_dataFeedCount);
      if (s_dataFeedPending.size() < DATA_FEED_MAX_PENDING)
      {
         s_dataFeedPending.push_back(s_dataFeedMessage);
      }
      else
      {
         s_dataFeedValuesDropped += s_dataFeedCount;
         delete s_dataFeedMessage;
      }
      s_dataFeedMessage = nullptr;
   }
#endif
}

/**
 * Get data collection feed statistics
 */
HADataFeedStats HAChannelGetDataFeedStats()
{
   HADataFeedStats stats;
   stats.enabled = s_dataFeedEnabled;
   stats.valuesSent = s_dataFeedValuesSent.load();
   stats.valuesDropped = s_dataFeedValuesDropped.load();
   stats.valuesApplied = s_dataFeedValuesApplied.load();
   stats.valuesDiscarded = s_dataFeedValuesDiscarded.load();
   return stats;
}

/**
 * Register the journal batch apply handler and the applier's starting
 * watermark (journal head captured before the warm state load began). Must
 * be called before HAChannelStart.
 */
void HAChannelSetupSync(std::function<void(const std::vector<HAJournalEntry>&)> handler, int64_t initialWatermark)
{
   s_applyHandler = handler;
   s_initialWatermark = initialWatermark;
}

/**
 * Stop the live applier and replay the remaining journal tail (activation
 * path, called after this node wins the lease). The old active's writes have
 * quiesced at this point, so gaps in the sequence are rolled-back
 * transactions and are skipped without waiting. Returns false if the warm
 * state cannot be trusted (the caller must restart into a fresh standby).
 */
bool HAChannelFinalizeSync()
{
   s_applierQuiesce = true;
   s_applierWakeup.set();
   ThreadJoin(s_applierThread);
   s_applierThread = INVALID_THREAD_HANDLE;

   int64_t watermark = s_appliedWatermark.load();
   if (HAJournalGetTruncationPoint() > watermark)
      HASyncSetDirty(L"change journal truncated past this node's watermark");
   if (HASyncIsDirty())
      return false;

   std::vector<HAJournalEntry> batch;
   watermark = HAJournalReplay(watermark,
      [&batch] (const HAJournalEntry& entry) -> void
      {
         batch.push_back(entry);
      });
   if (!batch.empty() && (s_applyHandler != nullptr))
      s_applyHandler(batch);
   s_appliedWatermark = watermark;
   HAJournalSaveWatermark(watermark);

   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Journal synchronization finalized at activation (%d entries replayed, watermark " INT64_FMTW L")",
         static_cast<int>(batch.size()), watermark);
   return !HASyncIsDirty();
}

/**
 * Get this node's contiguous applied journal watermark
 */
int64_t HAChannelGetAppliedWatermark()
{
   return s_appliedWatermark.load();
}

/**
 * Start cluster peer channel and journal applier
 */
bool HAChannelStart()
{
#ifdef _WITH_ENCRYPTION
   if (!LoadClusterSecret())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot load cluster secret");
      return false;
   }

   s_listenerThread = ThreadCreateEx(ListenerThread);
   if (s_peerAddress[0] != 0)
      s_dialerThread = ThreadCreateEx(DialerThread);
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"PeerAddress is not configured; peer channel will accept inbound connections only");
   s_senderThread = ThreadCreateEx(SenderThread);
#else
   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Server built without encryption support; peer channel disabled (journal replay still provides standby synchronization)");
#endif
   s_applierThread = ThreadCreateEx(ApplierThread);
   return true;
}

/**
 * Announce demotion to the peer (graceful handover; called before the lease
 * is released so the peer polls for acquisition immediately)
 */
void HAChannelNotifyDemotion()
{
#ifdef _WITH_ENCRYPTION
   NXCPMessage msg(HA_CMD_DEMOTED, 0, 5);
   SendToPeer(msg);
#endif
}

/**
 * Check if an authenticated peer channel session exists
 */
bool NXCORE_EXPORTABLE HAChannelIsPeerConnected()
{
#ifdef _WITH_ENCRYPTION
   LockGuard lockGuard(s_sessionLock);
   return ((s_inboundSession != nullptr) && s_inboundSession->isAuthenticated()) ||
          ((s_outboundSession != nullptr) && s_outboundSession->isAuthenticated());
#else
   return false;
#endif
}

/**
 * Get last watermark reported by the peer over the channel
 */
int64_t NXCORE_EXPORTABLE HAChannelGetPeerWatermark()
{
   return s_peerWatermark.load();
}

/**
 * Shut down cluster peer channel
 */
void HAChannelShutdown()
{
   s_shutdown = true;
   s_dataFeedActive = false;
   s_applierWakeup.set();
#ifdef _WITH_ENCRYPTION
   s_dataFeedLock.lock();
   delete_and_null(s_dataFeedMessage);
   for(NXCPMessage *msg : s_dataFeedPending)
      delete msg;
   s_dataFeedPending.clear();
   s_dataFeedLock.unlock();
   s_sessionLock.lock();
   if (s_inboundSession != nullptr)
      s_inboundSession->stop();
   if (s_outboundSession != nullptr)
      s_outboundSession->stop();
   s_sessionLock.unlock();
#endif
   ThreadJoin(s_listenerThread);
   ThreadJoin(s_dialerThread);
   ThreadJoin(s_senderThread);
   ThreadJoin(s_applierThread);
#ifdef _WITH_ENCRYPTION
   s_sessionLock.lock();
   delete_and_null(s_inboundSession);
   delete_and_null(s_outboundSession);
   s_sessionLock.unlock();
#endif
   nxlog_debug_tag(DEBUG_TAG, 2, L"Peer channel stopped");
}
