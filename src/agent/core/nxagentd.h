/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxagentd.h
**
**/

#ifndef _nxagentd_h_
#define _nxagentd_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nms_agent.h>
#include <nxcpapi.h>
#include <nxqueue.h>
#include <nxlog.h>
#include <nxcldefs.h>
#include <nxdbapi.h>
#include <nxsnmp.h>
#include "localdb.h"

#ifdef _WIN32
#include <aclapi.h>
#endif

/**
 * Default files
 */
#if defined(_WIN32)
#define AGENT_DEFAULT_CONFIG     _T("{search}")
#define AGENT_DEFAULT_CONFIG_D   _T("{search}")
#define AGENT_DEFAULT_LOG        _T("C:\\nxagentd.log")
#define AGENT_DEFAULT_FILE_STORE _T("C:\\")
#define AGENT_DEFAULT_DATA_DIR   _T("{default}")
#else
#define AGENT_DEFAULT_CONFIG     _T("{search}")
#define AGENT_DEFAULT_CONFIG_D   _T("{search}")
#define AGENT_DEFAULT_LOG        _T("/var/log/nxagentd")
#define AGENT_DEFAULT_FILE_STORE _T("/tmp")
#define AGENT_DEFAULT_DATA_DIR   _T("{default}")
#endif

/**
 * Constants
 */
#ifdef _WIN32
#define DEFAULT_AGENT_SERVICE_NAME    _T("NetXMSAgentdW32")
#define DEFAULT_AGENT_EVENT_SOURCE    _T("NetXMS Win32 Agent")
#define NXAGENTD_SYSLOG_NAME          g_windowsEventSourceName
#else
#define NXAGENTD_SYSLOG_NAME          _T("nxagentd")
#endif

#define DEBUG_TAG_COMM           _T("comm")
#define DEBUG_TAG_LOCALDB        _T("db.agent")
#define DEBUG_TAG_STARTUP        _T("startup")

#define DEFAULT_CONFIG_SECTION   _T("CORE")

#define MAX_PSUFFIX_LENGTH 32
#define MAX_SERVERS        32
#define MAX_ESA_USER_NAME  64
#define MAX_AGENT_MSG_SIZE (16 * 1024 * 1024)   // 16MB

#define AF_DAEMON                   0x00000001
#define AF_DISABLE_LOCAL_DATABASE   0x00000002
#define AF_SUBAGENT_LOADER          0x00000004
#define AF_REQUIRE_AUTH             0x00000008
#define AF_ENABLE_ACTIONS           0x00000010
#define AF_REQUIRE_ENCRYPTION       0x00000020
#define AF_HIDE_WINDOW              0x00000040
#define AF_ENABLE_PROXY             0x00000080
#define AF_CENTRAL_CONFIG           0x00000100
#define AF_ENABLE_SNMP_PROXY        0x00000200
#define AF_SHUTDOWN                 0x00000400
#define AF_INTERACTIVE_SERVICE      0x00000800
#define AF_DISABLE_IPV4             0x00001000
#define AF_DISABLE_IPV6             0x00002000
#define AF_ENABLE_SNMP_TRAP_PROXY   0x00004000
#define AF_ENABLE_SYSLOG_PROXY      0x00008000
#define AF_ENABLE_TCP_PROXY         0x00010000
#define AF_SYSTEMD_DAEMON           0x00020000
#define AF_ENABLE_SSL_TRACE         0x00040000
#define AF_CHECK_SERVER_CERTIFICATE 0x00080000
#define AF_ENABLE_WEBSVC_PROXY      0x00100000
#define AF_ENABLE_TFTP_PROXY        0x00200000
#define AF_ENABLE_MODBUS_PROXY      0x00400000
#define AF_DISABLE_HEARTBEAT        0x00800000
#define AF_SYNC_TIME_WITH_SERVER    0x01000000

// Flags for component failures
#define FAIL_OPEN_LOG               0x00000001
#define FAIL_OPEN_DATABASE          0x00000002
#define FAIL_UPGRADE_DATABASE       0x00000004
#define FAIL_LOAD_CONFIG            0x00000008
#define FAIL_CRASH_SERVER_START     0x00000010

/**
 * Thread pool stats
 */
enum ThreadPoolStat
{
   THREAD_POOL_CURR_SIZE,
   THREAD_POOL_MIN_SIZE,
   THREAD_POOL_MAX_SIZE,
   THREAD_POOL_ACTIVE_REQUESTS,
   THREAD_POOL_SCHEDULED_REQUESTS,
   THREAD_POOL_LOAD,
   THREAD_POOL_USAGE,
   THREAD_POOL_LOADAVG_1,
   THREAD_POOL_LOADAVG_5,
   THREAD_POOL_LOADAVG_15,
   THREAD_POOL_AVG_WAIT_TIME
};

/**
 * Request types for H_DirInfo
 */
#define DIRINFO_FILE_COUNT       1
#define DIRINFO_FILE_SIZE        2
#define DIRINFO_FOLDER_COUNT     3
#define DIRINFO_FILE_LINE_COUNT  4

