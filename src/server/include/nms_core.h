/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
#include "server_timers.h"
#include "nms_dcoll.h"
#include "nms_users.h"
#include "nxcore_winperf.h"
#include "nms_objects.h"
#include "nms_locks.h"
#include "nms_pkg.h"
#include "nms_topo.h"
#include "nms_script.h"
#include "nxcore_situations.h"
#include "nxcore_jobs.h"
#include "nxcore_logs.h"

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
#define IDG_CONTAINER_CAT     1
#define IDG_EVENT             2
#define IDG_ITEM              3
#define IDG_SNMP_TRAP         4
#define IDG_JOB               5
#define IDG_ACTION            6
#define IDG_EVENT_GROUP       7
#define IDG_THRESHOLD         8
#define IDG_USER              9
#define IDG_USER_GROUP        10
#define IDG_ALARM             11
#define IDG_ALARM_NOTE        12
#define IDG_PACKAGE           13
#define IDG_SLM_TICKET        14
#define IDG_OBJECT_TOOL       15
#define IDG_SCRIPT            16
#define IDG_AGENT_CONFIG      17
#define IDG_GRAPH					18
#define IDG_CERTIFICATE			19
#define IDG_SITUATION         20
#define IDG_DCT_COLUMN        21
#define IDG_MAPPING_TABLE     22
#define IDG_DCI_SUMMARY_TABLE 23

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
 * Information categories for UPDATE_INFO structure
 */
#define INFO_CAT_EVENT           1
#define INFO_CAT_OBJECT_CHANGE   2
#define INFO_CAT_ALARM           3
#define INFO_CAT_ACTION          4
#define INFO_CAT_SYSLOG_MSG      5
#define INFO_CAT_SNMP_TRAP       6
#define INFO_CAT_AUDIT_RECORD    7
#define INFO_CAT_SITUATION       8
#define INFO_CAT_LIBRARY_IMAGE   9

/**
 * Certificate types
 */
#define CERT_TYPE_TRUSTED_CA		0
#define CERT_TYPE_USER				1

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
	UINT32 zoneId;
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
 * Node information for autodiscovery filter
 */
