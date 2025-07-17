/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nms_core.h
**
**/

#ifndef _nms_core_h_
#define _nms_core_h_

#ifndef MODULE_NXDBMGR_EXTENSION

#ifdef NXCORE_EXPORTS
#define NXCORE_EXPORTABLE __EXPORT
#define NXCORE_EXPORTABLE_VAR(v)  __EXPORT_VAR(v)
#else
#define NXCORE_EXPORTABLE __IMPORT
#define NXCORE_EXPORTABLE_VAR(v)  __IMPORT_VAR(v)
#endif

#ifdef NXCORE_TEMPLATE_EXPORTS
#define NXCORE_TEMPLATE_EXPORTABLE __EXPORT
#else
#define NXCORE_TEMPLATE_EXPORTABLE __IMPORT
#endif

#define LIBNXCL_NO_DECLARATIONS  1

#include <nms_common.h>

#ifndef _WIN32

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif


#define WSAGetLastError() (errno)

#endif   /* _WIN32 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <openssl/ssl.h>

/**
 * Unique identifier group codes
 */
#define IDG_NETWORK_OBJECT          0
#define IDG_EVENT                   1
#define IDG_ITEM                    2
#define IDG_SNMP_TRAP               3
#define IDG_ACTION                  4
#define IDG_THRESHOLD               5
#define IDG_USER                    6
#define IDG_USER_GROUP              7
#define IDG_ALARM                   8
#define IDG_ALARM_NOTE              9
#define IDG_PACKAGE                 10
#define IDG_BUSINESS_SERVICE_TICKET 11
#define IDG_OBJECT_TOOL             12
#define IDG_SCRIPT                  13
#define IDG_AGENT_CONFIG            14
#define IDG_GRAPH                   15
#define IDG_AUTHTOKEN               16
#define IDG_MAPPING_TABLE           17
#define IDG_DCI_SUMMARY_TABLE       18
#define IDG_ALARM_CATEGORY          19
#define IDG_UA_MESSAGE              20
#define IDG_RACK_ELEMENT            21
#define IDG_PHYSICAL_LINK           22
#define IDG_WEBSVC_DEFINITION       23
#define IDG_OBJECT_CATEGORY         24
#define IDG_GEO_AREA                25
#define IDG_SSH_KEY                 26
#define IDG_OBJECT_QUERY            27
#define IDG_BUSINESS_SERVICE_CHECK  28
#define IDG_BUSINESS_SERVICE_RECORD 29
#define IDG_MAINTENANCE_JOURNAL     30
#define IDG_PACKAGE_DEPLOYMENT_JOB  31

/**** ID functions *****/
bool InitIdTable();
uint32_t CreateUniqueId(int group);
void SaveCurrentFreeId();


//
// Common includes
//

#include <nms_util.h>
#include <dbdrv.h>
#include <nms_cscp.h>
#include <uuid.h>
#include <nxsrvapi.h>
#include <nxqueue.h>
#include <nxsnmp.h>
#include <nxmodule.h>
#include <nxsl.h>
#include <nxdbapi.h>
#include <nddrv.h>
#include <nxcore_smclp.h>
#include <nxproc.h>

/**
 * Server includes
 */
#include "server_console.h"
#include "nms_dcoll.h"
#include "nxcore_winperf.h"
#include "nxcore_schedule.h"
#include "nms_objects.h"
#include "nms_locks.h"
#include "nms_script.h"

/**
 * Common constants and macros
 */
#define DEFAULT_AFFINITY_MASK    0xFFFFFFFF

#define PING_TIME_TIMEOUT     10000

#define DEBUG_TAG_AGENT             _T("node.agent")
#define DEBUG_TAG_AUTOBIND_POLL     _T("poll.autobind")
#define DEBUG_TAG_BIZSVC            _T("bizsvc")
#define DEBUG_TAG_CONF_POLL         _T("poll.conf")
#define DEBUG_TAG_DC_AGENT          _T("dc.agent")
#define DEBUG_TAG_DC_CACHE          _T("dc.cache")
#define DEBUG_TAG_DC_COLLECTOR      _T("dc.collector")
#define DEBUG_TAG_DC_EIP            _T("dc.eip")
#define DEBUG_TAG_DC_MODBUS         _T("dc.modbus")
#define DEBUG_TAG_DC_POLLER         _T("dc.poller")
#define DEBUG_TAG_DC_TEMPLATES      _T("dc.templates")
#define DEBUG_TAG_DC_THRESHOLDS     _T("dc.thresholds")
#define DEBUG_TAG_DC_TRANSFORM      _T("dc.transform")
#define DEBUG_TAG_INSTANCE_POLL     _T("poll.instance")
#define DEBUG_TAG_GEOLOCATION       _T("geolocation")
#define DEBUG_TAG_LOGS              _T("logs")
#define DEBUG_TAG_MAINTENANCE       _T("obj.maint")
#define DEBUG_TAG_OBJECT_INIT       _T("obj.init")
#define DEBUG_TAG_OBJECT_RELATIONS  _T("obj.relations")
#define DEBUG_TAG_OBJECT_LIFECYCLE  _T("obj.lifecycle")
#define DEBUG_TAG_OBJECT_DATA       _T("obj.data")
#define DEBUG_TAG_SMCLP             _T("node.smclp")
#define DEBUG_TAG_SSH               _T("ssh")
#define DEBUG_TAG_STATUS_POLL       _T("poll.status")
#define DEBUG_TAG_TOPOLOGY_POLL     _T("poll.topology")
#define DEBUG_TAG_VNC               _T("vnc")

/**
 * Prefixes for poller messages
 */
#define POLLER_ERROR    _T("\x7F") _T("e")
#define POLLER_WARNING  _T("\x7Fw")
#define POLLER_INFO     _T("\x7Fi")

/**
 * Exit codes for console commands
 */
#define CMD_EXIT_CONTINUE        0
#define CMD_EXIT_CLOSE_SESSION   1
#define CMD_EXIT_SHUTDOWN        2

/**
 * Network discovery mode
 */
#define DISCOVERY_DISABLED       0
#define DISCOVERY_PASSIVE_ONLY   1
#define DISCOVERY_ACTIVE         2

/**
 * Client session flags
 */
#define CSF_TERMINATED           ((uint32_t)0x00000001)
#define CSF_EPP_LOCKED           ((uint32_t)0x00000002)
#define CSF_EPP_UPLOAD           ((uint32_t)0x00000010)
#define CSF_CONSOLE_OPEN         ((uint32_t)0x00000020)
#define CSF_AUTHENTICATED        ((uint32_t)0x00000080)
#define CSF_COMPRESSION_ENABLED  ((uint32_t)0x00000100)
#define CSF_OBJECT_SYNC_FINISHED ((uint32_t)0x00000800)
#define CSF_OBJECTS_OUT_OF_SYNC  ((uint32_t)0x00001000)
#define CSF_TERMINATE_REQUESTED  ((uint32_t)0x00002000)

/**
 * Options for primary IP update on status poll
 */
enum class PrimaryIPUpdateMode
{
   NEVER = 0,
   ALWAYS = 1,
   ON_FAILURE = 2
};

/**
 * Certificate types
 */
enum CertificateType
{
   CERT_TYPE_TRUSTED_CA = 0,
   CERT_TYPE_USER = 1,
   CERT_TYPE_AGENT = 2,
   CERT_TYPE_SERVER = 3
};

/**
 * Certificate operations
 */
enum CertificateOperation
{
   ISSUE_CERTIFICATE = 1,
   REVOKE_CERTIFICATE = 2
};

/**
 * "Reload CRLs" scheduled task
 */
#define RELOAD_CRLS_TASK_ID _T("System.ReloadCRLs")

/**
 * "Execute report" scheduled task
 */
#define EXECUTE_REPORT_TASK_ID _T("Report.Execute")

/**
 * Audit subsystems
 */
#define AUDIT_SECURITY     _T("SECURITY")
#define AUDIT_OBJECTS      _T("OBJECTS")
#define AUDIT_SYSCFG       _T("SYSCFG")
#define AUDIT_CONSOLE      _T("CONSOLE")

#define AUDIT_SYSTEM_SID   (-1)

/**
 * Event handling subsystem definitions
 */
#include "nms_events.h"
#include "nms_actions.h"
#include "nms_alarm.h"

/**
 * New node flags
 */
#define NNF_IS_SNMP           0x0001
#define NNF_IS_AGENT          0x0002
#define NNF_IS_ROUTER         0x0004
#define NNF_IS_BRIDGE         0x0008
#define NNF_IS_PRINTER        0x0010
#define NNF_IS_CDP            0x0020
#define NNF_IS_SONMP          0x0040
#define NNF_IS_LLDP           0x0080
#define NNF_IS_SSH            0x0100

/**
 * Address list element types
 */
enum AddressListElementType
{
   InetAddressListElement_SUBNET = 0,
   InetAddressListElement_RANGE = 1
};

/**
 * SNMP config global ID
 */
#define SNMP_CONFIG_GLOBAL -1

/**
 * Max address list comments length
 */
#define MAX_ADDRESS_LIST_COMMENTS_LEN  256

/**
 * IP address list element
 */
