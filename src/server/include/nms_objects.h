/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
#include <jansson.h>
#include "nxcore_jobs.h"
#include "nms_topo.h"

/**
 * Forward declarations of classes
 */
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
#define NDF_NEW_TUNNEL_BIND            0x040000

#define NDF_PERSISTENT (NDF_UNREACHABLE | NDF_NETWORK_PATH_PROBLEM | NDF_AGENT_UNREACHABLE | NDF_SNMP_UNREACHABLE | NDF_CPSNMP_UNREACHABLE | NDF_CACHE_MODE_NOT_SUPPORTED)

#define __NDF_FLAGS_DEFINED

/**
 * Cluster runtime flags
 */
#define CLF_QUEUED_FOR_STATUS_POLL        0x0001
#define CLF_DOWN                          0x0002
#define CLF_QUEUED_FOR_CONFIGURATION_POLL 0x0004

class AgentTunnel;

/**
 * Extended agent connection
 */
class NXCORE_EXPORTABLE AgentConnectionEx : public AgentConnection
{
protected:
   UINT32 m_nodeId;
   AgentTunnel *m_tunnel;
   AgentTunnel *m_proxyTunnel;

   virtual AbstractCommChannel *createChannel();
   virtual void onTrap(NXCPMessage *msg);
   virtual void onSyslogMessage(NXCPMessage *pMsg);
   virtual void onDataPush(NXCPMessage *msg);
   virtual void onFileMonitoringData(NXCPMessage *msg);
   virtual void onSnmpTrap(NXCPMessage *pMsg);
   virtual UINT32 processCollectedData(NXCPMessage *msg);
   virtual UINT32 processBulkCollectedData(NXCPMessage *request, NXCPMessage *response);
   virtual bool processCustomMessage(NXCPMessage *msg);

   virtual ~AgentConnectionEx();

public:
   AgentConnectionEx(UINT32 nodeId, const InetAddress& ipAddr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL, bool allowCompression = true);
   AgentConnectionEx(UINT32 nodeId, AgentTunnel *tunnel, int authMethod = AUTH_NONE, const TCHAR *secret = NULL, bool allowCompression = true);

   UINT32 deployPolicy(AgentPolicy *policy);
   UINT32 uninstallPolicy(AgentPolicy *policy);

   void setTunnel(AgentTunnel *tunnel);

   using AgentConnection::setProxy;
   void setProxy(AgentTunnel *tunnel, int authMethod, const TCHAR *secret);
};

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

   PollerType getType() const { return m_type; }
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
	ObjectArray<NetObj> *findObjects(bool (*comparator)(NetObj *, void *), void *data);

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
	NXSL_Array *getChildrenForNXSL();

	UINT32 getClass() { return m_class; }
	const TCHAR *getFirmware() { return m_firmware; }
   const TCHAR *getModel() { return m_model; }
   const TCHAR *getName() { return m_name; }
   const TCHAR *getSerial() { return m_serial; }
   const TCHAR *getVendor() { return m_vendor; }

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

	SoftwarePackage();

public:
	~SoftwarePackage();

	void fillMessage(NXCPMessage *msg, UINT32 baseId) const;

	const TCHAR *getName() const { return m_name; }
	const TCHAR *getVersion() const { return m_version; }

	static SoftwarePackage *createFromTableRow(const Table *table, int row);
};

/**
 * Summary table flags
 */
#define SUMMARY_TABLE_MULTI_INSTANCE      0x0001
#define SUMMARY_TABLE_TABLE_DCI_SOURCE    0x0002

/**
 * Summary table column flags
 */
#define COLUMN_DEFINITION_REGEXP_MATCH    0x0001
#define COLUMN_DEFINITION_MULTIVALUED     0x0002

/**
 * Object modification flags
 */
#define MODIFY_RUNTIME              0x0000
#define MODIFY_OTHER                0x0001
#define MODIFY_CUSTOM_ATTRIBUTES    0x0002
#define MODIFY_DATA_COLLECTION      0x0004
#define MODIFY_RELATIONS            0x0008
#define MODIFY_COMMON_PROPERTIES    0x0010
#define MODIFY_ACCESS_LIST          0x0020
#define MODIFY_NODE_PROPERTIES      0x0040
#define MODIFY_INTERFACE_PROPERTIES 0x0080
#define MODIFIED_CLUSTER_RESOURCES  0x0100
#define MODIFY_MAP_CONTENT          0x0200
#define MODIFY_SENSOR_PROPERTIES    0x0400
#define MODIFY_ALL                  0xFFFF

/**
 * Column definition for DCI summary table
 */
class NXCORE_EXPORTABLE SummaryTableColumn
{
public:
   TCHAR m_name[MAX_DB_STRING];
   TCHAR m_dciName[MAX_PARAM_NAME];
   UINT32 m_flags;
   TCHAR m_separator[16];

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
   TCHAR m_tableDciName[MAX_PARAM_NAME];

   SummaryTable(INT32 id, DB_RESULT hResult);

public:
   static SummaryTable *loadFromDB(INT32 id, UINT32 *rcc);

   SummaryTable(NXCPMessage *msg);
   ~SummaryTable();

   bool filter(DataCollectionTarget *node);
   Table *createEmptyResultTable();

   int getNumColumns() const { return m_columns->size(); }
   SummaryTableColumn *getColumn(int index) const { return m_columns->get(index); }
   AggregationFunction getAggregationFunction() const { return m_aggregationFunction; }
   const TCHAR *getTableDciName() const { return m_tableDciName; }
   time_t getPeriodStart() const { return m_periodStart; }
   time_t getPeriodEnd() const { return m_periodEnd; }
   bool isMultiInstance() const { return (m_flags & SUMMARY_TABLE_MULTI_INSTANCE) ? true : false; }
   bool isTableDciSource() const { return (m_flags & SUMMARY_TABLE_TABLE_DCI_SOURCE) ? true : false; }

   void createExportRecord(String &xml) const;
};

/**
 * Object-associated URL
 */
class NXCORE_EXPORTABLE ObjectUrl
{
private:
   UINT32 m_id;
   TCHAR *m_url;
   TCHAR *m_description;

public:
   ObjectUrl(NXCPMessage *msg, UINT32 baseId);
   ObjectUrl(DB_RESULT hResult, int row);
   ~ObjectUrl();

   void fillMessage(NXCPMessage *msg, UINT32 baseId);

   UINT32 getId() const { return m_id; }
   const TCHAR *getUrl() const { return m_url; }
   const TCHAR *getDescription() const { return m_description; }

   json_t *toJson() const;
};

/**
 * Base class for network objects
 */
class NXCORE_EXPORTABLE NetObj
{
   DISABLE_COPY_CTOR(NetObj)

private:
	static void onObjectDeleteCallback(NetObj *object, void *data);

	void getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly);

protected:
   UINT32 m_id;
	uuid m_guid;
   time_t m_timestamp;       // Last change time stamp
   UINT32 m_dwRefCount;        // Number of references. Object can be destroyed only when this counter is zero
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR *m_comments;      // User comments
   int m_status;
   int m_statusCalcAlg;      // Status calculation algorithm
   int m_statusPropAlg;      // Status propagation algorithm
   int m_fixedStatus;        // Status if propagation is "Fixed"
   int m_statusShift;        // Shift value for "shifted" status propagation
   int m_statusTranslation[4];
   int m_statusSingleThreshold;
   int m_statusThresholds[4];
   UINT32 m_modified;
   bool m_isDeleted;
   bool m_isHidden;
	bool m_isSystem;
	bool m_maintenanceMode;
	UINT64 m_maintenanceEventId;
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
	ObjectArray<ObjectUrl> *m_urls;  // URLs associated with this object

   ObjectArray<NetObj> *m_childList;     // Array of pointers to child objects
   ObjectArray<NetObj> *m_parentList;    // Array of pointers to parent objects

   AccessList *m_accessList;
   bool m_inheritAccessRights;
   MUTEX m_mutexACL;

   IntegerArray<UINT32> *m_trustedNodes;

	StringMap m_customAttributes;
   StringObjectMap<ModuleData> *m_moduleData;

   void lockProperties() const { MutexLock(m_mutexProperties); }
   void unlockProperties() const { MutexUnlock(m_mutexProperties); }
   void lockACL() { MutexLock(m_mutexACL); }
   void unlockACL() { MutexUnlock(m_mutexACL); }
   void lockParentList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockParentList, INFINITE);
      else
         RWLockReadLock(m_rwlockParentList, INFINITE);
   }
   void unlockParentList() { RWLockUnlock(m_rwlockParentList); }
   void lockChildList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockChildList, INFINITE);
      else
         RWLockReadLock(m_rwlockChildList, INFINITE);
   }
   void unlockChildList() { RWLockUnlock(m_rwlockChildList); }

   void setModified(UINT32 flags, bool notify = true);                  // Used to mark object as modified

   bool loadACLFromDB(DB_HANDLE hdb);
   bool saveACLToDB(DB_HANDLE hdb);
   bool loadCommonProperties(DB_HANDLE hdb);
   bool saveCommonProperties(DB_HANDLE hdb);
	bool loadTrustedNodes(DB_HANDLE hdb);
	bool saveTrustedNodes(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query) { return ExecuteQueryOnObject(hdb, m_id, query); }

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 objectId);

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual void fillMessageInternalStage2(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *msg);

   void addLocationToHistory();
   bool isLocationTableExists(DB_HANDLE hdb);
   bool createLocationHistoryTable(DB_HANDLE hdb);

