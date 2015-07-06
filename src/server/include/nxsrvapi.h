/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
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

#include <nxcpapi.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxsnmp.h>
#include <netxms_isc.h>
#include <nxcldefs.h>

#ifdef INCLUDE_LIBNXSRV_MESSAGES
#include "../libnxsrv/messages.h"
#endif

/**
 * Default files
 */
#ifdef _WIN32

#define DEFAULT_CONFIG_FILE   _T("C:\\netxmsd.conf")

#define DEFAULT_SHELL         _T("cmd.exe")
#define DEFAULT_LOG_FILE      _T("C:\\netxmsd.log")
#define DEFAULT_DATA_DIR      _T("C:\\NetXMS\\var")
#define DEFAULT_LIBDIR        _T("C:\\NetXMS\\lib")
#define DEFAULT_DUMP_DIR      _T("C:\\NetXMS\\dump")

#define LDIR_NDD              _T("\\ndd")
#define LDIR_PDSDRV           _T("\\pdsdrv")

#define DDIR_MIBS             _T("\\mibs")
#define DDIR_PACKAGES         _T("\\packages")
#define DDIR_BACKGROUNDS      _T("\\backgrounds")
#define DDIR_SHARED_FILES     _T("\\shared")
#define DFILE_KEYS            _T("\\server_key")
#define DFILE_COMPILED_MIB    _T("\\mibs\\netxms.mib")
#define DDIR_IMAGES           _T("\\images")
#define DDIR_FILES            _T("\\files")

#else    /* _WIN32 */

#define DEFAULT_CONFIG_FILE   _T("{search}")

#define DEFAULT_SHELL         _T("/bin/sh")

#ifndef DATADIR
#define DATADIR              _T("/var/lib/netxms")
#endif

#ifndef LIBDIR
#define LIBDIR               _T("/usr/lib")
#endif

#ifndef PKGLIBDIR
#define PKGLIBDIR            _T("/usr/lib/netxms")
#endif

#define DEFAULT_LOG_FILE      _T("/var/log/netxmsd.log")
#define DEFAULT_DATA_DIR      DATADIR
#define DEFAULT_LIBDIR        PKGLIBDIR
#define DEFAULT_DUMP_DIR      _T("/var/tmp")

#define LDIR_NDD              _T("/ndd")
#define LDIR_PDSDRV           _T("/pdsdrv")

#define DDIR_MIBS             _T("/mibs")
#define DDIR_PACKAGES         _T("/packages")
#define DDIR_BACKGROUNDS      _T("/backgrounds")
#define DDIR_SHARED_FILES     _T("/shared")
#define DFILE_KEYS            _T("/.server_key")
#define DFILE_COMPILED_MIB    _T("/mibs/netxms.mib")
#define DDIR_IMAGES           _T("/images")
#define DDIR_FILES            _T("/files")

#endif   /* _WIN32 */

/**
 * Application flags
 */