typedef struct
{
   InetAddress ipAddr;
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
 * Extended agent connection
 */
class AgentConnectionEx : public AgentConnection
{
protected:
	UINT32 m_nodeId;

   virtual void printMsg(const TCHAR *format, ...);
   virtual void onTrap(NXCPMessage *msg);
   virtual void onDataPush(NXCPMessage *msg);
   virtual void onFileMonitoringData(NXCPMessage *msg);
	virtual void onSnmpTrap(NXCPMessage *pMsg);
	virtual UINT32 processCollectedData(NXCPMessage *msg);
   virtual bool processCustomMessage(NXCPMessage *msg);

public:
   AgentConnectionEx(UINT32 nodeId, InetAddress ipAddr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL) :
            AgentConnection(ipAddr, port, authMethod, secret) { m_nodeId = nodeId; }
   virtual ~AgentConnectionEx();

	UINT32 deployPolicy(AgentPolicy *policy);
	UINT32 uninstallPolicy(AgentPolicy *policy);
};

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
	struct sockaddr *m_clientAddr;
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
   MobileDeviceSession(SOCKET hSocket, struct sockaddr *addr);
   ~MobileDeviceSession();

   void run();

   void postMessage(NXCPMessage *pMsg) { m_pSendQueue->put(pMsg->createMessage()); }
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
 * Client (user) session
 */
#define DECLARE_THREAD_STARTER(func) static void ThreadStarter_##func(void *);

class NXCORE_EXPORTABLE ClientSession
{
private:
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   Queue *m_pUpdateQueue;
   int m_id;
   int m_state;
   WORD m_wCurrentCmd;
   UINT32 m_dwUserId;
   UINT64 m_dwSystemAccess;    // User's system access rights
   UINT32 m_dwFlags;           // Session flags
	int m_clientType;				// Client system type - desktop, web, mobile, etc.
   NXCP_BUFFER *m_pMsgBuffer;
   NXCPEncryptionContext *m_pCtx;
	BYTE m_challenge[CLIENT_CHALLENGE_SIZE];
   THREAD m_hWriteThread;
   THREAD m_hProcessingThread;
   THREAD m_hUpdateThread;
	MUTEX m_mutexSocketWrite;
   MUTEX m_mutexSendEvents;
   MUTEX m_mutexSendSyslog;
   MUTEX m_mutexSendTrapLog;
   MUTEX m_mutexSendObjects;
   MUTEX m_mutexSendAlarms;
   MUTEX m_mutexSendActions;
	MUTEX m_mutexSendAuditLog;
	MUTEX m_mutexSendSituations;
   MUTEX m_mutexPollerInit;
	struct sockaddr *m_clientAddr;
	TCHAR m_workstation[256];      // IP address or name of connected host in textual form
   TCHAR m_webServerAddress[256]; // IP address or name of web server for web sessions
   TCHAR m_szUserName[MAX_SESSION_NAME];   // String in form login_name@host
   TCHAR m_szClientInfo[96];  // Client app info string
   TCHAR m_language[8];       // Client's desired language
   time_t m_loginTime;
   UINT32 m_dwOpenDCIListSize; // Number of open DCI lists
   UINT32 *m_pOpenDCIList;     // List of nodes with DCI lists open
   UINT32 m_dwNumRecordsToUpload; // Number of records to be uploaded
   UINT32 m_dwRecordsUploaded;
   EPRule **m_ppEPPRuleList;   // List of loaded EPP rules
   int m_hCurrFile;
   UINT32 m_dwFileRqId;
   UINT32 m_dwUploadCommand;
   UINT32 m_dwUploadData;
   uuid_t m_uploadImageGuid;
   TCHAR m_szCurrFileName[MAX_PATH];
   UINT32 m_dwRefCount;
   UINT32 m_dwEncryptionRqId;
   UINT32 m_dwEncryptionResult;
   CONDITION m_condEncryptionSetup;
   UINT32 m_dwActiveChannels;     // Active data channels
	CONSOLE_CTX m_console;			// Server console context
	StringList m_musicTypeList;
	ObjectIndex m_agentConn;

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL updateThreadStarter(void *);
   static void pollerThreadStarter(void *);

	DECLARE_THREAD_STARTER(getCollectedData)
	DECLARE_THREAD_STARTER(getTableCollectedData)
	DECLARE_THREAD_STARTER(clearDCIData)
	DECLARE_THREAD_STARTER(forceDCIPoll)
	DECLARE_THREAD_STARTER(queryParameter)
	DECLARE_THREAD_STARTER(queryAgentTable)
	DECLARE_THREAD_STARTER(queryL2Topology)
	DECLARE_THREAD_STARTER(sendEventLog)
	DECLARE_THREAD_STARTER(sendSyslog)
	DECLARE_THREAD_STARTER(createObject)
	DECLARE_THREAD_STARTER(getServerFile)
	DECLARE_THREAD_STARTER(getAgentFile)
	DECLARE_THREAD_STARTER(cancelFileMonitoring)
	DECLARE_THREAD_STARTER(queryServerLog)
	DECLARE_THREAD_STARTER(getServerLogQueryData)
	DECLARE_THREAD_STARTER(executeAction)
	DECLARE_THREAD_STARTER(findNodeConnection)
	DECLARE_THREAD_STARTER(findMacAddress)
	DECLARE_THREAD_STARTER(findIpAddress)
	DECLARE_THREAD_STARTER(processConsoleCommand)
	DECLARE_THREAD_STARTER(sendMib)
	DECLARE_THREAD_STARTER(getNetworkPath)
	DECLARE_THREAD_STARTER(getAlarmEvents)
	DECLARE_THREAD_STARTER(openHelpdeskIssue)
	DECLARE_THREAD_STARTER(forwardToReportingServer)
   DECLARE_THREAD_STARTER(fileManagerControl)
   DECLARE_THREAD_STARTER(uploadUserFileToAgent)
   DECLARE_THREAD_STARTER(getSwitchForwardingDatabase)
   DECLARE_THREAD_STARTER(getRoutingTable)
   DECLARE_THREAD_STARTER(getLocationHistory)
   DECLARE_THREAD_STARTER(executeScript)

   void readThread();
   void writeThread();
   void processingThread();
   void updateThread();
   void pollerThread(Node *pNode, int iPollType, UINT32 dwRqId);

   void setupEncryption(NXCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void onFileUpload(BOOL bSuccess);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(UINT32 dwRqId);
   void login(NXCPMessage *pRequest);
   void sendAllObjects(NXCPMessage *pRequest);
   void sendSelectedObjects(NXCPMessage *pRequest);
   void sendEventLog(NXCPMessage *pRequest);
   void sendAllConfigVars(UINT32 dwRqId);
   void setConfigVariable(NXCPMessage *pRequest);
   void deleteConfigVariable(NXCPMessage *pRequest);
   void sendUserDB(UINT32 dwRqId);
   void sendAllAlarms(UINT32 dwRqId);
   void createUser(NXCPMessage *pRequest);
   void updateUser(NXCPMessage *pRequest);
   void deleteUser(NXCPMessage *pRequest);
   void setPassword(NXCPMessage *pRequest);
   void lockUserDB(UINT32 dwRqId, BOOL bLock);
   void sendEventDB(UINT32 dwRqId);
   void modifyEventTemplate(NXCPMessage *pRequest);
   void deleteEventTemplate(NXCPMessage *pRequest);
   void generateEventCode(UINT32 dwRqId);
   void modifyObject(NXCPMessage *pRequest);
   void changeObjectMgmtStatus(NXCPMessage *pRequest);
   void openNodeDCIList(NXCPMessage *pRequest);
   void closeNodeDCIList(NXCPMessage *pRequest);
   void modifyNodeDCI(NXCPMessage *pRequest);
   void copyDCI(NXCPMessage *pRequest);
   void applyTemplate(NXCPMessage *pRequest);
   void getCollectedData(NXCPMessage *pRequest);
   void getTableCollectedData(NXCPMessage *pRequest);
	bool getCollectedDataFromDB(NXCPMessage *request, NXCPMessage *response, DataCollectionTarget *object, int dciType);
	void clearDCIData(NXCPMessage *pRequest);
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
   void getAlarm(NXCPMessage *request);
   void getAlarmEvents(NXCPMessage *request);
   void acknowledgeAlarm(NXCPMessage *request);
   void resolveAlarm(NXCPMessage *request, bool terminate);
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
   void SendContainerCategories(UINT32 dwRqId);
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
   void sendObjectTools(UINT32 dwRqId);
   void sendObjectToolDetails(NXCPMessage *pRequest);
   void updateObjectTool(NXCPMessage *pRequest);
   void deleteObjectTool(NXCPMessage *pRequest);
   void changeObjectToolStatus(NXCPMessage *pRequest);
   void generateObjectToolId(UINT32 dwRqId);
   void execTableTool(NXCPMessage *pRequest);
   void changeSubscription(NXCPMessage *pRequest);
   void sendSyslog(NXCPMessage *pRequest);
   void sendServerStats(UINT32 dwRqId);
   void sendScriptList(UINT32 dwRqId);
   void sendScript(NXCPMessage *pRequest);
   void updateScript(NXCPMessage *pRequest);
   void renameScript(NXCPMessage *pRequest);
   void deleteScript(NXCPMessage *pRequest);
   void SendSessionList(UINT32 dwRqId);
   void KillSession(NXCPMessage *pRequest);
   void SendTrapLog(NXCPMessage *pRequest);
   void StartSnmpWalk(NXCPMessage *pRequest);
   void resolveDCINames(NXCPMessage *pRequest);
   UINT32 resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR **ppszName);
   void sendConfigForAgent(NXCPMessage *pRequest);
   void sendAgentCfgList(UINT32 dwRqId);
   void OpenAgentConfig(NXCPMessage *pRequest);
   void SaveAgentConfig(NXCPMessage *pRequest);
   void DeleteAgentConfig(NXCPMessage *pRequest);
   void SwapAgentConfigs(NXCPMessage *pRequest);
   void SendObjectComments(NXCPMessage *pRequest);
   void updateObjectComments(NXCPMessage *pRequest);
   void pushDCIData(NXCPMessage *pRequest);
   void getAddrList(NXCPMessage *pRequest);
   void setAddrList(NXCPMessage *pRequest);
   void resetComponent(NXCPMessage *pRequest);
   void getDCIEventList(NXCPMessage *request);
   void getDCIScriptList(NXCPMessage *request);
	void SendDCIInfo(NXCPMessage *pRequest);
   void sendPerfTabDCIList(NXCPMessage *pRequest);
   void exportConfiguration(NXCPMessage *pRequest);
   void importConfiguration(NXCPMessage *pRequest);
	void sendGraphList(UINT32 dwRqId);
	void saveGraph(NXCPMessage *pRequest);
	void deleteGraph(NXCPMessage *pRequest);
	void AddCACertificate(NXCPMessage *pRequest);
	void DeleteCertificate(NXCPMessage *pRequest);
	void UpdateCertificateComments(NXCPMessage *pRequest);
	void getCertificateList(UINT32 dwRqId);
	void queryL2Topology(NXCPMessage *pRequest);
	void sendSMS(NXCPMessage *pRequest);
	void SendCommunityList(UINT32 dwRqId);
	void UpdateCommunityList(NXCPMessage *pRequest);
	void getSituationList(UINT32 dwRqId);
	void createSituation(NXCPMessage *pRequest);
	void updateSituation(NXCPMessage *pRequest);
	void deleteSituation(NXCPMessage *pRequest);
	void deleteSituationInstance(NXCPMessage *pRequest);
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
	void sendLibraryImage(NXCPMessage *request);
	void updateLibraryImage(NXCPMessage *request);
	void listLibraryImages(NXCPMessage *request);
	void deleteLibraryImage(NXCPMessage *request);
	void executeServerCommand(NXCPMessage *request);
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

public:
   ClientSession(SOCKET hSocket, struct sockaddr *addr);
   ~ClientSession();

   void incRefCount() { m_dwRefCount++; }
   void decRefCount() { if (m_dwRefCount > 0) m_dwRefCount--; }

   void run();

   void postMessage(NXCPMessage *pMsg) { m_pSendQueue->put(pMsg->createMessage()); }
   void sendMessage(NXCPMessage *pMsg);
   void sendRawMessage(NXCP_MESSAGE *pMsg);
   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszMsg);
	BOOL sendFile(const TCHAR *file, UINT32 dwRqId, long offset);

   int getId() { return m_id; }
   void setId(int id) { if (m_id == -1) m_id = id; }
   int getState() { return m_state; }
   const TCHAR *getUserName() { return m_szUserName; }
   const TCHAR *getClientInfo() { return m_szClientInfo; }
	const TCHAR *getWorkstation() { return m_workstation; }
   const TCHAR *getWebServerAddress() { return m_webServerAddress; }
   UINT32 getUserId() { return m_dwUserId; }
	UINT64 getSystemRights() { return m_dwSystemAccess; }
   UINT32 getFlags() { return m_dwFlags; }
   bool isAuthenticated() { return (m_dwFlags & CSF_AUTHENTICATED) ? true : false; }
   bool isTerminated() { return (m_dwFlags & CSF_TERMINATED) ? true : false; }
   bool isConsoleOpen() { return (m_dwFlags & CSF_CONSOLE_OPEN) ? true : false; }
   bool isSubscribed(UINT32 dwChannel) { return (m_dwActiveChannels & dwChannel) ? true : false; }
   WORD getCurrentCmd() { return m_wCurrentCmd; }
   int getCipher() { return (m_pCtx == NULL) ? -1 : m_pCtx->getCipher(); }
	int getClientType() { return m_clientType; }
   time_t getLoginTime() { return m_loginTime; }

	bool checkSysAccessRights(UINT64 requiredAccess)
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

	void queueUpdate(UPDATE_INFO *pUpdate) { m_pUpdateQueue->put(pUpdate); }
   void onNewEvent(Event *pEvent);
   void onSyslogMessage(NX_SYSLOG_RECORD *pRec);
   void onNewSNMPTrap(NXCPMessage *pMsg);
   void onObjectChange(NetObj *pObject);
   void onUserDBUpdate(int code, UINT32 id, UserDatabaseObject *user);
   void onAlarmUpdate(UINT32 dwCode, NXC_ALARM *pAlarm);
   void onActionDBUpdate(UINT32 dwCode, NXC_ACTION *pAction);
   void onSituationChange(NXCPMessage *msg);
   void onLibraryImageChange(uuid_t *guid, bool removed = false);
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
 * Functions
 */
bool NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
#ifdef UNICODE
bool NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault);
#else
#define ConfigReadStrA ConfigReadStr
#endif
bool NXCORE_EXPORTABLE ConfigReadStrUTF8(const TCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault);
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault);
UINT32 NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, UINT32 dwDefault);
bool NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *pszVar, int *pnArray, int nSize, int nDefault);
bool NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *szVar, const TCHAR *szValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, UINT32 dwValue, bool bCreate, bool isVisible = true, bool needRestart = false);
bool NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray, int nSize, bool bCreate, bool isVisible = true, bool needRestart = false);
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *var, const TCHAR *defValue);
bool NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *var, const TCHAR *value, bool bCreate);
bool NXCORE_EXPORTABLE ConfigDelete(const TCHAR *name);