public:
   NetObj();
   virtual ~NetObj();

   virtual int getObjectClass() const { return OBJECT_GENERIC; }
   virtual const TCHAR *getObjectClassName() const;

   UINT32 getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   int getStatus() const { return m_status; }
   int getPropagatedStatus();
   time_t getTimeStamp() const { return m_timestamp; }
	const uuid& getGuid() const { return m_guid; }
	const TCHAR *getComments() const { return CHECK_NULL_EX(m_comments); }

	const GeoLocation& getGeoLocation() const { return m_geoLocation; }
	void setGeoLocation(const GeoLocation& geoLocation);

   const PostalAddress *getPostalAddress() const { return m_postalAddress; }
   void setPostalAddress(PostalAddress * addr) { lockProperties(); delete m_postalAddress; m_postalAddress = addr; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   const uuid& getMapImage() { return m_image; }
   void setMapImage(const uuid& image) { lockProperties(); m_image = image; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   bool isModified() const { return m_modified != 0; }
   bool isDeleted() const { return m_isDeleted; }
   bool isOrphaned() const { return m_parentList->size() == 0; }
   bool isEmpty() const { return m_childList->size() == 0; }

	bool isSystem() const { return m_isSystem; }
	void setSystemFlag(bool flag) { m_isSystem = flag; }

   UINT32 getRefCount();
   void incRefCount();
   void decRefCount();

   bool isChild(UINT32 id);
	bool isTrustedNode(UINT32 id);

   void addChild(NetObj *object);     // Add reference to child object
   void addParent(NetObj *object);    // Add reference to parent object

   void deleteChild(NetObj *object);  // Delete reference to child object
   void deleteParent(NetObj *object); // Delete reference to parent object

   void deleteObject(NetObj *initiator = NULL);     // Prepare object for deletion

   bool isHidden() { return m_isHidden; }
   void hide();
   void unhide();
   void markAsModified(UINT32 flags) { lockProperties(); setModified(flags); unlockProperties(); }  // external API to mark object as modified

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool saveRuntimeData(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();

   void setId(UINT32 dwId) { m_id = dwId; setModified(MODIFY_ALL); }
   void generateGuid() { m_guid = uuid::generate(); }
   void setName(const TCHAR *pszName) { lockProperties(); _tcslcpy(m_name, pszName, MAX_OBJECT_NAME); setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
   void resetStatus() { m_status = STATUS_UNKNOWN; setModified(MODIFY_RUNTIME); }
   void setComments(TCHAR *text);	/* text must be dynamically allocated */

   bool isInMaintenanceMode() const { return m_maintenanceMode; }
   UINT64 getMaintenanceEventId() const { return m_maintenanceEventId; }
   virtual void enterMaintenanceMode();
   virtual void leaveMaintenanceMode();

   void fillMessage(NXCPMessage *msg, UINT32 userId);
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

   TCHAR *getCustomAttribute(const TCHAR *name, TCHAR *buffer, size_t size) const;
   TCHAR *getCustomAttributeCopy(const TCHAR *name) const;
   NXSL_Value *getCustomAttributeForNXSL(const TCHAR *name) const;
   NXSL_Value *getCustomAttributesForNXSL() const;
   void setCustomAttribute(const TCHAR *name, const TCHAR *value);
   void setCustomAttributePV(const TCHAR *name, TCHAR *value);
   void deleteCustomAttribute(const TCHAR *name);

   virtual NXSL_Value *createNXSLObject();

   ModuleData *getModuleData(const TCHAR *module);
   void setModuleData(const TCHAR *module, ModuleData *data);

	ObjectArray<NetObj> *getParentList(int typeFilter);
	ObjectArray<NetObj> *getChildList(int typeFilter);
	ObjectArray<NetObj> *getFullChildList(bool eventSourceOnly, bool updateRefCount);

   NetObj *findChildObject(const TCHAR *name, int typeFilter);
   Node *findChildNode(const InetAddress& addr);

   int getChildCount() { return m_childList->size(); }
   int getParentCount() { return m_parentList->size(); }

	virtual NXSL_Array *getParentsForNXSL();
	virtual NXSL_Array *getChildrenForNXSL();

	virtual bool showThresholdSummary();
   virtual bool isEventSource();
   virtual bool isDataCollectionTarget();

   void setStatusCalculation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);
   void setStatusPropagation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);

   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...);

   virtual json_t *toJson();

   // Debug methods
   const TCHAR *dbgGetParentList(TCHAR *szBuffer);
   const TCHAR *dbgGetChildList(TCHAR *szBuffer);

   static const TCHAR *getObjectClassName(int objectClass);
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
	NXSL_Program *m_applyFilter;
	RWLOCK m_dciAccessLock;

   virtual void prepareForDeletion();

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void onDataCollectionChange();

   void loadItemsFromDB(DB_HANDLE hdb);
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

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   UINT32 getFlags() const { return m_flags; }
   int getVersionMajor() { return m_dwVersion >> 16; }
   int getVersionMinor() { return m_dwVersion & 0xFFFF; }

   int getItemCount() { return m_dcObjects->size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false);
   bool updateDCObject(UINT32 dwItemId, NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId, UINT32 userId);
   bool deleteDCObject(UINT32 dcObjectId, bool needLock, UINT32 userId);
   bool setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus);
   DCObject *getDCObjectById(UINT32 itemId, UINT32 userId, bool lock = true);
   DCObject *getDCObjectByGUID(const uuid& guid, UINT32 userId, bool lock = true);
   DCObject *getDCObjectByTemplateId(UINT32 tmplItemId, UINT32 userId);
   DCObject *getDCObjectByName(const TCHAR *name, UINT32 userId);
   DCObject *getDCObjectByDescription(const TCHAR *description, UINT32 userId);
   NXSL_Value *getAllDCObjectsForNXSL(const TCHAR *name, const TCHAR *description, UINT32 userId);
   bool lockDCIList(int sessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner);
   bool unlockDCIList(int sessionId);
   void setDCIModificationFlag() { m_dciListModified = true; }
   void sendItemsToClient(ClientSession *pSession, UINT32 dwRqId);
   bool isLockedBySession(int sessionId) { return m_dciLockStatus == sessionId; }
   IntegerArray<UINT32> *getDCIEventsList();
   StringSet *getDCIScriptList();

   BOOL applyToTarget(DataCollectionTarget *pNode);
	AutoBindDecision isApplicable(DataCollectionTarget *object);
	bool isAutoApplyEnabled() { return (m_flags & TF_AUTO_APPLY) ? true : false; }
	bool isAutoRemoveEnabled() { return ((m_flags & (TF_AUTO_APPLY | TF_AUTO_REMOVE)) == (TF_AUTO_APPLY | TF_AUTO_REMOVE)) ? true : false; }
	void setAutoApplyFilter(const TCHAR *filter);
   void queueUpdate();
   void queueRemoveFromTarget(UINT32 targetId, bool removeDCI);

   void createExportRecord(String &str);
   void updateFromImport(ConfigEntry *config);

	bool enumDCObjects(bool (* pfCallback)(DCObject *, UINT32, void *), void *pArg);
	void associateItems();

   UINT32 getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId);
};

