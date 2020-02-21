/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
#include <math.h>
#include "nxcore_jobs.h"
#include "nms_topo.h"
#include <gauge_helpers.h>

/**
 * Forward declarations of classes
 */
class ClientSession;
class Queue;
class DataCollectionTarget;
class Cluster;
class ComponentTree;

/**
 * Global variables used by inline methods
 */
extern UINT32 g_discoveryPollingInterval;
extern UINT32 g_statusPollingInterval;
extern UINT32 g_configurationPollingInterval;
extern UINT32 g_routingTableUpdateInterval;
extern UINT32 g_topologyPollingInterval;
extern UINT32 g_conditionPollingInterval;
extern UINT32 g_instancePollingInterval;
extern UINT32 g_icmpPollingInterval;
extern INT16 g_defaultAgentCacheMode;

/**
 * Utility functions used by inline methods
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, UINT32 objectId, const TCHAR *query);
void NXCORE_EXPORTABLE ObjectTransactionStart();
void NXCORE_EXPORTABLE ObjectTransactionEnd();

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
//#define BUILTIN_OID_POLICYROOT            5
#define BUILTIN_OID_NETWORKMAPROOT        6
#define BUILTIN_OID_DASHBOARDROOT         7
#define BUILTIN_OID_BUSINESSSERVICEROOT   9

/**
 * "All zones" pseudo-ID
 */
#define ALL_ZONES ((UINT32)0xFFFFFFFF)

class AgentTunnel;
class GenericAgentPolicy;

/**
 * Extended agent connection
 */
class NXCORE_EXPORTABLE AgentConnectionEx : public AgentConnection
{
protected:
   UINT32 m_nodeId;
   AgentTunnel *m_tunnel;
   AgentTunnel *m_proxyTunnel;
   ClientSession *m_tcpProxySession;

   virtual AbstractCommChannel *createChannel() override;
   virtual void onTrap(NXCPMessage *msg) override;
   virtual void onSyslogMessage(NXCPMessage *pMsg) override;
   virtual void onDataPush(NXCPMessage *msg) override;
   virtual void onFileMonitoringData(NXCPMessage *msg) override;
   virtual void onSnmpTrap(NXCPMessage *pMsg) override;
   virtual UINT32 processCollectedData(NXCPMessage *msg) override;
   virtual UINT32 processBulkCollectedData(NXCPMessage *request, NXCPMessage *response) override;
   virtual bool processCustomMessage(NXCPMessage *msg) override;
   virtual void processTcpProxyData(UINT32 channelId, const void *data, size_t size) override;

public:
   AgentConnectionEx(UINT32 nodeId, const InetAddress& ipAddr, WORD port = AGENT_LISTEN_PORT, int authMethod = AUTH_NONE, const TCHAR *secret = NULL, bool allowCompression = true);
   AgentConnectionEx(UINT32 nodeId, AgentTunnel *tunnel, int authMethod = AUTH_NONE, const TCHAR *secret = NULL, bool allowCompression = true);
   virtual ~AgentConnectionEx();

   UINT32 deployPolicy(GenericAgentPolicy *policy, bool newTypeFormatSupported);
   UINT32 uninstallPolicy(uuid guid, TCHAR *type, bool newTypeFormatSupported);

   void setTunnel(AgentTunnel *tunnel);

   using AgentConnection::setProxy;
   void setProxy(AgentTunnel *tunnel, int authMethod, const TCHAR *secret);

   void setTcpProxySession(ClientSession *session);
};

/**
 * Poller types
 */
enum class PollerType
{
   STATUS = 0,
   CONFIGURATION = 1,
   INSTANCE_DISCOVERY = 2,
   ROUTING_TABLE = 3,
   DISCOVERY = 4,
   BUSINESS_SERVICE = 5,
   CONDITION = 6,
   TOPOLOGY = 7,
   ZONE_HEALTH = 8,
   ICMP = 9
};

/**
 * Poller information
 */
class NXCORE_EXPORTABLE PollerInfo
{
   friend PollerInfo *RegisterPoller(PollerType, NetObj*, bool);

private:
   PollerType m_type;
   NetObj *m_object;
   bool m_objectCreation;
   TCHAR m_status[128];

   PollerInfo(PollerType type, NetObj *object, bool objectCreation)
   {
      m_type = type;
      m_object = object;
      m_objectCreation = objectCreation;
      _tcscpy(m_status, _T("awaiting execution"));
   }

public:
   ~PollerInfo();

   PollerType getType() const { return m_type; }
   NetObj *getObject() const { return m_object; }
   bool isObjectCreation() const { return m_objectCreation; }
   const TCHAR *getStatus() const { return m_status; }

   void startObjectTransaction() { if (!m_objectCreation) ObjectTransactionStart(); }
   void endObjectTransaction() { if (!m_objectCreation) ObjectTransactionEnd(); }

   void startExecution() { _tcscpy(m_status, _T("started")); }
   void setStatus(const TCHAR *status) { _tcslcpy(m_status, status, 128); }
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
 * ICMP statistic collection mode
 */
enum class IcmpStatCollectionMode
{
   DEFAULT = 0,
   ON = 1,
   OFF = 2
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
   DataCollectionOwner *pTemplate;
   UINT32 targetId;
   bool removeDCI;
};

/**
 * Index head
 */
struct INDEX_HEAD;

/**
 * Generic index implementation
 */
class NXCORE_EXPORTABLE AbstractIndexBase
{
   DISABLE_COPY_CTOR(AbstractIndexBase)

protected:
	INDEX_HEAD* volatile m_primary;
	INDEX_HEAD* volatile m_secondary;
	MUTEX m_writerLock;
	bool m_owner;
   bool m_startupMode;
   bool m_dirty;
   void (*m_objectDestructor)(void *);

   void destroyObject(void *object)
   {
      if (object != NULL)
         m_objectDestructor(object);
   }

   INDEX_HEAD *acquireIndex();
   void swapAndWait();

	static ssize_t findElement(INDEX_HEAD *index, UINT64 key);
   void findAll(Array *resultSet, bool (*comparator)(void *, void *), void *data);

public:
	AbstractIndexBase(Ownership owner);
	~AbstractIndexBase();

   size_t size();
	bool put(UINT64 key, void *object);
	void remove(UINT64 key);
	void clear();
	void *get(UINT64 key);

	void *find(bool (*comparator)(void *, void *), void *data);

	void forEach(void (*callback)(void *, void *), void *data);

   bool isOwner() const
   {
      return m_owner;
   }

   void setOwner(Ownership owner)
   {
      m_owner = static_cast<bool>(owner);
   }

   void setStartupMode(bool startupMode);
};

/**
 * Index that keeps shared pointers
 */
template<typename T> class SharedPointerIndex : public AbstractIndexBase
{
   DISABLE_COPY_CTOR(SharedPointerIndex)

private:
   static void destructor(void *object)
   {
      delete static_cast<shared_ptr<T>*>(object);
   }

   static bool comparatorWrapper(shared_ptr<T> *element, std::pair<bool (*)(T*, void*), void*> *context)
   {
      return context->first(element->get(), context->second);
   }

   static void enumeratorWrapper(shared_ptr<T> *element, std::pair<void (*)(T*, void*), void*> *context)
   {
      context->first(element->get(), context->second);
   }

public:
   SharedPointerIndex<T>() : AbstractIndexBase(Ownership::True)
   {
      this->m_objectDestructor = destructor;
   }

   bool put(UINT64 key, T *object)
   {
      return AbstractIndexBase::put(key, new shared_ptr<T>(object));
   }

   bool put(UINT64 key, shared_ptr<T> object)
   {
      return AbstractIndexBase::put(key, new shared_ptr<T>(object));
   }

   shared_ptr<T> get(UINT64 key)
   {
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::get(key));
      return (v != NULL) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   shared_ptr<T> find(bool (*comparator)(T *, void *), void *context)
   {
      std::pair<bool (*)(T*, void*), void*> wrapperData(comparator, context);
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::find(reinterpret_cast<bool (*)(void*, void*)>(comparatorWrapper), &wrapperData));
      return (v != NULL) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   template<typename P> shared_ptr<T> find(bool (*comparator)(T *, P *), P *context)
   {
      return find(reinterpret_cast<bool (*)(T *, void *)>(comparator), (void *)context);
   }

   SharedObjectArray<T> *findAll(bool (*comparator)(T *, void *), void *context)
   {
      std::pair<bool (*)(T*, void*), void*> wrapperData(comparator, context);
      ObjectArray<shared_ptr<T>> tempResultSet;
      AbstractIndexBase::findAll(&tempResultSet, reinterpret_cast<bool (*)(void*, void*)>(comparatorWrapper), &wrapperData);
      auto resultSet = new SharedObjectArray<T>(tempResultSet.size());
      for(int i = 0; i < tempResultSet.size(); i++)
         resultSet->add(*tempResultSet.get(i));
      return resultSet;
   }

   template<typename P> SharedObjectArray<T> *findAll(bool (*comparator)(T *, P *), P *context)
   {
      return findAll(reinterpret_cast<bool (*)(T *, void *)>(comparator), (void *)context);
   }

   void forEach(void (*callback)(T *, void *), void *context)
   {
      std::pair<void (*)(T*, void*), void*> wrapperData(callback, context);
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(enumeratorWrapper), &wrapperData);
   }

   template<typename P> void forEach(void (*callback)(T *, P *), P *context)
   {
      std::pair<void (*)(T*, void*), void*> wrapperData(reinterpret_cast<void (*)(T*, void*)>(callback), context);
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(enumeratorWrapper), &wrapperData);
   }
};

/**
 * Abstract index template
 */
template<typename T> class AbstractIndex : public AbstractIndexBase
{
   DISABLE_COPY_CTOR(AbstractIndex)

public:
   AbstractIndex<T>(Ownership owner) : AbstractIndexBase(owner)
   {
   }

   bool put(UINT64 key, T *object)
   {
      return AbstractIndexBase::put(key, object);
   }

   T *get(UINT64 key)
   {
      return static_cast<T*>(AbstractIndexBase::get(key));
   }

   T *find(bool (*comparator)(T *, void *), void *context)
   {
      return static_cast<T*>(AbstractIndexBase::find(reinterpret_cast<bool (*)(void*, void*)>(comparator), context));
   }

   template<typename P> T *find(bool (*comparator)(T *, P *), P *context)
   {
      return static_cast<T*>(AbstractIndexBase::find(reinterpret_cast<bool (*)(void*, void*)>(comparator), (void *)context));
   }

   ObjectArray<T> *findAll(bool (*comparator)(T *, void *), void *context)
   {
      ObjectArray<T> *resultSet = new ObjectArray<T>(64, 64, Ownership::False);
      AbstractIndexBase::findAll(resultSet, reinterpret_cast<bool (*)(void*, void*)>(comparator), context);
      return resultSet;
   }

   template<typename P> ObjectArray<T> *findAll(bool (*comparator)(T *, P *), P *context)
   {
      return findAll(reinterpret_cast<bool (*)(T *, void *)>(comparator), (void *)context);
   }

   void forEach(void (*callback)(T *, void *), void *context)
   {
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(callback), context);
   }

   template<typename P> void forEach(void (*callback)(T *, P *), P *context)
   {
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(callback), context);
   }
};

/**
 * Abstract index template
 */
template<typename T> class AbstractIndexWithDestructor : public AbstractIndex<T>
{
   DISABLE_COPY_CTOR(AbstractIndexWithDestructor)

private:
   static void destructor(void *object)
   {
      delete static_cast<T*>(object);
   }

public:
   AbstractIndexWithDestructor<T>(Ownership owner) : AbstractIndex<T>(owner)
   {
      this->m_objectDestructor = destructor;
   }
};

/**
 * Object index
 */
class NXCORE_EXPORTABLE ObjectIndex : public AbstractIndex<NetObj>
{
   DISABLE_COPY_CTOR(ObjectIndex)

public:
   ObjectIndex() : AbstractIndex<NetObj>(Ownership::False) { }

   ObjectArray<NetObj> *getObjects(bool updateRefCount, bool (*filter)(NetObj *, void *) = NULL, void *context = NULL);

   template<typename C>
   ObjectArray<NetObj> *getObjects(bool updateRefCount, bool (*filter)(NetObj *, C *), C *context)
   {
      return getObjects(updateRefCount, reinterpret_cast<bool (*)(NetObj*, void*)>(filter), context);
   }
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

struct HashIndexHead;

/**
 * Generic hash-based index
 */
class NXCORE_EXPORTABLE HashIndexBase
{
private:
   HashIndexHead* volatile m_primary;
   HashIndexHead* volatile m_secondary;
   MUTEX m_writerLock;
   size_t m_keyLength;

   HashIndexHead *acquireIndex();
   void swapAndWait();

public:
   HashIndexBase(size_t keyLength);
   ~HashIndexBase();

   bool put(const void *key, NetObj *object);
   void remove(const void *key);

   int size();
   NetObj *get(const void *key);
   NetObj *find(bool (*comparator)(NetObj *, void *), void *context);
   void forEach(void (*callback)(const void *, NetObj *, void *), void *context);
};

/**
 * Hash index template
 */
template<typename K> class HashIndex : public HashIndexBase
{
public:
   HashIndex() : HashIndexBase(sizeof(K)) { }

   bool put(const K *key, NetObj *object) { return HashIndexBase::put(key, object); }
   bool put(const K& key, NetObj *object) { return HashIndexBase::put(&key, object); }
   void remove(const K *key) { HashIndexBase::remove(key); }
   void remove(const K& key) { HashIndexBase::remove(&key); }

   NetObj *get(const K *key) { return HashIndexBase::get(key); }
   NetObj *get(const K& key) { return HashIndexBase::get(&key); }
   void forEach(void (*callback)(const K *, NetObj *, void *), void *context) { HashIndexBase::forEach(reinterpret_cast<void (*)(const void *, NetObj *, void *)>(callback), context); }
};

/**
 * Change code
 */
enum ChangeCode
{
   CHANGE_NONE = 0,
   CHANGE_ADDED = 1,
   CHANGE_UPDATED = 2,
   CHANGE_REMOVED = 3
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
	ChangeCode m_changeCode;

	SoftwarePackage();

public:
	SoftwarePackage(DB_RESULT result, int row);
	~SoftwarePackage();

	void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
	bool saveToDatabase(DB_STATEMENT hStmt) const;

	const TCHAR *getName() const { return m_name; }
	const TCHAR *getVersion() const { return m_version; }
	ChangeCode getChangeCode() const { return m_changeCode; }

	void setChangeCode(ChangeCode c) { m_changeCode = c; }

	static SoftwarePackage *createFromTableRow(const Table *table, int row);
};

/**
 * Hardware component categories
 */
enum HardwareComponentCategory
{
   HWC_OTHER = 0,
   HWC_BASEBOARD = 1,
   HWC_PROCESSOR = 2,
   HWC_MEMORY = 3,
   HWC_STORAGE = 4,
   HWC_BATTERY = 5,
   HWC_NETWORK_ADAPTER = 6
};

/**
 * Hardware component information
 */
class HardwareComponent
{
private:
   HardwareComponentCategory m_category;
   ChangeCode m_changeCode;
   UINT32 m_index;
   UINT64 m_capacity;
   TCHAR *m_type;
   TCHAR *m_vendor;
   TCHAR *m_model;
   TCHAR *m_partNumber;
   TCHAR *m_serialNumber;
   TCHAR *m_location;
   TCHAR *m_description;

public:
   HardwareComponent(HardwareComponentCategory category, UINT32 index, const TCHAR *type,
            const TCHAR *vendor, const TCHAR *model, const TCHAR *partNumber, const TCHAR *serialNumber);
   HardwareComponent(DB_RESULT result, int row);
   HardwareComponent(HardwareComponentCategory category, const Table *table, int row);
   ~HardwareComponent();

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   bool saveToDatabase(DB_STATEMENT hStmt) const;

