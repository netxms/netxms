/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#ifdef _WITH_ENCRYPTION
#include <openssl/ssl.h>
#endif

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
#include "nms_users.h"
#include "nxcore_winperf.h"
#include "nms_objects.h"
#include "nms_locks.h"
#include "nms_topo.h"
#include "nms_script.h"
#include "nxcore_jobs.h"
#include "nxcore_schedule.h"
#ifdef WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Common constants and macros
 */
#define MAX_LINE_SIZE            4096
#define GROUP_FLAG_BIT           ((UINT32)0x80000000)
#define CHECKPOINT_SNMP_PORT     260
#define DEFAULT_AFFINITY_MASK    0xFFFFFFFF

#define MAX_DEVICE_SESSIONS   256

#define PING_TIME_TIMEOUT     10000

#define DEBUG_TAG_AGENT             _T("node.agent")
#define DEBUG_TAG_CONF_POLL         _T("poll.conf")
#define DEBUG_TAG_GEOLOCATION       _T("geolocation")
#define DEBUG_TAG_STATUS_POLL       _T("poll.status")

/**
 * Prefixes for poller messages
 */
#define POLLER_ERROR    _T("\x7F") _T("e")
#define POLLER_WARNING  _T("\x7Fw")
#define POLLER_INFO     _T("\x7Fi")

/**
 * Unique identifier group codes
 */
#define IDG_NETWORK_OBJECT    0
#define IDG_EVENT             1
#define IDG_ITEM              2
#define IDG_SNMP_TRAP         3
#define IDG_JOB               4
#define IDG_ACTION            5
#define IDG_THRESHOLD         6
#define IDG_USER              7
#define IDG_USER_GROUP        8
#define IDG_ALARM             9
#define IDG_ALARM_NOTE        10
#define IDG_PACKAGE           11
#define IDG_SLM_TICKET        12
#define IDG_OBJECT_TOOL       13
#define IDG_SCRIPT            14
#define IDG_AGENT_CONFIG      15
#define IDG_GRAPH             16
#define IDG_DCT_COLUMN        17
#define IDG_MAPPING_TABLE     18
#define IDG_DCI_SUMMARY_TABLE 19
#define IDG_SCHEDULED_TASK    20
#define IDG_ALARM_CATEGORY    21
#define IDG_UA_MESSAGE        22
#define IDG_RACK_ELEMENT      23
#define IDG_PHYSICAL_LINK     24
#define IDG_WEBSVC_DEFINITION 25
#define IDG_OBJECT_CATEGORIES 26
#define IDG_GEO_AREAS         27
#define IDG_SSH_KEYS          28

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
#define CSF_USER_DB_LOCKED       ((uint32_t)0x00000008)
#define CSF_EPP_UPLOAD           ((uint32_t)0x00000010)
#define CSF_CONSOLE_OPEN         ((uint32_t)0x00000020)
#define CSF_AUTHENTICATED        ((uint32_t)0x00000080)
#define CSF_COMPRESSION_ENABLED  ((uint32_t)0x00000100)
#define CSF_RECEIVING_MAP_DATA   ((uint32_t)0x00000200)
#define CSF_SYNC_OBJECT_COMMENTS ((uint32_t)0x00000400)
#define CSF_OBJECT_SYNC_FINISHED ((uint32_t)0x00000800)
#define CSF_OBJECTS_OUT_OF_SYNC  ((uint32_t)0x00001000)
#define CSF_TERMINATE_REQUESTED  ((uint32_t)0x00002000)

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
 * Source type for discovered address
 */
enum DiscoveredAddressSourceType
{
   DA_SRC_ARP_CACHE = 0,
   DA_SRC_ROUTING_TABLE = 1,
   DA_SRC_AGENT_REGISTRATION = 2,
   DA_SRC_SNMP_TRAP = 3,
   DA_SRC_SYSLOG = 4,
   DA_SRC_ACTIVE_DISCOVERY = 5
};

/**
 * Discovered address information
 */
struct DiscoveredAddress
{
   InetAddress ipAddr;
   int32_t zoneUIN;
	bool ignoreFilter;
	BYTE bMacAddr[MAC_ADDR_LENGTH];
	DiscoveredAddressSourceType sourceType;
   UINT32 sourceNodeId;
};

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
   UINT32 m_proxyId;
   InetAddress m_baseAddress;
   InetAddress m_endAddress;
   TCHAR m_comments[MAX_ADDRESS_LIST_COMMENTS_LEN];

public:
   InetAddressListElement(NXCPMessage *msg, UINT32 baseId);
   InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr);
   InetAddressListElement(const InetAddress& baseAddr, int maskBits);
   InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr, int32_t zoneUIN, UINT32 proxyId);
   InetAddressListElement(DB_RESULT hResult, int row);

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;

   bool contains(const InetAddress& addr) const;

   AddressListElementType getType() const { return m_type; }
   const InetAddress& getBaseAddress() const { return m_baseAddress; }
   const InetAddress& getEndAddress() const { return m_endAddress; }
   int32_t getZoneUIN() const { return m_zoneUIN; }
   UINT32 getProxyId() const { return m_proxyId; }
   const TCHAR *getComments() const { return m_comments; }

   String toString() const;
};

/**
 * Data update structure for client sessions
 */
typedef struct
{
   UINT32 dwCategory;    // Data category - event, network object, etc.
   UINT32 dwCode;        // Data-specific update code
   void *pData;         // Pointer to data block
} UPDATE_INFO;

/**
 * Mobile device session
 */
class NXCORE_EXPORTABLE MobileDeviceSession
{
private:
   SOCKET m_socket;
   int m_id;
   uint32_t m_userId;
   uint32_t m_deviceObjectId;
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
	MUTEX m_mutexSocketWrite;
	InetAddress m_clientAddr;
	TCHAR m_hostName[256]; // IP address of name of conneced host in textual form
   TCHAR m_userName[MAX_SESSION_NAME];   // String in form login_name@host
   TCHAR m_clientInfo[96];  // Client app info string
   uint32_t m_encryptionRqId;
   uint32_t m_encryptionResult;
   CONDITION m_condEncryptionSetup;
   uint32_t m_refCount;
	bool m_authenticated;

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);

   void readThread();

   void processRequest(NXCPMessage *request);
   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(uint32_t requestId);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(uint32_t requestId);
   void login(NXCPMessage *request);
   void updateDeviceInfo(NXCPMessage *request);
   void updateDeviceStatus(NXCPMessage *request);
   void pushData(NXCPMessage *request);