#define AF_DAEMON                              _ULL(0x0000000000000001)
#define AF_USE_SYSLOG                          _ULL(0x0000000000000002)
#define AF_ENABLE_NETWORK_DISCOVERY            _ULL(0x0000000000000004)
#define AF_ACTIVE_NETWORK_DISCOVERY            _ULL(0x0000000000000008)
#define AF_LOG_SQL_ERRORS                      _ULL(0x0000000000000010)
#define AF_DELETE_EMPTY_SUBNETS                _ULL(0x0000000000000020)
#define AF_ENABLE_SNMP_TRAPD                   _ULL(0x0000000000000040)
#define AF_ENABLE_ZONING                       _ULL(0x0000000000000080)
#define AF_SYNC_NODE_NAMES_WITH_DNS            _ULL(0x0000000000000100)
#define AF_CHECK_TRUSTED_NODES                 _ULL(0x0000000000000200)
#define AF_ENABLE_NXSL_CONTAINER_FUNCS         _ULL(0x0000000000000400)
#define AF_USE_FQDN_FOR_NODE_NAMES             _ULL(0x0000000000000800)
#define AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE _ULL(0x0000000000001000)
#define AF_DEBUG_CONSOLE_DISABLED              _ULL(0x0000000000002000)
#define AF_ENABLE_OBJECT_TRANSACTIONS          _ULL(0x0000000000004000)
#define AF_WRITE_FULL_DUMP                     _ULL(0x0000000000080000)
#define AF_RESOLVE_NODE_NAMES                  _ULL(0x0000000000100000)
#define AF_CATCH_EXCEPTIONS                    _ULL(0x0000000000200000)
#define AF_HELPDESK_LINK_ACTIVE                _ULL(0x0000000000400000)
#define AF_DB_CONNECTION_POOL_READY            _ULL(0x0000000000800000)
#define AF_DB_LOCKED                           _ULL(0x0000000001000000)
#define AF_ENABLE_MULTIPLE_DB_CONN             _ULL(0x0000000002000000)
#define AF_DB_CONNECTION_LOST                  _ULL(0x0000000004000000)
#define AF_NO_NETWORK_CONNECTIVITY             _ULL(0x0000000008000000)
#define AF_EVENT_STORM_DETECTED                _ULL(0x0000000010000000)
#define AF_SNMP_TRAP_DISCOVERY                 _ULL(0x0000000020000000)
#define AF_TRAPS_FROM_UNMANAGED_NODES          _ULL(0x0000000040000000)
#define AF_RESOLVE_IP_FOR_EACH_STATUS_POLL     _ULL(0x0000000080000000)
#define AF_PERFDATA_STORAGE_DRIVER_LOADED      _ULL(0x0000000100000000)
#define AF_BACKGROUND_LOG_WRITER               _ULL(0x0000000200000000)
#define AF_SERVER_INITIALIZED                  _ULL(0x4000000000000000)
#define AF_SHUTDOWN                            _ULL(0x8000000000000000)

/**
 * Encryption usage policies
 */
#define ENCRYPTION_DISABLED   0
#define ENCRYPTION_ALLOWED    1
#define ENCRYPTION_PREFERRED  2
#define ENCRYPTION_REQUIRED   3

/**
 * Agent action output callback events
 */
enum ActionCallbackEvent
{
   ACE_CONNECTED = 0,
   ACE_DATA = 1,
   ACE_DISCONNECTED = 2
};

/**
 * Win32 service and syslog constants
 */
#ifdef _WIN32

#define CORE_SERVICE_NAME     _T("NetXMSCore")
#define CORE_EVENT_SOURCE     _T("NetXMSCore")
#define NETXMSD_SYSLOG_NAME   CORE_EVENT_SOURCE

#else

#define NETXMSD_SYSLOG_NAME   _T("netxmsd")

#endif   /* _WIN32 */

/**
 * Single ARP cache entry
 */
typedef struct
{
   UINT32 dwIndex;       // Interface index
   InetAddress ipAddr;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
} ARP_ENTRY;

/**
 * ARP cache structure used by discovery functions and AgentConnection class
 */
typedef struct
{
   UINT32 dwNumEntries;
   ARP_ENTRY *pEntries;
} ARP_CACHE;

/**
 * Interface information structure used by discovery functions and AgentConnection class
 */
class InterfaceInfo
{
public:
   UINT32 index;
   TCHAR name[MAX_DB_STRING];			// Interface display name
	TCHAR description[MAX_DB_STRING];	// Value of ifDescr MIB variable for SNMP agents
	TCHAR alias[MAX_DB_STRING];	// Value of ifDescr MIB variable for SNMP agents
   UINT32 type;
   UINT32 mtu;
	UINT32 bridgePort;
	UINT32 slot;
	UINT32 port;
   InetAddressList ipAddrList;
   BYTE macAddr[MAC_ADDR_LENGTH];
	bool isPhysicalPort;
   bool isSystem;

   InterfaceInfo(UINT32 ifIndex)
   {
      index = ifIndex;
      name[0] = 0;
      description[0] = 0;
      alias[0] = 0;
      type = IFTYPE_OTHER;
      mtu = 0;
      bridgePort = 0;
      slot = 0;
      port = 0;
      memset(macAddr, 0, sizeof(macAddr));
      isPhysicalPort = false;
      isSystem = false;
   }

   bool hasAddress(const InetAddress& addr) { return ipAddrList.hasAddress(addr); }
};

