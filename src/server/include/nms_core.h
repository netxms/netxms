/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

#ifdef _WIN32
#ifdef NXCORE_EXPORTS
#define NXCORE_EXPORTABLE __declspec(dllexport)
#else
#define NXCORE_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define NXCORE_EXPORTABLE
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
#include <npe.h>
#include <nxcore_smclp.h>

/**
 * Console context
 */
struct __console_ctx
{
   SOCKET hSocket;
	MUTEX socketMutex;
   NXCPMessage *pMsg;
	ClientSession *session;
   String *output;
};

typedef __console_ctx * CONSOLE_CTX;

/**
 * Server includes
 */
#include "nms_dcoll.h"
#include "nms_users.h"
#include "nxcore_winperf.h"
#include "nms_objects.h"
#include "nms_locks.h"
#include "nms_pkg.h"
#include "nms_topo.h"
#include "nms_script.h"
#include "nxcore_ps.h"
#include "nxcore_jobs.h"
#include "nxcore_logs.h"
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

#define MAX_CLIENT_SESSIONS   128
#define MAX_DEVICE_SESSIONS   256

#define PING_TIME_TIMEOUT     10000

typedef void * HSNMPSESSION;

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
#define IDG_EVENT_GROUP       6
#define IDG_THRESHOLD         7
#define IDG_USER              8
#define IDG_USER_GROUP        9
#define IDG_ALARM             10
#define IDG_ALARM_NOTE        11
#define IDG_PACKAGE           12
#define IDG_SLM_TICKET        13
#define IDG_OBJECT_TOOL       14
#define IDG_SCRIPT            15
#define IDG_AGENT_CONFIG      16
#define IDG_GRAPH					17
#define IDG_CERTIFICATE			18
#define IDG_DCT_COLUMN        19
#define IDG_MAPPING_TABLE     20
#define IDG_DCI_SUMMARY_TABLE 21
#define IDG_SCHEDULED_TASK    22
#define IDG_ALARM_CATEGORY    23

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
#define CSF_TERMINATED           ((UINT32)0x00000001)
#define CSF_EPP_LOCKED           ((UINT32)0x00000002)
#define CSF_PACKAGE_DB_LOCKED    ((UINT32)0x00000004)
#define CSF_USER_DB_LOCKED       ((UINT32)0x00000008)
#define CSF_EPP_UPLOAD           ((UINT32)0x00000010)
#define CSF_CONSOLE_OPEN         ((UINT32)0x00000020)
#define CSF_AUTHENTICATED        ((UINT32)0x00000080)
#define CSF_COMPRESSION_ENABLED  ((UINT32)0x00000100)
#define CSF_RECEIVING_MAP_DATA   ((UINT32)0x00000200)
#define CSF_SYNC_OBJECT_COMMENTS ((UINT32)0x00000400)
#define CSF_CUSTOM_LOCK_1        ((UINT32)0x01000000)
#define CSF_CUSTOM_LOCK_2        ((UINT32)0x02000000)
#define CSF_CUSTOM_LOCK_3        ((UINT32)0x04000000)
#define CSF_CUSTOM_LOCK_4        ((UINT32)0x08000000)
#define CSF_CUSTOM_LOCK_5        ((UINT32)0x10000000)
#define CSF_CUSTOM_LOCK_6        ((UINT32)0x20000000)
#define CSF_CUSTOM_LOCK_7        ((UINT32)0x40000000)
#define CSF_CUSTOM_LOCK_8        ((UINT32)0x80000000)

/**
 * Client session states
 */
#define SESSION_STATE_INIT       0
#define SESSION_STATE_IDLE       1
#define SESSION_STATE_PROCESSING 2

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
 * New node information
 */
typedef struct
{
   InetAddress ipAddr;
	UINT32 zoneUIN;
	BOOL ignoreFilter;
	BYTE bMacAddr[MAC_ADDR_LENGTH];
} NEW_NODE;

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
#define InetAddressListElement_SUBNET     0
#define InetAddressListElement_RANGE      1

/**
 * IP address list element
 */
class NXCORE_EXPORTABLE InetAddressListElement
{
private:
   int m_type;
   InetAddress m_baseAddress;
   InetAddress m_endAddress;

public:
   InetAddressListElement(NXCPMessage *msg, UINT32 baseId);
   InetAddressListElement(const InetAddress& baseAddr, const InetAddress& endAddr);
   InetAddressListElement(const InetAddress& baseAddr, int maskBits);
   InetAddressListElement(DB_RESULT hResult, int row);

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;

   bool contains(const InetAddress& addr) const;

   int getType() const { return m_type; }
   const InetAddress& getBaseAddress() const { return m_baseAddress; }
   const InetAddress& getEndAddress() const { return m_endAddress; }

   String toString() const;
};

/**
 * Node information for autodiscovery filter
 */
