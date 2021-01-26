/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
extern uint32_t g_discoveryPollingInterval;
extern uint32_t g_statusPollingInterval;
extern uint32_t g_configurationPollingInterval;
extern uint32_t g_routingTableUpdateInterval;
extern uint32_t g_topologyPollingInterval;
extern uint32_t g_conditionPollingInterval;
extern uint32_t g_instancePollingInterval;
extern uint32_t g_icmpPollingInterval;
extern int16_t g_defaultAgentCacheMode;

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
#define ALL_ZONES ((int32_t)-1)

class AgentTunnel;
class GenericAgentPolicy;

#ifdef _WIN32
template class NXCORE_EXPORTABLE ObjectArray<GeoLocation>;
#endif

/**
 * Geo area
 */
class NXCORE_EXPORTABLE GeoArea
{
private:
   uint32_t m_id;
   TCHAR m_name[128];
   TCHAR *m_comments;
   ObjectArray<GeoLocation> m_border;

public:
   GeoArea(const NXCPMessage& msg);
   GeoArea(DB_RESULT hResult, int row);
   ~GeoArea();

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   bool containsLocation(const GeoLocation& location) const { return location.isWithinArea(m_border); }

   uint32_t getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getComments() const { return m_comments; }
   StringBuffer getBorderAsString() const;
};

/**
 * Extended agent connection
 */
class NXCORE_EXPORTABLE AgentConnectionEx : public AgentConnection
{
protected:
   uint32_t m_nodeId;
   AgentTunnel *m_tunnel;
   AgentTunnel *m_proxyTunnel;
   ClientSession *m_tcpProxySession;

   virtual AbstractCommChannel *createChannel() override;
   virtual void onTrap(NXCPMessage *msg) override;
   virtual void onSyslogMessage(const NXCPMessage& msg) override;
   virtual void onWindowsEvent(const NXCPMessage& msg) override;
   virtual void onDataPush(NXCPMessage *msg) override;
   virtual void onFileMonitoringData(NXCPMessage *msg) override;
   virtual void onSnmpTrap(NXCPMessage *pMsg) override;
   virtual void onDisconnect() override;
   virtual UINT32 processCollectedData(NXCPMessage *msg) override;
   virtual UINT32 processBulkCollectedData(NXCPMessage *request, NXCPMessage *response) override;
   virtual bool processCustomMessage(NXCPMessage *msg) override;
   virtual void processTcpProxyData(uint32_t channelId, const void *data, size_t size) override;
   virtual void getSshKeys(NXCPMessage *msg, NXCPMessage *response) override;

   AgentConnectionEx(uint32_t nodeId, const InetAddress& ipAddr, uint16_t port, const TCHAR *secret, bool allowCompression);
   AgentConnectionEx(uint32_t nodeId, AgentTunnel *tunnel, const TCHAR *secret, bool allowCompression);

public:
   static shared_ptr<AgentConnectionEx> create(uint32_t nodeId, const InetAddress& ipAddr, uint16_t port = AGENT_LISTEN_PORT, const TCHAR *secret = nullptr, bool allowCompression = true)
   {
      auto object = new AgentConnectionEx(nodeId, ipAddr, port, secret, allowCompression);
      auto p = shared_ptr<AgentConnectionEx>(object);
      object->setSelfPtr(p);
      return p;
   }
   static shared_ptr<AgentConnectionEx> create(uint32_t nodeId, AgentTunnel *tunnel, const TCHAR *secret = nullptr, bool allowCompression = true)
   {
      auto object = new AgentConnectionEx(nodeId, tunnel, secret, allowCompression);
      auto p = shared_ptr<AgentConnectionEx>(object);
      object->setSelfPtr(p);
      return p;
   }

   virtual ~AgentConnectionEx();

   shared_ptr<AgentConnectionEx> self() const { return static_pointer_cast<AgentConnectionEx>(AgentConnection::self()); }

   uint32_t deployPolicy(NXCPMessage *msg);
   uint32_t uninstallPolicy(uuid guid, const TCHAR *type, bool newTypeFormatSupported);

   void setTunnel(AgentTunnel *tunnel);

   using AgentConnection::setProxy;
   void setProxy(AgentTunnel *tunnel, const TCHAR *secret);

   void setTcpProxySession(ClientSession *session);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<AgentConnectionEx>;
#endif

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
   friend PollerInfo *RegisterPoller(PollerType, const shared_ptr<NetObj>&, bool);

private:
   PollerType m_type;
   shared_ptr<NetObj> m_object;
   bool m_objectCreation;
   TCHAR m_status[128];

   PollerInfo(PollerType type, const shared_ptr<NetObj>& object, bool objectCreation) : m_object(object)
   {
      m_type = type;
      m_objectCreation = objectCreation;
      _tcscpy(m_status, _T("awaiting execution"));
   }

public:
   ~PollerInfo();

   PollerType getType() const { return m_type; }
   NetObj *getObject() const { return m_object.get(); }
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
 * Queued template update task
 */
struct TemplateUpdateTask
{
   shared_ptr<DataCollectionOwner> source;
   uint32_t targetId;
   int updateType;
   bool removeDCI;

   TemplateUpdateTask(const shared_ptr<DataCollectionOwner>& _source, uint32_t _targetId, int _updateType, bool _removeDCI) : source(_source)
   {
      targetId = _targetId;
      updateType = _updateType;
      removeDCI = _removeDCI;
   }
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
      if (object != nullptr)
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
      m_owner = (owner == Ownership::True);
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

   bool put(UINT64 key, const shared_ptr<T>& object)
   {
      return AbstractIndexBase::put(key, new shared_ptr<T>(object));
   }

   shared_ptr<T> get(UINT64 key)
   {
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::get(key));
      return (v != nullptr) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   shared_ptr<T> find(bool (*comparator)(T *, void *), void *context)
   {
      std::pair<bool (*)(T*, void*), void*> wrapperData(comparator, context);
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::find(reinterpret_cast<bool (*)(void*, void*)>(comparatorWrapper), &wrapperData));
      return (v != nullptr) ? shared_ptr<T>(*v) : shared_ptr<T>();
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
class NXCORE_EXPORTABLE ObjectIndex : public SharedPointerIndex<NetObj>
{
   DISABLE_COPY_CTOR(ObjectIndex)

public:
   ObjectIndex() : SharedPointerIndex<NetObj>() { }

   SharedObjectArray<NetObj> *getObjects(bool (*filter)(NetObj *, void *) = nullptr, void *context = nullptr);

   template<typename C>
   SharedObjectArray<NetObj> *getObjects(bool (*filter)(NetObj *, C *), C *context)
   {
      return getObjects(reinterpret_cast<bool (*)(NetObj*, void*)>(filter), context);
   }

   void getObjects(SharedObjectArray<NetObj> *destination, bool (*filter)(NetObj *, void *) = nullptr, void *context = nullptr);

   template<typename C>
   void getObjects(SharedObjectArray<NetObj> *destination, bool (*filter)(NetObj *, C *), C *context)
   {
      getObjects(destination, reinterpret_cast<bool (*)(NetObj*, void*)>(filter), context);
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

	bool put(const InetAddress& addr, const shared_ptr<NetObj>& object);
	bool put(const InetAddressList *addrList, const shared_ptr<NetObj>& object);
	void remove(const InetAddress& addr);
	void remove(const InetAddressList *addrList);
	shared_ptr<NetObj> get(const InetAddress& addr) const;
	shared_ptr<NetObj> find(bool (*comparator)(NetObj *, void *), void *context) const;

	int size() const;
	SharedObjectArray<NetObj> *getObjects(bool (*filter)(NetObj *, void *) = nullptr, void *context = nullptr) const;

	void forEach(void (*callback)(const InetAddress&, NetObj *, void *), void *context) const;
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

   HashIndexHead *acquireIndex() const;
   void swapAndWait();

public:
   HashIndexBase(size_t keyLength);
   ~HashIndexBase();

   bool put(const void *key, const shared_ptr<NetObj>& object);
   void remove(const void *key);

   int size() const;
   shared_ptr<NetObj> get(const void *key) const;
   shared_ptr<NetObj> find(bool (*comparator)(NetObj *, void *), void *context) const;
   void forEach(void (*callback)(const void *, NetObj *, void *), void *context) const;
};

/**
 * Hash index template
 */
template<typename K> class HashIndex : public HashIndexBase
{
public:
   HashIndex() : HashIndexBase(sizeof(K)) { }

   bool put(const K *key, const shared_ptr<NetObj>& object) { return HashIndexBase::put(key, object); }
   bool put(const K& key, const shared_ptr<NetObj>& object) { return HashIndexBase::put(&key, object); }
   void remove(const K *key) { HashIndexBase::remove(key); }
   void remove(const K& key) { HashIndexBase::remove(&key); }

   shared_ptr<NetObj> get(const K *key) const { return HashIndexBase::get(key); }
   shared_ptr<NetObj> get(const K& key) const { return HashIndexBase::get(&key); }
   void forEach(void (*callback)(const K *, NetObj *, void *), void *context) const { HashIndexBase::forEach(reinterpret_cast<void (*)(const void *, NetObj *, void *)>(callback), context); }
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

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<Cluster>;
#endif

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
   uint16_t sshPort;
   shared_ptr<Cluster> cluster;
   int32_t zoneUIN;
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
#define MODIFY_DC_TARGET            0x020000
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

   bool filter(const shared_ptr<DataCollectionTarget>& node);
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
   uint32_t m_id;
   TCHAR *m_url;
   TCHAR *m_description;

public:
   ObjectUrl(NXCPMessage *msg, uint32_t baseId);
   ObjectUrl(DB_RESULT hResult, int row);
   ~ObjectUrl();

   void fillMessage(NXCPMessage *msg, uint32_t baseId);

   uint32_t getId() const { return m_id; }
   const TCHAR *getUrl() const { return m_url; }
   const TCHAR *getDescription() const { return m_description; }

   json_t *toJson() const;
};

/**
 * Object category
 */
class ObjectCategory
{
private:
   uint32_t m_id;
   TCHAR m_name[MAX_OBJECT_NAME];
   uuid m_mapImage;
   uuid m_icon;

public:
   ObjectCategory(const NXCPMessage& msg);
   ObjectCategory(DB_RESULT hResult, int row);

   uint32_t getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const uuid& getMapImage() const { return m_mapImage; }
   const uuid& getIcon() const { return m_icon; }

   void fillMessage(NXCPMessage *msg, uint32_t baseId);
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

	static void onObjectDeleteCallback(NetObj *object, NetObj *context);

	void getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly) const;

	bool saveTrustedNodes(DB_HANDLE hdb);
   bool saveACLToDB(DB_HANDLE hdb);
   bool saveModuleData(DB_HANDLE hdb);

protected:
   time_t m_timestamp;           // Last change time stamp
   SharedString m_alias;         // Object's alias
   SharedString m_comments;      // User comments
   SharedString m_nameOnMap;     // Object's name on network map
   int m_status;
   int m_savedStatus;            // Object status in database
   int m_statusCalcAlg;          // Status calculation algorithm
   int m_statusPropAlg;          // Status propagation algorithm
   int m_fixedStatus;            // Status if propagation is "Fixed"
   int m_statusShift;            // Shift value for "shifted" status propagation
   int m_statusTranslation[4];
   int m_statusSingleThreshold;
   int m_statusThresholds[4];
   uint32_t m_flags;
   uint32_t m_runtimeFlags;
   uint32_t m_state;
   uint32_t m_stateBeforeMaintenance;
   uint64_t m_maintenanceEventId;
   uint32_t m_maintenanceInitiator;
   VolatileCounter m_modified;
   bool m_isDeleted;
   bool m_isDeleteInitiated;
   bool m_isHidden;
	bool m_isSystem;
	uuid m_mapImage;
   uint32_t m_submapId;          // Map object which should be open on drill-down request
   uint32_t m_categoryId;
   MUTEX m_mutexProperties;         // Object data access mutex
	GeoLocation m_geoLocation;
   PostalAddress *m_postalAddress;
   ClientSession *m_pollRequestor;
	IntegerArray<UINT32> *m_dashboards; // Dashboards associated with this object
	ObjectArray<ObjectUrl> *m_urls;  // URLs associated with this object
	uint32_t m_primaryZoneProxyId;     // ID of assigned primary zone proxy node
	uint32_t m_backupZoneProxyId;      // ID of assigned backup zone proxy node

   AccessList *m_accessList;
   bool m_inheritAccessRights;
   MUTEX m_mutexACL;

   IntegerArray<uint32_t> *m_trustedNodes;

   StringObjectMap<ModuleData> *m_moduleData;
   MUTEX m_moduleDataLock;

   IntegerArray<UINT32> *m_responsibleUsers;
   RWLOCK m_rwlockResponsibleUsers;

