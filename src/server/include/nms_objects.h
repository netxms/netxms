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

/**
 * Forward declarations of classes
 */
class AgentConnection;
class AgentConnectionEx;
class ClientSession;
class Queue;
class DataCollectionTarget;

/**
 * Global variables used by inline methods
 */
extern UINT32 g_dwDiscoveryPollingInterval;
extern UINT32 g_dwStatusPollingInterval;
extern UINT32 g_dwConfigurationPollingInterval;
extern UINT32 g_dwRoutingTableUpdateInterval;
extern UINT32 g_dwTopologyPollingInterval;
extern UINT32 g_dwConditionPollingInterval;
extern UINT32 g_instancePollingInterval;
extern INT16 g_defaultAgentCacheMode;

/**
 * Utility functions used by inline methods
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, UINT32 objectId, const TCHAR *query);

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
#define BUILTIN_OID_BUSINESSSERVICEROOT   9

/**
 * "All zones" pseudo-ID
 */
#define ALL_ZONES ((UINT32)0xFFFFFFFF)

/**
 * Node runtime (dynamic) flags
 */
#define NDF_QUEUED_FOR_STATUS_POLL     0x000001
#define NDF_QUEUED_FOR_CONFIG_POLL     0x000002
#define NDF_UNREACHABLE                0x000004
#define NDF_AGENT_UNREACHABLE          0x000008
#define NDF_SNMP_UNREACHABLE           0x000010
#define NDF_QUEUED_FOR_DISCOVERY_POLL  0x000020
#define NDF_FORCE_STATUS_POLL          0x000040
#define NDF_FORCE_CONFIGURATION_POLL   0x000080
#define NDF_QUEUED_FOR_ROUTE_POLL      0x000100
#define NDF_CPSNMP_UNREACHABLE         0x000200
#define NDF_RECHECK_CAPABILITIES       0x000400
#define NDF_POLLING_DISABLED           0x000800
#define NDF_CONFIGURATION_POLL_PASSED  0x001000
#define NDF_QUEUED_FOR_TOPOLOGY_POLL   0x002000
#define NDF_DELETE_IN_PROGRESS         0x004000
#define NDF_NETWORK_PATH_PROBLEM       0x008000
#define NDF_QUEUED_FOR_INSTANCE_POLL   0x010000
#define NDF_CACHE_MODE_NOT_SUPPORTED   0x020000

#define NDF_PERSISTENT (NDF_UNREACHABLE | NDF_NETWORK_PATH_PROBLEM | NDF_AGENT_UNREACHABLE | NDF_SNMP_UNREACHABLE | NDF_CPSNMP_UNREACHABLE | NDF_CACHE_MODE_NOT_SUPPORTED)

#define __NDF_FLAGS_DEFINED

/**
 * Cluster runtime flags
 */
#define CLF_QUEUED_FOR_STATUS_POLL     0x0001
#define CLF_DOWN                       0x0002

/**
 * Poller types
 */
enum PollerType
{
   POLLER_TYPE_STATUS = 0,
   POLLER_TYPE_CONFIGURATION = 1,
   POLLER_TYPE_INSTANCE_DISCOVERY = 2,
   POLLER_TYPE_ROUTING_TABLE = 3,
   POLLER_TYPE_DISCOVERY = 4,
   POLLER_TYPE_BUSINESS_SERVICE = 5,
   POLLER_TYPE_CONDITION = 6,
   POLLER_TYPE_TOPOLOGY = 7
};

/**
 * Poller information
 */
class NXCORE_EXPORTABLE PollerInfo
{
private:
   PollerType m_type;
   NetObj *m_object;
   TCHAR m_status[128];

public:
   PollerInfo(PollerType type, NetObj *object) { m_type = type; m_object = object; _tcscpy(m_status, _T("awaiting execution")); }
   ~PollerInfo();

   const PollerType getType() const { return m_type; }
   NetObj *getObject() const { return m_object; }
   const TCHAR *getStatus() const { return m_status; }

   void startExecution() { _tcscpy(m_status, _T("started")); }
   void setStatus(const TCHAR *status) { nx_strncpy(m_status, status, 128); }
};

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
 * Auto bind/apply decisions
 */
enum AutoBindDecision
{
   AutoBindDecision_Ignore = -1,
   AutoBindDecision_Unbind = 0,
   AutoBindDecision_Bind = 1
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
   int updateType;
   Template *pTemplate;
   UINT32 targetId;
   bool removeDCI;
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

	int size();
	ObjectArray<NetObj> *getObjects(bool updateRefCount, bool (*filter)(NetObj *, void *) = NULL, void *userData = NULL);

	void forEach(void (*callback)(NetObj *, void *), void *data);
};

struct InetAddressIndexEntry;

/**
 * Object index by IP address
 */
class NXCORE_EXPORTABLE InetAddressIndex
{
private:
   InetAddressIndexEntry *m_root;
	RWLOCK m_lock;

public:
   InetAddressIndex();
   ~InetAddressIndex();

	bool put(const InetAddress& addr, NetObj *object);
	bool put(const InetAddressList *addrList, NetObj *object);
	void remove(const InetAddress& addr);
	void remove(const InetAddressList *addrList);
	NetObj *get(const InetAddress& addr);
	NetObj *find(bool (*comparator)(NetObj *, void *), void *data);

	int size();
	ObjectArray<NetObj> *getObjects(bool updateRefCount, bool (*filter)(NetObj *, void *) = NULL, void *userData = NULL);

	void forEach(void (*callback)(const InetAddress&, NetObj *, void *), void *data);
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

	UINT32 fillMessage(NXCPMessage *msg, UINT32 baseId);

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

	void fillMessage(NXCPMessage *msg, UINT32 baseId);
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

	void fillMessage(NXCPMessage *msg, UINT32 baseId);
};

/**
 * Summary table flags
 */
#define SUMMARY_TABLE_MULTI_INSTANCE      0x0001

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

   SummaryTableColumn(NXCPMessage *msg, UINT32 baseId);
   SummaryTableColumn(TCHAR *configStr);

   void createExportRecord(String &xml, int id);
};

/**
 * DCI summary table class
 */
class NXCORE_EXPORTABLE SummaryTable
{
private:
   INT32 m_id;
   uuid m_guid;
   TCHAR m_title[MAX_DB_STRING];
   UINT32 m_flags;
   ObjectArray<SummaryTableColumn> *m_columns;
   TCHAR *m_filterSource;
   NXSL_VM *m_filter;
   AggregationFunction m_aggregationFunction;
   time_t m_periodStart;
   time_t m_periodEnd;
   TCHAR m_menuPath[MAX_DB_STRING];

   SummaryTable(INT32 id, DB_RESULT hResult);

public:
   static SummaryTable *loadFromDB(INT32 id, UINT32 *rcc);

   SummaryTable(NXCPMessage *msg);
   ~SummaryTable();

   bool filter(DataCollectionTarget *node);
   Table *createEmptyResultTable();