typedef struct
{
   InetAddress ipAddr;
   UINT32 zoneUIN;
   UINT32 dwFlags;
   int nSNMPVersion;
   TCHAR szObjectId[MAX_OID_LEN * 4];    // SNMP OID
   TCHAR szAgentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN];
} DISCOVERY_FILTER_DATA;

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
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   int m_id;
   int m_state;
   WORD m_wCurrentCmd;
   UINT32 m_dwUserId;
	UINT32 m_deviceObjectId;
   NXCP_BUFFER *m_pMsgBuffer;
   NXCPEncryptionContext *m_pCtx;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
   THREAD m_hWriteThread;
   THREAD m_hProcessingThread;
	MUTEX m_mutexSocketWrite;
	InetAddress m_clientAddr;
	TCHAR m_szHostName[256]; // IP address of name of conneced host in textual form
   TCHAR m_szUserName[MAX_SESSION_NAME];   // String in form login_name@host
   TCHAR m_szClientInfo[96];  // Client app info string
   UINT32 m_dwEncryptionRqId;
   UINT32 m_dwEncryptionResult;
   CONDITION m_condEncryptionSetup;
   UINT32 m_dwRefCount;
	bool m_isAuthenticated;

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);

   void readThread();
   void writeThread();
   void processingThread();

   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(UINT32 dwRqId);
   void login(NXCPMessage *pRequest);
   void updateDeviceInfo(NXCPMessage *pRequest);
   void updateDeviceStatus(NXCPMessage *pRequest);
   void pushData(NXCPMessage *request);

public:
   MobileDeviceSession(SOCKET hSocket, const InetAddress& addr);
   ~MobileDeviceSession();

   void run();

   void postMessage(NXCPMessage *pMsg) { m_pSendQueue->put(pMsg->serialize()); }
   void sendMessage(NXCPMessage *pMsg);

	int getId() { return m_id; }
   void setId(int id) { if (m_id == -1) m_id = id; }
   int getState() { return m_state; }
   const TCHAR *getUserName() { return m_szUserName; }
   const TCHAR *getClientInfo() { return m_szClientInfo; }
	const TCHAR *getHostName() { return m_szHostName; }
   UINT32 getUserId() { return m_dwUserId; }
   bool isAuthenticated() { return m_isAuthenticated; }
   WORD getCurrentCmd() { return m_wCurrentCmd; }
   int getCipher() { return (m_pCtx == NULL) ? -1 : m_pCtx->getCipher(); }
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
   UINT32 m_uploadCommand;
   UINT32 m_uploadData;
   uuid m_uploadImageGuid;

public:
   ServerDownloadFileInfo(const TCHAR *name, UINT32 uploadCommand, time_t lastModTime = 0);
   virtual ~ServerDownloadFileInfo();

   virtual void close(bool success);

   void setUploadData(UINT32 data) { m_uploadData = data; }
   void setImageGuid(const uuid& guid) { m_uploadImageGuid = guid; }
   void updateAgentPkgDBInfo(const TCHAR *description, const TCHAR *pkgName, const TCHAR *pkgVersion, const TCHAR *platform, const TCHAR *cleanFileName);
};

/**
 * Client (user) session
 */