   const SharedObjectArray<NetObj> &getChildList() const { return reinterpret_cast<const SharedObjectArray<NetObj>&>(super::getChildList()); }
   const SharedObjectArray<NetObj> &getParentList() const { return reinterpret_cast<const SharedObjectArray<NetObj>&>(super::getParentList()); }

   void lockProperties() const { MutexLock(m_mutexProperties); }
   void unlockProperties() const { MutexUnlock(m_mutexProperties); }
   void lockACL() const { MutexLock(m_mutexACL); }
   void unlockACL() const { MutexUnlock(m_mutexACL); }
   void lockResponsibleUsersList(bool writeLock)
   {
      if (writeLock)
         RWLockWriteLock(m_rwlockResponsibleUsers);
      else
         RWLockReadLock(m_rwlockResponsibleUsers);
   }
   void unlockResponsibleUsersList() { RWLockUnlock(m_rwlockResponsibleUsers); }

   void setModified(uint32_t flags, bool notify = true);                  // Used to mark object as modified

   bool loadACLFromDB(DB_HANDLE hdb);
   bool loadCommonProperties(DB_HANDLE hdb);
   bool loadTrustedNodes(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query) { return ExecuteQueryOnObject(hdb, m_id, query); }

   virtual void prepareForDeletion();
   virtual void onObjectDelete(UINT32 objectId);

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId);
   virtual void fillMessageInternalStage2(NXCPMessage *msg, UINT32 userId);
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *msg);
   virtual UINT32 modifyFromMessageInternalStage2(NXCPMessage *msg);

   bool isGeoLocationHistoryTableExists(DB_HANDLE hdb) const;
   bool createGeoLocationHistoryTable(DB_HANDLE hdb);

   void getAllResponsibleUsersInternal(IntegerArray<UINT32> *list);

public:
   NetObj();
   virtual ~NetObj();

   shared_ptr<NetObj> self() const { return static_pointer_cast<NetObj>(NObject::self()); }

   virtual int getObjectClass() const { return OBJECT_GENERIC; }
   virtual const WCHAR *getObjectClassNameW() const;
   virtual const char *getObjectClassNameA() const;
#ifdef UNICODE
   const WCHAR *getObjectClassName() const { return getObjectClassNameW(); }
#else
   const char *getObjectClassName() const { return getObjectClassNameA(); }
#endif

   virtual int32_t getZoneUIN() const { return 0; }

   virtual InetAddress getPrimaryIpAddress() const { return InetAddress::INVALID; }

   int getStatus() const { return m_status; }
   uint32_t getState() const { return m_state; }
   uint32_t getRuntimeFlags() const { return m_runtimeFlags; }
   uint32_t getFlags() const { return m_flags; }
   int getPropagatedStatus();
   time_t getTimeStamp() const { return m_timestamp; }
   SharedString getAlias() const { return GetAttributeWithLock(m_alias, m_mutexProperties); }
	SharedString getComments() const { return GetAttributeWithLock(m_comments, m_mutexProperties); }
   SharedString getNameOnMap() const { return GetAttributeWithLock(m_nameOnMap, m_mutexProperties); }

   uint32_t getCategoryId() const { return m_categoryId; }
   void setCategoryId(uint32_t id) { m_categoryId = id; setModified(MODIFY_COMMON_PROPERTIES); }

	const GeoLocation& getGeoLocation() const { return m_geoLocation; }
	void setGeoLocation(const GeoLocation& geoLocation);