   int getNumColumns() { return m_columns->size(); }
   SummaryTableColumn *getColumn(int index) { return m_columns->get(index); }
   AggregationFunction getAggregationFunction() { return m_aggregationFunction; }
   time_t getPeriodStart() { return m_periodStart; }
   time_t getPeriodEnd() { return m_periodEnd; }
   bool isMultiInstance() { return (m_flags & SUMMARY_TABLE_MULTI_INSTANCE) ? true : false; }

   void createExportRecord(String &xml);
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
   UINT32 m_id;
	uuid m_guid;
   UINT32 m_dwTimeStamp;       // Last change time stamp
   UINT32 m_dwRefCount;        // Number of references. Object can be destroyed only when this counter is zero
   TCHAR m_name[MAX_OBJECT_NAME];
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
	uuid m_image;
   MUTEX m_mutexProperties;         // Object data access mutex
   MUTEX m_mutexRefCount;     // Reference counter access mutex
   RWLOCK m_rwlockParentList; // Lock for parent list
   RWLOCK m_rwlockChildList;  // Lock for child list
	GeoLocation m_geoLocation;
   PostalAddress *m_postalAddress;
   ClientSession *m_pollRequestor;
	UINT32 m_submapId;				// Map object which should be open on drill-down request
	IntegerArray<UINT32> *m_dashboards; // Dashboards associated with this object

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
   StringObjectMap<ModuleData> *m_moduleData;

   void lockProperties() { MutexLock(m_mutexProperties); }
   void unlockProperties() { MutexUnlock(m_mutexProperties); }
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

   void setModified();                  // Used to mark object as modified

   bool loadACLFromDB();
   bool saveACLToDB(DB_HANDLE hdb);
   bool loadCommonProperties();
   bool saveCommonProperties(DB_HANDLE hdb);
	bool loadTrustedNodes();
	bool saveTrustedNodes(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query) { return ExecuteQueryOnObject(hdb, m_id, query); }

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 dwObjectId);

   virtual void fillMessageInternal(NXCPMessage *msg);
   virtual void fillMessageInternalStage2(NXCPMessage *msg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *msg);

   void addLocationToHistory();
   bool isLocationTableExists();
   bool createLocationHistoryTable(DB_HANDLE hdb);

public:
   NetObj();
   virtual ~NetObj();

   virtual int getObjectClass() const { return OBJECT_GENERIC; }

   UINT32 getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   int Status() const { return m_iStatus; }
   int getPropagatedStatus();
   UINT32 getTimeStamp() const { return m_dwTimeStamp; }
	const uuid& getGuid() const { return m_guid; }
	const TCHAR *getComments() { return CHECK_NULL_EX(m_pszComments); }
   PostalAddress *getPostalAddress() { return m_postalAddress; }
   void setPostalAddress(PostalAddress * addr) { delete m_postalAddress; m_postalAddress = addr; markAsModified();}

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
   void markAsModified() { lockProperties(); setModified(); unlockProperties(); }  // external API to mar object as modified

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   void setId(UINT32 dwId) { m_id = dwId; setModified(); }
   void generateGuid() { m_guid = uuid::generate(); }
   void setName(const TCHAR *pszName) { nx_strncpy(m_name, pszName, MAX_OBJECT_NAME); setModified(); }
   void resetStatus() { m_iStatus = STATUS_UNKNOWN; setModified(); }
   void setComments(TCHAR *text);	/* text must be dynamically allocated */

   void fillMessage(NXCPMessage *msg);
   UINT32 modifyFromMessage(NXCPMessage *msg);

	virtual void postModify();

   void commentsToMessage(NXCPMessage *pMsg);

   virtual void setMgmtStatus(BOOL bIsManaged);
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   UINT32 getUserRights(UINT32 dwUserId);
   BOOL checkAccessRights(UINT32 dwUserId, UINT32 dwRequiredRights);
   void dropUserAccess(UINT32 dwUserId);

   void addChildNodesToList(ObjectArray<Node> *nodeList, UINT32 dwUserId);
   void addChildDCTargetsToList(ObjectArray<DataCollectionTarget> *dctList, UINT32 dwUserId);

   const TCHAR *getCustomAttribute(const TCHAR *name) { return m_customAttributes.get(name); }
   void setCustomAttribute(const TCHAR *name, const TCHAR *value) { m_customAttributes.set(name, value); setModified(); }
   void setCustomAttributePV(const TCHAR *name, TCHAR *value) { m_customAttributes.setPreallocated(_tcsdup(name), value); setModified(); }
   void deleteCustomAttribute(const TCHAR *name) { m_customAttributes.remove(name); setModified(); }

   ModuleData *getModuleData(const TCHAR *module);
   void setModuleData(const TCHAR *module, ModuleData *data);

	ObjectArray<NetObj> *getParentList(int typeFilter);
	ObjectArray<NetObj> *getChildList(int typeFilter);
	ObjectArray<NetObj> *getFullChildList(bool eventSourceOnly, bool updateRefCount);

   NetObj *findChildObject(const TCHAR *name, int typeFilter);

   int getChildCount() { return (int)m_dwChildCount; }
   int getParentCount() { return (int)m_dwParentCount; }

	virtual NXSL_Array *getParentsForNXSL();
	virtual NXSL_Array *getChildrenForNXSL();

	virtual bool showThresholdSummary();
   virtual bool isEventSource();

   void setStatusCalculation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);
   void setStatusPropagation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);

   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...);

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
   int m_dciLockStatus;
   UINT32 m_dwVersion;
	UINT32 m_flags;
   bool m_dciListModified;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];
	TCHAR *m_applyFilterSource;
	NXSL_VM *m_applyFilter;
	RWLOCK m_dciAccessLock;

   virtual void prepareForDeletion();

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void onDataCollectionChange();

   void loadItemsFromDB();
   void destroyItems();
   void updateInstanceDiscoveryItems(DCItem *dci);

   void lockDciAccess(bool writeLock) { if (writeLock) { RWLockWriteLock(m_dciAccessLock, INFINITE); } else { RWLockReadLock(m_dciAccessLock, INFINITE); } }
	void unlockDciAccess() { RWLockUnlock(m_dciAccessLock); }

   void deleteChildDCIs(UINT32 dcObjectId);
   void destroyItem(DCObject *object, int index);