   ChangeCode getChangeCode() const { return m_changeCode; };
   HardwareComponentCategory getCategory() const { return m_category; };
   UINT32 getIndex() const { return m_index; }
   UINT64 getCapacity() const { return m_capacity; }
   const TCHAR *getType() const { return CHECK_NULL_EX(m_type); };
   const TCHAR *getVendor() const { return CHECK_NULL_EX(m_vendor); };
   const TCHAR *getModel() const { return CHECK_NULL_EX(m_model); };
   const TCHAR *getLocation() const { return CHECK_NULL_EX(m_location); };
   const TCHAR *getPartNumber() const { return CHECK_NULL_EX(m_partNumber); };
   const TCHAR *getSerialNumber() const { return CHECK_NULL_EX(m_serialNumber); };
   const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); };

   void setIndex(int index) { m_index = index; }
   void setChangeCode(ChangeCode code) { m_changeCode = code; }
};

/**
 * New node origin
 */
enum NodeOrigin
{
   NODE_ORIGIN_MANUAL = 0,
   NODE_ORIGIN_NETWORK_DISCOVERY = 1,
   NODE_ORIGIN_TUNNEL_AUTOBIND = 2
};

/**
 * Data for new node creation
 */
struct NXCORE_EXPORTABLE NewNodeData
{
   InetAddress ipAddr;
   uint32_t creationFlags;
   uint16_t agentPort;
   uint16_t snmpPort;
   uint16_t eipPort;
   TCHAR name[MAX_OBJECT_NAME];
   uint32_t agentProxyId;
   uint32_t snmpProxyId;
   uint32_t eipProxyId;
   uint32_t icmpProxyId;
   uint32_t sshProxyId;
   TCHAR sshLogin[MAX_SSH_LOGIN_LEN];
   TCHAR sshPassword[MAX_SSH_PASSWORD_LEN];
   Cluster *cluster;
   uint32_t zoneUIN;
   bool doConfPoll;
   NodeOrigin origin;
   SNMP_SecurityContext *snmpSecurity;
   uuid agentId;

   NewNodeData(const InetAddress& ipAddr);
   NewNodeData(const NXCPMessage *msg, const InetAddress& ipAddr);
   ~NewNodeData();
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
#define MODIFY_RUNTIME              0x000000
#define MODIFY_OTHER                0x000001
#define MODIFY_CUSTOM_ATTRIBUTES    0x000002
#define MODIFY_DATA_COLLECTION      0x000004
#define MODIFY_RELATIONS            0x000008
#define MODIFY_COMMON_PROPERTIES    0x000010
#define MODIFY_ACCESS_LIST          0x000020
#define MODIFY_NODE_PROPERTIES      0x000040
#define MODIFY_INTERFACE_PROPERTIES 0x000080
#define MODIFY_CLUSTER_RESOURCES    0x000100
#define MODIFY_MAP_CONTENT          0x000200
#define MODIFY_SENSOR_PROPERTIES    0x000400
#define MODIFY_MODULE_DATA          0x000800
#define MODIFY_POLICY               0x001000
#define MODIFY_SOFTWARE_INVENTORY   0x002000
#define MODIFY_HARDWARE_INVENTORY   0x004000
#define MODIFY_COMPONENTS           0x008000
#define MODIFY_ICMP_POLL_SETTINGS   0x010000
#define MODIFY_ALL                  0xFFFFFF

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

   void createExportRecord(StringBuffer &xml, int id);
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

   void createExportRecord(StringBuffer &xml) const;
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
class NXCORE_EXPORTABLE NetObj : public NObject
{
   friend class AutoBindTarget;
   friend class VersionableObject;

   DISABLE_COPY_CTOR(NetObj)

private:
   typedef NObject super;
   time_t m_creationTime; //Object creation time

	static void onObjectDeleteCallback(NetObj *object, void *data);

	void getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly);

protected:
   time_t m_timestamp;       // Last change time stamp
   VolatileCounter m_refCount;        // Number of references. Object can be destroyed only when this counter is zero
   TCHAR *m_comments;      // User comments
   int m_status;
   int m_savedStatus;      // Object status in database
   int m_statusCalcAlg;      // Status calculation algorithm
   int m_statusPropAlg;      // Status propagation algorithm
   int m_fixedStatus;        // Status if propagation is "Fixed"
   int m_statusShift;        // Shift value for "shifted" status propagation
   int m_statusTranslation[4];
   int m_statusSingleThreshold;
   int m_statusThresholds[4];
   UINT32 m_flags;
   UINT32 m_runtimeFlags;
   UINT32 m_state;
   UINT32 m_stateBeforeMaintenance;
   VolatileCounter m_modified;
   bool m_isDeleted;
   bool m_isDeleteInitiated;
   bool m_isHidden;
	bool m_isSystem;
	UINT64 m_maintenanceEventId;
	uuid m_image;
   MUTEX m_mutexProperties;         // Object data access mutex
	GeoLocation m_geoLocation;
   PostalAddress *m_postalAddress;
   ClientSession *m_pollRequestor;
	UINT32 m_submapId;				// Map object which should be open on drill-down request
	IntegerArray<UINT32> *m_dashboards; // Dashboards associated with this object
	ObjectArray<ObjectUrl> *m_urls;  // URLs associated with this object
	UINT32 m_primaryZoneProxyId;     // ID of assigned primary zone proxy node
   UINT32 m_backupZoneProxyId;      // ID of assigned backup zone proxy node

   AccessList *m_accessList;
   bool m_inheritAccessRights;
   MUTEX m_mutexACL;

   IntegerArray<UINT32> *m_trustedNodes;

   StringObjectMap<ModuleData> *m_moduleData;

   IntegerArray<UINT32> *m_responsibleUsers;
   RWLOCK m_rwlockResponsibleUsers;

   const ObjectArray<NetObj> *getChildList() { return reinterpret_cast<const ObjectArray<NetObj>*>(super::getChildList()); }
   const ObjectArray<NetObj> *getParentList() { return reinterpret_cast<const ObjectArray<NetObj>*>(super::getParentList()); }

   void lockProperties() const { MutexLock(m_mutexProperties); }
   void unlockProperties() const { MutexUnlock(m_mutexProperties); }
   void lockACL() { MutexLock(m_mutexACL); }
   void unlockACL() { MutexUnlock(m_mutexACL); }
   void lockResponsibleUsersList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockResponsibleUsers, INFINITE);
      else
         RWLockReadLock(m_rwlockResponsibleUsers, INFINITE);
   }
   void unlockResponsibleUsersList() { RWLockUnlock(m_rwlockResponsibleUsers); }

   void setModified(UINT32 flags, bool notify = true);                  // Used to mark object as modified

   bool loadACLFromDB(DB_HANDLE hdb);
   bool saveACLToDB(DB_HANDLE hdb);
   bool loadCommonProperties(DB_HANDLE hdb);
   bool saveCommonProperties(DB_HANDLE hdb);
   bool saveModuleData(DB_HANDLE hdb);
   bool loadTrustedNodes(DB_HANDLE hdb);
	bool saveTrustedNodes(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query) { return ExecuteQueryOnObject(hdb, m_id, query); }

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 objectId);

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual void fillMessageInternalStage2(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *msg);
   virtual UINT32 modifyFromMessageInternalStage2(NXCPMessage *msg);

   void addLocationToHistory();
   bool isLocationTableExists(DB_HANDLE hdb);
   bool createLocationHistoryTable(DB_HANDLE hdb);

   void getAllResponsibleUsersInternal(IntegerArray<UINT32> *list);

public:
   NetObj();
   virtual ~NetObj();

   virtual int getObjectClass() const { return OBJECT_GENERIC; }
   virtual const WCHAR *getObjectClassNameW() const;
   virtual const char *getObjectClassNameA() const;
#ifdef UNICODE
   const WCHAR *getObjectClassName() const { return getObjectClassNameW(); }
#else
   const char *getObjectClassName() const { return getObjectClassNameA(); }
#endif

   virtual uint32_t getZoneUIN() const { return 0; }

   virtual InetAddress getPrimaryIpAddress() const { return InetAddress::INVALID; }

   int getStatus() const { return m_status; }
   UINT32 getState() const { return m_state; }
   UINT32 getRuntimeFlags() const { return m_runtimeFlags; }
   UINT32 getFlags() const { return m_flags; }
   int getPropagatedStatus();
   time_t getTimeStamp() const { return m_timestamp; }
	const TCHAR *getComments() const { return CHECK_NULL_EX(m_comments); }

	const GeoLocation& getGeoLocation() const { return m_geoLocation; }
	void setGeoLocation(const GeoLocation& geoLocation);

   const PostalAddress *getPostalAddress() const { return m_postalAddress; }
   void setPostalAddress(PostalAddress * addr) { lockProperties(); delete m_postalAddress; m_postalAddress = addr; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   const uuid& getMapImage() { return m_image; }
   void setMapImage(const uuid& image) { lockProperties(); m_image = image; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   bool isModified() const { return m_modified != 0; }
   bool isModified(UINT32 bit) const { return (m_modified & bit) != 0; }
   bool isDeleted() const { return m_isDeleted; }
   bool isDeleteInitiated() const { return m_isDeleteInitiated; }
   bool isOrphaned() const { return getParentCount() == 0; }
   bool isEmpty() const { return getChildCount() == 0; }

	bool isSystem() const { return m_isSystem; }
	void setSystemFlag(bool flag) { m_isSystem = flag; }
	void setFlag(UINT32 flag) { lockProperties(); m_flags |= flag; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
	void clearFlag(UINT32 flag) { lockProperties(); m_flags &= ~flag; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   UINT32 getRefCount() const { return static_cast<UINT32>(m_refCount); }
   void incRefCount() { InterlockedIncrement(&m_refCount); }
   void decRefCount() { InterlockedDecrement(&m_refCount); }

	bool isTrustedNode(UINT32 id);

   virtual void onCustomAttributeChange() override;

   void addChild(NetObj *object);     // Add reference to child object
   void addParent(NetObj *object);    // Add reference to parent object

   void deleteChild(NetObj *object);  // Delete reference to child object
   void deleteParent(NetObj *object); // Delete reference to parent object

   void deleteObject(NetObj *initiator = NULL);     // Prepare object for deletion
   void destroy();   // Destroy partially loaded object

   void updateObjectIndexes();

   bool isHidden() { return m_isHidden; }
   void hide();
   void unhide();
   void markAsModified(UINT32 flags) { setModified(flags); }  // external API to mark object as modified
   void markAsSaved() { m_modified = 0; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool saveRuntimeData(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();

   void setId(UINT32 dwId) { m_id = dwId; setModified(MODIFY_ALL); }
   void generateGuid() { m_guid = uuid::generate(); }
   void setName(const TCHAR *pszName) { lockProperties(); _tcslcpy(m_name, pszName, MAX_OBJECT_NAME); setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
   void resetStatus() { lockProperties(); m_status = STATUS_UNKNOWN; setModified(MODIFY_RUNTIME); unlockProperties(); }
   void setComments(TCHAR *text);	/* text must be dynamically allocated */
   void setCreationTime() { m_creationTime = time(NULL); }
   time_t getCreationTime() { return m_creationTime; }

   bool isInMaintenanceMode() const { return m_maintenanceEventId != 0; }
   UINT64 getMaintenanceEventId() const { return m_maintenanceEventId; }
   virtual void enterMaintenanceMode(const TCHAR *comments);
   virtual void leaveMaintenanceMode();

   void fillMessage(NXCPMessage *msg, UINT32 userId);
   UINT32 modifyFromMessage(NXCPMessage *msg);

	virtual void postModify();

   void commentsToMessage(NXCPMessage *pMsg);

   virtual bool setMgmtStatus(BOOL bIsManaged);
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   UINT32 getUserRights(UINT32 dwUserId);
   BOOL checkAccessRights(UINT32 dwUserId, UINT32 dwRequiredRights);
   void dropUserAccess(UINT32 dwUserId);

   void addChildNodesToList(ObjectArray<Node> *nodeList, UINT32 dwUserId);
   void addChildDCTargetsToList(ObjectArray<DataCollectionTarget> *dctList, UINT32 dwUserId);

   UINT32 getAssignedZoneProxyId(bool backup = false) const { return backup ? m_backupZoneProxyId : m_primaryZoneProxyId; }
   void setAssignedZoneProxyId(UINT32 id, bool backup) { if (backup) m_backupZoneProxyId = id; else m_primaryZoneProxyId = id; }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm);

   void executeHookScript(const TCHAR *hookName);

   ModuleData *getModuleData(const TCHAR *module);
   void setModuleData(const TCHAR *module, ModuleData *data);

	ObjectArray<NetObj> *getParents(int typeFilter = -1);
	ObjectArray<NetObj> *getChildren(int typeFilter = -1);
	ObjectArray<NetObj> *getAllChildren(bool eventSourceOnly, bool updateRefCount);

   NetObj *findChildObject(const TCHAR *name, int typeFilter);
   Node *findChildNode(const InetAddress& addr);

	virtual NXSL_Array *getParentsForNXSL(NXSL_VM *vm);
	virtual NXSL_Array *getChildrenForNXSL(NXSL_VM *vm);

   virtual bool showThresholdSummary();
   virtual bool isEventSource();
   virtual bool isDataCollectionTarget();
   virtual bool isAgentPolicy();

   void setStatusCalculation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);
   void setStatusPropagation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);

   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...);

   StringBuffer expandText(const TCHAR *textTemplate, const Alarm *alarm, const Event *event, const TCHAR *userName,
            const TCHAR *objectName, const StringMap *inputFields, const StringList *args);

   IntegerArray<UINT32> *getAllResponsibleUsers();

   virtual json_t *toJson();

   // Debug methods
   static const WCHAR *getObjectClassNameW(int objectClass);
   static const char *getObjectClassNameA(int objectClass);
#ifdef UNICODE
   static const WCHAR *getObjectClassName(int objectClass) { return getObjectClassNameW(objectClass); }
#else
   static const char *getObjectClassName(int objectClass) { return getObjectClassNameA(objectClass); }
#endif
};

/**
 * Generic versionable object
 */
class NXCORE_EXPORTABLE VersionableObject
{
private:
   NetObj *m_this;

protected:
   VolatileCounter m_version;

   void fillMessage(NXCPMessage *msg);

   bool saveToDatabase(DB_HANDLE hdb);
   bool deleteFromDatabase(DB_HANDLE hdb);
   bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);

   void toJson(json_t *root);
   void updateFromImport(ConfigEntry *config);

public:
   VersionableObject(NetObj *_this);
   VersionableObject(NetObj *_this, ConfigEntry *config);

   void updateVersion() { InterlockedIncrement(&m_version); }
   UINT32 getVersion() const { return m_version; }
};

/**
 * Generic auto-bind target. Intended to be used only as secondary class in multi-inheritance with primary class derived from NetObj.
 */
class NXCORE_EXPORTABLE AutoBindTarget
{
private:
   NetObj *m_this;
   MUTEX m_mutexProperties;

   void internalLock() const { MutexLock(m_mutexProperties); }
   void internalUnlock() const { MutexUnlock(m_mutexProperties); }

protected:
   TCHAR *m_bindFilterSource;
   NXSL_Program *m_bindFilter;
   bool m_autoBindFlag;
   bool m_autoUnbindFlag;

   void modifyFromMessage(NXCPMessage *request);
   void fillMessage(NXCPMessage *msg);

   bool loadFromDatabase(DB_HANDLE hdb, UINT32 objectId);
   bool saveToDatabase(DB_HANDLE hdb);
   bool deleteFromDatabase(DB_HANDLE hdb);
   void updateFromImport(ConfigEntry *config);

   void toJson(json_t *root);
   void createExportRecord(StringBuffer &str);

public:
   AutoBindTarget(NetObj *_this);
   AutoBindTarget(NetObj *_this, ConfigEntry *config);
   virtual ~AutoBindTarget();

   AutoBindDecision isApplicable(DataCollectionTarget *object);
   bool isAutoBindEnabled() { return m_autoBindFlag; }
   bool isAutoUnbindEnabled() { return m_autoBindFlag && m_autoUnbindFlag; }
   void setAutoBindFilter(const TCHAR *filter);
   void setAutoBindMode(bool doBind, bool doUnbind);

   const TCHAR *getAutoBindScriptSource() const { return m_bindFilterSource; }
};

/**
 * ICMP statistics collector
 */
class NXCORE_EXPORTABLE IcmpStatCollector
{
private:
   UINT32 m_lastResponseTime;
   UINT32 m_minResponseTime;
   UINT32 m_maxResponseTime;
   UINT32 m_avgResponseTime;
   UINT32 m_packetLoss;
   UINT16 *m_rawResponseTimes;
   int m_writePos;
   int m_bufferSize;

   void recalculate();

public:
   IcmpStatCollector(int period);
   ~IcmpStatCollector();

   UINT32 last() const { return m_lastResponseTime; }
   UINT32 average() const { return m_avgResponseTime; }
   UINT32 min() const { return m_minResponseTime; }
   UINT32 max() const { return m_maxResponseTime; }
   UINT32 packetLoss() const { return m_packetLoss; }

   void update(UINT32 responseTime);
   void resize(int period);

   bool saveToDatabase(DB_HANDLE hdb, UINT32 objectId, const TCHAR *target) const;

   static IcmpStatCollector *loadFromDatabase(DB_HANDLE hdb, UINT32 objectId, const TCHAR *target, int period);
};

/**
 * Data collection owner class
 */
class NXCORE_EXPORTABLE DataCollectionOwner : public NetObj
{
private:
   typedef NetObj super;

protected:
	SharedObjectArray<DCObject> *m_dcObjects;
   bool m_dciListModified;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];
	RWLOCK m_dciAccessLock;

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionLoad() { }
   virtual void onDataCollectionChange() { }

   void loadItemsFromDB(DB_HANDLE hdb);
   void destroyItems();
   void updateInstanceDiscoveryItems(DCObject *dci);

   void lockDciAccess(bool writeLock) { if (writeLock) { RWLockWriteLock(m_dciAccessLock, INFINITE); } else { RWLockReadLock(m_dciAccessLock, INFINITE); } }
	void unlockDciAccess() { RWLockUnlock(m_dciAccessLock); }

   void deleteChildDCIs(UINT32 dcObjectId);
   void deleteDCObject(DCObject *object);

public:
   DataCollectionOwner();
   DataCollectionOwner(const TCHAR *pszName);
   DataCollectionOwner(ConfigEntry *config);
   virtual ~DataCollectionOwner();

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual void updateFromImport(ConfigEntry *config);

   virtual json_t *toJson() override;

   int getItemCount() { return m_dcObjects->size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false, bool notify = true);
   bool updateDCObject(UINT32 dwItemId, NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId, UINT32 userId);
   bool deleteDCObject(UINT32 dcObjectId, bool needLock, UINT32 userId);
   bool setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus);
   shared_ptr<DCObject> getDCObjectById(UINT32 itemId, UINT32 userId, bool lock = true);
   shared_ptr<DCObject> getDCObjectByGUID(const uuid& guid, UINT32 userId, bool lock = true);
   shared_ptr<DCObject> getDCObjectByTemplateId(UINT32 tmplItemId, UINT32 userId);
   shared_ptr<DCObject> getDCObjectByName(const TCHAR *name, UINT32 userId);
   shared_ptr<DCObject> getDCObjectByDescription(const TCHAR *description, UINT32 userId);
   SharedObjectArray<DCObject> *getDCObjectsByRegex(const TCHAR *regex, bool searchName, UINT32 userId);
   NXSL_Value *getAllDCObjectsForNXSL(NXSL_VM *vm, const TCHAR *name, const TCHAR *description, UINT32 userId);
   virtual void applyDCIChanges();
   void setDCIModificationFlag() { m_dciListModified = true; }
   void sendItemsToClient(ClientSession *pSession, UINT32 dwRqId);
   IntegerArray<UINT32> *getDCIEventsList();
   StringSet *getDCIScriptList();
   bool isDataCollectionSource(UINT32 nodeId);
   virtual BOOL applyToTarget(DataCollectionTarget *pNode);

   void queueUpdate();
   void queueRemoveFromTarget(UINT32 targetId, bool removeDCI);

	bool enumDCObjects(bool (*callback)(shared_ptr<DCObject>, UINT32, void *), void *context);
	void associateItems();
};

