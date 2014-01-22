/*
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#define SHOW_FLAG_VALUE(x) _T("  %-32s = %d\n"), _T(#x), (g_dwFlags & x) ? 1 : 0


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
   CSCPMessage *pMsg;
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
#define CSF_EPP_LOCKED           ((UINT32)0x0002)
#define CSF_PACKAGE_DB_LOCKED    ((UINT32)0x0004)
#define CSF_USER_DB_LOCKED       ((UINT32)0x0008)
#define CSF_EPP_UPLOAD           ((UINT32)0x0010)
#define CSF_CONSOLE_OPEN         ((UINT32)0x0020)
#define CSF_AUTHENTICATED        ((UINT32)0x0080)
#define CSF_RECEIVING_MAP_DATA   ((UINT32)0x0200)
#define CSF_SYNC_OBJECT_COMMENTS ((UINT32)0x0400)

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
   UINT32 dwIpAddr;
   UINT32 dwNetMask;
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
   UINT32 dwIpAddr;
   UINT32 dwNetMask;
   UINT32 dwSubnetAddr;
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
   virtual void onTrap(CSCPMessage *msg);
   virtual void onDataPush(CSCPMessage *msg);
   virtual void onFileMonitoringData(CSCPMessage *msg);

public:
   AgentConnectionEx(UINT32 nodeId, UINT32 ipAddr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL) :
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
   UINT32 m_dwIndex;
   int m_iState;
   WORD m_wCurrentCmd;
   UINT32 m_dwUserId;
	UINT32 m_deviceObjectId;
   CSCP_BUFFER *m_pMsgBuffer;
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

   void setupEncryption(CSCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(UINT32 dwRqId);
   void login(CSCPMessage *pRequest);
   void updateDeviceInfo(CSCPMessage *pRequest);
   void updateDeviceStatus(CSCPMessage *pRequest);

public:
   MobileDeviceSession(SOCKET hSocket, struct sockaddr *addr);
   ~MobileDeviceSession();

   void incRefCount() { m_dwRefCount++; }
   void decRefCount() { if (m_dwRefCount > 0) m_dwRefCount--; }

   void run();

   void postMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void sendMessage(CSCPMessage *pMsg);

	UINT32 getIndex() { return m_dwIndex; }
   void setIndex(UINT32 dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }
   int getState() { return m_iState; }
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
#define DECLARE_THREAD_STARTER(func) static THREAD_RESULT THREAD_CALL ThreadStarter_##func(void *);

class NXCORE_EXPORTABLE ClientSession
{
private:
   SOCKET m_hSocket;
   Queue *m_pSendQueue;
   Queue *m_pMessageQueue;
   Queue *m_pUpdateQueue;
   UINT32 m_dwIndex;
   int m_iState;
   WORD m_wCurrentCmd;
   UINT32 m_dwUserId;
   UINT32 m_dwSystemAccess;    // User's system access rights
   UINT32 m_dwFlags;           // Session flags
	int m_clientType;				// Client system type - desktop, web, mobile, etc.
   CSCP_BUFFER *m_pMsgBuffer;
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

   static THREAD_RESULT THREAD_CALL readThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL writeThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL processingThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL updateThreadStarter(void *);
   static THREAD_RESULT THREAD_CALL pollerThreadStarter(void *);

	DECLARE_THREAD_STARTER(getCollectedData)
	DECLARE_THREAD_STARTER(getTableCollectedData)
	DECLARE_THREAD_STARTER(clearDCIData)
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
	DECLARE_THREAD_STARTER(getReportResults)
	DECLARE_THREAD_STARTER(deleteReportResults)
	DECLARE_THREAD_STARTER(renderReport)
	DECLARE_THREAD_STARTER(getNetworkPath)
	DECLARE_THREAD_STARTER(getAlarmEvents)
	DECLARE_THREAD_STARTER(forwardToReportingServer)

   void readThread();
   void writeThread();
   void processingThread();
   void updateThread();
   void pollerThread(Node *pNode, int iPollType, UINT32 dwRqId);

   void setupEncryption(CSCPMessage *request);
   void respondToKeepalive(UINT32 dwRqId);
   void onFileUpload(BOOL bSuccess);
   void debugPrintf(int level, const TCHAR *format, ...);
   void sendServerInfo(UINT32 dwRqId);
   void login(CSCPMessage *pRequest);
   void sendAllObjects(CSCPMessage *pRequest);
   void sendSelectedObjects(CSCPMessage *pRequest);
   void sendEventLog(CSCPMessage *pRequest);
   void sendAllConfigVars(UINT32 dwRqId);
   void setConfigVariable(CSCPMessage *pRequest);
   void deleteConfigVariable(CSCPMessage *pRequest);
   void sendUserDB(UINT32 dwRqId);
   void sendAllAlarms(UINT32 dwRqId);
   void createUser(CSCPMessage *pRequest);
   void updateUser(CSCPMessage *pRequest);
   void deleteUser(CSCPMessage *pRequest);
   void setPassword(CSCPMessage *pRequest);
   void lockUserDB(UINT32 dwRqId, BOOL bLock);
   void sendEventDB(UINT32 dwRqId);
   void modifyEventTemplate(CSCPMessage *pRequest);
   void deleteEventTemplate(CSCPMessage *pRequest);
   void generateEventCode(UINT32 dwRqId);
   void modifyObject(CSCPMessage *pRequest);
   void changeObjectMgmtStatus(CSCPMessage *pRequest);
   void openNodeDCIList(CSCPMessage *pRequest);
   void closeNodeDCIList(CSCPMessage *pRequest);
   void modifyNodeDCI(CSCPMessage *pRequest);
   void copyDCI(CSCPMessage *pRequest);
   void applyTemplate(CSCPMessage *pRequest);
   void getCollectedData(CSCPMessage *pRequest);
   void getTableCollectedData(CSCPMessage *pRequest);
	bool getCollectedDataFromDB(CSCPMessage *request, CSCPMessage *response, DataCollectionTarget *object, int dciType);
	void clearDCIData(CSCPMessage *pRequest);
   void changeDCIStatus(CSCPMessage *pRequest);
   void getLastValues(CSCPMessage *pRequest);
   void getTableLastValues(CSCPMessage *pRequest);
	void getThresholdSummary(CSCPMessage *request);
   void openEPP(CSCPMessage *request);
   void closeEPP(UINT32 dwRqId);
   void saveEPP(CSCPMessage *pRequest);
   void processEPPRecord(CSCPMessage *pRequest);
   void sendMIBTimestamp(UINT32 dwRqId);
   void sendMib(CSCPMessage *request);
   void createObject(CSCPMessage *request);
   void changeObjectBinding(CSCPMessage *request, BOOL bBind);
   void deleteObject(CSCPMessage *request);
   void getAlarm(CSCPMessage *request);
   void getAlarmEvents(CSCPMessage *request);
   void acknowledgeAlarm(CSCPMessage *request);
   void resolveAlarm(CSCPMessage *request, bool terminate);
   void deleteAlarm(CSCPMessage *request);
	void getAlarmNotes(CSCPMessage *pRequest);
	void updateAlarmNote(CSCPMessage *pRequest);
   void createAction(CSCPMessage *pRequest);
   void updateAction(CSCPMessage *pRequest);
   void deleteAction(CSCPMessage *pRequest);
   void sendAllActions(UINT32 dwRqId);
   void SendContainerCategories(UINT32 dwRqId);
   void forcedNodePoll(CSCPMessage *pRequest);
   void onTrap(CSCPMessage *pRequest);
   void onWakeUpNode(CSCPMessage *pRequest);
   void queryParameter(CSCPMessage *pRequest);
   void queryAgentTable(CSCPMessage *pRequest);
   void editTrap(int iOperation, CSCPMessage *pRequest);
   void LockTrapCfg(UINT32 dwRqId, BOOL bLock);
   void sendAllTraps(UINT32 dwRqId);
   void sendAllTraps2(UINT32 dwRqId);
   void LockPackageDB(UINT32 dwRqId, BOOL bLock);
   void SendAllPackages(UINT32 dwRqId);
   void InstallPackage(CSCPMessage *pRequest);
   void RemovePackage(CSCPMessage *pRequest);
   void DeployPackage(CSCPMessage *pRequest);
   void getParametersList(CSCPMessage *pRequest);
   void getUserVariable(CSCPMessage *pRequest);
   void setUserVariable(CSCPMessage *pRequest);
   void copyUserVariable(CSCPMessage *pRequest);
   void enumUserVariables(CSCPMessage *pRequest);
   void deleteUserVariable(CSCPMessage *pRequest);
   void changeObjectZone(CSCPMessage *pRequest);
   void getAgentConfig(CSCPMessage *pRequest);
   void updateAgentConfig(CSCPMessage *pRequest);
   void executeAction(CSCPMessage *pRequest);
   void sendObjectTools(UINT32 dwRqId);
   void sendObjectToolDetails(CSCPMessage *pRequest);
   void updateObjectTool(CSCPMessage *pRequest);
   void deleteObjectTool(CSCPMessage *pRequest);
   void generateObjectToolId(UINT32 dwRqId);
   void execTableTool(CSCPMessage *pRequest);
   void changeSubscription(CSCPMessage *pRequest);
   void sendSyslog(CSCPMessage *pRequest);
   void sendServerStats(UINT32 dwRqId);
   void sendScriptList(UINT32 dwRqId);
   void sendScript(CSCPMessage *pRequest);
   void updateScript(CSCPMessage *pRequest);
   void renameScript(CSCPMessage *pRequest);
   void deleteScript(CSCPMessage *pRequest);
   void SendSessionList(UINT32 dwRqId);
   void KillSession(CSCPMessage *pRequest);
   void SendTrapLog(CSCPMessage *pRequest);
   void StartSnmpWalk(CSCPMessage *pRequest);
   void resolveDCINames(CSCPMessage *pRequest);
   UINT32 resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR **ppszName);
   void SendConfigForAgent(CSCPMessage *pRequest);
   void sendAgentCfgList(UINT32 dwRqId);
   void OpenAgentConfig(CSCPMessage *pRequest);
   void SaveAgentConfig(CSCPMessage *pRequest);
   void DeleteAgentConfig(CSCPMessage *pRequest);
   void SwapAgentConfigs(CSCPMessage *pRequest);
   void SendObjectComments(CSCPMessage *pRequest);
   void updateObjectComments(CSCPMessage *pRequest);
   void pushDCIData(CSCPMessage *pRequest);
   void getAddrList(CSCPMessage *pRequest);
   void setAddrList(CSCPMessage *pRequest);
   void resetComponent(CSCPMessage *pRequest);
   void sendDCIEventList(CSCPMessage *request);
	void SendDCIInfo(CSCPMessage *pRequest);
   void sendPerfTabDCIList(CSCPMessage *pRequest);
   void exportConfiguration(CSCPMessage *pRequest);
   void importConfiguration(CSCPMessage *pRequest);
	void SendGraphList(UINT32 dwRqId);
	void SaveGraph(CSCPMessage *pRequest);
	void DeleteGraph(CSCPMessage *pRequest);
	void AddCACertificate(CSCPMessage *pRequest);
	void DeleteCertificate(CSCPMessage *pRequest);
	void UpdateCertificateComments(CSCPMessage *pRequest);
	void getCertificateList(UINT32 dwRqId);
	void queryL2Topology(CSCPMessage *pRequest);
	void sendSMS(CSCPMessage *pRequest);
	void SendCommunityList(UINT32 dwRqId);
	void UpdateCommunityList(CSCPMessage *pRequest);
	void getSituationList(UINT32 dwRqId);
	void createSituation(CSCPMessage *pRequest);
	void updateSituation(CSCPMessage *pRequest);
	void deleteSituation(CSCPMessage *pRequest);
	void deleteSituationInstance(CSCPMessage *pRequest);
	void setConfigCLOB(CSCPMessage *pRequest);
	void getConfigCLOB(CSCPMessage *pRequest);
	void registerAgent(CSCPMessage *pRequest);
	void getServerFile(CSCPMessage *pRequest);
	void cancelFileMonitoring(CSCPMessage *request);
	void getAgentFile(CSCPMessage *pRequest);
	void testDCITransformation(CSCPMessage *pRequest);
	void sendJobList(UINT32 dwRqId);
	void cancelJob(CSCPMessage *pRequest);
	void holdJob(CSCPMessage *pRequest);
	void unholdJob(CSCPMessage *pRequest);
	void deployAgentPolicy(CSCPMessage *pRequest, bool uninstallFlag);
	void getUserCustomAttribute(CSCPMessage *request);
	void setUserCustomAttribute(CSCPMessage *request);
	void openServerLog(CSCPMessage *request);
	void closeServerLog(CSCPMessage *request);
	void queryServerLog(CSCPMessage *request);
	void getServerLogQueryData(CSCPMessage *request);
	void sendUsmCredentials(UINT32 dwRqId);
	void updateUsmCredentials(CSCPMessage *pRequest);
	void sendDCIThresholds(CSCPMessage *request);
	void addClusterNode(CSCPMessage *request);
	void findNodeConnection(CSCPMessage *request);
	void findMacAddress(CSCPMessage *request);
	void findIpAddress(CSCPMessage *request);
	void sendLibraryImage(CSCPMessage *request);
	void updateLibraryImage(CSCPMessage *request);
	void listLibraryImages(CSCPMessage *request);
	void deleteLibraryImage(CSCPMessage *request);
	void executeServerCommand(CSCPMessage *request);
	void uploadFileToAgent(CSCPMessage *request);
	void listServerFileStore(CSCPMessage *request);
	void processConsoleCommand(CSCPMessage *msg);
	void openConsole(UINT32 rqId);
	void closeConsole(UINT32 rqId);
	void getVlans(CSCPMessage *msg);
	void receiveFile(CSCPMessage *request);
	void deleteFile(CSCPMessage *request);
	void executeReport(CSCPMessage *msg);
	void getReportResults(CSCPMessage *msg);
	void deleteReportResults(CSCPMessage *msg);
	void renderReport(CSCPMessage *request);
	void getNetworkPath(CSCPMessage *request);
	void getNodeComponents(CSCPMessage *request);
	void getNodeSoftware(CSCPMessage *request);
	void getWinPerfObjects(CSCPMessage *request);
	void listMappingTables(CSCPMessage *request);
	void getMappingTable(CSCPMessage *request);
	void updateMappingTable(CSCPMessage *request);
	void deleteMappingTable(CSCPMessage *request);
	void getWirelessStations(CSCPMessage *request);
   void getSummaryTables(UINT32 rqId);
   void getSummaryTableDetails(CSCPMessage *request);
   void modifySummaryTable(CSCPMessage *request);
   void deleteSummaryTable(CSCPMessage *request);
   void querySummaryTable(CSCPMessage *request);
   void forwardToReportingServer(CSCPMessage *request);
   void getSubnetAddressMap(CSCPMessage *request);

public:
   ClientSession(SOCKET hSocket, struct sockaddr *addr);
   ~ClientSession();

   void incRefCount() { m_dwRefCount++; }
   void decRefCount() { if (m_dwRefCount > 0) m_dwRefCount--; }

   void run();

   void postMessage(CSCPMessage *pMsg) { m_pSendQueue->Put(pMsg->CreateMessage()); }
   void sendMessage(CSCPMessage *pMsg);
   void sendRawMessage(CSCP_MESSAGE *pMsg);
   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszMsg);
	BOOL sendFile(const TCHAR *file, UINT32 dwRqId, long sizeLimit);

   UINT32 getIndex() { return m_dwIndex; }
   void setIndex(UINT32 dwIndex) { if (m_dwIndex == INVALID_INDEX) m_dwIndex = dwIndex; }
   int getState() { return m_iState; }
   const TCHAR *getUserName() { return m_szUserName; }
   const TCHAR *getClientInfo() { return m_szClientInfo; }
	const TCHAR *getWorkstation() { return m_workstation; }
   const TCHAR *getWebServerAddress() { return m_webServerAddress; }
   UINT32 getUserId() { return m_dwUserId; }
	UINT32 getSystemRights() { return m_dwSystemAccess; }
   bool isAuthenticated() { return (m_dwFlags & CSF_AUTHENTICATED) ? true : false; }
   bool isConsoleOpen() { return (m_dwFlags & CSF_CONSOLE_OPEN) ? true : false; }
   bool isSubscribed(UINT32 dwChannel) { return (m_dwActiveChannels & dwChannel) ? true : false; }
   WORD getCurrentCmd() { return m_wCurrentCmd; }
   int getCipher() { return (m_pCtx == NULL) ? -1 : m_pCtx->getCipher(); }
	int getClientType() { return m_clientType; }
   time_t getLoginTime() { return m_loginTime; }

	bool checkSysAccessRights(UINT32 requiredAccess)
   {
      return (m_dwUserId == 0) ? true :
         ((requiredAccess & m_dwSystemAccess) == requiredAccess);
   }

   void kill();
   void notify(UINT32 dwCode, UINT32 dwData = 0);

	void queueUpdate(UPDATE_INFO *pUpdate) { m_pUpdateQueue->Put(pUpdate); }
   void onNewEvent(Event *pEvent);
   void onSyslogMessage(NX_SYSLOG_RECORD *pRec);
   void onNewSNMPTrap(CSCPMessage *pMsg);
   void onObjectChange(NetObj *pObject);
   void onUserDBUpdate(int code, UINT32 id, UserDatabaseObject *user);
   void onAlarmUpdate(UINT32 dwCode, NXC_ALARM *pAlarm);
   void onActionDBUpdate(UINT32 dwCode, NXC_ACTION *pAction);
   void onSituationChange(CSCPMessage *msg);
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
BOOL NXCORE_EXPORTABLE ConfigReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);
#ifdef UNICODE
BOOL NXCORE_EXPORTABLE ConfigReadStrA(const WCHAR *szVar, char *szBuffer, int iBufSize, const char *szDefault);
#else
#define ConfigReadStrA ConfigReadStr
#endif
int NXCORE_EXPORTABLE ConfigReadInt(const TCHAR *szVar, int iDefault);
UINT32 NXCORE_EXPORTABLE ConfigReadULong(const TCHAR *szVar, UINT32 dwDefault);
BOOL NXCORE_EXPORTABLE ConfigReadByteArray(const TCHAR *pszVar, int *pnArray,
                                           int nSize, int nDefault);
BOOL NXCORE_EXPORTABLE ConfigWriteStr(const TCHAR *szVar, const TCHAR *szValue, BOOL bCreate,
												  BOOL isVisible = TRUE, BOOL needRestart = FALSE);
BOOL NXCORE_EXPORTABLE ConfigWriteInt(const TCHAR *szVar, int iValue, BOOL bCreate,
												  BOOL isVisible = TRUE, BOOL needRestart = FALSE);
BOOL NXCORE_EXPORTABLE ConfigWriteULong(const TCHAR *szVar, UINT32 dwValue, BOOL bCreate,
													 BOOL isVisible = TRUE, BOOL needRestart = FALSE);
BOOL NXCORE_EXPORTABLE ConfigWriteByteArray(const TCHAR *pszVar, int *pnArray,
                                            int nSize, BOOL bCreate,
														  BOOL isVisible = TRUE, BOOL needRestart = FALSE);
TCHAR NXCORE_EXPORTABLE *ConfigReadCLOB(const TCHAR *var, const TCHAR *defValue);
BOOL NXCORE_EXPORTABLE ConfigWriteCLOB(const TCHAR *var, const TCHAR *value, BOOL bCreate);

BOOL NXCORE_EXPORTABLE MetaDataReadStr(const TCHAR *szVar, TCHAR *szBuffer, int iBufSize, const TCHAR *szDefault);

BOOL NXCORE_EXPORTABLE LoadConfig();

void NXCORE_EXPORTABLE Shutdown();
void NXCORE_EXPORTABLE FastShutdown();
BOOL NXCORE_EXPORTABLE Initialize();
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *);
void NXCORE_EXPORTABLE ShutdownDB();
void InitiateShutdown();

BOOL NXCORE_EXPORTABLE SleepAndCheckForShutdown(int iSeconds);

void ConsolePrintf(CONSOLE_CTX pCtx, const TCHAR *pszFormat, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
int ProcessConsoleCommand(const TCHAR *pszCmdLine, CONSOLE_CTX pCtx);

void SaveObjects(DB_HANDLE hdb);
void NXCORE_EXPORTABLE ObjectTransactionStart();
void NXCORE_EXPORTABLE ObjectTransactionEnd();

void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query);
void NXCORE_EXPORTABLE QueueSQLRequest(const TCHAR *query, int bindCount, int *sqlTypes, const TCHAR **values);
void QueueIDataInsert(time_t timestamp, UINT32 nodeId, UINT32 dciId, const TCHAR *value);
void StartDBWriter();
void StopDBWriter();

bool NXCORE_EXPORTABLE IsDatabaseRecordExist(DB_HANDLE hdb, const TCHAR *table, const TCHAR *idColumn, UINT32 id);

void DecodeSQLStringAndSetVariable(CSCPMessage *pMsg, UINT32 dwVarId, TCHAR *pszStr);

SNMP_SecurityContext *SnmpCheckCommSettings(SNMP_Transport *pTransport, int *version, SNMP_SecurityContext *originalContext, StringList *customTestOids);
void StrToMac(const TCHAR *pszStr, BYTE *pBuffer);

void InitLocalNetInfo();

ARP_CACHE *GetLocalArpCache();
ARP_CACHE *SnmpGetArpCache(UINT32 dwVersion, SNMP_Transport *pTransport);

InterfaceList *GetLocalInterfaceList();
void SnmpGetInterfaceStatus(UINT32 dwVersion, SNMP_Transport *pTransport, UINT32 dwIfIndex, int *adminState, int *operState);

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
Node *PollNewNode(UINT32 dwIpAddr, UINT32 dwNetMask, UINT32 dwCreationFlags, WORD agentPort,
                  WORD snmpPort, const TCHAR *pszName, UINT32 dwProxyNode, UINT32 dwSNMPProxy, Cluster *pCluster,
						UINT32 zoneId, bool doConfPoll, bool discoveredNode);

void NXCORE_EXPORTABLE EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg);
void NXCORE_EXPORTABLE NotifyClientSessions(UINT32 dwCode, UINT32 dwData);
int GetSessionCount(bool withRoot = true);
bool IsLoggedIn(UINT32 dwUserId);

void GetSysInfoStr(TCHAR *pszBuffer, int nMaxSize);
UINT32 GetLocalIpAddr();
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
void CreateTrapCfgMessage(CSCPMessage &msg);
UINT32 CreateNewTrap(UINT32 *pdwTrapId);
UINT32 CreateNewTrap(NXC_TRAP_CFG_ENTRY *pTrap);
UINT32 UpdateTrapFromMsg(CSCPMessage *pMsg);
UINT32 DeleteTrap(UINT32 dwId);
void CreateNXMPTrapRecord(String &str, UINT32 dwId);

BOOL IsTableTool(UINT32 dwToolId);
BOOL CheckObjectToolAccess(UINT32 dwToolId, UINT32 dwUserId);
UINT32 ExecuteTableTool(UINT32 dwToolId, Node *pNode, UINT32 dwRqId, ClientSession *pSession);
UINT32 DeleteObjectToolFromDB(UINT32 dwToolId);
UINT32 UpdateObjectToolFromMessage(CSCPMessage *pMsg);

UINT32 ModifySummaryTable(CSCPMessage *msg, LONG *newId);
UINT32 DeleteSummaryTable(LONG tableId);
Table *QuerySummaryTable(LONG tableId, UINT32 baseObjectId, UINT32 userId, UINT32 *rcc);

void CreateMessageFromSyslogMsg(CSCPMessage *pMsg, NX_SYSLOG_RECORD *pRec);
void ReinitializeSyslogParser();

void EscapeString(String &str);

void InitAuditLog();
void NXCORE_EXPORTABLE WriteAuditLog(const TCHAR *subsys, BOOL isSuccess, UINT32 userId,
                                     const TCHAR *workstation, UINT32 objectId,
                                     const TCHAR *format, ...);

bool ValidateConfig(Config *config, UINT32 flags, TCHAR *errorText, int errorTextLen);
UINT32 ImportConfig(Config *config, UINT32 flags);

#ifdef _WITH_ENCRYPTION
X509 *CertificateFromLoginMessage(CSCPMessage *pMsg);
BOOL ValidateUserCertificate(X509 *pCert, const TCHAR *pszLogin, BYTE *pChallenge,
									  BYTE *pSignature, UINT32 dwSigLen, int nMappingMethod,
									  const TCHAR *pszMappingData);
void ReloadCertificates();
#endif

#ifndef _WIN32
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *);
#endif   /* not _WIN32 */