   const PostalAddress *getPostalAddress() const { return m_postalAddress; }
   void setPostalAddress(PostalAddress * addr) { lockProperties(); delete m_postalAddress; m_postalAddress = addr; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   const uuid& getMapImage() const { return m_mapImage; }
   void setMapImage(const uuid& image) { lockProperties(); m_mapImage = image; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

   bool isModified() const { return m_modified != 0; }
   bool isModified(uint32_t bit) const { return (m_modified & bit) != 0; }
   bool isDeleted() const { return m_isDeleted; }
   bool isDeleteInitiated() const { return m_isDeleteInitiated; }
   bool isOrphaned() const { return getParentCount() == 0; }
   bool isEmpty() const { return getChildCount() == 0; }

	bool isSystem() const { return m_isSystem; }
	void setSystemFlag(bool flag) { m_isSystem = flag; }
	void setFlag(uint32_t flag) { lockProperties(); m_flags |= flag; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
	void clearFlag(uint32_t flag) { lockProperties(); m_flags &= ~flag; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

	bool isTrustedNode(uint32_t id) const;

   virtual void onCustomAttributeChange() override;

   void addChild(const shared_ptr<NetObj>& object);     // Add reference to child object
   void addParent(const shared_ptr<NetObj>& object);    // Add reference to parent object

   void deleteChild(const NetObj& object);  // Delete reference to child object
   void deleteParent(const NetObj& object); // Delete reference to parent object

   void deleteObject(NetObj *initiator = nullptr);     // Prepare object for deletion
   void destroy();   // Destroy partially loaded object

   void updateObjectIndexes();

   bool isHidden() const { return m_isHidden; }
   void hide();
   void unhide();
   void markAsModified(uint32_t flags) { setModified(flags); }  // external API to mark object as modified
   void markAsSaved() { m_modified = 0; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool saveRuntimeData(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();

   void setId(uint32_t dwId) { m_id = dwId; setModified(MODIFY_ALL); }
   void generateGuid() { m_guid = uuid::generate(); }
   void setName(const TCHAR *name) { lockProperties(); _tcslcpy(m_name, name, MAX_OBJECT_NAME); setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
   void resetStatus() { lockProperties(); m_status = STATUS_UNKNOWN; setModified(MODIFY_RUNTIME); unlockProperties(); }
   void setAlias(const TCHAR *alias);
   void setComments(const TCHAR *comments);
   void setNameOnMap(const TCHAR *name);
   void setCreationTime() { m_creationTime = time(nullptr); }
   time_t getCreationTime() { return m_creationTime; }

   bool isInMaintenanceMode() const { return m_maintenanceEventId != 0; }
   uint64_t getMaintenanceEventId() const { return m_maintenanceEventId; }
   uint32_t getMaintenanceInitiator() const { return m_maintenanceInitiator; }
   virtual void enterMaintenanceMode(uint32_t userId, const TCHAR *comments);
   virtual void leaveMaintenanceMode(uint32_t userId);

   void fillMessage(NXCPMessage *msg, UINT32 userId);
   UINT32 modifyFromMessage(NXCPMessage *msg);

	virtual void postModify();

   void commentsToMessage(NXCPMessage *pMsg);

   virtual bool setMgmtStatus(bool bIsManaged);
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   uint32_t getUserRights(uint32_t userId) const;
   bool checkAccessRights(uint32_t userId, uint32_t requiredRights) const;
   void dropUserAccess(uint32_t userId);

   void addChildNodesToList(SharedObjectArray<Node> *nodeList, uint32_t userId);
   void addChildDCTargetsToList(SharedObjectArray<DataCollectionTarget> *dctList, uint32_t userId);

   uint32_t getAssignedZoneProxyId(bool backup = false) const { return backup ? m_backupZoneProxyId : m_primaryZoneProxyId; }
   void setAssignedZoneProxyId(uint32_t id, bool backup) { if (backup) m_backupZoneProxyId = id; else m_primaryZoneProxyId = id; }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const;

   void executeHookScript(const TCHAR *hookName);

   ModuleData *getModuleData(const TCHAR *module);
   void setModuleData(const TCHAR *module, ModuleData *data);

	SharedObjectArray<NetObj> *getParents(int typeFilter = -1) const;
	SharedObjectArray<NetObj> *getChildren(int typeFilter = -1) const;
	SharedObjectArray<NetObj> *getAllChildren(bool eventSourceOnly) const;
   int getParentsCount(int typeFilter = -1) const;
   int getChildrenCount(int typeFilter = -1) const;

   shared_ptr<NetObj> findChildObject(const TCHAR *name, int typeFilter) const;
   shared_ptr<Node> findChildNode(const InetAddress& addr) const;

	virtual NXSL_Array *getParentsForNXSL(NXSL_VM *vm);
	virtual NXSL_Array *getChildrenForNXSL(NXSL_VM *vm);

   virtual bool showThresholdSummary() const;
   virtual bool isEventSource() const;
   virtual bool isDataCollectionTarget() const;

   void setStatusCalculation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);
   void setStatusPropagation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);

   void sendPollerMsg(UINT32 dwRqId, const TCHAR *pszFormat, ...);

   StringBuffer expandText(const TCHAR *textTemplate, const Alarm *alarm, const Event *event, const shared_ptr<DCObjectInfo>& dci,
            const TCHAR *userName, const TCHAR *objectName, const StringMap *inputFields, const StringList *args);

   void updateGeoLocationHistory(GeoLocation location);

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
   virtual ~AutoBindTarget();

   AutoBindDecision isApplicable(const shared_ptr<DataCollectionTarget>& object);
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
   uint32_t m_lastResponseTime;
   uint32_t m_minResponseTime;
   uint32_t m_maxResponseTime;
   uint32_t m_avgResponseTime;
   uint32_t m_packetLoss;
   uint16_t *m_rawResponseTimes;
   int m_writePos;
   int m_bufferSize;

   void recalculate();

public:
   IcmpStatCollector(int period);
   ~IcmpStatCollector();

   uint32_t last() const { return m_lastResponseTime; }
   uint32_t average() const { return m_avgResponseTime; }
   uint32_t min() const { return m_minResponseTime; }
   uint32_t max() const { return m_maxResponseTime; }
   uint32_t packetLoss() const { return m_packetLoss; }

   void update(uint32_t responseTime);
   void resize(int period);

   bool saveToDatabase(DB_HANDLE hdb, uint32_t objectId, const TCHAR *target) const;

   static IcmpStatCollector *loadFromDatabase(DB_HANDLE hdb, uint32_t objectId, const TCHAR *target, int period);
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
   bool m_instanceDiscoveryChanges;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];
	RWLOCK m_dciAccessLock;

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionLoad() { }
   virtual void onDataCollectionChange() { }
   virtual void onInstanceDiscoveryChange() { }

   void loadItemsFromDB(DB_HANDLE hdb);
   void destroyItems();
   void updateInstanceDiscoveryItems(DCObject *dci);

   void readLockDciAccess() const { RWLockReadLock(m_dciAccessLock); }
   void writeLockDciAccess() { RWLockWriteLock(m_dciAccessLock); }
	void unlockDciAccess() const { RWLockUnlock(m_dciAccessLock); }

   void deleteChildDCIs(UINT32 dcObjectId);
   void deleteDCObject(DCObject *object);

public:
   DataCollectionOwner();
   DataCollectionOwner(const TCHAR *name, const uuid& guid = uuid::NULL_UUID);
   virtual ~DataCollectionOwner();

   shared_ptr<DataCollectionOwner> self() const { return static_pointer_cast<DataCollectionOwner>(NObject::self()); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual void updateFromImport(ConfigEntry *config);

   virtual json_t *toJson() override;

   int getItemCount() const { return m_dcObjects->size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false, bool notify = true);
   bool updateDCObject(uint32_t dcObjectId, const NXCPMessage& msg, uint32_t *numMaps, uint32_t **mapIndex, uint32_t **mapId, uint32_t userId);
   bool deleteDCObject(uint32_t dcObjectId, bool needLock, uint32_t userId);
   bool setItemStatus(UINT32 dwNumItems, UINT32 *pdwItemList, int iStatus);
   int updateMultipleDCObjects(const NXCPMessage& request);
   shared_ptr<DCObject> getDCObjectById(uint32_t itemId, uint32_t userId, bool lock = true) const;
   shared_ptr<DCObject> getDCObjectByGUID(const uuid& guid, uint32_t userId, bool lock = true) const;
   shared_ptr<DCObject> getDCObjectByTemplateId(uint32_t tmplItemId, uint32_t userId) const;
   shared_ptr<DCObject> getDCObjectByName(const TCHAR *name, uint32_t userId) const;
   shared_ptr<DCObject> getDCObjectByDescription(const TCHAR *description, uint32_t userId) const;
   SharedObjectArray<DCObject> *getDCObjectsByRegex(const TCHAR *regex, bool searchName, UINT32 userId) const;
   NXSL_Value *getAllDCObjectsForNXSL(NXSL_VM *vm, const TCHAR *name, const TCHAR *description, UINT32 userId) const;
   void setDCIModificationFlag() { lockProperties(); m_dciListModified = true; unlockProperties(); }
   void sendItemsToClient(ClientSession *pSession, UINT32 dwRqId) const;
   virtual HashSet<uint32_t> *getRelatedEventsList() const;
   StringSet *getDCIScriptList() const;
   bool isDataCollectionSource(UINT32 nodeId) const;

   virtual void applyDCIChanges(bool forcedChange);
   virtual bool applyToTarget(const shared_ptr<DataCollectionTarget>& target);

   void queueUpdate() const;
   void queueRemoveFromTarget(UINT32 targetId, bool removeDCI) const;

	bool enumDCObjects(bool (*callback)(const shared_ptr<DCObject>&, uint32_t, void *), void *context) const;
	void associateItems();
};

#define EXPAND_MACRO 1

/**
 * Data for agent policy deployment task
 */
struct AgentPolicyDeploymentData
{
   shared_ptr<AgentConnectionEx> conn;
   shared_ptr<NetObj> object;
   bool forceInstall;
   uint32_t currVersion;
   uint8_t currHash[MD5_DIGEST_SIZE];
   bool newTypeFormat;
   TCHAR debugId[256];

   AgentPolicyDeploymentData(const shared_ptr<AgentConnectionEx>& _conn, const shared_ptr<NetObj>& _object, bool _newTypeFormat) : conn(_conn), object(_object)
   {
      forceInstall = false;
      currVersion = 0;
      memset(currHash, 0, MD5_DIGEST_SIZE);
      newTypeFormat = _newTypeFormat;
      debugId[0] = 0;
   }
};

/**
 * Data for agent policy removal task
 */
struct AgentPolicyRemovalData
{
   shared_ptr<AgentConnectionEx> conn;
   uuid guid;
   TCHAR policyType[MAX_POLICY_TYPE_LEN];
   bool newTypeFormat;
   TCHAR debugId[256];

   AgentPolicyRemovalData(const shared_ptr<AgentConnectionEx>& _conn, const uuid& _guid, const TCHAR *_type, bool _newTypeFormat) : conn(_conn), guid(_guid)
   {
      conn = _conn;
      _tcslcpy(policyType, _type, MAX_POLICY_TYPE_LEN);
      newTypeFormat = _newTypeFormat;
      debugId[0] = 0;
   }
};

void RemoveAgentPolicy(const shared_ptr<AgentPolicyRemovalData>& data);

/**
 * Generic agent policy object
 */
class NXCORE_EXPORTABLE GenericAgentPolicy
{
protected:
   uint32_t m_ownerId;
   uuid m_guid;
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR m_type[MAX_POLICY_TYPE_LEN];
   char *m_content;
   uint32_t m_version;
   uint32_t m_flags;
   MUTEX m_contentLock;

   virtual bool createDeploymentMessage(NXCPMessage *msg, char *content, bool newTypeFormatSupported);

public:
   GenericAgentPolicy(const uuid& guid, const TCHAR *type, uint32_t ownerId);
   GenericAgentPolicy(const TCHAR *name, const TCHAR *type, uint32_t ownerId);
   virtual ~GenericAgentPolicy();

   const TCHAR *getName() const { return m_name; }
   const uuid getGuid() const { return m_guid; }
   const uint32_t getVersion() const { return m_version; }
   const TCHAR *getType() const { return m_type; }
   const uint32_t getFlags() const { return m_flags; }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb);

   virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   virtual void fillUpdateMessage(NXCPMessage *msg) const;
   virtual uint32_t modifyFromMessage(const NXCPMessage& request);

   virtual void updateFromImport(const ConfigEntry *config);
   virtual void createExportRecord(StringBuffer &xml, uint32_t recordId);
   virtual void getEventList(HashSet<uint32_t> *eventList) const {}

   virtual json_t *toJson();

   virtual void deploy(shared_ptr<AgentPolicyDeploymentData> data);
};

/**
 * File delivery policy
 */
class NXCORE_EXPORTABLE FileDeliveryPolicy : public GenericAgentPolicy
{
public:
   FileDeliveryPolicy(const uuid& guid, uint32_t ownerId) : GenericAgentPolicy(guid, _T("FileDelivery"), ownerId) { }
   FileDeliveryPolicy(const TCHAR *name, uint32_t ownerId) : GenericAgentPolicy(name, _T("FileDelivery"), ownerId) { }
   virtual ~FileDeliveryPolicy() { }

   virtual uint32_t modifyFromMessage(const NXCPMessage& request) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual void deploy(shared_ptr<AgentPolicyDeploymentData> data) override;
};

/**
 * Log parser policy
 */
class NXCORE_EXPORTABLE LogParserPolicy : public GenericAgentPolicy
{
public:
   LogParserPolicy(const uuid& guid, uint32_t ownerId) : GenericAgentPolicy(guid, _T("LogParserConfig"), ownerId) { }
   LogParserPolicy(const TCHAR *name, uint32_t ownerId) : GenericAgentPolicy(name, _T("LogParserConfig"), ownerId) { }
   virtual ~LogParserPolicy() { }

   virtual void getEventList(HashSet<uint32_t> *eventList) const override;
};

/**
 * Data collection template class
 */
class NXCORE_EXPORTABLE Template : public DataCollectionOwner, public AutoBindTarget, public VersionableObject
{
private:
   typedef DataCollectionOwner super;

protected:
   SharedObjectArray<GenericAgentPolicy> *m_policyList;
   SharedObjectArray<GenericAgentPolicy> *m_deletedPolicyList;

   virtual void prepareForDeletion() override;
   virtual void onDataCollectionChange() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   void forceDeployPolicies(const shared_ptr<Node>& target);

public:
   Template();
   Template(const TCHAR *name, const uuid& guid = uuid::NULL_UUID);
   ~Template();

   shared_ptr<Template> self() const { return static_pointer_cast<Template>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_TEMPLATE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void applyDCIChanges(bool forcedChange) override;
   virtual bool applyToTarget(const shared_ptr<DataCollectionTarget>& target) override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual void updateFromImport(ConfigEntry *config) override;
   virtual json_t *toJson() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   void createExportRecord(StringBuffer &xml);
   virtual HashSet<uint32_t> *getRelatedEventsList() const override;

   bool hasPolicy(const uuid& guid) const;
   void fillPolicyListMessage(NXCPMessage *pMsg) const;
   bool fillPolicyDetailsMessage(NXCPMessage *msg, const uuid& guid) const;
   uuid updatePolicyFromMessage(const NXCPMessage& request);
   bool removePolicy(const uuid& guid);
   void applyPolicyChanges();
   void forceApplyPolicyChanges();
   void checkPolicyDeployment(const shared_ptr<Node>& node, AgentPolicyInfo *ap);
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
   uint32_t m_parentInterfaceId;
   uint32_t m_index;
   MacAddress m_macAddr;
   InetAddressList m_ipAddressList;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
	uint32_t m_type;
	uint32_t m_mtu;
   uint64_t m_speed;
   uint32_t m_bridgePortNumber;		 // 802.1D port number
	InterfacePhysicalLocation m_physicalLocation;
	uint32_t m_peerNodeId;				 // ID of peer node object, or 0 if unknown
	uint32_t m_peerInterfaceId;		 // ID of peer interface object, or 0 if unknown
   LinkLayerProtocol m_peerDiscoveryProtocol;  // Protocol used to discover peer node
	int16_t m_adminState;				 // interface administrative state
	int16_t m_operState;				 // interface operational state
	int16_t m_pendingOperState;
	int16_t m_confirmedOperState;
	int16_t m_dot1xPaeAuthState;		 // 802.1x port auth state
	int16_t m_dot1xBackendAuthState; // 802.1x backend auth state
   uint64_t m_lastDownEventId;
   int32_t m_pendingStatus;
   int32_t m_statusPollCount;
   int32_t m_operStatePollCount;
	int32_t m_requiredPollCount;
	int32_t m_zoneUIN;
	int32_t m_ifTableSuffixLen;
   uint32_t *m_ifTableSuffix;
   IntegerArray<uint32_t> *m_vlans;

   void icmpStatusPoll(UINT32 rqId, UINT32 nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState);
	void paeStatusPoll(uint32_t rqId, SNMP_Transport *transport, Node *node);

protected:
   virtual void onObjectDelete(UINT32 objectId) override;
   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   void setExpectedStateInternal(int state);

public:
   Interface();
   Interface(const InetAddressList& addrList, int32_t zoneUIN, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, UINT32 index, const InetAddressList& addrList, UINT32 ifType, int32_t zoneUIN);
   virtual ~Interface();

   shared_ptr<Interface> self() const { return static_pointer_cast<Interface>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_INTERFACE; }
   virtual InetAddress getPrimaryIpAddress() const override { lockProperties(); auto a = m_ipAddressList.getFirstUnicastAddress(); unlockProperties(); return a; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   shared_ptr<Node> getParentNode() const;
   uint32_t getParentNodeId() const;
   String getParentNodeName() const;
   uint32_t getParentInterfaceId() const { return m_parentInterfaceId; }

   const InetAddressList *getIpAddressList() const { return &m_ipAddressList; }
   InetAddress getFirstIpAddress() const;
   uint32_t getIfIndex() const { return m_index; }
   uint32_t getIfType() const { return m_type; }
   uint32_t getMTU() const { return m_mtu; }
   uint64_t getSpeed() const { return m_speed; }
   uint32_t getBridgePortNumber() const { return m_bridgePortNumber; }
   uint32_t getChassis() const { return m_physicalLocation.chassis; }
   uint32_t getModule() const { return m_physicalLocation.module; }
   uint32_t getPIC() const { return m_physicalLocation.pic; }
   uint32_t getPort() const { return m_physicalLocation.port; }
	InterfacePhysicalLocation getPhysicalLocation() const { return GetAttributeWithLock(m_physicalLocation, m_mutexProperties); }
	uint32_t getPeerNodeId() const { return m_peerNodeId; }
	uint32_t getPeerInterfaceId() const { return m_peerInterfaceId; }
   LinkLayerProtocol getPeerDiscoveryProtocol() const { return m_peerDiscoveryProtocol; }
	int getExpectedState() const { return (int)((m_flags & IF_EXPECTED_STATE_MASK) >> 28); }
	int getAdminState() const { return (int)m_adminState; }
	int getOperState() const { return (int)m_operState; }
   int getConfirmedOperState() const { return (int)m_confirmedOperState; }
	int getDot1xPaeAuthState() const { return (int)m_dot1xPaeAuthState; }
	int getDot1xBackendAuthState() const { return (int)m_dot1xBackendAuthState; }
	const TCHAR *getDescription() const { return m_description; }
   const MacAddress& getMacAddr() const { return m_macAddr; }
   int getIfTableSuffixLen() const { return m_ifTableSuffixLen; }
   const uint32_t *getIfTableSuffix() const { return m_ifTableSuffix; }
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

   uint64_t getLastDownEventId() const { return m_lastDownEventId; }
   void setLastDownEventId(uint64_t id) { m_lastDownEventId = id; }

   void setMacAddr(const MacAddress& macAddr, bool updateMacDB);
   void setIpAddress(const InetAddress& addr);
   void setBridgePortNumber(uint32_t bpn)
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
   void setDescription(const TCHAR *description)
   {
      lockProperties();
      _tcslcpy(m_description, description, MAX_DB_STRING);
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
	void setSpeed(uint64_t speed)
	{
	   lockProperties();
	   m_speed = speed;
	   setModified(MODIFY_INTERFACE_PROPERTIES);
	   unlockProperties();
	}
   void setIfTableSuffix(int len, const uint32_t *suffix)
   {
      lockProperties();
      MemFree(m_ifTableSuffix);
      m_ifTableSuffixLen = len;
      m_ifTableSuffix = (len > 0) ? MemCopyArray(suffix, len) : nullptr;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setParentInterface(uint32_t parentInterfaceId)
   {
      lockProperties();
      m_parentInterfaceId = parentInterfaceId;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void updateVlans(IntegerArray<uint32_t> *vlans);

	void updateZoneUIN();

   void statusPoll(ClientSession *session, uint32_t rqId, ObjectQueue<Event> *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, uint32_t nodeIcmpProxy);

   uint32_t wakeUp();
	void setExpectedState(int state) { lockProperties(); setExpectedStateInternal(state); unlockProperties(); }
   void setExcludeFromTopology(bool excluded);
   void setIncludeInIcmpPoll(bool included);

   void expandName(const TCHAR *originalName, TCHAR *expandedName);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE weak_ptr<Node>;
#endif

/**
 * Network service class
 */
class NXCORE_EXPORTABLE NetworkService : public NetObj
{
private:
   typedef NetObj super;

protected:
   int m_serviceType;   // SSH, POP3, etc.
   weak_ptr<Node> m_hostNode;    // Pointer to node object which hosts this service
   uint32_t m_pollerNode; // ID of node object which is used for polling
                          // If 0, m_pHostNode->m_dwPollerNode will be used
   uint16_t m_proto;        // Protocol (TCP, UDP, etc.)
   uint16_t m_port;         // TCP or UDP port number
   InetAddress m_ipAddress;
   TCHAR *m_request;  // Service-specific request
   TCHAR *m_response; // Service-specific expected response
	int m_pendingStatus;
	uint32_t m_pollCount;
	uint32_t m_requiredPollCount;
	uint32_t m_responseTime;  // Response time from last poll

   virtual void onObjectDelete(UINT32 objectId) override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  shared_ptr<Node> hostNode = shared_ptr<Node>(), uint32_t pollerNode = 0);
   virtual ~NetworkService();

   virtual int getObjectClass() const override { return OBJECT_NETWORKSERVICE; }

   virtual json_t *toJson() override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   void statusPoll(ClientSession *session, UINT32 rqId, const shared_ptr<Node>& pollerNode, ObjectQueue<Event> *eventQueue);

   uint32_t getResponseTime() const { return m_responseTime; }
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

   shared_ptr<Node> getParentNode() const;

public:
   VPNConnector();
   VPNConnector(bool hidden);
   virtual ~VPNConnector();

   virtual int getObjectClass() const override { return OBJECT_VPNCONNECTOR; }

   virtual json_t *toJson() override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   bool isLocalAddr(const InetAddress& addr) const;
   bool isRemoteAddr(const InetAddress& addr) const;
   UINT32 getPeerGatewayId() const { return m_dwPeerGateway; }
   InetAddress getPeerGatewayAddr() const;
};

/**
 * Reset poll timers for all objects
 */
void ResetObjectPollTimers(const shared_ptr<ScheduledTaskParameters>& parameters);

/**
 * Poll timers reset task
 */
#define DCT_RESET_POLL_TIMERS_TASK_ID _T("System.ResetPollTimers")

#define pollerLock(name) \
   _pollerLock(); \
   uint64_t __pollStartTime = GetCurrentTimeMs(); \
   PollState *__pollState = &m_ ##name##PollState; \

#define pollerUnlock() \
   __pollState->complete(GetCurrentTimeMs() - __pollStartTime); \
   setModified(MODIFY_DC_TARGET, false);\
   _pollerUnlock();

/**
 * Data collection proxy information structure
 */
struct ProxyInfo
{
   uint32_t proxyId;
   NXCPMessage *msg;
   uint32_t baseInfoFieldId;
   uint32_t extraInfoFieldId; // Separate field set is needed to keep compatibility with older agents
   uint32_t count;
   uint32_t nodeInfoFieldId;
   uint32_t nodeInfoCount;
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
   void complete(int64_t elapsed)
   {
      MutexLock(m_lock);
      m_lastCompleted = time(nullptr);
      m_timer->update(elapsed);
      InterlockedDecrement(&m_pollerCount);
      MutexUnlock(m_lock);
   }

   /**
    * Set last completed time (intended only for reading from database)
    */
   void setLastCompleted(time_t lastCompleted)
   {
      m_lastCompleted = lastCompleted;
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
   int64_t getTimerAverage()
   {
      MutexLock(m_lock);
      int64_t v = static_cast<int64_t>(m_timer->getAverage());
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get timer minimum value
    */
   int64_t getTimerMin()
   {
      MutexLock(m_lock);
      int64_t v = m_timer->getMin();
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get timer maximum value
    */
   int64_t getTimerMax()
   {
      MutexLock(m_lock);
      int64_t v = m_timer->getMax();
      MutexUnlock(m_lock);
      return v;
   }

   /**
    * Get last timer value
    */
   int64_t getTimerLast()
   {
      MutexLock(m_lock);
      int64_t v = m_timer->getCurrent();
      MutexUnlock(m_lock);
      return v;
   }
};

/**
 * Geolocation control mode for objects
 */
enum GeoLocationControlMode
{
   GEOLOCATION_NO_CONTROL = 0,
   GEOLOCATION_RESTRICTED_AREAS = 1,
   GEOLOCATION_ALLOWED_AREAS = 2
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE IntegerArray<uint32_t>;
#endif

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public DataCollectionOwner
{
private:
   typedef DataCollectionOwner super;

protected:
   IntegerArray<uint32_t> m_deletedItems;
   IntegerArray<uint32_t> m_deletedTables;
   StringMap m_scriptErrorReports;
   GeoLocationControlMode m_geoLocationControlMode;
   IntegerArray<uint32_t> m_geoAreas;
   bool m_geoLocationRestrictionsViolated;
   bool m_instanceDiscoveryPending;
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
   virtual void onInstanceDiscoveryChange() override;
	virtual bool isDataCollectionDisabled();

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);
   virtual void instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId);

   virtual StringMap *getInstanceList(DCObject *dco);
   void doInstanceDiscovery(UINT32 requestId);
   bool updateInstances(DCObject *root, StringObjectMap<InstanceDiscoveryData> *instances, UINT32 requestId);

   void updateDataCollectionTimeIntervals();

   void updateGeoLocation(const GeoLocation& geoLocation);

   void _pollerLock() { MutexLock(m_hPollerMutex); }
   void _pollerUnlock() { MutexUnlock(m_hPollerMutex); }

   shared_ptr<NetObj> objectFromParameter(const TCHAR *param) const;

   NXSL_VM *runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject);

   void applyUserTemplates();
   void updateContainerMembership();

   DataCollectionError queryWebService(const TCHAR *param, WebServiceRequestType queryType, TCHAR *buffer, size_t bufSize, StringList *list);

   void getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);
   void getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);

   void addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco, UINT32 primaryProxyId);
   void addProxySnmpTarget(ProxyInfo *info, const Node *node);
   void calculateProxyLoad();

   virtual void collectProxyInfo(ProxyInfo *info);
   static void collectProxyInfoCallback(NetObj *object, void *data);

   bool saveDCIListForCleanup(DB_HANDLE hdb);
   void loadDCIListForCleanup(DB_HANDLE hdb);

public:
   DataCollectionTarget();
   DataCollectionTarget(const TCHAR *name);
   virtual ~DataCollectionTarget();

   shared_ptr<DataCollectionTarget> self() const { return static_pointer_cast<DataCollectionTarget>(NObject::self()); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool setMgmtStatus(bool isManaged) override;
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;
   virtual bool isDataCollectionTarget() const override;

   virtual void enterMaintenanceMode(uint32_t userId, const TCHAR *comments) override;
   virtual void leaveMaintenanceMode(uint32_t userId) override;

   virtual DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size);
   virtual DataCollectionError getInternalTable(const TCHAR *name, Table **result);

   DataCollectionError getMetricFromScript(const TCHAR *param, TCHAR *buffer, size_t bufSize, DataCollectionTarget *targetObject);
   DataCollectionError getTableFromScript(const TCHAR *param, Table **result, DataCollectionTarget *targetObject);
   DataCollectionError getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject);
   DataCollectionError getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject);

   DataCollectionError getMetricFromWebService(const TCHAR *param, TCHAR *buffer, size_t bufSize);
   DataCollectionError getListFromWebService(const TCHAR *param, StringList **list);

   virtual uint32_t getEffectiveSourceNode(DCObject *dco);

   virtual json_t *toJson() override;

   NXSL_Array *getTemplatesForNXSL(NXSL_VM *vm);

   uint32_t getTableLastValue(uint32_t dciId, NXCPMessage *msg);
   uint32_t getDciLastValue(uint32_t dciId, NXCPMessage *msg);
   UINT32 getThresholdSummary(NXCPMessage *msg, UINT32 baseId, UINT32 userId);
   UINT32 getPerfTabDCIList(NXCPMessage *pMsg, UINT32 userId);
   void getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, UINT32 userId);
   UINT32 getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, UINT32 userId);
   double getProxyLoadFactor() const { return GetAttributeWithLock(m_proxyLoadFactor, m_mutexProperties); }

   void updateDciCache();
   void updateDCItemCacheSize(UINT32 dciId, UINT32 conditionId = 0);
   void reloadDCItemCache(UINT32 dciId);
   void cleanDCIData(DB_HANDLE hdb);
   void calculateDciCutoffTimes(time_t *cutoffTimeIData, time_t *cutoffTimeTData);
   void queueItemsForPolling();
   bool processNewDCValue(const shared_ptr<DCObject>& dco, time_t currTime, void *value);
   void scheduleItemDataCleanup(UINT32 dciId);
   void scheduleTableDataCleanup(UINT32 dciId);
   void queuePredictionEngineTraining();

   bool applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject);
   void cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList);
   virtual void unbindFromTemplate(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI);

   virtual bool isEventSource() const override;

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

   uint64_t getCacheMemoryUsage();
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
       (m_instanceDiscoveryPending || (static_cast<UINT32>(time(nullptr) - m_instancePollState.getLastCompleted()) > g_instancePollingInterval)))
   {
      success = m_instancePollState.schedule();
      m_instanceDiscoveryPending = false;
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
               (static_cast<UINT32>(time(nullptr) - m_configurationPollState.getLastCompleted()) > g_configurationPollingInterval))
      {
         success = m_configurationPollState.schedule();
      }
	}
   unlockProperties();
   return success;
}