class NXCORE_EXPORTABLE ClientSession
{
private:
   SOCKET m_hSocket;
   Queue *m_sendQueue;
   Queue *m_requestQueue;
   int m_id;
   int m_state;
   WORD m_wCurrentCmd;
   UINT32 m_dwUserId;
   UINT64 m_dwSystemAccess;    // User's system access rights
   UINT32 m_dwFlags;           // Session flags
	int m_clientType;				// Client system type - desktop, web, mobile, etc.
   NXCPEncryptionContext *m_pCtx;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
   THREAD m_hWriteThread;
   THREAD m_hProcessingThread;
	MUTEX m_mutexSocketWrite;
   MUTEX m_mutexSendObjects;
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
   UINT32 m_dwOpenDCIListSize; // Number of open DCI lists
   UINT32 *m_pOpenDCIList;     // List of nodes with DCI lists open
   UINT32 m_dwNumRecordsToUpload; // Number of records to be uploaded
   UINT32 m_dwRecordsUploaded;
   EPRule **m_ppEPPRuleList;   // List of loaded EPP rules
   HashMap<UINT32, ServerDownloadFileInfo> *m_downloadFileMap;
   VolatileCounter m_refCount;
   UINT32 m_dwEncryptionRqId;
   UINT32 m_dwEncryptionResult;
   CONDITION m_condEncryptionSetup;
	CONSOLE_CTX m_console;			// Server console context
	StringList m_musicTypeList;
	ObjectIndex m_agentConn;
	StringObjectMap<UINT32> *m_subscriptions;
	MUTEX m_subscriptionLock;
	HashMap<UINT32, CommandExec> *m_serverCommands;

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);
   static void pollerThreadStarter(void *);

   DECLARE_THREAD_STARTER(cancelFileMonitoring)
   DECLARE_THREAD_STARTER(clearDCIData)
   DECLARE_THREAD_STARTER(createObject)
   DECLARE_THREAD_STARTER(deleteDCIEntry)
   DECLARE_THREAD_STARTER(executeAction)
   DECLARE_THREAD_STARTER(executeScript)
   DECLARE_THREAD_STARTER(executeLibraryScript)
   DECLARE_THREAD_STARTER(fileManagerControl)
   DECLARE_THREAD_STARTER(findIpAddress)
   DECLARE_THREAD_STARTER(findMacAddress)
   DECLARE_THREAD_STARTER(findHostname)
   DECLARE_THREAD_STARTER(findNodeConnection)
   DECLARE_THREAD_STARTER(forceDCIPoll)
   DECLARE_THREAD_STARTER(forwardToReportingServer)
   DECLARE_THREAD_STARTER(getAgentFile)
   DECLARE_THREAD_STARTER(getAlarmEvents)
   DECLARE_THREAD_STARTER(getAlarms)
   DECLARE_THREAD_STARTER(getCollectedData)
   DECLARE_THREAD_STARTER(getPredictedData)
   DECLARE_THREAD_STARTER(getLocationHistory)
   DECLARE_THREAD_STARTER(getNetworkPath)
   DECLARE_THREAD_STARTER(getRoutingTable)
   DECLARE_THREAD_STARTER(getServerFile)
   DECLARE_THREAD_STARTER(getServerLogQueryData)
   DECLARE_THREAD_STARTER(getSwitchForwardingDatabase)
   DECLARE_THREAD_STARTER(getTableCollectedData)
   DECLARE_THREAD_STARTER(importConfiguration)
   DECLARE_THREAD_STARTER(openHelpdeskIssue)
   DECLARE_THREAD_STARTER(processConsoleCommand)
   DECLARE_THREAD_STARTER(queryAgentTable)
   DECLARE_THREAD_STARTER(queryL2Topology)
   DECLARE_THREAD_STARTER(queryParameter)
   DECLARE_THREAD_STARTER(queryServerLog)
   DECLARE_THREAD_STARTER(sendMib)
   DECLARE_THREAD_STARTER(uploadUserFileToAgent)
   DECLARE_THREAD_STARTER(getRepositories)
   DECLARE_THREAD_STARTER(addRepository)
   DECLARE_THREAD_STARTER(modifyRepository)
   DECLARE_THREAD_STARTER(deleteRepository)

   void readThread();
   void writeThread();
   void processingThread();
   void pollerThread(Node *pNode, int iPollType, UINT32 dwRqId);

   void debugPrintf(int level, const TCHAR *format, ...);

   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void onFileUpload(BOOL bSuccess);
   void sendServerInfo(UINT32 dwRqId);
   void login(NXCPMessage *pRequest);
   void sendAllObjects(NXCPMessage *pRequest);
   void sendSelectedObjects(NXCPMessage *pRequest);
   void getConfigurationVariables(UINT32 dwRqId);
   void getPublicConfigurationVariable(NXCPMessage *request);
   void setDefaultConfigurationVariableValues(NXCPMessage *pRequest);
   void setConfigurationVariable(NXCPMessage *pRequest);
   void deleteConfigurationVariable(NXCPMessage *pRequest);
   void sendUserDB(UINT32 dwRqId);
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
   void sendEventDB(UINT32 dwRqId);
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
   void applyTemplate(NXCPMessage *pRequest);
   void getCollectedData(NXCPMessage *pRequest);
   void getTableCollectedData(NXCPMessage *pRequest);
	bool getCollectedDataFromDB(NXCPMessage *request, NXCPMessage *response, DataCollectionTarget *object, int dciType);
	void clearDCIData(NXCPMessage *pRequest);
	void deleteDCIEntry(NXCPMessage *request);
	void forceDCIPoll(NXCPMessage *pRequest);
   void changeDCIStatus(NXCPMessage *pRequest);
   void getLastValues(NXCPMessage *pRequest);
   void getLastValuesByDciId(NXCPMessage *pRequest);
   void getTableLastValues(NXCPMessage *pRequest);
	void getThresholdSummary(NXCPMessage *request);
   void openEPP(NXCPMessage *request);
   void closeEPP(UINT32 dwRqId);
   void saveEPP(NXCPMessage *pRequest);
   void processEPPRecord(NXCPMessage *pRequest);
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
   void LockPackageDB(UINT32 dwRqId, BOOL bLock);
   void SendAllPackages(UINT32 dwRqId);
   void InstallPackage(NXCPMessage *pRequest);
   void RemovePackage(NXCPMessage *pRequest);
   void DeployPackage(NXCPMessage *pRequest);
   void getParametersList(NXCPMessage *pRequest);
   void getUserVariable(NXCPMessage *pRequest);
   void setUserVariable(NXCPMessage *pRequest);
   void copyUserVariable(NXCPMessage *pRequest);
   void enumUserVariables(NXCPMessage *pRequest);
   void deleteUserVariable(NXCPMessage *pRequest);
   void changeObjectZone(NXCPMessage *pRequest);
   void getAgentConfig(NXCPMessage *pRequest);
   void updateAgentConfig(NXCPMessage *pRequest);
   void executeAction(NXCPMessage *pRequest);
   void getObjectTools(UINT32 requestId);
   void getObjectToolDetails(NXCPMessage *request);
   void updateObjectTool(NXCPMessage *pRequest);
   void deleteObjectTool(NXCPMessage *pRequest);
   void changeObjectToolStatus(NXCPMessage *pRequest);
   void generateObjectToolId(UINT32 dwRqId);
   void execTableTool(NXCPMessage *pRequest);
   void changeSubscription(NXCPMessage *pRequest);
   void sendServerStats(UINT32 dwRqId);
   void sendScriptList(UINT32 dwRqId);
   void sendScript(NXCPMessage *pRequest);
   void updateScript(NXCPMessage *pRequest);
   void renameScript(NXCPMessage *pRequest);
   void deleteScript(NXCPMessage *pRequest);
   void SendSessionList(UINT32 dwRqId);
   void KillSession(NXCPMessage *pRequest);
   void StartSnmpWalk(NXCPMessage *pRequest);
   void resolveDCINames(NXCPMessage *pRequest);
   UINT32 resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR *ppszName);
   void sendConfigForAgent(NXCPMessage *pRequest);
   void sendAgentCfgList(UINT32 dwRqId);
   void OpenAgentConfig(NXCPMessage *pRequest);
   void SaveAgentConfig(NXCPMessage *pRequest);
   void DeleteAgentConfig(NXCPMessage *pRequest);
   void SwapAgentConfigs(NXCPMessage *pRequest);
   void SendObjectComments(NXCPMessage *pRequest);
   void updateObjectComments(NXCPMessage *pRequest);
   void pushDCIData(NXCPMessage *pRequest);
   void getAddrList(NXCPMessage *request);
   void setAddrList(NXCPMessage *request);
   void resetComponent(NXCPMessage *pRequest);
   void getDCIEventList(NXCPMessage *request);
   void getDCIScriptList(NXCPMessage *request);
	void SendDCIInfo(NXCPMessage *pRequest);
   void sendPerfTabDCIList(NXCPMessage *pRequest);
   void exportConfiguration(NXCPMessage *pRequest);
   void importConfiguration(NXCPMessage *pRequest);
	void sendGraphList(NXCPMessage *request);
	void saveGraph(NXCPMessage *pRequest);
	void deleteGraph(NXCPMessage *pRequest);
	void addCACertificate(NXCPMessage *pRequest);
	void deleteCertificate(NXCPMessage *pRequest);
	void updateCertificateComments(NXCPMessage *pRequest);
	void getCertificateList(UINT32 dwRqId);
	void queryL2Topology(NXCPMessage *pRequest);
	void sendSMS(NXCPMessage *pRequest);
	void SendCommunityList(UINT32 dwRqId);
	void UpdateCommunityList(NXCPMessage *pRequest);
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
	void deployAgentPolicy(NXCPMessage *pRequest, bool uninstallFlag);
	void getUserCustomAttribute(NXCPMessage *request);
	void setUserCustomAttribute(NXCPMessage *request);
	void openServerLog(NXCPMessage *request);
	void closeServerLog(NXCPMessage *request);
	void queryServerLog(NXCPMessage *request);
	void getServerLogQueryData(NXCPMessage *request);
	void sendUsmCredentials(UINT32 dwRqId);
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
   void getPredictionEngines(NXCPMessage *request);
   void getPredictedData(NXCPMessage *request);