public:
   Template();
   Template(const TCHAR *pszName);
	Template(ConfigEntry *config);
   virtual ~Template();

   virtual int getObjectClass() const { return OBJECT_TEMPLATE; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   int getVersionMajor() { return m_dwVersion >> 16; }
   int getVersionMinor() { return m_dwVersion & 0xFFFF; }

   int getItemCount() { return m_dcObjects->size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false);
   bool updateDCObject(UINT32 dwItemId, NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId);
   bool deleteDCObject(UINT32 dcObjectId, bool needLock);
   bool setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus);
   int getItemType(UINT32 dwItemId);
   DCObject *getDCObjectById(UINT32 itemId, bool lock = true);
   DCObject *getDCObjectByTemplateId(UINT32 tmplItemId);
   DCObject *getDCObjectByIndex(int index);
   DCObject *getDCObjectByName(const TCHAR *name);
   DCObject *getDCObjectByDescription(const TCHAR *description);
   NXSL_Value *getAllDCObjectsForNXSL(const TCHAR *name, const TCHAR *description);
   bool lockDCIList(int sessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner);
   bool unlockDCIList(int sessionId);
   void setDCIModificationFlag() { m_dciListModified = true; }
   void sendItemsToClient(ClientSession *pSession, UINT32 dwRqId);
   BOOL isLockedBySession(int sessionId) { return m_dciLockStatus == sessionId; }
   UINT32 *getDCIEventsList(UINT32 *pdwCount);
   StringSet *getDCIScriptList();

   BOOL applyToTarget(DataCollectionTarget *pNode);
	AutoBindDecision isApplicable(Node *node);
	bool isAutoApplyEnabled() { return (m_flags & TF_AUTO_APPLY) ? true : false; }
	bool isAutoRemoveEnabled() { return ((m_flags & (TF_AUTO_APPLY | TF_AUTO_REMOVE)) == (TF_AUTO_APPLY | TF_AUTO_REMOVE)) ? true : false; }
	void setAutoApplyFilter(const TCHAR *filter);
   void queueUpdate();
   void queueRemoveFromTarget(UINT32 targetId, bool removeDCI);

   void createNXMPRecord(String &str);

	bool enumDCObjects(bool (* pfCallback)(DCObject *, UINT32, void *), void *pArg);
	void associateItems();

   UINT32 getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects);
};

class Cluster;

/**
 * Interface class
 */
class NXCORE_EXPORTABLE Interface : public NetObj
{
protected:
   UINT32 m_index;
   BYTE m_macAddr[MAC_ADDR_LENGTH];
   InetAddressList m_ipAddressList;
	UINT32 m_flags;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
	TCHAR m_alias[MAX_DB_STRING];	// Interface alias - value of ifAlias for SNMP, empty for NetXMS agent
   UINT32 m_type;
   UINT32 m_mtu;
   UINT64 m_speed;
	UINT32 m_bridgePortNumber;		// 802.1D port number
	UINT32 m_slotNumber;				// Vendor/device specific slot number
	UINT32 m_portNumber;				// Vendor/device specific port number
	UINT32 m_peerNodeId;				// ID of peer node object, or 0 if unknown
	UINT32 m_peerInterfaceId;		// ID of peer interface object, or 0 if unknown
   LinkLayerProtocol m_peerDiscoveryProtocol;  // Protocol used to discover peer node
	WORD m_adminState;				// interface administrative state
	WORD m_operState;					// interface operational state
	WORD m_dot1xPaeAuthState;		// 802.1x port auth state
	WORD m_dot1xBackendAuthState;	// 802.1x backend auth state
   UINT64 m_lastDownEventId;
	int m_pendingStatus;
	int m_pollCount;
	int m_requiredPollCount;
   UINT32 m_zoneId;
   UINT32 m_pingTime;
   time_t m_pingLastTimeStamp;
   int m_ifTableSuffixLen;
   UINT32 *m_ifTableSuffix;

   void icmpStatusPoll(UINT32 rqId, UINT32 nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState);
	void paeStatusPoll(UINT32 rqId, SNMP_Transport *pTransport, Node *node);

protected:
   virtual void onObjectDelete(UINT32 dwObjectId);

	virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Interface();
   Interface(const InetAddressList& addrList, UINT32 zoneId, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, const InetAddressList& addrList, UINT32 ifType, UINT32 zoneId);
   virtual ~Interface();

   virtual int getObjectClass() const { return OBJECT_INTERFACE; }
   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   Node *getParentNode();
   UINT32 getParentNodeId();

   const InetAddressList *getIpAddressList() { return &m_ipAddressList; }
   const InetAddress& getFirstIpAddress();
   UINT32 getZoneId() { return m_zoneId; }
   UINT32 getIfIndex() { return m_index; }
   UINT32 getIfType() { return m_type; }
   UINT32 getMTU() { return m_mtu; }
   UINT64 getSpeed() { return m_speed; }
	UINT32 getBridgePortNumber() { return m_bridgePortNumber; }
	UINT32 getSlotNumber() { return m_slotNumber; }
	UINT32 getPortNumber() { return m_portNumber; }
	UINT32 getPeerNodeId() { return m_peerNodeId; }
	UINT32 getPeerInterfaceId() { return m_peerInterfaceId; }
   LinkLayerProtocol getPeerDiscoveryProtocol() { return m_peerDiscoveryProtocol; }
	UINT32 getFlags() { return m_flags; }
	int getAdminState() { return (int)m_adminState; }
	int getOperState() { return (int)m_operState; }
	int getDot1xPaeAuthState() { return (int)m_dot1xPaeAuthState; }
	int getDot1xBackendAuthState() { return (int)m_dot1xBackendAuthState; }
	const TCHAR *getDescription() { return m_description; }
	const TCHAR *getAlias() { return m_alias; }
   const BYTE *getMacAddr() { return m_macAddr; }
   int getIfTableSuffixLen() { return m_ifTableSuffixLen; }
   const UINT32 *getIfTableSuffix() { return m_ifTableSuffix; }
   UINT32 getPingTime();
	bool isSyntheticMask() { return (m_flags & IF_SYNTHETIC_MASK) ? true : false; }
	bool isPhysicalPort() { return (m_flags & IF_PHYSICAL_PORT) ? true : false; }
	bool isLoopback() { return (m_flags & IF_LOOPBACK) ? true : false; }
	bool isManuallyCreated() { return (m_flags & IF_CREATED_MANUALLY) ? true : false; }
	bool isExcludedFromTopology() { return (m_flags & (IF_EXCLUDE_FROM_TOPOLOGY | IF_LOOPBACK)) ? true : false; }
   bool isFake() { return (m_index == 1) &&
                          (m_type == IFTYPE_OTHER) &&
                          (!_tcscmp(m_name, _T("lan0")) || !_tcscmp(m_name, _T("unknown"))) &&
                          (!memcmp(m_macAddr, "\x00\x00\x00\x00\x00\x00", 6)); }

   UINT64 getLastDownEventId() { return m_lastDownEventId; }
   void setLastDownEventId(QWORD id) { m_lastDownEventId = id; }

   void setMacAddr(const BYTE *pbNewMac);
   void setIpAddress(const InetAddress& addr);
   void setBridgePortNumber(UINT32 bpn) { m_bridgePortNumber = bpn; setModified(); }
   void setSlotNumber(UINT32 slot) { m_slotNumber = slot; setModified(); }
   void setPortNumber(UINT32 port) { m_portNumber = port; setModified(); }
	void setPhysicalPortFlag(bool isPhysical) { if (isPhysical) m_flags |= IF_PHYSICAL_PORT; else m_flags &= ~IF_PHYSICAL_PORT; setModified(); }
	void setManualCreationFlag(bool isManual) { if (isManual) m_flags |= IF_CREATED_MANUALLY; else m_flags &= ~IF_CREATED_MANUALLY; setModified(); }
	void setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol, bool reflection);
   void clearPeer() { lockProperties(); m_peerNodeId = 0; m_peerInterfaceId = 0; m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN; setModified(); unlockProperties(); }
   void setDescription(const TCHAR *descr) { lockProperties(); nx_strncpy(m_description, descr, MAX_DB_STRING); setModified(); unlockProperties(); }
   void setAlias(const TCHAR *alias) { lockProperties(); nx_strncpy(m_alias, alias, MAX_DB_STRING); setModified(); unlockProperties(); }
   void addIpAddress(const InetAddress& addr);
   void deleteIpAddress(InetAddress addr);
   void setNetMask(const InetAddress& addr);
	void setMTU(int mtu) { m_mtu = mtu; setModified(); }
	void setSpeed(UINT64 speed) { m_speed = speed; setModified(); }
   void setIfTableSuffix(int len, const UINT32 *suffix) { lockProperties(); safe_free(m_ifTableSuffix); m_ifTableSuffixLen = len; m_ifTableSuffix = (len > 0) ? (UINT32 *)nx_memdup(suffix, len * sizeof(UINT32)) : NULL; setModified(); unlockProperties(); }

	void updateZoneId();

   void statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, UINT32 nodeIcmpProxy);

   UINT32 wakeUp();
	void setExpectedState(int state);
	void updatePingData();
};