/**
 * Interface list used by discovery functions and AgentConnection class
 */
class LIBNXSRV_EXPORTABLE InterfaceList
{
private:
   ObjectArray<InterfaceInfo> *m_interfaces;
   void *m_data;                  // Can be used by custom enumeration handlers
   bool m_needPrefixWalk;

public:
	InterfaceList(int initialAlloc = 8);
	~InterfaceList();

   void add(InterfaceInfo *iface) { m_interfaces->add(iface); }
   void remove(int index) { m_interfaces->remove(index); }

	int size() { return m_interfaces->size(); }
	InterfaceInfo *get(int index) { return m_interfaces->get(index); }
	InterfaceInfo *findByIfIndex(UINT32 ifIndex);

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }

   bool isPrefixWalkNeeded() { return m_needPrefixWalk; }
   void setPrefixWalkNeeded() { m_needPrefixWalk = true; }
};

/**
 * Vlan information
 */
#define VLAN_PRM_IFINDEX   0
#define VLAN_PRM_SLOTPORT  1
#define VLAN_PRM_BPORT     2

class LIBNXSRV_EXPORTABLE VlanInfo
{
private:
	int m_vlanId;
	TCHAR *m_name;
	int m_portRefMode;	// Port reference mode - by ifIndex or by slot/port
	int m_allocated;
	int m_numPorts;	// Number of ports in VLAN
	UINT32 *m_ports;	// member ports (slot/port pairs or ifIndex)
	UINT32 *m_indexes;	// ifIndexes for ports
	UINT32 *m_ids;		// Interface object IDs for ports

public:
	VlanInfo(int vlanId, int prm);
	~VlanInfo();

	int getVlanId() { return m_vlanId; }
	int getPortReferenceMode() { return m_portRefMode; }
	const TCHAR *getName() { return CHECK_NULL_EX(m_name); }
	int getNumPorts() { return m_numPorts; }
	UINT32 *getPorts() { return m_ports; }
	UINT32 *getIfIndexes() { return m_indexes; }
	UINT32 *getIfIds() { return m_ids; }

	void add(UINT32 slot, UINT32 port);
	void add(UINT32 ifIndex);
	void setName(const TCHAR *name);

	void prepareForResolve();
	void resolvePort(int index, UINT32 sp, UINT32 ifIndex, UINT32 id);
};

/**
 * Vlan list
 */
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
	void addMemberPort(int vlanId, UINT32 portId);

	int size() { return m_size; }
	VlanInfo *get(int index) { return ((index >= 0) && (index < m_size)) ? m_vlans[index] : NULL; }
	VlanInfo *findById(int id);
	VlanInfo *findByName(const TCHAR *name);

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }

	void fillMessage(NXCPMessage *msg);
};

/**
 * Route information
 */
typedef struct
{
   UINT32 dwDestAddr;
   UINT32 dwDestMask;
   UINT32 dwNextHop;
   UINT32 dwIfIndex;
   UINT32 dwRouteType;
} ROUTE;

/**
 * Routing table
 */
typedef struct
{
   int iNumEntries;     // Number of entries
   ROUTE *pRoutes;      // Route list
} ROUTING_TABLE;

/**
 * Information about policies installed on agent
 */
class LIBNXSRV_EXPORTABLE AgentPolicyInfo
{
private:
	int m_size;
	BYTE *m_guidList;
	int *m_typeList;
	TCHAR **m_serverList;

public:
	AgentPolicyInfo(NXCPMessage *msg);
	~AgentPolicyInfo();

	int size() { return m_size; }
	bool getGuid(int index, uuid_t guid);
	int getType(int index) { return ((index >= 0) && (index < m_size)) ? m_typeList[index] : -1; }
	const TCHAR *getServer(int index) { return ((index >= 0) && (index < m_size)) ? m_serverList[index] : NULL; }
};

/**
 * Agent parameter definition
 */
class LIBNXSRV_EXPORTABLE AgentParameterDefinition
{
private:
   TCHAR *m_name;
   TCHAR *m_description;
   int m_dataType;

public:
   AgentParameterDefinition(NXCPMessage *msg, UINT32 baseId);
   AgentParameterDefinition(AgentParameterDefinition *src);
   ~AgentParameterDefinition();

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