/**
 * Request types for H_FileTime
 */
#define FILETIME_ATIME           1
#define FILETIME_MTIME           2
#define FILETIME_CTIME           3

/**
 * External table definition
 */
struct ExternalTableDefinition
{
   TCHAR *cmdLine;
   TCHAR separator;
   bool mergeSeparators;
   int instanceColumnCount;
   TCHAR **instanceColumns;
   StringMap columnDataTypes;
   int defaultColumnDataType;

   ExternalTableDefinition()
   {
      cmdLine = nullptr;
      separator = 0;
      mergeSeparators = false;
      instanceColumnCount = 0;
      instanceColumns = nullptr;
      defaultColumnDataType = DCI_DT_INT;
   }

   ~ExternalTableDefinition()
   {
      MemFree(cmdLine);
      for(int i = 0; i < instanceColumnCount; i++)
         MemFree(instanceColumns[i]);
      MemFree(instanceColumns);
   }
};

bool IsParametrizedCommand(const TCHAR *command);

/**
 * External table definition
 */
struct StructuredExtractorParameterDefinition
{
   String query;
   String description;
   uint16_t dataType;
   bool isParametrized;

   StructuredExtractorParameterDefinition(const TCHAR *q, const TCHAR *d, uint16_t dt) : query(q), description(d)
   {
      dataType = dt;
      isParametrized = IsParametrizedCommand(q);
   }
};

/**
 * Action definition structure
 */
struct ACTION
{
   TCHAR name[MAX_PARAM_NAME];
   bool isExternal;
   union
   {
      TCHAR *cmdLine;
      struct __subagentAction
      {
         uint32_t (*handler)(const shared_ptr<ActionExecutionContext>&);
         const void *arg;
         const TCHAR *subagentName;
      } sa;
   } handler;
   TCHAR description[MAX_DB_STRING];
};

/**
 * Loaded subagent information
 */
struct SUBAGENT
{
   HMODULE moduleHandle;        // Subagent's module handle
   NETXMS_SUBAGENT_INFO *info;  // Information provided by subagent
   TCHAR moduleName[MAX_PATH];  // Name of the module  // to TCHAR by LWX
};

struct ActionList;

/**
 * External subagent information
 */
class ExternalSubagent
{
private:
	TCHAR m_name[MAX_SUBAGENT_NAME];
	TCHAR m_user[MAX_ESA_USER_NAME];
	NamedPipeListener *m_listener;
	NamedPipe *m_pipe;
	bool m_connected;
	MsgWaitQueue *m_msgQueue;
	uint32_t m_requestId;
   uint32_t m_listenerStartDelay;

	bool sendMessage(const NXCPMessage *msg);
	NXCPMessage *waitForMessage(uint16_t code, uint32_t id);
	NETXMS_SUBAGENT_PARAM *getSupportedParameters(UINT32 *count);
	NETXMS_SUBAGENT_LIST *getSupportedLists(UINT32 *count);
	NETXMS_SUBAGENT_TABLE *getSupportedTables(UINT32 *count);
	ActionList *getSupportedActions();

public:
	ExternalSubagent(const TCHAR *name, const TCHAR *user);
	~ExternalSubagent();

	void startListener();
   void stopListener();
	void connect(NamedPipe *pipe);