class NXCORE_EXPORTABLE InetAddressListElement
{
private:
   AddressListElementType m_type;
   int32_t m_zoneUIN;
   uint32_t m_proxyId;
   InetAddress m_baseAddress;
   InetAddress m_endAddress;
   TCHAR m_comments[MAX_ADDRESS_LIST_COMMENTS_LEN];

public:
   InetAddressListElement(const NXCPMessage& msg, uint32_t baseId);
   InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr);
   InetAddressListElement(const InetAddress& baseAddr, int maskBits);
   InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr, int32_t zoneUIN, uint32_t proxyId);
   InetAddressListElement(DB_RESULT hResult, int row);

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   bool contains(const InetAddress& addr) const;

   AddressListElementType getType() const { return m_type; }
   const InetAddress& getBaseAddress() const { return m_baseAddress; }
   const InetAddress& getEndAddress() const { return m_endAddress; }
   int32_t getZoneUIN() const { return m_zoneUIN; }
   uint32_t getProxyId() const { return m_proxyId; }
   const TCHAR *getComments() const { return m_comments; }

   String toString() const;
};

/**
 * Mobile device session
 */
class NXCORE_EXPORTABLE MobileDeviceSession
{
private:
   SOCKET m_socket;
   session_id_t m_id;
   uint32_t m_userId;
   uint32_t m_deviceObjectId;
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
	Mutex m_mutexSocketWrite;
	InetAddress m_clientAddr;
	TCHAR m_hostName[256]; // IP address of name of conneced host in textual form
   TCHAR m_userName[MAX_SESSION_NAME];   // String in form login_name@host
   TCHAR m_clientInfo[96];  // Client app info string
   uint32_t m_encryptionRqId;
   uint32_t m_encryptionResult;
   Condition m_condEncryptionSetup;
   uint32_t m_refCount;
	bool m_authenticated;

   void readThread();

   void processRequest(NXCPMessage *request);
   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(uint32_t requestId);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(uint32_t requestId);
   void login(const NXCPMessage& request);
   void updateDeviceInfo(NXCPMessage *request);
   void updateDeviceStatus(NXCPMessage *request);
   void pushData(NXCPMessage *request);

public:
   MobileDeviceSession(SOCKET hSocket, const InetAddress& addr);
   ~MobileDeviceSession();

   void run();

   void sendMessage(const NXCPMessage& msg);

	session_id_t getId() const { return m_id; }
   void setId(session_id_t id) { if (m_id == -1) m_id = id; }
   const TCHAR *getUserName() const { return m_userName; }
   const TCHAR *getClientInfo() const { return m_clientInfo; }
	const TCHAR *getHostName() const { return m_hostName; }
   uint32_t getUserId() const { return m_userId; }
   bool isAuthenticated() const { return m_authenticated; }
   int getCipher() const { return (m_encryptionContext == nullptr) ? -1 : m_encryptionContext->getCipher(); }
};

/**
 * Processing thread starter declaration for client session
 */
#define DECLARE_THREAD_STARTER(func) static void ThreadStarter_##func(void *);

/**
 * Class that stores information about file that will be received
 */
class ServerDownloadFileInfo : public DownloadFileInfo
{
protected:
   std::function<void (const wchar_t*, uint32_t, bool)> m_completionCallback;
   uint32_t m_uploadData;

public:
   ServerDownloadFileInfo(const wchar_t *name, time_t fileModificationTime = 0) : DownloadFileInfo(name, fileModificationTime)
   {
      m_uploadData = 0;
   }
   ServerDownloadFileInfo(const wchar_t *name, std::function<void (const wchar_t*, uint32_t, bool)> completionCallback, time_t fileModificationTime = 0) :
         DownloadFileInfo(name, fileModificationTime), m_completionCallback(completionCallback)
   {
      m_uploadData = 0;
   }
   virtual ~ServerDownloadFileInfo();

   virtual void close(bool success);

   void setUploadData(uint32_t data) { m_uploadData = data; }
   void updatePackageDBInfo(const wchar_t *description, const wchar_t *pkgName, const wchar_t *pkgVersion, const wchar_t *pkgType,
      const wchar_t *platform, const wchar_t *cleanFileName, const wchar_t *command);
};

/**
 * TCP proxy information
 */
struct TcpProxy
{
   shared_ptr<AgentConnectionEx> agentConnection;
   uint32_t agentChannelId;
   uint32_t clientChannelId;
   uint32_t nodeId;

   TcpProxy(const shared_ptr<AgentConnectionEx>& c, uint32_t aid, uint32_t cid, uint32_t nid) : agentConnection(c)
   {
      agentChannelId = aid;
      clientChannelId = cid;
      nodeId = nid;
   }
};

/**
 * Client session console
 */
class NXCORE_EXPORTABLE ClientSessionConsole : public ServerConsole
{
private:
   ClientSession *m_session;
   uint16_t m_messageCode;

protected:
   virtual void write(const TCHAR *text) override;

public:
   ClientSessionConsole(ClientSession *session, uint16_t msgCode = CMD_ADM_MESSAGE);
   virtual ~ClientSessionConsole();
};

/**
 * Agent file transfer
 */
struct AgentFileTransfer
{
   shared_ptr<AgentConnection> connection;
   uint32_t requestId;
   TCHAR key[24];
   TCHAR *fileName;
   Mutex mutex;
   bool active;
   bool failure;
   bool reportProgress;
   time_t lastReportTime;
   uint64_t bytesTransferred;

   AgentFileTransfer(session_id_t sessionId, uint32_t _requestId, const shared_ptr<AgentConnection>& _connection, bool _reportProgress) : connection(_connection), mutex(MutexType::FAST)
   {
      requestId = _requestId;
      _sntprintf(key, 24, _T("FT_%d_%u"), sessionId, _requestId);
      fileName = nullptr;
      active = true;
      failure = false;
      reportProgress = _reportProgress;
      lastReportTime = time(nullptr);
      bytesTransferred = 0;
   }

   ~AgentFileTransfer()
   {
      if (fileName != nullptr)
      {
         _tremove(fileName);
         MemFree(fileName);
      }
   }
};

/**
 * Interface for all kinds of client sessions that provides minimal information about logged in user as well as audit logging functionality
 */
class NXCORE_EXPORTABLE GenericClientSession
{
protected:
   session_id_t m_id;
   uint32_t m_userId;
   uint64_t m_systemAccessRights; // User's system access rights
   TCHAR m_loginName[MAX_USER_NAME];
   TCHAR m_workstation[64];       // IP address or name of connected host in textual form

   GenericClientSession()
   {
      m_userId = 0;
      m_systemAccessRights = 0;
      m_loginName[0] = 0;
      m_workstation[0] = 0;
   }

public:
   virtual ~GenericClientSession() {}

   session_id_t getId() const { return m_id; }
   void setId(session_id_t id) { if (m_id == -1) m_id = id; }

   const TCHAR *getLoginName() const { return m_loginName; }
   uint32_t getUserId() const { return m_userId; }
   uint64_t getSystemAccessRights() const { return m_systemAccessRights; }
   const TCHAR *getWorkstation() const { return m_workstation; }

   bool checkSystemAccessRights(uint64_t requiredAccess) const
   {
      return (m_userId == 0) ? true : ((requiredAccess & m_systemAccessRights) == requiredAccess);
   }

   virtual void writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const = 0;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...) const = 0;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...) const = 0;
};

// Explicit instantiation of template classes
#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE HashMap<uint32_t, ServerDownloadFileInfo>;
template class NXCORE_TEMPLATE_EXPORTABLE HashMap<uint32_t, NXSL_VM>;
template class NXCORE_TEMPLATE_EXPORTABLE ObjectArray<TcpProxy>;
template class NXCORE_TEMPLATE_EXPORTABLE SynchronizedObjectMemoryPool<shared_ptr<AgentFileTransfer>>;
template class NXCORE_TEMPLATE_EXPORTABLE SynchronizedObjectMemoryPool<shared_ptr<ProcessExecutor>>;
template class NXCORE_TEMPLATE_EXPORTABLE SynchronizedHashMap<uint32_t, ServerDownloadFileInfo>;
template class NXCORE_TEMPLATE_EXPORTABLE SharedPointerIndex<AgentFileTransfer>;
template class NXCORE_TEMPLATE_EXPORTABLE SharedPointerIndex<ProcessExecutor>;
#endif

/**
 * Forward declaration for syslog message class
 */
class SyslogMessage;

/**
 * Client login information
 */
struct LoginInfo;

/**
 * Client (user) session
 */
class NXCORE_EXPORTABLE ClientSession : public GenericClientSession
{
private:
   SOCKET m_socket;
   BackgroundSocketPollerHandle *m_socketPoller;
   SocketMessageReceiver *m_messageReceiver;
   LoginInfo *m_loginInfo;
   VolatileCounter m_flags;       // Session flags
	int m_clientType;              // Client system type - desktop, web, mobile, etc.
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
	Mutex m_mutexSocketWrite;
	Mutex m_mutexSendAlarms;
	Mutex m_mutexSendActions;
	Mutex m_mutexSendAuditLog;
	InetAddress m_clientAddr;
	wchar_t m_webServerAddress[64];  // IP address or name of web server for web sessions
	wchar_t m_sessionName[MAX_SESSION_NAME];   // String in form login_name@host
	wchar_t m_clientInfo[96];    // Client app info string
	wchar_t m_language[8];       // Client's desired language
   time_t m_loginTime;
   SynchronizedCountingHashSet<uint32_t> m_openDataCollectionConfigurations; // List of nodes with DCI lists open
   uint32_t m_dwNumRecordsToUpload; // Number of records to be uploaded
   uint32_t m_dwRecordsUploaded;
   EPRule **m_ppEPPRuleList;   // List of loaded EPP rules
   SynchronizedHashMap<uint32_t, ServerDownloadFileInfo> m_downloadFileMap;
   VolatileCounter m_refCount;
   uint32_t m_encryptionRqId;
   uint32_t m_encryptionResult;
   Condition m_condEncryptionSetup;
	ClientSessionConsole *m_console;			// Server console context
	StringList m_soundFileTypes;
	SharedPointerIndex<AgentFileTransfer> m_agentFileTransfers;
	StringSet m_subscriptions;
	Mutex m_subscriptionLock;
	SharedPointerIndex<ProcessExecutor> m_serverCommands;
	ObjectArray<TcpProxy> m_tcpProxyConnections;
	Mutex m_tcpProxyLock;
	VolatileCounter m_tcpProxyChannelId;
	HashSet<uint32_t> m_pendingObjectNotifications;
   Mutex m_pendingObjectNotificationsLock;
   bool m_objectNotificationScheduled;
   uint32_t m_objectNotificationDelay;
   size_t m_objectNotificationBatchSize;
   time_t m_lastScreenshotTime;
   uint32_t m_lastScreenshotObject;
   uint32_t m_scriptExecutorId;
   HashMap<uint32_t, NXSL_VM> m_scriptExecutors;
   Mutex m_scriptExecutorsLock;

