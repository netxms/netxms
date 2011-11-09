/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxsrvapi.h
**
**/

#ifndef _nxsrvapi_h_
#define _nxsrvapi_h_

#ifdef _WIN32
#ifdef LIBNXSRV_EXPORTS
#define LIBNXSRV_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSRV_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSRV_EXPORTABLE
#endif

#ifndef LIBNXCL_NO_DECLARATIONS
#define LIBNXCL_NO_DECLARATIONS 1
#endif
#include <nxclapi.h>
#include <nxcpapi.h>
#include <nms_agent.h>
#include "../libnxsrv/messages.h"
#include <nxsnmp.h>
#include <netxms_isc.h>


//
// Default files
//

#ifdef _WIN32

#define DEFAULT_CONFIG_FILE   _T("C:\\netxmsd.conf")

#define DEFAULT_SHELL         _T("cmd.exe")
#define DEFAULT_LOG_FILE      _T("C:\\NetXMS.log")
#define DEFAULT_DATA_DIR      _T("C:\\NetXMS\\var")
#define DEFAULT_LIBDIR        _T("C:\\NetXMS\\lib")
#define DEFAULT_JAVA_LIBDIR   _T("C:\\NetXMS\\lib\\java")
#define DEFAULT_DUMP_DIR      _T("C:\\")

#define LDIR_NDD              _T("\\ndd")

#define DDIR_MIBS             _T("\\mibs")
#define DDIR_PACKAGES         _T("\\packages")
#define DDIR_BACKGROUNDS      _T("\\backgrounds")
#define DDIR_SHARED_FILES     _T("\\shared")
#define DFILE_KEYS            _T("\\server_key")
#define DFILE_COMPILED_MIB    _T("\\mibs\\netxms.mib")
#define DDIR_IMAGES           _T("\\images")
#define DDIR_FILES            _T("\\files")
#define DDIR_REPORTS          _T("\\reports")

#else    /* _WIN32 */

#define DEFAULT_CONFIG_FILE   _T("{search}")

#define DEFAULT_SHELL         _T("/bin/sh")

#ifndef DATADIR
#define DATADIR              _T("/var/netxms")
#endif

#ifndef LIBDIR
#define LIBDIR               _T("/usr/lib")
#endif

#ifndef PKGLIBDIR
#define PKGLIBDIR            _T("/usr/lib/netxms")
#endif

#define DEFAULT_LOG_FILE      DATADIR _T("/log/netxmsd.log")
#define DEFAULT_DATA_DIR      DATADIR
#define DEFAULT_LIBDIR        PKGLIBDIR
#define DEFAULT_JAVA_LIBDIR   PKGLIBDIR _T("/java")
#define DEFAULT_DUMP_DIR      _T("/")

#define LDIR_NDD              _T("/ndd")

#define DDIR_MIBS             _T("/mibs")
#define DDIR_PACKAGES         _T("/packages")
#define DDIR_BACKGROUNDS      _T("/backgrounds")
#define DDIR_SHARED_FILES     _T("/shared")
#define DFILE_KEYS            _T("/.server_key")
#define DFILE_COMPILED_MIB    _T("/mibs/netxms.mib")
#define DDIR_IMAGES           _T("/images")
#define DDIR_FILES            _T("/files")
#define DDIR_REPORTS          _T("/reports")

#endif   /* _WIN32 */


//
// Application flags
//

#define AF_DAEMON                         0x00000001
#define AF_USE_SYSLOG                     0x00000002
#define AF_ENABLE_NETWORK_DISCOVERY       0x00000004
#define AF_ACTIVE_NETWORK_DISCOVERY       0x00000008
#define AF_LOG_SQL_ERRORS                 0x00000010
#define AF_DELETE_EMPTY_SUBNETS           0x00000020
#define AF_ENABLE_SNMP_TRAPD              0x00000040
#define AF_ENABLE_ZONING                  0x00000080
#define AF_SYNC_NODE_NAMES_WITH_DNS       0x00000100
#define AF_CHECK_TRUSTED_NODES            0x00000200
#define AF_WRITE_FULL_DUMP                0x00080000
#define AF_RESOLVE_NODE_NAMES             0x00100000
#define AF_CATCH_EXCEPTIONS               0x00200000
#define AF_INTERNAL_CA                    0x00400000
#define AF_DB_LOCKED                      0x01000000
#define AF_ENABLE_MULTIPLE_DB_CONN        0x02000000
#define AF_DB_CONNECTION_LOST             0x04000000
#define AF_NO_NETWORK_CONNECTIVITY        0x08000000
#define AF_EVENT_STORM_DETECTED           0x10000000
#define AF_SERVER_INITIALIZED             0x40000000
#define AF_SHUTDOWN                       0x80000000