bool NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
INT32 NXCORE_EXPORTABLE MetaDataReadInt(const TCHAR *var, UINT32 defaultValue);
bool NXCORE_EXPORTABLE MetaDataWriteStr(const TCHAR *varName, const TCHAR *value);

bool NXCORE_EXPORTABLE LoadConfig();

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

void SaveObjects(DB_HANDLE hdb);
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

SNMP_SecurityContext *SnmpCheckCommSettings(SNMP_Transport *pTransport, INT16 *version, SNMP_SecurityContext *originalContext, StringList *customTestOids);
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

void WatchdogInit();
UINT32 WatchdogAddThread(const TCHAR *szName, time_t tNotifyInterval);
void WatchdogNotify(UINT32 dwId);
void WatchdogPrintStatus(CONSOLE_CTX pCtx);

void CheckForMgmtNode();
Node NXCORE_EXPORTABLE *PollNewNode(const InetAddress& ipAddr, UINT32 dwCreationFlags, WORD agentPort,
                                    WORD snmpPort, const TCHAR *pszName, UINT32 dwProxyNode, UINT32 dwSNMPProxy, Cluster *pCluster,
						                  UINT32 zoneId, bool doConfPoll, bool discoveredNode);

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg);
void NXCORE_EXPORTABLE NotifyClientSessions(UINT32 dwCode, UINT32 dwData);
void NXCORE_EXPORTABLE NotifyClientGraphUpdate(NXCPMessage *update, UINT32 graphId);
int GetSessionCount(bool withRoot = true);
bool IsLoggedIn(UINT32 dwUserId);
bool NXCORE_EXPORTABLE KillClientSession(int id);