   static void socketPollerCallback(BackgroundSocketPollResult pollResult, SOCKET hSocket, ClientSession *session);
   static void terminate(ClientSession *session);
   static EnumerationCallbackResult checkFileTransfer(const uint32_t &key, ServerDownloadFileInfo *fileTransfer,
      std::pair<ClientSession*, IntegerArray<uint32_t>*> *context);

   bool readSocket();
   MessageReceiverResult readMessage(bool allowSocketRead);
   void finalize();

   void processRequest(NXCPMessage *request);
   void processFileTransferMessage(NXCPMessage *msg);
   void sendAgentFileTransferMessage(shared_ptr<AgentFileTransfer> ft, NXCPMessage *msg);
   void agentFileTransferFromFile(shared_ptr<AgentFileTransfer> ft);
   bool sendDataFromCacheFile(AgentFileTransfer *ft);

   void postRawMessageAndDelete(NXCP_MESSAGE *msg);
   void sendRawMessageAndDelete(NXCP_MESSAGE *msg);

   void debugPrintf(int level, const TCHAR *format, ...);

   void setupEncryption(const NXCPMessage& request);
   void login(const NXCPMessage& request);
   uint32_t finalizeLogin(const NXCPMessage& request, NXCPMessage *response);
   uint32_t authenticateUserByPassword(const NXCPMessage& request, LoginInfo *loginInfo);
   uint32_t authenticateUserByCertificate(const NXCPMessage& request, LoginInfo *loginInfo);
   uint32_t authenticateUserBySSOTicket(const NXCPMessage& request, LoginInfo *loginInfo);
   uint32_t authenticateUserByToken(const NXCPMessage& request, LoginInfo *loginInfo);
   void prepare2FAChallenge(const NXCPMessage& request);
   void validate2FAResponse(const NXCPMessage& request);
   void issueAuthToken(const NXCPMessage& request);
   void revokeAuthToken(const NXCPMessage& request);
   void getAuthTokens(const NXCPMessage& request);

   unique_ptr<SharedObjectArray<DCObject>> resolveDCOsByRegex(uint32_t objectId, const TCHAR *objectNameRegex, const TCHAR *dciRegex, bool searchByName);

   void respondToKeepalive(uint32_t requestId)
   {
      NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
      msg.setField(VID_RCC, RCC_SUCCESS);
      postMessage(msg);
   }

   bool getCollectedDataFromDB(const NXCPMessage& request, NXCPMessage *response, const DataCollectionTarget& object, int dciType, HistoricalDataType historicalDataType);

   void forwardToReportingServer(NXCPMessage *request);
   void fileManagerControl(NXCPMessage *request);
   void uploadUserFileToAgent(NXCPMessage *request);