/**
 * Generic agent policy object
 */
class NXCORE_EXPORTABLE GenericAgentPolicy
{
protected:
   UINT32 m_ownerId;
   uuid m_guid;
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR m_type[MAX_POLICY_TYPE_LEN];
   char *m_content;
   UINT32 m_version;

   GenericAgentPolicy(const GenericAgentPolicy *src);

public:
   GenericAgentPolicy(const uuid& guid, const TCHAR *type, UINT32 ownerId);
   GenericAgentPolicy(const TCHAR *name, const TCHAR *type, UINT32 ownerId);
   virtual ~GenericAgentPolicy();

   virtual GenericAgentPolicy *clone() const;

   const TCHAR *getName() const { return m_name; }
   const uuid getGuid() const { return m_guid; }
   const UINT32 getVersion() const { return m_version; }
   const TCHAR *getType() const { return m_type; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb);

   virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual void fillUpdateMessage(NXCPMessage *msg);
   virtual UINT32 modifyFromMessage(NXCPMessage *request);

   virtual void updateFromImport(ConfigEntry *config);
   virtual void createExportRecord(StringBuffer &str);

   virtual json_t *toJson();

   virtual UINT32 deploy(AgentConnectionEx *conn, bool newTypeFormatSupported, const TCHAR *debugId);
   virtual bool createDeploymentMessage(NXCPMessage *msg, bool newTypeFormatSupported);
};

/**
 * File delivery policy
 */
class NXCORE_EXPORTABLE FileDeliveryPolicy : public GenericAgentPolicy
{
protected:
   FileDeliveryPolicy(const FileDeliveryPolicy *src) : GenericAgentPolicy(src) { }

public:
   FileDeliveryPolicy(const uuid& guid, UINT32 ownerId) : GenericAgentPolicy(guid, _T("FileDelivery"), ownerId) { }
   FileDeliveryPolicy(const TCHAR *name, UINT32 ownerId) : GenericAgentPolicy(name, _T("FileDelivery"), ownerId) { }
   virtual ~FileDeliveryPolicy() { }

   virtual GenericAgentPolicy *clone() const override;
   virtual UINT32 modifyFromMessage(NXCPMessage *request) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual UINT32 deploy(AgentConnectionEx *conn, bool newTypeFormatSupported, const TCHAR *debugId) override;
};

/**
 * Data collection template class
 */
class NXCORE_EXPORTABLE Template : public DataCollectionOwner, public AutoBindTarget, public VersionableObject
{
private:
   typedef DataCollectionOwner super;

protected:
   HashMap<uuid, GenericAgentPolicy> *m_policyList;
   ObjectArray<GenericAgentPolicy> *m_deletedPolicyList;

   virtual void prepareForDeletion() override;
   virtual void onDataCollectionChange() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   Template();
   Template(ConfigEntry *config);
   Template(const TCHAR *pszName);
   ~Template();

   virtual int getObjectClass() const override { return OBJECT_TEMPLATE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void applyDCIChanges() override;
   virtual BOOL applyToTarget(DataCollectionTarget *pNode) override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual void updateFromImport(ConfigEntry *config) override;
   virtual json_t *toJson() override;

   void createExportRecord(StringBuffer &xml);

   GenericAgentPolicy *getAgentPolicyCopy(const uuid& guid);
   bool hasPolicy(const uuid& guid);
   uuid updatePolicyFromMessage(NXCPMessage *request);
   bool removePolicy(const uuid& guid);
   void fillPolicyMessage(NXCPMessage *pMsg);
   void applyPolicyChanges();
   void forceApplyPolicyChanges();
   void applyPolicyChanges(DataCollectionTarget *object);
   void checkPolicyBind(Node *node, AgentPolicyInfo *ap, ObjectArray<NetObj> *unbindList);
   void forceInstallPolicy(DataCollectionTarget *target);
   void removeAllPolicies(Node *node);
};

/**
 * Interface class
 */
class NXCORE_EXPORTABLE Interface : public NetObj
{
private:
   typedef NetObj super;

protected:
   UINT32 m_parentInterfaceId;
   UINT32 m_index;
   MacAddress m_macAddr;
   InetAddressList m_ipAddressList;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
	TCHAR m_alias[MAX_DB_STRING];	// Interface alias - value of ifAlias for SNMP, empty for NetXMS agent
   UINT32 m_type;
   UINT32 m_mtu;
   UINT64 m_speed;
	UINT32 m_bridgePortNumber;		 // 802.1D port number
	InterfacePhysicalLocation m_physicalLocation;
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
   int m_ifTableSuffixLen;
   UINT32 *m_ifTableSuffix;
   IntegerArray<UINT32> *m_vlans;

   void icmpStatusPoll(UINT32 rqId, UINT32 nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState);
	void paeStatusPoll(UINT32 rqId, SNMP_Transport *pTransport, Node *node);

protected:
   virtual void onObjectDelete(UINT32 objectId) override;
   virtual void prepareForDeletion() override;

	virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   void setExpectedStateInternal(int state);

public:
   Interface();
   Interface(const InetAddressList& addrList, UINT32 zoneUIN, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, const InetAddressList& addrList, UINT32 ifType, UINT32 zoneUIN);
   virtual ~Interface();