class Cluster;

/**
 * Interface class
 */
class NXCORE_EXPORTABLE Interface : public NetObj
{
protected:
   UINT32 m_parentInterfaceId;
   UINT32 m_index;
   BYTE m_macAddr[MAC_ADDR_LENGTH];
   InetAddressList m_ipAddressList;
	UINT32 m_flags;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
	TCHAR m_alias[MAX_DB_STRING];	// Interface alias - value of ifAlias for SNMP, empty for NetXMS agent
   UINT32 m_type;
   UINT32 m_mtu;
   UINT64 m_speed;
	UINT32 m_bridgePortNumber;		 // 802.1D port number
	UINT32 m_slotNumber;				 // Vendor/device specific slot number
	UINT32 m_portNumber;				 // Vendor/device specific port number
	UINT32 m_peerNodeId;				 // ID of peer node object, or 0 if unknown
	UINT32 m_peerInterfaceId;		 // ID of peer interface object, or 0 if unknown
   LinkLayerProtocol m_peerDiscoveryProtocol;  // Protocol used to discover peer node
	INT16 m_adminState;				 // interface administrative state
	INT16 m_operState;				 // interface operational state
   INT16 m_pendingOperState;
	INT16 m_confirmedOperState;
	INT16 m_dot1xPaeAuthState;		 // 802.1x port auth state
	INT16 m_dot1xBackendAuthState; // 802.1x backend auth state
   UINT64 m_lastDownEventId;
	int m_pendingStatus;
	int m_statusPollCount;
	int m_operStatePollCount;
	int m_requiredPollCount;
   UINT32 m_zoneUIN;
   UINT32 m_pingTime;
   time_t m_pingLastTimeStamp;
   int m_ifTableSuffixLen;
   UINT32 *m_ifTableSuffix;

   void icmpStatusPoll(UINT32 rqId, UINT32 nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState);
	void paeStatusPoll(UINT32 rqId, SNMP_Transport *pTransport, Node *node);

protected:
   virtual void onObjectDelete(UINT32 objectId);

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   void setExpectedStateInternal(int state);

public:
   Interface();
   Interface(const InetAddressList& addrList, UINT32 zoneUIN, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, const InetAddressList& addrList, UINT32 ifType, UINT32 zoneUIN);
   virtual ~Interface();

   virtual int getObjectClass() const { return OBJECT_INTERFACE; }
   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

   Node *getParentNode();
   UINT32 getParentNodeId();
   UINT32 getParentInterfaceId() const { return m_parentInterfaceId; }

   const InetAddressList *getIpAddressList() { return &m_ipAddressList; }
   const InetAddress& getFirstIpAddress();
   UINT32 getZoneUIN() const { return m_zoneUIN; }
   UINT32 getIfIndex() const { return m_index; }
   UINT32 getIfType() const { return m_type; }
   UINT32 getMTU() const { return m_mtu; }
   UINT64 getSpeed() const { return m_speed; }
	UINT32 getBridgePortNumber() const { return m_bridgePortNumber; }
	UINT32 getSlotNumber() const { return m_slotNumber; }
	UINT32 getPortNumber() const { return m_portNumber; }
	UINT32 getPeerNodeId() const { return m_peerNodeId; }
	UINT32 getPeerInterfaceId() const { return m_peerInterfaceId; }
   LinkLayerProtocol getPeerDiscoveryProtocol() const { return m_peerDiscoveryProtocol; }
	UINT32 getFlags() const { return m_flags; }
	int getExpectedState() const { return (int)((m_flags & IF_EXPECTED_STATE_MASK) >> 28); }
	int getAdminState() const { return (int)m_adminState; }
	int getOperState() const { return (int)m_operState; }
   int getConfirmedOperState() const { return (int)m_confirmedOperState; }
	int getDot1xPaeAuthState() const { return (int)m_dot1xPaeAuthState; }
	int getDot1xBackendAuthState() const { return (int)m_dot1xBackendAuthState; }
	const TCHAR *getDescription() const { return m_description; }
	const TCHAR *getAlias() const { return m_alias; }
   const BYTE *getMacAddr() const { return m_macAddr; }
   int getIfTableSuffixLen() const { return m_ifTableSuffixLen; }
   const UINT32 *getIfTableSuffix() const { return m_ifTableSuffix; }
   UINT32 getPingTime();
	bool isSyntheticMask() const { return (m_flags & IF_SYNTHETIC_MASK) ? true : false; }
	bool isPhysicalPort() const { return (m_flags & IF_PHYSICAL_PORT) ? true : false; }
	bool isLoopback() const { return (m_flags & IF_LOOPBACK) ? true : false; }
	bool isManuallyCreated() const { return (m_flags & IF_CREATED_MANUALLY) ? true : false; }
	bool isExcludedFromTopology() const { return (m_flags & (IF_EXCLUDE_FROM_TOPOLOGY | IF_LOOPBACK)) ? true : false; }
   bool isFake() const { return (m_index == 1) &&
                                (m_type == IFTYPE_OTHER) &&
                                !_tcscmp(m_name, _T("unknown")); }
   bool isSubInterface() const { return m_parentInterfaceId != 0; }

   UINT64 getLastDownEventId() const { return m_lastDownEventId; }
   void setLastDownEventId(UINT64 id) { m_lastDownEventId = id; }

   void setMacAddr(const BYTE *macAddr, bool updateMacDB);
   void setIpAddress(const InetAddress& addr);
   void setBridgePortNumber(UINT32 bpn) { m_bridgePortNumber = bpn; setModified(MODIFY_INTERFACE_PROPERTIES); }
   void setSlotNumber(UINT32 slot) { m_slotNumber = slot; setModified(MODIFY_INTERFACE_PROPERTIES); }
   void setPortNumber(UINT32 port) { m_portNumber = port; setModified(MODIFY_INTERFACE_PROPERTIES); }
	void setPhysicalPortFlag(bool isPhysical) { if (isPhysical) m_flags |= IF_PHYSICAL_PORT; else m_flags &= ~IF_PHYSICAL_PORT; setModified(MODIFY_INTERFACE_PROPERTIES); }
	void setManualCreationFlag(bool isManual) { if (isManual) m_flags |= IF_CREATED_MANUALLY; else m_flags &= ~IF_CREATED_MANUALLY; setModified(MODIFY_INTERFACE_PROPERTIES); }
	void setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol, bool reflection);
   void clearPeer() { lockProperties(); m_peerNodeId = 0; m_peerInterfaceId = 0; m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN; setModified(MODIFY_INTERFACE_PROPERTIES); unlockProperties(); }
   void setDescription(const TCHAR *descr) { lockProperties(); nx_strncpy(m_description, descr, MAX_DB_STRING); setModified(MODIFY_INTERFACE_PROPERTIES); unlockProperties(); }
   void setAlias(const TCHAR *alias) { lockProperties(); nx_strncpy(m_alias, alias, MAX_DB_STRING); setModified(MODIFY_INTERFACE_PROPERTIES); unlockProperties(); }
   void addIpAddress(const InetAddress& addr);
   void deleteIpAddress(InetAddress addr);
   void setNetMask(const InetAddress& addr);
	void setMTU(int mtu) { m_mtu = mtu; setModified(MODIFY_INTERFACE_PROPERTIES); }
	void setSpeed(UINT64 speed) { m_speed = speed; setModified(MODIFY_INTERFACE_PROPERTIES); }
   void setIfTableSuffix(int len, const UINT32 *suffix) { lockProperties(); free(m_ifTableSuffix); m_ifTableSuffixLen = len; m_ifTableSuffix = (len > 0) ? (UINT32 *)nx_memdup(suffix, len * sizeof(UINT32)) : NULL; setModified(MODIFY_INTERFACE_PROPERTIES); unlockProperties(); }
   void setParentInterface(UINT32 parentInterfaceId) { m_parentInterfaceId = parentInterfaceId; setModified(MODIFY_INTERFACE_PROPERTIES); }

	void updateZoneUIN();

   void statusPoll(ClientSession *session, UINT32 rqId, Queue *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, UINT32 nodeIcmpProxy);

   UINT32 wakeUp();
	void setExpectedState(int state) { lockProperties(); setExpectedStateInternal(state); unlockProperties(); }
   void setExcludeFromTopology(bool excluded);
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
	UINT32 m_pollCount;
	UINT32 m_requiredPollCount;
   UINT32 m_responseTime;  // Response time from last poll

   virtual void onObjectDelete(UINT32 objectId);

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  Node *pHostNode = NULL, UINT32 dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int getObjectClass() const { return OBJECT_NETWORKSERVICE; }

   virtual json_t *toJson();

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   Node *getParentNode();