   const TCHAR *getName() { return m_name; }
   const TCHAR *getDescription() { return m_description; }
   int getDataType() { return m_dataType; }
};

/**
 * Agent table column definition
 */
struct AgentTableColumnDefinition
{
   TCHAR m_name[MAX_COLUMN_NAME];
   int m_dataType;

   AgentTableColumnDefinition(AgentTableColumnDefinition *src)
   {
      nx_strncpy(m_name, src->m_name, MAX_COLUMN_NAME);
      m_dataType = src->m_dataType;
   }
};

/**
 * Agent table definition
 */
class LIBNXSRV_EXPORTABLE AgentTableDefinition
{
private:
   TCHAR *m_name;
   TCHAR *m_description;
   StringList *m_instanceColumns;
   ObjectArray<AgentTableColumnDefinition> *m_columns;

public:
   AgentTableDefinition(NXCPMessage *msg, UINT32 baseId);
   AgentTableDefinition(AgentTableDefinition *src);
   ~AgentTableDefinition();

   UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

   const TCHAR *getName() { return m_name; }
   const TCHAR *getDescription() { return m_description; }
};

/**
 * Agent connection
 */
class LIBNXSRV_EXPORTABLE AgentConnection
{
private:
   InetAddress m_addr;
   int m_nProtocolVersion;
   int m_iAuthMethod;
   char m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   SOCKET m_hSocket;
   UINT32 m_dwNumDataLines;
   VolatileCounter m_requestId;
   UINT32 m_dwCommandTimeout;
	UINT32 m_connectionTimeout;
   UINT32 m_dwRecvTimeout;
   TCHAR **m_ppDataLines;
   MsgWaitQueue *m_pMsgWaitQueue;
   bool m_isConnected;
   MUTEX m_mutexDataLock;
	MUTEX m_mutexSocketWrite;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_pCtx;
   int m_iEncryptionPolicy;
   BOOL m_bUseProxy;
   InetAddress m_proxyAddr;
   WORD m_wPort;
   WORD m_wProxyPort;
   int m_iProxyAuth;
   char m_szProxySecret[MAX_SECRET_LENGTH];
	int m_hCurrFile;
	TCHAR m_currentFileName[MAX_PATH];
	UINT32 m_dwDownloadRequestId;
	CONDITION m_condFileDownload;
	BOOL m_fileDownloadSucceeded;
	void (*m_downloadProgressCallback)(size_t, void *);
	void *m_downloadProgressCallbackArg;
	bool m_deleteFileOnDownloadFailure;
	void (*m_sendToClientMessageCallback)(NXCP_MESSAGE*, void *);
	bool m_fileUploadInProgress;

   void receiverThread();
   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *);

protected:
   void destroyResultData();
   UINT32 waitForRCC(UINT32 dwRqId, UINT32 dwTimeOut);
   UINT32 setupEncryption(RSA *pServerKey);
   UINT32 authenticate(BOOL bProxyData);
   UINT32 setupProxyConnection();
   const InetAddress& getIpAddr() { return m_addr; }
	UINT32 prepareFileDownload(const TCHAR *fileName, UINT32 rqId, bool append, void (*downloadProgressCallback)(size_t, void *), void (*fileResendCallback)(NXCP_MESSAGE*, void *), void *cbArg);

   virtual void printMsg(const TCHAR *format, ...);
   virtual void onTrap(NXCPMessage *pMsg);
	virtual void onDataPush(NXCPMessage *msg);
	virtual void onFileMonitoringData(NXCPMessage *msg);
	virtual void onSnmpTrap(NXCPMessage *pMsg);
	virtual void onFileDownload(BOOL success);
	virtual UINT32 processCollectedData(NXCPMessage *msg);
	virtual bool processCustomMessage(NXCPMessage *pMsg);

   void lock() { MutexLock(m_mutexDataLock); }
   void unlock() { MutexUnlock(m_mutexDataLock); }
	NXCPEncryptionContext *acquireEncryptionContext();