   virtual int getObjectClass() const override { return OBJECT_INTERFACE; }
   virtual InetAddress getPrimaryIpAddress() const override { lockProperties(); auto a = m_ipAddressList.getFirstUnicastAddress(); unlockProperties(); return a; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual uint32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   Node *getParentNode();
   UINT32 getParentNodeId();
   String getParentNodeName();
   UINT32 getParentInterfaceId() const { return m_parentInterfaceId; }

   const InetAddressList *getIpAddressList() const { return &m_ipAddressList; }
   InetAddress getFirstIpAddress() const;
   UINT32 getIfIndex() const { return m_index; }
   UINT32 getIfType() const { return m_type; }
   UINT32 getMTU() const { return m_mtu; }
   UINT64 getSpeed() const { return m_speed; }
	UINT32 getBridgePortNumber() const { return m_bridgePortNumber; }
   UINT32 getChassis() const { return m_physicalLocation.chassis; }
	UINT32 getModule() const { return m_physicalLocation.module; }
   UINT32 getPIC() const { return m_physicalLocation.pic; }
	UINT32 getPort() const { return m_physicalLocation.port; }
	InterfacePhysicalLocation getPhysicalLocation() const { return GetAttributeWithLock(m_physicalLocation, m_mutexProperties); }
	UINT32 getPeerNodeId() const { return m_peerNodeId; }
	UINT32 getPeerInterfaceId() const { return m_peerInterfaceId; }
   LinkLayerProtocol getPeerDiscoveryProtocol() const { return m_peerDiscoveryProtocol; }
	int getExpectedState() const { return (int)((m_flags & IF_EXPECTED_STATE_MASK) >> 28); }
	int getAdminState() const { return (int)m_adminState; }
	int getOperState() const { return (int)m_operState; }
   int getConfirmedOperState() const { return (int)m_confirmedOperState; }
	int getDot1xPaeAuthState() const { return (int)m_dot1xPaeAuthState; }
	int getDot1xBackendAuthState() const { return (int)m_dot1xBackendAuthState; }
	const TCHAR *getDescription() const { return m_description; }
	const TCHAR *getAlias() const { return m_alias; }
   const MacAddress& getMacAddr() const { return m_macAddr; }
   int getIfTableSuffixLen() const { return m_ifTableSuffixLen; }
   const UINT32 *getIfTableSuffix() const { return m_ifTableSuffix; }
	bool isSyntheticMask() const { return (m_flags & IF_SYNTHETIC_MASK) ? true : false; }
	bool isPhysicalPort() const { return (m_flags & IF_PHYSICAL_PORT) ? true : false; }
	bool isLoopback() const { return (m_flags & IF_LOOPBACK) ? true : false; }
	bool isManuallyCreated() const { return (m_flags & IF_CREATED_MANUALLY) ? true : false; }
	bool isExcludedFromTopology() const { return (m_flags & (IF_EXCLUDE_FROM_TOPOLOGY | IF_LOOPBACK)) ? true : false; }
   bool isFake() const
   {
      return (m_index == 1) &&
             (m_type == IFTYPE_OTHER) &&
             !_tcscmp(m_name, _T("unknown"));
   }
   bool isSubInterface() const { return m_parentInterfaceId != 0; }
   bool isPointToPoint() const;
   bool isIncludedInIcmpPoll() const { return (m_flags & IF_INCLUDE_IN_ICMP_POLL) ? true : false; }
   NXSL_Value *getVlanListForNXSL(NXSL_VM *vm);

   UINT64 getLastDownEventId() const { return m_lastDownEventId; }
   void setLastDownEventId(UINT64 id) { m_lastDownEventId = id; }

   void setMacAddr(const MacAddress& macAddr, bool updateMacDB);
   void setIpAddress(const InetAddress& addr);
   void setBridgePortNumber(UINT32 bpn)
   {
      lockProperties();
      m_bridgePortNumber = bpn;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setPhysicalLocation(const InterfacePhysicalLocation& location);
	void setPhysicalPortFlag(bool isPhysical)
	{
	   lockProperties();
	   if (isPhysical)
	      m_flags |= IF_PHYSICAL_PORT;
	   else
	      m_flags &= ~IF_PHYSICAL_PORT;
	   setModified(MODIFY_COMMON_PROPERTIES);
	   unlockProperties();
	}
	void setManualCreationFlag(bool isManual)
	{
	   lockProperties();
	   if (isManual)
	      m_flags |= IF_CREATED_MANUALLY;
	   else
	      m_flags &= ~IF_CREATED_MANUALLY;
	   setModified(MODIFY_COMMON_PROPERTIES);
	   unlockProperties();
	}
	void setPeer(Node *node, Interface *iface, LinkLayerProtocol protocol, bool reflection);
   void clearPeer()
   {
      lockProperties();
      m_peerNodeId = 0;
      m_peerInterfaceId = 0;
      m_peerDiscoveryProtocol = LL_PROTO_UNKNOWN;
      m_flags &= ~IF_PEER_REFLECTION;
      setModified(MODIFY_INTERFACE_PROPERTIES | MODIFY_COMMON_PROPERTIES);
      unlockProperties();
   }
   void setDescription(const TCHAR *descr)
   {
      lockProperties();
      _tcslcpy(m_description, descr, MAX_DB_STRING);
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setAlias(const TCHAR *alias)
   {
      lockProperties();
      _tcslcpy(m_alias, alias, MAX_DB_STRING);
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void addIpAddress(const InetAddress& addr);
   void deleteIpAddress(InetAddress addr);
   void setNetMask(const InetAddress& addr);
	void setMTU(int mtu)
	{
	   lockProperties();
	   m_mtu = mtu;
	   setModified(MODIFY_INTERFACE_PROPERTIES);
	   unlockProperties();
	}
	void setSpeed(UINT64 speed)
	{
	   lockProperties();
	   m_speed = speed;
	   setModified(MODIFY_INTERFACE_PROPERTIES);
	   unlockProperties();
	}
   void setIfTableSuffix(int len, const UINT32 *suffix)
   {
      lockProperties();
      MemFree(m_ifTableSuffix);
      m_ifTableSuffixLen = len;
      m_ifTableSuffix = (len > 0) ? MemCopyArray(suffix, len) : NULL;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setParentInterface(UINT32 parentInterfaceId)
   {
      lockProperties();
      m_parentInterfaceId = parentInterfaceId;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void addVlan(UINT32 id);
   void clearVlanList()
   {
      lockProperties();
      if (m_vlans != NULL)
         m_vlans->clear();
      unlockProperties();
   }

	void updateZoneUIN();

   void statusPoll(ClientSession *session, UINT32 rqId, ObjectQueue<Event> *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, UINT32 nodeIcmpProxy);

   UINT32 wakeUp();
	void setExpectedState(int state) { lockProperties(); setExpectedStateInternal(state); unlockProperties(); }
   void setExcludeFromTopology(bool excluded);
   void setIncludeInIcmpPoll(bool included);

   void expandName(const TCHAR *originalName, TCHAR *expandedName);
};

/**
 * Network service class
 */
class NXCORE_EXPORTABLE NetworkService : public NetObj
{
private:
   typedef NetObj super;

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

   virtual void onObjectDelete(UINT32 objectId) override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  Node *pHostNode = NULL, UINT32 dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int getObjectClass() const override { return OBJECT_NETWORKSERVICE; }

   virtual json_t *toJson() override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   void statusPoll(ClientSession *session, UINT32 rqId, Node *pollerNode, ObjectQueue<Event> *eventQueue);

   UINT32 getResponseTime() { return m_responseTime; }
};

/**
 * VPN connector class
 */
class NXCORE_EXPORTABLE VPNConnector : public NetObj
{
private:
   typedef NetObj super;

protected:
   UINT32 m_dwPeerGateway;        // Object ID of peer gateway
   ObjectArray<InetAddress> *m_localNetworks;
   ObjectArray<InetAddress> *m_remoteNetworks;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   Node *getParentNode();

public:
   VPNConnector();
   VPNConnector(bool hidden);
   virtual ~VPNConnector();

   virtual int getObjectClass() const override { return OBJECT_VPNCONNECTOR; }

   virtual json_t *toJson() override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   bool isLocalAddr(const InetAddress& addr);
   bool isRemoteAddr(const InetAddress& addr);
   UINT32 getPeerGatewayId() const { return m_dwPeerGateway; }
   InetAddress getPeerGatewayAddr();
};

/**
 * Reset poll timers for all objects
 */
void ResetObjectPollTimers(shared_ptr<ScheduledTaskParameters> parameters);

/**
 * Poll timers reset task
 */
#define DCT_RESET_POLL_TIMERS_TASK_ID _T("System.ResetPollTimers")

#define pollerLock(name) \
   _pollerLock(); \
   UINT64 __pollStartTime = GetCurrentTimeMs(); \
   PollState *__pollState = &m_ ##name##PollState; \

#define pollerUnlock() \
   __pollState->complete(GetCurrentTimeMs() - __pollStartTime); \
   _pollerUnlock();

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
 * Poll state information
 */
class NXCORE_EXPORTABLE PollState
{
private:
   VolatileCounter m_pollerCount;
   time_t m_lastCompleted;
   ManualGauge64 *m_timer;
   MUTEX m_lock;

public:
   PollState(const TCHAR *name)
   {
      m_pollerCount = 0;
      m_lastCompleted = NEVER;
      m_timer = new ManualGauge64(name, 1, 1000);
      m_lock = MutexCreateFast();
   }

   ~PollState()
   {
      delete m_timer;
      MutexDestroy(m_lock);
   }

   /**
    * Check if poll is pending
    */
   bool isPending()
   {
      bool pending = (InterlockedIncrement(&m_pollerCount) > 1);
      InterlockedDecrement(&m_pollerCount);
      return pending;
   }

   /**
    * Schedule execution if there are no active/queued pollers
    */
   bool schedule()
   {
      if (InterlockedIncrement(&m_pollerCount) > 1)
      {
         InterlockedDecrement(&m_pollerCount);
         return false;
      }
      return true;
   }

   /**
    * Notify about manual poller start
    */
   void manualStart()
   {
      InterlockedIncrement(&m_pollerCount);
   }

   /**
    * Notify about poller completion
    */
   void complete(INT64 elapsed)
   {
      MutexLock(m_lock);
      m_lastCompleted = time(NULL);
      m_timer->update(elapsed);
      InterlockedDecrement(&m_pollerCount);
      MutexUnlock(m_lock);
   }

   /**
    * Reset poll timer
    */
   void resetTimer()
   {
      MutexLock(m_lock);
      m_timer->reset();
      MutexUnlock(m_lock);
   }

   /**
    * Get timestamp of last completed poll
    */
   time_t getLastCompleted()
   {
      MutexLock(m_lock);
      time_t t = m_lastCompleted;
      MutexUnlock(m_lock);
      return t;
   }

   /**
    * Get timer average value
    */
   INT64 getTimerAverage()
   {
      MutexLock(m_lock);
      INT64 v = static_cast<INT64>(m_timer->getAverage());
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get timer minimum value
    */
   INT64 getTimerMin()
   {
      MutexLock(m_lock);
      INT64 v = m_timer->getMin();
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get timer maximum value
    */
   INT64 getTimerMax()
   {
      MutexLock(m_lock);
      INT64 v = m_timer->getMax();
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get last timer value
    */
   INT64 getTimerLast()
   {
      MutexLock(m_lock);
      INT64 v = m_timer->getCurrent();
      MutexUnlock(m_lock);
      return v;
   }
};

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public DataCollectionOwner
{
private:
   typedef DataCollectionOwner super;

protected:
   IntegerArray<UINT32> *m_deletedItems;
   IntegerArray<UINT32> *m_deletedTables;
   StringMap *m_scriptErrorReports;
   PollState m_statusPollState;
   PollState m_configurationPollState;
   PollState m_instancePollState;
   MUTEX m_hPollerMutex;
   double m_proxyLoadFactor;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
	virtual void fillMessageInternalStage2(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionLoad() override;
   virtual void onDataCollectionChange() override;
	virtual bool isDataCollectionDisabled();

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual void instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);

   virtual StringMap *getInstanceList(DCObject *dco);
   void doInstanceDiscovery(UINT32 requestId);
   bool updateInstances(DCObject *root, StringObjectMap<InstanceDiscoveryData> *instances, UINT32 requestId);

   void updateDataCollectionTimeIntervals();

   void _pollerLock() { MutexLock(m_hPollerMutex); }
   void _pollerUnlock() { MutexUnlock(m_hPollerMutex); }

   NetObj *objectFromParameter(const TCHAR *param);

   NXSL_VM *runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject);

   void applyUserTemplates();
   void updateContainerMembership();

   void getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);
   void getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);

   void addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco, UINT32 primaryProxyId);
   void addProxySnmpTarget(ProxyInfo *info, const Node *node);
   void calculateProxyLoad();

   virtual void collectProxyInfo(ProxyInfo *info);
   static void collectProxyInfoCallback(NetObj *object, void *data);

public:
   DataCollectionTarget();
   DataCollectionTarget(const TCHAR *name);
   virtual ~DataCollectionTarget();

   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual bool setMgmtStatus(BOOL isManaged) override;
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;
   virtual bool isDataCollectionTarget() override;

   virtual void enterMaintenanceMode(const TCHAR *comments) override;
   virtual void leaveMaintenanceMode() override;

   virtual DataCollectionError getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer);
   virtual DataCollectionError getScriptItem(const TCHAR *param, size_t bufSize, TCHAR *buffer, DataCollectionTarget *targetObject);
   virtual DataCollectionError getScriptTable(const TCHAR *param, Table **result, DataCollectionTarget *targetObject);
   virtual DataCollectionError getWebServiceItem(const TCHAR *param, TCHAR *buffer, size_t bufSize);

   virtual UINT32 getEffectiveSourceNode(DCObject *dco);

   virtual json_t *toJson() override;

   NXSL_Array *getTemplatesForNXSL(NXSL_VM *vm);

   DataCollectionError getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject);
   DataCollectionError getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject);

   UINT32 getTableLastValues(UINT32 dciId, NXCPMessage *msg);
   UINT32 getThresholdSummary(NXCPMessage *msg, UINT32 baseId, UINT32 userId);
   UINT32 getPerfTabDCIList(NXCPMessage *pMsg, UINT32 userId);
   void getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);
   UINT32 getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId);
   double getProxyLoadFactor() { return GetAttributeWithLock(m_proxyLoadFactor, m_mutexProperties); }

   void updateDciCache();
   void updateDCItemCacheSize(UINT32 dciId, UINT32 conditionId = 0);
   void reloadDCItemCache(UINT32 dciId);
   void cleanDCIData(DB_HANDLE hdb);
   void calculateDciCutoffTimes(time_t *cutoffTimeIData, time_t *cutoffTimeTData);
   void queueItemsForPolling();
   bool processNewDCValue(shared_ptr<DCObject> dco, time_t currTime, void *value);
   void scheduleItemDataCleanup(UINT32 dciId);
   void scheduleTableDataCleanup(UINT32 dciId);
   void queuePredictionEngineTraining();

   bool applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject);
   void cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList);
   virtual void unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI);

   virtual bool isEventSource() override;

   uint32_t getEffectiveWebServiceProxy();

   int getMostCriticalDCIStatus();

   void statusPollWorkerEntry(PollerInfo *poller);
   void statusPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   void statusPollPollerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual bool lockForStatusPoll();
   void startForcedStatusPoll() { m_statusPollState.manualStart(); }

   void configurationPollWorkerEntry(PollerInfo *poller);
   void configurationPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual bool lockForConfigurationPoll();
   void startForcedConfigurationPoll() { m_configurationPollState.manualStart(); }

   void instanceDiscoveryPollWorkerEntry(PollerInfo *poller);
   void instanceDiscoveryPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual bool lockForInstancePoll();
   void startForcedInstancePoll() { m_instancePollState.manualStart(); }

   virtual void resetPollTimers();

   UINT64 getCacheMemoryUsage();
};

/**
 * Lock object for instance discovery poll
 */
inline bool DataCollectionTarget::lockForInstancePoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
	    (!(m_flags & DCF_DISABLE_CONF_POLL)) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<UINT32>(time(NULL) - m_instancePollState.getLastCompleted()) > g_instancePollingInterval))
   {
      success = m_instancePollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Lock object for configuration poll
 */
inline bool DataCollectionTarget::lockForConfigurationPoll()
{
   bool success = false;
   lockProperties();
	if (!m_isDeleted && !m_isDeleteInitiated)
	{
      if (m_runtimeFlags & ODF_FORCE_CONFIGURATION_POLL)
      {
         success = m_configurationPollState.schedule();
         if (success)
            m_runtimeFlags &= ~ODF_FORCE_CONFIGURATION_POLL;
      }
      else if ((m_status != STATUS_UNMANAGED) &&
               (!(m_flags & DCF_DISABLE_CONF_POLL)) &&
               (static_cast<UINT32>(time(NULL) - m_configurationPollState.getLastCompleted()) > g_configurationPollingInterval))
      {
         success = m_configurationPollState.schedule();
      }
	}
   unlockProperties();
   return success;
}

inline bool DataCollectionTarget::lockForStatusPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if (m_runtimeFlags & ODF_FORCE_STATUS_POLL)
      {
         success = m_statusPollState.schedule();
         if (success)
            m_runtimeFlags &= ~ODF_FORCE_STATUS_POLL;
      }
      else if ((m_status != STATUS_UNMANAGED) &&
               (!(m_flags & DCF_DISABLE_STATUS_POLL)) &&
               (!(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING)) &&
               ((UINT32)(time(NULL) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval))
      {
         success = m_statusPollState.schedule();
      }
   }
   unlockProperties();
   return success;
}

/**
 * Mobile device class
 */
class NXCORE_EXPORTABLE MobileDevice : public DataCollectionTarget
{
private:
   typedef DataCollectionTarget super;

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   virtual int getObjectClass() const override { return OBJECT_MOBILEDEVICE; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual json_t *toJson() override;

	void updateSystemInfo(NXCPMessage *msg);
	void updateStatus(NXCPMessage *msg);

	const TCHAR *getDeviceId() { return CHECK_NULL_EX(m_deviceId); }
	const TCHAR *getVendor() { return CHECK_NULL_EX(m_vendor); }
	const TCHAR *getModel() { return CHECK_NULL_EX(m_model); }
	const TCHAR *getSerialNumber() { return CHECK_NULL_EX(m_serialNumber); }
	const TCHAR *getOsName() { return CHECK_NULL_EX(m_osName); }
	const TCHAR *getOsVersion() { return CHECK_NULL_EX(m_osVersion); }
	const TCHAR *getUserId() { return CHECK_NULL_EX(m_userId); }
	LONG getBatteryLevel() { return m_batteryLevel; }

	virtual DataCollectionError getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer) override;

	virtual bool lockForStatusPoll() override { return false; }
	virtual bool lockForConfigurationPoll() override { return false; }
	virtual bool lockForInstancePoll() override { return false; }
};

/**
 * Access point class
 */
class NXCORE_EXPORTABLE AccessPoint : public DataCollectionTarget
{
private:
   typedef DataCollectionTarget super;

protected:
   UINT32 m_index;
   InetAddress m_ipAddress;
	UINT32 m_nodeId;
	MacAddress m_macAddr;
	TCHAR *m_vendor;
	TCHAR *m_model;
	TCHAR *m_serialNumber;
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;
   AccessPointState m_apState;
   AccessPointState m_prevState;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, UINT32 index, const BYTE *macAddr);
   virtual ~AccessPoint();

   virtual int getObjectClass() const override { return OBJECT_ACCESSPOINT; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual uint32_t getZoneUIN() const override;

   virtual json_t *toJson() override;

   void statusPollFromController(ClientSession *session, UINT32 rqId, ObjectQueue<Event> *eventQueue, Node *controller, SNMP_Transport *snmpTransport);

   UINT32 getIndex() const { return m_index; }
	const MacAddress& getMacAddr() const { return m_macAddr; }
   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
	bool isMyRadio(int rfIndex);
	bool isMyRadio(const BYTE *macAddr);
	void getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize);
   AccessPointState getApState() const { return m_apState; }
   Node *getParentNode() const;
   bool isIncludedInIcmpPoll() const { return (m_flags & APF_INCLUDE_IN_ICMP_POLL) ? true : false; }
   const TCHAR *getVendor() const { return CHECK_NULL_EX(m_vendor); }
   const TCHAR *getModel() const { return CHECK_NULL_EX(m_model); }
   const TCHAR *getSerialNumber() const { return CHECK_NULL_EX(m_serialNumber); }

	void attachToNode(UINT32 nodeId);
   void setIpAddress(const InetAddress& addr) { lockProperties(); m_ipAddress = addr; setModified(MODIFY_OTHER); unlockProperties(); }
	void updateRadioInterfaces(const ObjectArray<RadioInterfaceInfo> *ri);
	void updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber);
   void updateState(AccessPointState state);

   virtual bool lockForStatusPoll() override { return false; }
   virtual bool lockForConfigurationPoll() override { return false; }
   virtual bool lockForInstancePoll() override { return false; }
};