public:
   VPNConnector();
   VPNConnector(bool hidden);
   virtual ~VPNConnector();

   virtual int getObjectClass() const { return OBJECT_VPNCONNECTOR; }

   virtual json_t *toJson();

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   bool isLocalAddr(const InetAddress& addr);
   bool isRemoteAddr(const InetAddress& addr);
   UINT32 getPeerGatewayId() const { return m_dwPeerGateway; }
   InetAddress getPeerGatewayAddr();
};

/**
 * Data collection proxy information structure
 */
struct ProxyInfo
{
   UINT32 proxyId;
   NXCPMessage *msg;
   UINT32 fieldId;
   UINT32 count;
   UINT32 nodeInfoFieldId;
   UINT32 nodeInfoCount;
};

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public Template
{
protected:
   IntegerArray<UINT32> *m_deletedItems;
   IntegerArray<UINT32> *m_deletedTables;
   UINT32 m_pingTime;
   time_t m_pingLastTimeStamp;
   MUTEX m_hPollerMutex;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
	virtual void fillMessageInternalStage2(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	virtual bool isDataCollectionDisabled();
   virtual void updatePingData();

   void pollerLock() { MutexLock(m_hPollerMutex); }
   void pollerUnlock() { MutexUnlock(m_hPollerMutex); }

   NetObj *objectFromParameter(const TCHAR *param);

   NXSL_VM *runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject);

   void applyUserTemplates();
   void updateContainerMembership();

   void getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);
   void getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);

   void addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco);
   void addProxySnmpTarget(ProxyInfo *info, const Node *node);
   virtual void collectProxyInfo(ProxyInfo *info);
   static void collectProxyInfoCallback(NetObj *object, void *data);

public:
   DataCollectionTarget();
   DataCollectionTarget(const TCHAR *name);
   virtual ~DataCollectionTarget();

   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
   virtual bool isDataCollectionTarget();

   virtual void enterMaintenanceMode();
   virtual void leaveMaintenanceMode();

   virtual UINT32 getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);
   virtual UINT32 getScriptItem(const TCHAR *param, size_t bufSize, TCHAR *buffer, DataCollectionTarget *targetObject);
   virtual UINT32 getScriptTable(const TCHAR *param, Table **result, DataCollectionTarget *targetObject);

   virtual UINT32 getEffectiveSourceNode(DCObject *dco);

   virtual json_t *toJson();

   UINT32 getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject);
   UINT32 getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject);

   UINT32 getTableLastValues(UINT32 dciId, NXCPMessage *msg);
	UINT32 getThresholdSummary(NXCPMessage *msg, UINT32 baseId, UINT32 userId);
	UINT32 getPerfTabDCIList(NXCPMessage *pMsg, UINT32 userId);
   void getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);

   void updateDciCache();
   void updateDCItemCacheSize(UINT32 dciId, UINT32 conditionId = 0);
   void cleanDCIData(DB_HANDLE hdb);
   void queueItemsForPolling();
	bool processNewDCValue(DCObject *dco, time_t currTime, const void *value);
	void scheduleItemDataCleanup(UINT32 dciId);
   void scheduleTableDataCleanup(UINT32 dciId);

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   virtual int getObjectClass() const { return OBJECT_MOBILEDEVICE; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

	void updateSystemInfo(NXCPMessage *msg);
	void updateStatus(NXCPMessage *msg);

	const TCHAR *getDeviceId() { return CHECK_NULL_EX(m_deviceId); }
	const TCHAR *getVendor() { return CHECK_NULL_EX(m_vendor); }
	const TCHAR *getModel() { return CHECK_NULL_EX(m_model); }
	const TCHAR *getSerialNumber() { return CHECK_NULL_EX(m_serialNumber); }
	const TCHAR *getOsName() { return CHECK_NULL_EX(m_osName); }
	const TCHAR *getOsVersion() { return CHECK_NULL_EX(m_osVersion); }
	const TCHAR *getUserId() { return CHECK_NULL_EX(m_userId); }
	const LONG getBatteryLevel() { return m_batteryLevel; }

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void updatePingData();

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, UINT32 index, const BYTE *macAddr);
   virtual ~AccessPoint();

   virtual int getObjectClass() const { return OBJECT_ACCESSPOINT; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual json_t *toJson();

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
   void setIpAddress(const InetAddress& addr) { lockProperties(); m_ipAddress = addr; setModified(MODIFY_OTHER); unlockProperties(); }
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
	time_t m_lastStatusPoll;
   time_t m_lastConfigurationPoll;
	UINT32 m_zoneUIN;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void onDataCollectionChange();

   UINT32 getResourceOwnerInternal(UINT32 id, const TCHAR *name);

public:
	Cluster();
   Cluster(const TCHAR *pszName, UINT32 zoneUIN);
	virtual ~Cluster();

   virtual int getObjectClass() const { return OBJECT_CLUSTER; }
   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual bool showThresholdSummary();

   virtual void unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI);

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

	bool isSyncAddr(const InetAddress& addr);
	bool isVirtualAddr(const InetAddress& addr);
	bool isResourceOnNode(UINT32 dwResource, UINT32 dwNode);
	UINT32 getResourceOwner(UINT32 resourceId) { return getResourceOwnerInternal(resourceId, NULL); }
   UINT32 getResourceOwner(const TCHAR *resourceName) { return getResourceOwnerInternal(0, resourceName); }
   UINT32 getZoneUIN() const { return m_zoneUIN; }

   void statusPoll(PollerInfo *poller);
   void statusPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);
   void lockForStatusPoll() { m_flags |= CLF_QUEUED_FOR_STATUS_POLL; }
   bool isReadyForStatusPoll()
   {
      return ((m_status != STATUS_UNMANAGED) && (!m_isDeleted) &&
              (!(m_flags & CLF_QUEUED_FOR_STATUS_POLL)) &&
              ((UINT32)time(NULL) - (UINT32)m_lastStatusPoll > g_dwStatusPollingInterval))
                  ? true : false;
   }

   void configurationPoll(PollerInfo *poller);
   void configurationPoll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller);
   void lockForConfigurationPoll() { m_flags |= CLF_QUEUED_FOR_CONFIGURATION_POLL; }
   bool isReadyForConfigurationPoll()
   {
      return ((m_status != STATUS_UNMANAGED) && (!m_isDeleted) &&
              (!(m_flags & CLF_QUEUED_FOR_CONFIGURATION_POLL)) &&
              ((UINT32)time(NULL) - (UINT32)m_lastConfigurationPoll > g_dwConfigurationPollingInterval))
                  ? true : false;
   }

   UINT32 collectAggregatedData(DCItem *item, TCHAR *buffer);
   UINT32 collectAggregatedData(DCTable *table, Table **result);

   NXSL_Array *getNodesForNXSL();
};

/**
 * Rack orientation
 */
enum RackOrientation
{
   FILL = 0,
   FRONT = 1,
   REAR = 2
};

/**
 * Chassis (represents physical chassis)
 */
class NXCORE_EXPORTABLE Chassis : public DataCollectionTarget
{
protected:
   UINT32 m_controllerId;
   INT16 m_rackHeight;
   INT16 m_rackPosition;
   UINT32 m_rackId;
   uuid m_rackImage;
   RackOrientation m_rackOrientation;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request);

   virtual void onDataCollectionChange();
   virtual void collectProxyInfo(ProxyInfo *info);

   void updateRackBinding();
   void updateControllerBinding();