public:
   BOOL sendMessage(NXCPMessage *pMsg);
   NXCPMessage *waitForMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut) { return m_pMsgWaitQueue->waitForMessage(wCode, dwId, dwTimeOut); }
   AgentConnection(InetAddress addr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL);
   virtual ~AgentConnection();

   bool connect(RSA *pServerKey = NULL, BOOL bVerbose = FALSE, UINT32 *pdwError = NULL, UINT32 *pdwSocketError = NULL);
   void disconnect();
   bool isConnected() { return m_isConnected; }
	int getProtocolVersion() { return m_nProtocolVersion; }

	SOCKET getSocket() { return m_hSocket; }

   ARP_CACHE *getArpCache();
   InterfaceList *getInterfaceList();
   ROUTING_TABLE *getRoutingTable();
   UINT32 getParameter(const TCHAR *pszParam, UINT32 dwBufSize, TCHAR *pszBuffer);
   UINT32 getList(const TCHAR *pszParam);
   UINT32 getTable(const TCHAR *pszParam, Table **table);
   UINT32 nop();
   UINT32 enableIPv6();
   UINT32 setServerId(UINT64 serverId);
   UINT32 execAction(const TCHAR *pszAction, int argc, TCHAR **argv, bool withOutput = false, void (* outputCallback)(ActionCallbackEvent, const TCHAR *, void *) = NULL, void *cbData = NULL);
   UINT32 uploadFile(const TCHAR *localFile, const TCHAR *destinationFile = NULL, void (* progressCallback)(INT64, void *) = NULL, void *cbArg = NULL, NXCPCompressionMethod compMethod = NXCP_COMPRESSION_NONE);
   UINT32 startUpgrade(const TCHAR *pszPkgName);
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& addr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, const TCHAR *pszRequest = NULL, const TCHAR *pszResponse = NULL, UINT32 *responseTime = NULL);
   UINT32 getSupportedParameters(ObjectArray<AgentParameterDefinition> **paramList, ObjectArray<AgentTableDefinition> **tableList);
   UINT32 getConfigFile(TCHAR **ppszConfig, UINT32 *pdwSize);
   UINT32 updateConfigFile(const TCHAR *pszConfig);
   UINT32 enableTraps();
   UINT32 enableFileUpdates();
	UINT32 getPolicyInventory(AgentPolicyInfo **info);
	UINT32 uninstallPolicy(uuid_t guid);
   UINT32 takeScreenshot(const TCHAR *sessionName, BYTE **data, size_t *size);

	UINT32 generateRequestId() { return (UINT32)InterlockedIncrement(&m_requestId); }
	NXCPMessage *customRequest(NXCPMessage *pRequest, const TCHAR *recvFile = NULL, bool append = false, void (*downloadProgressCallback)(size_t, void *) = NULL,
	                           void (*fileResendCallback)(NXCP_MESSAGE*, void *) = NULL, void *cbArg = NULL);

   UINT32 getNumDataLines() { return m_dwNumDataLines; }
   const TCHAR *getDataLine(UINT32 dwIndex) { return dwIndex < m_dwNumDataLines ? m_ppDataLines[dwIndex] : _T("(error)"); }

   void setConnectionTimeout(UINT32 dwTimeout) { m_connectionTimeout = max(dwTimeout, 1000); }
	UINT32 getConnectionTimeout() { return m_connectionTimeout; }
   void setCommandTimeout(UINT32 dwTimeout) { m_dwCommandTimeout = max(dwTimeout, 500); }
	UINT32 getCommandTimeout() { return m_dwCommandTimeout; }
   void setRecvTimeout(UINT32 dwTimeout) { m_dwRecvTimeout = max(dwTimeout, 10000); }
   void setEncryptionPolicy(int iPolicy) { m_iEncryptionPolicy = iPolicy; }
   void setProxy(InetAddress addr, WORD wPort = AGENT_LISTEN_PORT,
                 int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   void setPort(WORD wPort) { m_wPort = wPort; }
   void setAuthData(int method, const TCHAR *secret);
   void setDeleteFileOnDownloadFailure(bool flag) { m_deleteFileOnDownloadFailure = flag; }
   BOOL sendRawMessage(NXCP_MESSAGE *pMsg);
};

/**
 * Proxy SNMP transport
 */