/**
 * Cluster class
 */
class NXCORE_EXPORTABLE Cluster : public DataCollectionTarget
{
private:
   typedef DataCollectionTarget super;

protected:
	UINT32 m_dwClusterType;
   ObjectArray<InetAddress> *m_syncNetworks;
	UINT32 m_dwNumResources;
	CLUSTER_RESOURCE *m_pResourceList;
	UINT32 m_zoneUIN;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionChange() override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

   UINT32 getResourceOwnerInternal(UINT32 id, const TCHAR *name);

public:
	Cluster();
   Cluster(const TCHAR *pszName, UINT32 zoneUIN);
	virtual ~Cluster();

   virtual int getObjectClass() const override { return OBJECT_CLUSTER; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool showThresholdSummary() override;

   virtual bool lockForInstancePoll() override { return false; }

   virtual void unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual uint32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

	bool isSyncAddr(const InetAddress& addr);
	bool isVirtualAddr(const InetAddress& addr);
	bool isResourceOnNode(UINT32 dwResource, UINT32 dwNode);
	UINT32 getResourceOwner(UINT32 resourceId) { return getResourceOwnerInternal(resourceId, NULL); }
   UINT32 getResourceOwner(const TCHAR *resourceName) { return getResourceOwnerInternal(0, resourceName); }

   UINT32 collectAggregatedData(DCItem *item, TCHAR *buffer);
   UINT32 collectAggregatedData(DCTable *table, Table **result);

   NXSL_Array *getNodesForNXSL(NXSL_VM *vm);
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
private:
   typedef DataCollectionTarget super;

protected:
   uint32_t m_controllerId;
   int16_t m_rackHeight;
   int16_t m_rackPosition;
   uint32_t m_rackId;
   uuid m_rackImageFront;
   uuid m_rackImageRear;
   RackOrientation m_rackOrientation;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   virtual void onDataCollectionChange() override;
   virtual void collectProxyInfo(ProxyInfo *info) override;

   void updateRackBinding();
   void updateControllerBinding();

public:
   Chassis();
   Chassis(const TCHAR *name, UINT32 controllerId);
   virtual ~Chassis();

   virtual int getObjectClass() const override { return OBJECT_CHASSIS; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;
   virtual bool showThresholdSummary() override;
   virtual UINT32 getEffectiveSourceNode(DCObject *dco) override;

   virtual bool lockForStatusPoll() override { return false; }
   virtual bool lockForConfigurationPoll() override { return false; }
   virtual bool lockForInstancePoll() override { return false; }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual json_t *toJson() override;

   uint32_t getControllerId() const { return m_controllerId; }
   uint32_t getRackId() const { return m_rackId; }
   int16_t getRackHeight() const { return m_rackHeight; }
   int16_t getRackPosition() const { return m_rackPosition; }
   bool bindUnderController() { return (m_flags & CHF_BIND_UNDER_CONTROLLER) ? true : false; }

   void setBindUnderController(bool doBind);
};

/**
 * Sensor communication protocol type
 */
#define SENSOR_PROTO_UNKNOWN  0
#define COMM_LORAWAN          1
#define COMM_DLMS             2

/**
 * Sensor device class
 */
#define SENSOR_CLASS_UNKNOWN 0
#define SENSOR_UPS           1
#define SENSOR_WATER_METER   2
#define SENSOR_ELECTR_METER  3

/**
 * Mobile device class
 */
class NXCORE_EXPORTABLE Sensor : public DataCollectionTarget
{
   friend class Node;

private:
   typedef DataCollectionTarget super;

protected:
	MacAddress m_macAddress;
	UINT32 m_deviceClass; // Internal device class UPS, meeter
	TCHAR *m_vendor; //Vendoer name lorawan...
	UINT32 m_commProtocol; // lorawan, dlms, dlms throuht other protocols
	TCHAR *m_xmlConfig; //protocol specific configuration
	TCHAR *m_xmlRegConfig; //protocol specific registration configuration (cannot be changed afterwards)
	TCHAR *m_serialNumber; //Device serial number
	TCHAR *m_deviceAddress; //in case lora - lorawan id
	TCHAR *m_metaType;//brief type hot water, elecrticety
	TCHAR *m_description; //brief description
	time_t m_lastConnectionTime;
	UINT32 m_frameCount; //zero when no info
   INT32 m_signalStrenght; //+1 when no information(cannot be +)
   INT32 m_signalNoise; //*10 from origin number //MAX_INT32 when no value
   UINT32 m_frequency; //*10 from origin number // 0 when no value
   UINT32 m_proxyNodeId;

	virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

   virtual StringMap *getInstanceList(DCObject *dco) override;

   void calculateStatus(BOOL bForcedRecalc = FALSE);

   Sensor(TCHAR *name, UINT32 flags, MacAddress macAddress, UINT32 deviceClass, TCHAR *vendor,
               UINT32 commProtocol, TCHAR *xmlRegConfig, TCHAR *xmlConfig, TCHAR *serialNumber, TCHAR *deviceAddress,
               TCHAR *metaType, TCHAR *description, UINT32 proxyNode);
   static Sensor *registerLoraDevice(Sensor *sensor);

   void buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies);

public:
   Sensor();

   virtual ~Sensor();
   static Sensor *createSensor(TCHAR *name, NXCPMessage *msg);

   virtual int getObjectClass() const override { return OBJECT_SENSOR; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual void prepareForDeletion() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   const TCHAR *getXmlConfig() const { return m_xmlConfig; }
   const TCHAR *getXmlRegConfig() const { return m_xmlRegConfig; }
   UINT32 getProxyNodeId() const { return m_proxyNodeId; }
   const TCHAR *getDeviceAddress() const { return m_deviceAddress; }
   const MacAddress getMacAddress() const { return m_macAddress; }
   time_t getLastContact() const { return m_lastConnectionTime; }
   UINT32 getSensorClass() const { return m_deviceClass; }
   const TCHAR *getVendor() const { return m_vendor; }
   UINT32 getCommProtocol() const { return m_commProtocol; }
   const TCHAR *getSerialNumber() const { return m_serialNumber; }
   const TCHAR *getMetaType() const { return m_metaType; }
   const TCHAR *getDescription() const { return m_description; }
   UINT32 getFrameCount() const { return m_frameCount; }

   DataCollectionError getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
   DataCollectionError getListFromAgent(const TCHAR *name, StringList **list);

   void setProvisoned() { m_state |= SSF_PROVISIONED; }

   virtual json_t *toJson() override;

   AgentConnectionEx *getAgentConnection();

   void checkDlmsConverterAccessibility();
   void prepareDlmsDciParameters(StringBuffer &parameter);
   void prepareLoraDciParameters(StringBuffer &parameter);

   NetworkMapObjectList *buildInternalConnectionTopology();
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
   NODE_TYPE_CONTROLLER = 3,
   NODE_TYPE_CONTAINER = 4
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

/**
 * Proxy types
 */
enum ProxyType
{
   SNMP_PROXY = 0,
   SENSOR_PROXY = 1,
   ZONE_PROXY = 2,
   ETHERNET_IP_PROXY = 3,
   WEB_SERVICE_PROXY = 4,
   MAX_PROXY_TYPE = 5
};

/**
 * Duplicate check result
 */
enum DuplicateCheckResult
{
   NO_DUPLICATES,
   REMOVE_THIS,
   REMOVE_OTHER
};

/**
 * ICMP statistic function
 */
enum class IcmpStatFunction
{
   LAST,
   MIN,
   MAX,
   AVERAGE,
   LOSS
};

/**
 * Container for proxy agent connections
 */
class ProxyAgentConnection : public ObjectLock<AgentConnectionEx>
{
private:
   time_t m_lastConnect;

public:
   ProxyAgentConnection() : ObjectLock<AgentConnectionEx>() { m_lastConnect = 0; }
   ProxyAgentConnection(AgentConnectionEx *object) : ObjectLock<AgentConnectionEx>(object) { m_lastConnect = 0; }
   ProxyAgentConnection(const ProxyAgentConnection &src) : ObjectLock<AgentConnectionEx>(src) { m_lastConnect = src.m_lastConnect; }

   void setLastConnectTime(time_t t) { m_lastConnect = t; }
   time_t getLastConnectTime() const { return m_lastConnect; }
};

// Explicit instantiation of shared_ptr<*> to enable DLL interface for them
#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<VlanList>;
template class NXCORE_EXPORTABLE shared_ptr<ComponentTree>;
#endif

/**
 * Node
 */
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
	friend class Subnet;
   friend void Sensor::buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies);

private:
   typedef DataCollectionTarget super;

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

	bool updateSystemHardwareProperty(SharedString &property, const TCHAR *value, const TCHAR *displayName, UINT32 requestId);

   static void onDataCollectionChangeAsyncCallback(void *arg);

protected:
   InetAddress m_ipAddress;
	TCHAR m_primaryName[MAX_DNS_NAME];
	uuid m_tunnelId;
	uint32_t m_capabilities;
   NodeType m_type;
   TCHAR m_subType[MAX_NODE_SUBTYPE_LENGTH];
   TCHAR m_hypervisorType[MAX_HYPERVISOR_TYPE_LENGTH];
   SharedString m_hypervisorInfo;
   SharedString m_vendor;
   SharedString m_productName;
   SharedString m_productVersion;
   SharedString m_productCode;
   SharedString m_serialNumber;
	int m_pendingState;
	uint32_t m_pollCountAgent;
	uint32_t m_pollCountSNMP;
	uint32_t m_pollCountEtherNetIP;
	uint32_t m_pollCountAllDown;
	uint32_t m_requiredPollCount;
	uint32_t m_zoneUIN;
   uint16_t m_agentPort;
   INT16 m_agentAuthMethod;
   INT16 m_agentCacheMode;
   INT16 m_agentCompressionMode;  // agent compression mode (enabled/disabled/default)
   TCHAR m_szSharedSecret[MAX_SECRET_LENGTH];
   INT16 m_iStatusPollType;
   SNMP_Version m_snmpVersion;
   UINT16 m_snmpPort;
	UINT16 m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
	uuid m_agentId;
	TCHAR *m_agentCertSubject;
   TCHAR m_agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_platformName[MAX_PLATFORM_NAME_LEN];
   TCHAR *m_snmpObjectId;
	TCHAR *m_sysDescription;   // Agent's System.Uname or SNMP sysDescr
	TCHAR *m_sysName;				// SNMP sysName
	TCHAR *m_sysLocation;      // SNMP sysLocation
	TCHAR *m_sysContact;       // SNMP sysContact
	TCHAR *m_lldpNodeId;			// lldpLocChassisId combined with lldpLocChassisIdSubtype, or NULL for non-LLDP nodes
	ObjectArray<LLDP_LOCAL_PORT_INFO> *m_lldpLocalPortInfo;
	NetworkDeviceDriver *m_driver;
	DriverData *m_driverData;
   ObjectArray<AgentParameterDefinition> *m_agentParameters; // List of metrics supported by agent
   ObjectArray<AgentTableDefinition> *m_agentTables; // List of supported tables
   ObjectArray<AgentParameterDefinition> *m_driverParameters; // List of metrics supported by driver
   PollState m_discoveryPollState;
   PollState m_topologyPollState;
   PollState m_routingPollState;
   PollState m_icmpPollState;
   time_t m_failTimeAgent;
   time_t m_failTimeSNMP;
   time_t m_failTimeEtherNetIP;
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
   ProxyAgentConnection *m_proxyConnections;
   VolatileCounter m_pendingDataConfigurationSync;
   SMCLP_Connection *m_smclpConnection;
	UINT64 m_lastAgentTrapId;	     // ID of last received agent trap
   UINT64 m_lastAgentPushRequestId; // ID of last received agent push request
   UINT32 m_lastSNMPTrapId;
   UINT64 m_lastSyslogMessageId; // ID of last received syslog message
   UINT32 m_pollerNode;      // Node used for network service polling
   UINT32 m_agentProxy;      // Node used as proxy for agent connection
	UINT32 m_snmpProxy;       // Node used as proxy for SNMP requests
   UINT32 m_eipProxy;        // Node used as proxy for EtherNet/IP requests
   UINT32 m_icmpProxy;       // Node used as proxy for ICMP ping
   UINT64 m_lastEvents[MAX_LAST_EVENTS];
   ObjectArray<RoutingLoopEvent> *m_routingLoopEvents;
   ROUTING_TABLE *m_pRoutingTable;
	ForwardingDatabase *m_fdb;
	ArpCache *m_arpCache;
	LinkLayerNeighbors *m_linkLayerNeighbors;
	shared_ptr<VlanList> m_vlans;
	VrrpInfo *m_vrrpInfo;
	ObjectArray<WirelessStationInfo> *m_wirelessStations;
	int m_adoptedApCount;
	int m_totalApCount;
	BYTE m_baseBridgeAddress[MAC_ADDR_LENGTH];	// Bridge base address (dot1dBaseBridgeAddress in bridge MIB)
	NetworkMapObjectList *m_topology;
	time_t m_topologyRebuildTimestamp;
	ServerJobQueue *m_jobQueue;
	shared_ptr<ComponentTree> m_components;		// Hardware components
	ObjectArray<SoftwarePackage> *m_softwarePackages;  // installed software packages
   ObjectArray<HardwareComponent> *m_hardwareComponents;  // installed hardware components
	ObjectArray<WinPerfObject> *m_winPerfObjects;  // Windows performance objects
	AgentConnection *m_fileUpdateConn;
	INT16 m_rackHeight;
	INT16 m_rackPosition;
	UINT32 m_physicalContainer;
	uuid m_rackImageFront;
   uuid m_rackImageRear;
	INT64 m_syslogMessageCount;
	INT64 m_snmpTrapCount;
	TCHAR m_sshLogin[MAX_SSH_LOGIN_LEN];
	TCHAR m_sshPassword[MAX_SSH_PASSWORD_LEN];
	UINT32 m_sshProxy;
	UINT32 m_portNumberingScheme;
	UINT32 m_portRowCount;
	RackOrientation m_rackOrientation;
   IcmpStatCollectionMode m_icmpStatCollectionMode;
   StringObjectMap<IcmpStatCollector> *m_icmpStatCollectors;
   InetAddressList m_icmpTargets;
   TCHAR *m_chassisPlacementConf;
   uint16_t m_eipPort;  // EtherNet/IP port
   uint16_t m_cipDeviceType;
   uint16_t m_cipStatus;
   uint16_t m_cipState;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

   void topologyPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   void routingTablePoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   void icmpPoll(PollerInfo *poller);

   virtual bool isDataCollectionDisabled() override;
   virtual void collectProxyInfo(ProxyInfo *info) override;

   virtual void prepareForDeletion() override;
   virtual void onObjectDelete(UINT32 objectId) override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionChange() override;

   virtual StringMap *getInstanceList(DCObject *dco) override;

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
   Interface *createInterfaceObject(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface, bool syntheticMask);
   Subnet *createSubnet(InetAddress& baseAddr, bool syntheticMask);
   void checkAgentPolicyBinding(AgentConnection *conn);
   void updatePrimaryIpAddr();
   bool confPollAgent(uint32_t requestId);
   bool confPollSnmp(uint32_t requestId);
   bool confPollEthernetIP(uint32_t requestId);
   NodeType detectNodeType(TCHAR *hypervisorType, TCHAR *hypervisorInfo);
   bool updateSystemHardwareInformation(PollerInfo *poller, UINT32 requestId);
   bool updateHardwareComponents(PollerInfo *poller, UINT32 requestId);
   bool updateSoftwarePackages(PollerInfo *poller, UINT32 requestId);
   bool querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, UINT32 pollRqId, TCHAR **value);
   void checkBridgeMib(SNMP_Transport *pTransport);
   void checkIfXTable(SNMP_Transport *pTransport);
   bool checkNetworkPath(UINT32 requestId);
   bool checkNetworkPathLayer2(UINT32 requestId, bool secondPass);
   bool checkNetworkPathLayer3(UINT32 requestId, bool secondPass);
   bool checkNetworkPathElement(UINT32 nodeId, const TCHAR *nodeType, bool isProxy, UINT32 requestId, bool secondPass);
   void icmpPollAddress(AgentConnection *conn, const TCHAR *target, const InetAddress& addr);

