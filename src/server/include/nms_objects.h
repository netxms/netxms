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
** File: nms_objects.h
**
**/

#ifndef _nms_objects_h_
#define _nms_objects_h_

#include <nms_agent.h>
#include <netxms_maps.h>
#include <geolocation.h>
#include "nxcore_jobs.h"
#include "nms_topo.h"
#include "nxcore_reports.h"

/**
 * Forward declarations of classes
 */
class AgentConnection;
class AgentConnectionEx;
class ClientSession;
class Queue;
class DataCollectionTarget;

/**
 * Global variables used by inline functions
 */
extern UINT32 g_dwDiscoveryPollingInterval;
extern UINT32 g_dwStatusPollingInterval;
extern UINT32 g_dwConfigurationPollingInterval;
extern UINT32 g_dwRoutingTableUpdateInterval;
extern UINT32 g_dwTopologyPollingInterval;
extern UINT32 g_dwConditionPollingInterval;

/**
 * Constants
 */
#define MAX_INTERFACES        4096
#define MAX_ATTR_NAME_LEN     128
#define INVALID_INDEX         0xFFFFFFFF

/**
 * Last events
 */
#define MAX_LAST_EVENTS       8

#define LAST_EVENT_NODE_DOWN  0
#define LAST_EVENT_AGENT_DOWN 1

/**
 * Built-in object IDs
 */
#define BUILTIN_OID_NETWORK               1
#define BUILTIN_OID_SERVICEROOT           2
#define BUILTIN_OID_TEMPLATEROOT          3
#define BUILTIN_OID_ZONE0                 4
#define BUILTIN_OID_POLICYROOT            5
#define BUILTIN_OID_NETWORKMAPROOT        6
#define BUILTIN_OID_DASHBOARDROOT         7
#define BUILTIN_OID_REPORTROOT            8
#define BUILTIN_OID_BUSINESSSERVICEROOT   9

/**
 * Node runtime (dynamic) flags
 */
#define NDF_QUEUED_FOR_STATUS_POLL     0x0001
#define NDF_QUEUED_FOR_CONFIG_POLL     0x0002
#define NDF_UNREACHABLE                0x0004
#define NDF_AGENT_UNREACHABLE          0x0008
#define NDF_SNMP_UNREACHABLE           0x0010
#define NDF_QUEUED_FOR_DISCOVERY_POLL  0x0020
#define NDF_FORCE_STATUS_POLL          0x0040
#define NDF_FORCE_CONFIGURATION_POLL   0x0080
#define NDF_QUEUED_FOR_ROUTE_POLL      0x0100
#define NDF_CPSNMP_UNREACHABLE         0x0200
#define NDF_RECHECK_CAPABILITIES       0x0400
#define NDF_POLLING_DISABLED           0x0800
#define NDF_CONFIGURATION_POLL_PASSED  0x1000
#define NDF_QUEUED_FOR_TOPOLOGY_POLL   0x2000
#define NDF_DELETE_IN_PROGRESS         0x4000
#define NDF_NETWORK_PATH_PROBLEM       0x8000

#define NDF_PERSISTENT (NDF_UNREACHABLE | NDF_NETWORK_PATH_PROBLEM | NDF_AGENT_UNREACHABLE | NDF_SNMP_UNREACHABLE | NDF_CPSNMP_UNREACHABLE)

#define __NDF_FLAGS_DEFINED

/**
 * Cluster runtime flags
 */
#define CLF_QUEUED_FOR_STATUS_POLL     0x0001
#define CLF_DOWN								0x0002

/**
 * Status poll types
 */
enum StatusPollType
{
   POLL_ICMP_PING = 0,
   POLL_SNMP = 1,
   POLL_NATIVE_AGENT =2
};

/**
 * Zone types
 */
#define ZONE_TYPE_PASSIVE     0
#define ZONE_TYPE_ACTIVE      1

/**
 * Template update types
 */
#define APPLY_TEMPLATE        0
#define REMOVE_TEMPLATE       1

/**
 * Queued template update information
 */
struct TEMPLATE_UPDATE_INFO
{
   int iUpdateType;
   Template *pTemplate;
   UINT32 targetId;
   BOOL bRemoveDCI;
};

/**
 * Object index element
 */
struct INDEX_ELEMENT
{
   QWORD key;
   NetObj *object;
};

/**
 * Object index
 */
class NXCORE_EXPORTABLE ObjectIndex
{
private:
	int m_size;
	int m_allocated;
	INDEX_ELEMENT *m_elements;
	RWLOCK m_lock;

	int findElement(QWORD key);

public:
	ObjectIndex();
	~ObjectIndex();

	bool put(QWORD key, NetObj *object);
	void remove(QWORD key);
	NetObj *get(QWORD key);
	NetObj *find(bool (*comparator)(NetObj *, void *), void *data);

	int getSize();
	ObjectArray<NetObj> *getObjects(bool updateRefCount);

	void forEach(void (*callback)(NetObj *, void *), void *data);
};

/**
 * Node component
 */
class Component
{
protected:
	UINT32 m_index;
	UINT32 m_class;
	UINT32 m_ifIndex;
	TCHAR *m_name;
	TCHAR *m_description;
	TCHAR *m_model;
	TCHAR *m_serial;
	TCHAR *m_vendor;
	TCHAR *m_firmware;
	UINT32 m_parentIndex;
	ObjectArray<Component> m_childs;

public:
	Component(UINT32 index, const TCHAR *name);
	virtual ~Component();

	UINT32 updateFromSnmp(SNMP_Transport *snmp);
	void buildTree(ObjectArray<Component> *elements);

	UINT32 getIndex() { return m_index; }
	UINT32 getParentIndex() { return m_parentIndex; }

	UINT32 fillMessage(CSCPMessage *msg, UINT32 baseId);

	void print(CONSOLE_CTX console, int level);
};

/**
 * Node component tree
 */
class ComponentTree : public RefCountObject
{
private:
	Component *m_root;

public:
	ComponentTree(Component *root);
	virtual ~ComponentTree();

	void fillMessage(CSCPMessage *msg, UINT32 baseId);
	void print(CONSOLE_CTX console) { if (m_root != NULL) m_root->print(console, 0); }

	bool isEmpty() { return m_root == NULL; }
	Component *getRoot() { return m_root; }
};

/**
 * Software package information
 */
class SoftwarePackage
{
private:
	TCHAR *m_name;
	TCHAR *m_version;
	TCHAR *m_vendor;
	time_t m_date;
	TCHAR *m_url;
	TCHAR *m_description;

public:
	SoftwarePackage(Table *table, int row);
	~SoftwarePackage();

	void fillMessage(CSCPMessage *msg, UINT32 baseId);
};

/**
 * Summary table column flags
 */
#define COLUMN_DEFINITION_REGEXP_MATCH    0x0001

/**
 * Column definition for DCI summary table
 */
class NXCORE_EXPORTABLE SummaryTableColumn
{
public:
   TCHAR m_name[MAX_DB_STRING];
   TCHAR m_dciName[MAX_PARAM_NAME];
   UINT32 m_flags;

   SummaryTableColumn(TCHAR *configStr);
};

/**
 * DCI summary table class
 */
class NXCORE_EXPORTABLE SummaryTable
{
private:
   TCHAR m_title[MAX_DB_STRING];
   UINT32 m_flags;
   ObjectArray<SummaryTableColumn> *m_columns;
   NXSL_VM *m_filter;

   SummaryTable(DB_RESULT hResult);

public:
   static SummaryTable *loadFromDB(LONG id, UINT32 *rcc);
   ~SummaryTable();

   bool filter(DataCollectionTarget *node);
   Table *createEmptyResultTable();

   int getNumColumns() { return m_columns->size(); }
   SummaryTableColumn *getColumn(int index) { return m_columns->get(index); }
};

/**
 * Base class for network objects
 */
class NXCORE_EXPORTABLE NetObj
{
private:
	static void onObjectDeleteCallback(NetObj *object, void *data);

	void getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly);