/**
 * Network service class
 */
class NXCORE_EXPORTABLE NetworkService : public NetObj
{
protected:
   int m_serviceType;   // SSH, POP3, etc.
   Node *m_hostNode;    // Pointer to node object which hosts this service
   UINT32 m_pollerNode; // ID of node object which is used for polling
                         // If 0, m_pHostNode->m_dwPollerNode will be used
   UINT16 m_proto;        // Protocol (TCP, UDP, etc.)
   UINT16 m_port;         // TCP or UDP port number
   InetAddress m_ipAddress;
   TCHAR *m_request;  // Service-specific request
   TCHAR *m_response; // Service-specific expected response
	int m_pendingStatus;
	int m_pollCount;
	int m_requiredPollCount;
   UINT32 m_responseTime;  // Response time from last poll

   virtual void onObjectDelete(UINT32 dwObjectId);

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  Node *pHostNode = NULL, UINT32 dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int getObjectClass() const { return OBJECT_NETWORKSERVICE; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   void statusPoll(ClientSession *session, UINT32 rqId, Node *pollerNode, Queue *eventQueue);

   UINT32 getResponseTime() { return m_responseTime; }
};

/**
 * VPN connector class
 */
class NXCORE_EXPORTABLE VPNConnector : public NetObj
{
protected:
   UINT32 m_dwPeerGateway;        // Object ID of peer gateway
   ObjectArray<InetAddress> *m_localNetworks;
   ObjectArray<InetAddress> *m_remoteNetworks;

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   Node *getParentNode();

public:
   VPNConnector();
   VPNConnector(bool hidden);
   virtual ~VPNConnector();

   virtual int getObjectClass() const { return OBJECT_VPNCONNECTOR; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   bool isLocalAddr(const InetAddress& addr);
   bool isRemoteAddr(const InetAddress& addr);
   UINT32 getPeerGatewayId() const { return m_dwPeerGateway; }
   InetAddress getPeerGatewayAddr();
};

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public Template
{
protected:
   UINT32 m_pingTime;
   time_t m_pingLastTimeStamp;

	virtual void fillMessageInternal(NXCPMessage *pMsg);
	virtual void fillMessageInternalStage2(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	virtual bool isDataCollectionDisabled();
   virtual void updatePingData();

   NetObj *objectFromParameter(const TCHAR *param);

public:
   DataCollectionTarget();
   DataCollectionTarget(const TCHAR *name);
   virtual ~DataCollectionTarget();

   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual UINT32 getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);
   virtual UINT32 getScriptItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);

   UINT32 getTableLastValues(UINT32 dciId, NXCPMessage *msg);
	UINT32 getThresholdSummary(NXCPMessage *msg, UINT32 baseId);
	UINT32 getPerfTabDCIList(NXCPMessage *pMsg);
   void getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData);

   void updateDciCache();
   void cleanDCIData(DB_HANDLE hdb);
   void queueItemsForPolling(Queue *pPollerQueue);
	bool processNewDCValue(DCObject *dco, time_t currTime, const void *value);

	bool applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject);
   void cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList);
   virtual void unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI);

   virtual bool isEventSource();

   int getMostCriticalDCIStatus();

   UINT32 getPingTime();
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
   InetAddress m_ipAddress;

	virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   virtual int getObjectClass() const { return OBJECT_MOBILEDEVICE; }

   virtual BOOL loadFromDatabase(UINT32 dwId);
   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	void updateSystemInfo(NXCPMessage *msg);
	void updateStatus(NXCPMessage *msg);

	const TCHAR *getDeviceId() { return CHECK_NULL_EX(m_deviceId); }

	virtual UINT32 getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);
};

/**
 * Access point class
 */
class NXCORE_EXPORTABLE AccessPoint : public DataCollectionTarget
{
protected:
   UINT32 m_index;
   InetAddress m_ipAddress;
	UINT32 m_nodeId;
	BYTE m_macAddr[MAC_ADDR_LENGTH];
	TCHAR *m_vendor;
	TCHAR *m_model;
	TCHAR *m_serialNumber;
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;
   AccessPointState m_state;
   AccessPointState m_prevState;

	virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void updatePingData();

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, UINT32 index, const BYTE *macAddr);
   virtual ~AccessPoint();

   virtual int getObjectClass() const { return OBJECT_ACCESSPOINT; }

   virtual BOOL loadFromDatabase(UINT32 dwId);
   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   void statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, Node *controller, SNMP_Transport *snmpTransport);

   UINT32 getIndex() { return m_index; }
	const BYTE *getMacAddr() { return m_macAddr; }
   const InetAddress& getIpAddress() { return m_ipAddress; }
	bool isMyRadio(int rfIndex);
	bool isMyRadio(const BYTE *macAddr);
	void getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize);
   AccessPointState getState() { return m_state; }
   Node *getParentNode();

	void attachToNode(UINT32 nodeId);
   void setIpAddress(const InetAddress& addr) { lockProperties(); m_ipAddress = addr; setModified(); unlockProperties(); }
	void updateRadioInterfaces(const ObjectArray<RadioInterfaceInfo> *ri);
	void updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber);
   void updateState(AccessPointState state);
};

/**
 * Cluster class
 */
class NXCORE_EXPORTABLE Cluster : public DataCollectionTarget
{
protected:
	UINT32 m_dwClusterType;
   ObjectArray<InetAddress> *m_syncNetworks;
	UINT32 m_dwNumResources;
	CLUSTER_RESOURCE *m_pResourceList;
	UINT32 m_dwFlags;
	time_t m_tmLastPoll;
	UINT32 m_zoneId;

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
	Cluster();
   Cluster(const TCHAR *pszName, UINT32 zoneId);
	virtual ~Cluster();

   virtual int getObjectClass() const { return OBJECT_CLUSTER; }
   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   virtual void unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI);

	bool isSyncAddr(const InetAddress& addr);
	bool isVirtualAddr(const InetAddress& addr);
	bool isResourceOnNode(UINT32 dwResource, UINT32 dwNode);
   UINT32 getZoneId() { return m_zoneId; }

   void statusPoll(PollerInfo *poller);
   void statusPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);
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
struct ProxyInfo;