	bool isConnected() { return m_connected; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getUserName() { return m_user; }

   uint32_t getParameter(const TCHAR *name, TCHAR *buffer);
   uint32_t getTable(const TCHAR *name, Table *value);
   uint32_t getList(const TCHAR *name, StringList *value);
   uint32_t executeAction(const TCHAR *name, const StringList& args, AbstractCommSession *session, uint32_t requestId, bool sendOutput);
	void listParameters(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listParameters(StringList *list);
	void listLists(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listLists(StringList *list);
	void listTables(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listTables(StringList *list);
	void listActions(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
	void listActions(StringList *list);
   void shutdown(bool restart);
   void restart();
   void syncPolicies();
   void notifyOnPolicyInstall(const uuid& guid);
   void notifyOnComponentToken(const AgentComponentToken *token);
};

/**
 * Server information
 */
class ServerInfo
{
private:
   char *m_name;
   InetAddress m_address;
   bool m_control;
   bool m_master;
   time_t m_lastResolveTime;
   bool m_redoResolve;
   Mutex m_mutex;

   void resolve(bool forceResolve);

public:
   ServerInfo(const TCHAR *name, bool control, bool master);
   ~ServerInfo();

   bool match(const InetAddress &addr, bool forceResolve);

   bool isMaster() const { return m_master; }
   bool isControl() const { return m_control; }
};

/**
 * Pending request information
 */
struct PendingRequest
{
   uint32_t id;
   int64_t expirationTime;
   bool completed;

   PendingRequest(NXCPMessage *msg, uint32_t timeout)
   {
      id = msg->getId();
      expirationTime = GetCurrentTimeMs() + static_cast<int64_t>(timeout);
      completed = false;
   }
};

class TcpProxy;

/**
 * Communication session
 */
class CommSession : public AbstractCommSession
{
private:
   uint32_t m_id;
   uint32_t m_index;
   TCHAR m_key[32];  // key for serialized background tasks
   TCHAR m_debugTag[16];
   shared_ptr<AbstractCommChannel> m_channel;
   int m_protocolVersion;
   THREAD m_proxyReadThread;
   THREAD m_tcpProxyReadThread;
   uint64_t m_serverId;
   InetAddress m_serverAddr;        // IP address of connected host
   bool m_disconnected;
   bool m_authenticated;
   bool m_masterServer;
   bool m_controlServer;
   bool m_proxyConnection;
   bool m_acceptData;
   bool m_acceptTraps;
   bool m_acceptFileUpdates;
   bool m_ipv6Aware;
   bool m_bulkReconciliationSupported;
   bool m_allowCompression;   // allow compression for structured messages
   bool m_acceptKeepalive;    // true if server will respond to keepalive messages
   bool m_stopCommandProcessing;
   VolatileCounter m_pendingRequests;
   HashMap<uint32_t, DownloadFileInfo> m_downloadFileMap;
	shared_ptr<NXCPEncryptionContext> m_encryptionContext;
   int64_t m_timestamp;       // Last activity timestamp
   SOCKET m_hProxySocket;     // Socket for proxy connection
	Mutex m_socketWriteMutex;
   VolatileCounter m_requestId;
   MsgWaitQueue *m_responseQueue;
   Mutex m_tcpProxyLock;
   ObjectArray<TcpProxy> m_tcpProxies;
   SynchronizedHashMap<uint32_t, Condition> m_responseConditionMap;

	bool sendRawMessage(NXCP_MESSAGE *msg, NXCPEncryptionContext *ctx);
   void authenticate(NXCPMessage *pRequest, NXCPMessage *pMsg);
   void getConfig(NXCPMessage *pMsg);
   void updateConfig(NXCPMessage *pRequest, NXCPMessage *pMsg);
   void getParameter(NXCPMessage *request, NXCPMessage *response);
   void getList(NXCPMessage *request, NXCPMessage *response);
   void getTable(NXCPMessage *request, NXCPMessage *response);
   void action(NXCPMessage *request, NXCPMessage *response);
   void recvFile(NXCPMessage *request, NXCPMessage *response);
   uint32_t installPackage(NXCPMessage *request);
   uint32_t upgrade(NXCPMessage *request);
   uint32_t setupProxyConnection(NXCPMessage *pRequest);
   void setupTcpProxy(NXCPMessage *request, NXCPMessage *response);
   uint32_t closeTcpProxy(NXCPMessage *request);
   void proxySnmpRequest(NXCPMessage *request);
   void queryWebService(NXCPMessage *request);
   void webServiceCustomRequest(NXCPMessage *request);
   void sendMessageInBackground(NXCP_MESSAGE *msg);
   void getHostNameByAddr(NXCPMessage *request, NXCPMessage *response);
   uint32_t setComponentToken(NXCPMessage *request);

   void processCommand(NXCPMessage *request);

   void readThread();
   void proxyReadThread();
   void tcpProxyReadThread();

   void setResponseSentCondition(uint32_t requestId);

public:
   CommSession(const shared_ptr<AbstractCommChannel>& channel, const InetAddress &serverAddr, bool masterServer, bool controlServer);
   virtual ~CommSession();

   shared_ptr<CommSession> self() const { return static_pointer_cast<CommSession>(AbstractCommSession::self()); }

   void run();
   void disconnect();

   virtual bool sendMessage(const NXCPMessage *msg) override;
   virtual void postMessage(const NXCPMessage *msg) override;
   virtual bool sendRawMessage(const NXCP_MESSAGE *msg) override;
   virtual void postRawMessage(const NXCP_MESSAGE *msg) override;
   virtual bool sendFile(uint32_t requestId, const TCHAR *file, off64_t offset, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag) override;
   virtual uint32_t doRequest(NXCPMessage *msg, uint32_t timeout) override;
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, uint32_t timeout) override;
   virtual NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout) override;
   virtual uint32_t generateRequestId() override;
   virtual int getProtocolVersion() override { return m_protocolVersion; }

   virtual uint32_t getId() override { return m_id; };

   virtual uint64_t getServerId() override { return m_serverId; }
	virtual const InetAddress& getServerAddress() override { return m_serverAddr; }

   virtual bool isMasterServer() override { return m_masterServer; }
   virtual bool isControlServer() override { return m_controlServer; }
   virtual bool canAcceptData() override { return m_acceptData; }
   virtual bool canAcceptTraps() override { return m_acceptTraps; }
   virtual bool canAcceptFileUpdates() override { return m_acceptFileUpdates; }
   virtual bool isBulkReconciliationSupported() override { return m_bulkReconciliationSupported; }
   virtual bool isIPv6Aware() override { return m_ipv6Aware; }

   virtual const TCHAR *getDebugTag() const override { return m_debugTag; }

   virtual void openFile(NXCPMessage *response, TCHAR *nameOfFile, uint32_t requestId, time_t fileModTime = 0, FileTransferResumeMode resumeMode = FileTransferResumeMode::OVERWRITE) override;

   virtual void debugPrintf(int level, const TCHAR *format, ...) override;
   virtual void writeLog(int16_t severity, const TCHAR *format, ...) override;

   virtual void prepareProxySessionSetupMsg(NXCPMessage *msg) override;

   int64_t getTimeStamp() { return m_timestamp; }
	void updateTimeStamp() { m_timestamp = GetMonotonicClockTime(); }

   bool sendMessage(const NXCPMessage& msg) { return sendMessage(&msg); }
   void postMessage(const NXCPMessage& msg) { postMessage(&msg); }

   virtual void registerForResponseSentCondition(uint32_t requestId) override;
   virtual void waitForResponseSentCondition(uint32_t requestId) override;
};

/**
 * Pseudo-session for cached data collection
 */
class VirtualSession : public AbstractCommSession
{
private:
   uint32_t m_id;
   uint64_t m_serverId;
   TCHAR m_debugTag[16];

public:
   VirtualSession(uint64_t serverId);
   virtual ~VirtualSession();

   virtual uint32_t getId() override { return m_id; }

   virtual uint64_t getServerId() override { return m_serverId; };
   virtual const InetAddress& getServerAddress() override { return InetAddress::LOOPBACK; }

   virtual bool isMasterServer() override { return false; }
   virtual bool isControlServer() override { return false; }
   virtual bool canAcceptData() override { return false; }
   virtual bool canAcceptTraps() override { return false; }
   virtual bool canAcceptFileUpdates() override { return false; }
   virtual bool isBulkReconciliationSupported() override { return false; }
   virtual bool isIPv6Aware() override { return true; }

   virtual const TCHAR *getDebugTag() const override { return m_debugTag; }

   virtual bool sendMessage(const NXCPMessage *pMsg) override { return false; }
   virtual void postMessage(const NXCPMessage *pMsg) override { }
   virtual bool sendRawMessage(const NXCP_MESSAGE *pMsg) override { return false; }
   virtual void postRawMessage(const NXCP_MESSAGE *pMsg) override { }
   virtual bool sendFile(uint32_t requestId, const TCHAR *file, off64_t offset, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancelationFlag) override { return false; }
   virtual uint32_t doRequest(NXCPMessage *msg, uint32_t timeout) override { return RCC_NOT_IMPLEMENTED; }
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, uint32_t timeout) override { return NULL; }
   virtual NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout) override { return NULL; }
   virtual uint32_t generateRequestId() override { return 0; }
   virtual int getProtocolVersion() override { return NXCP_VERSION; }
   virtual void openFile(NXCPMessage *response, TCHAR *fileName, uint32_t requestId, time_t fileModTime = 0, FileTransferResumeMode resumeMode = FileTransferResumeMode::OVERWRITE) override { response->setField(VID_RCC, ERR_INTERNAL_ERROR); }
   virtual void debugPrintf(int level, const TCHAR *format, ...) override;
   virtual void writeLog(int16_t severity, const TCHAR *format, ...) override;
   virtual void prepareProxySessionSetupMsg(NXCPMessage *msg) override { }
   virtual void registerForResponseSentCondition(uint32_t requestId) override { }
   virtual void waitForResponseSentCondition(uint32_t requestId) override { }
};

/**
 * Pseudo-session for external sub-agents
 */
class ProxySession : public AbstractCommSession
{
private:
   uint32_t m_id;
   uint64_t m_serverId;
   InetAddress m_serverAddress;
   bool m_masterServer;
   bool m_controlServer;
   bool m_canAcceptData;
   bool m_canAcceptTraps;
   bool m_canAcceptFileUpdates;
   bool m_ipv6Aware;
   TCHAR m_debugTag[16];

public:
   ProxySession(NXCPMessage *msg);
   virtual ~ProxySession();