#ifdef WITH_ZMQ
   void zmqManageSubscription(NXCPMessage *request, zmq::SubscriptionType type, bool subscribe);
   void zmqListSubscriptions(NXCPMessage *request, zmq::SubscriptionType type);
#endif
   void registerServerCommand(CommandExec *command) { m_serverCommands->set(command->getStreamId(), command); }

   void alarmUpdateWorker(Alarm *alarm);
   void sendActionDBUpdateMessage(NXCP_MESSAGE *msg);
   void sendObjectUpdate(NetObj *object);

public:
   ClientSession(SOCKET hSocket, const InetAddress& addr);
   ~ClientSession();

   void incRefCount() { InterlockedIncrement(&m_refCount); }
   void decRefCount() { InterlockedDecrement(&m_refCount); }

   void run();

   void postMessage(NXCPMessage *pMsg) { if (!isTerminated()) m_sendQueue->put(pMsg->serialize((m_dwFlags & CSF_COMPRESSION_ENABLED) != 0)); }
   bool sendMessage(NXCPMessage *pMsg);
   void sendRawMessage(NXCP_MESSAGE *pMsg);
   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszMsg);
	BOOL sendFile(const TCHAR *file, UINT32 dwRqId, long offset);

   void writeAuditLog(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *format, ...);
   void writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *oldValue, const TCHAR *newValue, const TCHAR *format, ...);
   void writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...);

   int getId() const { return m_id; }
   void setId(int id) { if (m_id == -1) m_id = id; }

   int getState() const { return m_state; }
   const TCHAR *getLoginName() const { return m_loginName; }
   const TCHAR *getSessionName() const { return m_sessionName; }
   const TCHAR *getClientInfo() const { return m_clientInfo; }
	const TCHAR *getWorkstation() const { return m_workstation; }
   const TCHAR *getWebServerAddress() const { return m_webServerAddress; }
   UINT32 getUserId() const { return m_dwUserId; }
	UINT64 getSystemRights() const { return m_dwSystemAccess; }
   UINT32 getFlags() const { return m_dwFlags; }
   bool isAuthenticated() const { return (m_dwFlags & CSF_AUTHENTICATED) ? true : false; }
   bool isTerminated() const { return (m_dwFlags & CSF_TERMINATED) ? true : false; }
   bool isConsoleOpen() const { return (m_dwFlags & CSF_CONSOLE_OPEN) ? true : false; }
   bool isCompressionEnabled() const { return (m_dwFlags & CSF_COMPRESSION_ENABLED) ? true : false; }
   UINT16 getCurrentCmd() const { return m_wCurrentCmd; }
   int getCipher() const { return (m_pCtx == NULL) ? -1 : m_pCtx->getCipher(); }
	int getClientType() const { return m_clientType; }
   time_t getLoginTime() const { return m_loginTime; }
   bool isSubscribedTo(const TCHAR *channel) const;

	bool checkSysAccessRights(UINT64 requiredAccess) const
   {
      return (m_dwUserId == 0) ? true :
         ((requiredAccess & m_dwSystemAccess) == requiredAccess);
   }

   void setCustomLock(UINT32 bit, bool value)
   {
      if (value)
         m_dwFlags |= (bit & 0xFF000000);
      else
         m_dwFlags &= ~(bit & 0xFF000000);
   }

   void kill();
   void notify(UINT32 dwCode, UINT32 dwData = 0);

   void onNewEvent(Event *pEvent);
   void onSyslogMessage(NX_SYSLOG_RECORD *pRec);
   void onNewSNMPTrap(NXCPMessage *pMsg);
   void onObjectChange(NetObj *pObject);
   void onAlarmUpdate(UINT32 dwCode, const Alarm *alarm);
   void onActionDBUpdate(UINT32 dwCode, NXC_ACTION *pAction);
   void onLibraryImageChange(const uuid& guid, bool removed = false);
};