protected:
   UINT32 m_dwId;
	uuid_t m_guid;
   UINT32 m_dwTimeStamp;       // Last change time stamp
   UINT32 m_dwRefCount;        // Number of references. Object can be destroyed only when this counter is zero
   TCHAR m_szName[MAX_OBJECT_NAME];
   TCHAR *m_pszComments;      // User comments
   int m_iStatus;
   int m_iStatusCalcAlg;      // Status calculation algorithm
   int m_iStatusPropAlg;      // Status propagation algorithm
   int m_iFixedStatus;        // Status if propagation is "Fixed"
   int m_iStatusShift;        // Shift value for "shifted" status propagation
   int m_iStatusTranslation[4];
   int m_iStatusSingleThreshold;
   int m_iStatusThresholds[4];
   bool m_isModified;
   bool m_isDeleted;
   bool m_isHidden;
	bool m_isSystem;
	uuid_t m_image;
   MUTEX m_mutexData;         // Object data access mutex
   MUTEX m_mutexRefCount;     // Reference counter access mutex
   RWLOCK m_rwlockParentList; // Lock for parent list
   RWLOCK m_rwlockChildList;  // Lock for child list
   UINT32 m_dwIpAddr;          // Every object should have an IP address
	GeoLocation m_geoLocation;
   ClientSession *m_pollRequestor;
	UINT32 m_submapId;				// Map object which should be open on drill-down request

   UINT32 m_dwChildCount;      // Number of child objects
   NetObj **m_pChildList;     // Array of pointers to child objects

   UINT32 m_dwParentCount;     // Number of parent objects
   NetObj **m_pParentList;    // Array of pointers to parent objects

   AccessList *m_pAccessList;
   BOOL m_bInheritAccessRights;
   MUTEX m_mutexACL;

	UINT32 m_dwNumTrustedNodes;	// Trusted nodes
	UINT32 *m_pdwTrustedNodes;

	StringMap m_customAttributes;

   void LockData() { MutexLock(m_mutexData); }
   void UnlockData() { MutexUnlock(m_mutexData); }
   void lockACL() { MutexLock(m_mutexACL); }
   void unlockACL() { MutexUnlock(m_mutexACL); }
   void LockParentList(BOOL bWrite)
   {
      if (bWrite)
         RWLockWriteLock(m_rwlockParentList, INFINITE);
      else
         RWLockReadLock(m_rwlockParentList, INFINITE);
   }
   void UnlockParentList() { RWLockUnlock(m_rwlockParentList); }
   void LockChildList(BOOL bWrite)
   {
      if (bWrite)
         RWLockWriteLock(m_rwlockChildList, INFINITE);
      else
         RWLockReadLock(m_rwlockChildList, INFINITE);
   }
   void UnlockChildList() { RWLockUnlock(m_rwlockChildList); }

   void Modify();                  // Used to mark object as modified

   BOOL loadACLFromDB();
   BOOL saveACLToDB(DB_HANDLE hdb);
   BOOL loadCommonProperties();
   BOOL saveCommonProperties(DB_HANDLE hdb);
	BOOL loadTrustedNodes();
	BOOL saveTrustedNodes(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query);

   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...);

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 dwObjectId);

public:
   NetObj();
   virtual ~NetObj();

   virtual int Type() { return OBJECT_GENERIC; }

   UINT32 IpAddr() { return m_dwIpAddr; }
   UINT32 Id() { return m_dwId; }
   const TCHAR *Name() { return m_szName; }
   int Status() { return m_iStatus; }
   int getPropagatedStatus();
   UINT32 getTimeStamp() { return m_dwTimeStamp; }
	void getGuid(uuid_t out) { memcpy(out, m_guid, UUID_LENGTH); }
	const TCHAR *getComments() { return CHECK_NULL_EX(m_pszComments); }

   bool isModified() { return m_isModified; }
   bool isDeleted() { return m_isDeleted; }
   bool isOrphaned() { return m_dwParentCount == 0; }
   bool isEmpty() { return m_dwChildCount == 0; }

	bool isSystem() { return m_isSystem; }
	void setSystemFlag(bool flag) { m_isSystem = flag; }

   UINT32 getRefCount();
   void incRefCount();
   void decRefCount();

   bool isChild(UINT32 id);
	bool isTrustedNode(UINT32 id);

   void AddChild(NetObj *pObject);     // Add reference to child object
   void AddParent(NetObj *pObject);    // Add reference to parent object

   void DeleteChild(NetObj *pObject);  // Delete reference to child object
   void DeleteParent(NetObj *pObject); // Delete reference to parent object

   void deleteObject(NetObj *initiator = NULL);     // Prepare object for deletion

   bool isHidden() { return m_isHidden; }
   void hide();
   void unhide();

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   void setId(UINT32 dwId) { m_dwId = dwId; Modify(); }
	void generateGuid() { uuid_generate(m_guid); }
   void setName(const TCHAR *pszName) { nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME); Modify(); }
   void resetStatus() { m_iStatus = STATUS_UNKNOWN; Modify(); }
   void setComments(TCHAR *pszText);	/* pszText must be dynamically allocated */

   virtual void setMgmtStatus(BOOL bIsManaged);
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
	virtual void postModify();

   void commentsToMessage(CSCPMessage *pMsg);

   UINT32 getUserRights(UINT32 dwUserId);
   BOOL checkAccessRights(UINT32 dwUserId, UINT32 dwRequiredRights);
   void dropUserAccess(UINT32 dwUserId);

   void addChildNodesToList(ObjectArray<Node> *nodeList, UINT32 dwUserId);
   void addChildDCTargetsToList(ObjectArray<DataCollectionTarget> *dctList, UINT32 dwUserId);

   const TCHAR *getCustomAttribute(const TCHAR *name) { return m_customAttributes.get(name); }
   void setCustomAttribute(const TCHAR *name, const TCHAR *value) { m_customAttributes.set(name, value); Modify(); }
   void setCustomAttributePV(const TCHAR *name, TCHAR *value) { m_customAttributes.setPreallocated(_tcsdup(name), value); Modify(); }
   void deleteCustomAttribute(const TCHAR *name) { m_customAttributes.remove(name); Modify(); }

	ObjectArray<NetObj> *getParentList(int typeFilter);
	ObjectArray<NetObj> *getChildList(int typeFilter);
	ObjectArray<NetObj> *getFullChildList(bool eventSourceOnly, bool updateRefCount);

   int getChildCount() { return (int)m_dwChildCount; }
   int getParentCount() { return (int)m_dwParentCount; }

	virtual NXSL_Array *getParentsForNXSL();
	virtual NXSL_Array *getChildrenForNXSL();

	virtual bool showThresholdSummary();
   virtual bool isEventSource();

   // Debug methods
   const TCHAR *dbgGetParentList(TCHAR *szBuffer);
   const TCHAR *dbgGetChildList(TCHAR *szBuffer);
};

/**
 * Get object's reference count
 */
inline UINT32 NetObj::getRefCount()
{
   UINT32 dwRefCount;

   MutexLock(m_mutexRefCount);
   dwRefCount = m_dwRefCount;
   MutexUnlock(m_mutexRefCount);
   return dwRefCount;
}

/**
 * Increment object's reference count
 */
inline void NetObj::incRefCount()
{
   MutexLock(m_mutexRefCount);
   m_dwRefCount++;
   MutexUnlock(m_mutexRefCount);
}

/**
 * Decrement object's reference count
 */
inline void NetObj::decRefCount()
{
   MutexLock(m_mutexRefCount);
   if (m_dwRefCount > 0)
      m_dwRefCount--;
   MutexUnlock(m_mutexRefCount);
}

/**
 * Data collection template class
 */
class NXCORE_EXPORTABLE Template : public NetObj
{
protected:
	ObjectArray<DCObject> *m_dcObjects;
   UINT32 m_dwDCILockStatus;
   UINT32 m_dwVersion;
	UINT32 m_flags;
   BOOL m_bDCIListModified;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];
	TCHAR *m_applyFilterSource;
	NXSL_VM *m_applyFilter;
	RWLOCK m_dciAccessLock;

   virtual void prepareForDeletion();

   void loadItemsFromDB();
   void destroyItems();
   void updateInstanceDiscoveryItems(DCItem *dci);

   void lockDciAccess(bool writeLock) { if (writeLock) { RWLockWriteLock(m_dciAccessLock, INFINITE); } else { RWLockReadLock(m_dciAccessLock, INFINITE); } }
	void unlockDciAccess() { RWLockUnlock(m_dciAccessLock); }