public:
   Chassis();
   Chassis(const TCHAR *name, UINT32 controllerId);
   virtual ~Chassis();

   virtual int getObjectClass() const { return OBJECT_CHASSIS; }
   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();
   virtual bool showThresholdSummary();
   virtual UINT32 getEffectiveSourceNode(DCObject *dco);

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

   UINT32 getControllerId() const { return m_controllerId; }
   UINT32 getRackId() const { return m_rackId; }
   INT16 getRackHeight() const { return m_rackHeight; }
   INT16 getRackPosition() const { return m_rackPosition; }
   bool bindUnderController() { return (m_flags & CHF_BIND_UNDER_CONTROLLER) ? true : false; }

   void setBindUnderController(bool doBind);
};


class Subnet;
struct ProxyInfo;

/**
 * Node subtypes
 */
enum NodeType
{
   NODE_TYPE_UNKNOWN = 0,
   NODE_TYPE_PHYSICAL = 1,
   NODE_TYPE_VIRTUAL = 2,
   NODE_TYPE_CONTROLLER = 3
};

/**
 * Node agent compression modes
 */
enum NodeAgentCompressionMode
{
   NODE_AGENT_COMPRESSION_DEFAULT = 0,
   NODE_AGENT_COMPRESSION_ENABLED = 1,
   NODE_AGENT_COMPRESSION_DISABLED = 2
};

/**
 * Routing loop event information
 */
class RoutingLoopEvent
{
private:
   InetAddress m_address;
   UINT32 m_nodeId;
   UINT64 m_eventId;

public:
   RoutingLoopEvent(const InetAddress& address, UINT32 nodeId, UINT64 eventId)
   {
      m_address = address;
      m_nodeId = nodeId;
      m_eventId = eventId;
   }

   const InetAddress& getAddress() const { return m_address; }
   UINT32 getNodeId() const { return m_nodeId; }
   UINT64 getEventId() const { return m_eventId; }
};

enum ProxyType
{
   SNMP_PROXY = 0,
   ZONE_PROXY = 1,
   MAX_PROXY_TYPE = 2
};

/**
 * Node
 */
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
	friend class Subnet;

private:
	/**
	 * Delete agent connection
	 */
	void deleteAgentConnection()
	{
	   if (m_agentConnection != NULL)
	   {
	      m_agentConnection->decRefCount();
	      m_agentConnection = NULL;
	   }
	}

	void onSnmpProxyChange(UINT32 oldProxy);

   static void onDataCollectionChangeAsyncCallback(void *arg);

protected:
   InetAddress m_ipAddress;
	TCHAR m_primaryName[MAX_DNS_NAME];
	uuid m_tunnelId;
   UINT32 m_dwDynamicFlags;       // Flags used at runtime by server
   NodeType m_type;
   TCHAR m_subType[MAX_NODE_SUBTYPE_LENGTH];
	int m_pendingState;
	UINT32 m_pollCountAgent;
	UINT32 m_pollCountSNMP;
   UINT32 m_pollCountAllDown;
	UINT32 m_requiredPollCount;
   UINT32 m_zoneUIN;
   UINT16 m_agentPort;
   INT16 m_agentAuthMethod;
   INT16 m_agentCacheMode;
   INT16 m_agentCompressionMode;  // agent compression mode (enabled/disabled/default)
   TCHAR m_szSharedSecret[MAX_SECRET_LENGTH];
   INT16 m_iStatusPollType;
   INT16 m_snmpVersion;
   UINT16 m_snmpPort;
	UINT16 m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
   TCHAR m_agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_platformName[MAX_PLATFORM_NAME_LEN];
   TCHAR m_snmpObjectId[MAX_OID_LEN * 4];
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
   time_t m_lastAgentCommTime;
   time_t m_lastAgentConnectAttempt;
   MUTEX m_hAgentAccessMutex;
   MUTEX m_hSmclpAccessMutex;
   MUTEX m_mutexRTAccess;
	MUTEX m_mutexTopoAccess;
   AgentConnectionEx *m_agentConnection;
   ObjectLock<AgentConnectionEx> *m_proxyConnections;
   SMCLP_Connection *m_smclpConnection;
	UINT64 m_lastAgentTrapId;	     // ID of last received agent trap
   UINT64 m_lastAgentPushRequestId; // ID of last received agent push request
   UINT32 m_lastSNMPTrapId;
   UINT64 m_lastSyslogMessageId; // ID of last received syslog message
   UINT32 m_pollerNode;      // Node used for network service polling
   UINT32 m_agentProxy;      // Node used as proxy for agent connection
	UINT32 m_snmpProxy;       // Node used as proxy for SNMP requests
   UINT32 m_icmpProxy;       // Node used as proxy for ICMP ping
   UINT64 m_lastEvents[MAX_LAST_EVENTS];
   ObjectArray<RoutingLoopEvent> *m_routingLoopEvents;
   ROUTING_TABLE *m_pRoutingTable;
	ForwardingDatabase *m_fdb;
	LinkLayerNeighbors *m_linkLayerNeighbors;
	VlanList *m_vlans;
	VrrpInfo *m_vrrpInfo;
	ObjectArray<WirelessStationInfo> *m_wirelessStations;
	int m_adoptedApCount;
	int m_totalApCount;
	BYTE m_baseBridgeAddress[MAC_ADDR_LENGTH];	// Bridge base address (dot1dBaseBridgeAddress in bridge MIB)
	NetworkMapObjectList *m_topology;
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
	UINT32 m_chassisId;
	INT64 m_syslogMessageCount;
	INT64 m_snmpTrapCount;
	TCHAR m_sshLogin[MAX_SSH_LOGIN_LEN];
	TCHAR m_sshPassword[MAX_SSH_PASSWORD_LEN];
	UINT32 m_sshProxy;
	UINT32 m_portNumberingScheme;
	UINT32 m_portRowCount;
	RackOrientation m_rackOrientation;

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
	void setPrimaryIPAddress(const InetAddress& addr);

   bool setAgentProxy(AgentConnectionEx *conn);
   bool isAgentCompressionAllowed();

   UINT32 getInterfaceCount(Interface **ppInterface);

   void checkInterfaceNames(InterfaceList *pIfList);
   bool filterInterface(InterfaceInfo *info);
   Subnet *createSubnet(InetAddress& baseAddr, bool syntheticMask);
	void checkAgentPolicyBinding(AgentConnection *conn);
	void updatePrimaryIpAddr();
	bool confPollAgent(UINT32 dwRqId);
	bool confPollSnmp(UINT32 dwRqId);
	NodeType detectNodeType();
	bool updateSoftwarePackages(PollerInfo *poller, UINT32 requestId);
	bool querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, UINT32 pollRqId, TCHAR **value);
	void checkBridgeMib(SNMP_Transport *pTransport);
	void checkIfXTable(SNMP_Transport *pTransport);
	void executeHookScript(const TCHAR *hookName);
	bool checkNetworkPath(UINT32 requestId);
   bool checkNetworkPathLayer2(UINT32 requestId, bool secondPass);
   bool checkNetworkPathLayer3(UINT32 requestId, bool secondPass);
	bool checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 requestId, bool secondPass);

	void doInstanceDiscovery(UINT32 requestId);
	StringMap *getInstanceList(DCObject *dco);
	bool updateInstances(DCObject *root, StringMap *instances, UINT32 requestId);
   void syncDataCollectionWithAgent(AgentConnectionEx *conn);

	bool updateInterfaceConfiguration(UINT32 rqid, int maskBits);
   bool deleteDuplicateInterfaces(UINT32 rqid);
   void updatePhysicalContainerBinding(int containerClass, UINT32 containerId);

   bool connectToAgent(UINT32 *error = NULL, UINT32 *socketError = NULL, bool *newConnection = NULL, bool forceConnect = false);
   void setLastAgentCommTime() { m_lastAgentCommTime = time(NULL); }

	void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedObject, bool vpnLink, bool includeEndNodes);

	virtual bool isDataCollectionDisabled();
   virtual void collectProxyInfo(ProxyInfo *info);

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 objectId);

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

   virtual void updatePingData();

   virtual void onDataCollectionChange();