public:
   MobileDeviceSession(SOCKET hSocket, const InetAddress& addr);
   ~MobileDeviceSession();

   void run();

   void sendMessage(const NXCPMessage& msg);

	int getId() const { return m_id; }
   void setId(int id) { if (m_id == -1) m_id = id; }
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
   uint32_t m_uploadCommand;
   uint32_t m_uploadData;
   uuid m_uploadImageGuid;

public:
   ServerDownloadFileInfo(const TCHAR *name, uint32_t uploadCommand, time_t fileModificationTime = 0);
   virtual ~ServerDownloadFileInfo();

   virtual void close(bool success);

   void setUploadData(uint32_t data) { m_uploadData = data; }
   void setImageGuid(const uuid& guid) { m_uploadImageGuid = guid; }
   void updateAgentPkgDBInfo(const TCHAR *description, const TCHAR *pkgName, const TCHAR *pkgVersion, const TCHAR *platform, const TCHAR *cleanFileName);
};

/**
 * TCP proxy information
 */
struct TcpProxy
{
   shared_ptr<AgentConnectionEx> agentConnection;
   UINT32 agentChannelId;
   UINT32 clientChannelId;
   UINT32 nodeId;

   TcpProxy(const shared_ptr<AgentConnectionEx>& c, UINT32 aid, UINT32 cid, UINT32 nid) : agentConnection(c)
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
   UINT16 m_messageCode;

protected:
   virtual void write(const TCHAR *text) override;

public:
   ClientSessionConsole(ClientSession *session, UINT16 msgCode = CMD_ADM_MESSAGE);
   virtual ~ClientSessionConsole();
};

/**
 * Session ID
 */
typedef int session_id_t;

// Explicit instantiation of template classes
#ifdef _WIN32
template class NXCORE_EXPORTABLE SharedPointerIndex<AgentConnection>;
template class NXCORE_EXPORTABLE HashSet<uint32_t>;
template class NXCORE_EXPORTABLE SynchronizedHashSet<uint32_t>;
#endif

/**
 * Forward declaration for syslog message class
 */
class SyslogMessage;

/**
 * Client (user) session
 */
class NXCORE_EXPORTABLE ClientSession
{
private:
   session_id_t m_id;
   SOCKET m_socket;
   BackgroundSocketPollerHandle *m_socketPoller;
   SocketMessageReceiver *m_messageReceiver;
   uint32_t m_dwUserId;
   uint64_t m_systemAccessRights; // User's system access rights
   VolatileCounter m_flags;       // Session flags
	int m_clientType;              // Client system type - desktop, web, mobile, etc.
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
	MUTEX m_mutexSocketWrite;
   MUTEX m_mutexSendAlarms;
   MUTEX m_mutexSendActions;
	MUTEX m_mutexSendAuditLog;
   MUTEX m_mutexPollerInit;
	InetAddress m_clientAddr;
	TCHAR m_workstation[256];      // IP address or name of connected host in textual form
   TCHAR m_webServerAddress[256]; // IP address or name of web server for web sessions
   TCHAR m_loginName[MAX_USER_NAME];
   TCHAR m_sessionName[MAX_SESSION_NAME];   // String in form login_name@host
   TCHAR m_clientInfo[96];  // Client app info string
   TCHAR m_language[8];       // Client's desired language
   time_t m_loginTime;
   SynchronizedHashSet<uint32_t> m_openDataCollectionConfigurations; // List of nodes with DCI lists open
   UINT32 m_dwNumRecordsToUpload; // Number of records to be uploaded
   UINT32 m_dwRecordsUploaded;
   EPRule **m_ppEPPRuleList;   // List of loaded EPP rules
   SynchronizedHashMap<uint32_t, ServerDownloadFileInfo> *m_downloadFileMap;
   VolatileCounter m_refCount;
   uint32_t m_encryptionRqId;
   uint32_t m_encryptionResult;
   CONDITION m_condEncryptionSetup;
	ClientSessionConsole *m_console;			// Server console context
	StringList m_soundFileTypes;
	SharedPointerIndex<AgentConnection> m_agentConnections;
	StringObjectMap<UINT32> *m_subscriptions;
	MUTEX m_subscriptionLock;
	SynchronizedSharedHashMap<pid_t, ProcessExecutor> *m_serverCommands;
	ObjectArray<TcpProxy> *m_tcpProxyConnections;
	MUTEX m_tcpProxyLock;
	VolatileCounter m_tcpProxyChannelId;
	HashSet<uint32_t> *m_pendingObjectNotifications;
   MUTEX m_pendingObjectNotificationsLock;
   bool m_objectNotificationScheduled;
   uint32_t m_objectNotificationDelay;
   size_t m_objectNotificationBatchSize;

   static void socketPollerCallback(BackgroundSocketPollResult pollResult, SOCKET hSocket, ClientSession *session);
   static void terminate(ClientSession *session);
   static EnumerationCallbackResult checkFileTransfer(const uint32_t &key, ServerDownloadFileInfo *fileTransfer,
      std::pair<ClientSession*, IntegerArray<uint32_t>*> *context);

   bool readSocket();
   MessageReceiverResult readMessage(bool allowSocketRead);
   void finalize();
   void pollerThread(shared_ptr<DataCollectionTarget> object, int pollType, uint32_t requestId);

   void processRequest(NXCPMessage *request);
   void processFileTransferMessage(NXCPMessage *msg);

   void postRawMessageAndDelete(NXCP_MESSAGE *msg);
   void sendRawMessageAndDelete(NXCP_MESSAGE *msg);