/**
 * Lock object for status poll
 */
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
               ((UINT32)(time(nullptr) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval))
      {
         success = m_statusPollState.schedule();
      }
   }
   unlockProperties();
   return success;
}

/**
 * Mobile device system information
 */
struct MobileDeviceInfo
{
   const TCHAR *commProtocol;
   SharedString vendor;
   SharedString model;
   SharedString serialNumber;
   SharedString osName;
   SharedString osVersion;
   SharedString userId;
};

/**
 * Moile device status
 */
struct MobileDeviceStatus
{
   const TCHAR *commProtocol;
   GeoLocation geoLocation;
   int32_t altitude;
   float speed;
   int16_t direction;
   int8_t batteryLevel;
   InetAddress ipAddress;

   MobileDeviceStatus()
   {
      commProtocol = _T("UNKNOWN");
      altitude = 0;
      speed = -1;
      direction = -1;
      batteryLevel = -1;
   }
};

/**
 * Mobile device class
 */
class NXCORE_EXPORTABLE MobileDevice : public DataCollectionTarget
{
private:
   typedef DataCollectionTarget super;

protected:
   TCHAR m_commProtocol[32];
	time_t m_lastReportTime;
	SharedString m_deviceId;
	SharedString m_vendor;
	SharedString m_model;
	SharedString m_serialNumber;
	SharedString m_osName;
	SharedString m_osVersion;
	SharedString m_userId;
	int8_t m_batteryLevel;
   InetAddress m_ipAddress;
   int32_t m_altitude;
   float m_speed;
   int16_t m_direction;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   shared_ptr<MobileDevice> self() const { return static_pointer_cast<MobileDevice>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_MOBILEDEVICE; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual json_t *toJson() override;

	void updateSystemInfo(const MobileDeviceInfo& deviceInfo);
	void updateStatus(const MobileDeviceStatus& status);

	const TCHAR *getCommProtocol() const { return m_commProtocol; }
	time_t getLastReportTime() const { return GetAttributeWithLock(m_lastReportTime, m_mutexProperties); }
	SharedString getDeviceId() const { return GetAttributeWithLock(m_deviceId, m_mutexProperties); }
	SharedString getVendor() const { return GetAttributeWithLock(m_vendor, m_mutexProperties); }
	SharedString getModel() const { return GetAttributeWithLock(m_model, m_mutexProperties); }
	SharedString getSerialNumber() const { return GetAttributeWithLock(m_serialNumber, m_mutexProperties); }
	SharedString getOsName() const { return GetAttributeWithLock(m_osName, m_mutexProperties); }
	SharedString getOsVersion() const { return GetAttributeWithLock(m_osVersion, m_mutexProperties); }
	SharedString getUserId() const { return GetAttributeWithLock(m_userId, m_mutexProperties); }
	int8_t getBatteryLevel() const { return GetAttributeWithLock(m_batteryLevel, m_mutexProperties); }
   int16_t getDirection() const { return GetAttributeWithLock(m_direction, m_mutexProperties); }
   float getSpeed() const { return GetAttributeWithLock(m_speed, m_mutexProperties); }
   int32_t getAltitude() const { return GetAttributeWithLock(m_altitude, m_mutexProperties); }

	virtual DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size) override;

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
   uint32_t m_index;
   InetAddress m_ipAddress;
   uint32_t m_nodeId;
	MacAddress m_macAddr;
	TCHAR *m_vendor;
	TCHAR *m_model;
	TCHAR *m_serialNumber;
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;
   AccessPointState m_apState;
   AccessPointState m_prevState;

	virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, uint32_t index, const MacAddress& macAddr);
   virtual ~AccessPoint();

   shared_ptr<AccessPoint> self() const { return static_pointer_cast<AccessPoint>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_ACCESSPOINT; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual int32_t getZoneUIN() const override;

   virtual json_t *toJson() override;

   void statusPollFromController(ClientSession *session, UINT32 rqId, ObjectQueue<Event> *eventQueue, Node *controller, SNMP_Transport *snmpTransport);

   uint32_t getIndex() const { return m_index; }
	const MacAddress& getMacAddr() const { return m_macAddr; }
   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
	bool isMyRadio(int rfIndex);
	bool isMyRadio(const BYTE *macAddr);
	void getRadioName(int rfIndex, TCHAR *buffer, size_t bufSize);
   AccessPointState getApState() const { return m_apState; }
   shared_ptr<Node> getParentNode() const;
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
   virtual bool lockForInstancePoll() override { return false; }
};