void GetSysInfoStr(TCHAR *pszBuffer, int nMaxSize);
InetAddress GetLocalIpAddr();
TCHAR *GetLocalHostName(TCHAR *buffer, size_t bufSize);

BOOL ExecCommand(TCHAR *pszCommand);
BOOL SendMagicPacket(UINT32 dwIpAddr, BYTE *pbMacAddr, int iNumPackets);

BOOL InitIdTable();
UINT32 CreateUniqueId(int iGroup);
QWORD CreateUniqueEventId();
void SaveCurrentFreeId();

void InitMailer();
void ShutdownMailer();
void NXCORE_EXPORTABLE PostMail(const TCHAR *pszRcpt, const TCHAR *pszSubject, const TCHAR *pszText);

void InitSMSSender();
void ShutdownSMSSender();
void NXCORE_EXPORTABLE PostSMS(const TCHAR *pszRcpt, const TCHAR *pszText);

void InitTraps();
void SendTrapsToClient(ClientSession *pSession, UINT32 dwRqId);
void CreateTrapCfgMessage(NXCPMessage &msg);
UINT32 CreateNewTrap(UINT32 *pdwTrapId);
UINT32 CreateNewTrap(NXC_TRAP_CFG_ENTRY *pTrap);
UINT32 UpdateTrapFromMsg(NXCPMessage *pMsg);
UINT32 DeleteTrap(UINT32 dwId);
void CreateTrapExportRecord(String &xml, UINT32 id);