/**
 * Delayed SQL request
 */
typedef struct
{
	TCHAR *query;
	int bindCount;
	BYTE *sqlTypes;
	TCHAR *bindings[1]; /* actual size determined by bindCount field */
} DELAYED_SQL_REQUEST;

/**
 * Delayed request for idata_ INSERT
 */
typedef struct
{
	time_t timestamp;
	UINT32 nodeId;
	UINT32 dciId;
	TCHAR value[MAX_RESULT_LENGTH];
} DELAYED_IDATA_INSERT;

/**
 * Delayed request for raw_dci_values UPDATE
 */
typedef struct
{
	time_t timestamp;
	UINT32 dciId;
	TCHAR rawValue[MAX_RESULT_LENGTH];
	TCHAR transformedValue[MAX_RESULT_LENGTH];
} DELAYED_RAW_DATA_UPDATE;

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
 * Information about graph with name existence and it's ID.
 */
struct GRAPH_ACL_AND_ID
{
   UINT32 graphId;
   UINT32 status;
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
   THREAD_POOL_LOADAVG_15
};

/**
 * Server command execution data
 */
class ServerCommandExec : public CommandExec
{
private:
   UINT32 m_requestId;
   ClientSession *m_session;

   virtual void onOutput(const char *text);
   virtual void endOfOutput();

public:
   ServerCommandExec(NXCPMessage *msg, ClientSession *session);
   ~ServerCommandExec();
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
   TCHAR m_description[MAX_DB_STRING];

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
   UINT32 m_id;                   // Entry ID
   SNMP_ObjectId m_objectId;      // Trap OID
   UINT32 m_eventCode;            // Event code
   ObjectArray<SNMPTrapParameterMapping> m_mappings;
   TCHAR m_description[MAX_DB_STRING];
   TCHAR m_userTag[MAX_USERTAG_LENGTH];

public:
   SNMPTrapConfiguration();
   SNMPTrapConfiguration(DB_RESULT trapResult, DB_STATEMENT stmt, int row);
   SNMPTrapConfiguration(ConfigEntry *entry, const uuid& guid, UINT32 id, UINT32 eventCode);
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
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getUserTag() const { return m_userTag; }
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
 * Functions
 */
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
bool NXCORE_EXPORTABLE ConfigReadStrEx(DB_HANDLE hdb, const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
#ifdef UNICODE
bool NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault);
#else
#define ConfigReadStrA ConfigReadStr
#endif
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const TCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault);
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault);
int NXCORE_EXPORTABLE ConfigReadIntEx(DB_HANDLE hdb, const TCHAR *szVar, int iDefault);
UINT32 NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, UINT32 dwDefault);
bool NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *pszVar, int *pnArray, int nSize, int nDefault);
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *szVar, const TCHAR *szValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, UINT32 dwValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray, int nSize, bool bCreate, bool isVisible = true, bool needRestart = false);
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *var, const TCHAR *defValue);
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *var, const TCHAR *value, bool bCreate);
bool NXCORE_EXPORTABLE ConfigDelete(const TCHAR *name);

void MetaDataPreLoad();
bool NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
INT32 NXCORE_EXPORTABLE MetaDataReadInt32(const TCHAR *var, INT32 defaultValue);
bool NXCORE_EXPORTABLE MetaDataWriteStr(const TCHAR *varName, const TCHAR *value);
bool NXCORE_EXPORTABLE MetaDataWriteInt32(const TCHAR *name, INT32 value);

bool NXCORE_EXPORTABLE LoadConfig(int *debugLevel);

void NXCORE_EXPORTABLE Shutdown();
void NXCORE_EXPORTABLE FastShutdown();
BOOL NXCORE_EXPORTABLE Initialize();
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *);
void NXCORE_EXPORTABLE ShutdownDB();
void InitiateShutdown();