/**
 * Cluster class
 */
class NXCORE_EXPORTABLE Cluster : public DataCollectionTarget, public AutoBindTarget
{
private:
   typedef DataCollectionTarget super;

protected:
	UINT32 m_dwClusterType;
   ObjectArray<InetAddress> *m_syncNetworks;
	UINT32 m_dwNumResources;
	CLUSTER_RESOURCE *m_pResourceList;
	int32_t m_zoneUIN;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *pRequest) override;

   virtual void onDataCollectionChange() override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

   UINT32 getResourceOwnerInternal(UINT32 id, const TCHAR *name);

public:
	Cluster();
   Cluster(const TCHAR *pszName, int32_t zoneUIN);
	virtual ~Cluster();

   shared_ptr<Cluster> self() const { return static_pointer_cast<Cluster>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CLUSTER; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool showThresholdSummary() const override;

   virtual bool lockForInstancePoll() override { return false; }

   virtual void unbindFromTemplate(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

	bool isSyncAddr(const InetAddress& addr);
	bool isVirtualAddr(const InetAddress& addr);
	bool isResourceOnNode(UINT32 dwResource, UINT32 dwNode);
	UINT32 getResourceOwner(UINT32 resourceId) { return getResourceOwnerInternal(resourceId, nullptr); }
   UINT32 getResourceOwner(const TCHAR *resourceName) { return getResourceOwnerInternal(0, resourceName); }

   UINT32 collectAggregatedData(DCItem *item, TCHAR *buffer);
   UINT32 collectAggregatedData(DCTable *table, Table **result);

   NXSL_Array *getNodesForNXSL(NXSL_VM *vm);
   void addNode(shared_ptr<Node> node);
   void removeNode(shared_ptr<Node> node);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<Cluster>;
#endif

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

   shared_ptr<Chassis> self() const { return static_pointer_cast<Chassis>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CHASSIS; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;
   virtual bool showThresholdSummary() const override;
   virtual uint32_t getEffectiveSourceNode(DCObject *dco) override;

   virtual bool lockForStatusPoll() override { return false; }
   virtual bool lockForConfigurationPoll() override { return false; }
   virtual bool lockForInstancePoll() override { return false; }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

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
	uint32_t m_deviceClass; // Internal device class UPS, meeter
	TCHAR *m_vendor; // Vendor name lorawan...
	uint32_t m_commProtocol; // lorawan, dlms, dlms throuht other protocols
	TCHAR *m_xmlConfig; //protocol specific configuration
	TCHAR *m_xmlRegConfig; //protocol specific registration configuration (cannot be changed afterwards)
	TCHAR *m_serialNumber; //Device serial number
	TCHAR *m_deviceAddress; //in case lora - lorawan id
	TCHAR *m_metaType;//brief type hot water, elecrticety
	TCHAR *m_description; //brief description
	time_t m_lastConnectionTime;
	uint32_t m_frameCount; //zero when no info
   INT32 m_signalStrenght; //+1 when no information(cannot be +)
   INT32 m_signalNoise; //*10 from origin number //MAX_INT32 when no value
   uint32_t m_frequency; //*10 from origin number // 0 when no value
   uint32_t m_proxyNodeId;

	virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId) override;

   virtual StringMap *getInstanceList(DCObject *dco) override;

   void calculateStatus(BOOL bForcedRecalc = FALSE);

   static bool registerLoraDevice(Sensor *sensor);

   void buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies);

public:
   static shared_ptr<Sensor> create(const TCHAR *name, const NXCPMessage *msg);

   Sensor();
   Sensor(const TCHAR *name, const NXCPMessage *msg); // Intended for use by create() call only
   virtual ~Sensor();

   shared_ptr<Sensor> self() const { return static_pointer_cast<Sensor>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_SENSOR; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual void prepareForDeletion() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

   const TCHAR *getXmlConfig() const { return m_xmlConfig; }
   const TCHAR *getXmlRegConfig() const { return m_xmlRegConfig; }
   uint32_t getProxyNodeId() const { return m_proxyNodeId; }
   const TCHAR *getDeviceAddress() const { return m_deviceAddress; }
   const MacAddress getMacAddress() const { return m_macAddress; }
   time_t getLastContact() const { return m_lastConnectionTime; }
   uint32_t getSensorClass() const { return m_deviceClass; }
   const TCHAR *getVendor() const { return m_vendor; }
   uint32_t getCommProtocol() const { return m_commProtocol; }
   const TCHAR *getSerialNumber() const { return m_serialNumber; }
   const TCHAR *getMetaType() const { return m_metaType; }
   const TCHAR *getDescription() const { return m_description; }
   uint32_t getFrameCount() const { return m_frameCount; }

   DataCollectionError getMetricFromAgent(const TCHAR *naram, TCHAR *buffer, size_t bufferSize);
   DataCollectionError getListFromAgent(const TCHAR *name, StringList **list);
   DataCollectionError getTableFromAgent(const TCHAR *name, Table **table);

   void setProvisoned() { m_state |= SSF_PROVISIONED; }

   virtual json_t *toJson() override;

   shared_ptr<AgentConnectionEx> getAgentConnection();

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
   uint32_t m_nodeId;
   uint64_t m_eventId;

public:
   RoutingLoopEvent(const InetAddress& address, uint32_t nodeId, uint64_t eventId)
   {
      m_address = address;
      m_nodeId = nodeId;
      m_eventId = eventId;
   }

   const InetAddress& getAddress() const { return m_address; }
   uint32_t getNodeId() const { return m_nodeId; }
   uint64_t getEventId() const { return m_eventId; }
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
 * Reason of network path check failure
 */
enum class NetworkPathFailureReason
{
   NONE = 0,
   ROUTER_DOWN = 1,
   SWITCH_DOWN = 2,
   WIRELESS_AP_DOWN = 3,
   PROXY_NODE_DOWN = 4,
   PROXY_AGENT_UNREACHABLE = 5,
   VPN_TUNNEL_DOWN = 6,
   ROUTING_LOOP = 7,
   INTERFACE_DISABLED = 8
};

/**
 * Get failure reason enum value from int
 */
static inline NetworkPathFailureReason NetworkPathFailureReasonFromInt(int32_t i)
{
   switch(i)
   {
      case 1: return NetworkPathFailureReason::ROUTER_DOWN;
      case 2: return NetworkPathFailureReason::SWITCH_DOWN;
      case 3: return NetworkPathFailureReason::WIRELESS_AP_DOWN;
      case 4: return NetworkPathFailureReason::PROXY_NODE_DOWN;
      case 5: return NetworkPathFailureReason::PROXY_AGENT_UNREACHABLE;
      case 6: return NetworkPathFailureReason::VPN_TUNNEL_DOWN;
      case 7: return NetworkPathFailureReason::ROUTING_LOOP;
      case 8: return NetworkPathFailureReason::INTERFACE_DISABLED;
      default: return NetworkPathFailureReason::NONE;
   }
}

/**
 * Result of network path check
 */
struct NetworkPathCheckResult
{
   bool rootCauseFound;
   NetworkPathFailureReason reason;
   uint32_t rootCauseNodeId;
   uint32_t rootCauseInterfaceId;

   /**
    * Construct negative result
    */
   NetworkPathCheckResult()
   {
      rootCauseFound = false;
      reason = NetworkPathFailureReason::NONE;
      rootCauseNodeId = 0;
      rootCauseInterfaceId = 0;
   }

   /**
    * Construct positive result for other reasons
    */
   NetworkPathCheckResult(NetworkPathFailureReason r, uint32_t nodeId, uint32_t interfaceId = 0)
   {
      rootCauseFound = true;
      reason = r;
      rootCauseNodeId = nodeId;
      rootCauseInterfaceId = interfaceId;
   }
};

/**
 * Container for proxy agent connections
 */
class ProxyAgentConnection : public ObjectLock<shared_ptr<AgentConnectionEx>>
{
private:
   time_t m_lastConnect;

public:
   ProxyAgentConnection() : ObjectLock<shared_ptr<AgentConnectionEx>>() { m_lastConnect = 0; }
   ProxyAgentConnection(const shared_ptr<AgentConnectionEx>& object) : ObjectLock<shared_ptr<AgentConnectionEx>>(object) { m_lastConnect = 0; }
   ProxyAgentConnection(const ProxyAgentConnection &src) : ObjectLock<shared_ptr<AgentConnectionEx>>(src) { m_lastConnect = src.m_lastConnect; }

   void setLastConnectTime(time_t t) { m_lastConnect = t; }
   time_t getLastConnectTime() const { return m_lastConnect; }
};

// Explicit instantiation of shared_ptr<*> to enable DLL interface for them
#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<ComponentTree>;
template class NXCORE_EXPORTABLE shared_ptr<NetworkPath>;
template class NXCORE_EXPORTABLE shared_ptr<VlanList>;
#endif

/**
 * Node hardware ID
 */
class NXCORE_EXPORTABLE NodeHardwareId : public GenericId<HARDWARE_ID_LENGTH>
{
public:
   NodeHardwareId() : GenericId<HARDWARE_ID_LENGTH>(HARDWARE_ID_LENGTH) { }
   NodeHardwareId(const BYTE *value) : GenericId<HARDWARE_ID_LENGTH>(value, HARDWARE_ID_LENGTH) { }
   NodeHardwareId(const NodeHardwareId& src) : GenericId<HARDWARE_ID_LENGTH>(src) { }

   bool equals(const NodeHardwareId &a) const { return GenericId<HARDWARE_ID_LENGTH>::equals(a); }
   bool equals(const BYTE *value) const { return GenericId<HARDWARE_ID_LENGTH>::equals(value, HARDWARE_ID_LENGTH); }
};

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
	   if (m_agentConnection != nullptr)
	      m_agentConnection.reset();
	}

	void onSnmpProxyChange(uint32_t oldProxy);
   void onDataCollectionChangeAsyncCallback();

	bool updateSystemHardwareProperty(SharedString &property, const TCHAR *value, const TCHAR *displayName, uint32_t requestId);

protected:
   InetAddress m_ipAddress;
	SharedString m_primaryHostName;
	uuid m_tunnelId;
	CertificateMappingMethod m_agentCertMappingMethod;
   TCHAR *m_agentCertMappingData;
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
	int32_t m_zoneUIN;
   uint16_t m_agentPort;
   int16_t m_agentCacheMode;
   int16_t m_agentCompressionMode;  // agent compression mode (enabled/disabled/default)
   TCHAR m_agentSecret[MAX_SECRET_LENGTH];
   int16_t m_iStatusPollType;
   SNMP_Version m_snmpVersion;
   uint16_t m_snmpPort;
   uint16_t m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
	uuid m_agentId;
	TCHAR *m_agentCertSubject;
   TCHAR m_agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_platformName[MAX_PLATFORM_NAME_LEN];
   NodeHardwareId m_hardwareId;
   TCHAR *m_snmpObjectId;
	TCHAR *m_sysDescription;   // Agent's System.Uname or SNMP sysDescr
	TCHAR *m_sysName;				// SNMP sysName
	TCHAR *m_sysLocation;      // SNMP sysLocation
	TCHAR *m_sysContact;       // SNMP sysContact
	TCHAR *m_lldpNodeId;			// lldpLocChassisId combined with lldpLocChassisIdSubtype, or nullptr for non-LLDP nodes
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
   time_t m_recoveryTime;
	time_t m_downSince;
   time_t m_bootTime;
   time_t m_agentUpTime;
   time_t m_lastAgentCommTime;
   time_t m_lastAgentConnectAttempt;
   MUTEX m_hAgentAccessMutex;
   MUTEX m_hSmclpAccessMutex;
   MUTEX m_mutexRTAccess;
	MUTEX m_mutexTopoAccess;
   shared_ptr<AgentConnectionEx> m_agentConnection;
   ProxyAgentConnection *m_proxyConnections;
   VolatileCounter m_pendingDataConfigurationSync;
   SMCLP_Connection *m_smclpConnection;
   uint64_t m_lastAgentTrapId;	     // ID of last received agent trap
   uint64_t m_lastAgentPushRequestId; // ID of last received agent push request
   uint32_t m_lastSNMPTrapId;
   uint64_t m_lastSyslogMessageId; // ID of last received syslog message
   uint64_t m_lastWindowsEventId;  // ID of last received Windows event
   uint32_t m_pollerNode;      // Node used for network service polling
   uint32_t m_agentProxy;      // Node used as proxy for agent connection
   uint32_t m_snmpProxy;       // Node used as proxy for SNMP requests
   uint32_t m_eipProxy;        // Node used as proxy for EtherNet/IP requests
   uint32_t m_icmpProxy;       // Node used as proxy for ICMP ping
   uint64_t m_lastEvents[MAX_LAST_EVENTS];
   ObjectArray<RoutingLoopEvent> *m_routingLoopEvents;
   RoutingTable *m_routingTable;
   shared_ptr<NetworkPath> m_lastKnownNetworkPath;
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
	shared_ptr<ComponentTree> m_components;		// Hardware components
	ObjectArray<SoftwarePackage> *m_softwarePackages;  // installed software packages
   ObjectArray<HardwareComponent> *m_hardwareComponents;  // installed hardware components
	ObjectArray<WinPerfObject> *m_winPerfObjects;  // Windows performance objects
	shared_ptr<AgentConnection> m_fileUpdateConnection;
	int16_t m_rackHeight;
	int16_t m_rackPosition;
	uint32_t m_physicalContainer;
	uuid m_rackImageFront;
   uuid m_rackImageRear;
	int64_t m_syslogMessageCount;
   int64_t m_snmpTrapCount;
   int64_t m_snmpTrapLastTotal;
   time_t m_snmpTrapStormLastCheckTime;
   uint32_t m_snmpTrapStormActualDuration;
   TCHAR m_sshLogin[MAX_SSH_LOGIN_LEN];
	TCHAR m_sshPassword[MAX_SSH_PASSWORD_LEN];
	uint32_t m_sshKeyId;
	uint16_t m_sshPort;
	uint32_t m_sshProxy;
	uint32_t m_portNumberingScheme;
	uint32_t m_portRowCount;
	RackOrientation m_rackOrientation;
   IcmpStatCollectionMode m_icmpStatCollectionMode;
   StringObjectMap<IcmpStatCollector> *m_icmpStatCollectors;
   InetAddressList m_icmpTargets;
   TCHAR *m_chassisPlacementConf;
   uint16_t m_eipPort;  // EtherNet/IP port
   uint16_t m_cipDeviceType;
   uint16_t m_cipStatus;
   uint16_t m_cipState;
   uint16_t m_cipVendorCode;

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

   virtual bool getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const override;

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

   uint32_t getInterfaceCount(Interface **lastInterface);
   void deleteInterface(Interface *iface);

   void checkInterfaceNames(InterfaceList *pIfList);
   shared_ptr<Interface> createInterfaceObject(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface, bool syntheticMask);
   shared_ptr<Subnet> createSubnet(InetAddress& baseAddr, bool syntheticMask);
   void checkAgentPolicyBinding(const shared_ptr<AgentConnectionEx>& conn);
   void updatePrimaryIpAddr();
   bool confPollAgent(uint32_t requestId);
   bool confPollSnmp(uint32_t requestId);
   bool confPollEthernetIP(uint32_t requestId);
   NodeType detectNodeType(TCHAR *hypervisorType, TCHAR *hypervisorInfo);
   bool updateSystemHardwareInformation(PollerInfo *poller, uint32_t requestId);
   bool updateHardwareComponents(PollerInfo *poller, uint32_t requestId);
   bool updateSoftwarePackages(PollerInfo *poller, uint32_t requestId);
   bool querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, UINT32 pollRqId, TCHAR **value);
   void checkBridgeMib(SNMP_Transport *pTransport);
   void checkIfXTable(SNMP_Transport *pTransport);
   NetworkPathCheckResult checkNetworkPath(uint32_t requestId);
   NetworkPathCheckResult checkNetworkPathLayer2(uint32_t requestId, bool secondPass);
   NetworkPathCheckResult checkNetworkPathLayer3(uint32_t requestId, bool secondPass);
   NetworkPathCheckResult checkNetworkPathElement(uint32_t nodeId, const TCHAR *nodeType, bool isProxy, bool isSwitch, uint32_t requestId, bool secondPass);
   void icmpPollAddress(AgentConnection *conn, const TCHAR *target, const InetAddress& addr);

   void syncDataCollectionWithAgent(AgentConnectionEx *conn);

   bool updateInterfaceConfiguration(uint32_t requestId, int maskBits);
   bool deleteDuplicateInterfaces(uint32_t requestId);
   void executeInterfaceUpdateHook(Interface *iface);
   void updatePhysicalContainerBinding(uint32_t containerId);
   DuplicateCheckResult checkForDuplicates(shared_ptr<Node> *duplicate, TCHAR *reason, size_t size);
   bool isDuplicateOf(Node *node, TCHAR *reason, size_t size);
   void reconcileWithDuplicateNode(Node *node);

   bool connectToAgent(UINT32 *error = nullptr, UINT32 *socketError = nullptr, bool *newConnection = nullptr, bool forceConnect = false);
   void setLastAgentCommTime() { m_lastAgentCommTime = time(nullptr); }

   void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedObject, const TCHAR *linkName, bool vpnLink, bool includeEndNodes);
   void buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology);
   void buildInternalConnectionTopologyInternal(NetworkMapObjectList *topology, UINT32 seedNode, bool agentConnectionOnly, bool checkAllProxies);
   bool checkProxyAndLink(NetworkMapObjectList *topology, UINT32 seedNode, UINT32 proxyId, UINT32 linkType, const TCHAR *linkName, bool checkAllProxies);

   void updateClusterMembership();