BOOL IsTableTool(UINT32 dwToolId);
BOOL CheckObjectToolAccess(UINT32 dwToolId, UINT32 dwUserId);
UINT32 ExecuteTableTool(UINT32 dwToolId, Node *pNode, UINT32 dwRqId, ClientSession *pSession);
UINT32 DeleteObjectToolFromDB(UINT32 dwToolId);
UINT32 ChangeObjectToolStatus(UINT32 toolId, bool enabled);
UINT32 UpdateObjectToolFromMessage(NXCPMessage *pMsg);
void CreateObjectToolExportRecord(String &xml, UINT32 id);
bool ImportObjectTool(ConfigEntry *config);

UINT32 ModifySummaryTable(NXCPMessage *msg, LONG *newId);
UINT32 DeleteSummaryTable(LONG tableId);
Table *QuerySummaryTable(LONG tableId, SummaryTable *adHocDefinition, UINT32 baseObjectId, UINT32 userId, UINT32 *rcc);
bool CreateSummaryTableExportRecord(INT32 id, String &xml);
bool ImportSummaryTable(ConfigEntry *config);

void CreateMessageFromSyslogMsg(NXCPMessage *pMsg, NX_SYSLOG_RECORD *pRec);
void ReinitializeSyslogParser();

void EscapeString(String &str);