   void syncDataCollectionWithAgent(AgentConnectionEx *conn);

   bool updateInterfaceConfiguration(UINT32 rqid, int maskBits);
   bool deleteDuplicateInterfaces(UINT32 rqid);
   void executeInterfaceUpdateHook(Interface *iface);
   void updatePhysicalContainerBinding(UINT32 containerId);
   DuplicateCheckResult checkForDuplicates(Node **duplicate, TCHAR *reason, size_t size);
   bool isDuplicateOf(Node *node, TCHAR *reason, size_t size);
   void reconcileWithDuplicateNode(Node *node);

   bool connectToAgent(UINT32 *error = NULL, UINT32 *socketError = NULL, bool *newConnection = NULL, bool forceConnect = false);
   void setLastAgentCommTime() { m_lastAgentCommTime = time(NULL); }

   void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedObject, const TCHAR *linkName, bool vpnLink, bool includeEndNodes);
   void buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology);
   void buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, UINT32 seedNode, bool agentConnectionOnly, bool checkAllProxies);
   bool checkProxyAndLink(NetworkMapObjectList *topology, UINT32 seedNode, UINT32 proxyId, UINT32 linkType, const TCHAR *linkName, bool checkAllProxies);

public:
   Node();
   Node(const NewNodeData *newNodeData, UINT32 flags);
   virtual ~Node();

   virtual int getObjectClass() const override { return OBJECT_NODE; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool saveRuntimeData(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool lockForStatusPoll() override;
   bool lockForDiscoveryPoll();
   bool lockForRoutePoll();
   bool lockForTopologyPoll();
   bool lockForIcmpPoll();
   void startForcedDiscoveryPoll() { m_discoveryPollState.manualStart(); }
   void startForcedRoutePoll() { m_routingPollState.manualStart(); }
   void startForcedTopologyPoll() { m_topologyPollState.manualStart(); }

   void completeDiscoveryPoll(INT64 elapsedTime) { m_discoveryPollState.complete(elapsedTime); }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual uint32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   Cluster *getMyCluster();

   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
   NodeType getType() const { return m_type; }
   bool isVirtual() const { return (m_type == NODE_TYPE_VIRTUAL) || (m_type == NODE_TYPE_CONTAINER); }
   const TCHAR *getSubType() { return m_subType; }
   const TCHAR *getHypervisorType() const { return m_hypervisorType; }
   SharedString getHypervisorInfo() const { return GetAttributeWithLock(m_hypervisorInfo, m_mutexProperties); }
   SharedString getVendor() const { return GetAttributeWithLock(m_vendor, m_mutexProperties); }
   SharedString getProductName() const { return GetAttributeWithLock(m_productName, m_mutexProperties); }
   SharedString getProductVersion() const { return GetAttributeWithLock(m_productVersion, m_mutexProperties); }
   SharedString getProductCode() const { return GetAttributeWithLock(m_productCode, m_mutexProperties); }
   SharedString getSerialNumber() const { return GetAttributeWithLock(m_serialNumber, m_mutexProperties); }

   uint32_t getCapabilities() { return m_capabilities; }
   void setCapabilities(uint32_t flag) { lockProperties(); m_capabilities |= flag; setModified(MODIFY_NODE_PROPERTIES); unlockProperties(); }
   void clearCapabilities(uint32_t flag) { lockProperties(); m_capabilities &= ~flag; setModified(MODIFY_NODE_PROPERTIES); unlockProperties(); }
   void setLocalMgmtFlag() { setCapabilities(NC_IS_LOCAL_MGMT); }
   void clearLocalMgmtFlag() { clearCapabilities(NC_IS_LOCAL_MGMT); }

   void setType(NodeType type, const TCHAR *subType) { lockProperties(); m_type = type; _tcslcpy(m_subType, subType, MAX_NODE_SUBTYPE_LENGTH); unlockProperties(); }

   bool isSNMPSupported() const { return m_capabilities & NC_IS_SNMP ? true : false; }
   bool isNativeAgent() const { return m_capabilities & NC_IS_NATIVE_AGENT ? true : false; }
   bool isEthernetIPSupported() const { return m_capabilities & NC_IS_ETHERNET_IP ? true : false; }
   bool isModbusTCPSupported() const { return m_capabilities & NC_IS_MODBUS_TCP ? true : false; }
   bool isProfiNetSupported() const { return m_capabilities & NC_IS_PROFINET ? true : false; }
   bool isBridge() const { return m_capabilities & NC_IS_BRIDGE ? true : false; }
   bool isRouter() const { return m_capabilities & NC_IS_ROUTER ? true : false; }
   bool isLocalManagement() const { return m_capabilities & NC_IS_LOCAL_MGMT ? true : false; }
   bool isPerVlanFdbSupported() const { return (m_driver != NULL) ? m_driver->isPerVlanFdbSupported() : false; }
   bool isWirelessController() const { return m_capabilities & NC_IS_WIFI_CONTROLLER ? true : false; }
   bool supportNewTypeFormat() const { return m_capabilities & NC_IS_NEW_POLICY_TYPES ? true : false; }

   bool isIcmpStatCollectionEnabled() const
   {
      return (m_icmpStatCollectionMode == IcmpStatCollectionMode::DEFAULT) ?
               ((g_flags & AF_COLLECT_ICMP_STATISTICS) != 0) : (m_icmpStatCollectionMode == IcmpStatCollectionMode::ON);
   }

   const uuid& getAgentId() const { return m_agentId; }
   const TCHAR *getAgentVersion() const { return m_agentVersion; }
   const TCHAR *getPlatformName() const { return m_platformName; }
   SNMP_Version getSNMPVersion() const { return m_snmpVersion; }
   UINT16 getSNMPPort() const { return m_snmpPort; }
   UINT32 getSNMPProxy() const { return m_snmpProxy; }
   String getSNMPObjectId() const { return GetStringAttributeWithLock(m_snmpObjectId, m_mutexProperties); }
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
   INT16 getAgentCacheMode() const { return (m_state & NSF_CACHE_MODE_NOT_SUPPORTED) ? AGENT_CACHE_OFF : ((m_agentCacheMode == AGENT_CACHE_DEFAULT) ? g_defaultAgentCacheMode : m_agentCacheMode); }
   const TCHAR *getSharedSecret() const { return m_szSharedSecret; }
   UINT32 getAgentProxy() const { return m_agentProxy; }
   UINT32 getPhysicalContainerId() const { return m_physicalContainer; }
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
   const TCHAR *getAgentCertificateSubject() const { return m_agentCertSubject; }
   UINT32 getRequiredPollCount() const { return m_requiredPollCount; }
   uint16_t getCipDeviceType() const { return m_cipDeviceType; }
   uint16_t getCipStatus() const { return m_cipStatus; }
   uint16_t getCipState() const { return m_cipState; }

   bool isDown() { return (m_state & DCSF_UNREACHABLE) ? true : false; }
   time_t getDownSince() const { return m_downSince; }

   void setNewTunnelBindFlag() { lockProperties(); m_runtimeFlags |= NDF_NEW_TUNNEL_BIND; unlockProperties(); }
   void clearNewTunnelBindFlag() { lockProperties(); m_runtimeFlags &= ~NDF_NEW_TUNNEL_BIND; unlockProperties(); }

   void addInterface(Interface *pInterface) { addChild(pInterface); pInterface->addParent(this); }
   Interface *createNewInterface(InterfaceInfo *ifInfo, bool manuallyCreated, bool fakeInterface);
   Interface *createNewInterface(const InetAddress& ipAddr, const MacAddress& macAddr, bool fakeInterface);
   void deleteInterface(Interface *iface);

   void setPrimaryName(const TCHAR *name) { lockProperties(); _tcslcpy(m_primaryName, name, MAX_DNS_NAME); unlockProperties(); }
   void setAgentPort(UINT16 port) { m_agentPort = port; }
   void setSnmpPort(UINT16 port) { m_snmpPort = port; }
   void setSshCredentials(const TCHAR *login, const TCHAR *password);
   void changeIPAddress(const InetAddress& ipAddr);
   void changeZone(UINT32 newZone);
   void setTunnelId(const uuid& tunnelId, const TCHAR *certSubject);
   void setFileUpdateConnection(AgentConnection *conn);
   void clearDataCollectionConfigFromAgent(AgentConnectionEx *conn);
   void forceSyncDataCollectionConfig();
   void relatedNodeDataCollectionChanged() { onDataCollectionChange(); }

   ArpCache *getArpCache(bool forceRead = false);
   InterfaceList *getInterfaceList();
   Interface *findInterfaceByIndex(UINT32 ifIndex);
   Interface *findInterfaceByName(const TCHAR *name);
   Interface *findInterfaceByAlias(const TCHAR *alias);
   Interface *findInterfaceByMAC(const MacAddress& macAddr);
   Interface *findInterfaceByIP(const InetAddress& addr);
   Interface *findInterfaceBySubnet(const InetAddress& subnet);
   Interface *findInterfaceByLocation(const InterfacePhysicalLocation& location);
   Interface *findBridgePort(UINT32 bridgePortNumber);
   AccessPoint *findAccessPointByMAC(const MacAddress& macAddr);
   AccessPoint *findAccessPointByBSSID(const BYTE *bssid);
   AccessPoint *findAccessPointByRadioId(int rfIndex);
   ObjectArray<WirelessStationInfo> *getWirelessStations();
   bool isMyIP(const InetAddress& addr);
   void getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 dwIndex, int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState);
   void getInterfaceStatusFromAgent(UINT32 dwIndex, InterfaceAdminState *adminState, InterfaceOperState *operState);
   ROUTING_TABLE *getRoutingTable();
   ROUTING_TABLE *getCachedRoutingTable() { return m_pRoutingTable; }
   LinkLayerNeighbors *getLinkLayerNeighbors();
   shared_ptr<VlanList> getVlans();
   bool getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, UINT32 *ifIndex, bool *isVpn, TCHAR *name);
   bool getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, UINT32 *srcIfIndex);
   shared_ptr<ComponentTree> getComponents();
   bool getLldpLocalPortInfo(UINT32 idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port);
   void showLLDPInfo(CONSOLE_CTX console);

	void setRecheckCapsFlag() { lockProperties(); m_runtimeFlags |= NDF_RECHECK_CAPABILITIES; unlockProperties(); }
	void topologyPollWorkerEntry(PollerInfo *poller);
	void topologyPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
	void resolveVlanPorts(VlanList *vlanList);
	void updateInterfaceNames(ClientSession *pSession, UINT32 dwRqId);
	void routingTablePollWorkerEntry(PollerInfo *poller);
   void routingTablePollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   void icmpPollWorkerEntry(PollerInfo *poller);
	void checkSubnetBinding();
   AccessPointState getAccessPointState(AccessPoint *ap, SNMP_Transport *snmpTransport, const ObjectArray<RadioInterfaceInfo> *radioInterfaces);
   virtual void resetPollTimers() override;

	void forceConfigurationPoll() { lockProperties(); m_runtimeFlags |= ODF_FORCE_CONFIGURATION_POLL; unlockProperties(); }

	virtual bool setMgmtStatus(BOOL isManaged) override;
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

	bool checkAgentTrapId(UINT64 id);
	bool checkSNMPTrapId(UINT32 id);
   bool checkSyslogMessageId(UINT64 id);
   bool checkAgentPushRequestId(UINT64 id);

   bool connectToSMCLP();

   virtual DataCollectionError getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer) override;

   DataCollectionError getItemFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *param, size_t bufSize, TCHAR *buffer, int interpretRawValue);
   DataCollectionError getTableFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, ObjectArray<DCTableColumn> *columns, Table **table);
   DataCollectionError getListFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, StringList **list);
   DataCollectionError getOIDSuffixListFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, StringMap **values);
   DataCollectionError getItemFromAgent(const TCHAR *szParam, UINT32 dwBufSize, TCHAR *szBuffer);
   DataCollectionError getTableFromAgent(const TCHAR *name, Table **table);
   DataCollectionError getListFromAgent(const TCHAR *name, StringList **list);
   DataCollectionError getItemFromSMCLP(const TCHAR *param, TCHAR *buffer, size_t size);
   DataCollectionError getItemFromDeviceDriver(const TCHAR *param, TCHAR *buffer, size_t size);

   double getMetricFromAgentAsDouble(const TCHAR *name, double defaultValue = 0);
   INT32 getMetricFromAgentAsInt32(const TCHAR *name, INT32 defaultValue = 0);
   UINT32 getMetricFromAgentAsUInt32(const TCHAR *name, UINT32 defaultValue = 0);
   INT64 getMetricFromAgentAsInt64(const TCHAR *name, INT64 defaultValue = 0);
   UINT64 getMetricFromAgentAsUInt64(const TCHAR *name, UINT64 defaultValue = 0);

   UINT32 getItemForClient(int iOrigin, UINT32 userId, const TCHAR *pszParam, TCHAR *pszBuffer, UINT32 dwBufSize);
   UINT32 getTableForClient(const TCHAR *name, Table **table);

	virtual NXSL_Array *getParentsForNXSL(NXSL_VM *vm) override;
	NXSL_Array *getInterfacesForNXSL(NXSL_VM *vm);

   ObjectArray<AgentParameterDefinition> *openParamList(int origin);
   void closeParamList() { unlockProperties(); }

   ObjectArray<AgentTableDefinition> *openTableList();
   void closeTableList() { unlockProperties(); }

   AgentConnectionEx *createAgentConnection(bool sendServerId = false);
   AgentConnectionEx *getAgentConnection(bool forcePrimary = false);
   AgentConnectionEx *acquireProxyConnection(ProxyType type, bool validate = false);
	SNMP_Transport *createSnmpTransport(UINT16 port = 0, SNMP_Version version = SNMP_VERSION_DEFAULT, const TCHAR *context = NULL);
	SNMP_SecurityContext *getSnmpSecurityContext() const;

	uint32_t getEffectiveSnmpProxy(bool backup = false);
	uint32_t getEffectiveEtherNetIPProxy(bool backup = false);
	uint32_t getEffectiveSshProxy();
	uint32_t getEffectiveIcmpProxy();
	uint32_t getEffectiveAgentProxy();

   void writeParamListToMessage(NXCPMessage *pMsg, int origin, WORD flags);
	void writeWinPerfObjectsToMessage(NXCPMessage *msg);
	void writePackageListToMessage(NXCPMessage *msg);
	void writeWsListToMessage(NXCPMessage *msg);
	void writeHardwareListToMessage(NXCPMessage *msg);

   UINT32 wakeUp();

   void addService(NetworkService *pNetSrv) { addChild(pNetSrv); pNetSrv->addParent(this); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL, UINT32 *responseTime = NULL);

   UINT64 getLastEventId(int index) { return ((index >= 0) && (index < MAX_LAST_EVENTS)) ? m_lastEvents[index] : 0; }
   void setLastEventId(int index, UINT64 eventId) { if ((index >= 0) && (index < MAX_LAST_EVENTS)) m_lastEvents[index] = eventId; }
   void setRoutingLoopEvent(const InetAddress& address, UINT32 nodeId, UINT64 eventId);

   UINT32 callSnmpEnumerate(const TCHAR *pszRootOid,
      UINT32 (* pHandler)(SNMP_Variable *, SNMP_Transport *, void *), void *pArg,
      const TCHAR *context = NULL, bool failOnShutdown = false);

   NetworkMapObjectList *getL2Topology();
   NetworkMapObjectList *buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes);
	ForwardingDatabase *getSwitchForwardingDatabase();
	NetObj *findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type);
	void addHostConnections(LinkLayerNeighbors *nbs);
	void addExistingConnections(LinkLayerNeighbors *nbs);
	NetworkMapObjectList *buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes);

   bool getIcmpStatistics(const TCHAR *target, UINT32 *last, UINT32 *min, UINT32 *max, UINT32 *avg, UINT32 *loss);
   DataCollectionError getIcmpStatistic(const TCHAR *param, IcmpStatFunction function, TCHAR *value);
   StringList *getIcmpStatCollectors();

	ServerJobQueue *getJobQueue() { return m_jobQueue; }
	int getJobCount(const TCHAR *type = NULL) { return m_jobQueue->getJobCount(type); }

	DriverData *getDriverData() { return m_driverData; }
	void setDriverData(DriverData *data) { m_driverData = data; }

	void incSyslogMessageCount();
	void incSnmpTrapCount();

	static const TCHAR *typeName(NodeType type);

	NetworkMapObjectList *buildInternalConnectionTopology();
	NetworkMapObjectList *buildInternalCommunicationTopology();
};