public:
   Node();
   Node(const InetAddress& addr, UINT32 dwFlags, UINT32 agentProxy, UINT32 snmpProxy, UINT32 icmpProxy, UINT32 sshProxy, UINT32 zoneUIN);
   virtual ~Node();

   virtual int getObjectClass() const { return OBJECT_NODE; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool saveRuntimeData(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

	TCHAR *expandText(const TCHAR *textTemplate, StringMap *inputFields, const TCHAR *userName);

	Cluster *getMyCluster();

   const InetAddress& getIpAddress() const { return m_ipAddress; }
   UINT32 getZoneUIN() const { return m_zoneUIN; }
   NodeType getType() const { return m_type; }
   const TCHAR *getSubType() const { return m_subType; }
   UINT32 getRuntimeFlags() const { return m_dwDynamicFlags; }

   void setFlag(UINT32 flag) { lockProperties(); m_flags |= flag; setModified(MODIFY_NODE_PROPERTIES); unlockProperties(); }
   void clearFlag(UINT32 flag) { lockProperties(); m_flags &= ~flag; setModified(MODIFY_NODE_PROPERTIES); unlockProperties(); }
   void setLocalMgmtFlag() { setFlag(NF_IS_LOCAL_MGMT); }
   void clearLocalMgmtFlag() { clearFlag(NF_IS_LOCAL_MGMT); }

   void setType(NodeType type, const TCHAR *subType) { lockProperties(); m_type = type; nx_strncpy(m_subType, subType, MAX_NODE_SUBTYPE_LENGTH); unlockProperties(); }

   bool isSNMPSupported() const { return m_flags & NF_IS_SNMP ? true : false; }
   bool isNativeAgent() const { return m_flags & NF_IS_NATIVE_AGENT ? true : false; }
   bool isBridge() const { return m_flags & NF_IS_BRIDGE ? true : false; }
   bool isRouter() const { return m_flags & NF_IS_ROUTER ? true : false; }
   bool isLocalManagement() const { return m_flags & NF_IS_LOCAL_MGMT ? true : false; }
	bool isPerVlanFdbSupported() const { return (m_driver != NULL) ? m_driver->isPerVlanFdbSupported() : false; }
	bool isWirelessController() const { return m_flags & NF_IS_WIFI_CONTROLLER ? true : false; }

	const TCHAR *getAgentVersion() const { return m_agentVersion; }
	const TCHAR *getPlatformName() const { return m_platformName; }
   INT16 getSNMPVersion() const { return m_snmpVersion; }
   UINT16 getSNMPPort() const { return m_snmpPort; }
   const TCHAR *getSNMPObjectId() const { return m_snmpObjectId; }
	const TCHAR *getSysName() const { return CHECK_NULL_EX(m_sysName); }
	const TCHAR *getSysDescription() const { return CHECK_NULL_EX(m_sysDescription); }
   const TCHAR *getSysContact() const { return CHECK_NULL_EX(m_sysContact); }
   const TCHAR *getSysLocation() const { return CHECK_NULL_EX(m_sysLocation); }
   time_t getBootTime() const { return m_bootTime; }
	const TCHAR *getLLDPNodeId() const { return m_lldpNodeId; }
   const BYTE *getBridgeId() const { return m_baseBridgeAddress; }
	const TCHAR *getDriverName() const { return (m_driver != NULL) ? m_driver->getName() : _T("GENERIC"); }
	UINT16 getAgentPort() const { return m_agentPort; }
	INT16 getAgentAuthMethod() const { return m_agentAuthMethod; }
   INT16 getAgentCacheMode() const { return (m_dwDynamicFlags & NDF_CACHE_MODE_NOT_SUPPORTED) ? AGENT_CACHE_OFF : ((m_agentCacheMode == AGENT_CACHE_DEFAULT) ? g_defaultAgentCacheMode : m_agentCacheMode); }
	const TCHAR *getSharedSecret() const { return m_szSharedSecret; }
	UINT32 getRackId() const { return m_rackId; }
   INT16 getRackHeight() const { return m_rackHeight; }
   INT16 getRackPosition() const { return m_rackPosition; }
	bool hasFileUpdateConnection() const { lockProperties(); bool result = (m_fileUpdateConn != NULL); unlockProperties(); return result; }
   UINT32 getIcmpProxy() const { return m_icmpProxy; }
   const TCHAR *getSshLogin() const { return m_sshLogin; }
   const TCHAR *getSshPassword() const { return m_sshPassword; }
   UINT32 getSshProxy() const { return m_sshProxy; }
   time_t getLastAgentCommTime() const { return m_lastAgentCommTime; }
   const TCHAR *getPrimaryName() const { return m_primaryName; }
   const uuid& getTunnelId() const { return m_tunnelId; }
   UINT32 getRequiredPollCount() const { return m_requiredPollCount; }

   bool isDown() { return (m_dwDynamicFlags & NDF_UNREACHABLE) ? true : false; }
	time_t getDownTime() const { return m_downSince; }

   void setNewTunnelBindFlag() { lockProperties(); m_dwDynamicFlags |= NDF_NEW_TUNNEL_BIND; unlockProperties(); }
   void clearNewTunnelBindFlag() { lockProperties(); m_dwDynamicFlags &= ~NDF_NEW_TUNNEL_BIND; unlockProperties(); }

   void addInterface(Interface *pInterface) { addChild(pInterface); pInterface->addParent(this); }
   Interface *createNewInterface(InterfaceInfo *ifInfo, bool manuallyCreated, bool fakeInterface);
   Interface *createNewInterface(const InetAddress& ipAddr, BYTE *macAddr, bool fakeInterface);
   void deleteInterface(Interface *iface);

	void setPrimaryName(const TCHAR *name) { nx_strncpy(m_primaryName, name, MAX_DNS_NAME); }
	void setAgentPort(UINT16 port) { m_agentPort = port; }
	void setSnmpPort(UINT16 port) { m_snmpPort = port; }
   void setSshCredentials(const TCHAR *login, const TCHAR *password);
   void changeIPAddress(const InetAddress& ipAddr);
	void changeZone(UINT32 newZone);
	void setTunnelId(const uuid& tunnelId);
	void setFileUpdateConnection(AgentConnection *conn);
   void clearDataCollectionConfigFromAgent(AgentConnectionEx *conn);
   void forceSyncDataCollectionConfig();
   void relatedNodeDataCollectionChanged() { onDataCollectionChange(); }

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
   bool getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, UINT32 *ifIndex, bool *isVpn, TCHAR *name);
   bool getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, UINT32 *srcIfIndex);
	ComponentTree *getComponents();
   bool getLldpLocalPortInfo(UINT32 idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port);
   void showLLDPInfo(CONSOLE_CTX console);

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
   void setChassis(UINT32 chassisId);

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

	bool checkAgentTrapId(UINT64 id);
	bool checkSNMPTrapId(UINT32 id);
   bool checkSyslogMessageId(UINT64 id);
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
   UINT32 getItemForClient(int iOrigin, UINT32 userId, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize);
   UINT32 getTableForClient(const TCHAR *name, Table **table);
   UINT32 getItemFromSMCLP(const TCHAR *param, UINT32 bufSize, TCHAR *buffer);

	virtual NXSL_Array *getParentsForNXSL();
	virtual NXSL_Array *getTemplatesForNXSL();
	NXSL_Array *getInterfacesForNXSL();

   void openParamList(ObjectArray<AgentParameterDefinition> **paramList);
   void closeParamList() { unlockProperties(); }

   void openTableList(ObjectArray<AgentTableDefinition> **tableList);
   void closeTableList() { unlockProperties(); }

   AgentConnectionEx *createAgentConnection(bool sendServerId = false);
   AgentConnectionEx *getAgentConnection();
   AgentConnectionEx *getConnectionToZoneNodeProxy(bool validate = false);
   AgentConnectionEx *acquireProxyConnection(ProxyType type, bool validate = false);
	SNMP_Transport *createSnmpTransport(WORD port = 0, const TCHAR *context = NULL);
	SNMP_SecurityContext *getSnmpSecurityContext() const;
   UINT32 getEffectiveSnmpProxy() const;

   void writeParamListToMessage(NXCPMessage *pMsg, WORD flags);
	void writeWinPerfObjectsToMessage(NXCPMessage *msg);
	void writePackageListToMessage(NXCPMessage *msg);
	void writeWsListToMessage(NXCPMessage *msg);

   UINT32 wakeUp();

   void addService(NetworkService *pNetSrv) { addChild(pNetSrv); pNetSrv->addParent(this); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL, UINT32 *responseTime = NULL);

   UINT64 getLastEventId(int index) { return ((index >= 0) && (index < MAX_LAST_EVENTS)) ? m_lastEvents[index] : 0; }
   void setLastEventId(int index, UINT64 eventId) { if ((index >= 0) && (index < MAX_LAST_EVENTS)) m_lastEvents[index] = eventId; }
   void setRoutingLoopEvent(const InetAddress& address, UINT32 nodeId, UINT64 eventId);

   UINT32 callSnmpEnumerate(const TCHAR *pszRootOid,
      UINT32 (* pHandler)(SNMP_Variable *, SNMP_Transport *, void *), void *pArg, const TCHAR *context = NULL);

   NetworkMapObjectList *getL2Topology();
   NetworkMapObjectList *buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes);
	ForwardingDatabase *getSwitchForwardingDatabase();
	NetObj *findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type);
	void addHostConnections(LinkLayerNeighbors *nbs);
	void addExistingConnections(LinkLayerNeighbors *nbs);

	NetworkMapObjectList *buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes);

	ServerJobQueue *getJobQueue() { return m_jobQueue; }
	int getJobCount(const TCHAR *type = NULL) { return m_jobQueue->getJobCount(type); }

	DriverData *getDriverData() { return m_driverData; }
	void setDriverData(DriverData *data) { m_driverData = data; }

	void incSyslogMessageCount();
	void incSnmpTrapCount();

	static const TCHAR *typeName(NodeType type);
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
   return (m_status != STATUS_UNMANAGED) &&
	       (!(m_flags & NF_DISABLE_STATUS_POLL)) &&
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
   return (m_status != STATUS_UNMANAGED) &&
	       (!(m_flags & NF_DISABLE_CONF_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_CONFIG_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          ((UINT32)(time(NULL) - m_lastConfigurationPoll) > g_dwConfigurationPollingInterval);
}

inline bool Node::isReadyForDiscoveryPoll()
{
	if (m_isDeleted)
		return false;
   return (g_flags & AF_ENABLE_NETWORK_DISCOVERY) &&
          (m_status != STATUS_UNMANAGED) &&
			 (!(m_flags & NF_DISABLE_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)(time(NULL) - m_lastDiscoveryPoll) > g_dwDiscoveryPollingInterval);
}

inline bool Node::isReadyForRoutePoll()
{
	if (m_isDeleted)
		return false;
   return (m_status != STATUS_UNMANAGED) &&
	       (!(m_flags & NF_DISABLE_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)(time(NULL) - m_lastRTUpdate) > g_dwRoutingTableUpdateInterval);
}

inline bool Node::isReadyForTopologyPoll()
{
	if (m_isDeleted)
		return false;
   return (m_status != STATUS_UNMANAGED) &&
	       (!(m_flags & NF_DISABLE_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((UINT32)(time(NULL) - m_lastTopologyPoll) > g_dwTopologyPollingInterval);
}

inline bool Node::isReadyForInstancePoll()
{
	if (m_isDeleted)
		return false;
   return (m_status != STATUS_UNMANAGED) &&
	       (!(m_flags & NF_DISABLE_CONF_POLL)) &&
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
	friend void Node::buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedSubnet, bool vpnLink, bool includeEndNodes);

protected:
   InetAddress m_ipAddress;
   UINT32 m_zoneUIN;
	bool m_bSyntheticMask;

   virtual void prepareForDeletion();

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);

   void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes);

public:
   Subnet();
   Subnet(const InetAddress& addr, UINT32 zoneUIN, bool bSyntheticMask);
   virtual ~Subnet();

   virtual int getObjectClass() const { return OBJECT_SUBNET; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

   void addNode(Node *node) { addChild(node); node->addParent(this); calculateCompoundStatus(TRUE); }

	virtual bool showThresholdSummary();

   const InetAddress& getIpAddress() const { return m_ipAddress; }
   UINT32 getZoneUIN() const { return m_zoneUIN; }
	bool isSyntheticMask() const { return m_bSyntheticMask; }

	void setCorrectMask(const InetAddress& addr);

	bool findMacAddress(const InetAddress& ipAddr, BYTE *macAddr);

   UINT32 *buildAddressMap(int *length);
};

/**
 * Universal root object
 */
class NXCORE_EXPORTABLE UniversalRoot : public NetObj
{
   using NetObj::loadFromDatabase;

public:
   UniversalRoot();
   virtual ~UniversalRoot();

   virtual bool saveToDatabase(DB_HANDLE hdb);
   void loadFromDatabase(DB_HANDLE hdb);
   virtual void linkObjects();
   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }
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
	NXSL_Program *m_bindFilter;
	TCHAR *m_bindFilterSource;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request);

   void setAutoBindFilterInternal(const TCHAR *script);