/**
 * Node
 */
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
	friend class Subnet;

private:
   void onSnmpProxyChange(UINT32 oldProxy);

   static void onDataCollectionChangeAsyncCallback(void *arg);

protected:
   InetAddress m_ipAddress;
	TCHAR m_primaryName[MAX_DNS_NAME];
   UINT32 m_dwFlags;
   UINT32 m_dwDynamicFlags;       // Flags used at runtime by server
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;
   UINT32 m_zoneId;
   UINT16 m_agentPort;
   INT16 m_agentAuthMethod;
   INT16 m_agentCacheMode;
   TCHAR m_szSharedSecret[MAX_SECRET_LENGTH];
   INT16 m_iStatusPollType;
   INT16 m_snmpVersion;
   UINT16 m_snmpPort;
	UINT16 m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
   TCHAR m_szObjectId[MAX_OID_LEN * 4];
   TCHAR m_szAgentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_szPlatformName[MAX_PLATFORM_NAME_LEN];
	TCHAR *m_sysDescription;  // Agent's System.Uname or SNMP sysDescr
	TCHAR *m_sysName;				// SNMP sysName
	TCHAR *m_sysLocation;      // SNMP sysLocation
	TCHAR *m_sysContact;       // SNMP sysContact
	TCHAR *m_lldpNodeId;			// lldpLocChassisId combined with lldpLocChassisIdSubtype, or NULL for non-LLDP nodes
	ObjectArray<LLDP_LOCAL_PORT_INFO> *m_lldpLocalPortInfo;
	NetworkDeviceDriver *m_driver;
	DriverData *m_driverData;
   ObjectArray<AgentParameterDefinition> *m_paramList; // List of supported parameters
   ObjectArray<AgentTableDefinition> *m_tableList; // List of supported tables
   time_t m_lastDiscoveryPoll;
   time_t m_lastStatusPoll;
   time_t m_lastConfigurationPoll;
	time_t m_lastInstancePoll;
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
   UINT32 m_lastSNMPTrapId;
   UINT32 m_pollerNode;      // Node used for network service polling
   UINT32 m_agentProxy;      // Node used as proxy for agent connection
	UINT32 m_snmpProxy;       // Node used as proxy for SNMP requests
   UINT32 m_icmpProxy;       // Node used as proxy for ICMP ping
   UINT64 m_qwLastEvents[MAX_LAST_EVENTS];
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
	AgentConnection *m_fileUpdateConn;
	INT16 m_rackHeight;
	INT16 m_rackPosition;
	UINT32 m_rackId;
	uuid m_rackImage;

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
	void setPrimaryIPAddress(const InetAddress& addr);

   UINT32 getInterfaceCount(Interface **ppInterface);

   void checkInterfaceNames(InterfaceList *pIfList);
   Subnet *createSubnet(const InetAddress& baseAddr, bool syntheticMask);
	void checkAgentPolicyBinding(AgentConnection *conn);
	void updatePrimaryIpAddr();
	bool confPollAgent(UINT32 dwRqId);
	bool confPollSnmp(UINT32 dwRqId);
	bool querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, UINT32 pollRqId, TCHAR **value);
	void checkBridgeMib(SNMP_Transport *pTransport);
	void checkIfXTable(SNMP_Transport *pTransport);
	void executeHookScript(const TCHAR *hookName);
	bool checkNetworkPath(UINT32 dwRqId);
	bool checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 dwRqId);

	void applyUserTemplates();
	void doInstanceDiscovery(UINT32 requestId);
	StringMap *getInstanceList(DCItem *dci);
	void updateInstances(DCItem *root, StringMap *instances, UINT32 requestId);
   void syncDataCollectionWithAgent(AgentConnectionEx *conn);

   void collectProxyInfo(ProxyInfo *info);
   static void collectProxyInfoCallback(NetObj *node, void *data);

	void updateContainerMembership();
	bool updateInterfaceConfiguration(UINT32 rqid, int maskBits);
   bool deleteDuplicateInterfaces(UINT32 rqid);
   void updateRackBinding();

	void buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedObject, bool vpnLink, bool includeEndNodes);

	virtual bool isDataCollectionDisabled();

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 dwObjectId);

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void updatePingData();

   virtual void onDataCollectionChange();