inline bool Node::lockForStatusPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if (m_runtimeFlags & ODF_FORCE_STATUS_POLL)
      {
         success = m_statusPollState.schedule();
         if (success)
            m_runtimeFlags &= ~ODF_FORCE_STATUS_POLL;
      }
      else if ((m_status != STATUS_UNMANAGED) &&
               (!(m_flags & DCF_DISABLE_STATUS_POLL)) &&
               (getMyCluster() == NULL) &&
               (!(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING)) &&
               (static_cast<UINT32>(time(NULL) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval))
      {
         success = m_statusPollState.schedule();
      }
   }
   unlockProperties();
   return success;
}

inline bool Node::lockForDiscoveryPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (g_flags & AF_PASSIVE_NETWORK_DISCOVERY) &&
       (m_status != STATUS_UNMANAGED) &&
		 (!(m_flags & NF_DISABLE_DISCOVERY_POLL)) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<UINT32>(time(NULL) - m_discoveryPollState.getLastCompleted()) > g_discoveryPollingInterval))
   {
      success = m_discoveryPollState.schedule();
   }
   unlockProperties();
   return success;
}

inline bool Node::lockForRoutePoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
	    (!(m_flags & NF_DISABLE_ROUTE_POLL)) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<UINT32>(time(NULL) - m_routingPollState.getLastCompleted()) > g_routingTableUpdateInterval))
   {
      success = m_routingPollState.schedule();
   }
   unlockProperties();
   return success;
}

inline bool Node::lockForTopologyPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
	    (!(m_flags & NF_DISABLE_TOPOLOGY_POLL)) &&
       (m_runtimeFlags & ODF_CONFIGURATION_POLL_PASSED) &&
       (static_cast<UINT32>(time(NULL) - m_topologyPollState.getLastCompleted()) > g_topologyPollingInterval))
   {
      success = m_topologyPollState.schedule();
   }
   unlockProperties();
   return success;
}

inline bool Node::lockForIcmpPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated &&
       (m_status != STATUS_UNMANAGED) &&
       isIcmpStatCollectionEnabled() &&
       (!(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING)) &&
       ((UINT32)(time(NULL) - m_icmpPollState.getLastCompleted()) > g_icmpPollingInterval))
   {
      success = m_icmpPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Subnet
 */
class NXCORE_EXPORTABLE Subnet : public NetObj
{
	friend void Node::buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth,
	         UINT32 seedObject, const TCHAR *linkName, bool vpnLink, bool includeEndNodes);

private:
   typedef NetObj super;

protected:
   InetAddress m_ipAddress;
   UINT32 m_zoneUIN;
	bool m_bSyntheticMask;

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;

   void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes);

public:
   Subnet();
   Subnet(const InetAddress& addr, UINT32 zoneUIN, bool bSyntheticMask);
   virtual ~Subnet();

   virtual int getObjectClass() const override { return OBJECT_SUBNET; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual uint32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   void addNode(Node *node) { addChild(node); node->addParent(this); calculateCompoundStatus(TRUE); }
   Node *getOtherNode(UINT32 nodeId);

   virtual bool showThresholdSummary() override;

   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
	bool isSyntheticMask() const { return m_bSyntheticMask; }
	bool isPointToPoint() const;

	void setCorrectMask(const InetAddress& addr);

	MacAddress findMacAddress(const InetAddress& ipAddr);

   UINT32 *buildAddressMap(int *length);
};

/**
 * Universal root object
 */
class NXCORE_EXPORTABLE UniversalRoot : public NetObj
{
   using NetObj::loadFromDatabase;

private:
   typedef NetObj super;

public:
   UniversalRoot();
   virtual ~UniversalRoot();

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   void loadFromDatabase(DB_HANDLE hdb);
   virtual void linkObjects() override;
   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }
};

/**
 * Service root
 */
class NXCORE_EXPORTABLE ServiceRoot : public UniversalRoot
{
private:
   typedef UniversalRoot super;

public:
   ServiceRoot();
   virtual ~ServiceRoot();

   virtual int getObjectClass() const override { return OBJECT_SERVICEROOT; }

   virtual bool showThresholdSummary() override;
};

/**
 * Template root
 */
class NXCORE_EXPORTABLE TemplateRoot : public UniversalRoot
{
private:
   typedef UniversalRoot super;

public:
   TemplateRoot();
   virtual ~TemplateRoot();

   virtual int getObjectClass() const override { return OBJECT_TEMPLATEROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;
};

/**
 * Generic container object
 */
class NXCORE_EXPORTABLE AbstractContainer : public NetObj
{
private:
   typedef NetObj super;

   UINT32 *m_pdwChildIdList;
   UINT32 m_dwChildIdListSize;

protected:
   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

public:
   AbstractContainer();
   AbstractContainer(const TCHAR *pszName, UINT32 dwCategory);
   virtual ~AbstractContainer();

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual json_t *toJson() override;

   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }
};

/**
 * Object container
 */
class NXCORE_EXPORTABLE Container : public AbstractContainer, public AutoBindTarget
{
protected:
   typedef AbstractContainer super;

   UINT32 modifyFromMessageInternal(NXCPMessage *request);
   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);

public:
   Container() : super(), AutoBindTarget(this) {}
   Container(const TCHAR *pszName, UINT32 dwCategory) : super(pszName, dwCategory), AutoBindTarget(this) {}
   virtual ~Container() {}

   virtual int getObjectClass() const { return OBJECT_CONTAINER; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual bool showThresholdSummary();

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm);

};

/**
 * Template group object
 */
class NXCORE_EXPORTABLE TemplateGroup : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

public:
   TemplateGroup() : AbstractContainer() { }
   TemplateGroup(const TCHAR *pszName) : super(pszName, 0) { m_status = STATUS_NORMAL; }
   virtual ~TemplateGroup() { }

   virtual int getObjectClass() const override { return OBJECT_TEMPLATEGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual bool showThresholdSummary() override;
};

enum RackElementType
{
   PATCH_PANEL = 0,
   FILLER_PANEL = 1,
   ORGANISER = 2
};

class NXCORE_EXPORTABLE RackPassiveElement
{
private:
   UINT32 m_id;
   TCHAR *m_name;
   RackElementType m_type;
   UINT32 m_position;
   RackOrientation m_orientation;
   UINT32 m_portCount;

public:
   RackPassiveElement(DB_RESULT hResult, int row);
   RackPassiveElement();
   RackPassiveElement(NXCPMessage *pRequest, UINT32 base);
   ~RackPassiveElement() { MemFree(m_name); }

   json_t *toJson();
   bool saveToDatabase(DB_HANDLE hdb, UINT32 parentId);
   bool deleteChildren(DB_HANDLE hdb, UINT32 parentId);
   void fillMessage(NXCPMessage *pMsg, UINT32 base);

   RackElementType getType() const { return m_type; }
   UINT32 getId() const { return m_id; }
};

/**
 * Rack object
 */
class NXCORE_EXPORTABLE Rack : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

protected:
	int m_height;	// Rack height in units
	bool m_topBottomNumbering;
	ObjectArray<RackPassiveElement> *m_passiveElements;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void prepareForDeletion() override;

public:
   Rack();
   Rack(const TCHAR *name, int height);
   virtual ~Rack();

   virtual int getObjectClass() const override { return OBJECT_RACK; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual json_t *toJson() override;
};

/**
 * Zone proxy information
 */
struct ZoneProxy
{
   UINT32 nodeId;
   bool isAvailable;    // True if proxy is available
   UINT32 assignments;  // Number of objects where this proxy is assigned
   INT64 rawDataSenderLoad;
   double cpuLoad;
   double dataCollectorLoad;
   double dataSenderLoad;
   double dataSenderLoadTrend;
   time_t loadBalanceTimestamp;

   ZoneProxy(UINT32 _nodeId)
   {
      nodeId = _nodeId;
      isAvailable = false;
      assignments = 0;
      rawDataSenderLoad = 0;
      cpuLoad = 0;
      dataCollectorLoad = 0;
      dataSenderLoad = 0;
      dataSenderLoadTrend = 0;
      loadBalanceTimestamp = NEVER;
   }

   int compareLoad(const ZoneProxy *p)
   {
      if (dataSenderLoadTrend == 0)
         return (p->dataSenderLoadTrend != 0) ? -1 : 0;

      if (p->dataSenderLoadTrend == 0)
         return (dataSenderLoadTrend != 0) ? 1 : 0;

      if ((dataSenderLoadTrend < 0) && (p->dataSenderLoadTrend > 0))
         return -1;

      if ((dataSenderLoadTrend > 0) && (p->dataSenderLoadTrend < 0))
         return 1;

      double d = fabs(dataSenderLoadTrend) - fabs(p->dataSenderLoadTrend);
      if (fabs(d) > 0.1)
         return (d < 0) ? -1 : 1;

      d = dataSenderLoad - p->dataSenderLoad;
      if (fabs(d) > 0.1)
         return (d < 0) ? -1 : 1;

      d = dataCollectorLoad - p->dataCollectorLoad;
      if (fabs(d) > 0.1)
         return (d < 0) ? -1 : 1;

      d = cpuLoad - p->cpuLoad;
      if (fabs(d) > 0.1)
         return (d < 0) ? -1 : 1;

      return 0;
   }

   json_t *toJson() const
   {
      json_t *root = json_object();
      json_object_set_new(root, "nodeId", json_integer(nodeId));
      return root;
   }
};

/**
 * Zone object
 */
class NXCORE_EXPORTABLE Zone : public NetObj
{
protected:
   typedef NetObj super;

protected:
   UINT32 m_uin;
   ObjectArray<ZoneProxy> *m_proxyNodes;
   BYTE m_proxyAuthKey[ZONE_PROXY_KEY_LENGTH];
	InetAddressIndex *m_idxNodeByAddr;
	InetAddressIndex *m_idxInterfaceByAddr;
	InetAddressIndex *m_idxSubnetByAddr;
	StringList m_snmpPorts;
	time_t m_lastHealthCheck;
	bool m_lockedForHealthCheck;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   void updateProxyLoadData(Node *node);
   void migrateProxyLoad(ZoneProxy *source, ZoneProxy *target);

public:
   Zone();
   Zone(UINT32 uin, const TCHAR *name);
   virtual ~Zone();

   virtual int getObjectClass() const override { return OBJECT_ZONE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool showThresholdSummary() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual json_t *toJson() override;

   UINT32 getUIN() const { return m_uin; }
   const StringList *getSnmpPortList() const { return &m_snmpPorts; }

   UINT32 getProxyNodeId(NetObj *object, bool backup = false);
	bool isProxyNode(UINT32 nodeId) const;
	UINT32 getProxyNodeAssignments(UINT32 nodeId) const;
   bool isProxyNodeAvailable(UINT32 nodeId) const;
	IntegerArray<UINT32> *getAllProxyNodes() const;
	void fillAgentConfigurationMessage(NXCPMessage *msg) const;

   AgentConnectionEx *acquireConnectionToProxy(bool validate = false);

   void addProxy(Node *node);
   void updateProxyStatus(Node *node, bool activeMode);

   bool lockForHealthCheck();
   void healthCheck(PollerInfo *poller);

   void addSubnet(Subnet *pSubnet) { addChild(pSubnet); pSubnet->addParent(this); }
	void addToIndex(Subnet *subnet) { m_idxSubnetByAddr->put(subnet->getIpAddress(), subnet); }
   void addToIndex(Interface *iface) { m_idxInterfaceByAddr->put(iface->getIpAddressList(), iface); }
   void addToIndex(const InetAddress& addr, Interface *iface) { m_idxInterfaceByAddr->put(addr, iface); }
	void addToIndex(Node *node) { m_idxNodeByAddr->put(node->getIpAddress(), node); }
   void addToIndex(const InetAddress& addr, Node *node) { m_idxNodeByAddr->put(addr, node); }
	void removeFromIndex(Subnet *subnet) { m_idxSubnetByAddr->remove(subnet->getIpAddress()); }
	void removeFromIndex(Interface *iface);
   void removeFromInterfaceIndex(const InetAddress& addr) { m_idxInterfaceByAddr->remove(addr); }
	void removeFromIndex(Node *node) { m_idxNodeByAddr->remove(node->getIpAddress()); }
   void removeFromNodeIndex(const InetAddress& addr) { m_idxNodeByAddr->remove(addr); }
	void updateInterfaceIndex(const InetAddress& oldIp, const InetAddress& newIp, Interface *iface);
   void updateNodeIndex(const InetAddress& oldIp, const InetAddress& newIp, Node *node);
	Subnet *getSubnetByAddr(const InetAddress& ipAddr) { return (Subnet *)m_idxSubnetByAddr->get(ipAddr); }
	Interface *getInterfaceByAddr(const InetAddress& ipAddr) { return (Interface *)m_idxInterfaceByAddr->get(ipAddr); }
	Node *getNodeByAddr(const InetAddress& ipAddr) { return (Node *)m_idxNodeByAddr->get(ipAddr); }
	Subnet *findSubnet(bool (*comparator)(NetObj *, void *), void *data) { return (Subnet *)m_idxSubnetByAddr->find(comparator, data); }
	Interface *findInterface(bool (*comparator)(NetObj *, void *), void *data) { return (Interface *)m_idxInterfaceByAddr->find(comparator, data); }
	Node *findNode(bool (*comparator)(NetObj *, void *), void *data) { return (Node *)m_idxNodeByAddr->find(comparator, data); }
   void forEachSubnet(void (*callback)(const InetAddress& addr, NetObj *, void *), void *data) { m_idxSubnetByAddr->forEach(callback, data); }
   ObjectArray<NetObj> *getSubnets(bool updateRefCount) { return m_idxSubnetByAddr->getObjects(updateRefCount); }

   void dumpState(ServerConsole *console);
   void dumpInterfaceIndex(ServerConsole *console);
   void dumpNodeIndex(ServerConsole *console);
   void dumpSubnetIndex(ServerConsole *console);
};

inline bool Zone::lockForHealthCheck()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if ((m_status != STATUS_UNMANAGED) &&
          !m_lockedForHealthCheck &&
          ((UINT32)(time(NULL) - m_lastHealthCheck) >= g_statusPollingInterval))
      {
         m_lockedForHealthCheck = true;
         success = true;
      }
   }
   unlockProperties();
   return success;
}