public:
   Template();
   Template(const TCHAR *pszName);
	Template(ConfigEntry *config);
   virtual ~Template();

   virtual int Type() { return OBJECT_TEMPLATE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   int getVersionMajor() { return m_dwVersion >> 16; }
   int getVersionMinor() { return m_dwVersion & 0xFFFF; }

   int getItemCount() { return m_dcObjects->size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false);
   bool updateDCObject(UINT32 dwItemId, CSCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId);
   bool deleteDCObject(UINT32 dcObjectId, bool needLock);
   bool setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus);
   int getItemType(UINT32 dwItemId);
   DCObject *getDCObjectById(UINT32 itemId);
   DCObject *getDCObjectByTemplateId(UINT32 tmplItemId);
   DCObject *getDCObjectByIndex(int index);
   DCObject *getDCObjectByName(const TCHAR *name);
   DCObject *getDCObjectByDescription(const TCHAR *description);
   NXSL_Value *getAllDCObjectsForNXSL(const TCHAR *name, const TCHAR *description);
   BOOL lockDCIList(UINT32 dwSessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner);
   BOOL unlockDCIList(UINT32 dwSessionId);
   void setDCIModificationFlag() { m_bDCIListModified = TRUE; }
   void sendItemsToClient(ClientSession *pSession, UINT32 dwRqId);
   BOOL isLockedBySession(UINT32 dwSessionId) { return m_dwDCILockStatus == dwSessionId; }
   UINT32 *getDCIEventsList(UINT32 *pdwCount);

   BOOL applyToTarget(DataCollectionTarget *pNode);
	bool isApplicable(Node *node);
	bool isAutoApplyEnabled() { return (m_flags & TF_AUTO_APPLY) ? true : false; }
	bool isAutoRemoveEnabled() { return ((m_flags & (TF_AUTO_APPLY | TF_AUTO_REMOVE)) == (TF_AUTO_APPLY | TF_AUTO_REMOVE)) ? true : false; }
	void setAutoApplyFilter(const TCHAR *filter);
   void queueUpdate();
   void queueRemoveFromTarget(UINT32 targetId, BOOL bRemoveDCI);

   void createNXMPRecord(String &str);

	bool enumDCObjects(bool (* pfCallback)(DCObject *, UINT32, void *), void *pArg);
	void associateItems();

   UINT32 getLastValues(CSCPMessage *msg, bool objectTooltipOnly);
};

/**
 * Interface class
 */
class NXCORE_EXPORTABLE Interface : public NetObj
{
protected:
	UINT32 m_flags;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
   UINT32 m_dwIfIndex;
   UINT32 m_dwIfType;
   UINT32 m_dwIpNetMask;
   BYTE m_bMacAddr[MAC_ADDR_LENGTH];
	UINT32 m_bridgePortNumber;		// 802.1D port number
	UINT32 m_slotNumber;				// Vendor/device specific slot number
	UINT32 m_portNumber;				// Vendor/device specific port number
	UINT32 m_peerNodeId;				// ID of peer node object, or 0 if unknown
	UINT32 m_peerInterfaceId;		// ID of peer interface object, or 0 if unknown
	WORD m_adminState;				// interface administrative state
	WORD m_operState;					// interface operational state
	WORD m_dot1xPaeAuthState;		// 802.1x port auth state
	WORD m_dot1xBackendAuthState;	// 802.1x backend auth state
   QWORD m_lastDownEventId;
	int m_pendingStatus;
	int m_pollCount;
	int m_requiredPollCount;
   UINT32 m_zoneId;

	void paeStatusPoll(ClientSession *pSession, UINT32 dwRqId, SNMP_Transport *pTransport, Node *node);

protected:
   virtual void onObjectDelete(UINT32 dwObjectId);

public:
   Interface();
   Interface(UINT32 dwAddr, UINT32 dwNetMask, UINT32 zoneId, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, UINT32 ipAddr, UINT32 ipNetMask, UINT32 ifType, UINT32 zoneId);
   virtual ~Interface();

   virtual int Type() { return OBJECT_INTERFACE; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   Node *getParentNode();
   UINT32 getParentNodeId();

   UINT32 getZoneId() { return m_zoneId; }
   UINT32 getIpNetMask() { return m_dwIpNetMask; }
   UINT32 getIfIndex() { return m_dwIfIndex; }
   UINT32 getIfType() { return m_dwIfType; }
	UINT32 getBridgePortNumber() { return m_bridgePortNumber; }
	UINT32 getSlotNumber() { return m_slotNumber; }
	UINT32 getPortNumber() { return m_portNumber; }
	UINT32 getPeerNodeId() { return m_peerNodeId; }
	UINT32 getPeerInterfaceId() { return m_peerInterfaceId; }
	UINT32 getFlags() { return m_flags; }
	int getAdminState() { return (int)m_adminState; }
	int getOperState() { return (int)m_operState; }
	int getDot1xPaeAuthState() { return (int)m_dot1xPaeAuthState; }
	int getDot1xBackendAuthState() { return (int)m_dot1xBackendAuthState; }
	const TCHAR *getDescription() { return m_description; }
   const BYTE *getMacAddr() { return m_bMacAddr; }
	bool isSyntheticMask() { return (m_flags & IF_SYNTHETIC_MASK) ? true : false; }
	bool isPhysicalPort() { return (m_flags & IF_PHYSICAL_PORT) ? true : false; }
	bool isLoopback() { return (m_flags & IF_LOOPBACK) ? true : false; }
	bool isManuallyCreated() { return (m_flags & IF_CREATED_MANUALLY) ? true : false; }
	bool isExcludedFromTopology() { return (m_flags & (IF_EXCLUDE_FROM_TOPOLOGY | IF_LOOPBACK)) ? true : false; }
   bool isFake() { return (m_dwIfIndex == 1) &&
                          (m_dwIfType == IFTYPE_OTHER) &&
                          (!_tcscmp(m_szName, _T("lan0")) || !_tcscmp(m_szName, _T("unknown"))) &&
                          (!memcmp(m_bMacAddr, "\x00\x00\x00\x00\x00\x00", 6)); }

   QWORD getLastDownEventId() { return m_lastDownEventId; }
   void setLastDownEventId(QWORD id) { m_lastDownEventId = id; }

   void setMacAddr(const BYTE *pbNewMac) { memcpy(m_bMacAddr, pbNewMac, MAC_ADDR_LENGTH); Modify(); }
   void setIpAddr(UINT32 dwNewAddr);
   void setIpNetMask(UINT32 dwNewMask);
   void setBridgePortNumber(UINT32 bpn) { m_bridgePortNumber = bpn; Modify(); }
   void setSlotNumber(UINT32 slot) { m_slotNumber = slot; Modify(); }
   void setPortNumber(UINT32 port) { m_portNumber = port; Modify(); }
	void setPhysicalPortFlag(bool isPhysical) { if (isPhysical) m_flags |= IF_PHYSICAL_PORT; else m_flags &= ~IF_PHYSICAL_PORT; Modify(); }
	void setManualCreationFlag(bool isManual) { if (isManual) m_flags |= IF_CREATED_MANUALLY; else m_flags &= ~IF_CREATED_MANUALLY; Modify(); }
	void setPeer(Node *node, Interface *iface);
   void clearPeer() { m_peerNodeId = 0; m_peerInterfaceId = 0; Modify(); }
   void setDescription(const TCHAR *descr) { nx_strncpy(m_description, descr, MAX_DB_STRING); Modify(); }

	void updateZoneId();

   void statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, bool clusterSync, SNMP_Transport *snmpTransport);

	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   UINT32 wakeUp();
	void setExpectedState(int state);
};

/**
 * Network service class
 */
class NetworkService : public NetObj
{
protected:
   int m_serviceType;   // SSH, POP3, etc.
   Node *m_hostNode;    // Pointer to node object which hosts this service
   UINT32 m_pollerNode; // ID of node object which is used for polling
                         // If 0, m_pHostNode->m_dwPollerNode will be used
   WORD m_proto;        // Protocol (TCP, UDP, etc.)
   WORD m_port;         // TCP or UDP port number
   TCHAR *m_request;  // Service-specific request
   TCHAR *m_response; // Service-specific expected response
	int m_pendingStatus;
	int m_pollCount;
	int m_requiredPollCount;

   virtual void onObjectDelete(UINT32 dwObjectId);

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  Node *pHostNode = NULL, UINT32 dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int Type() { return OBJECT_NETWORKSERVICE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   void statusPoll(ClientSession *session, UINT32 rqId, Node *pollerNode, Queue *eventQueue);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};

/**
 * VPN connector class
 */
class VPNConnector : public NetObj
{
protected:
   UINT32 m_dwPeerGateway;        // Object ID of peer gateway
   UINT32 m_dwNumLocalNets;
   IP_NETWORK *m_pLocalNetList;
   UINT32 m_dwNumRemoteNets;
   IP_NETWORK *m_pRemoteNetList;

   Node *GetParentNode();

public:
   VPNConnector();
   VPNConnector(bool hidden);
   virtual ~VPNConnector();