//
// Encryption usage policies
//

#define ENCRYPTION_DISABLED   0
#define ENCRYPTION_ALLOWED    1
#define ENCRYPTION_PREFERRED  2
#define ENCRYPTION_REQUIRED   3


//
// Flags for SnmpGet
//

#define SG_VERBOSE        0x0001
#define SG_STRING_RESULT  0x0002
#define SG_RAW_RESULT     0x0004
#define SG_HSTRING_RESULT 0x0008
#define SG_PSTRING_RESULT 0x0010


//
// Win32 service and syslog constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME     _T("NetXMSCore")
#define CORE_EVENT_SOURCE     _T("NetXMSCore")
#define NETXMSD_SYSLOG_NAME   CORE_EVENT_SOURCE

#else

#define NETXMSD_SYSLOG_NAME   _T("netxmsd")

#endif   /* _WIN32 */


//
// Single ARP cache entry
//

typedef struct
{
   DWORD dwIndex;       // Interface index
   DWORD dwIpAddr;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
} ARP_ENTRY;


//
// ARP cache structure used by discovery functions and AgentConnection class
//

typedef struct
{
   DWORD dwNumEntries;
   ARP_ENTRY *pEntries;
} ARP_CACHE;


//
// Interface information structure used by discovery functions and AgentConnection class
//

typedef struct
{
   TCHAR szName[MAX_DB_STRING];			// Interface display name
	TCHAR szDescription[MAX_DB_STRING];	// Value of ifDescr MIB variable for SNMP agents
   DWORD dwIndex;
   DWORD dwType;
	DWORD dwBridgePortNumber;
	DWORD dwSlotNumber;
	DWORD dwPortNumber;
   DWORD dwIpAddr;
   DWORD dwIpNetMask;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
   int iNumSecondary;      // Number of secondary IP's on this interface
	bool isPhysicalPort;
} NX_INTERFACE_INFO;


//
// Interface list used by discovery functions and AgentConnection class
//

class LIBNXSRV_EXPORTABLE InterfaceList
{
private:
   int m_size;       				 // Number of valid entries
	int m_allocated;               // Number of allocated entries
   void *m_data;                  // Can be used by custom enumeration handlers
   NX_INTERFACE_INFO *m_interfaces;  // Interface entries

public:
	InterfaceList(int initialAlloc = 8);
	~InterfaceList();

	void add(NX_INTERFACE_INFO *iface);
	void remove(int index);
	void removeLoopbacks();

	int getSize() { return m_size; }
	NX_INTERFACE_INFO *get(int index) { return ((index >= 0) && (index < m_size)) ? &m_interfaces[index] : NULL; }
	NX_INTERFACE_INFO *findByIfIndex(DWORD ifIndex);

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }
};


//
// Vlan information
//

#define VLAN_PRM_IFINDEX   0
#define VLAN_PRM_SLOTPORT  1

class LIBNXSRV_EXPORTABLE VlanInfo
{
private:
	int m_vlanId;
	TCHAR *m_name;
	int m_portRefMode;	// Port reference mode - by ifIndex or by slot/port
	int m_allocated;
	int m_numPorts;	// Number of ports in VLAN
	DWORD *m_ports;	// member ports (slot/port pairs or ifIndex)
	DWORD *m_indexes;	// ifIndexes for ports
	DWORD *m_ids;		// Interface object IDs for ports

public:
	VlanInfo(int vlanId, int prm);
	~VlanInfo();

	int getVlanId() { return m_vlanId; }
	int getPortReferenceMode() { return m_portRefMode; }
	const TCHAR *getName() { return CHECK_NULL_EX(m_name); }
	int getNumPorts() { return m_numPorts; }
	DWORD *getPorts() { return m_ports; }
	DWORD *getIfIndexes() { return m_indexes; }
	DWORD *getIfIds() { return m_ids; }