   void debugPrintf(int level, const TCHAR *format, ...);

   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void onFileUpload(BOOL bSuccess);
   void sendServerInfo(UINT32 dwRqId);
   void login(NXCPMessage *pRequest);
   void getObjects(NXCPMessage *request);
   void getSelectedObjects(NXCPMessage *request);
   void queryObjects(NXCPMessage *request);
   void queryObjectDetails(NXCPMessage *request);
   void getConfigurationVariables(UINT32 dwRqId);
   void getPublicConfigurationVariable(NXCPMessage *request);
   void setDefaultConfigurationVariableValues(NXCPMessage *pRequest);
   void setConfigurationVariable(NXCPMessage *pRequest);
   void deleteConfigurationVariable(NXCPMessage *pRequest);
   void sendUserDB(UINT32 dwRqId);
   void getSelectedUsers(NXCPMessage *request);
   void createUser(NXCPMessage *pRequest);
   void updateUser(NXCPMessage *pRequest);
   void detachLdapUser(NXCPMessage *pRequest);
   void deleteUser(NXCPMessage *pRequest);
   void setPassword(NXCPMessage *request);
   void validatePassword(NXCPMessage *request);
   void lockUserDB(UINT32 dwRqId, BOOL bLock);
   void getAlarmCategories(UINT32 requestId);
   void modifyAlarmCategory(NXCPMessage *pRequest);
   void deleteAlarmCategory(NXCPMessage *pRequest);
   void sendEventDB(NXCPMessage *pRequest);
   void modifyEventTemplate(NXCPMessage *pRequest);
   void deleteEventTemplate(NXCPMessage *pRequest);
   void generateEventCode(UINT32 dwRqId);
   void modifyObject(NXCPMessage *pRequest);
   void changeObjectMgmtStatus(NXCPMessage *pRequest);
   void enterMaintenanceMode(NXCPMessage *request);
   void leaveMaintenanceMode(NXCPMessage *request);
   void openNodeDCIList(NXCPMessage *pRequest);
   void closeNodeDCIList(NXCPMessage *pRequest);
   void modifyNodeDCI(NXCPMessage *pRequest);
   void copyDCI(NXCPMessage *pRequest);
   void bulkDCIUpdate(NXCPMessage *request);
   void applyTemplate(NXCPMessage *pRequest);
   void getCollectedData(NXCPMessage *pRequest);
   void getTableCollectedData(NXCPMessage *pRequest);
	bool getCollectedDataFromDB(NXCPMessage *request, NXCPMessage *response, const DataCollectionTarget& object, int dciType, HistoricalDataType historicalDataType);
	void clearDCIData(NXCPMessage *pRequest);
	void deleteDCIEntry(NXCPMessage *request);
	void forceDCIPoll(NXCPMessage *pRequest);
   void recalculateDCIValues(NXCPMessage *request);
   void changeDCIStatus(NXCPMessage *pRequest);
   void getLastValues(NXCPMessage *pRequest);
   void getLastValuesByDciId(NXCPMessage *pRequest);
   void getTableLastValue(NXCPMessage *request);
   void getLastValue(NXCPMessage *request);
   void getActiveThresholds(NXCPMessage *pRequest);
	void getThresholdSummary(NXCPMessage *request);
   void openEventProcessingPolicy(NXCPMessage *request);
   void closeEventProcessingPolicy(NXCPMessage *request);
   void saveEventProcessingPolicy(NXCPMessage *request);
   void processEventProcessingPolicyRecord(NXCPMessage *request);
   void sendMIBTimestamp(UINT32 dwRqId);
   void sendMib(NXCPMessage *request);
   void createObject(NXCPMessage *request);
   void changeObjectBinding(NXCPMessage *request, BOOL bBind);
   void deleteObject(NXCPMessage *request);
   void getAlarms(NXCPMessage *request);
   void getAlarm(NXCPMessage *request);
   void getAlarmEvents(NXCPMessage *request);
   void acknowledgeAlarm(NXCPMessage *request);
   void resolveAlarm(NXCPMessage *request, bool terminate);
   void bulkResolveAlarms(NXCPMessage *pRequest, bool terminate);
   void deleteAlarm(NXCPMessage *request);
   void openHelpdeskIssue(NXCPMessage *request);
   void getHelpdeskUrl(NXCPMessage *request);
   void unlinkHelpdeskIssue(NXCPMessage *request);
	void getAlarmComments(NXCPMessage *pRequest);
	void updateAlarmComment(NXCPMessage *pRequest);
	void deleteAlarmComment(NXCPMessage *request);
	void updateAlarmStatusFlow(NXCPMessage *request);
   void createAction(NXCPMessage *pRequest);
   void updateAction(NXCPMessage *pRequest);
   void deleteAction(NXCPMessage *pRequest);
   void sendAllActions(UINT32 dwRqId);
   void forcedNodePoll(NXCPMessage *pRequest);
   void onTrap(NXCPMessage *pRequest);
   void onWakeUpNode(NXCPMessage *pRequest);
   void queryParameter(NXCPMessage *pRequest);
   void queryAgentTable(NXCPMessage *pRequest);
   void editTrap(int iOperation, NXCPMessage *pRequest);
   void LockTrapCfg(UINT32 dwRqId, BOOL bLock);
   void sendAllTraps(UINT32 dwRqId);
   void sendAllTraps2(UINT32 dwRqId);
   void getInstalledPackages(UINT32 requestId);
   void installPackage(NXCPMessage *request);
   void removePackage(NXCPMessage *request);
   void deployPackage(NXCPMessage *request);
   void getParametersList(NXCPMessage *pRequest);
   void getUserVariable(NXCPMessage *pRequest);
   void setUserVariable(NXCPMessage *pRequest);
   void copyUserVariable(NXCPMessage *pRequest);
   void enumUserVariables(NXCPMessage *pRequest);
   void deleteUserVariable(NXCPMessage *pRequest);
   void changeObjectZone(NXCPMessage *pRequest);
   void readAgentConfigFile(NXCPMessage *request);
   void writeAgentConfigFile(NXCPMessage *request);
   void executeAction(NXCPMessage *pRequest);
   void getObjectTools(UINT32 requestId);
   void getObjectToolDetails(NXCPMessage *request);
   void updateObjectTool(NXCPMessage *request);
   void deleteObjectTool(NXCPMessage *request);
   void changeObjectToolStatus(NXCPMessage *request);
   void generateObjectToolId(uint32_t requestId);
   void execTableTool(NXCPMessage *pRequest);
   void changeSubscription(NXCPMessage *pRequest);
   void sendServerStats(UINT32 dwRqId);
   void sendScriptList(UINT32 dwRqId);
   void sendScript(NXCPMessage *pRequest);
   void updateScript(NXCPMessage *pRequest);
   void renameScript(NXCPMessage *pRequest);
   void deleteScript(NXCPMessage *pRequest);
   void getSessionList(uint32_t requestId);
   void killSession(NXCPMessage *request);
   void startSnmpWalk(NXCPMessage *request);
   void resolveDCINames(NXCPMessage *request);
   uint32_t resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR *ppszName);
   void sendConfigForAgent(NXCPMessage *pRequest);
   void getAgentConfigurationList(uint32_t requestId);
   void getAgentConfiguration(NXCPMessage *request);
   void updateAgentConfiguration(NXCPMessage *request);
   void deleteAgentConfiguration(NXCPMessage *request);
   void swapAgentConfigurations(NXCPMessage *request);
   void getObjectComments(NXCPMessage *request);
   void updateObjectComments(NXCPMessage *request);
   void pushDCIData(NXCPMessage *pRequest);
   void getAddrList(NXCPMessage *request);
   void setAddrList(NXCPMessage *request);
   void resetComponent(NXCPMessage *pRequest);
   void getRelatedEventList(NXCPMessage *request);
   void getDCIScriptList(NXCPMessage *request);
	void SendDCIInfo(NXCPMessage *pRequest);
   void sendPerfTabDCIList(NXCPMessage *pRequest);
   void exportConfiguration(NXCPMessage *pRequest);
   void importConfiguration(NXCPMessage *pRequest);
   void sendGraph(NXCPMessage *request);
	void sendGraphList(NXCPMessage *request);
	void saveGraph(NXCPMessage *pRequest);
	void deleteGraph(NXCPMessage *pRequest);
	void queryL2Topology(NXCPMessage *pRequest);
   void queryInternalCommunicationTopology(NXCPMessage *pRequest);
   void getDependentNodes(NXCPMessage *request);
	void sendNotification(NXCPMessage *pRequest);
	void sendNetworkCredList(NXCPMessage *request);
	void updateCommunityList(NXCPMessage *pRequest);
	void getPersistantStorage(UINT32 dwRqId);
	void setPstorageValue(NXCPMessage *pRequest);
	void deletePstorageValue(NXCPMessage *pRequest);
	void setConfigCLOB(NXCPMessage *pRequest);
	void getConfigCLOB(NXCPMessage *pRequest);
	void registerAgent(NXCPMessage *pRequest);
	void getServerFile(NXCPMessage *pRequest);
	void cancelFileMonitoring(NXCPMessage *request);
	void getAgentFile(NXCPMessage *pRequest);
	void testDCITransformation(NXCPMessage *pRequest);
	void sendJobList(UINT32 dwRqId);
	void cancelJob(NXCPMessage *pRequest);
	void holdJob(NXCPMessage *pRequest);
	void unholdJob(NXCPMessage *pRequest);
	void getUserCustomAttribute(NXCPMessage *request);
	void setUserCustomAttribute(NXCPMessage *request);
	void openServerLog(NXCPMessage *request);
	void closeServerLog(NXCPMessage *request);
	void queryServerLog(NXCPMessage *request);
	void getServerLogQueryData(NXCPMessage *request);
   void getServerLogRecordDetails(NXCPMessage *request);
	void updateUsmCredentials(NXCPMessage *pRequest);
	void sendDCIThresholds(NXCPMessage *request);
	void addClusterNode(NXCPMessage *request);
	void findNodeConnection(NXCPMessage *request);
	void findMacAddress(NXCPMessage *request);
	void findIpAddress(NXCPMessage *request);
	void findHostname(NXCPMessage *request);
	void sendLibraryImage(NXCPMessage *request);
	void updateLibraryImage(NXCPMessage *request);
	void listLibraryImages(NXCPMessage *request);
	void deleteLibraryImage(NXCPMessage *request);
	void executeServerCommand(NXCPMessage *request);
	void stopServerCommand(NXCPMessage *request);
	void uploadFileToAgent(NXCPMessage *request);
	void listServerFileStore(NXCPMessage *request);
	void processConsoleCommand(NXCPMessage *msg);
	void openConsole(UINT32 rqId);
	void closeConsole(UINT32 rqId);
	void getVlans(NXCPMessage *msg);
	void receiveFile(NXCPMessage *request);
	void deleteFile(NXCPMessage *request);
	void getNetworkPath(NXCPMessage *request);
	void getNodeComponents(NXCPMessage *request);
	void getNodeSoftware(NXCPMessage *request);
   void getNodeHardware(NXCPMessage *request);
	void getWinPerfObjects(NXCPMessage *request);
	void listMappingTables(NXCPMessage *request);
	void getMappingTable(NXCPMessage *request);
	void updateMappingTable(NXCPMessage *request);
	void deleteMappingTable(NXCPMessage *request);
	void getWirelessStations(NXCPMessage *request);
   void getSummaryTables(UINT32 rqId);
   void getSummaryTableDetails(NXCPMessage *request);
   void modifySummaryTable(NXCPMessage *request);
   void deleteSummaryTable(NXCPMessage *request);
   void querySummaryTable(NXCPMessage *request);
   void queryAdHocSummaryTable(NXCPMessage *request);
   void forwardToReportingServer(NXCPMessage *request);
   void getSubnetAddressMap(NXCPMessage *request);
   void getEffectiveRights(NXCPMessage *request);
   void fileManagerControl(NXCPMessage *request);
   void uploadUserFileToAgent(NXCPMessage *request);
   void getSwitchForwardingDatabase(NXCPMessage *request);
   void getRoutingTable(NXCPMessage *request);
   void getLocationHistory(NXCPMessage *request);
   void getScreenshot(NXCPMessage *request);
	void executeScript(NXCPMessage *request);
   void executeLibraryScript(NXCPMessage *request);
   void compileScript(NXCPMessage *request);
	void resyncAgentDciConfiguration(NXCPMessage *request);
   void cleanAgentDciConfiguration(NXCPMessage *request);
   void getSchedulerTaskHandlers(NXCPMessage *request);
   void getScheduledTasks(NXCPMessage *request);
   void getScheduledReportingTasks(NXCPMessage *request);
   void addScheduledTask(NXCPMessage *request);
   void updateScheduledTask(NXCPMessage *request);
   void removeScheduledTask(NXCPMessage *request);
   void getRepositories(NXCPMessage *request);
   void addRepository(NXCPMessage *request);
   void modifyRepository(NXCPMessage *request);
   void deleteRepository(NXCPMessage *request);
   void getAgentTunnels(NXCPMessage *request);
   void bindAgentTunnel(NXCPMessage *request);
   void unbindAgentTunnel(NXCPMessage *request);
   void setupTcpProxy(NXCPMessage *request);
   void closeTcpProxy(NXCPMessage *request);
   void getPredictionEngines(NXCPMessage *request);
   void getPredictedData(NXCPMessage *request);
   void expandMacros(NXCPMessage *request);
   void updatePolicy(NXCPMessage *request);
   void deletePolicy(NXCPMessage *request);
   void getPolicyList(NXCPMessage *request);
   void getPolicy(NXCPMessage *request);
   void onPolicyEditorClose(NXCPMessage *request);
   void forceApplyPolicy(NXCPMessage *pRequest);
   void getMatchingDCI(NXCPMessage *request);
   void getUserAgentNotification(NXCPMessage *request);
   void addUserAgentNotification(NXCPMessage *request);
   void recallUserAgentNotification(NXCPMessage *request);
   SharedObjectArray<DCObject> *resolveDCOsByRegex(UINT32 objectId, const TCHAR *objectNameRegex, const TCHAR *dciRegex, bool searchByName);
   void getNotificationChannels(UINT32 requestId);
   void addNotificationChannel(NXCPMessage *request);
   void updateNotificationChannel(NXCPMessage *request);
   void removeNotificationChannel(NXCPMessage *request);
   void renameNotificationChannel(NXCPMessage *request);
   void getNotificationDriverNames(UINT32 requestId);
   void startActiveDiscovery(NXCPMessage *request);
   void getPhysicalLinks(NXCPMessage *request);
   void updatePhysicalLink(NXCPMessage *request);
   void deletePhysicalLink(NXCPMessage *request);
   void getWebServices(NXCPMessage *request);
   void modifyWebService(NXCPMessage *request);
   void deleteWebService(NXCPMessage *request);
   void getObjectCategories(NXCPMessage *request);
   void modifyObjectCategory(NXCPMessage *request);
   void deleteObjectCategory(NXCPMessage *request);
   void getGeoAreas(NXCPMessage *request);
   void modifyGeoArea(NXCPMessage *request);
   void deleteGeoArea(NXCPMessage *request);
   void updateSharedSecretList(NXCPMessage *request);
   void updateSNMPPortList(NXCPMessage *request);
   void findProxyForNode(NXCPMessage *request);
   void getSshKeys(NXCPMessage *request);
   void deleteSshKey(NXCPMessage *request);
   void updateSshKey(NXCPMessage *request);
   void generateSshKey(NXCPMessage *request);