   virtual int Type() { return OBJECT_VPNCONNECTOR; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   BOOL isLocalAddr(UINT32 dwIpAddr);
   BOOL isRemoteAddr(UINT32 dwIpAddr);
   UINT32 getPeerGatewayAddr();
};

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public Template
{
protected:
	virtual bool isDataCollectionDisabled();

public:
   DataCollectionTarget();
   DataCollectionTarget(const TCHAR *name);
   virtual ~DataCollectionTarget();

   virtual bool deleteFromDB(DB_HANDLE hdb);

	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual UINT32 getInternalItem(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);

   UINT32 getTableLastValues(UINT32 dciId, CSCPMessage *msg);
	UINT32 getThresholdSummary(CSCPMessage *msg, UINT32 baseId);
	UINT32 getPerfTabDCIList(CSCPMessage *pMsg);
   void getLastValuesSummary(SummaryTable *tableDefinition, Table *tableData);

   void updateDciCache();
   void cleanDCIData();
   void queueItemsForPolling(Queue *pPollerQueue);
	void processNewDCValue(DCObject *dco, time_t currTime, void *value);

	bool applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject);
   void cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList);
   virtual void unbindFromTemplate(UINT32 dwTemplateId, BOOL bRemoveDCI);

   virtual bool isEventSource();
};

/**
 * Mobile device class
 */
class NXCORE_EXPORTABLE MobileDevice : public DataCollectionTarget
{
protected:
	time_t m_lastReportTime;
	TCHAR *m_deviceId;
	TCHAR *m_vendor;
	TCHAR *m_model;
	TCHAR *m_serialNumber;
	TCHAR *m_osName;
	TCHAR *m_osVersion;
	TCHAR *m_userId;
	LONG m_batteryLevel;

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   virtual int Type() { return OBJECT_MOBILEDEVICE; }

   virtual BOOL CreateFromDB(UINT32 dwId);
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);

	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	void updateSystemInfo(CSCPMessage *msg);
	void updateStatus(CSCPMessage *msg);

	const TCHAR *getDeviceId() { return CHECK_NULL_EX(m_deviceId); }

	virtual UINT32 getInternalItem(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
};

/**
 * Access point class
 */
class NXCORE_EXPORTABLE AccessPoint : public DataCollectionTarget
{
protected:
	UINT32 m_nodeId;
	BYTE m_macAddr[MAC_ADDR_LENGTH];
	TCHAR *m_vendor;
	TCHAR *m_model;
	TCHAR *m_serialNumber;
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, BYTE *macAddr);
   virtual ~AccessPoint();

   virtual int Type() { return OBJECT_ACCESSPOINT; }

   virtual BOOL CreateFromDB(UINT32 dwId);
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);

	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	BYTE *getMacAddr() { return m_macAddr; }
	bool isMyRadio(int rfIndex);
	void getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize);

	void attachToNode(UINT32 nodeId);
	void updateRadioInterfaces(ObjectArray<RadioInterfaceInfo> *ri);
	void updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber);
};

/**
 * Cluster class
 */
class NXCORE_EXPORTABLE Cluster : public DataCollectionTarget
{
protected:
	UINT32 m_dwClusterType;
   UINT32 m_dwNumSyncNets;
   IP_NETWORK *m_pSyncNetList;
	UINT32 m_dwNumResources;
	CLUSTER_RESOURCE *m_pResourceList;
	UINT32 m_dwFlags;
	time_t m_tmLastPoll;
	UINT32 m_zoneId;

public:
	Cluster();
   Cluster(const TCHAR *pszName, UINT32 zoneId);
	virtual ~Cluster();

   virtual int Type() { return OBJECT_CLUSTER; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual void unbindFromTemplate(UINT32 dwTemplateId, BOOL bRemoveDCI);

	bool isSyncAddr(UINT32 dwAddr);
	bool isVirtualAddr(UINT32 dwAddr);
	bool isResourceOnNode(UINT32 dwResource, UINT32 dwNode);
   UINT32 getZoneId() { return m_zoneId; }

   void statusPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller);
   void lockForStatusPoll() { m_dwFlags |= CLF_QUEUED_FOR_STATUS_POLL; }
   bool isReadyForStatusPoll()
   {
      return ((m_iStatus != STATUS_UNMANAGED) && (!m_isDeleted) &&
              (!(m_dwFlags & CLF_QUEUED_FOR_STATUS_POLL)) &&
              ((UINT32)time(NULL) - (UINT32)m_tmLastPoll > g_dwStatusPollingInterval))
                  ? true : false;
   }

   UINT32 collectAggregatedData(DCItem *item, TCHAR *buffer);
   UINT32 collectAggregatedData(DCTable *table, Table **result);
};

class Subnet;

/**
 * Node
 */
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
	friend class Subnet;

protected:
	TCHAR m_primaryName[MAX_DNS_NAME];
   UINT32 m_dwFlags;
   UINT32 m_dwDynamicFlags;       // Flags used at runtime by server
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;
   UINT32 m_zoneId;
   WORD m_wAgentPort;
   WORD m_wAuthMethod;
   TCHAR m_szSharedSecret[MAX_SECRET_LENGTH];
   int m_iStatusPollType;
   int m_snmpVersion;
   WORD m_wSNMPPort;
	WORD m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
   TCHAR m_szObjectId[MAX_OID_LEN * 4];
   TCHAR m_szAgentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_szPlatformName[MAX_PLATFORM_NAME_LEN];
	TCHAR *m_sysDescription;  // Agent's System.Uname or SNMP sysDescr
	TCHAR *m_sysName;				// SNMP sysName
	TCHAR *m_lldpNodeId;			// lldpLocChassisId combined with lldpLocChassisIdSubtype, or NULL for non-LLDP nodes
	ObjectArray<LLDP_LOCAL_PORT_INFO> *m_lldpLocalPortInfo;
	NetworkDeviceDriver *m_driver;
	DriverData *m_driverData;
   ObjectArray<AgentParameterDefinition> *m_paramList; // List of supported parameters
   ObjectArray<AgentTableDefinition> *m_tableList; // List of supported tables
   time_t m_lastDiscoveryPoll;
   time_t m_lastStatusPoll;
   time_t m_lastConfigurationPoll;
	time_t m_lastTopologyPoll;
   time_t m_lastRTUpdate;
   time_t m_failTimeSNMP;
   time_t m_failTimeAgent;
	time_t m_downSince;
   time_t m_bootTime;
   time_t m_agentUpTime;
   MUTEX m_hPollerMutex;
   MUTEX m_hAgentAccessMutex;
   MUTEX m_hSmclpAccessMutex;
   MUTEX m_mutexRTAccess;
	MUTEX m_mutexTopoAccess;
   AgentConnectionEx *m_pAgentConnection;
   SMCLP_Connection *m_smclpConnection;
	QWORD m_lastAgentTrapId;	     // ID of last received agent trap
   QWORD m_lastAgentPushRequestId; // ID of last received agent push request
   UINT32 m_dwPollerNode;      // Node used for network service polling
   UINT32 m_dwProxyNode;       // Node used as proxy for agent connection
	UINT32 m_dwSNMPProxy;			// Node used as proxy for SNMP requests
   QWORD m_qwLastEvents[MAX_LAST_EVENTS];
   ROUTING_TABLE *m_pRoutingTable;
	ForwardingDatabase *m_fdb;
	LinkLayerNeighbors *m_linkLayerNeighbors;
	VlanList *m_vlans;
	VrrpInfo *m_vrrpInfo;
	ObjectArray<WirelessStationInfo> *m_wirelessStations;
	int m_adoptedApCount;
	int m_totalApCount;
	BYTE m_baseBridgeAddress[MAC_ADDR_LENGTH];	// Bridge base address (dot1dBaseBridgeAddress in bridge MIB)
	nxmap_ObjList *m_pTopology;
	time_t m_topologyRebuildTimestamp;
	ServerJobQueue *m_jobQueue;
	ComponentTree *m_components;		// Hardware components
	ObjectArray<SoftwarePackage> *m_softwarePackages;  // installed software packages
	ObjectArray<WinPerfObject> *m_winPerfObjects;  // Windows performance objects

   void pollerLock() { MutexLock(m_hPollerMutex); }
   void pollerUnlock() { MutexUnlock(m_hPollerMutex); }

   void agentLock() { MutexLock(m_hAgentAccessMutex); }
   void agentUnlock() { MutexUnlock(m_hAgentAccessMutex); }

   void smclpLock() { MutexLock(m_hSmclpAccessMutex); }
   void smclpUnlock() { MutexUnlock(m_hSmclpAccessMutex); }

   void routingTableLock() { MutexLock(m_mutexRTAccess); }
   void routingTableUnlock() { MutexUnlock(m_mutexRTAccess); }

   BOOL checkSNMPIntegerValue(SNMP_Transport *pTransport, const TCHAR *pszOID, int nValue);
   void checkOSPFSupport(SNMP_Transport *pTransport);
	void addVrrpInterfaces(InterfaceList *ifList);
	BOOL resolveName(BOOL useOnlyDNS);
   void setAgentProxy(AgentConnection *pConn);
	void setPrimaryIPAddress(UINT32 addr);

   UINT32 getInterfaceCount(Interface **ppInterface);

   void checkInterfaceNames(InterfaceList *pIfList);
   Subnet *createSubnet(DWORD ipAddr, DWORD netMask, bool syntheticMask);
	void checkAgentPolicyBinding(AgentConnection *conn);
	void updatePrimaryIpAddr();
	bool confPollAgent(UINT32 dwRqId);
	bool confPollSnmp(UINT32 dwRqId);
	void checkBridgeMib(SNMP_Transport *pTransport);
	void checkIfXTable(SNMP_Transport *pTransport);
	void executeHookScript(const TCHAR *hookName);
	bool checkNetworkPath(UINT32 dwRqId);
	bool checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 dwRqId);

	void applyUserTemplates();
	void doInstanceDiscovery();
	StringList *getInstanceList(DCItem *dci);
	void updateInstances(DCItem *root, StringList *instances);

	void updateContainerMembership();
	BOOL updateInterfaceConfiguration(UINT32 dwRqId, UINT32 dwNetMask);

	void buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedSubnet, bool includeEndNodes);

	virtual bool isDataCollectionDisabled();

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 dwObjectId);