   void sendServerInfo(const NXCPMessage& request);
   void getObjects(const NXCPMessage& request);
   void getSelectedObjects(const NXCPMessage& request);
   void queryObjects(const NXCPMessage& request);
   void queryObjectDetails(const NXCPMessage& request);
   void getObjectQueries(const NXCPMessage& request);
   void modifyObjectQuery(const NXCPMessage& request);
   void deleteObjectQuery(const NXCPMessage& request);
   void getConfigurationVariables(const NXCPMessage& request);
   void getPublicConfigurationVariable(const NXCPMessage& request);
   void setDefaultConfigurationVariableValues(const NXCPMessage& request);
   void setConfigurationVariable(const NXCPMessage& request);
   void deleteConfigurationVariable(const NXCPMessage& request);
   void sendUserDB(const NXCPMessage& request);
   void getSelectedUsers(const NXCPMessage& request);
   void createUser(const NXCPMessage& request);
   void updateUser(const NXCPMessage& request);
   void detachLdapUser(const NXCPMessage& request);
   void deleteUser(const NXCPMessage& request);
   void setPassword(const NXCPMessage& request);
   void validatePassword(const NXCPMessage& request);
   void getAlarmCategories(const NXCPMessage& request);
   void modifyAlarmCategory(const NXCPMessage& request);
   void deleteAlarmCategory(const NXCPMessage& request);
   void getEventTemplates(const NXCPMessage& request);
   void modifyEventTemplate(const NXCPMessage& request);
   void deleteEventTemplate(const NXCPMessage& request);
   void generateEventCode(const NXCPMessage& request);
   void modifyObject(const NXCPMessage& request);
   void enableAnonymousObjectAccess(const NXCPMessage& request);
   void changeObjectMgmtStatus(const NXCPMessage& request);
   void enterMaintenanceMode(const NXCPMessage& request);
   void leaveMaintenanceMode(const NXCPMessage& request);
   void openNodeDCIList(const NXCPMessage& request);
   void closeNodeDCIList(const NXCPMessage& request);
   void modifyNodeDCI(const NXCPMessage& request);
   void copyDCI(const NXCPMessage& request);
   void bulkDCIUpdate(const NXCPMessage& request);
   void getCollectedData(const NXCPMessage& request);
   void getTableCollectedData(const NXCPMessage& request);
	void clearDCIData(const NXCPMessage& request);
	void deleteDCIEntry(const NXCPMessage& request);
	void forceDCIPoll(const NXCPMessage& request);
   void recalculateDCIValues(const NXCPMessage& request);
   void changeDCIStatus(const NXCPMessage& request);
   void getDataCollectionSummary(const NXCPMessage& request);
   void getLastValuesByDciId(const NXCPMessage& request);
   void getTooltipLastValues(const NXCPMessage& request);
   void getTableLastValue(const NXCPMessage& request);
   void getLastValue(const NXCPMessage& request);
   void getActiveThresholds(const NXCPMessage& request);
	void getThresholdSummary(const NXCPMessage& request);
   void getMIBTimestamp(const NXCPMessage& request);
   void getMIBFile(const NXCPMessage& request);
   void createObject(const NXCPMessage& request);
   void changeObjectBinding(const NXCPMessage& request, bool bind);
   void deleteObject(const NXCPMessage& request);
   void getAlarms(const NXCPMessage& request);
   void getAlarm(const NXCPMessage& request);
   void getAlarmEvents(const NXCPMessage& request);
   void acknowledgeAlarm(const NXCPMessage& request);
   void resolveAlarm(const NXCPMessage& request, bool terminate);
   void bulkResolveAlarms(const NXCPMessage& request, bool terminate);
   void deleteAlarm(const NXCPMessage& request);
   void openHelpdeskIssue(const NXCPMessage& request);
   void getHelpdeskUrl(const NXCPMessage& request);
   void unlinkHelpdeskIssue(const NXCPMessage& request);
	void getAlarmComments(const NXCPMessage& request);
	void updateAlarmComment(const NXCPMessage& request);
	void deleteAlarmComment(const NXCPMessage& request);
	void updateAlarmStatusFlow(const NXCPMessage& request);
   void createAction(const NXCPMessage& request);
   void updateAction(const NXCPMessage& request);
   void deleteAction(const NXCPMessage& request);
   void sendAllActions(const NXCPMessage& request);
   void forcedObjectPoll(const NXCPMessage& request);
   void onTrap(const NXCPMessage& request);
   void onWakeUpNode(const NXCPMessage& request);
   void queryMetric(const NXCPMessage& request);
   void queryTable(const NXCPMessage& request);
   void editTrap(const NXCPMessage& request, int operation);
   void lockTrapCfg(const NXCPMessage& request, bool lock);
   void sendAllTraps(const NXCPMessage& request);
   void sendAllTraps2(const NXCPMessage& request);
   void getInstalledPackages(const NXCPMessage& request);
   void installPackage(const NXCPMessage& request);
   void updatePackageMetadata(const NXCPMessage& request);
   void removePackage(const NXCPMessage& request);
   void deployPackage(const NXCPMessage& request);
   void getPackageDeploymentJobs(const NXCPMessage& request);
   void cancelPackageDeploymentJob(const NXCPMessage& request);
   void getParametersList(const NXCPMessage& request);
   void getUserVariable(const NXCPMessage& request);
   void setUserVariable(const NXCPMessage& request);
   void copyUserVariable(const NXCPMessage& request);
   void enumUserVariables(const NXCPMessage& request);
   void deleteUserVariable(const NXCPMessage& request);
   void changeObjectZone(const NXCPMessage& request);
   void readAgentConfigFile(const NXCPMessage& request);
   void writeAgentConfigFile(const NXCPMessage& request);
   void executeAction(const NXCPMessage& request);
   void getObjectTools(const NXCPMessage& request);
   void getObjectToolDetails(const NXCPMessage& request);
   void updateObjectTool(const NXCPMessage& request);
   void deleteObjectTool(const NXCPMessage& request);
   void changeObjectToolStatus(const NXCPMessage& request);
   void generateObjectToolId(const NXCPMessage& request);
   void execTableTool(const NXCPMessage& request);
   void changeSubscription(const NXCPMessage& request);
   void sendServerStats(const NXCPMessage& request);
   void sendScriptList(const NXCPMessage& request);
   void sendScript(const NXCPMessage& request);
   void updateScript(const NXCPMessage& request);
   void renameScript(const NXCPMessage& request);
   void deleteScript(const NXCPMessage& request);
   void getSessionList(const NXCPMessage& request);
   void killSession(const NXCPMessage& request);
   void startSnmpWalk(const NXCPMessage& request);
   void resolveDCINames(const NXCPMessage& request);
   void sendConfigForAgent(const NXCPMessage& request);
   void getAgentConfigurationList(const NXCPMessage& request);
   void getAgentConfiguration(const NXCPMessage& request);
   void updateAgentConfiguration(const NXCPMessage& request);
   void deleteAgentConfiguration(const NXCPMessage& request);
   void swapAgentConfigurations(const NXCPMessage& request);
   void updateObjectComments(const NXCPMessage& request);
   void updateResponsibleUsers(const NXCPMessage& request);
   void pushDCIData(const NXCPMessage& request);
   void getAddrList(const NXCPMessage& request);
   void setAddrList(const NXCPMessage& request);
   void resetComponent(const NXCPMessage& request);
   void getRelatedEventList(const NXCPMessage& request);
   void getDCIScriptList(const NXCPMessage& request);
	void getDCIInfo(const NXCPMessage& request);
   void getDciMeasurementUnits(const NXCPMessage& request);
   void getPerfTabDCIList(const NXCPMessage& request);
   void exportConfiguration(const NXCPMessage& request);
   void importConfiguration(const NXCPMessage& request);
   void importConfigurationFromFile(const NXCPMessage& request);
   void getGraph(const NXCPMessage& request);
	void getGraphList(const NXCPMessage& request);
	void saveGraph(const NXCPMessage& request);
	void deleteGraph(const NXCPMessage& request);
	void queryL2Topology(const NXCPMessage& request);
   void queryIPTopology(const NXCPMessage& request);
   void queryOSPFTopology(const NXCPMessage& request);
   void queryInternalCommunicationTopology(const NXCPMessage& request);
   void getDependentNodes(const NXCPMessage& request);
	void sendNotification(const NXCPMessage& request);
	void getPersistantStorage(const NXCPMessage& request);
	void setPstorageValue(const NXCPMessage& request);
	void deletePstorageValue(const NXCPMessage& request);
	void setConfigCLOB(const NXCPMessage& request);
	void getConfigCLOB(const NXCPMessage& request);
	void registerAgent(const NXCPMessage& request);
	void getServerFile(const NXCPMessage& request);
	void getAgentFile(const NXCPMessage& request);
   void cancelFileMonitoring(const NXCPMessage& request);
	void testDCITransformation(const NXCPMessage& request);
	void getBackgroundTaskState(const NXCPMessage& request);
	void getUserCustomAttribute(const NXCPMessage& request);
	void setUserCustomAttribute(const NXCPMessage& request);
	void openServerLog(const NXCPMessage& request);
	void closeServerLog(const NXCPMessage& request);
	void queryServerLog(const NXCPMessage& request);
	void getServerLogQueryData(const NXCPMessage& request);
   void getServerLogRecordDetails(const NXCPMessage& request);
	void sendDCIThresholds(const NXCPMessage& request);
	void addClusterNode(const NXCPMessage& request);
   void addWirelessDomainController(const NXCPMessage& request);
	void findNodeConnection(const NXCPMessage& request);
	void findMacAddress(const NXCPMessage& request);
	void findIpAddress(const NXCPMessage& request);
	void findHostname(const NXCPMessage& request);
	void sendLibraryImage(const NXCPMessage& request);
	void updateLibraryImage(const NXCPMessage& request);
	void listLibraryImages(const NXCPMessage& request);
	void deleteLibraryImage(const NXCPMessage& request);
	void executeServerCommand(const NXCPMessage& request);
	void stopServerCommand(const NXCPMessage& request);
	void uploadFileToAgent(const NXCPMessage& request);
	void listServerFileStore(const NXCPMessage& request);
	void processConsoleCommand(const NXCPMessage& request);
	void openConsole(const NXCPMessage& request);
	void closeConsole(const NXCPMessage& request);
	void getVlans(const NXCPMessage& request);
	void receiveFile(const NXCPMessage& request);
   void renameFile(const NXCPMessage& request);
	void deleteFile(const NXCPMessage& request);
	void getNetworkPath(const NXCPMessage& request);
	void getNodeComponents(const NXCPMessage& request);
   void getDeviceView(const NXCPMessage& request);
	void getNodeSoftware(const NXCPMessage& request);
   void getNodeHardware(const NXCPMessage& request);
	void getWinPerfObjects(const NXCPMessage& request);
	void getUserSessions(const NXCPMessage& request);
	void listMappingTables(const NXCPMessage& request);
	void getMappingTable(const NXCPMessage& request);
	void updateMappingTable(const NXCPMessage& request);
	void deleteMappingTable(const NXCPMessage& request);
	void getWirelessStations(const NXCPMessage& request);
   void getSummaryTables(const NXCPMessage& request);
   void getSummaryTableDetails(const NXCPMessage& request);
   void modifySummaryTable(const NXCPMessage& request);
   void deleteSummaryTable(const NXCPMessage& request);
   void querySummaryTable(const NXCPMessage& request);
   void queryAdHocSummaryTable(const NXCPMessage& request);
   void getSubnetAddressMap(const NXCPMessage& request);
   void getEffectiveRights(const NXCPMessage& request);
   void getSwitchForwardingDatabase(const NXCPMessage& request);
   void getRoutingTable(const NXCPMessage& request);
   void getArpCache(const NXCPMessage& request);
   void getLocationHistory(const NXCPMessage& request);
   void getScreenshot(const NXCPMessage& request);
   void compileScript(const NXCPMessage& request);
	void executeScript(const NXCPMessage& request);
   void executeLibraryScript(const NXCPMessage& request);
   void executeDashboardScript(const NXCPMessage& request);
   void stopScript(const NXCPMessage& request);
	void resyncAgentDciConfiguration(const NXCPMessage& request);
   void cleanAgentDciConfiguration(const NXCPMessage& request);
   void getSchedulerTaskHandlers(const NXCPMessage& request);
   void getScheduledTasks(const NXCPMessage& request);
   void getScheduledReportingTasks(const NXCPMessage& request);
   void addScheduledTask(const NXCPMessage& request);
   void updateScheduledTask(const NXCPMessage& request);
   void removeScheduledTask(const NXCPMessage& request);
   void getRepositories(const NXCPMessage& request);
   void addRepository(const NXCPMessage& request);
   void modifyRepository(const NXCPMessage& request);
   void deleteRepository(const NXCPMessage& request);
   void getAgentTunnels(const NXCPMessage& request);
   void bindAgentTunnel(const NXCPMessage& request);
   void unbindAgentTunnel(const NXCPMessage& request);
   void setupTcpProxy(const NXCPMessage& request);
   void closeTcpProxy(const NXCPMessage& request);
   void getPredictionEngines(const NXCPMessage& request);
   void getPredictedData(const NXCPMessage& request);
   void expandMacros(const NXCPMessage& request);
   void updatePolicy(const NXCPMessage& request);
   void deletePolicy(const NXCPMessage& request);
   void getPolicyList(const NXCPMessage& request);
   void getPolicy(const NXCPMessage& request);
   void onPolicyEditorClose(const NXCPMessage& request);
   void forceApplyPolicy(const NXCPMessage& request);
   void getMatchingDCI(const NXCPMessage& request);
   void getUserAgentNotification(const NXCPMessage& request);
   void addUserAgentNotification(const NXCPMessage& request);
   void recallUserAgentNotification(const NXCPMessage& request);
   void getNotificationChannels(const NXCPMessage& request);
   void addNotificationChannel(const NXCPMessage& request);
   void updateNotificationChannel(const NXCPMessage& request);
   void removeNotificationChannel(const NXCPMessage& request);
   void renameNotificationChannel(const NXCPMessage& request);
   void getNotificationDrivers(const NXCPMessage& request);
   void startActiveDiscovery(const NXCPMessage& request);
   void getPhysicalLinks(const NXCPMessage& request);
   void updatePhysicalLink(const NXCPMessage& request);
   void deletePhysicalLink(const NXCPMessage& request);
   void getWebServices(const NXCPMessage& request);
   void modifyWebService(const NXCPMessage& request);
   void deleteWebService(const NXCPMessage& request);
   void getObjectCategories(const NXCPMessage& request);
   void modifyObjectCategory(const NXCPMessage& request);
   void deleteObjectCategory(const NXCPMessage& request);
   void getGeoAreas(const NXCPMessage& request);
   void modifyGeoArea(const NXCPMessage& request);
   void deleteGeoArea(const NXCPMessage& request);
   void getNetworkCredentials(const NXCPMessage& request);
   void updateSharedSecretList(const NXCPMessage& request);
   void updateCommunityList(const NXCPMessage& request);
   void updateUsmCredentials(const NXCPMessage& request);
   void updateSshCredentials(const NXCPMessage& request);
   void updateWellKnownPortList(const NXCPMessage& request);
   void findProxyForNode(const NXCPMessage& request);
   void getSshKeys(const NXCPMessage& request);
   void deleteSshKey(const NXCPMessage& request);
   void updateSshKey(const NXCPMessage& request);
   void generateSshKey(const NXCPMessage& request);
   void get2FADrivers(const NXCPMessage& request);
   void get2FAMethods(const NXCPMessage& request);
   void modify2FAMethod(const NXCPMessage& request);
   void rename2FAMethod(const NXCPMessage& request);
   void delete2FAMethod(const NXCPMessage& request);
   void getUser2FABindings(const NXCPMessage& request);
   void modifyUser2FABinding(const NXCPMessage& request);
   void deleteUser2FABinding(const NXCPMessage& request);
   void getBusinessServiceCheckList(const NXCPMessage& request);
   void modifyBusinessServiceCheck(const NXCPMessage& request);
   void deleteBusinessServiceCheck(const NXCPMessage& request);
   void getBusinessServiceUptime(const NXCPMessage& request);
   void getBusinessServiceTickets(const NXCPMessage& request);
   void executeSshCommand(const NXCPMessage& request);
   void findDci(const NXCPMessage& request);
   void getEventRefences(const NXCPMessage& request);
   void readMaintenanceJournal(const NXCPMessage& request);
   void writeMaintenanceJournal(const NXCPMessage& request);
   void updateMaintenanceJournal(const NXCPMessage& request);
   void cloneNetworkMap(const NXCPMessage& request);
   void findVendorByMac(const NXCPMessage& request);
   void getOspfData(const NXCPMessage& request);
   void getAssetManagementSchema(const NXCPMessage& request);
   void createAssetAttribute(const NXCPMessage& request);
   void updateAssetAttribute(const NXCPMessage& request);
   void deleteAssetAttribute(const NXCPMessage& request);
   void setAssetProperty(const NXCPMessage& request);
   void deleteAssetProperty(const NXCPMessage& request);
   void linkAsset(const NXCPMessage& request);
   void unlinkAsset(const NXCPMessage& request);
   void updateNetworkMapElementLocaiton(const NXCPMessage& request);
   void compileMibs(const NXCPMessage& request);
   void openEventProcessingPolicy(const NXCPMessage& request);
   void closeEventProcessingPolicy(const NXCPMessage& request);
   void saveEventProcessingPolicy(const NXCPMessage& request);
   void processEventProcessingPolicyRecord(const NXCPMessage& request);
   void updatePeerInterface(const NXCPMessage& request);
   void clearPeerInterface(const NXCPMessage& request);
   void queryAiAssistant(const NXCPMessage& request);
   void clearAiAssistantChat(const NXCPMessage& request);
   void getSmclpProperties(const NXCPMessage& request);
   void getInterfaceTrafficDcis(const NXCPMessage& request);
   void autoConnectNetworkMapNodes(const NXCPMessage& request);