#ifdef WITH_ZMQ
   void zmqManageSubscription(NXCPMessage *request, zmq::SubscriptionType type, bool subscribe);
   void zmqListSubscriptions(NXCPMessage *request, zmq::SubscriptionType type);
#endif

   void alarmUpdateWorker(Alarm *alarm);
   void sendActionDBUpdateMessage(NXCP_MESSAGE *msg);
   void sendObjectUpdates();

   void finalizeFileTransferToAgent(shared_ptr<AgentConnection> conn, uint32_t requestId);

public:
   ClientSession(SOCKET hSocket, const InetAddress& addr);
   ~ClientSession();

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
   void sendPollerMsg(uint32_t requestIf, const TCHAR *text);
	bool sendFile(const TCHAR *file, uint32_t requestId, long offset, bool allowCompression = true);

   void writeAuditLog(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *format, ...);
   void writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...);
   void writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...);

   session_id_t getId() const { return m_id; }
   void setId(session_id_t id) { if (m_id == -1) m_id = id; }

   void setSocketPoller(BackgroundSocketPollerHandle *p) { m_socketPoller = p; }

   const TCHAR *getLoginName() const { return m_loginName; }
   const TCHAR *getSessionName() const { return m_sessionName; }
   const TCHAR *getClientInfo() const { return m_clientInfo; }
	const TCHAR *getWorkstation() const { return m_workstation; }
   const TCHAR *getWebServerAddress() const { return m_webServerAddress; }
   uint32_t getUserId() const { return m_dwUserId; }
   uint64_t getSystemRights() const { return m_systemAccessRights; }
   uint32_t getFlags() const { return static_cast<uint32_t>(m_flags); }
   bool isAuthenticated() const { return (m_flags & CSF_AUTHENTICATED) ? true : false; }
   bool isTerminated() const { return (m_flags & CSF_TERMINATED) ? true : false; }
   bool isConsoleOpen() const { return (m_flags & CSF_CONSOLE_OPEN) ? true : false; }
   bool isCompressionEnabled() const { return (m_flags & CSF_COMPRESSION_ENABLED) ? true : false; }
   int getCipher() const { return (m_encryptionContext == nullptr) ? -1 : m_encryptionContext->getCipher(); }
	int getClientType() const { return m_clientType; }
   time_t getLoginTime() const { return m_loginTime; }
   bool isSubscribedTo(const TCHAR *channel) const;
   bool isDataCollectionConfigurationOpen(uint32_t objectId) const { return m_openDataCollectionConfigurations.contains(objectId); }

	bool checkSysAccessRights(uint64_t requiredAccess) const
   {
      return (m_dwUserId == 0) ? true :
         ((requiredAccess & m_systemAccessRights) == requiredAccess);
   }

   void runHousekeeper();

   void terminate();
   void notify(uint32_t code, uint32_t data = 0);

   void updateSystemAccessRights();

   void onNewEvent(Event *pEvent);
   void onSyslogMessage(const SyslogMessage *sm);
   void onNewSNMPTrap(NXCPMessage *pMsg);
   void onObjectChange(const shared_ptr<NetObj>& object);
   void onAlarmUpdate(UINT32 dwCode, const Alarm *alarm);
   void onActionDBUpdate(UINT32 dwCode, const Action *action);
   void onLibraryImageChange(const uuid& guid, bool removed = false);
   void processTcpProxyData(AgentConnectionEx *conn, UINT32 agentChannelId, const void *data, size_t size);

   void unregisterServerCommand(pid_t taskId);
};