	void add(DWORD slot, DWORD port);
	void add(DWORD ifIndex);
	void setName(const TCHAR *name);

	void prepareForResolve();
	void resolvePort(int index, DWORD sp, DWORD ifIndex, DWORD id);
};


//
// Vlan list
//

class LIBNXSRV_EXPORTABLE VlanList : public RefCountObject
{
private:
   int m_size;          // Number of valid entries
	int m_allocated;     // Number of allocated entries
   void *m_data;        // Can be used by custom enumeration handlers
   VlanInfo **m_vlans;  // VLAN entries

public:
	VlanList(int initialAlloc = 8);
	virtual ~VlanList();

	void add(VlanInfo *vlan);
	void addMemberPort(int vlanId, DWORD portId);

	int getSize() { return m_size; }
	VlanInfo *get(int index) { return ((index >= 0) && (index < m_size)) ? m_vlans[index] : NULL; }
	VlanInfo *findById(int id);

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }

	void fillMessage(CSCPMessage *msg);
};


//
// Route information
//

typedef struct
{
   DWORD dwDestAddr;
   DWORD dwDestMask;
   DWORD dwNextHop;
   DWORD dwIfIndex;
   DWORD dwRouteType;
} ROUTE;


//
// Routing table
//

typedef struct
{
   int iNumEntries;     // Number of entries
   ROUTE *pRoutes;      // Route list
} ROUTING_TABLE;


//
// Information about policies installed on agent
//

class LIBNXSRV_EXPORTABLE AgentPolicyInfo
{
private:
	int m_size;
	BYTE *m_guidList;
	int *m_typeList;
	TCHAR **m_serverList;

public:
	AgentPolicyInfo(CSCPMessage *msg);
	~AgentPolicyInfo();

	int getSize() { return m_size; }
	bool getGuid(int index, uuid_t guid);
	int getType(int index) { return ((index >= 0) && (index < m_size)) ? m_typeList[index] : -1; }
	const TCHAR *getServer(int index) { return ((index >= 0) && (index < m_size)) ? m_serverList[index] : NULL; }
};


//
// Agent connection
//

class LIBNXSRV_EXPORTABLE AgentConnection
{
private:
   DWORD m_dwAddr;
   int m_nProtocolVersion;
   int m_iAuthMethod;
   char m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   SOCKET m_hSocket;
   DWORD m_dwNumDataLines;
   DWORD m_dwRequestId;
   DWORD m_dwCommandTimeout;
	DWORD m_connectionTimeout;
   DWORD m_dwRecvTimeout;
   TCHAR **m_ppDataLines;
   MsgWaitQueue *m_pMsgWaitQueue;
   BOOL m_bIsConnected;
   MUTEX m_mutexDataLock;
	MUTEX m_mutexSocketWrite;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_pCtx;
   int m_iEncryptionPolicy;
   BOOL m_bUseProxy;
   DWORD m_dwProxyAddr;
   WORD m_wPort;
   WORD m_wProxyPort;
   int m_iProxyAuth;
   char m_szProxySecret[MAX_SECRET_LENGTH];
	int m_hCurrFile;
	TCHAR m_currentFileName[MAX_PATH];
	DWORD m_dwDownloadRequestId;
	CONDITION m_condFileDownload;
	BOOL m_fileDownloadSucceeded;
	void (*m_downloadProgressCallback)(size_t, void *);
	void *m_downloadProgressCallbackArg;
	bool m_deleteFileOnDownloadFailure;
	bool m_fileUploadInProgress;

   void ReceiverThread();
   static THREAD_RESULT THREAD_CALL ReceiverThreadStarter(void *);

protected:
   void destroyResultData();
   BOOL sendMessage(CSCPMessage *pMsg);
   CSCPMessage *waitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut) { return m_pMsgWaitQueue->WaitForMessage(wCode, dwId, dwTimeOut); }
   DWORD waitForRCC(DWORD dwRqId, DWORD dwTimeOut);
   DWORD setupEncryption(RSA *pServerKey);
   DWORD authenticate(BOOL bProxyData);
   DWORD setupProxyConnection();
   DWORD getIpAddr() { return ntohl(m_dwAddr); }
	DWORD prepareFileDownload(const TCHAR *fileName, DWORD rqId, bool append, void (*downloadProgressCallback)(size_t, void *), void *cbArg);

   virtual void PrintMsg(const TCHAR *pszFormat, ...);
   virtual void onTrap(CSCPMessage *pMsg);
	virtual void onDataPush(CSCPMessage *msg);
	virtual bool processCustomMessage(CSCPMessage *pMsg);
	virtual void onFileDownload(BOOL success);

   void Lock() { MutexLock(m_mutexDataLock, INFINITE); }
   void Unlock() { MutexUnlock(m_mutexDataLock); }
	NXCPEncryptionContext *acquireEncryptionContext();