public:
   Node();
   Node(const InetAddress& addr, UINT32 dwFlags, UINT32 agentProxy, UINT32 snmpProxy, UINT32 dwZone);
   virtual ~Node();

   virtual int getObjectClass() const { return OBJECT_NODE; }
	UINT32 getIcmpProxy() { return m_icmpProxy; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	TCHAR *expandText(const TCHAR *textTemplate, StringMap *inputFields, const TCHAR *userName);

	Cluster *getMyCluster();

   const InetAddress& getIpAddress() { return m_ipAddress; }
   UINT32 getZoneId() { return m_zoneId; }
   UINT32 getFlags() { return m_dwFlags; }
   UINT32 getRuntimeFlags() { return m_dwDynamicFlags; }
   void setFlag(UINT32 flag) { lockProperties(); m_dwFlags |= flag; setModified(); unlockProperties(); }
   void clearFlag(UINT32 flag) { lockProperties(); m_dwFlags &= ~flag; setModified(); unlockProperties(); }
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
   const TCHAR *getSysContact() { return CHECK_NULL_EX(m_sysContact); }
   const TCHAR *getSysLocation() { return CHECK_NULL_EX(m_sysLocation); }
   time_t getBootTime() { return m_bootTime; }
	const TCHAR *getLLDPNodeId() { return m_lldpNodeId; }
   const BYTE *getBridgeId() { return m_baseBridgeAddress; }
	const TCHAR *getDriverName() { return (m_driver != NULL) ? m_driver->getName() : _T("GENERIC"); }
	UINT16 getAgentPort() { return m_agentPort; }
	INT16 getAgentAuthMethod() { return m_agentAuthMethod; }
   INT16 getAgentCacheMode() { return (m_dwDynamicFlags & NDF_CACHE_MODE_NOT_SUPPORTED) ? AGENT_CACHE_OFF : ((m_agentCacheMode == AGENT_CACHE_DEFAULT) ? g_defaultAgentCacheMode : m_agentCacheMode); }
	const TCHAR *getSharedSecret() { return m_szSharedSecret; }
	AgentConnection *getFileUpdateConn() { return m_fileUpdateConn; }

   bool isDown() { return (m_dwDynamicFlags & NDF_UNREACHABLE) ? true : false; }
	time_t getDownTime() const { return m_downSince; }

   void addInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   Interface *createNewInterface(InterfaceInfo *ifInfo, bool manuallyCreated);
   Interface *createNewInterface(const InetAddress& ipAddr, BYTE *macAddr);
   void deleteInterface(Interface *iface);

	void setPrimaryName(const TCHAR *name) { nx_strncpy(m_primaryName, name, MAX_DNS_NAME); }
	void setAgentPort(WORD port) { m_agentPort = port; }
	void setSnmpPort(WORD port) { m_snmpPort = port; }
   void changeIPAddress(const InetAddress& ipAddr);
	void changeZone(UINT32 newZone);
	void setFileUpdateConn(AgentConnection *conn);
   void clearDataCollectionConfigFromAgent(AgentConnectionEx *conn);
   void forceSyncDataCollectionConfig(Node *node);

   ARP_CACHE *getArpCache();
   InterfaceList *getInterfaceList();
   Interface *findInterfaceByIndex(UINT32 ifIndex);
   Interface *findInterfaceByName(const TCHAR *name);
	Interface *findInterfaceByMAC(const BYTE *macAddr);
	Interface *findInterfaceByIP(const InetAddress& addr);
	Interface *findInterfaceBySlotAndPort(UINT32 slot, UINT32 port);
	Interface *findBridgePort(UINT32 bridgePortNumber);
   AccessPoint *findAccessPointByMAC(const BYTE *macAddr);
   AccessPoint *findAccessPointByBSSID(const BYTE *bssid);
   AccessPoint *findAccessPointByRadioId(int rfIndex);
   ObjectArray<WirelessStationInfo> *getWirelessStations();
	bool isMyIP(const InetAddress& addr);
   void getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 dwIndex, int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState);
   void getInterfaceStatusFromAgent(UINT32 dwIndex, InterfaceAdminState *adminState, InterfaceOperState *operState);
   ROUTING_TABLE *getRoutingTable();
   ROUTING_TABLE *getCachedRoutingTable() { return m_pRoutingTable; }
	LinkLayerNeighbors *getLinkLayerNeighbors();
	VlanList *getVlans();
   bool getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, UINT32 *ifIndex, bool *isVpn, TCHAR *name);
   bool getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, UINT32 *srcIfIndex);
	ComponentTree *getComponents();
   bool getLldpLocalPortInfo(BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port);

	void setRecheckCapsFlag() { m_dwDynamicFlags |= NDF_RECHECK_CAPABILITIES; }
   void setDiscoveryPollTimeStamp();
   void statusPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);
   void statusPoll(PollerInfo *poller);
   void configurationPoll(PollerInfo *poller);
   void configurationPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller, int maskBits);
	void instanceDiscoveryPoll(PollerInfo *poller);
	void instanceDiscoveryPoll(ClientSession *session, UINT32 requestId, PollerInfo *poller);
	void topologyPoll(PollerInfo *poller);
	void topologyPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);
	void resolveVlanPorts(VlanList *vlanList);
	void updateInterfaceNames(ClientSession *pSession, UINT32 dwRqId);
	void routingTablePoll(PollerInfo *poller);
   void updateRoutingTable();
	void checkSubnetBinding();
   AccessPointState getAccessPointState(AccessPoint *ap, SNMP_Transport *snmpTransport);

   bool isReadyForStatusPoll();
   bool isReadyForConfigurationPoll();
   bool isReadyForInstancePoll();
   bool isReadyForDiscoveryPoll();
   bool isReadyForRoutePoll();
   bool isReadyForTopologyPoll();

   void lockForStatusPoll();
   void lockForConfigurationPoll();
   void lockForInstancePoll();
   void lockForDiscoveryPoll();
   void lockForRoutePoll();
   void lockForTopologyPoll();
	void forceConfigurationPoll() { m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL; }

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   bool connectToAgent(UINT32 *error = NULL, UINT32 *socketError = NULL, bool *newConnection = NULL);
	bool checkAgentTrapId(UINT64 id);
	bool checkSNMPTrapId(UINT32 id);
   bool checkAgentPushRequestId(UINT64 id);

   bool connectToSMCLP();

	virtual UINT32 getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);

   UINT32 getItemFromSNMP(WORD port, const TCHAR *param, size_t bufSize, TCHAR *buffer, int interpretRawValue);
	UINT32 getTableFromSNMP(WORD port, const TCHAR *oid, ObjectArray<DCTableColumn> *columns, Table **table);
   UINT32 getListFromSNMP(WORD port, const TCHAR *oid, StringList **list);
   UINT32 getOIDSuffixListFromSNMP(WORD port, const TCHAR *oid, StringMap **values);
   UINT32 getItemFromCheckPointSNMP(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
   UINT32 getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
	UINT32 getTableFromAgent(const TCHAR *name, Table **table);
	UINT32 getListFromAgent(const TCHAR *name, StringList **list);
   UINT32 getItemForClient(int iOrigin, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize);
   UINT32 getTableForClient(const TCHAR *name, Table **table);
   UINT32 getItemFromSMCLP(const TCHAR *param, UINT32 bufSize, TCHAR *buffer);

	virtual NXSL_Array *getParentsForNXSL();
	virtual NXSL_Array *getTemplatesForNXSL();
	NXSL_Array *getInterfacesForNXSL();

   void openParamList(ObjectArray<AgentParameterDefinition> **paramList);
   void closeParamList() { unlockProperties(); }

   void openTableList(ObjectArray<AgentTableDefinition> **tableList);
   void closeTableList() { unlockProperties(); }

   AgentConnectionEx *createAgentConnection();
	SNMP_Transport *createSnmpTransport(WORD port = 0, const TCHAR *context = NULL);
	SNMP_SecurityContext *getSnmpSecurityContext();
   UINT32 getEffectiveSnmpProxy();

   void writeParamListToMessage(NXCPMessage *pMsg, WORD flags);
	void writeWinPerfObjectsToMessage(NXCPMessage *msg);
	void writePackageListToMessage(NXCPMessage *msg);
	void writeWsListToMessage(NXCPMessage *msg);

   UINT32 wakeUp();

   void addService(NetworkService *pNetSrv) { AddChild(pNetSrv); pNetSrv->AddParent(this); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL, UINT32 *responseTime = NULL);

   QWORD getLastEventId(int nIndex) { return ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) ? m_qwLastEvents[nIndex] : 0; }
   void setLastEventId(int nIndex, QWORD qwId) { if ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) m_qwLastEvents[nIndex] = qwId; }

   UINT32 callSnmpEnumerate(const TCHAR *pszRootOid,
      UINT32 (* pHandler)(UINT32, SNMP_Variable *, SNMP_Transport *, void *), void *pArg, const TCHAR *context = NULL);

	nxmap_ObjList *getL2Topology();
	nxmap_ObjList *buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes);
	ForwardingDatabase *getSwitchForwardingDatabase();
	NetObj *findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type);
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
          ((UINT32)(time(NULL) - m_lastStatusPoll) > g_dwStatusPollingInterval);
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
          ((UINT32)(time(NULL) - m_lastConfigurationPoll) > g_dwConfigurationPollingInterval);
}

inline bool Node::isReadyForDiscoveryPoll()
{
	if (m_isDeleted)
		return false;
   return (g_flags & AF_ENABLE_NETWORK_DISCOVERY) &&
          (m_iStatus != STATUS_UNMANAGED) &&
			 (!(m_dwFlags & NF_DISABLE_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)(time(NULL) - m_lastDiscoveryPoll) > g_dwDiscoveryPollingInterval);
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
          ((UINT32)(time(NULL) - m_lastRTUpdate) > g_dwRoutingTableUpdateInterval);
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
          ((UINT32)(time(NULL) - m_lastTopologyPoll) > g_dwTopologyPollingInterval);
}