/**
 * Graph ACL entry
 */
struct GRAPH_ACL_ENTRY
{
   UINT32 dwGraphId;
   UINT32 dwUserId;
   UINT32 dwAccess;
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

   virtual void onOutput(const char *text) override;
   virtual void endOfOutput() override;

public:
   ServerCommandExecutor(NXCPMessage *msg, ClientSession *session);
   ~ServerCommandExecutor();

   const TCHAR *getMaskedCommand() const { return m_maskedCommand.cstr(); }
};

/**
 * SNMP Trap parameter map object
 */
class SNMPTrapParameterMapping
{
private:
   SNMP_ObjectId *m_objectId;           // Trap OID
   UINT32 m_position;                   // Trap position
   UINT32 m_flags;
   TCHAR *m_description;

public:
   SNMPTrapParameterMapping();
   SNMPTrapParameterMapping(DB_RESULT mapResult, int row);
   SNMPTrapParameterMapping(ConfigEntry *entry);
   SNMPTrapParameterMapping(SNMP_ObjectId *oid, UINT32 flags, TCHAR *description);
   SNMPTrapParameterMapping(NXCPMessage *msg, UINT32 base);
   ~SNMPTrapParameterMapping();

   void fillMessage(NXCPMessage *msg, UINT32 base) const;

   SNMP_ObjectId *getOid() const { return m_objectId; }
   int getPosition() const { return m_position; }
   bool isPositional() const { return m_objectId == NULL; }

   UINT32 getFlags() const { return m_flags; }

   const TCHAR *getDescription() const { return m_description; }
};

/**
 * SNMP Trap configuration object
 */