   void alarmUpdateWorker(Alarm *alarm);
   void sendActionDBUpdateMessage(NXCP_MESSAGE *msg);
   void sendObjectUpdates();

   void finalizeFileTransferToAgent(shared_ptr<AgentConnection> conn, uint32_t requestId);
   void finalizeConfigurationImport(const Config& config, uint32_t flags, NXCPMessage *response);
   uint32_t resolveDCIName(uint32_t nodeId, uint32_t dciId, WCHAR *name, WCHAR *description, WCHAR *tag);

public:
   ClientSession(SOCKET hSocket, const InetAddress& addr);
   virtual ~ClientSession();

   void incRefCount() { InterlockedIncrement(&m_refCount); }
   void decRefCount() { InterlockedDecrement(&m_refCount); }

   bool start();

   void postMessage(const NXCPMessage& msg)
   {
      if (!isTerminated())
         postRawMessageAndDelete(msg.serialize((m_flags & CSF_COMPRESSION_ENABLED) != 0));
   }
   void postMessage(const NXCPMessage *msg)
   {
      postMessage(*msg);
   }
   bool sendMessage(const NXCPMessage& msg);
   bool sendMessage(const NXCPMessage *msg)
   {
      return sendMessage(*msg);
   }
   void sendRawMessage(NXCP_MESSAGE *msg);
   void sendPollerMsg(uint32_t requestIf, const wchar_t *text);
	bool sendFile(const TCHAR *file, uint32_t requestId, off64_t offset, bool allowCompression = true);

   virtual void writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const override;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...) const override;
   virtual void writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...) const override;

   void setSocketPoller(BackgroundSocketPollerHandle *p) { m_socketPoller = p; }

   const wchar_t *getSessionName() const { return m_sessionName; }
   const wchar_t *getClientInfo() const { return m_clientInfo; }
   const wchar_t *getWebServerAddress() const { return m_webServerAddress; }
   uint32_t getFlags() const { return static_cast<uint32_t>(m_flags); }
   bool isAuthenticated() const { return (m_flags & CSF_AUTHENTICATED) != 0; }
   bool isTerminated() const { return (m_flags & (CSF_TERMINATED | CSF_TERMINATE_REQUESTED)) != 0; }
   bool isConsoleOpen() const { return (m_flags & CSF_CONSOLE_OPEN) != 0; }
   bool isCompressionEnabled() const { return (m_flags & CSF_COMPRESSION_ENABLED) != 0; }
   int getCipher() const { return (m_encryptionContext == nullptr) ? -1 : m_encryptionContext->getCipher(); }
	int getClientType() const { return m_clientType; }
   time_t getLoginTime() const { return m_loginTime; }
   bool isSubscribedTo(const wchar_t *channel) const;
   bool isDataCollectionConfigurationOpen(uint32_t objectId) const { return m_openDataCollectionConfigurations.contains(objectId); }

   void runHousekeeper();

   void terminate();
   void notify(uint32_t code, uint32_t data = 0);

   void updateSystemAccessRights();

   void onNewEvent(Event *pEvent);
   void onSyslogMessage(const SyslogMessage *sm);
   void onObjectChange(const shared_ptr<NetObj>& object);
   void onAlarmUpdate(UINT32 dwCode, const Alarm *alarm);
   void onActionDBUpdate(UINT32 dwCode, const Action *action);
   void onLibraryImageChange(const uuid& guid, bool removed = false);
   void processTcpProxyData(AgentConnectionEx *conn, uint32_t agentChannelId, const void *data, size_t size, bool errorIndicator);
   void processTcpProxyAgentDisconnect(AgentConnectionEx *conn);

   void unregisterServerCommand(pid_t taskId);
};

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
   THREAD_POOL_AVERAGE_WAIT_TIME
};

/**
 * Server command execution data
 */
class ServerCommandExecutor : public ProcessExecutor
{
private:
   uint32_t m_requestId;
   ClientSession *m_session;
   StringBuffer m_maskedCommand;

   ServerCommandExecutor(const TCHAR *command, const TCHAR *maskedCommand, bool sendOutput, ClientSession *session, uint32_t requestId);

   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   virtual ~ServerCommandExecutor();

   const TCHAR *getMaskedCommand() const { return m_maskedCommand.cstr(); }

   static shared_ptr<ServerCommandExecutor> createFromMessage(const NXCPMessage& request, Alarm *alarm, ClientSession *session);
};

/**
 * SNMP Trap parameter map object
 */
class SNMPTrapParameterMapping
{
private:
   SNMP_ObjectId *m_objectId;           // Trap OID
   uint32_t m_position;                 // Trap position
   uint32_t m_flags;
   TCHAR *m_description;

public:
   SNMPTrapParameterMapping();
   SNMPTrapParameterMapping(DB_RESULT mapResult, int row);
   SNMPTrapParameterMapping(ConfigEntry *entry);
   SNMPTrapParameterMapping(const NXCPMessage& msg, uint32_t base);
   ~SNMPTrapParameterMapping();

   void fillMessage(NXCPMessage *msg, uint32_t base) const;

   SNMP_ObjectId *getOid() const { return m_objectId; }
   int getPosition() const { return m_position; }
   bool isPositional() const { return m_objectId == nullptr; }

   uint32_t getFlags() const { return m_flags; }

   const TCHAR *getDescription() const { return m_description; }
};

/**
 * SNMP Trap mapping object
 */
class SNMPTrapMapping
{
private:
   uuid m_guid;                   // Trap guid
   uint32_t m_id;                 // Entry ID
   SNMP_ObjectId m_objectId;      // Trap OID
   uint32_t m_eventCode;          // Event code
   TCHAR *m_eventTag;
   TCHAR *m_description;
   TCHAR *m_scriptSource;
   NXSL_Program *m_script;
   ObjectArray<SNMPTrapParameterMapping> m_mappings;

   void compileScript();

public:
   SNMPTrapMapping();
   SNMPTrapMapping(DB_RESULT trapResult, DB_HANDLE hdb, DB_STATEMENT stmt, int row);
   SNMPTrapMapping(const ConfigEntry& entry, const uuid& guid, uint32_t id, uint32_t eventCode, bool nxslV5);
   SNMPTrapMapping(const NXCPMessage& msg);
   ~SNMPTrapMapping();

   void fillMessage(NXCPMessage *msg) const;
   void fillMessage(NXCPMessage *msg, uint32_t base) const;
   bool saveParameterMapping(DB_HANDLE hdb);
   void notifyOnTrapCfgChange(uint32_t code);

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const SNMP_ObjectId& getOid() const { return m_objectId; }
   const SNMPTrapParameterMapping *getParameterMapping(int index) const { return m_mappings.get(index); }
   int getParameterMappingCount() const { return m_mappings.size(); }
   uint32_t getEventCode() const { return m_eventCode; }
   const TCHAR *getEventTag() const { return m_eventTag; }
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getScriptSource() const { return m_scriptSource; }
   const NXSL_Program *getScript() const { return m_script; }
};