public:
   Node();
   Node(UINT32 dwAddr, UINT32 dwFlags, UINT32 dwProxyNode, UINT32 dwSNMPProxy, UINT32 dwZone);
   virtual ~Node();

   virtual int Type() { return OBJECT_NODE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

	TCHAR *expandText(const TCHAR *textTemplate);

	Cluster *getMyCluster();

   UINT32 getZoneId() { return m_zoneId; }
   UINT32 getFlags() { return m_dwFlags; }
   UINT32 getRuntimeFlags() { return m_dwDynamicFlags; }
   void setFlag(UINT32 flag) { LockData(); m_dwFlags |= flag; Modify(); UnlockData(); }
   void clearFlag(UINT32 flag) { LockData(); m_dwFlags &= ~flag; Modify(); UnlockData(); }
   void setLocalMgmtFlag() { m_dwFlags |= NF_IS_LOCAL_MGMT; }
   void clearLocalMgmtFlag() { m_dwFlags &= ~NF_IS_LOCAL_MGMT; }

   bool isSNMPSupported() { return m_dwFlags & NF_IS_SNMP ? true : false; }
   bool isNativeAgent() { return m_dwFlags & NF_IS_NATIVE_AGENT ? true : false; }
   bool isBridge() { return m_dwFlags & NF_IS_BRIDGE ? true : false; }
   bool isRouter() { return m_dwFlags & NF_IS_ROUTER ? true : false; }
   bool isLocalManagement() { return m_dwFlags & NF_IS_LOCAL_MGMT ? true : false; }
	bool isPerVlanFdbSupported() { return (m_driver != NULL) ? m_driver->isPerVlanFdbSupported() : false; }
	bool isWirelessController() { return m_dwFlags & NF_IS_WIFI_CONTROLLER ? true : false; }

	LONG getSNMPVersion() { return m_snmpVersion; }
	const TCHAR *getSNMPObjectId() { return m_szObjectId; }
	const TCHAR *getAgentVersion() { return m_szAgentVersion; }
	const TCHAR *getPlatformName() { return m_szPlatformName; }
   const TCHAR *getObjectId() { return m_szObjectId; }
	const TCHAR *getSysName() { return CHECK_NULL_EX(m_sysName); }
	const TCHAR *getSysDescription() { return CHECK_NULL_EX(m_sysDescription); }
   time_t getBootTime() { return m_bootTime; }
	const TCHAR *getLLDPNodeId() { return m_lldpNodeId; }
	const TCHAR *getDriverName() { return (m_driver != NULL) ? m_driver->getName() : _T("GENERIC"); }
	WORD getAgentPort() { return m_wAgentPort; }
	WORD getAuthMethod() { return m_wAuthMethod; }
	const TCHAR *getSharedSecret() { return m_szSharedSecret; }

   BOOL isDown() { return m_dwDynamicFlags & NDF_UNREACHABLE ? TRUE : FALSE; }
	time_t getDownTime() const { return m_downSince; }

   void addInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   Interface *createNewInterface(UINT32 dwAddr, UINT32 dwNetMask, const TCHAR *name = NULL, const TCHAR *descr = NULL,
                                 UINT32 dwIndex = 0, UINT32 dwType = 0, BYTE *pbMacAddr = NULL, UINT32 bridgePort = 0,
											UINT32 slot = 0, UINT32 port = 0, bool physPort = false, bool manuallyCreated = false,
                                 bool system = false);
   void deleteInterface(Interface *pInterface);

	void setPrimaryName(const TCHAR *name) { nx_strncpy(m_primaryName, name, MAX_DNS_NAME); }
	void setAgentPort(WORD port) { m_wAgentPort = port; }
	void setSnmpPort(WORD port) { m_wSNMPPort = port; }
   void changeIPAddress(UINT32 dwIpAddr);
	void changeZone(UINT32 newZone);

   ARP_CACHE *getArpCache();
   InterfaceList *getInterfaceList();
   Interface *findInterface(UINT32 dwIndex, UINT32 dwHostAddr);
   Interface *findInterface(const TCHAR *name);
	Interface *findInterfaceByMAC(const BYTE *macAddr);
	Interface *findInterfaceByIP(UINT32 ipAddr);
	Interface *findInterfaceBySlotAndPort(UINT32 slot, UINT32 port);
	Interface *findBridgePort(UINT32 bridgePortNumber);
	BOOL isMyIP(UINT32 dwIpAddr);
   void getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 dwIndex, int *adminState, int *operState);
   void getInterfaceStatusFromAgent(UINT32 dwIndex, int *adminState, int *operState);
   ROUTING_TABLE *getRoutingTable();
   ROUTING_TABLE *getCachedRoutingTable() { return m_pRoutingTable; }
	LinkLayerNeighbors *getLinkLayerNeighbors();
	VlanList *getVlans();
   BOOL getNextHop(UINT32 dwSrcAddr, UINT32 dwDestAddr, UINT32 *pdwNextHop,
                   UINT32 *pdwIfIndex, BOOL *pbIsVPN);
	ComponentTree *getComponents();
   bool getLldpLocalPortInfo(BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port);

	void setRecheckCapsFlag() { m_dwDynamicFlags |= NDF_RECHECK_CAPABILITIES; }
   void setDiscoveryPollTimeStamp();
   void statusPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller);
   void configurationPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller, UINT32 dwNetMask);
	void topologyPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller);
	void resolveVlanPorts(VlanList *vlanList);
	void updateInterfaceNames(ClientSession *pSession, UINT32 dwRqId);
   void updateRoutingTable();
	void checkSubnetBinding(InterfaceList *pIfList);

   bool isReadyForStatusPoll();
   bool isReadyForConfigurationPoll();
   bool isReadyForDiscoveryPoll();
   bool isReadyForRoutePoll();
   bool isReadyForTopologyPoll();

   void lockForStatusPoll();
   void lockForConfigurationPoll();
   void lockForDiscoveryPoll();
   void lockForRoutePoll();
   void lockForTopologyPoll();
	void forceConfigurationPoll() { m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL; }

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   BOOL connectToAgent(UINT32 *error = NULL, UINT32 *socketError = NULL);
	bool checkAgentTrapId(QWORD id);
   bool checkAgentPushRequestId(QWORD id);

   bool connectToSMCLP();

	virtual UINT32 getInternalItem(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
   UINT32 getItemFromSNMP(WORD port, const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer, int interpretRawValue);
	UINT32 getTableFromSNMP(WORD port, const TCHAR *oid, ObjectArray<DCTableColumn> *columns, Table **table);
   UINT32 getListFromSNMP(WORD port, const TCHAR *oid, StringList **list);
   UINT32 getOIDSuffixListFromSNMP(WORD port, const TCHAR *oid, StringList **list);
   UINT32 getItemFromCheckPointSNMP(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
   UINT32 getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
	UINT32 getTableFromAgent(const TCHAR *name, Table **table);
	UINT32 getListFromAgent(const TCHAR *name, StringList **list);
   UINT32 getItemForClient(int iOrigin, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize);
   UINT32 getTableForClient(const TCHAR *name, Table **table);
   UINT32 getItemFromSMCLP(const TCHAR *param, UINT32 bufSize, TCHAR *buffer);

	virtual NXSL_Array *getParentsForNXSL();
	NXSL_Array *getInterfacesForNXSL();

   void openParamList(ObjectArray<AgentParameterDefinition> **paramList);
   void closeParamList() { UnlockData(); }

   void openTableList(ObjectArray<AgentTableDefinition> **tableList);
   void closeTableList() { UnlockData(); }

   AgentConnectionEx *createAgentConnection();
	SNMP_Transport *createSnmpTransport(WORD port = 0, const TCHAR *context = NULL);
	SNMP_SecurityContext *getSnmpSecurityContext();

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
   void writeParamListToMessage(CSCPMessage *pMsg, WORD flags);
	void writeWinPerfObjectsToMessage(CSCPMessage *msg);
	void writePackageListToMessage(CSCPMessage *msg);
	void writeWsListToMessage(CSCPMessage *msg);

   UINT32 wakeUp();

   void addService(NetworkService *pNetSrv) { AddChild(pNetSrv); pNetSrv->AddParent(this); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, UINT32 dwIpAddr, int iServiceType, WORD wPort = 0,
                             WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL);

   QWORD getLastEventId(int nIndex) { return ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) ? m_qwLastEvents[nIndex] : 0; }
   void setLastEventId(int nIndex, QWORD qwId) { if ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) m_qwLastEvents[nIndex] = qwId; }

   UINT32 callSnmpEnumerate(const TCHAR *pszRootOid,
      UINT32 (* pHandler)(UINT32, SNMP_Variable *, SNMP_Transport *, void *), void *pArg, const TCHAR *context = NULL);

	nxmap_ObjList *getL2Topology();
	nxmap_ObjList *buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes);
	ForwardingDatabase *getSwitchForwardingDatabase();
	Interface *findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, bool *exactMatch);
	void addHostConnections(LinkLayerNeighbors *nbs);
	void addExistingConnections(LinkLayerNeighbors *nbs);

	nxmap_ObjList *buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes);

	ServerJobQueue *getJobQueue() { return m_jobQueue; }
	int getJobCount(const TCHAR *type = NULL) { return m_jobQueue->getJobCount(type); }

	DriverData *getDriverData() { return m_driverData; }
	void setDriverData(DriverData *data) { m_driverData = data; }
};