   virtual uint32_t getId() override { return m_id; }

   virtual uint64_t getServerId() override { return m_serverId; };
   virtual const InetAddress& getServerAddress() override { return m_serverAddress; }

   virtual bool isMasterServer() override { return m_masterServer; }
   virtual bool isControlServer() override { return m_controlServer; }
   virtual bool canAcceptData() override { return m_canAcceptData; }
   virtual bool canAcceptTraps() override { return m_canAcceptTraps; }
   virtual bool canAcceptFileUpdates() override { return m_canAcceptFileUpdates; }
   virtual bool isBulkReconciliationSupported() override { return false; }
   virtual bool isIPv6Aware() override { return m_ipv6Aware; }

   virtual const TCHAR *getDebugTag() const override { return m_debugTag; }

   virtual bool sendMessage(const NXCPMessage *pMsg) override;
   virtual void postMessage(const NXCPMessage *pMsg) override;
   virtual bool sendRawMessage(const NXCP_MESSAGE *pMsg) override;
   virtual void postRawMessage(const NXCP_MESSAGE *pMsg) override;
   virtual bool sendFile(uint32_t requestId, const TCHAR *file, off64_t offset, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancelationFlag) override { return false; }
   virtual uint32_t doRequest(NXCPMessage *msg, uint32_t timeout) override { return RCC_NOT_IMPLEMENTED; }
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, uint32_t timeout) override { return NULL; }
   virtual NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout) override { return NULL; }
   virtual uint32_t generateRequestId() override { return 0; }
   virtual int getProtocolVersion() override { return NXCP_VERSION; }
   virtual void openFile(NXCPMessage *response, TCHAR *fileName, uint32_t requestId, time_t fileModTime = 0, FileTransferResumeMode resumeMode = FileTransferResumeMode::OVERWRITE) override { response->setField(VID_RCC, ERR_INTERNAL_ERROR); }
   virtual void debugPrintf(int level, const TCHAR *format, ...) override;
   virtual void writeLog(int16_t severity, const TCHAR *format, ...) override;
   virtual void prepareProxySessionSetupMsg(NXCPMessage *msg) override { }
   virtual void registerForResponseSentCondition(uint32_t requestId) override { }
   virtual void waitForResponseSentCondition(uint32_t requestId) override { }
};