inline bool Node::isReadyForInstancePoll()
{
	if (m_isDeleted)
		return false;
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_CONF_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_INSTANCE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)(time(NULL) - m_lastInstancePoll) > g_instancePollingInterval);
}

inline void Node::lockForStatusPoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_STATUS_POLL;
   unlockProperties();
}

inline void Node::lockForConfigurationPoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_CONFIG_POLL;
   unlockProperties();
}

inline void Node::lockForInstancePoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_INSTANCE_POLL;
   unlockProperties();
}

inline void Node::lockForDiscoveryPoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_DISCOVERY_POLL;
   unlockProperties();
}

inline void Node::lockForTopologyPoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_TOPOLOGY_POLL;
   unlockProperties();
}

inline void Node::lockForRoutePoll()
{
   lockProperties();
   m_dwDynamicFlags |= NDF_QUEUED_FOR_ROUTE_POLL;
   unlockProperties();
}

/**
 * Subnet
 */
class NXCORE_EXPORTABLE Subnet : public NetObj
{
	friend void Node::buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedSubnet, bool vpnLink, bool includeEndNodes);

protected:
   InetAddress m_ipAddress;
   UINT32 m_zoneId;
	bool m_bSyntheticMask;

   virtual void prepareForDeletion();

   virtual void fillMessageInternal(NXCPMessage *pMsg);

   void buildIPTopologyInternal(nxmap_ObjList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes);

public:
   Subnet();
   Subnet(const InetAddress& addr, UINT32 dwZone, bool bSyntheticMask);
   virtual ~Subnet();

   virtual int getObjectClass() const { return OBJECT_SUBNET; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   void addNode(Node *node) { AddChild(node); node->AddParent(this); calculateCompoundStatus(TRUE); }

	virtual bool showThresholdSummary();

   const InetAddress& getIpAddress() { return m_ipAddress; }
   UINT32 getZoneId() { return m_zoneId; }
	bool isSyntheticMask() { return m_bSyntheticMask; }

	void setCorrectMask(const InetAddress& addr);

	bool findMacAddress(const InetAddress& ipAddr, BYTE *macAddr);

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

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
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

   virtual int getObjectClass() const { return OBJECT_SERVICEROOT; }

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

   virtual int getObjectClass() const { return OBJECT_TEMPLATEROOT; }
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

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Container();
   Container(const TCHAR *pszName, UINT32 dwCategory);
   virtual ~Container();

   virtual int getObjectClass() const { return OBJECT_CONTAINER; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	virtual bool showThresholdSummary();

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   UINT32 getCategory() { return m_dwCategory; }

   void linkChildObjects();
   void linkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }

   AutoBindDecision isSuitableForNode(Node *node);
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
   TemplateGroup(const TCHAR *pszName) : Container(pszName, 0) { m_iStatus = STATUS_NORMAL; }
   virtual ~TemplateGroup() { }

   virtual int getObjectClass() const { return OBJECT_TEMPLATEGROUP; }
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

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Rack();
   Rack(const TCHAR *name, int height);
   virtual ~Rack();

   virtual int getObjectClass() const { return OBJECT_RACK; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);
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
	InetAddressIndex *m_idxNodeByAddr;
	InetAddressIndex *m_idxInterfaceByAddr;
	InetAddressIndex *m_idxSubnetByAddr;

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Zone();
   Zone(UINT32 zoneId, const TCHAR *name);
   virtual ~Zone();

   virtual int getObjectClass() const { return OBJECT_ZONE; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	virtual bool showThresholdSummary();

   UINT32 getZoneId() { return m_zoneId; }
	UINT32 getAgentProxy() { return m_agentProxy; }
	UINT32 getSnmpProxy() { return m_snmpProxy; }
	UINT32 getIcmpProxy() { return m_icmpProxy; }

   void addSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }

	void addToIndex(Subnet *subnet) { m_idxSubnetByAddr->put(subnet->getIpAddress(), subnet); }
   void addToIndex(Interface *iface) { m_idxInterfaceByAddr->put(iface->getIpAddressList(), iface); }
   void addToIndex(const InetAddress& addr, Interface *iface) { m_idxInterfaceByAddr->put(addr, iface); }
	void addToIndex(Node *node) { m_idxNodeByAddr->put(node->getIpAddress(), node); }
	void removeFromIndex(Subnet *subnet) { m_idxSubnetByAddr->remove(subnet->getIpAddress()); }
	void removeFromIndex(Interface *iface);
   void removeFromInterfaceIndex(const InetAddress& addr) { m_idxInterfaceByAddr->remove(addr); }
	void removeFromIndex(Node *node) { m_idxNodeByAddr->remove(node->getIpAddress()); }
	void updateInterfaceIndex(const InetAddress& oldIp, const InetAddress& newIp, Interface *iface);
	Subnet *getSubnetByAddr(const InetAddress& ipAddr) { return (Subnet *)m_idxSubnetByAddr->get(ipAddr); }
	Interface *getInterfaceByAddr(const InetAddress& ipAddr) { return (Interface *)m_idxInterfaceByAddr->get(ipAddr); }
	Node *getNodeByAddr(const InetAddress& ipAddr) { return (Node *)m_idxNodeByAddr->get(ipAddr); }
	Subnet *findSubnet(bool (*comparator)(NetObj *, void *), void *data) { return (Subnet *)m_idxSubnetByAddr->find(comparator, data); }
	Interface *findInterface(bool (*comparator)(NetObj *, void *), void *data) { return (Interface *)m_idxInterfaceByAddr->find(comparator, data); }
	Node *findNode(bool (*comparator)(NetObj *, void *), void *data) { return (Node *)m_idxNodeByAddr->find(comparator, data); }
   void forEachSubnet(void (*callback)(const InetAddress& addr, NetObj *, void *), void *data) { m_idxSubnetByAddr->forEach(callback, data); }
};

/**
 * Entire network
 */
class NXCORE_EXPORTABLE Network : public NetObj
{
public:
   Network();
   virtual ~Network();

   virtual int getObjectClass() const { return OBJECT_NETWORK; }
   virtual BOOL saveToDatabase(DB_HANDLE hdb);

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

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Condition();
   Condition(bool hidden);
   virtual ~Condition();