public:
   Node();
   Node(const NewNodeData *newNodeData, UINT32 flags);
   virtual ~Node();

   shared_ptr<Node> self() const { return static_pointer_cast<Node>(NObject::self()); }

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

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   shared_ptr<Cluster> getMyCluster();

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

   void setType(NodeType type, const TCHAR *subType)
   {
      lockProperties();
      m_type = type;
      _tcslcpy(m_subType, subType, MAX_NODE_SUBTYPE_LENGTH);
      unlockProperties();
   }

   void setIfXtableUsageMode(int mode)
   {
      lockProperties();
      m_nUseIfXTable = mode;
      setModified(MODIFY_NODE_PROPERTIES);
      unlockProperties();
   }

   bool isSNMPSupported() const { return m_capabilities & NC_IS_SNMP ? true : false; }
   bool isNativeAgent() const { return m_capabilities & NC_IS_NATIVE_AGENT ? true : false; }
   bool isEthernetIPSupported() const { return m_capabilities & NC_IS_ETHERNET_IP ? true : false; }
   bool isModbusTCPSupported() const { return m_capabilities & NC_IS_MODBUS_TCP ? true : false; }
   bool isProfiNetSupported() const { return m_capabilities & NC_IS_PROFINET ? true : false; }
   bool isBridge() const { return m_capabilities & NC_IS_BRIDGE ? true : false; }
   bool isRouter() const { return m_capabilities & NC_IS_ROUTER ? true : false; }
   bool isLocalManagement() const { return m_capabilities & NC_IS_LOCAL_MGMT ? true : false; }
   bool isPerVlanFdbSupported() const { return (m_driver != nullptr) ? m_driver->isPerVlanFdbSupported() : false; }
   bool isWirelessController() const { return m_capabilities & NC_IS_WIFI_CONTROLLER ? true : false; }
   bool isNewPolicyTypeFormatSupported() const { return m_capabilities & NC_IS_NEW_POLICY_TYPES ? true : false; }

   bool isIcmpStatCollectionEnabled() const
   {
      return (m_icmpStatCollectionMode == IcmpStatCollectionMode::DEFAULT) ?
               ((g_flags & AF_COLLECT_ICMP_STATISTICS) != 0) : (m_icmpStatCollectionMode == IcmpStatCollectionMode::ON);
   }

   bool is8021xPollingEnabled() const
   {
      return ((g_flags & AF_ENABLE_8021X_STATUS_POLL) != 0) && ((m_flags & NF_DISABLE_8021X_STATUS_POLL) == 0);
   }

   const uuid& getAgentId() const { return m_agentId; }
   const TCHAR *getAgentVersion() const { return m_agentVersion; }
   const TCHAR *getPlatformName() const { return m_platformName; }
   const NodeHardwareId& getHardwareId() const { return m_hardwareId; }
   SNMP_Version getSNMPVersion() const { return m_snmpVersion; }
   uint16_t getSNMPPort() const { return m_snmpPort; }
   uint32_t getSNMPProxy() const { return m_snmpProxy; }
   String getSNMPObjectId() const { return GetStringAttributeWithLock(m_snmpObjectId, m_mutexProperties); }
   const TCHAR *getSysName() const { return CHECK_NULL_EX(m_sysName); }
   const TCHAR *getSysDescription() const { return CHECK_NULL_EX(m_sysDescription); }
   const TCHAR *getSysContact() const { return CHECK_NULL_EX(m_sysContact); }
   const TCHAR *getSysLocation() const { return CHECK_NULL_EX(m_sysLocation); }
   time_t getBootTime() const { return m_bootTime; }
   const TCHAR *getLLDPNodeId() const { return m_lldpNodeId; }
   const BYTE *getBridgeId() const { return m_baseBridgeAddress; }
   const TCHAR *getDriverName() const { return (m_driver != nullptr) ? m_driver->getName() : _T("GENERIC"); }
   uint16_t getAgentPort() const { return m_agentPort; }
   int16_t getAgentCacheMode() const { return (m_state & NSF_CACHE_MODE_NOT_SUPPORTED) ? AGENT_CACHE_OFF : ((m_agentCacheMode == AGENT_CACHE_DEFAULT) ? g_defaultAgentCacheMode : m_agentCacheMode); }
   const TCHAR *getAgentSecret() const { return m_agentSecret; }
   uint32_t getAgentProxy() const { return m_agentProxy; }
   uint32_t getPhysicalContainerId() const { return m_physicalContainer; }
   INT16 getRackHeight() const { return m_rackHeight; }
   INT16 getRackPosition() const { return m_rackPosition; }
   bool hasFileUpdateConnection() const { lockProperties(); bool result = (m_fileUpdateConnection != nullptr); unlockProperties(); return result; }
   uint32_t getIcmpProxy() const { return m_icmpProxy; }
   const TCHAR *getSshLogin() const { return m_sshLogin; }
   const TCHAR *getSshPassword() const { return m_sshPassword; }
   uint16_t getSshPort() const { return m_sshPort; }
   uint32_t getSshKeyId() const { return m_sshKeyId; }
   uint32_t getSshProxy() const { return m_sshProxy; }
   time_t getLastAgentCommTime() const { return m_lastAgentCommTime; }
   SharedString getPrimaryHostName() const { return GetAttributeWithLock(m_primaryHostName, m_mutexProperties); }
   const uuid& getTunnelId() const { return m_tunnelId; }
   const TCHAR *getAgentCertificateSubject() const { return m_agentCertSubject; }
   CertificateMappingMethod getAgentCertificateMappingMethod() const { return m_agentCertMappingMethod; }
   const TCHAR *getAgentCertificateMappingData() const { return m_agentCertMappingData; }
   uint32_t getRequiredPollCount() const { return m_requiredPollCount; }
   uint16_t getCipDeviceType() const { return m_cipDeviceType; }
   uint16_t getCipStatus() const { return m_cipStatus; }
   uint16_t getCipState() const { return m_cipState; }
   uint16_t getCipVendorCode() const { return m_cipVendorCode; }

   bool isDown() { return (m_state & DCSF_UNREACHABLE) ? true : false; }
   time_t getDownSince() const { return m_downSince; }

   void setNewTunnelBindFlag() { lockProperties(); m_runtimeFlags |= NDF_NEW_TUNNEL_BIND; unlockProperties(); }
   void clearNewTunnelBindFlag() { lockProperties(); m_runtimeFlags &= ~NDF_NEW_TUNNEL_BIND; unlockProperties(); }

   void addInterface(const shared_ptr<Interface>& iface) { addChild(iface); iface->addParent(self()); }
   shared_ptr<Interface> createNewInterface(InterfaceInfo *ifInfo, bool manuallyCreated, bool fakeInterface);
   shared_ptr<Interface> createNewInterface(const InetAddress& ipAddr, const MacAddress& macAddr, bool fakeInterface);

   void setPrimaryHostName(const TCHAR *name) { lockProperties(); m_primaryHostName = name; unlockProperties(); }
   void setAgentPort(UINT16 port) { m_agentPort = port; }
   void setSnmpPort(UINT16 port) { m_snmpPort = port; }
   void setSshCredentials(const TCHAR *login, const TCHAR *password);
   void changeIPAddress(const InetAddress& ipAddr);
   void changeZone(UINT32 newZone);
   void setTunnelId(const uuid& tunnelId, const TCHAR *certSubject);
   bool setFileUpdateConnection(const shared_ptr<AgentConnection>& connection);
   void clearFileUpdateConnection();
   void clearDataCollectionConfigFromAgent(AgentConnectionEx *conn);
   void forceSyncDataCollectionConfig();
   void relatedNodeDataCollectionChanged() { onDataCollectionChange(); }
   void clearSshKey() { m_sshKeyId = 0; }

   ArpCache *getArpCache(bool forceRead = false);
   InterfaceList *getInterfaceList();
   shared_ptr<Interface> findInterfaceByIndex(UINT32 ifIndex) const;
   shared_ptr<Interface> findInterfaceByName(const TCHAR *name) const;
   shared_ptr<Interface> findInterfaceByAlias(const TCHAR *alias) const;
   shared_ptr<Interface> findInterfaceByMAC(const MacAddress& macAddr) const;
   shared_ptr<Interface> findInterfaceByIP(const InetAddress& addr) const;
   shared_ptr<Interface> findInterfaceBySubnet(const InetAddress& subnet) const;
   shared_ptr<Interface> findInterfaceByLocation(const InterfacePhysicalLocation& location) const;
   shared_ptr<Interface> findBridgePort(UINT32 bridgePortNumber) const;
   shared_ptr<AccessPoint> findAccessPointByMAC(const MacAddress& macAddr) const;
   shared_ptr<AccessPoint> findAccessPointByBSSID(const BYTE *bssid) const;
   shared_ptr<AccessPoint> findAccessPointByRadioId(int rfIndex) const;
   ObjectArray<WirelessStationInfo> *getWirelessStations() const;
   bool isMyIP(const InetAddress& addr) const;
   void getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, UINT32 dwIndex, int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState);
   void getInterfaceStatusFromAgent(UINT32 dwIndex, InterfaceAdminState *adminState, InterfaceOperState *operState);
   RoutingTable *getRoutingTable();
   RoutingTable *getCachedRoutingTable() { return m_routingTable; }
   shared_ptr<NetworkPath> getLastKnownNetworkPath() const { return GetAttributeWithLock(m_lastKnownNetworkPath, m_mutexProperties); }
   LinkLayerNeighbors *getLinkLayerNeighbors();
   shared_ptr<VlanList> getVlans();
   bool getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, uint32_t *ifIndex, bool *isVpn, TCHAR *name);
   bool getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, uint32_t *srcIfIndex);
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

	virtual bool setMgmtStatus(bool isManaged) override;
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) override;

	bool checkAgentTrapId(UINT64 id);
	bool checkSNMPTrapId(UINT32 id);
   bool checkSyslogMessageId(UINT64 id);
   bool checkWindowsEventId(uint64_t id);
   bool checkAgentPushRequestId(UINT64 id);

   bool connectToSMCLP();

   virtual DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size) override;
   virtual DataCollectionError getInternalTable(const TCHAR *name, Table **result) override;

   DataCollectionError getMetricFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *name, TCHAR *buffer, size_t size, int interpretRawValue);
   DataCollectionError getTableFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, const ObjectArray<DCTableColumn> &columns, Table **table);
   DataCollectionError getListFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, StringList **list);
   DataCollectionError getOIDSuffixListFromSNMP(UINT16 port, SNMP_Version version, const TCHAR *oid, StringMap **values);
   DataCollectionError getMetricFromAgent(const TCHAR *name, TCHAR *buffer, size_t size);
   DataCollectionError getTableFromAgent(const TCHAR *name, Table **table);
   DataCollectionError getListFromAgent(const TCHAR *name, StringList **list);
   DataCollectionError getMetricFromSMCLP(const TCHAR *name, TCHAR *buffer, size_t size);
   DataCollectionError getMetricFromDeviceDriver(const TCHAR *name, TCHAR *buffer, size_t size);

   double getMetricFromAgentAsDouble(const TCHAR *name, double defaultValue = 0);
   int32_t getMetricFromAgentAsInt32(const TCHAR *name, int32_t defaultValue = 0);
   uint32_t getMetricFromAgentAsUInt32(const TCHAR *name, uint32_t defaultValue = 0);
   int64_t getMetricFromAgentAsInt64(const TCHAR *name, int64_t defaultValue = 0);
   uint64_t getMetricFromAgentAsUInt64(const TCHAR *name, uint64_t defaultValue = 0);

   uint32_t getMetricForClient(int origin, uint32_t userId, const TCHAR *name, TCHAR *buffer, size_t size);
   uint32_t getTableForClient(const TCHAR *name, Table **table);

	virtual NXSL_Array *getParentsForNXSL(NXSL_VM *vm) override;
	NXSL_Array *getInterfacesForNXSL(NXSL_VM *vm);

   ObjectArray<AgentParameterDefinition> *openParamList(int origin);
   void closeParamList() { unlockProperties(); }

   ObjectArray<AgentTableDefinition> *openTableList();
   void closeTableList() { unlockProperties(); }

   shared_ptr<AgentConnectionEx> createAgentConnection(bool sendServerId = false);
   shared_ptr<AgentConnectionEx> getAgentConnection(bool forcePrimary = false);
   shared_ptr<AgentConnectionEx> acquireProxyConnection(ProxyType type, bool validate = false);
	SNMP_Transport *createSnmpTransport(UINT16 port = 0, SNMP_Version version = SNMP_VERSION_DEFAULT, const TCHAR *context = nullptr);
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

	Table *wsListAsTable();

   UINT32 wakeUp();

   void addService(const shared_ptr<NetworkService>& service) { addChild(service); service->addParent(self()); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, TCHAR *pszRequest = nullptr, TCHAR *pszResponse = nullptr, UINT32 *responseTime = nullptr);

   uint64_t getLastEventId(int index) const { return ((index >= 0) && (index < MAX_LAST_EVENTS)) ? m_lastEvents[index] : 0; }
   void setLastEventId(int index, uint64_t eventId) { if ((index >= 0) && (index < MAX_LAST_EVENTS)) m_lastEvents[index] = eventId; }
   void setRoutingLoopEvent(const InetAddress& address, uint32_t nodeId, uint64_t eventId);

   UINT32 callSnmpEnumerate(const TCHAR *pszRootOid,
      UINT32 (* pHandler)(SNMP_Variable *, SNMP_Transport *, void *), void *pArg,
      const TCHAR *context = nullptr, bool failOnShutdown = false);

   NetworkMapObjectList *getL2Topology();
   NetworkMapObjectList *buildL2Topology(UINT32 *pdwStatus, int radius, bool includeEndNodes);
	ForwardingDatabase *getSwitchForwardingDatabase();
	shared_ptr<NetObj> findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type);
	void addHostConnections(LinkLayerNeighbors *nbs);
	void addExistingConnections(LinkLayerNeighbors *nbs);
	NetworkMapObjectList *buildIPTopology(UINT32 *pdwStatus, int radius, bool includeEndNodes);

   bool getIcmpStatistics(const TCHAR *target, UINT32 *last, UINT32 *min, UINT32 *max, UINT32 *avg, UINT32 *loss) const;
   DataCollectionError getIcmpStatistic(const TCHAR *param, IcmpStatFunction function, TCHAR *value) const;
   StringList *getIcmpStatCollectors() const;

   NetworkDeviceDriver *getDriver() const { return m_driver; }
	DriverData *getDriverData() { return m_driverData; }
	void setDriverData(DriverData *data) { m_driverData = data; }

	void incSyslogMessageCount();
	void incSnmpTrapCount();
	bool checkTrapShouldBeProcessed();

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
               (getMyCluster() == nullptr) &&
               (!(m_runtimeFlags & ODF_CONFIGURATION_POLL_PENDING)) &&
               (static_cast<UINT32>(time(nullptr) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval))
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
       (static_cast<UINT32>(time(nullptr) - m_discoveryPollState.getLastCompleted()) > g_discoveryPollingInterval))
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
       (static_cast<UINT32>(time(nullptr) - m_routingPollState.getLastCompleted()) > g_routingTableUpdateInterval))
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
       (static_cast<UINT32>(time(nullptr) - m_topologyPollState.getLastCompleted()) > g_topologyPollingInterval))
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
       ((UINT32)(time(nullptr) - m_icmpPollState.getLastCompleted()) > g_icmpPollingInterval))
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
   int32_t m_zoneUIN;
	bool m_bSyntheticMask;

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *pMsg, UINT32 userId) override;

   void buildIPTopologyInternal(NetworkMapObjectList &topology, int nDepth, UINT32 seedNode, bool includeEndNodes);