/**
 * Session agent connector
 */
class SessionAgentConnector
{
   friend shared_ptr<SessionAgentConnector> CreateSessionAgentConnector(uint32_t id, SOCKET s);

private:
   weak_ptr<SessionAgentConnector> m_self;
   uint32_t m_id;
   SOCKET m_socket;
   Mutex m_mutex;
   MsgWaitQueue m_msgQueue;
   uint32_t m_sessionId;
   TCHAR *m_sessionName;
   int16_t m_sessionState;
   TCHAR *m_userName;
   TCHAR *m_clientName;
   uint32_t m_processId;
   bool m_userAgent;
   VolatileCounter m_requestId;

   void setSelfPtr(const shared_ptr<SessionAgentConnector>& self) { m_self = self; }

   void readThread();
   bool sendMessage(const NXCPMessage *msg);
   uint32_t nextRequestId() { return InterlockedIncrement(&m_requestId); }

public:
   SessionAgentConnector(uint32_t id, SOCKET s);
   ~SessionAgentConnector();

   shared_ptr<SessionAgentConnector> self() const { return m_self.lock(); }

   void disconnect();

   uint32_t getId() const { return m_id; }
   uint32_t getSessionId() const { return m_sessionId; }
   uint32_t getProcessId() const { return m_processId;  }
   int16_t getSessionState() const { return m_sessionState; }
   const TCHAR *getSessionName() const { return CHECK_NULL_EX(m_sessionName); }
   const TCHAR *getUserName() const { return CHECK_NULL_EX(m_userName); }
   const TCHAR *getClientName() const { return CHECK_NULL_EX(m_clientName); }
   bool isUserAgent() const { return m_userAgent; }

   bool testConnection();
   void takeScreenshot(NXCPMessage *msg);
   bool getScreenInfo(uint32_t *width, uint32_t *height, uint32_t *bpp);
   void updateUserAgentConfig();
   void updateUserAgentEnvironment();
   void updateUserAgentNotifications();
   void addUserAgentNotification(UserAgentNotification *n);
   void removeUserAgentNotification(const ServerObjectKey& id);
   void shutdown(bool restart);
};

/**
 * Create and run session agent connector
 */
inline shared_ptr<SessionAgentConnector> CreateSessionAgentConnector(uint32_t id, SOCKET s)
{
   shared_ptr<SessionAgentConnector> c = make_shared<SessionAgentConnector>(id, s);
   c->setSelfPtr(c);
   ThreadCreate(c, &SessionAgentConnector::readThread);
   return c;
}

/**
 * Class to reciever traps
 */
class SNMP_TrapProxyTransport : public SNMP_UDPTransport
{
public:
   SNMP_TrapProxyTransport(SOCKET hSocket) : SNMP_UDPTransport(hSocket) {}

   int readRawMessage(BYTE **rawData, uint32_t timeout, struct sockaddr *sender, socklen_t *addrSize);
};

/**
 * SNMP target
 */
class SNMPTarget
{
private:
   uuid m_guid;
   uint64_t m_serverId;
   InetAddress m_ipAddress;
   SNMP_Version m_snmpVersion;
   uint16_t m_port;
   SNMP_AuthMethod m_authType;
   SNMP_EncryptionMethod m_encType;
   char *m_authName;
   char *m_authPassword;
   char *m_encPassword;
   SNMP_Transport *m_transport;

public:
   SNMPTarget(uint64_t serverId, const NXCPMessage& msg, uint32_t baseId);
   SNMPTarget(DB_RESULT hResult, int row);
   ~SNMPTarget();