bool NXCORE_EXPORTABLE SleepAndCheckForShutdown(int iSeconds);

void ConsolePrintf(CONSOLE_CTX pCtx, const TCHAR *pszFormat, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void ConsoleWrite(CONSOLE_CTX pCtx, const TCHAR *text);
int ProcessConsoleCommand(const TCHAR *pszCmdLine, CONSOLE_CTX pCtx);

void SaveObjects(DB_HANDLE hdb, UINT32 watchdogId, bool saveRuntimeData);
void NXCORE_EXPORTABLE ObjectTransactionStart();
void NXCORE_EXPORTABLE ObjectTransactionEnd();

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query);
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values);
void QueueIDataInsert(time_t timestamp, UINT32 nodeId, UINT32 dciId, const TCHAR *value);
void QueueRawDciDataUpdate(time_t timestamp, UINT32 dciId, const TCHAR *rawValue, const TCHAR *transformedValue);
void StartDBWriter();
void StopDBWriter();

void PerfDataStorageRequest(DCItem *dci, time_t timestamp, const TCHAR *value);
void PerfDataStorageRequest(DCTable *dci, time_t timestamp, Table *value);

void DecodeSQLStringAndSetVariable(NXCPMessage *pMsg, UINT32 dwVarId, TCHAR *pszStr);

SNMP_Transport *SnmpCheckCommSettings(UINT32 snmpProxy, const InetAddress& ipAddr, INT16 *version, UINT16 originalPort, SNMP_SecurityContext *originalContext, StringList *customTestOids);
void StrToMac(const TCHAR *pszStr, BYTE *pBuffer);

void InitLocalNetInfo();

ARP_CACHE *GetLocalArpCache();
ARP_CACHE *SnmpGetArpCache(UINT32 dwVersion, SNMP_Transport *pTransport);

InterfaceList *GetLocalInterfaceList();

ROUTING_TABLE *SnmpGetRoutingTable(UINT32 dwVersion, SNMP_Transport *pTransport);

void LoadNetworkDeviceDrivers();
NetworkDeviceDriver *FindDriverForNode(Node *node, SNMP_Transport *pTransport);
NetworkDeviceDriver *FindDriverByName(const TCHAR *name);
void AddDriverSpecificOids(StringList *list);

bool LookupDevicePortLayout(const SNMP_ObjectId& objectId, NDD_MODULE_LAYOUT *layout);

void CheckForMgmtNode();
void CheckPotentialNode(const InetAddress& ipAddr, UINT32 zoneUIN);
Node NXCORE_EXPORTABLE *PollNewNode(NewNodeData *newNodeData);

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg);
void NXCORE_EXPORTABLE NotifyClientSessions(UINT32 dwCode, UINT32 dwData);
void NXCORE_EXPORTABLE NotifyClientSession(UINT32 sessionId, UINT32 dwCode, UINT32 dwData);
void NXCORE_EXPORTABLE NotifyClientGraphUpdate(NXCPMessage *update, UINT32 graphId);
int GetSessionCount(bool includeSystemAccount);
bool IsLoggedIn(UINT32 dwUserId);
bool NXCORE_EXPORTABLE KillClientSession(int id);
void CloseOtherSessions(UINT32 userId, UINT32 thisSession);

void GetSysInfoStr(TCHAR *pszBuffer, int nMaxSize);
InetAddress GetLocalIpAddr();
TCHAR *GetLocalHostName(TCHAR *buffer, size_t bufSize);

InetAddress NXCORE_EXPORTABLE ResolveHostName(UINT32 zoneUIN, const TCHAR *hostname);

BOOL ExecCommand(TCHAR *pszCommand);
BOOL SendMagicPacket(UINT32 dwIpAddr, BYTE *pbMacAddr, int iNumPackets);

BOOL InitIdTable();
UINT32 CreateUniqueId(int iGroup);
QWORD CreateUniqueEventId();
void SaveCurrentFreeId();

void InitMailer();
void ShutdownMailer();
void NXCORE_EXPORTABLE PostMail(const TCHAR *pszRcpt, const TCHAR *pszSubject, const TCHAR *pszText, bool isHtml = false);

void InitSMSSender();
void ShutdownSMSSender();
void NXCORE_EXPORTABLE PostSMS(const TCHAR *pszRcpt, const TCHAR *pszText);

void InitTraps();
void SendTrapsToClient(ClientSession *pSession, UINT32 dwRqId);
void CreateTrapCfgMessage(NXCPMessage *msg);
UINT32 CreateNewTrap(UINT32 *pdwTrapId);
UINT32 UpdateTrapFromMsg(NXCPMessage *pMsg);
UINT32 DeleteTrap(UINT32 dwId);
void CreateTrapExportRecord(String &xml, UINT32 id);
UINT32 ResolveTrapGuid(const uuid& guid);
void AddTrapCfgToList(SNMPTrapConfiguration *trapCfg);