class SNMPTrapConfiguration
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
   SNMPTrapConfiguration();
   SNMPTrapConfiguration(DB_RESULT trapResult, DB_HANDLE hdb, DB_STATEMENT stmt, int row);
   SNMPTrapConfiguration(const ConfigEntry& entry, const uuid& guid, uint32_t id, uint32_t eventCode);
   SNMPTrapConfiguration(NXCPMessage *msg);
   ~SNMPTrapConfiguration();

   void fillMessage(NXCPMessage *msg) const;
   void fillMessage(NXCPMessage *msg, UINT32 base) const;
   bool saveParameterMapping(DB_HANDLE hdb);
   void notifyOnTrapCfgChange(UINT32 code);

   UINT32 getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   const SNMP_ObjectId& getOid() const { return m_objectId; }
   const SNMPTrapParameterMapping *getParameterMapping(int index) const { return m_mappings.get(index); }
   int getParameterMappingCount() const { return m_mappings.size(); }
   UINT32 getEventCode() const { return m_eventCode; }
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
   shared_ptr<AgentConnection> m_agentConnection;
   uint32_t m_requestId;
   TCHAR *m_localFile;
   TCHAR *m_remoteFile;
   uint64_t m_fileSize;
   uint64_t m_currentSize;
   uint64_t m_maxFileSize;
   bool m_monitor;
   bool m_allowExpansion;

   static void fileResendCallback(NXCPMessage *msg, void *arg);
   static TCHAR *buildServerFileName(uint32_t nodeId, const TCHAR *remoteFile, TCHAR *buffer, size_t bufferSize);

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
 * Functions
 */
void ConfigPreLoad();
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue);
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE hdb, const TCHAR *variable, TCHAR *buffer, size_t size, const TCHAR *defaultValue);
#ifdef UNICODE
bool NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *variable, char *buffer, size_t size, const char *defaultValue);
#else
#define ConfigReadStrA ConfigReadStr
#endif
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const TCHAR *variable, char *buffer, size_t size, const char *defaultValue);
int32_t NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *variable, int32_t defaultValue);
int32_t NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const TCHAR *variable, int32_t defaultValue);
uint32_t NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *variable, uint32_t defaultValue);
int64_t NXCORE_EXPORTABLE ConfigReadInt64(const TCHAR *variable, int64_t defaultValue);
uint64_t NXCORE_EXPORTABLE ConfigReadUInt64(const TCHAR *variable, uint64_t defaultValue);
bool NXCORE_EXPORTABLE ConfigReadBoolean(const TCHAR *variable, bool defaultValue);
bool NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *variable, int *buffer, size_t size, int defaultElementValue);
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *variable, const TCHAR *value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *variable, int32_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *variable, uint32_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt64(const TCHAR *variable, int64_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteUInt64(const TCHAR *variable, uint64_t value, bool create, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *variable, int *value, size_t size, bool create, bool isVisible = true, bool needRestart = false);
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *varariable, const TCHAR *defaultValue);
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *variable, const TCHAR *value, bool create);
bool NXCORE_EXPORTABLE ConfigDelete(const TCHAR *variable);

void MetaDataPreLoad();
bool NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *variable, TCHAR *buffer, int size, const TCHAR *defaultValue);
INT32 NXCORE_EXPORTABLE MetaDataReadInt32(const TCHAR *variable, INT32 defaultValue);
bool NXCORE_EXPORTABLE MetaDataWriteStr(const TCHAR *variable, const TCHAR *value);
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const TCHAR *variable, INT32 value);

bool NXCORE_EXPORTABLE LoadConfig(int *debugLevel);

bool LockDatabase(InetAddress *lockAddr, TCHAR *lockInfo);
void NXCORE_EXPORTABLE UnlockDatabase();

void NXCORE_EXPORTABLE Shutdown();
void NXCORE_EXPORTABLE FastShutdown(ShutdownReason reason);
BOOL NXCORE_EXPORTABLE Initialize();
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *);
void NXCORE_EXPORTABLE ShutdownDatabase();
void NXCORE_EXPORTABLE InitiateShutdown(ShutdownReason reason);

int ProcessConsoleCommand(const TCHAR *pszCmdLine, CONSOLE_CTX pCtx);

void SaveObjects(DB_HANDLE hdb, UINT32 watchdogId, bool saveRuntimeData);

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query);
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values);
void QueueIDataInsert(time_t timestamp, uint32_t nodeId, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue, DCObjectStorageClass storageClass);
void QueueRawDciDataUpdate(time_t timestamp, uint32_t dciId, const TCHAR *rawValue, const TCHAR *transformedValue);
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
         int32_t zoneUIN);

void InitLocalNetInfo();

ArpCache *GetLocalArpCache();
InterfaceList *GetLocalInterfaceList();

RoutingTable *SnmpGetRoutingTable(SNMP_Transport *pTransport);

void LoadNetworkDeviceDrivers();
NetworkDeviceDriver *FindDriverForNode(Node *node, SNMP_Transport *snmpTransport);
NetworkDeviceDriver *FindDriverForNode(const TCHAR *name, const TCHAR *snmpObjectId, const TCHAR *defaultDriver, SNMP_Transport *snmpTransport);
NetworkDeviceDriver *FindDriverByName(const TCHAR *name);
void AddDriverSpecificOids(StringList *list);
void PrintNetworkDeviceDriverList(ServerConsole *console);

void LoadNotificationChannelDrivers();
void LoadNotificationChannels();
void ShutdownNotificationChannels();
void SendNotification(const TCHAR *name, TCHAR *recipient, const TCHAR *subject, const TCHAR *message);
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
void CheckPotentialNode(const InetAddress& ipAddr, int32_t zoneUIN, DiscoveredAddressSourceType sourceType, UINT32 sourceNodeId);
shared_ptr<Node> NXCORE_EXPORTABLE PollNewNode(NewNodeData *newNodeData);
int64_t GetDiscoveryPollerQueueSize();

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*handler)(ClientSession*, void*), void *context);
template <typename C> void EnumerateClientSessions(void (*handler)(ClientSession *, C *), C *context)
{
   EnumerateClientSessions(reinterpret_cast<void (*)(ClientSession*, void*)>(handler), context);
}
template <typename C> void EnumerateClientSessions(void (*handler)(ClientSession *, const C *), const C *context)
{
   EnumerateClientSessions(reinterpret_cast<void (*)(ClientSession*, void*)>(handler), const_cast<C*>(context));
}
static inline void EnumerateClientSessions_WrapperHandler(ClientSession *session, void *context)
{
   reinterpret_cast<void (*)(ClientSession*)>(context)(session);
}
static inline void EnumerateClientSessions(void (*handler)(ClientSession*))
{
   EnumerateClientSessions(EnumerateClientSessions_WrapperHandler, reinterpret_cast<void*>(handler));
}