void DbgTestRWLock(RWLOCK hLock, const TCHAR *szName, CONSOLE_CTX pCtx);
void DumpClientSessions(CONSOLE_CTX pCtx);
void DumpMobileDeviceSessions(CONSOLE_CTX pCtx);
void ShowPollerState(CONSOLE_CTX pCtx);
void SetPollerInfo(int nIdx, const TCHAR *pszMsg);
void ShowServerStats(CONSOLE_CTX pCtx);
void ShowQueueStats(CONSOLE_CTX pCtx, Queue *pQueue, const TCHAR *pszName);
void DumpProcess(CONSOLE_CTX pCtx);

GRAPH_ACL_ENTRY *LoadGraphACL(UINT32 graphId, int *pnACLSize);
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
extern TCHAR g_szDataDir[];
extern TCHAR g_szLibDir[];
extern TCHAR g_szJavaLibDir[];
extern UINT32 NXCORE_EXPORTABLE g_processAffinityMask;
extern QWORD g_qwServerId;
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
extern TCHAR NXCORE_EXPORTABLE g_szJavaPath[];
extern DB_DRIVER g_dbDriver;
extern DB_HANDLE NXCORE_EXPORTABLE g_hCoreDB;
extern Queue *g_pLazyRequestQueue;
extern Queue *g_pIDataInsertQueue;

extern int NXCORE_EXPORTABLE g_nDBSyntax;
extern FileMonitoringList g_monitoringList;

#endif   /* _nms_core_h_ */