   const uuid& getGuid() const { return m_guid; }
   SNMP_Transport *getTransport(uint16_t port);

   bool saveToDatabase(DB_HANDLE hdb);
};

/**
 * TCP proxy
 */
class TcpProxy
{
private:
   uint32_t m_channelId;
   CommSession *m_session;
   SOCKET m_socket;
   bool m_readError;

public:
   TcpProxy(CommSession *session, uint32_t channelId, SOCKET s);
   ~TcpProxy();

   uint32_t getChannelId() const { return m_channelId; }
   SOCKET getSocket() const { return m_socket; }
   bool isReadError() const { return m_readError; }

   bool readSocket();
   void writeSocket(const BYTE *data, size_t size);
};

#define PROXY_CHALLENGE_SIZE 8

#pragma pack(1)

/**
 * Data collection proxy message
 */
struct ProxyMsg
{
   BYTE challenge[PROXY_CHALLENGE_SIZE];
   UINT64 serverId;
   UINT32 proxyIdDest;
   UINT32 proxyIdSelf;
   int32_t zoneUin;
   BYTE hmac[SHA256_DIGEST_SIZE];
};

#pragma pack()

/**
 * Zone configuration
 */
class ZoneConfiguration
{
private:
   uint64_t m_serverId;
   uint32_t m_thisNodeId;
   int32_t m_zoneUin;
   BYTE m_sharedSecret[ZONE_PROXY_KEY_LENGTH];

public:
   ZoneConfiguration(uint64_t serverId, uint32_t thisNodeId, int32_t zoneUin, const BYTE *sharedSecret);

   void update(const ZoneConfiguration& src);

   uint64_t getServerId() const { return m_serverId; }
   uint32_t getThisNodeId() const { return m_thisNodeId; }
   int32_t getZoneUIN() const { return m_zoneUin; }
   const BYTE *getSharedSecret() const { return m_sharedSecret; }
};

/**
 * Data collection proxy information
 */
class DataCollectionProxy
{
private:
   uint64_t m_serverId;
   uint32_t m_proxyId;
   InetAddress m_address;
   bool m_connected;
   bool m_inUse;

public:
   DataCollectionProxy(uint64_t serverId, uint32_t proxyId, const InetAddress& address);
   DataCollectionProxy(const DataCollectionProxy *src);

   void checkConnection();
   void update(const DataCollectionProxy *src);
   void setInUse(bool inUse) { m_inUse = inUse; }

   ServerObjectKey getKey() const { return ServerObjectKey(m_serverId, m_proxyId); }
   bool isConnected() const { return m_connected; }
   bool isInUse() const { return m_inUse; }
   const InetAddress &getAddress() const { return m_address; }
   uint64_t getServerId() const { return m_serverId; }
   uint32_t getProxyId() const { return m_proxyId; }
};

/**
 * SNMP table column definition
 */
class SNMPTableColumnDefinition
{
private:
   TCHAR m_name[MAX_COLUMN_NAME];
   TCHAR *m_displayName;
   SNMP_ObjectId m_snmpOid;
   uint16_t m_flags;

public:
   SNMPTableColumnDefinition(const NXCPMessage& msg, uint32_t baseId);
   SNMPTableColumnDefinition(DB_RESULT hResult, int row);
   SNMPTableColumnDefinition(const SNMPTableColumnDefinition *src);
   ~SNMPTableColumnDefinition()
   {
      MemFree(m_displayName);
   }

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return (m_displayName != nullptr) ? m_displayName : m_name; }
   uint16_t getFlags() const { return m_flags; }
   int getDataType() const { return TCF_GET_DATA_TYPE(m_flags); }
   const SNMP_ObjectId& getSnmpOid() const { return m_snmpOid; }
   bool isInstanceColumn() const { return (m_flags & TCF_INSTANCE_COLUMN) != 0; }
   bool isConvertSnmpStringToHex() const { return (m_flags & TCF_SNMP_HEX_STRING) != 0; }
};

#ifdef _WIN32

/**
  * Structure to hold certificate information including fingerprint
  */
struct PE_CERT_INFO
{
   BYTE *fingerprint;
   DWORD fingerprintSize;
   LPWSTR subjectName;
   LPWSTR issuerName;

   PE_CERT_INFO()
   {
      fingerprint = nullptr;
      fingerprintSize = 0;
      subjectName = nullptr;
      issuerName = nullptr;
   }

   ~PE_CERT_INFO()
   {
      MemFree(fingerprint);
      MemFree(subjectName);
      MemFree(issuerName);
   }
};

#endif

/**
 * Functions
 */
BOOL Initialize();
void Shutdown();
void Main();

void ConsolePrintf(const TCHAR *format, ...);
void ConsolePrintf2(const TCHAR *format, va_list args);
void DebugPrintf(int level, const TCHAR *format, ...);

void BuildFullPath(const TCHAR *pszFileName, TCHAR *pszFullPath);

bool DownloadConfig(const TCHAR *server);