class LIBNXSRV_EXPORTABLE SNMP_ProxyTransport : public SNMP_Transport
{
protected:
	AgentConnection *m_agentConnection;
	NXCPMessage *m_response;
	InetAddress m_ipAddr;
	WORD m_port;
	bool m_waitForResponse;

public:
	SNMP_ProxyTransport(AgentConnection *conn, const InetAddress& ipAddr, WORD port);
	virtual ~SNMP_ProxyTransport();

   virtual int readMessage(SNMP_PDU **ppData, UINT32 dwTimeout = INFINITE,
                           struct sockaddr *pSender = NULL, socklen_t *piAddrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL);
   virtual int sendMessage(SNMP_PDU *pdu);
   virtual InetAddress getPeerIpAddress();

   void setWaitForResponse(bool wait) { m_waitForResponse = wait; }
};

/**
 * ISC flags
 */
#define ISCF_IS_CONNECTED        ((UINT32)0x00000001)
#define ISCF_REQUIRE_ENCRYPTION  ((UINT32)0x00000002)

/**
 * Inter-server connection (ISC)
 */
class LIBNXSRV_EXPORTABLE ISC
{
private:
	UINT32 m_flags;
   InetAddress m_addr;
	WORD m_port;
   SOCKET m_socket;
   int m_protocolVersion;
	VolatileCounter m_requestId;
	UINT32 m_recvTimeout;
   MsgWaitQueue *m_msgWaitQueue;
   MUTEX m_mutexDataLock;
	MUTEX m_socketLock;
   THREAD m_hReceiverThread;
   NXCPEncryptionContext *m_ctx;
	UINT32 m_commandTimeout;

   void receiverThread();
   static THREAD_RESULT THREAD_CALL receiverThreadStarter(void *);

protected:
   UINT32 setupEncryption(RSA *pServerKey);
	UINT32 connectToService(UINT32 service);

   void lock() { MutexLock(m_mutexDataLock); }
   void unlock() { MutexUnlock(m_mutexDataLock); }

   virtual void printMessage(const TCHAR *format, ...);
   virtual void onBinaryMessage(NXCP_MESSAGE *rawMsg);
   virtual bool onMessage(NXCPMessage *msg);

public:
   ISC();
   ISC(const InetAddress& addr, WORD port = NETXMS_ISC_PORT);
   virtual ~ISC();

   UINT32 connect(UINT32 service, RSA *serverKey = NULL, BOOL requireEncryption = FALSE);
	void disconnect();
   bool connected() { return m_flags & ISCF_IS_CONNECTED; };

   BOOL sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(WORD code, UINT32 id, UINT32 timeOut) { return m_msgWaitQueue->waitForMessage(code, id, timeOut); }
   UINT32 waitForRCC(UINT32 rqId, UINT32 timeOut);
   UINT32 generateMessageId() { return (UINT32)InterlockedIncrement(&m_requestId); }

   UINT32 nop();
};


//
// Functions
//

void LIBNXSRV_EXPORTABLE DestroyArpCache(ARP_CACHE *pArpCache);
void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT);
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT);
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(UINT32 err);
UINT32 LIBNXSRV_EXPORTABLE AgentErrorToRCC(UINT32 err);

void LIBNXSRV_EXPORTABLE WriteLogOther(WORD wType, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;

void LIBNXSRV_EXPORTABLE DbgPrintf(int level, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;

void LIBNXSRV_EXPORTABLE DbgPrintf2(int level, const TCHAR *format, va_list args);

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy);

const TCHAR LIBNXSRV_EXPORTABLE *ISCErrorCodeToText(UINT32 code);

/**
 * Variables
 */
extern UINT64 LIBNXSRV_EXPORTABLE g_flags;
extern UINT32 LIBNXSRV_EXPORTABLE g_debugLevel;

/**
 * Helper finctions for checking server flags
 */
inline bool IsStandalone()
{
	return !(g_flags & AF_DAEMON) ? true : false;
}

inline bool IsZoningEnabled()
{
	return (g_flags & AF_ENABLE_ZONING) ? true : false;
}

inline bool IsShutdownInProgress()
{
	return (g_flags & AF_SHUTDOWN) ? true : false;
}


#endif   /* _nxsrvapi_h_ */