/**
 * File download task
 */
class FileDownloadTask
{
private:
   shared_ptr<Node> m_node;
   ClientSession *m_session;
   uint32_t m_requestId;
   TCHAR m_localFile[60];
   TCHAR *m_remoteFile;
   uint64_t m_fileSize;
   uint64_t m_currentSize;
   uint64_t m_maxFileSize;
   bool m_monitor;
   bool m_allowExpansion;

public:
   FileDownloadTask(const shared_ptr<Node>& node, ClientSession *session, uint32_t requestId, const TCHAR *remoteName, bool allowExpansion, uint64_t maxFileSize, bool monitor);
   ~FileDownloadTask();

   void run();
};

/**
 * Structure for Windows event
 */
struct WindowsEvent
{
   time_t timestamp;
   uint32_t  nodeId;
   int32_t zoneUIN;
   TCHAR logName[64];
   time_t originTimestamp;
   TCHAR eventSource[128];
   int eventSeverity;
   int eventCode;
   TCHAR *message;
   TCHAR *rawData;

   WindowsEvent(uint32_t _nodeId, int32_t _zoneUIN, const NXCPMessage& msg);
   ~WindowsEvent();
};

/**
 * Watchdog thread state codes
 */
enum WatchdogState
{
   WATCHDOG_UNKNOWN = -1,
   WATCHDOG_RUNNING = 0,
   WATCHDOG_SLEEPING = 1,
   WATCHDOG_NOT_RESPONDING = 2
};

/**
 * Shutdown reasons
 */
enum class ShutdownReason
{
   OTHER                = 0,
   FROM_LOCAL_CONSOLE   = 1,
   FROM_REMOTE_CONSOLE  = 2,
   BY_SIGNAL            = 3,
   BY_SERVICE_MANAGER   = 4
};

/**
 * Structure for SSH credentials
 */
struct SSHCredentials
{
   TCHAR login[MAX_USER_NAME];
   TCHAR password[MAX_PASSWORD];
   uint32_t keyId;
};

/**
 * Import context
 */
class ImportContext : public StringBuffer
{
public:
   void log(int severity, const TCHAR *function, const TCHAR *message, ...)
   {
      switch(severity)
      {
         case NXLOG_ERROR:
            append(_T("[ERROR] "));
            break;
         case NXLOG_WARNING:
            append(_T("[WARNING] "));
            break;
         default:
            append(_T("[INFO] "));
            break;
      }

      va_list args;
      va_start(args, message);
      appendFormattedStringV(message, args);
      va_end(args);
      append(_T("\n"));

      TCHAR logMessage[1024];
      _sntprintf(logMessage, 1024, _T("%s: %s%s"), function, (severity == NXLOG_ERROR) ? _T("ERROR: ") : _T(""), message);
      va_start(args, message);
      nxlog_debug_tag2(_T("import"), 4, logMessage, args);
      va_end(args);
   }
};

/**
 * Functions
 */
void ConfigPreLoad();
bool NXCORE_EXPORTABLE ConfigReadStr(const wchar_t *variable, wchar_t *buffer, size_t size, const wchar_t *defaultValue);
wchar_t NXCORE_EXPORTABLE *ConfigReadStr(const wchar_t *variable, const wchar_t *defaultValue);
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE hdb, const wchar_t *variable, wchar_t *buffer, size_t size, const wchar_t *defaultValue);
bool NXCORE_EXPORTABLE ConfigReadStrA(const wchar_t *variable, char *buffer, size_t size, const char *defaultValue);
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const wchar_t *variable, char *buffer, size_t size, const char *defaultValue);
char NXCORE_EXPORTABLE *ConfigReadStrUTF8(const wchar_t *variable, const char *defaultValue);
int32_t NXCORE_EXPORTABLE ConfigReadInt(const wchar_t *variable, int32_t defaultValue);
int32_t NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const wchar_t *variable, int32_t defaultValue);
uint32_t NXCORE_EXPORTABLE ConfigReadULong(const wchar_t *variable, uint32_t defaultValue);
int64_t NXCORE_EXPORTABLE ConfigReadInt64(const wchar_t *variable, int64_t defaultValue);
uint64_t NXCORE_EXPORTABLE ConfigReadUInt64(const wchar_t *variable, uint64_t defaultValue);
bool NXCORE_EXPORTABLE ConfigReadBoolean(const wchar_t *variable, bool defaultValue);
bool NXCORE_EXPORTABLE ConfigReadByteArray(const wchar_t *variable, int *buffer, size_t size, int defaultElementValue);
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *variable, const TCHAR *value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *variable, int32_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *variable, uint32_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt64(const TCHAR *variable, int64_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteUInt64(const TCHAR *variable, uint64_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *variable, int *value, size_t size, bool create, bool isVisible = true, bool needRestart = false);
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *varariable, const TCHAR *defaultValue);
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *variable, const TCHAR *value, bool create);
bool NXCORE_EXPORTABLE ConfigDelete(const wchar_t *variable);

void MetaDataPreLoad();
bool NXCORE_EXPORTABLE MetaDataReadStr(const wchar_t *variable, wchar_t *buffer, int size, const wchar_t *defaultValue);
int32_t NXCORE_EXPORTABLE MetaDataReadInt32(const wchar_t *variable, int32_t defaultValue);
bool NXCORE_EXPORTABLE MetaDataWriteStr(const wchar_t *variable, const wchar_t *value);
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const wchar_t *variable, int32_t value);

void NXCORE_EXPORTABLE FindConfigFile();
bool NXCORE_EXPORTABLE LoadConfig(int *debugLevel);

bool LockDatabase(InetAddress *lockAddr, TCHAR *lockInfo);
void NXCORE_EXPORTABLE UnlockDatabase();

void NXCORE_EXPORTABLE Shutdown();
void NXCORE_EXPORTABLE FastShutdown(ShutdownReason reason);
bool NXCORE_EXPORTABLE Initialize();
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *);
void NXCORE_EXPORTABLE ShutdownDatabase();
void NXCORE_EXPORTABLE InitiateShutdown(ShutdownReason reason);

int ProcessConsoleCommand(const wchar_t *command, ServerConsole *console);

void SaveObjects(DB_HANDLE hdb, uint32_t watchdogId, bool saveRuntimeData);

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query);
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values);
void QueueIDataInsert(time_t timestamp, uint32_t nodeId, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, DCObjectStorageClass storageClass);
void QueueRawDciDataUpdate(time_t timestamp, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, time_t cacheTimestamp, bool anomalyDetected);
void QueueRawDciDataDelete(uint32_t dciId);
int64_t GetIDataWriterQueueSize();
int64_t GetRawDataWriterQueueSize();
uint64_t GetRawDataWriterMemoryUsage();
void StartDBWriter();
void StopDBWriter();
void OnDBWriterMaxQueueSizeChange();
void ClearDBWriterData(ServerConsole *console, const TCHAR *component);

void PerfDataStorageRequest(DCItem *dci, time_t timestamp, const TCHAR *value);
void PerfDataStorageRequest(DCTable *dci, time_t timestamp, Table *value);

bool SnmpTestRequest(SNMP_Transport *snmp, const StringList &testOids, bool separateRequests);
SNMP_Transport *SnmpCheckCommSettings(uint32_t snmpProxy, const InetAddress& ipAddr, SNMP_Version *version,
         uint16_t originalPort, SNMP_SecurityContext *originalContext, const StringList &customTestOids,
         int32_t zoneUIN, bool initialDiscovery);
unique_ptr<StringList> SnmpGetKnownCommunities(int32_t zoneUIN);

bool SSHCheckConnection(const shared_ptr<Node>& proxyNode, const InetAddress& addr, uint16_t port, const TCHAR *login, const TCHAR *password, uint32_t keyId);
bool SSHCheckConnection(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port, const TCHAR *login, const TCHAR *password, uint32_t keyId);
bool SSHCheckCommSettings(uint32_t proxyNodeId, const InetAddress& addr, int32_t zoneUIN, SSHCredentials *selectedCredentials, uint16_t *selectedPort);

bool VNCCheckConnection(Node *proxyNode, const InetAddress& addr, uint16_t port);
bool VNCCheckConnection(uint32_t proxyNodeId, const InetAddress& addr, uint16_t port);
bool VNCCheckCommSettings(uint32_t proxyNodeId, const InetAddress& addr, int32_t zoneUIN, uint16_t *selectedPort);

void InitLocalNetInfo();

shared_ptr<ArpCache> GetLocalArpCache();
InterfaceList *GetLocalInterfaceList();

shared_ptr<RoutingTable> SnmpGetRoutingTable(SNMP_Transport *snmp, const Node& node);

void LoadNetworkDeviceDrivers();
NetworkDeviceDriver *FindDriverForNode(Node *node, SNMP_Transport *snmpTransport);
NetworkDeviceDriver *FindDriverForNode(const TCHAR *name, const SNMP_ObjectId& snmpObjectId, const TCHAR *defaultDriver, SNMP_Transport *snmpTransport);
NetworkDeviceDriver *FindDriverByName(const TCHAR *name);
void AddDriverSpecificOids(StringList *list);
void PrintNetworkDeviceDriverList(ServerConsole *console);