public:
   Subnet();
   Subnet(const InetAddress& addr, int32_t zoneUIN, bool bSyntheticMask);
   virtual ~Subnet();

   shared_ptr<Subnet> self() const { return static_pointer_cast<Subnet>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_SUBNET; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   void addNode(const shared_ptr<Node>& node) { addChild(node); node->addParent(self()); calculateCompoundStatus(TRUE); }
   shared_ptr<Node> getOtherNode(uint32_t nodeId);

   virtual bool showThresholdSummary() const override;

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
   void linkObject(const shared_ptr<NetObj>& object) { addChild(object); object->addParent(self()); }
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

   virtual bool showThresholdSummary() const override;
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

   void linkObject(const shared_ptr<NetObj>& object) { addChild(object); object->addParent(self()); }
};

/**
 * Object container
 */
class NXCORE_EXPORTABLE Container : public AbstractContainer, public AutoBindTarget
{
protected:
   typedef AbstractContainer super;

   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;
   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;

public:
   Container() : super(), AutoBindTarget(this) {}
   Container(const TCHAR *pszName, UINT32 dwCategory) : super(pszName, dwCategory), AutoBindTarget(this) {}
   virtual ~Container() {}

   shared_ptr<Container> self() const { return static_pointer_cast<Container>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CONTAINER; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool showThresholdSummary() const override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;
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

   virtual bool showThresholdSummary() const override;
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

   shared_ptr<Rack> self() const { return static_pointer_cast<Rack>(NObject::self()); }

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
   int32_t m_uin;
   ObjectArray<ZoneProxy> *m_proxyNodes;
   BYTE m_proxyAuthKey[ZONE_PROXY_KEY_LENGTH];
	InetAddressIndex *m_idxNodeByAddr;
	InetAddressIndex *m_idxInterfaceByAddr;
	InetAddressIndex *m_idxSubnetByAddr;
	time_t m_lastHealthCheck;
	bool m_lockedForHealthCheck;

   virtual void fillMessageInternal(NXCPMessage *msg, UINT32 userId) override;
   virtual UINT32 modifyFromMessageInternal(NXCPMessage *request) override;

   void updateProxyLoadData(shared_ptr<Node> node);
   void migrateProxyLoad(ZoneProxy *source, ZoneProxy *target);

public:
   Zone();
   Zone(int32_t uin, const TCHAR *name);
   virtual ~Zone();

   shared_ptr<Zone> self() const { return static_pointer_cast<Zone>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_ZONE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool showThresholdSummary() const override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) const override;

   virtual json_t *toJson() override;

   int32_t getUIN() const { return m_uin; }

   UINT32 getProxyNodeId(NetObj *object, bool backup = false);
	bool isProxyNode(UINT32 nodeId) const;
	UINT32 getProxyNodeAssignments(UINT32 nodeId) const;
   bool isProxyNodeAvailable(UINT32 nodeId) const;
	IntegerArray<UINT32> *getAllProxyNodes() const;
	void fillAgentConfigurationMessage(NXCPMessage *msg) const;

   shared_ptr<AgentConnectionEx> acquireConnectionToProxy(bool validate = false);

   void addProxy(const Node& node);
   void updateProxyStatus(const shared_ptr<Node>& node, bool activeMode);

   bool lockForHealthCheck();
   void healthCheck(PollerInfo *poller);

   void addSubnet(const shared_ptr<Subnet>& subnet) { addChild(subnet); subnet->addParent(self()); }
	void addToIndex(const shared_ptr<Subnet>& subnet) { m_idxSubnetByAddr->put(subnet->getIpAddress(), subnet); }
   void addToIndex(const shared_ptr<Interface>& iface) { m_idxInterfaceByAddr->put(iface->getIpAddressList(), iface); }
   void addToIndex(const InetAddress& addr, const shared_ptr<Interface>& iface) { m_idxInterfaceByAddr->put(addr, iface); }
	void addToIndex(const shared_ptr<Node>& node) { m_idxNodeByAddr->put(node->getIpAddress(), node); }
   void addToIndex(const InetAddress& addr, const shared_ptr<Node>& node) { m_idxNodeByAddr->put(addr, node); }
	void removeFromIndex(const Subnet& subnet) { m_idxSubnetByAddr->remove(subnet.getIpAddress()); }
	void removeFromIndex(const Interface& iface);
   void removeFromInterfaceIndex(const InetAddress& addr) { m_idxInterfaceByAddr->remove(addr); }
	void removeFromIndex(const Node& node) { m_idxNodeByAddr->remove(node.getIpAddress()); }
   void removeFromNodeIndex(const InetAddress& addr) { m_idxNodeByAddr->remove(addr); }
	void updateInterfaceIndex(const InetAddress& oldIp, const InetAddress& newIp, const shared_ptr<Interface>& iface);
   void updateNodeIndex(const InetAddress& oldIp, const InetAddress& newIp, const shared_ptr<Node>& node);
   shared_ptr<Subnet> getSubnetByAddr(const InetAddress& ipAddr) const { return static_pointer_cast<Subnet>(m_idxSubnetByAddr->get(ipAddr)); }
   shared_ptr<Interface> getInterfaceByAddr(const InetAddress& ipAddr) const { return static_pointer_cast<Interface>(m_idxInterfaceByAddr->get(ipAddr)); }
   shared_ptr<Node> getNodeByAddr(const InetAddress& ipAddr) const { return static_pointer_cast<Node>(m_idxNodeByAddr->get(ipAddr)); }
   shared_ptr<Subnet> findSubnet(bool (*comparator)(NetObj *, void *), void *context) const { return static_pointer_cast<Subnet>(m_idxSubnetByAddr->find(comparator, context)); }
   shared_ptr<Interface> findInterface(bool (*comparator)(NetObj *, void *), void *context) const { return static_pointer_cast<Interface>(m_idxInterfaceByAddr->find(comparator, context)); }
   shared_ptr<Node> findNode(bool (*comparator)(NetObj *, void *), void *context) const { return static_pointer_cast<Node>(m_idxNodeByAddr->find(comparator, context)); }
   void forEachSubnet(void (*callback)(const InetAddress& addr, NetObj *, void *), void *context) const { m_idxSubnetByAddr->forEach(callback, context); }
   SharedObjectArray<NetObj> *getSubnets() const { return m_idxSubnetByAddr->getObjects(); }

   void dumpState(ServerConsole *console) const;
   void dumpInterfaceIndex(ServerConsole *console) const;
   void dumpNodeIndex(ServerConsole *console) const;
   void dumpSubnetIndex(ServerConsole *console) const;
};

inline bool Zone::lockForHealthCheck()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated)
   {
      if ((m_status != STATUS_UNMANAGED) &&
          !m_lockedForHealthCheck &&
          ((UINT32)(time(nullptr) - m_lastHealthCheck) >= g_statusPollingInterval))
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

   virtual bool showThresholdSummary() const override;

   void addSubnet(const shared_ptr<Subnet>& subnet) { addChild(subnet); subnet->addParent(self()); }
   void addZone(const shared_ptr<Zone>& zone) { addChild(zone); zone->addParent(self()); }
   void loadFromDatabase(DB_HANDLE hdb);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE StructArray<INPUT_DCI>;
#endif

/**
 * Condition
 */
class NXCORE_EXPORTABLE ConditionObject : public NetObj
{
protected:
   typedef NetObj super;

protected:
   StructArray<INPUT_DCI> m_dciList;
   TCHAR *m_scriptSource;
   NXSL_Program *m_script;
   uint32_t m_activationEventCode;
   uint32_t m_deactivationEventCode;
   uint32_t m_sourceObject;
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

   shared_ptr<ConditionObject> self() const { return static_pointer_cast<ConditionObject>(NObject::self()); }

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
              (static_cast<uint32_t>(time(nullptr) - m_lastPoll) > g_conditionPollingInterval));
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

   virtual bool showThresholdSummary() const override;
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
	uint32_t m_nextElementId;
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
   bool isAllowedOnMap(const shared_ptr<NetObj>& object);

   static bool objectFilter(uint32_t objectId, NetworkMap *map);

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

	DashboardElement() { m_data = nullptr; m_layout = nullptr; }
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

   virtual bool showThresholdSummary() const override;
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

   virtual bool showThresholdSummary() const override;
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

   shared_ptr<SlmCheck> self() const { return static_pointer_cast<SlmCheck>(NObject::self()); }

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
	static INT32 getSecondsSinceBeginningOf(Period period, time_t *beginTime = nullptr);

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

	virtual bool showThresholdSummary() const override;

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

   void linkObject(const shared_ptr<NetObj>& object) { addChild(object); object->addParent(self()); }
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

	void getApplicableTemplates(ServiceContainer *target, SharedObjectArray<SlmCheck> *templates);
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
   shared_ptr<NetObj> object;
   StringList *values;

   ObjectQueryResult(const shared_ptr<NetObj>& _object, StringList *_values) : object(_object)
   {
      values = _values;
   }

   ~ObjectQueryResult()
   {
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
   uint32_t m_id;
   TCHAR m_message[MAX_USER_AGENT_MESSAGE_SIZE];
   IntegerArray<uint32_t> m_objects;
   time_t m_startTime;
   time_t m_endTime;
   bool m_recall;
   bool m_onStartup;
   VolatileCounter m_refCount;
   time_t m_creationTime;
   uint32_t m_creatorId;

public:
   UserAgentNotificationItem(DB_RESULT result, int row);
   UserAgentNotificationItem(const TCHAR *message, const IntegerArray<uint32_t>& objects, time_t startTime, time_t endTime, bool startup, uint32_t userId);
   ~UserAgentNotificationItem() { }

   uint32_t getId() const { return m_id; }
   time_t getEndTime() const { return m_endTime; }
   time_t getStartTime() const { return m_startTime; }
   time_t getCreationTime() const { return m_creationTime; }
   bool isRecalled() const { return m_recall; }

   void recall() { m_recall = true; processUpdate(); };
   void processUpdate();
   bool isApplicable(uint32_t nodeId) const;
   void fillMessage(uint32_t base, NXCPMessage *msg, bool fullInfo = true) const;
   void saveToDatabase();
   void incRefCount() { InterlockedIncrement(&m_refCount); }
   void decRefCount() { InterlockedDecrement(&m_refCount); }
   bool hasNoRef() const { return m_refCount == 0; }

   json_t *toJson() const;
};

/**
 * Functions
 */
void ObjectsInit();

void NXCORE_EXPORTABLE NetObjInsert(const shared_ptr<NetObj>& object, bool newObject, bool importedObject);
void NetObjDeleteFromIndexes(const NetObj& object);

void UpdateInterfaceIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, const shared_ptr<Interface>& iface);
void UpdateNodeIndex(const InetAddress& oldIpAddr, const InetAddress& newIpAddr, const shared_ptr<Node>& node);