BOOL IsTableTool(UINT32 dwToolId);
BOOL CheckObjectToolAccess(UINT32 dwToolId, UINT32 dwUserId);
UINT32 ExecuteTableTool(UINT32 dwToolId, Node *pNode, UINT32 dwRqId, ClientSession *pSession);
UINT32 DeleteObjectToolFromDB(UINT32 dwToolId);
UINT32 ChangeObjectToolStatus(UINT32 toolId, bool enabled);
UINT32 UpdateObjectToolFromMessage(NXCPMessage *pMsg);
void CreateObjectToolExportRecord(String &xml, UINT32 id);
bool ImportObjectTool(ConfigEntry *config);
UINT32 GetObjectToolsIntoMessage(NXCPMessage *msg, UINT32 userId, bool fullAccess);
UINT32 GetObjectToolDetailsIntoMessage(UINT32 toolId, NXCPMessage *msg);

UINT32 ModifySummaryTable(NXCPMessage *msg, LONG *newId);
UINT32 DeleteSummaryTable(LONG tableId);
Table *QuerySummaryTable(LONG tableId, SummaryTable *adHocDefinition, UINT32 baseObjectId, UINT32 userId, UINT32 *rcc);
bool CreateSummaryTableExportRecord(INT32 id, String &xml);
bool ImportSummaryTable(ConfigEntry *config);

void CreateMessageFromSyslogMsg(NXCPMessage *pMsg, NX_SYSLOG_RECORD *pRec);
void ReinitializeSyslogParser();
void OnSyslogConfigurationChange(const TCHAR *name, const TCHAR *value);

void EscapeString(String &str);

void InitAuditLog();
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                     const TCHAR *workstation, int sessionId, UINT32 objectId,
                                     const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLog2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                      const TCHAR *workstation, int sessionId, UINT32 objectId,
                                      const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithValues(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                               const TCHAR *workstation, int sessionId, UINT32 objectId,
                                               const TCHAR *oldValue, const TCHAR *newValue,
                                               const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                                   const TCHAR *workstation, int sessionId, UINT32 objectId,
                                                   json_t *oldValue, json_t *newValue,
                                                   const TCHAR *format, ...);
void NXCORE_EXPORTABLE WriteAuditLogWithValues2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                                const TCHAR *workstation, int sessionId, UINT32 objectId,
                                                const TCHAR *oldValue, const TCHAR *newValue,
                                                const TCHAR *format, va_list args);
void NXCORE_EXPORTABLE WriteAuditLogWithJsonValues2(const TCHAR *subsys, bool isSuccess, UINT32 userId,
                                                    const TCHAR *workstation, int sessionId, UINT32 objectId,
                                                    json_t *oldValue, json_t *newValue,
                                                    const TCHAR *format, va_list args);

bool ValidateConfig(Config *config, UINT32 flags, TCHAR *errorText, int errorTextLen);
UINT32 ImportConfig(Config *config, UINT32 flags);

#ifdef _WITH_ENCRYPTION
X509 *CertificateFromLoginMessage(NXCPMessage *pMsg);
bool ValidateUserCertificate(X509 *cert, const TCHAR *login, const BYTE *challenge,
									  const BYTE *signature, size_t sigLen, int mappingMethod,
									  const TCHAR *mappingData);
bool ValidateAgentCertificate(X509 *cert);
void ReloadCertificates();
bool GetCertificateSubjectField(X509 *cert, int nid, TCHAR *buffer, size_t size);
bool GetCertificateCN(X509 *cert, TCHAR *buffer, size_t size);
bool GetCertificateOU(X509 *cert, TCHAR *buffer, size_t size);
String GetCertificateSubjectString(X509 *cert);
bool GetServerCertificateCountry(TCHAR *buffer, size_t size);
bool GetServerCertificateOrganization(TCHAR *buffer, size_t size);
X509 *IssueCertificate(X509_REQ *request, const char *ou, const char *cn, int days);
#endif

#ifndef _WIN32
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);
#endif   /* not _WIN32 */

void DbgTestRWLock(RWLOCK hLock, const TCHAR *szName, CONSOLE_CTX console);
void DumpClientSessions(CONSOLE_CTX console);
void DumpMobileDeviceSessions(CONSOLE_CTX console);
void ShowServerStats(CONSOLE_CTX console);
void ShowQueueStats(CONSOLE_CTX console, Queue *pQueue, const TCHAR *pszName);
void ShowThreadPoolPendingQueue(CONSOLE_CTX console, ThreadPool *p, const TCHAR *pszName);
void ShowThreadPool(CONSOLE_CTX console, ThreadPool *p);
DataCollectionError GetThreadPoolStat(ThreadPoolStat stat, const TCHAR *param, TCHAR *value);
void DumpProcess(CONSOLE_CTX console);

#define GRAPH_FLAG_TEMPLATE 1

GRAPH_ACL_ENTRY *LoadGraphACL(DB_HANDLE hdb, UINT32 graphId, int *pnACLSize);
GRAPH_ACL_ENTRY *LoadAllGraphACL(DB_HANDLE hdb, int *pnACLSize);
BOOL CheckGraphAccess(GRAPH_ACL_ENTRY *pACL, int nACLSize, UINT32 graphId, UINT32 graphUserId, UINT32 graphDesiredAccess);
UINT32 GetGraphAccessCheckResult(UINT32 graphId, UINT32 graphUserId);
GRAPH_ACL_AND_ID IsGraphNameExists(const TCHAR *graphName);
void FillGraphListMsg(NXCPMessage *msg, UINT32 userId, bool templageGraphs);
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