void InitAuditLog();
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, BOOL isSuccess, UINT32 userId,
                                     const TCHAR *workstation, int sessionId, UINT32 objectId,
                                     const TCHAR *format, ...);

bool ValidateConfig(Config *config, UINT32 flags, TCHAR *errorText, int errorTextLen);
UINT32 ImportConfig(Config *config, UINT32 flags);

#ifdef _WITH_ENCRYPTION
X509 *CertificateFromLoginMessage(NXCPMessage *pMsg);
BOOL ValidateUserCertificate(X509 *pCert, const TCHAR *pszLogin, BYTE *pChallenge,
									  BYTE *pSignature, UINT32 dwSigLen, int nMappingMethod,
									  const TCHAR *pszMappingData);
void ReloadCertificates();
#endif

#ifndef _WIN32
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);
#endif   /* not _WIN32 */

void DbgTestRWLock(RWLOCK hLock, const TCHAR *szName, CONSOLE_CTX console);
void DumpClientSessions(CONSOLE_CTX console);
void DumpMobileDeviceSessions(CONSOLE_CTX console);
void ShowServerStats(CONSOLE_CTX console);
void ShowQueueStats(CONSOLE_CTX console, Queue *pQueue, const TCHAR *pszName);
void ShowThreadPool(CONSOLE_CTX console, ThreadPool *p);
void DumpProcess(CONSOLE_CTX console);

GRAPH_ACL_ENTRY *LoadGraphACL(DB_HANDLE hdb, UINT32 graphId, int *pnACLSize);
BOOL CheckGraphAccess(GRAPH_ACL_ENTRY *pACL, int nACLSize, UINT32 graphId, UINT32 graphUserId, UINT32 graphDesiredAccess);
UINT32 GetGraphAccessCheckResult(UINT32 graphId, UINT32 graphUserId);
GRAPH_ACL_AND_ID IsGraphNameExists(const TCHAR *graphName);

#if XMPP_SUPPORTED
bool SendXMPPMessage(const TCHAR *rcpt, const TCHAR *message);
#endif

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
   MONITORED_FILE* m_monitoredFile;

public:
   FileMonitoringList();
   ~FileMonitoringList();
   void addMonitoringFile(MONITORED_FILE *fileForAdd);
   bool checkDuplicate(MONITORED_FILE *fileForAdd);
   ObjectArray<ClientSession>* findClientByFNameAndNodeID(const TCHAR *fileName, UINT32 nodeID);
   bool removeMonitoringFile(MONITORED_FILE *fileForRemove);
   void removeDisconectedNode(UINT32 nodeId);

private:
   void lock();
   void unlock();
};

/**
 * Global variables
 */
extern TCHAR NXCORE_EXPORTABLE g_szConfigFile[];
extern TCHAR NXCORE_EXPORTABLE g_szLogFile[];
extern UINT32 g_dwLogRotationMode;
extern UINT32 g_dwMaxLogSize;
extern UINT32 g_dwLogHistorySize;
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
extern int g_requiredPolls;
extern UINT32 g_slmPollingInterval;

extern TCHAR g_szDbDriver[];
extern TCHAR g_szDbDrvParams[];
extern TCHAR g_szDbServer[];
extern TCHAR g_szDbLogin[];
extern TCHAR g_szDbPassword[];
extern TCHAR g_szDbName[];
extern TCHAR g_szDbSchema[];
extern DB_DRIVER g_dbDriver;
extern DB_HANDLE NXCORE_EXPORTABLE g_hCoreDB;
extern Queue *g_dbWriterQueue;
extern Queue *g_dciDataWriterQueue;
extern Queue *g_dciRawDataWriterQueue;

extern int NXCORE_EXPORTABLE g_dbSyntax;
extern FileMonitoringList g_monitoringList;

extern ThreadPool *g_mainThreadPool;

#endif   /* _nms_core_h_ */