void NXCORE_EXPORTABLE MacDbAddAccessPoint(const shared_ptr<AccessPoint>& ap);
void NXCORE_EXPORTABLE MacDbAddInterface(const shared_ptr<Interface>& iface);
void NXCORE_EXPORTABLE MacDbAddObject(const MacAddress& macAddr, const shared_ptr<NetObj>& object);
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr);
void NXCORE_EXPORTABLE MacDbRemove(const MacAddress& macAddr);
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const BYTE *macAddr);
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const MacAddress& macAddr);

shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectById(uint32_t id, int objClass = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByName(const TCHAR *name, int objClass = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByGUID(const uuid& guid, int objClass = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(bool (* comparator)(NetObj *, void *), void *userData, int objClass = -1);
SharedObjectArray<NetObj> NXCORE_EXPORTABLE *FindObjectsByRegex(const TCHAR *regex, int objClass = -1);
const TCHAR NXCORE_EXPORTABLE *GetObjectName(uint32_t id, const TCHAR *defaultName);
shared_ptr<Template> NXCORE_EXPORTABLE FindTemplateByName(const TCHAR *pszName);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, const InetAddress& ipAddr);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, bool allZones, const InetAddress& ipAddr);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByIP(int32_t zoneUIN, const InetAddressList *ipAddrList);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByMAC(const BYTE *macAddr);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByMAC(const MacAddress& macAddr);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByBridgeId(const BYTE *bridgeId);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByLLDPId(const TCHAR *lldpId);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeBySysName(const TCHAR *sysName);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByAgentId(const uuid& agentId);
shared_ptr<Node> NXCORE_EXPORTABLE FindNodeByHardwareId(const NodeHardwareId& hardwareId);
SharedObjectArray<NetObj> NXCORE_EXPORTABLE *FindNodesByHostname(int32_t zoneUIN, const TCHAR *hostname);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByIP(int32_t zoneUIN, const InetAddress& ipAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const BYTE *macAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const MacAddress& macAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByDescription(const TCHAR *description);
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetByIP(int32_t zoneUIN, const InetAddress& ipAddr);
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetForNode(int32_t zoneUIN, const InetAddress& nodeAddr);
bool AdjustSubnetBaseAddress(InetAddress& baseAddr, int32_t zoneUIN);
shared_ptr<MobileDevice> NXCORE_EXPORTABLE FindMobileDeviceByDeviceID(const TCHAR *deviceId);
shared_ptr<AccessPoint> NXCORE_EXPORTABLE FindAccessPointByMAC(const MacAddress& macAddr);
UINT32 NXCORE_EXPORTABLE FindLocalMgmtNode();
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByUIN(int32_t zoneUIN);
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByProxyId(uint32_t proxyId);
int32_t FindUnusedZoneUIN();
bool NXCORE_EXPORTABLE IsClusterIP(int32_t zoneUIN, const InetAddress& ipAddr);
bool NXCORE_EXPORTABLE IsParentObject(uint32_t object1, uint32_t object2);
ObjectArray<ObjectQueryResult> *QueryObjects(const TCHAR *query, uint32_t userId, TCHAR *errorMessage,
         size_t errorMessageLen, const StringList *fields = nullptr, const StringList *orderBy = nullptr,
         uint32_t limit = 0);
StructArray<DependentNode> *GetNodeDependencies(uint32_t nodeId);

BOOL LoadObjects();
void DumpObjects(CONSOLE_CTX pCtx, const TCHAR *filter);

bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(uint32_t userId, int objClass);

void DeleteUserFromAllObjects(UINT32 dwUserId);

bool IsValidParentClass(int childClass, int parentClass);
bool IsEventSource(int objectClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);

PollerInfo *RegisterPoller(PollerType type, const shared_ptr<NetObj>& object, bool objectCreation = false);
void ShowPollers(ServerConsole *console);

void InitUserAgentNotifications();
void DeleteExpiredUserAgentNotifications(DB_HANDLE hdb,UINT32 retentionTime);
void FillUserAgentNotificationsAll(NXCPMessage *msg, Node *node);
UserAgentNotificationItem *CreateNewUserAgentNotification(const TCHAR *message, const IntegerArray<uint32_t>& objects, time_t startTime, time_t endTime, bool onStartup, uint32_t userId);

void DeleteObjectFromPhysicalLinks(UINT32 id);
void DeletePatchPanelFromPhysicalLinks(UINT32 rackId, UINT32 patchPanelId);

shared_ptr<ObjectCategory> NXCORE_EXPORTABLE GetObjectCategory(uint32_t id);
shared_ptr<ObjectCategory> NXCORE_EXPORTABLE FindObjectCategoryByName(const TCHAR *name);
uint32_t ModifyObjectCategory(const NXCPMessage& msg, uint32_t *categoryId);
uint32_t DeleteObjectCategory(uint32_t id, bool forceDelete);
void ObjectCategoriesToMessage(NXCPMessage *msg);
void LoadObjectCategories();

shared_ptr<GeoArea> NXCORE_EXPORTABLE GetGeoArea(uint32_t id);
uint32_t ModifyGeoArea(const NXCPMessage& msg, uint32_t *areaId);
uint32_t DeleteGeoArea(uint32_t id, bool forceDelete);
void GeoAreasToMessage(NXCPMessage *msg);
void LoadGeoAreas();

void FillMessageWithSshKeys(NXCPMessage *msg, bool withPublicKey);
uint32_t GenerateSshKey(const TCHAR *name);
void CreateOrEditSshKey(NXCPMessage *msg);
void DeleteSshKey(NXCPMessage *response, uint32_t id, bool forceDelete);
void FindSshKeyById(uint32_t id, NXCPMessage *msg);
void LoadSshKeys();

/**
 * Global variables
 */
extern shared_ptr<Network> NXCORE_EXPORTABLE g_entireNetwork;
extern shared_ptr<ServiceRoot> NXCORE_EXPORTABLE g_infrastructureServiceRoot;
extern shared_ptr<TemplateRoot> NXCORE_EXPORTABLE g_templateRoot;
extern shared_ptr<NetworkMapRoot> NXCORE_EXPORTABLE g_mapRoot;
extern shared_ptr<DashboardRoot> NXCORE_EXPORTABLE g_dashboardRoot;
extern shared_ptr<BusinessServiceRoot> NXCORE_EXPORTABLE g_businessServiceRoot;

extern UINT32 NXCORE_EXPORTABLE g_dwMgmtNode;
extern BOOL g_bModificationsLocked;
extern ObjectQueue<TemplateUpdateTask> g_templateUpdateQueue;

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