public:
   AgentConnection(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT,
                   int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   virtual ~AgentConnection();

   BOOL connect(RSA *pServerKey = NULL, BOOL bVerbose = FALSE, DWORD *pdwError = NULL, DWORD *pdwSocketError = NULL);
   void disconnect();
   BOOL isConnected() { return m_bIsConnected; }
	int getProtocolVersion() { return m_nProtocolVersion; }

	SOCKET getSocket() { return m_hSocket; }

   ARP_CACHE *getArpCache();
   InterfaceList *getInterfaceList();
   ROUTING_TABLE *getRoutingTable();
   DWORD getParameter(const TCHAR *pszParam, DWORD dwBufSize, TCHAR *pszBuffer);
   DWORD getList(const TCHAR *pszParam);
   DWORD getTable(const TCHAR *pszParam, Table **table);
   DWORD nop();
   DWORD execAction(const TCHAR *pszAction, int argc, TCHAR **argv);
   DWORD uploadFile(const TCHAR *localFile, const TCHAR *destinationFile = NULL, void (* progressCallback)(INT64, void *) = NULL, void *cbArg = NULL);
   DWORD startUpgrade(const TCHAR *pszPkgName);
   DWORD checkNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0, 
                             WORD wProto = 0, const TCHAR *pszRequest = NULL, const TCHAR *pszResponse = NULL);
   DWORD getSupportedParameters(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList);
   DWORD getConfigFile(TCHAR **ppszConfig, DWORD *pdwSize);
   DWORD updateConfigFile(const TCHAR *pszConfig);
   DWORD enableTraps();
	DWORD getPolicyInventory(AgentPolicyInfo **info);
	DWORD uninstallPolicy(uuid_t guid);

	DWORD generateRequestId() { return m_dwRequestId++; }
	CSCPMessage *customRequest(CSCPMessage *pRequest, const TCHAR *recvFile = NULL, bool appendFile = false,
	                           void (*downloadProgressCallback)(size_t, void *) = NULL, void *cbArg = NULL);

   DWORD getNumDataLines() { return m_dwNumDataLines; }
   const TCHAR *getDataLine(DWORD dwIndex) { return dwIndex < m_dwNumDataLines ? m_ppDataLines[dwIndex] : _T("(error)"); }

   void setConnectionTimeout(DWORD dwTimeout) { m_connectionTimeout = max(dwTimeout, 1000); }
	DWORD getConnectionTimeout() { return m_connectionTimeout; }
   void setCommandTimeout(DWORD dwTimeout) { m_dwCommandTimeout = max(dwTimeout, 500); }
	DWORD getCommandTimeout() { return m_dwCommandTimeout; }
   void setRecvTimeout(DWORD dwTimeout) { m_dwRecvTimeout = max(dwTimeout, 10000); }
   void setEncryptionPolicy(int iPolicy) { m_iEncryptionPolicy = iPolicy; }
   void setProxy(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT,
                 int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   void setPort(WORD wPort) { m_wPort = wPort; }
   void setAuthData(int nMethod, const char *pszSecret) { m_iAuthMethod = nMethod; strncpy(m_szSecret, pszSecret, MAX_SECRET_LENGTH); m_szSecret[MAX_SECRET_LENGTH - 1] = 0; }
	void setDeleteFileOnDownloadFailure(bool flag) { m_deleteFileOnDownloadFailure = flag; }
};


//
// Proxy SNMP transport
//