/**
 * Entire network
 */
class NXCORE_EXPORTABLE Network : public NetObj
{
   using NetObj::loadFromDatabase;

protected:
   typedef NetObj super;

public:
   Network();
   virtual ~Network();

   virtual int getObjectClass() const override { return OBJECT_NETWORK; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;

   virtual bool showThresholdSummary() override;

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
   typedef NetObj super;

protected:
   int m_dciCount;
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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   ConditionObject();
   ConditionObject(bool hidden);
   virtual ~ConditionObject();

   virtual int getObjectClass() const override { return OBJECT_CONDITION; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual json_t *toJson() override;

   void lockForPoll();
   void doPoll(PollerInfo *poller);
   void check();

   bool isReadyForPoll()
   {
      return ((m_status != STATUS_UNMANAGED) &&
              (!m_queuedForPolling) && (!m_isDeleted) &&
              ((UINT32)time(NULL) - (UINT32)m_lastPoll > g_conditionPollingInterval));
   }

   int getCacheSizeForDCI(UINT32 itemId, bool noLock);
};

/**
 * Network map root
 */
class NXCORE_EXPORTABLE NetworkMapRoot : public UniversalRoot
{
protected:
   typedef UniversalRoot super;

public:
   NetworkMapRoot();
   virtual ~NetworkMapRoot();

   virtual int getObjectClass() const override { return OBJECT_NETWORKMAPROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;
};

/**
 * Network map group object
 */
class NXCORE_EXPORTABLE NetworkMapGroup : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

public:
   NetworkMapGroup() : super() { }
   NetworkMapGroup(const TCHAR *pszName) : super(pszName, 0) { }
   virtual ~NetworkMapGroup() { }

   virtual int getObjectClass() const override { return OBJECT_NETWORKMAPGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual bool showThresholdSummary() override;
};

/**
 * Network map object
 */
class NXCORE_EXPORTABLE NetworkMap : public NetObj
{
protected:
   typedef NetObj super;

protected:
	int m_mapType;
	IntegerArray<UINT32> *m_seedObjects;
	int m_discoveryRadius;
	int m_layout;
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

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

	void updateObjects(NetworkMapObjectList *objects);
	UINT32 objectIdFromElementId(UINT32 eid);
	UINT32 elementIdFromObjectId(UINT32 eid);

   void setFilter(const TCHAR *filter);

public:
   NetworkMap();
	NetworkMap(int type, IntegerArray<UINT32> *seeds);
   virtual ~NetworkMap();

   virtual int getObjectClass() const override { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

	virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual void onObjectDelete(UINT32 objectId) override;

   virtual json_t *toJson() override;

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
protected:
   typedef UniversalRoot super;

public:
   DashboardRoot();
   virtual ~DashboardRoot();

   virtual int getObjectClass() const override { return OBJECT_DASHBOARDROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;
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
	~DashboardElement() { MemFree(m_data); MemFree(m_layout); }

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
class NXCORE_EXPORTABLE Dashboard : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

protected:
	int m_numColumns;
	UINT32 m_options;
	ObjectArray<DashboardElement> *m_elements;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   Dashboard();
   Dashboard(const TCHAR *name);
   virtual ~Dashboard();

   virtual int getObjectClass() const override { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual json_t *toJson() override;

   virtual bool showThresholdSummary() override;
};

/**
 * Dashboard group object
 */
class NXCORE_EXPORTABLE DashboardGroup : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

public:
   DashboardGroup() : super() { }
   DashboardGroup(const TCHAR *pszName) : super(pszName, 0) { }
   virtual ~DashboardGroup() { }

   virtual int getObjectClass() const override { return OBJECT_DASHBOARDGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual bool showThresholdSummary() override;
};

/**
 * SLM check object
 */
class NXCORE_EXPORTABLE SlmCheck : public NetObj
{
protected:
   typedef NetObj super;

protected:
	Threshold *m_threshold;
	enum CheckType { check_undefined = 0, check_script = 1, check_threshold = 2 } m_type;
	TCHAR *m_script;
	NXSL_VM *m_pCompiledScript;
	TCHAR m_reason[256];
	bool m_isTemplate;
	UINT32 m_templateId;
	UINT32 m_currentTicketId;

   virtual void onObjectDelete(UINT32 objectId) override;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   void setScript(const TCHAR *script);
	UINT32 getOwnerId();
	NXSL_Value *getNodeObjectForNXSL(NXSL_VM *vm);
	bool insertTicket();
	void closeTicket();
	void setReason(const TCHAR *reason) { nx_strncpy(m_reason, CHECK_NULL_EX(reason), 256); }
	void compileScript();

public:
	SlmCheck();
	SlmCheck(const TCHAR *name, bool isTemplate);
	SlmCheck(SlmCheck *tmpl);
	virtual ~SlmCheck();

   virtual int getObjectClass() const override { return OBJECT_SLMCHECK; }

	virtual bool saveToDatabase(DB_HANDLE hdb) override;
	virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

	virtual void postModify() override;

	void execute();
	void updateFromTemplate(SlmCheck *tmpl);

	bool isTemplate() { return m_isTemplate; }
	UINT32 getTemplateId() { return m_templateId; }
	const TCHAR *getReason() { return m_reason; }
};

/**
 * Service container - common logic for BusinessService, NodeLink and BusinessServiceRoot
 */
class NXCORE_EXPORTABLE ServiceContainer : public AbstractContainer
{
	enum Period { DAY, WEEK, MONTH };

protected:
   typedef AbstractContainer super;

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

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

	void initServiceContainer();
	BOOL addHistoryRecord();
	double getUptimeFromDBFor(Period period, INT32 *downtime);

public:
	ServiceContainer();
	ServiceContainer(const TCHAR *pszName);

	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
	virtual bool saveToDatabase(DB_HANDLE hdb) override;
	virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

	virtual bool showThresholdSummary() override;

	void setStatus(int newStatus);

	void initUptimeStats();
	void updateUptimeStats(time_t currentTime = 0, BOOL updateChilds = FALSE);
};

/**
 * Business service root
 */
class NXCORE_EXPORTABLE BusinessServiceRoot : public ServiceContainer
{
   using ServiceContainer::loadFromDatabase;

protected:
   typedef ServiceContainer super;

public:
	BusinessServiceRoot();
	virtual ~BusinessServiceRoot();

	virtual int getObjectClass() const override { return OBJECT_BUSINESSSERVICEROOT; }

	virtual bool saveToDatabase(DB_HANDLE hdb) override;
   void loadFromDatabase(DB_HANDLE hdb);

   virtual void linkObjects() override;

   void linkObject(NetObj *pObject) { addChild(pObject); pObject->addParent(this); }
};

/**
 * Business service object
 */
class NXCORE_EXPORTABLE BusinessService : public ServiceContainer
{
protected:
   typedef ServiceContainer super;

protected:
	bool m_busy;
   bool m_pollingDisabled;
	time_t m_lastPollTime;
	int m_lastPollStatus;

   virtual void prepareForDeletion() override;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
	BusinessService();
	BusinessService(const TCHAR *name);
	virtual ~BusinessService();

	virtual int getObjectClass() const override { return OBJECT_BUSINESSSERVICE; }

	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
	virtual bool saveToDatabase(DB_HANDLE hdb) override;
	virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

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
   typedef ServiceContainer super;

protected:
	UINT32 m_nodeId;

   virtual void onObjectDelete(UINT32 dwObjectId) override;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
	virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

	void applyTemplate(SlmCheck *tmpl);

public:
	NodeLink();
	NodeLink(const TCHAR *name, UINT32 nodeId);
	virtual ~NodeLink();

	virtual int getObjectClass() const override { return OBJECT_NODELINK; }

	virtual bool saveToDatabase(DB_HANDLE hdb) override;
	virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
	virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

	void execute();
	void applyTemplates();

	UINT32 getNodeId() { return m_nodeId; }
};

/**
 * Node dependency types
 */
#define NODE_DEP_AGENT_PROXY  0x01
#define NODE_DEP_SNMP_PROXY   0x02
#define NODE_DEP_ICMP_PROXY   0x04
#define NODE_DEP_DC_SOURCE    0x08

/**
 * Dependent node information
 */
struct DependentNode
{
   UINT32 nodeId;
   UINT32 dependencyType;
};

/**
 * Object query result
 */
struct ObjectQueryResult
{
   NetObj *object;
   StringList *values;

   ObjectQueryResult(NetObj *_object, StringList *_values)
   {
      object = _object;
      object->incRefCount();
      values = _values;
   }

   ~ObjectQueryResult()
   {
      object->decRefCount();
      delete values;
   }
};

#define MAX_USER_AGENT_MESSAGE_SIZE 1024

/**
 * User agent message list
 */
class UserAgentNotificationItem
{
private:
   UINT32 m_id;
   TCHAR m_message[MAX_USER_AGENT_MESSAGE_SIZE];
   IntegerArray<UINT32> m_objects;
   time_t m_startTime;
   time_t m_endTime;
   bool m_recall;
   bool m_onStartup;
   VolatileCounter m_refCount;

public:
   UserAgentNotificationItem(DB_RESULT result, int row);
   UserAgentNotificationItem(const TCHAR *message, const IntegerArray<UINT32> *objects, time_t startTime, time_t endTime, bool startup);
   ~UserAgentNotificationItem() { }

   UINT32 getId() { return m_id; }
   time_t getEndTime() { return m_endTime; }
   time_t getStartTime() { return m_startTime; }
   bool isRecalled() { return m_recall; }

   void recall() { m_recall = true; processUpdate(); };
   void processUpdate();
   bool isApplicable(UINT32 nodeId);
   void fillMessage(UINT32 base, NXCPMessage *msg, bool fullInfo = true);
   void saveToDatabase();
   void incRefCount() { InterlockedIncrement(&m_refCount); }
   void decRefCount() { InterlockedDecrement(&m_refCount); }
   bool hasNoRef() { return m_refCount == 0; }

   json_t *toJson() const;
};

/**
 * Functions
 */
void ObjectsInit();

void NXCORE_EXPORTABLE NetObjInsert(NetObj *object, bool newObject, bool importedObject);
void NetObjDeleteFromIndexes(NetObj *object);
void NetObjDelete(NetObj *object);

void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Interface *iface);
void UpdateNodeIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, Node *node);

void NXCORE_EXPORTABLE MacDbAddAccessPoint(AccessPoint *ap);
void NXCORE_EXPORTABLE MacDbAddInterface(Interface *iface);
void NXCORE_EXPORTABLE MacDbAddObject(const MacAddress& macAddr, NetObj *object);
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr);
void NXCORE_EXPORTABLE MacDbRemove(const MacAddress& macAddr);
NetObj NXCORE_EXPORTABLE *MacDbFind(const BYTE *macAddr, bool updateRefCount = false);
NetObj NXCORE_EXPORTABLE *MacDbFind(const MacAddress& macAddr, bool updateRefCount = false);

NetObj NXCORE_EXPORTABLE *FindObjectById(UINT32 dwId, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(const uuid& guid, int objClass = -1);
NetObj NXCORE_EXPORTABLE *FindObject(bool (* comparator)(NetObj *, void *), void *userData, int objClass = -1);
ObjectArray<NetObj> NXCORE_EXPORTABLE *FindObjectsByRegex(const TCHAR *regex, int objClass = -1);
const TCHAR NXCORE_EXPORTABLE *GetObjectName(DWORD id, const TCHAR *defaultName);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddress& ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, bool allZones, const InetAddress& ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByIP(UINT32 zoneUIN, const InetAddressList *ipAddrList);
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr);
Node NXCORE_EXPORTABLE *FindNodeByBridgeId(const BYTE *bridgeId);
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId);
Node NXCORE_EXPORTABLE *FindNodeBySysName(const TCHAR *sysName);
ObjectArray<NetObj> *FindNodesByHostname(TCHAR *hostname, UINT32 zoneUIN);
Interface NXCORE_EXPORTABLE *FindInterfaceByIP(UINT32 zoneUIN, const InetAddress& ipAddr, bool updateRefCount = false);
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr, bool updateRefCount = false);
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description, bool updateRefCount = false);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(UINT32 zoneUIN, const InetAddress& ipAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(UINT32 zoneUIN, const InetAddress& nodeAddr);
bool AdjustSubnetBaseAddress(InetAddress& baseAddr, UINT32 zoneUIN);
MobileDevice NXCORE_EXPORTABLE *FindMobileDeviceByDeviceID(const TCHAR *deviceId);
AccessPoint NXCORE_EXPORTABLE *FindAccessPointByMAC(const BYTE *macAddr, bool updateRefCount = false);
UINT32 NXCORE_EXPORTABLE FindLocalMgmtNode();
Zone NXCORE_EXPORTABLE *FindZoneByUIN(UINT32 zoneUIN);
Zone NXCORE_EXPORTABLE *FindZoneByProxyId(UINT32 proxyId);
UINT32 FindUnusedZoneUIN();
bool NXCORE_EXPORTABLE IsClusterIP(UINT32 zoneUIN, const InetAddress& ipAddr);
bool NXCORE_EXPORTABLE IsParentObject(UINT32 object1, UINT32 object2);
ObjectArray<ObjectQueryResult> *QueryObjects(const TCHAR *query, UINT32 userId, TCHAR *errorMessage,
         size_t errorMessageLen, const StringList *fields = NULL, const StringList *orderBy = NULL,
         UINT32 limit = 0);
StructArray<DependentNode> *GetNodeDependencies(UINT32 nodeId);

BOOL LoadObjects();
void DestroyAllObjects();
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter);

bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(UINT32 userId, int objClass);

void DeleteUserFromAllObjects(UINT32 dwUserId);

bool IsValidParentClass(int childClass, int parentClass);
bool IsEventSource(int objectClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);

PollerInfo *RegisterPoller(PollerType type, NetObj *object, bool objectCreation = false);
void ShowPollers(CONSOLE_CTX console);

void InitUserAgentNotifications();
void DeleteExpiredUserAgentNotifications(DB_HANDLE hdb,UINT32 retentionTime);
void FillUserAgentNotificationsAll(NXCPMessage *msg, Node *node);
UserAgentNotificationItem *CreateNewUserAgentNotification(const TCHAR *message, const IntegerArray<UINT32> *objects, time_t startTime, time_t endTime, bool onStartup);

void DeleteObjectFromPhysicalLinks(UINT32 id);
void DeletePatchPanelFromPhysicalLinks(UINT32 rackId, UINT32 patchPanelId);

/**
 * Global variables
 */
extern Network NXCORE_EXPORTABLE *g_pEntireNet;
extern ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot;
extern TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot;
extern NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot;
extern DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot;
extern BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot;

extern UINT32 NXCORE_EXPORTABLE g_dwMgmtNode;
extern BOOL g_bModificationsLocked;
extern Queue g_templateUpdateQueue;

extern ObjectIndex NXCORE_EXPORTABLE g_idxObjectById;
extern HashIndex<uuid> g_idxObjectByGUID;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxSubnetByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxInterfaceByAddr;
extern InetAddressIndex NXCORE_EXPORTABLE g_idxNodeByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxZoneByUIN;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNetMapById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxChassisById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxClusterById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxMobileDeviceById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxAccessPointById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxConditionById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxServiceCheckById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxSensorById;

//User agent messages
extern Mutex g_userAgentNotificationListMutex;
extern ObjectArray<UserAgentNotificationItem> g_userAgentNotificationList;

#endif   /* _nms_objects_h_ */