/**
 * Set timestamp of last discovery poll to current time
 */
inline void Node::setDiscoveryPollTimeStamp()
{
   m_lastDiscoveryPoll = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_DISCOVERY_POLL;
}

inline bool Node::isReadyForStatusPoll()
{
	if (m_isDeleted)
		return false;
	//if (GetMyCluster() != NULL)
	//	return FALSE;	// Cluster nodes should be polled from cluster status poll
   if (m_dwDynamicFlags & NDF_FORCE_STATUS_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_STATUS_POLL;
      return true;
   }
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_STATUS_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_STATUS_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
			 (getMyCluster() == NULL) &&
          ((UINT32)time(NULL) - (UINT32)m_lastStatusPoll > g_dwStatusPollingInterval);
}

inline bool Node::isReadyForConfigurationPoll()
{
	if (m_isDeleted)
		return false;
   if (m_dwDynamicFlags & NDF_FORCE_CONFIGURATION_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_CONFIGURATION_POLL;
      return true;
   }
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_CONF_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_CONFIG_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          ((UINT32)time(NULL) - (UINT32)m_lastConfigurationPoll > g_dwConfigurationPollingInterval);
}

inline bool Node::isReadyForDiscoveryPoll()
{
	if (m_isDeleted)
		return false;
   return (g_dwFlags & AF_ENABLE_NETWORK_DISCOVERY) &&
          (m_iStatus != STATUS_UNMANAGED) &&
			 (!(m_dwFlags & NF_DISABLE_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)time(NULL) - (UINT32)m_lastDiscoveryPoll > g_dwDiscoveryPollingInterval);
}

inline bool Node::isReadyForRoutePoll()
{
	if (m_isDeleted)
		return false;
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)time(NULL) - (UINT32)m_lastRTUpdate > g_dwRoutingTableUpdateInterval);
}

inline bool Node::isReadyForTopologyPoll()
{
	if (m_isDeleted)
		return false;
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)time(NULL) - (UINT32)m_lastTopologyPoll > g_dwTopologyPollingInterval);
}

inline void Node::lockForStatusPoll()
{
   LockData();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_STATUS_POLL;
   UnlockData();
}

inline void Node::lockForConfigurationPoll()
{
   LockData();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_CONFIG_POLL;
   UnlockData();
}

inline void Node::lockForDiscoveryPoll()
{
   LockData();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_DISCOVERY_POLL;
   UnlockData();
}

inline void Node::lockForTopologyPoll()
{
   LockData();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_TOPOLOGY_POLL;
   UnlockData();
}

inline void Node::lockForRoutePoll()
{
   LockData();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_ROUTE_POLL;
   UnlockData();
}

/**
 * Subnet
 */
class NXCORE_EXPORTABLE Subnet : public NetObj
{
	friend void Node::buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedSubnet, bool includeEndNodes);

protected:
   UINT32 m_dwIpNetMask;
   UINT32 m_zoneId;
	bool m_bSyntheticMask;

   virtual void prepareForDeletion();

   void buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes);

public:
   Subnet();
   Subnet(UINT32 dwAddr, UINT32 dwNetMask, UINT32 dwZone, bool bSyntheticMask);
   virtual ~Subnet();

   virtual int Type() { return OBJECT_SUBNET; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   void AddNode(Node *pNode) { AddChild(pNode); pNode->AddParent(this); }
   virtual void CreateMessage(CSCPMessage *pMsg);

	virtual bool showThresholdSummary();

   UINT32 getIpNetMask() { return m_dwIpNetMask; }
   UINT32 getZoneId() { return m_zoneId; }
	bool isSyntheticMask() { return m_bSyntheticMask; }

	void setCorrectMask(UINT32 dwAddr, UINT32 dwMask);

	bool findMacAddress(UINT32 ipAddr, BYTE *macAddr);

   UINT32 *buildAddressMap(int *length);
};

/**
 * Universal root object
 */
class NXCORE_EXPORTABLE UniversalRoot : public NetObj
{
public:
   UniversalRoot();
   virtual ~UniversalRoot();

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual void LoadFromDB();

   void LinkChildObjects();
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }
};

/**
 * Service root
 */
class NXCORE_EXPORTABLE ServiceRoot : public UniversalRoot
{
public:
   ServiceRoot();
   virtual ~ServiceRoot();

   virtual int Type() { return OBJECT_SERVICEROOT; }

	virtual bool showThresholdSummary();
};

/**
 * Template root
 */
class NXCORE_EXPORTABLE TemplateRoot : public UniversalRoot
{
public:
   TemplateRoot();
   virtual ~TemplateRoot();

   virtual int Type() { return OBJECT_TEMPLATEROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};

/**
 * Generic container object
 */
class NXCORE_EXPORTABLE Container : public NetObj
{
private:
   UINT32 *m_pdwChildIdList;
   UINT32 m_dwChildIdListSize;

protected:
	UINT32 m_flags;
   UINT32 m_dwCategory;
	NXSL_VM *m_bindFilter;
	TCHAR *m_bindFilterSource;

public:
   Container();
   Container(const TCHAR *pszName, UINT32 dwCategory);
   virtual ~Container();

   virtual int Type(void) { return OBJECT_CONTAINER; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool showThresholdSummary();

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   UINT32 getCategory() { return m_dwCategory; }

   void linkChildObjects();
   void linkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }

	bool isSuitableForNode(Node *node);
	bool isAutoBindEnabled() { return (m_flags & CF_AUTO_BIND) ? true : false; }
	bool isAutoUnbindEnabled() { return ((m_flags & (CF_AUTO_BIND | CF_AUTO_UNBIND)) == (CF_AUTO_BIND | CF_AUTO_UNBIND)) ? true : false; }

	void setAutoBindFilter(const TCHAR *script);
};

/**
 * Template group object
 */
class NXCORE_EXPORTABLE TemplateGroup : public Container
{
public:
   TemplateGroup() : Container() { }
   TemplateGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~TemplateGroup() { }

   virtual int Type() { return OBJECT_TEMPLATEGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool showThresholdSummary();
};

/**
 * Rack object
 */
class NXCORE_EXPORTABLE Rack : public Container
{
protected:
	int m_height;	// Rack height in units

public:
   Rack();
   Rack(const TCHAR *name, int height);
   virtual ~Rack();

   virtual int Type() { return OBJECT_RACK; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};

/**
 * Zone object
 */
class Zone : public NetObj
{
protected:
   UINT32 m_zoneId;
   UINT32 m_agentProxy;
   UINT32 m_snmpProxy;
	UINT32 m_icmpProxy;
	ObjectIndex *m_idxNodeByAddr;
	ObjectIndex *m_idxInterfaceByAddr;
	ObjectIndex *m_idxSubnetByAddr;

public:
   Zone();
   Zone(UINT32 zoneId, const TCHAR *name);
   virtual ~Zone();