void AddMetric(const TCHAR *name, LONG (*handler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *),
         const TCHAR *arg, int dataType, const TCHAR *description, bool (*filter)(const TCHAR*, const TCHAR*, AbstractCommSession*));
void AddPushMetric(const TCHAR *name, int dataType, const TCHAR *description);
void AddList(const TCHAR *name, LONG (*handler)(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *), const TCHAR *arg, bool (*filter)(const TCHAR*, const TCHAR*, AbstractCommSession*));
void AddTable(const TCHAR *name, LONG (*handler)(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *),
         const TCHAR *arg, const TCHAR *instanceColumns, const TCHAR *description, int numColumns, NETXMS_SUBAGENT_TABLE_COLUMN *columns,
         bool (*filter)(const TCHAR*, const TCHAR*, AbstractCommSession*));
bool AddExternalMetric(TCHAR *config, bool isList);
bool AddBackgroundExternalMetric(TCHAR *config);
bool AddExternalTable(TCHAR *config);
bool AddExternalTable(ConfigEntry *config);
bool AddExternalStructuredDataProvider(ConfigEntry *config);
uint32_t GetMetricValue(const TCHAR *param, TCHAR *value, AbstractCommSession *session);
uint32_t GetListValue(const TCHAR *param, StringList *value, AbstractCommSession *session);
uint32_t GetTableValue(const TCHAR *param, Table *value, AbstractCommSession *session);
void GetParameterList(NXCPMessage *pMsg);
void GetEnumList(NXCPMessage *pMsg);
void GetTableList(NXCPMessage *pMsg);
void QueryWebService(NXCPMessage* request, shared_ptr<AbstractCommSession> session);
void WebServiceCustomRequest(NXCPMessage* request, shared_ptr<AbstractCommSession> session);
void GetActionList(NXCPMessage *msg);
bool LoadSubAgent(const TCHAR *moduleName);
void UnloadAllSubAgents();
bool ProcessCommandBySubAgent(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session);
void NotifySubAgents(uint32_t code, void *data);
bool AddAction(const TCHAR *name, bool isExternal, const void *arg, uint32_t (*handler)(const shared_ptr<ActionExecutionContext>&), const TCHAR *subAgent, const TCHAR *description);
bool AddActionFromConfig(const TCHAR *config);

void StartExternalMetricProviders();
void StopExternalMetricProviders();
void StartBackgroundMetricCollection();
int GetExternalDataProviderCount();
int GetBackgroundMetricCount();
bool AddMetricProvider(const TCHAR *line);
void AddStructuredMetricProvider(const TCHAR *name, const TCHAR *command, StringObjectMap<StructuredExtractorParameterDefinition> *metricDefenitions,
   StringObjectMap<StructuredExtractorParameterDefinition> *listDefinitions, bool forcePlainTextParser, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description);
LONG GetParameterValueFromExtProvider(const TCHAR *name, TCHAR *buffer);
void ListParametersFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListParametersFromExtProviders(StringList *list);
LONG GetListValueFromExtProvider(const TCHAR *name, StringList *buffer);
void ListListsFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListListsFromExtProviders(StringList *list);
void AddTableProvider(const TCHAR *name, ExternalTableDefinition *definition, uint32_t pollingInterval, uint32_t timeout, const TCHAR *description);
LONG GetTableValueFromExtProvider(const TCHAR *name, Table *table);
void ListTablesFromExtProviders(NXCPMessage *msg, uint32_t *baseId, uint32_t *count);
void ListTablesFromExtProviders(StringList *list);

bool AddExternalSubagent(const TCHAR *config);
void StopExternalSubagentConnectors();
uint32_t GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer);
uint32_t GetTableValueFromExtSubagent(const TCHAR *name, Table *value);
uint32_t GetListValueFromExtSubagent(const TCHAR *name, StringList *value);
uint32_t ExecuteActionByExtSubagent(const TCHAR *name, const StringList& args, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput);
void ListParametersFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListParametersFromExtSubagents(StringList *list);
void ListListsFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListListsFromExtSubagents(StringList *list);
void ListTablesFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListTablesFromExtSubagents(StringList *list);
void ListActionsFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count);
void ListActionsFromExtSubagents(StringList *list);
bool SendMessageToMasterAgent(NXCPMessage *msg);
bool SendRawMessageToMasterAgent(NXCP_MESSAGE *msg);
void ShutdownExtSubagents(bool restart);
void RestartExtSubagents();
void ExecuteAction(const TCHAR *name, const StringList& args);
void ExecuteAction(const NXCPMessage& request, NXCPMessage *response, const shared_ptr<AbstractCommSession>& session);

void RegisterApplicationAgent(const TCHAR *name);
UINT32 GetParameterValueFromAppAgent(const TCHAR *name, TCHAR *buffer);

bool WaitForProcess(const TCHAR *name);

String SubstituteCommandArguments(const TCHAR *cmdTemplate, const TCHAR *param);

uint32_t UpgradeAgent(const TCHAR *pkgFile);