class LIBNXSRV_EXPORTABLE SNMP_ProxyTransport : public SNMP_Transport
{
protected:
	AgentConnection *m_pAgentConnection;
	CSCPMessage *m_pResponse;
	DWORD m_dwIpAddr;
	WORD m_wPort;

public:
	SNMP_ProxyTransport(AgentConnection *pConn, DWORD dwIpAddr, WORD wPort);
	virtual ~SNMP_ProxyTransport();

   virtual int readMessage(SNMP_PDU **ppData, DWORD dwTimeout = INFINITE,
                           struct sockaddr *pSender = NULL, socklen_t *piAddrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL);
   virtual int sendMessage(SNMP_PDU *pdu);
};


//
// ISC flags
//

#define ISCF_IS_CONNECTED        ((DWORD)0x00000001)
#define ISCF_REQUIRE_ENCRYPTION  ((DWORD)0x00000002)


//
// Inter-server connection (ISC)
//

class LIBNXSRV_EXPORTABLE ISC
{
private:
	DWORD m_flags;
   DWORD m_addr;
	WORD m_port;
   SOCKET m_socket;
   int m_protocolVersion;
	DWORD m_requestId;
	DWORD m_recvTimeout;
   MsgWaitQueue *m_msgWaitQueue;
   MUTEX m_mutexDataLock;
	MUTEX m_socketLock;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_ctx;
	DWORD m_commandTimeout;

   void ReceiverThread(void);
   static THREAD_RESULT THREAD_CALL ReceiverThreadStarter(void *);

protected:
   void DestroyResultData(void);
   DWORD SetupEncryption(RSA *pServerKey);
	DWORD ConnectToService(DWORD service);

   void Lock(void) { MutexLock(m_mutexDataLock, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutexDataLock); }

   virtual void PrintMsg(const TCHAR *format, ...);

public:
   ISC();
   ISC(DWORD addr, WORD port = NETXMS_ISC_PORT);
   virtual ~ISC();

   DWORD Connect(DWORD service, RSA *serverKey = NULL, BOOL requireEncryption = FALSE);
	void Disconnect();

   BOOL SendMessage(CSCPMessage *msg);
   CSCPMessage *WaitForMessage(WORD code, DWORD id, DWORD timeOut) { return m_msgWaitQueue->WaitForMessage(code, id, timeOut); }
   DWORD WaitForRCC(DWORD rqId, DWORD timeOut);

   DWORD Nop(void);
};


//
// Functions
//

void LIBNXSRV_EXPORTABLE DestroyArpCache(ARP_CACHE *pArpCache);
void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT);
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT);
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(int iError);

void LIBNXSRV_EXPORTABLE WriteLogOther(WORD wType, const TCHAR *format, ...);
void LIBNXSRV_EXPORTABLE DbgPrintf(int level, const TCHAR *format, ...);
void LIBNXSRV_EXPORTABLE DbgPrintf2(int level, const TCHAR *format, va_list args);

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy);

const TCHAR LIBNXSRV_EXPORTABLE *ISCErrorCodeToText(DWORD code);

DWORD LIBNXSRV_EXPORTABLE SnmpNewRequestId();
DWORD LIBNXSRV_EXPORTABLE SnmpGet(DWORD dwVersion, SNMP_Transport *pTransport,
                                  const TCHAR *szOidStr, const DWORD *oidBinary, DWORD dwOidLen, void *pValue,
                                  DWORD dwBufferSize, DWORD dwFlags);
DWORD LIBNXSRV_EXPORTABLE SnmpEnumerate(DWORD dwVersion, SNMP_Transport *pTransport, const TCHAR *szRootOid,
						                      DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *),
                                        void *pUserArg, BOOL bVerbose);


//
// Variables
//

extern DWORD LIBNXSRV_EXPORTABLE g_dwFlags;
extern DWORD LIBNXSRV_EXPORTABLE g_dwSNMPTimeout;
extern int LIBNXSRV_EXPORTABLE g_nDebugLevel;


//
// Helper finctions for checking server flags
//

inline bool IsStandalone()
{
	return !(g_dwFlags & AF_DAEMON) ? true : false;
}

inline bool IsZoningEnabled()
{
	return (g_dwFlags & AF_ENABLE_ZONING) ? true : false;
}

inline bool IsShutdownInProgress()
{
	return (g_dwFlags & AF_SHUTDOWN) ? true : false;
}


#endif   /* _nxsrvapi_h_ */