   virtual int getObjectClass() const { return OBJECT_CONDITION; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   void lockForPoll();
   void doPoll(PollerInfo *poller);
   void check();

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

	BOOL savePolicyCommonProperties(DB_HANDLE hdb);

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   AgentPolicy(int type);
   AgentPolicy(const TCHAR *name, int type);

   virtual int getObjectClass() const { return OBJECT_AGENTPOLICY; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	virtual bool createDeploymentMessage(NXCPMessage *msg);
	virtual bool createUninstallMessage(NXCPMessage *msg);

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

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   AgentPolicyConfig();
   AgentPolicyConfig(const TCHAR *name);
   virtual ~AgentPolicyConfig();

   virtual int getObjectClass() const { return OBJECT_AGENTPOLICY_CONFIG; }

   virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	virtual bool createDeploymentMessage(NXCPMessage *msg);
	virtual bool createUninstallMessage(NXCPMessage *msg);
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

   virtual int getObjectClass() const { return OBJECT_POLICYGROUP; }
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

   virtual int getObjectClass() const { return OBJECT_POLICYROOT; }
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

   virtual int getObjectClass() const { return OBJECT_NETWORKMAPROOT; }
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

   virtual int getObjectClass() const { return OBJECT_NETWORKMAPGROUP; }
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
   int m_objectDisplayMode;
	uuid m_background;
	double m_backgroundLatitude;
	double m_backgroundLongitude;
	int m_backgroundZoom;
	UINT32 m_nextElementId;
	ObjectArray<NetworkMapElement> *m_elements;
	ObjectArray<NetworkMapLink> *m_links;
	TCHAR *m_filterSource;
	NXSL_VM *m_filter;

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void updateObjects(nxmap_ObjList *objects);
	UINT32 objectIdFromElementId(UINT32 eid);
	UINT32 elementIdFromObjectId(UINT32 eid);

   void setFilter(const TCHAR *filter);

public:
   NetworkMap();
	NetworkMap(int type, UINT32 seed);
   virtual ~NetworkMap();

   virtual int getObjectClass() const { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

   virtual void onObjectDelete(UINT32 dwObjectId);

   void updateContent();

   int getBackgroundColor() { return m_backgroundColor; }
   void setBackgroundColor(int color) { m_backgroundColor = color; }

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

   virtual int getObjectClass() const { return OBJECT_DASHBOARDROOT; }
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

   virtual void fillMessageInternal(NXCPMessage *pMsg);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Dashboard();
   Dashboard(const TCHAR *name);
   virtual ~Dashboard();

   virtual int getObjectClass() const { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual BOOL loadFromDatabase(UINT32 dwId);

	virtual bool showThresholdSummary();
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

	virtual void fillMessageInternal(NXCPMessage *pMsg);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

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

	virtual int getObjectClass() const { return OBJECT_SLMCHECK; }

	virtual BOOL saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);
	virtual BOOL loadFromDatabase(UINT32 dwId);

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

	virtual void fillMessageInternal(NXCPMessage *pMsg);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void initServiceContainer();
	BOOL addHistoryRecord();
	double getUptimeFromDBFor(Period period, INT32 *downtime);

public:
	ServiceContainer();
	ServiceContainer(const TCHAR *pszName);

	virtual BOOL loadFromDatabase(UINT32 dwId);
	virtual BOOL saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

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

	virtual int getObjectClass() const { return OBJECT_BUSINESSSERVICEROOT; }

	virtual BOOL saveToDatabase(DB_HANDLE hdb);
   void LoadFromDB();

   void LinkChildObjects();
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }
};

/**
 * Business service object
 */
class NXCORE_EXPORTABLE BusinessService : public ServiceContainer
{
protected:
	bool m_busy;
   bool m_pollingDisabled;
	time_t m_lastPollTime;
	int m_lastPollStatus;

   virtual void prepareForDeletion();

	virtual void fillMessageInternal(NXCPMessage *pMsg);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
	BusinessService();
	BusinessService(const TCHAR *name);
	virtual ~BusinessService();

	virtual int getObjectClass() const { return OBJECT_BUSINESSSERVICE; }

	virtual BOOL loadFromDatabase(UINT32 dwId);
	virtual BOOL saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	bool isReadyForPolling();
	void lockForPolling();
	void poll(PollerInfo *poller);
	void poll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);

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

	virtual void fillMessageInternal(NXCPMessage *pMsg);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void applyTemplate(SlmCheck *tmpl);

public:
	NodeLink();
	NodeLink(const TCHAR *name, UINT32 nodeId);
	virtual ~NodeLink();

	virtual int getObjectClass() const { return OBJECT_NODELINK; }

	virtual BOOL saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);
	virtual BOOL loadFromDatabase(UINT32 dwId);

	void execute();
	void applyTemplates();

	UINT32 getNodeId() { return m_nodeId; }
};

/**
 * Get IP address for object
 */
inline const InetAddress& GetObjectIpAddress(NetObj *object)
{
   if (object->getObjectClass() == OBJECT_NODE)
      return ((Node *)object)->getIpAddress();
   if (object->getObjectClass() == OBJECT_SUBNET)
      return ((Subnet *)object)->getIpAddress();
   if (object->getObjectClass() == OBJECT_ACCESSPOINT)
      return ((AccessPoint *)object)->getIpAddress();
   return InetAddress::INVALID;
}

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

void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Interface *iface);
ComponentTree *BuildComponentTree(Node *node, SNMP_Transport *snmp);

void NXCORE_EXPORTABLE MacDbAddAccessPoint(AccessPoint *ap);
void NXCORE_EXPORTABLE MacDbAddInterface(Interface *iface);
void NXCORE_EXPORTABLE MacDbAddObject(const BYTE *macAddr, NetObj *object);
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr);
NetObj NXCORE_EXPORTABLE *MacDbFind(const BYTE *macAddr);

NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass);
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(const uuid& guid, int objClass);
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneId, const InetAddress& ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneId, const InetAddressList *ipAddrList);
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr);
Node NXCORE_EXPORTABLE *FindNodeByBridgeId(const BYTE *bridgeId);
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId);
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneId, const InetAddress& ipAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneId, const InetAddress& ipAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneId, const InetAddress& nodeAddr);
MobileDevice NXCORE_EXPORTABLE *FindMobileDeviceByDeviceID(const TCHAR *deviceId);
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByMAC(const BYTE *macAddr);
UINT32 NXCORE_EXPORTABLE FindLocalMgmtNode();
CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(UINT32 dwId);
Zone NXCORE_EXPORTABLE *FindZoneByGUID(UINT32 dwZoneGUID);
Cluster NXCORE_EXPORTABLE *FindClusterByResourceIP(UINT32 zone, const InetAddress& ipAddr);
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zone, const InetAddress& ipAddr);

BOOL LoadObjects();
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter);

void DeleteUserFromAllObjects(UINT32 dwUserId);

bool IsValidParentClass(int iChildClass, int iParentClass);
bool IsAgentPolicyObject(NetObj *object);
bool IsEventSource(int objectClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);

PollerInfo *RegisterPoller(PollerType type, NetObj *object);
void ShowPollers(CONSOLE_CTX console);

/**
 * Global variables
 */
extern Network NXCORE_EXPORTABLE *g_pEntireNet;
extern ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot;
extern TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot;
extern PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot;
extern NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot;
extern DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot;
extern BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot;

extern UINT32 NXCORE_EXPORTABLE g_dwMgmtNode;
extern UINT32 g_dwNumCategories;
extern CONTAINER_CATEGORY *g_pContainerCatList;
extern const TCHAR *g_szClassName[];
extern BOOL g_bModificationsLocked;
extern Queue *g_pTemplateUpdateQueue;

extern ObjectIndex NXCORE_EXPORTABLE g_idxObjectById;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxSubnetByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxInterfaceByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxNodeByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxZoneByGUID;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxClusterById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxMobileDeviceById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxAccessPointById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxConditionById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxServiceCheckById;


#endif   /* _nms_objects_h_ */