bool IsVNCServerRunning(const InetAddress& addr, uint16_t port);

void PostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp);
void PostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const StringMap &parameters);
void ForwardEvent(NXCPMessage *msg);
void StartEventConnector();

void StartPushConnector();
bool PushData(const TCHAR *parameter, const TCHAR *value, uint32_t objectId, Timestamp timestamp);

void StartControlConnector();
bool SendControlMessage(NXCPMessage *msg);

void StartSessionAgentConnector();
shared_ptr<SessionAgentConnector> AcquireSessionAgentConnector(const TCHAR *sessionName);
shared_ptr<SessionAgentConnector> AcquireSessionAgentConnector(uint32_t sessionId);
void ShutdownSessionAgents(bool restart);
bool IsUserAgentInstalled();
bool GetScreenInfoForUserSession(uint32_t sessionId, uint32_t *width, uint32_t *height, uint32_t *bpp);

uint32_t GenerateMessageId();

void ConfigureDataCollection(uint64_t serverId, const NXCPMessage& request);

bool EnumerateSessions(EnumerationCallbackResult (*callback)(AbstractCommSession *, void* ), void *data);
shared_ptr<AbstractCommSession> FindServerSessionById(uint32_t id);
shared_ptr<AbstractCommSession> FindServerSessionByServerId(uint64_t serverId);
shared_ptr<AbstractCommSession> FindServerSession(bool (*comparator)(AbstractCommSession *, void *), void *userData);
void NotifyConnectedServers(const TCHAR *notificationCode);

void RegisterProblem(int severity, const TCHAR *key, const TCHAR *message);
void UnregisterProblem(const TCHAR *key);

bool LoadConfig(const TCHAR *configSection, const StringBuffer& cmdLineValues, bool firstStart, bool logErrors);

const TCHAR *GetAgentExecutableName();

#ifdef WITH_SYSTEMD
bool RestartService(UINT32 pid);
#endif

#ifdef _WIN32

void InitService();
void InstallService(TCHAR *execName, TCHAR *confFile, int debugLevel);
void RemoveService();
void StartAgentService();
void StopAgentService();
bool WaitForService(DWORD dwDesiredState);
void InstallEventSource(const TCHAR *path);
void RemoveEventSource();

bool ExecuteInAllSessions(const TCHAR *command);

BOOL GetPeCertificateInfo(LPCWSTR filePath, PE_CERT_INFO *certInfo);

#endif

/**
 * Global variables
 */
extern uuid g_agentId;
extern uint32_t g_dwFlags;
extern uint32_t g_failFlags;
extern TCHAR g_szLogFile[];
extern TCHAR g_szSharedSecret[];
extern TCHAR g_szConfigFile[];
extern TCHAR g_szFileStore[];
extern TCHAR g_szConfigServer[];
extern TCHAR g_registrar[];
extern TCHAR g_szListenAddress[];
extern TCHAR g_szConfigIncludeDir[];
extern TCHAR g_szConfigPolicyDir[];
extern TCHAR g_szLogParserDirectory[];
extern TCHAR g_userAgentPolicyDirectory[];
extern TCHAR g_certificateDirectory[];
extern TCHAR g_szDataDirectory[];
extern TCHAR g_masterAgent[];
extern TCHAR g_snmpTrapListenAddress[];
extern uint16_t g_wListenPort;
extern TCHAR g_systemName[];
extern ObjectArray<ServerInfo> g_serverList;
extern int64_t g_agentStartTime;
extern TCHAR g_szPlatformSuffix[];
extern uint32_t g_startupDelay;
extern uint32_t g_dwIdleTimeout;
extern uint32_t g_maxCommSessions;
extern uint32_t g_externalCommandTimeout;
extern uint32_t g_externalMetricTimeout;
extern uint32_t g_externalMetricProviderTimeout;
extern uint32_t g_snmpTimeout;
extern uint16_t g_snmpTrapPort;
extern uint32_t g_longRunningQueryThreshold;
extern uint16_t g_sessionAgentPort;
extern int32_t g_zoneUIN;
extern uint32_t g_tunnelKeepaliveInterval;
extern uint16_t g_syslogListenPort;
extern StringSet g_trustedRootCertificates;
extern shared_ptr_store<Config> g_config;
extern bool g_restartPending;

extern uint32_t g_acceptErrors;
extern uint32_t g_acceptedConnections;
extern uint32_t g_rejectedConnections;

extern SharedObjectArray<CommSession> g_sessions;
extern Mutex g_sessionLock;
extern ThreadPool *g_commThreadPool;
extern ThreadPool *g_executorThreadPool;
extern ThreadPool *g_timerThreadPool;
extern ThreadPool *g_webSvcThreadPool;
extern ObjectQueue<NXCPMessage> g_notificationProcessorQueue;

#ifdef _WIN32
extern TCHAR g_windowsEventSourceName[];
#endif   /* _WIN32 */

#endif   /* _nxagentd_h_ */