public:
   Container();
   Container(const TCHAR *pszName, UINT32 dwCategory);
   virtual ~Container();

   virtual int getObjectClass() const { return OBJECT_CONTAINER; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();

	virtual bool showThresholdSummary();

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual json_t *toJson();

   virtual NXSL_Value *createNXSLObject();

   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }

   AutoBindDecision isSuitableForObject(NetObj *object);
	bool isAutoBindEnabled() const { return (m_flags & CF_AUTO_BIND) ? true : false; }
	bool isAutoUnbindEnabled() const { return ((m_flags & (CF_AUTO_BIND | CF_AUTO_UNBIND)) == (CF_AUTO_BIND | CF_AUTO_UNBIND)) ? true : false; }
	const TCHAR *getAutoBindScriptSource() const { return m_bindFilterSource; }

	void setAutoBindFilter(const TCHAR *script) { lockProperties(); setAutoBindFilterInternal(script); unlockProperties(); }
	void setAutoBindMode(bool doBind, bool doUnbind);
};

/**
 * Template group object
 */
class NXCORE_EXPORTABLE TemplateGroup : public Container
{
public:
   TemplateGroup() : Container() { }
   TemplateGroup(const TCHAR *pszName) : Container(pszName, 0) { m_status = STATUS_NORMAL; }
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
	bool m_topBottomNumbering;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Rack();
   Rack(const TCHAR *name, int height);
   virtual ~Rack();

   virtual int getObjectClass() const { return OBJECT_RACK; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();
};

/**
 * Zone object
 */
class NXCORE_EXPORTABLE Zone : public NetObj
{
protected:
   UINT32 m_uin;
   UINT32 m_proxyNodeId;
	InetAddressIndex *m_idxNodeByAddr;
	InetAddressIndex *m_idxInterfaceByAddr;
	InetAddressIndex *m_idxSubnetByAddr;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request);

public:
   Zone();
   Zone(UINT32 uin, const TCHAR *name);
   virtual ~Zone();

   virtual int getObjectClass() const { return OBJECT_ZONE; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

	virtual bool showThresholdSummary();

   virtual NXSL_Value *createNXSLObject();

   virtual json_t *toJson();

   UINT32 getUIN() const { return m_uin; }
	UINT32 getProxyNodeId() const { return m_proxyNodeId; }

   void addSubnet(Subnet *pSubnet) { addChild(pSubnet); pSubnet->addParent(this); }

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
   ObjectArray<NetObj> *getSubnets(bool updateRefCount) { return m_idxSubnetByAddr->getObjects(updateRefCount); }
   void dumpInterfaceIndex(CONSOLE_CTX console);
   void dumpNodeIndex(CONSOLE_CTX console);
   void dumpSubnetIndex(CONSOLE_CTX console);
};

/**
 * Entire network
 */
class NXCORE_EXPORTABLE Network : public NetObj
{
   using NetObj::loadFromDatabase;

public:
   Network();
   virtual ~Network();