void NXCORE_EXPORTABLE NotifyClientSessions(uint32_t code, uint32_t data);
void NXCORE_EXPORTABLE NotifyClientSessions(const NXCPMessage& msg, const TCHAR *channel);
void NXCORE_EXPORTABLE NotifyClientSession(session_id_t sessionId, uint32_t code, uint32_t data);
void NXCORE_EXPORTABLE NotifyClientsOnGraphUpdate(const NXCPMessage& msg, uint32_t graphId);
void NotifyClientsOnPolicyUpdate(const NXCPMessage& msg, const Template& object);
void NotifyClientsOnPolicyDelete(uuid guid, const Template& object);
void NotifyClientsOnDCIUpdate(const DataCollectionOwner& object, DCObject *dco);
void NotifyClientsOnDCIDelete(const DataCollectionOwner& object, uint32_t dcoId);
void NotifyClientsOnDCIStatusChange(const DataCollectionOwner& object, uint32_t dcoId, int status);
void NotifyClientsOnDCIUpdate(const NXCPMessage& msg, const NetObj& object);
void NotifyClientsOnThresholdChange(UINT32 objectId, UINT32 dciId, UINT32 thresholdId, const TCHAR *instance, ThresholdCheckResult change);
int GetSessionCount(bool includeSystemAccount, bool includeNonAuthenticated, int typeFilter, const TCHAR *loginFilter);
bool IsLoggedIn(uint32_t userId);
bool NXCORE_EXPORTABLE KillClientSession(session_id_t id);
void CloseOtherSessions(uint32_t userId, session_id_t thisSession);

void GetSysInfoStr(TCHAR *buffer, int nMaxSize);
InetAddress GetLocalIpAddr();

InetAddress NXCORE_EXPORTABLE ResolveHostName(int32_t zoneUIN, const TCHAR *hostname);
bool EventNameResolver(const TCHAR *name, UINT32 *code);

BOOL ExecCommand(TCHAR *pszCommand);
bool SendMagicPacket(const InetAddress& ipAddr, const MacAddress& macAddr, int count);
StringList *SplitCommandLine(const TCHAR *command);

bool InitIdTable();
uint32_t CreateUniqueId(int group);
void SaveCurrentFreeId();

void InitTraps();
void SendTrapsToClient(ClientSession *pSession, UINT32 dwRqId);
void CreateTrapCfgMessage(NXCPMessage *msg);
UINT32 CreateNewTrap(UINT32 *pdwTrapId);
UINT32 UpdateTrapFromMsg(NXCPMessage *pMsg);
UINT32 DeleteTrap(UINT32 dwId);
void CreateTrapExportRecord(StringBuffer &xml, UINT32 id);
UINT32 ResolveTrapGuid(const uuid& guid);
void AddTrapCfgToList(SNMPTrapConfiguration *trapCfg);

BOOL IsTableTool(UINT32 dwToolId);
BOOL CheckObjectToolAccess(UINT32 dwToolId, UINT32 dwUserId);
uint32_t ExecuteTableTool(uint32_t toolId, const shared_ptr<Node>& node, uint32_t requestId, ClientSession *session);
UINT32 DeleteObjectToolFromDB(UINT32 dwToolId);
UINT32 ChangeObjectToolStatus(UINT32 toolId, bool enabled);
UINT32 UpdateObjectToolFromMessage(NXCPMessage *pMsg);
void CreateObjectToolExportRecord(StringBuffer &xml, UINT32 id);
bool ImportObjectTool(ConfigEntry *config, bool overwrite);
UINT32 GetObjectToolsIntoMessage(NXCPMessage *msg, UINT32 userId, bool fullAccess);
UINT32 GetObjectToolDetailsIntoMessage(UINT32 toolId, NXCPMessage *msg);

UINT32 ModifySummaryTable(NXCPMessage *msg, LONG *newId);
UINT32 DeleteSummaryTable(LONG tableId);
Table *QuerySummaryTable(LONG tableId, SummaryTable *adHocDefinition, UINT32 baseObjectId, UINT32 userId, UINT32 *rcc);
bool CreateSummaryTableExportRecord(INT32 id, StringBuffer &xml);
bool ImportSummaryTable(ConfigEntry *config, bool overwrite);

void GetFullCommunityList(NXCPMessage *msg);
void GetZoneCommunityList(NXCPMessage *msg, int32_t zoneUIN);
void GetFullUsmCredentialList(NXCPMessage *msg);
void GetZoneUsmCredentialList(NXCPMessage *msg, int32_t zoneUIN);
void GetFullSnmpPortList(NXCPMessage *msg);
void GetZoneSnmpPortList(NXCPMessage *msg, int32_t zoneUIN);
void GetFullAgentSecretList(NXCPMessage *msg);
void GetZoneAgentSecretList(NXCPMessage *msg, int32_t zoneUIN);

void ReinitializeSyslogParser();
void OnSyslogConfigurationChange(const TCHAR *name, const TCHAR *value);

void InitializeWindowsEventParser();
void OnWindowsEventsConfigurationChange(const TCHAR *name, const TCHAR *value);

void EscapeString(StringBuffer &str);

void InitAuditLog();
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                     const TCHAR *workstation, int sessionId, uint32_t objectId,
                                     const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                      const TCHAR *workstation, int sessionId, uint32_t objectId,
                                      const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithValues(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                               const TCHAR *workstation, int sessionId, uint32_t objectId,
                                               const TCHAR *oldValue, const TCHAR *newValue, char valueType,
                                               const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                                   const TCHAR *workstation, int sessionId, uint32_t objectId,
                                                   json_t *oldValue, json_t *newValue,
                                                   const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                                const TCHAR *workstation, int sessionId, uint32_t objectId,
                                                const TCHAR *oldValue, const TCHAR *newValue, char valueType,
                                                const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues2(const TCHAR *subsys, bool isSuccess, uint32_t userId,
                                                    const TCHAR *workstation, int sessionId, uint32_t objectId,
                                                    json_t *oldValue, json_t *newValue,
                                                    const TCHAR *format, va_list args);

bool ValidateConfig(const Config& config, uint32_t flags, TCHAR *errorText, int errorTextLen);
uint32_t ImportConfig(const Config& config, uint32_t flags);

#ifdef _WITH_ENCRYPTION
X509 *CertificateFromLoginMessage(const NXCPMessage *pMsg);
bool ValidateUserCertificate(X509 *cert, const TCHAR *login, const BYTE *challenge, const BYTE *signature,
      size_t sigLen, CertificateMappingMethod mappingMethod, const TCHAR *mappingData);
void ReloadCertificates();
bool GetServerCertificateCountry(TCHAR *buffer, size_t size);
bool GetServerCertificateOrganization(TCHAR *buffer, size_t size);
X509 *IssueCertificate(X509_REQ *request, const char *ou, const char *cn, int days);
void LogCertificateAction(CertificateOperation operation, UINT32 userId, UINT32 nodeId, const uuid& nodeGuid, CertificateType type, X509 *cert);
#endif
void LogCertificateAction(CertificateOperation operation, UINT32 userId, UINT32 nodeId, const uuid& nodeGuid, CertificateType type, const TCHAR *subject, INT32 serial);

#ifndef _WIN32
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);
#endif   /* not _WIN32 */