/**
 * Watchdog API
 */
void WatchdogInit();
void WatchdogShutdown();
UINT32 WatchdogAddThread(const TCHAR *name, time_t notifyInterval);
void WatchdogNotify(UINT32 id);
void WatchdogStartSleep(UINT32 id);
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
 * Alarm category functions
 */
void GetAlarmCategories(NXCPMessage *msg);
UINT32 UpdateAlarmCategory(const NXCPMessage *request, UINT32 *returnId);
UINT32 DeleteAlarmCategory(UINT32 id);
bool CheckAlarmCategoryAccess(UINT32 userId, UINT32 categoryId);
void LoadAlarmCategories();

/**
 * Alarm summary emails
 */
void SendAlarmSummaryEmail(const ScheduledTaskParameters *params);
void EnableAlarmSummaryEmails();

/**
 * NXSL script functions
 */
UINT32 UpdateScript(const NXCPMessage *request, UINT32 *scriptId);
UINT32 RenameScript(const NXCPMessage *request);
UINT32 DeleteScript(const NXCPMessage *request);

/**
 * File monitoring
 */
struct MONITORED_FILE
{
   TCHAR fileName[MAX_PATH];
   ClientSession *session;
   UINT32 nodeID;
};

class FileMonitoringList
{
private:
   MUTEX m_mutex;
   ObjectArray<MONITORED_FILE>  m_monitoredFiles;

public:
   FileMonitoringList();
   ~FileMonitoringList();
   void addMonitoringFile(MONITORED_FILE *fileForAdd, Node *obj, AgentConnection *conn);
   bool checkDuplicate(MONITORED_FILE *fileForAdd);
   ObjectArray<ClientSession>* findClientByFNameAndNodeID(const TCHAR *fileName, UINT32 nodeID);
   bool removeMonitoringFile(MONITORED_FILE *fileForRemove);
   void removeDisconnectedNode(UINT32 nodeId);

private:
   void lock();
   void unlock();
};

/**********************
 * Distance calculation
 **********************/

/**
 * Class stores object and distance to it from provided node
 */
class ObjectsDistance
{
public:
   NetObj *m_obj;
   int m_distance;

   ObjectsDistance(NetObj *obj, int distance) { m_obj = obj; m_distance = distance; }
   ~ObjectsDistance() { m_obj->decRefCount(); }
};

/**
 * Calculate nearest objects from current one
 * Object ref count will be automatically decreased on array delete
 */
ObjectArray<ObjectsDistance> *FindNearestObjects(UINT32 currObjectId, int maxDistance, bool (* filter)(NetObj *object, void *data), void *sortData, int (* calculateRealDistance)(GeoLocation &loc1, GeoLocation &loc2));

/**
 * Global variables
 */
extern TCHAR NXCORE_EXPORTABLE g_szConfigFile[];
extern TCHAR NXCORE_EXPORTABLE g_szLogFile[];
extern UINT32 g_logRotationMode;
extern UINT64 g_maxLogSize;
extern UINT32 g_logHistorySize;
extern TCHAR g_szDailyLogFileSuffix[64];
extern TCHAR NXCORE_EXPORTABLE g_szDumpDir[];
extern TCHAR NXCORE_EXPORTABLE g_szListenAddress[];
#ifndef _WIN32
extern TCHAR NXCORE_EXPORTABLE g_szPIDFile[];
#endif
extern TCHAR NXCORE_EXPORTABLE g_netxmsdDataDir[];
extern TCHAR NXCORE_EXPORTABLE g_netxmsdLibDir[];
extern UINT32 NXCORE_EXPORTABLE g_processAffinityMask;
extern UINT64 g_serverId;
extern RSA *g_pServerKey;
extern UINT32 g_icmpPingSize;
extern UINT32 g_icmpPingTimeout;
extern UINT32 g_auditFlags;
extern time_t g_serverStartTime;
extern UINT32 g_lockTimeout;
extern UINT32 g_agentCommandTimeout;
extern UINT32 g_thresholdRepeatInterval;
extern UINT32 g_requiredPolls;
extern UINT32 g_slmPollingInterval;
extern UINT32 g_offlineDataRelevanceTime;
extern INT32 g_instanceRetentionTime;

extern TCHAR g_szDbDriver[];
extern TCHAR g_szDbDrvParams[];
extern TCHAR g_szDbServer[];
extern TCHAR g_szDbLogin[];
extern TCHAR g_szDbPassword[];
extern TCHAR g_szDbName[];
extern TCHAR g_szDbSchema[];
extern DB_DRIVER g_dbDriver;
extern Queue *g_dbWriterQueue;
extern Queue *g_dciDataWriterQueue;
extern Queue *g_dciRawDataWriterQueue;
extern UINT64 g_idataWriteRequests;
extern UINT64 g_rawDataWriteRequests;
extern UINT64 g_otherWriteRequests;

extern int NXCORE_EXPORTABLE g_dbSyntax;
extern FileMonitoringList g_monitoringList;

extern ThreadPool NXCORE_EXPORTABLE *g_mainThreadPool;

#endif   /* _nms_core_h_ */