void LoadNotificationChannelDrivers();
void LoadNotificationChannels();
void ShutdownNotificationChannels();
void SendNotification(const TCHAR *name, TCHAR *recipient, const TCHAR *subject, const TCHAR *message, uint32_t eventCode, uint64_t eventId, const uuid& ruleId);
void GetNotificationChannels(NXCPMessage *msg);
void GetNotificationDrivers(NXCPMessage *msg);
char *GetNotificationChannelConfiguration(const TCHAR *name);
bool IsNotificationChannelExists(const TCHAR *name);
void CreateNotificationChannelAndSave(const TCHAR *name, const TCHAR *description, const TCHAR *driverName, char *configuration);
void UpdateNotificationChannel(const TCHAR *name, const TCHAR *description, const TCHAR *driverName, char *configuration);
void RenameNotificationChannel(TCHAR *name, TCHAR *newName);
bool DeleteNotificationChannel(const TCHAR *name);

bool LookupDevicePortLayout(const SNMP_ObjectId& objectId, NDD_MODULE_LAYOUT *layout);

void CheckForMgmtNode();
shared_ptr<Node> NXCORE_EXPORTABLE PollNewNode(NewNodeData *newNodeData);

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*callback)(ClientSession*, void*), void *context);
template <typename C> static inline void EnumerateClientSessions(void (*callback)(ClientSession *, C *), C *context)
{
   EnumerateClientSessions(reinterpret_cast<void (*)(ClientSession*, void*)>(callback), context);
}
template <typename C> static inline void EnumerateClientSessions(void (*callback)(ClientSession *, const C *), const C *context)
{
   EnumerateClientSessions(reinterpret_cast<void (*)(ClientSession*, void*)>(callback), const_cast<C*>(context));
}
void NXCORE_EXPORTABLE EnumerateClientSessions(std::function<void(ClientSession*)> callback);

void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, std::function<bool (ClientSession*)> filter);
void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, const wchar_t *channel = nullptr);
void NXCORE_EXPORTABLE NotifyClientSessions(uint32_t code, uint32_t data, const wchar_t *channel = nullptr);
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, uint32_t code, uint32_t data);
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, NXCPMessage *data);
void NXCORE_EXPORTABLE NotifyClientsOnGraphUpdate(const NXCPMessage& msg, uint32_t graphId);
void NotifyClientsOnPolicyUpdate(const NXCPMessage& msg, const Template& object);
void NotifyClientsOnPolicyDelete(uuid guid, const Template& object);
void NotifyClientsOnBusinessServiceCheckUpdate(const NetObj& service, const shared_ptr<BusinessServiceCheck>& check);
void NotifyClientsOnBusinessServiceCheckDelete(const NetObj& service, uint32_t checkId);
void NotifyClientsOnDCIUpdate(const DataCollectionOwner& object, DCObject *dco);
void NotifyClientsOnDCIDelete(const DataCollectionOwner& object, uint32_t dcoId);
void NotifyClientsOnDCIStatusChange(const DataCollectionOwner& object, uint32_t dcoId, int status);
void NotifyClientsOnDCIUpdate(const NXCPMessage& msg, const NetObj& object);
void NotifyClientsOnThresholdChange(UINT32 objectId, UINT32 dciId, UINT32 thresholdId, const TCHAR *instance, ThresholdCheckResult change);
int GetSessionCount(bool includeSystemAccount, bool includeNonAuthenticated, int typeFilter, const TCHAR *loginFilter);
bool IsLoggedIn(uint32_t userId);
bool NXCORE_EXPORTABLE KillClientSession(session_id_t id);
void NXCORE_EXPORTABLE CloseOtherSessions(uint32_t userId, session_id_t thisSession);

void GetSysInfoStr(TCHAR *buffer, int nMaxSize);
InetAddress GetLocalIPAddress();
bool IsLocalIPAddress(const InetAddress& addr);

InetAddress NXCORE_EXPORTABLE ResolveHostName(int32_t zoneUIN, const wchar_t *hostname, int afHint = AF_UNSPEC);
bool EventNameResolver(const wchar_t *name, uint32_t *code);

bool NXCORE_EXPORTABLE SendMagicPacket(const InetAddress& ipAddr, const MacAddress& macAddr, int count);
StringList NXCORE_EXPORTABLE SplitCommandLine(const wchar_t *command);

void SendTrapMappingsToClient(ClientSession *session, uint32_t requestId);
void CreateTrapMappingMessage(NXCPMessage *msg);
uint32_t CreateNewTrapMapping(uint32_t *trapId);
uint32_t UpdateTrapMappingFromMsg(const NXCPMessage& msg);
uint32_t DeleteTrapMapping(uint32_t id);
void CreateTrapMappingExportRecord(TextFileWriter& xml, uint32_t id);
uint32_t ResolveTrapMappingGuid(const uuid& guid);
void AddTrapMappingToList(const shared_ptr<SNMPTrapMapping>& tm);
shared_ptr<SNMPTrapMapping> FindBestMatchTrapMapping(const SNMP_ObjectId& oid);

bool NXCORE_EXPORTABLE IsTableTool(uint32_t toolId);
bool NXCORE_EXPORTABLE CheckObjectToolAccess(uint32_t toolId, uint32_t userId);
uint32_t ExecuteTableTool(uint32_t toolId, const shared_ptr<Node>& node, uint32_t requestId, ClientSession *session);
uint32_t DeleteObjectToolFromDB(uint32_t toolId);
uint32_t ChangeObjectToolStatus(uint32_t toolId, bool enabled);
uint32_t UpdateObjectToolFromMessage(const NXCPMessage& msg);
void CreateObjectToolExportRecord(TextFileWriter& xml, uint32_t id);
bool ImportObjectTool(ConfigEntry *config, bool overwrite, ImportContext *context);
uint32_t GetObjectToolsIntoMessage(NXCPMessage *msg, uint32_t userId, bool fullAccess);
uint32_t GetObjectToolDetailsIntoMessage(uint32_t toolId, NXCPMessage *msg);
json_t NXCORE_EXPORTABLE *GetObjectToolsIntoJSON(uint32_t userId, bool fullAccess);

uint32_t ModifySummaryTable(const NXCPMessage& msg, uint32_t *newId);
uint32_t NXCORE_EXPORTABLE DeleteSummaryTable(uint32_t tableId);
Table NXCORE_EXPORTABLE *QuerySummaryTable(uint32_t tableId, SummaryTable *adHocDefinition, uint32_t baseObjectId, uint32_t userId, uint32_t *rcc);
bool CreateSummaryTableExportRecord(uint32_t id, TextFileWriter& xml);
bool ImportSummaryTable(ConfigEntry *config, bool overwrite, ImportContext *context, bool nxslV5);
json_t NXCORE_EXPORTABLE *GetSummaryTablesList();

void FullCommunityListToMessage(uint32_t userId, NXCPMessage *msg);
void ZoneCommunityListToMessage(int32_t zoneUIN, NXCPMessage *msg);
uint32_t UpdateCommunityList(const NXCPMessage& request, int32_t zoneUIN);

void FullUsmCredentialsListToMessage(uint32_t userId, NXCPMessage *msg);
void ZoneUsmCredentialsListToMessage(int32_t zoneUIN, NXCPMessage *msg);
uint32_t UpdateUsmCredentialsList(const NXCPMessage& request, int32_t zoneUIN);

void FullAgentSecretListToMessage(uint32_t userId, NXCPMessage *msg);
void ZoneAgentSecretListToMessage(int32_t zoneUIN, NXCPMessage *msg);
uint32_t UpdateAgentSecretList(const NXCPMessage& request, int32_t zoneUIN);

void FullSSHCredentialsListToMessage(uint32_t userId, NXCPMessage *msg);
void ZoneSSHCredentialsListToMessage(int32_t zoneUIN, NXCPMessage *msg);
uint32_t UpdateSSHCredentials(const NXCPMessage& request, int32_t zoneUIN);
StructArray<SSHCredentials> GetSSHCredentials(int32_t zoneUIN);

void FullWellKnownPortListToMessage(const TCHAR *tag, uint32_t userId, NXCPMessage *msg);
void ZoneWellKnownPortListToMessage(const TCHAR *tag, int32_t zoneUIN, NXCPMessage *msg);
uint32_t UpdateWellKnownPortList(const NXCPMessage& request, const TCHAR *tag, int32_t zoneUIN);
IntegerArray<uint16_t> GetWellKnownPorts(const TCHAR *tag, int32_t zoneUIN);

void ReinitializeSyslogParser();
void OnSyslogConfigurationChange(const TCHAR *name, const TCHAR *value);

void InitializeWindowsEventParser();
void OnWindowsEventsConfigurationChange(const TCHAR *name, const TCHAR *value);

void EscapeString(StringBuffer &str);

void InitAuditLog();
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithValues(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, va_list args);

uint32_t ImportConfig(const Config& config, uint32_t flags, StringBuffer **log);
void ReportConfigurationError(const wchar_t *subsystem, const wchar_t *tag, const wchar_t *descriptionFormat, ...);

X509 *CertificateFromLoginMessage(const NXCPMessage& msg);
bool ValidateUserCertificate(X509 *cert, const TCHAR *login, const BYTE *challenge, const BYTE *signature,
      size_t sigLen, CertificateMappingMethod mappingMethod, const TCHAR *mappingData);
void ReloadCertificates();
bool GetServerCertificateCountry(TCHAR *buffer, size_t size);
bool GetServerCertificateOrganization(TCHAR *buffer, size_t size);
X509 *IssueCertificate(X509_REQ *request, const char *ou, const char *cn, int days);
void LogCertificateAction(CertificateOperation operation, uint32_t userId, uint32_t nodeId, const uuid& nodeGuid, CertificateType type, X509 *cert);
void LogCertificateAction(CertificateOperation operation, uint32_t userId, uint32_t nodeId, const uuid& nodeGuid, CertificateType type, const TCHAR *subject, int32_t serial);
String GetServerCertificateExpirationDate();
int GetServerCertificateDaysUntilExpiration();
time_t GetServerCertificateExpirationTime();
bool IsServerCertificateLoaded();