   virtual int getObjectClass() const { return OBJECT_NETWORK; }
   virtual bool saveToDatabase(DB_HANDLE hdb);

	virtual bool showThresholdSummary();

   void AddSubnet(Subnet *pSubnet) { addChild(pSubnet); pSubnet->addParent(this); }
   void AddZone(Zone *pZone) { addChild(pZone); pZone->addParent(this); }
   void loadFromDatabase(DB_HANDLE hdb);
};

/**
 * Condition
 */
class NXCORE_EXPORTABLE ConditionObject : public NetObj
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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   ConditionObject();
   ConditionObject(bool hidden);
   virtual ~ConditionObject();

   virtual int getObjectClass() const { return OBJECT_CONDITION; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

   void lockForPoll();
   void doPoll(PollerInfo *poller);
   void check();

   bool isReadyForPoll()
   {
      return ((m_status != STATUS_UNMANAGED) &&
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

	bool savePolicyCommonProperties(DB_HANDLE hdb);

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   AgentPolicy(int type);
   AgentPolicy(const TCHAR *name, int type);

   virtual int getObjectClass() const { return OBJECT_AGENTPOLICY; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

	virtual bool createDeploymentMessage(NXCPMessage *msg);
	virtual bool createUninstallMessage(NXCPMessage *msg);

	void linkNode(Node *node) { addChild(node); node->addParent(this); }
	void unlinkNode(Node *node) { deleteChild(node); node->deleteParent(this); }
};

/**
 * Agent config policy object
 */
class NXCORE_EXPORTABLE AgentPolicyConfig : public AgentPolicy
{
protected:
	TCHAR *m_fileContent;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   AgentPolicyConfig();
   AgentPolicyConfig(const TCHAR *name);
   virtual ~AgentPolicyConfig();

   virtual int getObjectClass() const { return OBJECT_AGENTPOLICY_CONFIG; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

	virtual bool createDeploymentMessage(NXCPMessage *msg);
	virtual bool createUninstallMessage(NXCPMessage *msg);
};

/**
 * Log parser configuration policy object
 */
class NXCORE_EXPORTABLE AgentPolicyLogParser : public AgentPolicy
{
 protected:
   TCHAR *m_fileContent;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);
 public:
   AgentPolicyLogParser();
   AgentPolicyLogParser(const TCHAR *name);
   virtual ~AgentPolicyLogParser();

   virtual int getObjectClass() const { return OBJECT_AGENTPOLICY_LOGPARSER; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

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
	IntegerArray<UINT32> *m_seedObjects;
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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void updateObjects(NetworkMapObjectList *objects);
	UINT32 objectIdFromElementId(UINT32 eid);
	UINT32 elementIdFromObjectId(UINT32 eid);

   void setFilter(const TCHAR *filter);

public:
   NetworkMap();
	NetworkMap(int type, IntegerArray<UINT32> *seeds);
   virtual ~NetworkMap();

   virtual int getObjectClass() const { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual void onObjectDelete(UINT32 objectId);

   virtual json_t *toJson();

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
class NXCORE_EXPORTABLE DashboardElement
{
public:
	int m_type;
	TCHAR *m_data;
	TCHAR *m_layout;

	DashboardElement() { m_data = NULL; m_layout = NULL; }
	~DashboardElement() { free(m_data); free(m_layout); }

	json_t *toJson()
	{
	   json_t *root = json_object();
	   json_object_set_new(root, "type", json_integer(m_type));
	   json_object_set_new(root, "data", json_string_t(m_data));
	   json_object_set_new(root, "layout", json_string_t(m_layout));
	   return root;
	}
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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
   Dashboard();
   Dashboard(const TCHAR *name);
   virtual ~Dashboard();

   virtual int getObjectClass() const { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   virtual json_t *toJson();

	virtual bool showThresholdSummary();
};

/**
 * Dashboard group object
 */
class NXCORE_EXPORTABLE DashboardGroup : public Container
{
public:
   DashboardGroup() : Container() { }
   DashboardGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~DashboardGroup() { }

   virtual int getObjectClass() const { return OBJECT_DASHBOARDGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
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

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);
	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void initServiceContainer();
	BOOL addHistoryRecord();
	double getUptimeFromDBFor(Period period, INT32 *downtime);

public:
	ServiceContainer();
	ServiceContainer(const TCHAR *pszName);

	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
	virtual bool saveToDatabase(DB_HANDLE hdb);
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
   using ServiceContainer::loadFromDatabase;

public:
	BusinessServiceRoot();
	virtual ~BusinessServiceRoot();

	virtual int getObjectClass() const { return OBJECT_BUSINESSSERVICEROOT; }

	virtual bool saveToDatabase(DB_HANDLE hdb);
   void loadFromDatabase(DB_HANDLE hdb);

   virtual void linkObjects();

   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }
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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

public:
	BusinessService();
	BusinessService(const TCHAR *name);
	virtual ~BusinessService();

	virtual int getObjectClass() const { return OBJECT_BUSINESSSERVICE; }

	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
	virtual bool saveToDatabase(DB_HANDLE hdb);
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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId);
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest);

	void applyTemplate(SlmCheck *tmpl);

public:
	NodeLink();
	NodeLink(const TCHAR *name, UINT32 nodeId);
	virtual ~NodeLink();

	virtual int getObjectClass() const { return OBJECT_NODELINK; }

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);
	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

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
   if (object->getObjectClass() == OBJECT_INTERFACE)
      return ((Interface *)object)->getIpAddressList()->getFirstUnicastAddress();
   return InetAddress::INVALID;
}

/**
 * Functions
 */
void ObjectsInit();

void NXCORE_EXPORTABLE NetObjInsert(NetObj *object, bool newObject, bool importedObject);
void NetObjDeleteFromIndexes(NetObj *object);
void NetObjDelete(NetObj *object);

void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Interface *iface);
ComponentTree *BuildComponentTree(Node *node, SNMP_Transport *snmp);

void NXCORE_EXPORTABLE MacDbAddAccessPoint(AccessPoint *ap);
void NXCORE_EXPORTABLE MacDbAddInterface(Interface *iface);
void NXCORE_EXPORTABLE MacDbAddObject(const BYTE *macAddr, NetObj *object);
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr);
NetObj NXCORE_EXPORTABLE *MacDbFind(const BYTE *macAddr);

NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(const uuid& guid, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObject(bool (* comparator)(NetObj *, void *), void *userData, int objClass = -1);
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddress& ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddressList *ipAddrList);
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr);
Node NXCORE_EXPORTABLE *FindNodeByBridgeId(const BYTE *bridgeId);
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId);
Node NXCORE_EXPORTABLE *FindNodeBySysName(const TCHAR *sysName);
ObjectArray<NetObj> *FindNodesByHostname(TCHAR *hostname, UINT32 zoneUIN);
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneUIN, const InetAddress& ipAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneUIN, const InetAddress& ipAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneUIN, const InetAddress& nodeAddr);
MobileDevice NXCORE_EXPORTABLE *FindMobileDeviceByDeviceID(const TCHAR *deviceId);
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByMAC(const BYTE *macAddr);
UINT32 NXCORE_EXPORTABLE FindLocalMgmtNode();
Zone NXCORE_EXPORTABLE *FindZoneByUIN(UINT32 zoneUIN);
UINT32 FindUnusedZoneUIN();
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zoneUIN, const InetAddress& ipAddr);
bool NXCORE_EXPORTABLE IsParentObject(UINT32 object1, UINT32 object2);

BOOL LoadObjects();
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter);

bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(UINT32 userId, int objClass);

void DeleteUserFromAllObjects(UINT32 dwUserId);

bool IsValidParentClass(int childClass, int parentClass);
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
extern BOOL g_bModificationsLocked;
extern Queue g_templateUpdateQueue;

extern ObjectIndex NXCORE_EXPORTABLE g_idxObjectById;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxSubnetByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxInterfaceByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxNodeByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxZoneByUIN;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNetMapById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxChassisById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxClusterById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxMobileDeviceById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxAccessPointById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxConditionById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxServiceCheckById;

#endif   /* _nms_objects_h_ */