   virtual int Type() { return OBJECT_ZONE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool showThresholdSummary();

   UINT32 getZoneId() { return m_zoneId; }
	UINT32 getAgentProxy() { return m_agentProxy; }
	UINT32 getSnmpProxy() { return m_snmpProxy; }
	UINT32 getIcmpProxy() { return m_icmpProxy; }

   void addSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }

	void addToIndex(Subnet *subnet) { m_idxSubnetByAddr->put(subnet->IpAddr(), subnet); }
	void addToIndex(Interface *iface) { m_idxInterfaceByAddr->put(iface->IpAddr(), iface); }
	void addToIndex(Node *node) { m_idxNodeByAddr->put(node->IpAddr(), node); }
	void removeFromIndex(Subnet *subnet) { m_idxSubnetByAddr->remove(subnet->IpAddr()); }
	void removeFromIndex(Interface *iface) { m_idxInterfaceByAddr->remove(iface->IpAddr()); }
	void removeFromIndex(Node *node) { m_idxNodeByAddr->remove(node->IpAddr()); }
	void updateInterfaceIndex(UINT32 oldIp, UINT32 newIp, Interface *iface);
	Subnet *getSubnetByAddr(UINT32 ipAddr) { return (Subnet *)m_idxSubnetByAddr->get(ipAddr); }
	Interface *getInterfaceByAddr(UINT32 ipAddr) { return (Interface *)m_idxInterfaceByAddr->get(ipAddr); }
	Node *getNodeByAddr(UINT32 ipAddr) { return (Node *)m_idxNodeByAddr->get(ipAddr); }
	Subnet *findSubnet(bool (*comparator)(NetObj *, void *), void *data) { return (Subnet *)m_idxSubnetByAddr->find(comparator, data); }
	Interface *findInterface(bool (*comparator)(NetObj *, void *), void *data) { return (Interface *)m_idxInterfaceByAddr->find(comparator, data); }
	Node *findNode(bool (*comparator)(NetObj *, void *), void *data) { return (Node *)m_idxNodeByAddr->find(comparator, data); }
   void forEachSubnet(void (*callback)(NetObj *, void *), void *data) { m_idxSubnetByAddr->forEach(callback, data); }
};

/**
 * Entire network
 */
class NXCORE_EXPORTABLE Network : public NetObj
{
public:
   Network();
   virtual ~Network();

   virtual int Type(void) { return OBJECT_NETWORK; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);

	virtual bool showThresholdSummary();

   void AddSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }
   void AddZone(Zone *pZone) { AddChild(pZone); pZone->AddParent(this); }
   void LoadFromDB(void);
};

/**
 * Condition
 */
class NXCORE_EXPORTABLE Condition : public NetObj
{
protected:
   UINT32 m_dciCount;
   INPUT_DCI *m_dciList;
   TCHAR *m_scriptSource;
   NXSL_VM *m_script;
   UINT32 m_activationEventCode;
   UINT32 m_deactivationEventCode;
   UINT32 m_sourceObject;
   int m_activeStatus;
   int m_inactiveStatus;
   bool m_isActive;
   time_t m_lastPoll;
   bool m_queuedForPolling;

public:
   Condition();
   Condition(bool hidden);
   virtual ~Condition();

   virtual int Type() { return OBJECT_CONDITION; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   void check();

   void lockForPoll();
   void endPoll();

   bool isReadyForPoll()
   {
      return ((m_iStatus != STATUS_UNMANAGED) &&
              (!m_queuedForPolling) && (!m_isDeleted) &&
              ((UINT32)time(NULL) - (UINT32)m_lastPoll > g_dwConditionPollingInterval));
   }

   int getCacheSizeForDCI(UINT32 itemId, bool noLock);
};

/**
 * Generic agent policy object
 */
class NXCORE_EXPORTABLE AgentPolicy : public NetObj
{
protected:
	UINT32 m_version;
	int m_policyType;
	TCHAR *m_description;

	BOOL savePolicyCommonProperties(DB_HANDLE hdb);

public:
   AgentPolicy(int type);
   AgentPolicy(const TCHAR *name, int type);
   virtual ~AgentPolicy();

   virtual int Type() { return OBJECT_AGENTPOLICY; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool createDeploymentMessage(CSCPMessage *msg);
	virtual bool createUninstallMessage(CSCPMessage *msg);

	void linkNode(Node *node) { AddChild(node); node->AddParent(this); }
	void unlinkNode(Node *node) { DeleteChild(node); node->DeleteParent(this); }
};

/**
 * Agent config policy object
 */
class NXCORE_EXPORTABLE AgentPolicyConfig : public AgentPolicy
{
protected:
	TCHAR *m_fileContent;

public:
   AgentPolicyConfig();
   AgentPolicyConfig(const TCHAR *name);
   virtual ~AgentPolicyConfig();

   virtual int Type() { return OBJECT_AGENTPOLICY_CONFIG; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool createDeploymentMessage(CSCPMessage *msg);
	virtual bool createUninstallMessage(CSCPMessage *msg);
};

/**
 * Policy group object
 */
class NXCORE_EXPORTABLE PolicyGroup : public Container
{
public:
   PolicyGroup() : Container() { }
   PolicyGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~PolicyGroup() { }

   virtual int Type() { return OBJECT_POLICYGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool showThresholdSummary();
};

/**
 * Policy root
 */
class NXCORE_EXPORTABLE PolicyRoot : public UniversalRoot
{
public:
   PolicyRoot();
   virtual ~PolicyRoot();

   virtual int Type() { return OBJECT_POLICYROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};

/**
 * Network map root
 */
class NXCORE_EXPORTABLE NetworkMapRoot : public UniversalRoot
{
public:
   NetworkMapRoot();
   virtual ~NetworkMapRoot();

   virtual int Type() { return OBJECT_NETWORKMAPROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};

/**
 * Network map group object
 */
class NXCORE_EXPORTABLE NetworkMapGroup : public Container
{
public:
   NetworkMapGroup() : Container() { }
   NetworkMapGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~NetworkMapGroup() { }

   virtual int Type() { return OBJECT_NETWORKMAPGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool showThresholdSummary();
};

/**
 * Network map object
 */
class NXCORE_EXPORTABLE NetworkMap : public NetObj
{
protected:
	int m_mapType;
	UINT32 m_seedObject;
	int m_discoveryRadius;
	int m_layout;
	UINT32 m_flags;
	int m_backgroundColor;
	int m_defaultLinkColor;
	int m_defaultLinkRouting;
	uuid_t m_background;
	double m_backgroundLatitude;
	double m_backgroundLongitude;
	int m_backgroundZoom;
	UINT32 m_nextElementId;
	ObjectArray<NetworkMapElement> *m_elements;
	ObjectArray<NetworkMapLink> *m_links;
	TCHAR *m_filterSource;
	NXSL_VM *m_filter;

	void updateObjects(nxmap_ObjList *objects);
	UINT32 objectIdFromElementId(UINT32 eid);
	UINT32 elementIdFromObjectId(UINT32 eid);

public:
   NetworkMap();
	NetworkMap(int type, UINT32 seed);
   virtual ~NetworkMap();

   virtual int Type() { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void onObjectDelete(UINT32 dwObjectId);

   void updateContent();

   int getBackgroundColor() { return m_backgroundColor; }
   void setBackgroundColor(int color) { m_backgroundColor = color; }

	void setFilter(const TCHAR *filter);
   bool isAllowedOnMap(NetObj *object);
};

/**
 * Dashboard tree root
 */
class NXCORE_EXPORTABLE DashboardRoot : public UniversalRoot
{
public:
   DashboardRoot();
   virtual ~DashboardRoot();

   virtual int Type() { return OBJECT_DASHBOARDROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};

/**
 * Dashboard element
 */
class DashboardElement
{
public:
	int m_type;
	TCHAR *m_data;
	TCHAR *m_layout;

	DashboardElement() { m_data = NULL; m_layout = NULL; }
	~DashboardElement() { safe_free(m_data); safe_free(m_layout); }
};

/**
 * Dashboard object
 */
class NXCORE_EXPORTABLE Dashboard : public Container
{
protected:
	int m_numColumns;
	UINT32 m_options;
	ObjectArray<DashboardElement> *m_elements;

public:
   Dashboard();
   Dashboard(const TCHAR *name);
   virtual ~Dashboard();

   virtual int Type() { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool showThresholdSummary();
};

/**
 * Report root
 */
class NXCORE_EXPORTABLE ReportRoot : public UniversalRoot
{
public:
   ReportRoot();
   virtual ~ReportRoot();

   virtual int Type() { return OBJECT_REPORTROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};

/**
 * Report group object
 */
class NXCORE_EXPORTABLE ReportGroup : public Container
{
public:
   ReportGroup() : Container() { }
   ReportGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~ReportGroup() { }