void DumpClientSessions(ServerConsole *console);
void DumpMobileDeviceSessions(CONSOLE_CTX console);
void ShowServerStats(CONSOLE_CTX console);
void ShowQueueStats(CONSOLE_CTX console, const Queue *queue, const TCHAR *name);
void ShowQueueStats(CONSOLE_CTX console, int64_t size, const TCHAR *name);
void ShowThreadPoolPendingQueue(CONSOLE_CTX console, ThreadPool *p, const TCHAR *name);
void ShowThreadPool(CONSOLE_CTX console, const TCHAR *p);
DataCollectionError GetThreadPoolStat(ThreadPoolStat stat, const TCHAR *param, TCHAR *value);
void DumpProcess(CONSOLE_CTX console);

#define GRAPH_FLAG_TEMPLATE 1

GRAPH_ACL_ENTRY *LoadGraphACL(DB_HANDLE hdb, UINT32 graphId, int *pnACLSize);
GRAPH_ACL_ENTRY *LoadAllGraphACL(DB_HANDLE hdb, int *pnACLSize);
BOOL CheckGraphAccess(GRAPH_ACL_ENTRY *pACL, int nACLSize, UINT32 graphId, UINT32 graphUserId, UINT32 graphDesiredAccess);
UINT32 GetGraphAccessCheckResult(UINT32 graphId, UINT32 graphUserId);
void FillGraphListMsg(NXCPMessage *msg, UINT32 userId, bool templageGraphs, UINT32 graphId = 0);
void SaveGraph(NXCPMessage *pRequest, UINT32 userId, NXCPMessage *msg);
UINT32 DeleteGraph(UINT32 graphId, UINT32 userId);

#if XMPP_SUPPORTED
void SendXMPPMessage(const TCHAR *rcpt, const TCHAR *text);
#endif

const TCHAR NXCORE_EXPORTABLE *CountryAlphaCode(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CountryName(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CurrencyAlphaCode(const TCHAR *numericCode);
int NXCORE_EXPORTABLE CurrencyExponent(const TCHAR *code);
const TCHAR NXCORE_EXPORTABLE *CurrencyName(const TCHAR *code);

void NXCORE_EXPORTABLE RegisterComponent(const TCHAR *id);
bool NXCORE_EXPORTABLE IsComponentRegistered(const TCHAR *id);

bool NXCORE_EXPORTABLE IsCommand(const TCHAR *cmdTemplate, const TCHAR *str, int minChars);

ObjectArray<InetAddressListElement> *LoadServerAddressList(int listType);

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
 * Housekeeper control
 */
void StartHouseKeeper();
void StopHouseKeeper();
void RunHouseKeeper();

/**
 * NXSL script functions
 */
uint32_t UpdateScript(const NXCPMessage *request, uint32_t *scriptId, ClientSession *session);
uint32_t RenameScript(uint32_t scriptId, const TCHAR *newName);
uint32_t DeleteScript(uint32_t scriptId);

/**
 * ICMP scan
 */
void ScanAddressRange(const InetAddress& from, const InetAddress& to, void(*callback)(const InetAddress&, int32_t, const Node *, uint32_t, ServerConsole *, void *), ServerConsole *console, void *context);

/**
 * Prepare MERGE statement if possible, otherwise INSERT or UPDATE depending on record existence
 * Identification column appended to provided column list
 */
DB_STATEMENT NXCORE_EXPORTABLE DBPrepareMerge(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, UINT32 id, const TCHAR * const *columns);

/**
 * Monitored file
 */
struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   ClientSession *session;
   uint32_t nodeID;
};

/**
 * List of monitored files
 */
class FileMonitoringList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE> m_monitoredFiles;

   void lock()
   {
      MutexLock(m_mutex);
   }

   void unlock()
   {
      MutexUnlock(m_mutex);
   }

public:
   FileMonitoringList();
   ~FileMonitoringList();

   void addFile(MONITORED_FILE *file, Node *node, const shared_ptr<AgentConnection>& conn);
   bool isDuplicate(MONITORED_FILE *file);
   bool removeFile(MONITORED_FILE *file);
   void removeDisconnectedNode(uint32_t nodeId);
   ObjectArray<ClientSession> *findClientByFNameAndNodeID(const TCHAR *fileName, uint32_t nodeID);
};

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
extern NXCORE_EXPORTABLE_VAR(UINT32 g_processAffinityMask);
extern uint64_t g_serverId;
extern RSA *g_pServerKey;
extern uint32_t g_icmpPingSize;
extern uint32_t g_icmpPingTimeout;
extern uint32_t g_auditFlags;
extern time_t g_serverStartTime;
extern uint32_t g_agentCommandTimeout;
extern uint32_t g_thresholdRepeatInterval;
extern uint32_t g_requiredPolls;
extern uint32_t g_slmPollingInterval;
extern uint32_t g_offlineDataRelevanceTime;
extern int32_t g_instanceRetentionTime;
extern uint32_t g_snmpTrapStormCountThreshold;
extern uint32_t g_snmpTrapStormDurationThreshold;

extern TCHAR g_szDbDriver[];
extern TCHAR g_szDbDrvParams[];
extern TCHAR g_szDbServer[];
extern TCHAR g_szDbLogin[];
extern TCHAR g_szDbPassword[];
extern TCHAR g_szDbName[];
extern TCHAR g_szDbSchema[];
extern DB_DRIVER g_dbDriver;
extern VolatileCounter64 g_idataWriteRequests;
extern uint64_t g_rawDataWriteRequests;
extern VolatileCounter64 g_otherWriteRequests;

struct DELAYED_SQL_REQUEST;
extern ObjectQueue<DELAYED_SQL_REQUEST> g_dbWriterQueue;

extern NXCORE_EXPORTABLE_VAR(int g_dbSyntax);
extern FileMonitoringList g_monitoringList;

extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_mainThreadPool);
extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_clientThreadPool);
extern NXCORE_EXPORTABLE_VAR(ThreadPool *g_mobileThreadPool);

#endif   /* MODULE_NXDBMGR_EXTENSION */

#endif   /* _nms_core_h_ */