#ifndef _WIN32
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);
#endif   /* not _WIN32 */

json_t NXCORE_EXPORTABLE *GetServerStats();

void DumpClientSessions(ServerConsole *console);
void DumpMobileDeviceSessions(CONSOLE_CTX console);
void ShowServerStats(CONSOLE_CTX console);
void ShowQueueStats(ServerConsole *console, const Queue *queue, const TCHAR *name);
void ShowQueueStats(ServerConsole *console, int64_t size, const TCHAR *name);
void ShowThreadPoolPendingQueue(ServerConsole *console, ThreadPool *p, const TCHAR *name);
void ShowThreadPool(ServerConsole *console, const TCHAR *p);
DataCollectionError GetThreadPoolStat(ThreadPoolStat stat, const TCHAR *param, TCHAR *value);
void DumpProcess(CONSOLE_CTX console);

#define GRAPH_FLAG_TEMPLATE 1

uint32_t CheckGraphAccess(uint32_t graphId, uint32_t userId, uint32_t requiredAccess);
void FillGraphListMsg(NXCPMessage *msg, uint32_t userId, bool templageGraphs, uint32_t graphId = 0);
uint32_t SaveGraph(const NXCPMessage& request, uint32_t userId, uint32_t *assignedId);
uint32_t DeleteGraph(uint32_t graphId, uint32_t userId);

const TCHAR NXCORE_EXPORTABLE *CountryAlphaCode(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CountryName(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CurrencyAlphaCode(const TCHAR *numericCode);
int NXCORE_EXPORTABLE CurrencyExponent(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CurrencyName(const TCHAR *code);

void NXCORE_EXPORTABLE RegisterComponent(const TCHAR *id);
bool NXCORE_EXPORTABLE IsComponentRegistered(const TCHAR *id);
json_t NXCORE_EXPORTABLE *ComponentsToJson();

bool NXCORE_EXPORTABLE IsCommand(const TCHAR *cmdTemplate, const TCHAR *str, int minChars);

ObjectArray<InetAddressListElement> *LoadServerAddressList(int listType);

bool ExecuteSQLCommandFile(const TCHAR *filePath, DB_HANDLE hdb);

/**
 * Watchdog API
 */
void WatchdogInit();
void WatchdogShutdown();
uint32_t WatchdogAddThread(const TCHAR *name, time_t notifyInterval);
void WatchdogNotify(uint32_t id);
void WatchdogStartSleep(uint32_t id);
void WatchdogPrintStatus(CONSOLE_CTX console);
WatchdogState WatchdogGetState(const TCHAR *name);
void WatchdogGetThreads(StringList *out);

/**
 * NXSL script functions
 */
uint32_t UpdateScript(const NXCPMessage& request, uint32_t *scriptId, ClientSession *session);
uint32_t RenameScript(uint32_t scriptId, const TCHAR *newName);
uint32_t DeleteScript(uint32_t scriptId);

/**
 * Address range scan functions
 */
void ScanAddressRangeICMP(const InetAddress& from, const InetAddress& to, void (*callback)(const InetAddress&, int32_t, const Node*, uint32_t, const TCHAR*, ServerConsole*, void*), ServerConsole *console, void *context);

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, uint32_t id, const TCHAR * const *columns);
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, const TCHAR *id, const TCHAR * const *columns);

/**
 * Managing file monitors
 */
void AddFileMonitor(Node *node, const shared_ptr<AgentConnection>& conn, ClientSession *session, const TCHAR *agentId, const uuid& clientId);
bool IsFileMonitorActive(uint32_t nodeId, const TCHAR *agentId);
bool RemoveFileMonitorsByAgentId(const TCHAR *agentId, session_id_t sessionId = -1);
bool RemoveFileMonitorByClientId(const uuid& clientId, session_id_t sessionId = -1);
void RemoveFileMonitorsBySessionId(session_id_t sessionId);
void RemoveFileMonitorsByNodeId(uint32_t nodeId);
unique_ptr<StructArray<std::pair<ClientSession*, uuid>>> FindFileMonitoringSessions(const TCHAR *agentId);

/**
 * License problem
 */
struct NXCORE_EXPORTABLE LicenseProblem
{
   uint32_t id;
   time_t timestamp;
   TCHAR component[MAX_OBJECT_NAME];
   TCHAR type[MAX_OBJECT_NAME];
   TCHAR *description;

   LicenseProblem(uint32_t id, const TCHAR *component, const TCHAR *type, const TCHAR *description)
   {
      this->id = id;
      this->timestamp = time(nullptr);
      _tcslcpy(this->component, component, MAX_OBJECT_NAME);
      _tcslcpy(this->type, type, MAX_OBJECT_NAME);
      this->description = MemCopyString(description);
   }

   ~LicenseProblem()
   {
      MemFree(description);
   }
};

uint32_t NXCORE_EXPORTABLE RegisterLicenseProblem(const TCHAR *component, const TCHAR *type, const TCHAR *description);
void NXCORE_EXPORTABLE UnregisterLicenseProblem(uint32_t id);
void NXCORE_EXPORTABLE UnregisterLicenseProblem(const TCHAR *component, const TCHAR *type);
void NXCORE_EXPORTABLE FillLicenseProblemsMessage(NXCPMessage *msg);
time_t NXCORE_EXPORTABLE GetLicenseProblemTimestamp(uint32_t id);

/**
 * Downtime information
 */
struct DowntimeInfo
{
   uint32_t objectId;
   uint32_t totalDowntime;  // number of seconds in downtime
   double uptimePct;  // uptime percentage
};

/**
 * Calculate downtime
 */
StructArray<DowntimeInfo> CalculateDowntime(uint32_t objectId, time_t periodStart, time_t periodEnd, const TCHAR *tag);


/**********************
 * Distance calculation
 **********************/

/**
 * Class stores object and distance to it from provided node
 */
struct ObjectsDistance
{
   shared_ptr<NetObj> object;
   int distance;

   ObjectsDistance(const shared_ptr<NetObj>& _object, int _distance) : object(_object)
   {
      distance = _distance;
   }
};

/**
 * Calculate nearest objects from current one
 * Object ref count will be automatically decreased on array delete
 */
ObjectArray<ObjectsDistance> *FindNearestObjects(uint32_t currObjectId, int maxDistance,
         bool (* filter)(NetObj *object, void *context), void *context, int (* calculateRealDistance)(GeoLocation &loc1, GeoLocation &loc2));

/**
 * Global variables
 */
extern NXCORE_EXPORTABLE_VAR(TCHAR g_szConfigFile[]);
extern NXCORE_EXPORTABLE_VAR(TCHAR g_szLogFile[]);
extern uint32_t g_logRotationMode;
extern uint64_t g_maxLogSize;
extern uint32_t g_logHistorySize;
extern TCHAR g_szDailyLogFileSuffix[64];
extern NXCORE_EXPORTABLE_VAR(TCHAR g_szDumpDir[]);
extern NXCORE_EXPORTABLE_VAR(TCHAR g_szListenAddress[]);
#ifndef _WIN32
extern NXCORE_EXPORTABLE_VAR(TCHAR g_szPIDFile[]);
#endif
extern NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdDataDir[]);
extern NXCORE_EXPORTABLE_VAR(TCHAR g_netxmsdLibDir[]);
extern NXCORE_EXPORTABLE_VAR(uint32_t g_processAffinityMask);
extern NXCORE_EXPORTABLE_VAR(uint64_t g_serverId);
extern RSA_KEY g_serverKey;
extern uint32_t g_icmpPingSize;
extern uint32_t g_icmpPingTimeout;
extern uint32_t g_auditFlags;
extern time_t g_serverStartTime;
extern uint32_t g_agentCommandTimeout;
extern uint32_t g_agentRestartWaitTime;
extern uint32_t g_thresholdRepeatInterval;
extern uint32_t g_requiredPolls;
extern uint32_t g_offlineDataRelevanceTime;
extern int32_t g_instanceRetentionTime;
extern uint32_t g_snmpTrapStormCountThreshold;
extern uint32_t g_snmpTrapStormDurationThreshold;
extern uint32_t g_pollsBetweenPrimaryIpUpdate;
extern PrimaryIPUpdateMode g_primaryIpUpdateMode;
extern char g_snmpCodepage[16];

extern wchar_t g_szDbDriver[];
extern wchar_t g_szDbDrvParams[];
extern wchar_t g_szDbServer[];
extern wchar_t g_szDbLogin[];
extern wchar_t g_szDbPassword[];
extern wchar_t g_szDbName[];
extern wchar_t g_szDbSchema[];
extern DB_DRIVER g_dbDriver;
extern VolatileCounter64 g_idataWriteRequests;
extern uint64_t g_rawDataWriteRequests;
extern VolatileCounter64 g_otherWriteRequests;

struct DELAYED_SQL_REQUEST;
extern ObjectQueue<DELAYED_SQL_REQUEST> g_dbWriterQueue;

extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_mainThreadPool);
extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_clientThreadPool);
extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_mobileThreadPool);

extern wchar_t g_startupSqlScriptPath[];
extern wchar_t g_dbSessionSetupSqlScriptPath[];

#endif   /* MODULE_NXDBMGR_EXTENSION */

#endif   /* _nms_core_h_ */