   virtual int Type() { return OBJECT_REPORTGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool showThresholdSummary();
};

/**
 * Report object
 */
class NXCORE_EXPORTABLE Report : public NetObj
{
protected:
	TCHAR *m_definition;

public:
   Report();
   Report(const TCHAR *name);
   virtual ~Report();

   virtual int Type() { return OBJECT_REPORT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual bool deleteFromDB(DB_HANDLE hdb);
   virtual BOOL CreateFromDB(UINT32 dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	const TCHAR *getDefinition() { return CHECK_NULL_EX(m_definition); }

	UINT32 execute(StringMap *parameters, UINT32 userId);
};

/**
 * SLM check object
 */
class NXCORE_EXPORTABLE SlmCheck : public NetObj
{
protected:
	static NXSL_VariableSystem m_nxslConstants;

	Threshold *m_threshold;
	enum CheckType { check_undefined = 0, check_script = 1, check_threshold = 2 } m_type;
	TCHAR *m_script;
	NXSL_VM *m_pCompiledScript;
	TCHAR m_reason[256];
	bool m_isTemplate;
	UINT32 m_templateId;
	UINT32 m_currentTicketId;

   virtual void onObjectDelete(UINT32 objectId);

	void setScript(const TCHAR *script);
	UINT32 getOwnerId();
	NXSL_Value *getNodeObjectForNXSL();
	bool insertTicket();
	void closeTicket();
	void setReason(const TCHAR *reason) { nx_strncpy(m_reason, CHECK_NULL_EX(reason), 256); }
	void compileScript();

public:
	SlmCheck();
	SlmCheck(const TCHAR *name, bool isTemplate);
	SlmCheck(SlmCheck *tmpl);
	virtual ~SlmCheck();

	static void init();

	virtual int Type() { return OBJECT_SLMCHECK; }

	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual bool deleteFromDB(DB_HANDLE hdb);
	virtual BOOL CreateFromDB(UINT32 dwId);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
	virtual void postModify();

	void execute();
	void updateFromTemplate(SlmCheck *tmpl);

	bool isTemplate() { return m_isTemplate; }
	UINT32 getTemplateId() { return m_templateId; }
	const TCHAR *getReason() { return m_reason; }
};

/**
 * Service container - common logic for BusinessService, NodeLink and BusinessServiceRoot
 */
class NXCORE_EXPORTABLE ServiceContainer : public Container
{
	enum Period { DAY, WEEK, MONTH };

protected:
	time_t m_prevUptimeUpdateTime;
	int m_prevUptimeUpdateStatus;
	double m_uptimeDay;
	double m_uptimeWeek;
	double m_uptimeMonth;
	INT32 m_downtimeDay;
	INT32 m_downtimeWeek;
	INT32 m_downtimeMonth;
	INT32 m_prevDiffDay;
	INT32 m_prevDiffWeek;
	INT32 m_prevDiffMonth;

	static INT32 logRecordId;
	static INT32 getSecondsInMonth();
	static INT32 getSecondsInPeriod(Period period) { return period == MONTH ? getSecondsInMonth() : (period == WEEK ? (3600 * 24 * 7) : (3600 * 24)); }
	static INT32 getSecondsSinceBeginningOf(Period period, time_t *beginTime = NULL);

	void initServiceContainer();
	BOOL addHistoryRecord();
	double getUptimeFromDBFor(Period period, INT32 *downtime);

public:
	ServiceContainer();
	ServiceContainer(const TCHAR *pszName);

	virtual BOOL CreateFromDB(UINT32 dwId);
	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual bool deleteFromDB(DB_HANDLE hdb);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
	virtual void setStatus(int newStatus);

	virtual bool showThresholdSummary();

	void initUptimeStats();
	void updateUptimeStats(time_t currentTime = 0, BOOL updateChilds = FALSE);
};

/**
 * Business service root
 */
class NXCORE_EXPORTABLE BusinessServiceRoot : public ServiceContainer
{
public:
	BusinessServiceRoot();
	virtual ~BusinessServiceRoot();

	virtual int Type() { return OBJECT_BUSINESSSERVICEROOT; }

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   void LoadFromDB();

   void LinkChildObjects();
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }
};


//
// Business service object
//

class NXCORE_EXPORTABLE BusinessService : public ServiceContainer
{
protected:
	bool m_busy;
	time_t m_lastPollTime;
	int m_lastPollStatus;

public:
	BusinessService();
	BusinessService(const TCHAR *name);
	virtual ~BusinessService();

	virtual int Type() { return OBJECT_BUSINESSSERVICE; }

	virtual BOOL CreateFromDB(UINT32 dwId);
	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual bool deleteFromDB(DB_HANDLE hdb);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	bool isReadyForPolling();
	void lockForPolling();
	void poll(ClientSession *pSession, UINT32 dwRqId, int nPoller);

	void getApplicableTemplates(ServiceContainer *target, ObjectArray<SlmCheck> *templates);
};

/**
 * Node link object for business service
 */
class NXCORE_EXPORTABLE NodeLink : public ServiceContainer
{
protected:
	UINT32 m_nodeId;

   virtual void onObjectDelete(UINT32 dwObjectId);

	void applyTemplate(SlmCheck *tmpl);

public:
	NodeLink();
	NodeLink(const TCHAR *name, UINT32 nodeId);
	virtual ~NodeLink();

	virtual int Type() { return OBJECT_NODELINK; }

	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual bool deleteFromDB(DB_HANDLE hdb);
	virtual BOOL CreateFromDB(UINT32 dwId);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual UINT32 ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	void execute();
	void applyTemplates();

	UINT32 getNodeId() { return m_nodeId; }
};


//
// Container category information
//

struct CONTAINER_CATEGORY
{
   UINT32 dwCatId;
   TCHAR szName[MAX_OBJECT_NAME];
   TCHAR *pszDescription;
   UINT32 dwImageId;
};


//
// Functions
//

void ObjectsInit();

void NXCORE_EXPORTABLE NetObjInsert(NetObj *pObject, BOOL bNewObject);
void NetObjDeleteFromIndexes(NetObj *pObject);
void NetObjDelete(NetObj *pObject);

void UpdateInterfaceIndex(UINT32 dwOldIpAddr, UINT32 dwNewIpAddr, Interface *pObject);
ComponentTree *BuildComponentTree(Node *node, SNMP_Transport *snmp);

void NXCORE_EXPORTABLE MacDbAddAccessPoint(AccessPoint *ap);
void NXCORE_EXPORTABLE MacDbAddInterface(Interface *iface);
void NXCORE_EXPORTABLE MacDbAddObject(const BYTE *macAddr, NetObj *object);
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr);
NetObj NXCORE_EXPORTABLE *MacDbFind(const BYTE *macAddr);

NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass);
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(uuid_t guid, int objClass);
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneId, UINT32 ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr);
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId);
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneId, UINT32 ipAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneId, UINT32 ipAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneId, UINT32 dwNodeAddr);
MobileDevice NXCORE_EXPORTABLE *FindMobileDeviceByDeviceID(const TCHAR *deviceId);
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByMAC(const BYTE *macAddr);
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByRadioId(int rfIndex);
UINT32 NXCORE_EXPORTABLE FindLocalMgmtNode();
CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(UINT32 dwId);
Zone NXCORE_EXPORTABLE *FindZoneByGUID(UINT32 dwZoneGUID);
Cluster NXCORE_EXPORTABLE *FindClusterByResourceIP(UINT32 zone, UINT32 ipAddr);
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zone, UINT32 ipAddr);

BOOL LoadObjects();
void DumpObjects(CONSOLE_CTX pCtx);

void DeleteUserFromAllObjects(UINT32 dwUserId);

bool IsValidParentClass(int iChildClass, int iParentClass);
bool IsAgentPolicyObject(NetObj *object);
bool IsEventSource(int objectClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);

/**
 * Global variables
 */
extern Network NXCORE_EXPORTABLE *g_pEntireNet;
extern ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot;
extern TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot;
extern PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot;
extern NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot;
extern DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot;
extern ReportRoot NXCORE_EXPORTABLE *g_pReportRoot;
extern BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot;

extern UINT32 NXCORE_EXPORTABLE g_dwMgmtNode;
extern UINT32 g_dwNumCategories;
extern CONTAINER_CATEGORY *g_pContainerCatList;
extern const TCHAR *g_szClassName[];
extern BOOL g_bModificationsLocked;
extern Queue *g_pTemplateUpdateQueue;

extern ObjectIndex NXCORE_EXPORTABLE g_idxObjectById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxSubnetByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxInterfaceByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxZoneByGUID;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxClusterById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxMobileDeviceById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxAccessPointById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxConditionById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxServiceCheckById;


#endif   /* _nms_objects_h_ */
