/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
class Pollable;
class EventReference;

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
extern uint32_t g_autobindPollingInterval;
extern uint32_t g_agentRestartWaitTime;
extern int16_t g_defaultAgentCacheMode;

/**
 * Utility functions used by inline methods
 */
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, uint32_t objectId, const TCHAR *query);
bool NXCORE_EXPORTABLE ExecuteQueryOnObject(DB_HANDLE hdb, const TCHAR *objectId, const TCHAR *query);

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
#define BUILTIN_OID_ASSETROOT             5
#define BUILTIN_OID_NETWORKMAPROOT        6
#define BUILTIN_OID_DASHBOARDROOT         7
#define BUILTIN_OID_BUSINESSSERVICEROOT   9

/**
 * Special object ID meaning "context" or "current object"
 */
#define BUILTIN_OID_CONTEXT_OBJECT        ((uint32_t)0xFFFFFFFF)

/**
 * "All zones" pseudo-ID
 */
#define ALL_ZONES ((int32_t)-1)

class AgentTunnel;
class GenericAgentPolicy;

#ifdef _WIN32
template class NXCORE_EXPORTABLE ObjectArray<GeoLocation>;
template class NXCORE_EXPORTABLE shared_ptr<AgentTunnel>;
#endif


/**
 * Error message max field lenght
 */
#define WEBSVC_ERROR_TEXT_MAX_SIZE 256

/**
 * Web service custom request result data
 */
struct WebServiceCallResult
{
   bool success;
   uint32_t agentErrorCode;
   uint32_t httpResponseCode;
   TCHAR errorMessage[WEBSVC_ERROR_TEXT_MAX_SIZE];
   TCHAR *document;

public:
   WebServiceCallResult();
   ~WebServiceCallResult();
};

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
   shared_ptr<AgentTunnel> m_tunnel;
   shared_ptr<AgentTunnel> m_proxyTunnel;
   ClientSession *m_tcpProxySession;
   int64_t m_dbWriterQueueThreshold;

   virtual shared_ptr<AbstractCommChannel> createChannel() override;
   virtual void onTrap(NXCPMessage *msg) override;
   virtual void onSyslogMessage(const NXCPMessage& msg) override;
   virtual void onWindowsEvent(const NXCPMessage& msg) override;
   virtual void onDataPush(NXCPMessage *msg) override;
   virtual void onFileMonitoringData(NXCPMessage *msg) override;
   virtual void onSnmpTrap(NXCPMessage *pMsg) override;
   virtual void onDisconnect() override;
   virtual void onNotify(NXCPMessage *msg) override;
   virtual uint32_t processCollectedData(NXCPMessage *msg) override;
   virtual uint32_t processBulkCollectedData(NXCPMessage *request, NXCPMessage *response) override;
   virtual bool processCustomMessage(NXCPMessage *msg) override;
   virtual void processTcpProxyData(uint32_t channelId, const void *data, size_t size, bool errorIndicator) override;
   virtual void getSshKeys(NXCPMessage *msg, NXCPMessage *response) override;

public:
   AgentConnectionEx(uint32_t nodeId, const InetAddress& ipAddr, uint16_t port = AGENT_LISTEN_PORT, const TCHAR *secret = nullptr, bool allowCompression = true);
   AgentConnectionEx(uint32_t nodeId, const shared_ptr<AgentTunnel>& tunnel, const TCHAR *secret = nullptr, bool allowCompression = true);
   virtual ~AgentConnectionEx();

   shared_ptr<AgentConnectionEx> self() { return static_pointer_cast<AgentConnectionEx>(AgentConnection::self()); }
   shared_ptr<const AgentConnectionEx> self() const { return static_pointer_cast<const AgentConnectionEx>(AgentConnection::self()); }

   uint32_t deployPolicy(NXCPMessage *msg);
   uint32_t uninstallPolicy(uuid guid, const TCHAR *type, bool newTypeFormatSupported);

   WebServiceCallResult *webServiceCustomRequest(HttpRequestMethod requestMEthod, const TCHAR *url, uint32_t requestTimeout, const TCHAR *login, const TCHAR *password,
         const WebServiceAuthType authType, const StringMap& headers, bool verifyCert, bool verifyHost, bool followLocation, const TCHAR *data);

   void setTunnel(const shared_ptr<AgentTunnel>& tunnel) { m_tunnel = tunnel; }

   using AgentConnection::setProxy;
   void setProxy(const shared_ptr<AgentTunnel>& tunnel, const TCHAR *secret);

   void setTcpProxySession(ClientSession *session);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<AgentConnectionEx>;
#endif

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
protected:
   INDEX_HEAD* volatile m_primary;
   INDEX_HEAD* volatile m_secondary;
   Mutex m_writerLock;
   bool m_owner;
   bool m_startupMode;
   bool m_dirty;
   void (*m_objectDestructor)(void*, AbstractIndexBase*);

   void destroyObject(void *object)
   {
      if (object != nullptr)
         m_objectDestructor(object, this);
   }

   INDEX_HEAD *acquireIndex() const;
   void swapAndWait();

   static ssize_t findElement(INDEX_HEAD *index, uint64_t key);

   void findAll(Array *resultSet, bool (*comparator)(void*, void*), void *data) const;
   void findAll(Array *resultSet, std::function<bool (void*)> comparator) const;

public:
   AbstractIndexBase(Ownership owner);
   AbstractIndexBase(const AbstractIndexBase& src) = delete;
   ~AbstractIndexBase();

   size_t size() const;
   bool put(uint64_t key, void *object);
   void remove(uint64_t key);
   void clear();
   void *get(uint64_t key) const;
   bool contains(uint64_t key) const { return get(key) != nullptr; }
   IntegerArray<uint64_t> keys() const;

   void *find(bool (*comparator)(void*, void*), void *data) const;
   void *find(std::function<bool (void*)> comparator) const;

   void forEach(void (*callback)(void*, void*), void *data) const;
   void forEach(std::function<void (void*)> callback) const;

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
private:
   SynchronizedObjectMemoryPool<shared_ptr<T>> m_pool;

   static void destructor(void *object, AbstractIndexBase *index)
   {
      static_cast<SharedPointerIndex<T>*>(index)->m_pool.destroy(static_cast<shared_ptr<T>*>(object));
   }

   static bool comparatorWrapper(shared_ptr<T> *element, std::pair<bool (*)(T*, void*), void*> *context)
   {
      return context->first(element->get(), context->second);
   }

   static void enumeratorWrapper(shared_ptr<T> *element, std::pair<void (*)(T*, void*), void*> *context)
   {
      context->first(element->get(), context->second);
   }

   static void getAllCallaback(shared_ptr<T> *element, SharedObjectArray<T> *resultSet)
   {
      resultSet->add(*element);
   }

public:
   SharedPointerIndex<T>() : AbstractIndexBase(Ownership::True)
   {
      this->m_objectDestructor = destructor;
   }
   SharedPointerIndex(const SharedPointerIndex& src) = delete;
   ~SharedPointerIndex<T>()
   {
      clear(); // Delete all entries before memory pool is destroyed
   }

   bool put(uint64_t key, T *object)
   {
      return AbstractIndexBase::put(key, new(m_pool.allocate()) shared_ptr<T>(object));
   }

   bool put(uint64_t key, const shared_ptr<T>& object)
   {
      return AbstractIndexBase::put(key, new(m_pool.allocate()) shared_ptr<T>(object));
   }

   shared_ptr<T> get(uint64_t key) const
   {
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::get(key));
      return (v != nullptr) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   unique_ptr<SharedObjectArray<T>> getAll() const
   {
      auto resultSet = make_unique<SharedObjectArray<T>>();
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(getAllCallaback), resultSet.get());
      return resultSet;
   }

   shared_ptr<T> find(bool (*comparator)(T *, void *), void *context) const
   {
      std::pair<bool (*)(T*, void*), void*> wrapperData(comparator, context);
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::find(reinterpret_cast<bool (*)(void*, void*)>(comparatorWrapper), &wrapperData));
      return (v != nullptr) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   template<typename P> shared_ptr<T> find(bool (*comparator)(T *, P *), P *context) const
   {
      return find(reinterpret_cast<bool (*)(T *, void *)>(comparator), (void *)context);
   }

   shared_ptr<T> find(std::function<bool (T*)> comparator) const
   {
      auto v = static_cast<shared_ptr<T>*>(AbstractIndexBase::find([comparator](void *element) { return comparator(static_cast<shared_ptr<T>*>(element)->get()); }));
      return (v != nullptr) ? shared_ptr<T>(*v) : shared_ptr<T>();
   }

   unique_ptr<SharedObjectArray<T>> findAll(bool (*comparator)(T *, void *), void *context) const
   {
      std::pair<bool (*)(T*, void*), void*> wrapperData(comparator, context);
      ObjectArray<shared_ptr<T>> tempResultSet;
      AbstractIndexBase::findAll(&tempResultSet, reinterpret_cast<bool (*)(void*, void*)>(comparatorWrapper), &wrapperData);
      auto resultSet = make_unique<SharedObjectArray<T>>(tempResultSet.size());
      for(int i = 0; i < tempResultSet.size(); i++)
         resultSet->add(*tempResultSet.get(i));
      return resultSet;
   }

   template<typename P> unique_ptr<SharedObjectArray<T>> findAll(bool (*comparator)(T *, P *), P *context) const
   {
      return findAll(reinterpret_cast<bool (*)(T *, void *)>(comparator), (void *)context);
   }

   unique_ptr<SharedObjectArray<T>> findAll(std::function<bool (T*)> comparator) const
   {
      ObjectArray<shared_ptr<T>> tempResultSet;
      AbstractIndexBase::findAll(&tempResultSet, [comparator](void *element) { return comparator(static_cast<shared_ptr<T>*>(element)->get()); });
      auto resultSet = make_unique<SharedObjectArray<T>>(tempResultSet.size());
      for(int i = 0; i < tempResultSet.size(); i++)
         resultSet->add(*tempResultSet.get(i));
      return resultSet;
   }

   void forEach(void (*callback)(T *, void *), void *context) const
   {
      std::pair<void (*)(T*, void*), void*> wrapperData(callback, context);
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(enumeratorWrapper), &wrapperData);
   }

   template<typename P> void forEach(void (*callback)(T *, P *), P *context) const
   {
      std::pair<void (*)(T*, void*), void*> wrapperData(reinterpret_cast<void (*)(T*, void*)>(callback), context);
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(enumeratorWrapper), &wrapperData);
   }

   void forEach(std::function<void (T*)> callback) const
   {
      AbstractIndexBase::forEach([callback](void *element) { callback(static_cast<shared_ptr<T>*>(element)->get()); });
   }
};

/**
 * Abstract index template
 */
template<typename T> class AbstractIndex : public AbstractIndexBase
{
public:
   AbstractIndex<T>(Ownership owner) : AbstractIndexBase(owner) { }
   AbstractIndex(const AbstractIndex& src) = delete;

   bool put(uint64_t key, T *object)
   {
      return AbstractIndexBase::put(key, object);
   }

   T *get(uint64_t key)
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

   T *find(std::function<bool (T*)> comparator)
   {
      return static_cast<T*>(AbstractIndexBase::find(reinterpret_cast<std::function<bool (void*)>>(comparator)));
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

   ObjectArray<T> *findAll(std::function<bool (T*)> comparator)
   {
      ObjectArray<T> *resultSet = new ObjectArray<T>(64, 64, Ownership::False);
      AbstractIndexBase::findAll(resultSet, reinterpret_cast<std::function<bool (void*)>>(comparator));
      return resultSet;
   }

   void forEach(void (*callback)(T *, void *), void *context)
   {
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(callback), context);
   }

   template<typename P> void forEach(void (*callback)(T *, P *), P *context)
   {
      AbstractIndexBase::forEach(reinterpret_cast<void (*)(void*, void*)>(callback), context);
   }

   void forEach(std::function<void (T*)> callback)
   {
      AbstractIndexBase::forEach(reinterpret_cast<std::function<void (void*)>>(callback));
   }
};

/**
 * Abstract index template
 */
template<typename T> class AbstractIndexWithDestructor : public AbstractIndex<T>
{
private:
   static void destructor(void *object, AbstractIndexBase *index)
   {
      delete static_cast<T*>(object);
   }

public:
   AbstractIndexWithDestructor<T>(Ownership owner) : AbstractIndex<T>(owner)
   {
      this->m_objectDestructor = destructor;
   }
   AbstractIndexWithDestructor(const AbstractIndexWithDestructor& src) = delete;
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE SynchronizedObjectMemoryPool<shared_ptr<NetObj>>;
#endif

struct InetAddressIndexEntry;

/**
 * Object index by IP address
 */
class NXCORE_EXPORTABLE InetAddressIndex
{
private:
   InetAddressIndexEntry *m_root;
   RWLock m_lock;

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
   unique_ptr<SharedObjectArray<NetObj>> getObjects(bool (*filter)(NetObj *, void *) = nullptr, void *context = nullptr) const;

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
   Mutex m_writerLock;
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
   SoftwarePackage(const SoftwarePackage& old);
   ~SoftwarePackage();

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   bool saveToDatabase(DB_STATEMENT hStmt) const;

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getVersion() const { return m_version; }
   const TCHAR* getVendor() const { return m_vendor; }
   time_t getDate() const { return m_date; }
   const TCHAR* getUrl() { return m_url; }
   const TCHAR* getDescription() { return m_description; }
   ChangeCode getChangeCode() const { return m_changeCode; }

   void setChangeCode(ChangeCode c) { m_changeCode = c; }

   static SoftwarePackage *createFromTableRow(const Table& table, int row);
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
   uint32_t m_index;
   uint64_t m_capacity;
   TCHAR *m_type;
   TCHAR *m_vendor;
   TCHAR *m_model;
   TCHAR *m_partNumber;
   TCHAR *m_serialNumber;
   TCHAR *m_location;
   TCHAR *m_description;

public:
   HardwareComponent(HardwareComponentCategory category, uint32_t index, const TCHAR *type,
            const TCHAR *vendor, const TCHAR *model, const TCHAR *partNumber, const TCHAR *serialNumber);
   HardwareComponent(DB_RESULT result, int row);
   HardwareComponent(HardwareComponentCategory category, const Table& table, int row);
   HardwareComponent(const HardwareComponent& src);
   ~HardwareComponent();

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   bool saveToDatabase(DB_STATEMENT hStmt) const;

   ChangeCode getChangeCode() const { return m_changeCode; };
   HardwareComponentCategory getCategory() const { return m_category; };
   const TCHAR *getCategoryName() const;
   uint32_t getIndex() const { return m_index; }
   uint64_t getCapacity() const { return m_capacity; }
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
   MacAddress macAddr;
   uint32_t creationFlags;
   uint16_t agentPort;
   uint16_t snmpPort;
   uint16_t eipPort;
   TCHAR name[MAX_OBJECT_NAME];
   uint32_t agentProxyId;
   uint32_t snmpProxyId;
   uint32_t eipProxyId;
   uint32_t mqttProxyId;
   uint32_t icmpProxyId;
   uint32_t sshProxyId;
   uint32_t webServiceProxyId;
   TCHAR sshLogin[MAX_USER_NAME];
   TCHAR sshPassword[MAX_PASSWORD];
   uint16_t sshPort;
   shared_ptr<Cluster> cluster;
   int32_t zoneUIN;
   bool doConfPoll;
   NodeOrigin origin;
   SNMP_SecurityContext *snmpSecurity;
   uuid agentId;

   NewNodeData(const InetAddress& ipAddress);
   NewNodeData(const InetAddress& ipAddress, const MacAddress& macAddress);
   NewNodeData(const NXCPMessage& msg, const InetAddress& ipAddress);
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
#define COLUMN_DEFINITION_BY_DESCRIPTION  0x0004

/**
 * Object modification flags
 */
#define MODIFY_RUNTIME              0x00000000
#define MODIFY_OTHER                0x00000001
#define MODIFY_CUSTOM_ATTRIBUTES    0x00000002
#define MODIFY_DATA_COLLECTION      0x00000004
#define MODIFY_RELATIONS            0x00000008
#define MODIFY_COMMON_PROPERTIES    0x00000010
#define MODIFY_ACCESS_LIST          0x00000020
#define MODIFY_NODE_PROPERTIES      0x00000040
#define MODIFY_INTERFACE_PROPERTIES 0x00000080
#define MODIFY_CLUSTER_RESOURCES    0x00000100
#define MODIFY_MAP_CONTENT          0x00000200
#define MODIFY_SENSOR_PROPERTIES    0x00000400
#define MODIFY_MODULE_DATA          0x00000800
#define MODIFY_POLICY               0x00001000
#define MODIFY_SOFTWARE_INVENTORY   0x00002000
#define MODIFY_HARDWARE_INVENTORY   0x00004000
#define MODIFY_COMPONENTS           0x00008000
#define MODIFY_ICMP_POLL_SETTINGS   0x00010000
#define MODIFY_DC_TARGET            0x00020000
#define MODIFY_BIZSVC_PROPERTIES    0x00040000
#define MODIFY_BIZSVC_CHECKS        0x00080000
#define MODIFY_POLL_TIMES           0x00100000
#define MODIFY_DASHBOARD_LIST       0x00200000
#define MODIFY_OSPF_AREAS           0x00400000
#define MODIFY_OSPF_NEIGHBORS       0x00800000
#define MODIFY_ASSET_PROPERTIES     0x01000080
#define MODIFY_ALL                  0xFFFFFFFF

/**
 * Column definition for DCI summary table
 */
class NXCORE_EXPORTABLE SummaryTableColumn
{
public:
   TCHAR m_name[MAX_DB_STRING];
   TCHAR m_dciName[MAX_PARAM_NAME];
   uint32_t m_flags;
   TCHAR m_separator[16];

   SummaryTableColumn(const NXCPMessage& msg, uint32_t baseId);
   SummaryTableColumn(TCHAR *configStr);

   void createExportRecord(StringBuffer &xml, uint32_t id) const;
};

/**
 * DCI summary table class
 */
class NXCORE_EXPORTABLE SummaryTable
{
private:
   uint32_t m_id;
   uuid m_guid;
   TCHAR m_title[MAX_DB_STRING];
   uint32_t m_flags;
   ObjectArray<SummaryTableColumn> *m_columns;
   TCHAR *m_filterSource;
   NXSL_VM *m_filter;
   AggregationFunction m_aggregationFunction;
   time_t m_periodStart;
   time_t m_periodEnd;
   TCHAR m_menuPath[MAX_DB_STRING];
   TCHAR m_tableDciName[MAX_PARAM_NAME];

   SummaryTable(uint32_t id, DB_RESULT hResult);

public:
   static SummaryTable *loadFromDB(uint32_t id, uint32_t *rcc);

   SummaryTable(const NXCPMessage& msg);
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
   ObjectUrl(const NXCPMessage& msg, uint32_t baseId);
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

#ifdef _WIN32
template class NXCORE_EXPORTABLE ObjectArray<ObjectUrl>;
#endif

/**
 * Poll state information
 */
class NXCORE_EXPORTABLE PollState
{
private:
   VolatileCounter m_pollerCount;
   time_t m_lastCompleted;
   ManualGauge64 m_timer;
   Mutex m_lock;
   bool m_saveNeeded;
   const TCHAR *m_name;

public:
   PollState(const TCHAR *name, bool saveNeeded = false) : m_timer(name, 1, 1000), m_lock(MutexType::FAST)
   {
      m_name = name;
      m_pollerCount = 0;
      m_lastCompleted = TIMESTAMP_NEVER;
      m_saveNeeded = saveNeeded;
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
      m_lock.lock();
      m_lastCompleted = time(nullptr);
      m_timer.update(elapsed);
      InterlockedDecrement(&m_pollerCount);
      m_lock.unlock();
   }

   /**
    * Set last completed time (intended only for reading from database)
    */
   void setLastCompleted(time_t lastCompleted)
   {
      m_lastCompleted = lastCompleted;
   }

   /**
    * Check if save is needed after this poll completion
    */
   bool isSaveNeeded()
   {
      return m_saveNeeded;
   }

   /**
    * Reset poll timer
    */
   void resetTimer()
   {
      m_lock.lock();
      m_timer.reset();
      m_lock.unlock();
   }

   /**
    * Get timestamp of last completed poll
    */
   time_t getLastCompleted()
   {
      m_lock.lock();
      time_t t = m_lastCompleted;
      m_lock.unlock();
      return t;
   }

   /**
    * Get timer average value
    */
   int64_t getTimerAverage()
   {
      m_lock.lock();
      int64_t v = static_cast<int64_t>(m_timer.getAverage());
      m_lock.unlock();
      return v;
   }

   /**
    * Get timer minimum value
    */
   int64_t getTimerMin()
   {
      m_lock.lock();
      int64_t v = m_timer.getMin();
      m_lock.unlock();
      return v;
   }

   /**
    * Get timer maximum value
    */
   int64_t getTimerMax()
   {
      m_lock.lock();
      int64_t v = m_timer.getMax();
      m_lock.unlock();
      return v;
   }

   /**
    * Get last timer value
    */
   int64_t getTimerLast()
   {
      m_lock.lock();
      int64_t v = m_timer.getCurrent();
      m_lock.unlock();
      return v;
   }

   /**
    * Fill NXCP message
    */
   void fillMessage(NXCPMessage *msg, uint32_t baseId)
   {
      msg->setField(baseId, m_name);
      msg->setField(baseId + 1, isPending());
      m_lock.lock();
      msg->setFieldFromTime(baseId + 2, m_lastCompleted);
      msg->setField(baseId + 3, m_timer.getCurrent());
      m_lock.unlock();
   }
};

/**
 * Class to make advanced search
 */
class SearchQuery
{
   StringSet *m_excludedTexts;
   StringSet *m_includedTexts;
   StringObjectMap<StringSet> *m_excludedAttributes;
   StringObjectMap<StringSet> *m_includedAttributes;

public:
   SearchQuery(const String &searchString);
   ~SearchQuery();
   bool match(const SearchAttributeProvider &provider) const;
};

class ObjectIndex;

/**
 * Base class for network objects
 */
class NXCORE_EXPORTABLE NetObj : public NObject
{
   friend class AutoBindTarget;
   friend class Pollable;
   friend class VersionableObject;

private:
   typedef NObject super;
   time_t m_creationTime; //Object creation time

   static void onObjectDeleteCallback(NetObj *object, NetObj *context);

   void getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly) const;

   bool saveTrustedObjects(DB_HANDLE hdb);
   bool saveACLToDB(DB_HANDLE hdb);
   bool saveModuleData(DB_HANDLE hdb);

   void expandScriptMacro(TCHAR *name, const Alarm *alarm, const Event *event, const shared_ptr<DCObjectInfo>& dci, StringBuffer *output);

protected:
   time_t m_timestamp;           // Last change time stamp
   SharedString m_alias;         // Object's alias
   SharedString m_comments;      // User comments
   SharedString m_commentsSource; // User comments with macros
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
   uint32_t m_savedState; // Object state in database
   uint32_t m_stateBeforeMaintenance;
   uint64_t m_maintenanceEventId;
   uint32_t m_maintenanceInitiator;
   VolatileCounter m_modified;
   bool m_isDeleted;
   bool m_isDeleteInitiated;
   bool m_isHidden;
   bool m_isSystem;
   uuid m_mapImage;
   uint32_t m_drilldownObjectId;    // Object that should be opened on drill-down request
   uint32_t m_categoryId;
   Mutex m_mutexProperties;         // Object data access mutex
   GeoLocation m_geoLocation;
   PostalAddress m_postalAddress;
   ClientSession *m_pollRequestor;
   uint32_t m_pollRequestId;
   IntegerArray<uint32_t> m_dashboards; // Dashboards associated with this object
   ObjectArray<ObjectUrl> m_urls;  // URLs associated with this object
   uint32_t m_primaryZoneProxyId;     // ID of assigned primary zone proxy node
   uint32_t m_backupZoneProxyId;      // ID of assigned backup zone proxy node
   uint32_t m_assetId;  // ID of linked asset object

   AccessList m_accessList;
   bool m_inheritAccessRights;
   Mutex m_mutexACL;

   IntegerArray<uint32_t> *m_trustedObjects;

   StringObjectMap<ModuleData> *m_moduleData;
   Mutex m_moduleDataLock;

   StructArray<ResponsibleUser> *m_responsibleUsers;
   Mutex m_mutexResponsibleUsers;

   Pollable* m_asPollable; // Only changed in Pollable class constructor

   const SharedObjectArray<NetObj> &getChildList() const { return reinterpret_cast<const SharedObjectArray<NetObj>&>(super::getChildList()); }
   const SharedObjectArray<NetObj> &getParentList() const { return reinterpret_cast<const SharedObjectArray<NetObj>&>(super::getParentList()); }

   void lockProperties() const { m_mutexProperties.lock(); }
   void unlockProperties() const { m_mutexProperties.unlock(); }
   void lockACL() const { m_mutexACL.lock(); }
   void unlockACL() const { m_mutexACL.unlock(); }
   void lockResponsibleUsersList() const { m_mutexResponsibleUsers.lock(); }
   void unlockResponsibleUsersList() const { m_mutexResponsibleUsers.unlock(); }

   void setModified(uint32_t flags, bool notify = true);                  // Used to mark object as modified

   bool loadACLFromDB(DB_HANDLE hdb);
   bool loadCommonProperties(DB_HANDLE hdb);
   bool loadTrustedObjects(DB_HANDLE hdb);
   bool executeQueryOnObject(DB_HANDLE hdb, const TCHAR *query) { return ExecuteQueryOnObject(hdb, m_id, query); }

   virtual void prepareForDeletion();
   virtual void onObjectDelete(const NetObj& object);

   virtual int getAdditionalMostCriticalStatus();

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId);
   virtual void fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId);
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg);
   virtual uint32_t modifyFromMessageInternalStage2(const NXCPMessage& msg);
   virtual void updateFlags(uint32_t flags, uint32_t mask);

   bool isGeoLocationHistoryTableExists(DB_HANDLE hdb) const;
   bool createGeoLocationHistoryTable(DB_HANDLE hdb);

   void getAllResponsibleUsersInternal(StructArray<ResponsibleUser> *list, const TCHAR *tag) const;

public:
   NetObj();
   NetObj(const NetObj& src) = delete;
   virtual ~NetObj();

   shared_ptr<NetObj> self() { return static_pointer_cast<NetObj>(NObject::self()); }
   shared_ptr<const NetObj> self() const { return static_pointer_cast<const NetObj>(NObject::self()); }

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

   virtual bool getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const override;

   int getStatus() const { return m_status; }
   uint32_t getState() const { return m_state; }
   uint32_t getRuntimeFlags() const { return m_runtimeFlags; }
   uint32_t getFlags() const { return m_flags; }
   int getPropagatedStatus();
   time_t getTimeStamp() const { return m_timestamp; }
   SharedString getAlias() const { return GetAttributeWithLock(m_alias, m_mutexProperties); }
   SharedString getComments() const { return GetAttributeWithLock(m_comments, m_mutexProperties); }
   SharedString getCommentsSource() const { return GetAttributeWithLock(m_commentsSource, m_mutexProperties); }
   SharedString getNameOnMap() const { return GetAttributeWithLock(m_nameOnMap, m_mutexProperties); }

   uint32_t getCategoryId() const { return m_categoryId; }
   void setCategoryId(uint32_t id) { m_categoryId = id; setModified(MODIFY_COMMON_PROPERTIES); }

   const GeoLocation& getGeoLocation() const { return m_geoLocation; }
   void setGeoLocation(const GeoLocation& geoLocation);
   void cleanGeoLocationHistoryTable(DB_HANDLE hdb, int64_t retentionTime);

   PostalAddress getPostalAddress() const { return GetAttributeWithLock(m_postalAddress, m_mutexProperties); }
   void setPostalAddress(const PostalAddress& addr) { lockProperties(); m_postalAddress = addr; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }

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

   bool isTrustedObject(uint32_t id) const;

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
   void markAsSaved() { InterlockedAnd(&m_modified, 0); }

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool saveRuntimeData(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id);
   virtual void linkObjects();
   virtual void cleanup();

   void setId(uint32_t dwId) { m_id = dwId; setModified(MODIFY_ALL); }
   void generateGuid() { m_guid = uuid::generate(); }
   void setName(const TCHAR *name) { lockProperties(); _tcslcpy(m_name, name, MAX_OBJECT_NAME); setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
   void resetStatus() { lockProperties(); m_status = STATUS_UNKNOWN; setModified(MODIFY_RUNTIME); unlockProperties(); }
   void setAlias(const TCHAR *alias);
   void setComments(const TCHAR *comments);
   void expandCommentMacros();
   void setNameOnMap(const TCHAR *name);
   void setCreationTime() { m_creationTime = time(nullptr); }
   time_t getCreationTime() const { return m_creationTime; }

   bool isInMaintenanceMode() const { return m_maintenanceEventId != 0; }
   uint64_t getMaintenanceEventId() const { return m_maintenanceEventId; }
   uint32_t getMaintenanceInitiator() const { return m_maintenanceInitiator; }
   virtual void enterMaintenanceMode(uint32_t userId, const TCHAR *comments);
   virtual void leaveMaintenanceMode(uint32_t userId);

   void fillMessage(NXCPMessage *msg, uint32_t userId);
   uint32_t modifyFromMessage(const NXCPMessage& msg);

   virtual void postModify();

   void commentsToMessage(NXCPMessage *pMsg);

   virtual bool setMgmtStatus(bool bIsManaged);
   virtual void calculateCompoundStatus(bool forcedRecalc = false);

   uint32_t getUserRights(uint32_t userId) const;
   bool checkAccessRights(uint32_t userId, uint32_t requiredRights) const;
   void dropUserAccess(uint32_t userId);

   void addChildNodesToList(SharedObjectArray<Node> *nodeList, uint32_t userId);
   void addChildDCTargetsToList(SharedObjectArray<DataCollectionTarget> *dctList, uint32_t userId);

   uint32_t getAssignedZoneProxyId(bool backup = false) const { return backup ? m_backupZoneProxyId : m_primaryZoneProxyId; }
   void setAssignedZoneProxyId(uint32_t id, bool backup) { if (backup) m_backupZoneProxyId = id; else m_primaryZoneProxyId = id; }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm);

   void executeHookScript(const TCHAR *hookName);

   void setAssetId(uint32_t assetId)  { lockProperties(); m_assetId = assetId; setModified(MODIFY_COMMON_PROPERTIES); unlockProperties(); }
   uint32_t getAssetId() const { return m_assetId; }

   bool addDashboard(uint32_t id);
   bool removeDashboard(uint32_t id);

   ModuleData *getModuleData(const TCHAR *module);
   void setModuleData(const TCHAR *module, ModuleData *data);

   unique_ptr<SharedObjectArray<NetObj>> getParents(int typeFilter = -1) const;
   unique_ptr<SharedObjectArray<NetObj>> getChildren(int typeFilter = -1) const;
   unique_ptr<SharedObjectArray<NetObj>> getAllChildren(bool eventSourceOnly) const;
   int getParentsCount(int typeFilter = -1) const;
   int getChildrenCount(int typeFilter = -1) const;

   shared_ptr<NetObj> findChildObject(const TCHAR *name, int typeFilter) const;
   shared_ptr<Node> findChildNode(const InetAddress& addr) const;

   virtual NXSL_Value *getParentsForNXSL(NXSL_VM *vm);
   virtual NXSL_Value *getChildrenForNXSL(NXSL_VM *vm);

   virtual bool showThresholdSummary() const;
   virtual bool isEventSource() const;
   virtual bool isDataCollectionTarget() const;

   bool isPollable() const { return m_asPollable != nullptr; }
   virtual Pollable *getAsPollable() { return m_asPollable; }

   void setStatusCalculation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);
   void setStatusPropagation(int method, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0);

   void sendPollerMsg(const TCHAR *format, ...);

   StringBuffer expandText(const TCHAR *textTemplate, const Alarm *alarm, const Event *event, const shared_ptr<DCObjectInfo>& dci,
            const TCHAR *userName, const TCHAR *objectName, const TCHAR *instance, const StringMap *inputFields, const StringList *args);

   void updateGeoLocationHistory(GeoLocation location);

   unique_ptr<StructArray<ResponsibleUser>> getAllResponsibleUsers(const TCHAR *tag = nullptr) const;

   virtual json_t *toJson();

   static const WCHAR *getObjectClassNameW(int objectClass);
   static const char *getObjectClassNameA(int objectClass);
#ifdef UNICODE
   static const WCHAR *getObjectClassName(int objectClass) { return getObjectClassNameW(objectClass); }
#else
   static const char *getObjectClassName(int objectClass) { return getObjectClassNameA(objectClass); }
#endif
   static int getObjectClassByNameW(const WCHAR *name);
   static int getObjectClassByNameA(const char *name);
#ifdef UNICODE
   static int getObjectClassByName(const WCHAR *name) { return getObjectClassByNameW(name); }
#else
   static int getObjectClassByName(const char *name) { return getObjectClassByNameA(name); }
#endif
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<NetObj>;
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
   TOPOLOGY = 5,
   ICMP = 6,
   AUTOBIND = 7
};

/**
 * Poller information
 */
class NXCORE_EXPORTABLE PollerInfo
{
   friend PollerInfo *RegisterPoller(PollerType, const shared_ptr<NetObj>&);

private:
   PollerType m_type;
   shared_ptr<NetObj> m_object;
   TCHAR m_status[128];

   PollerInfo(PollerType type, const shared_ptr<NetObj>& object) : m_object(object)
   {
      m_type = type;
      _tcscpy(m_status, _T("awaiting execution"));
   }

public:
   ~PollerInfo();

   PollerType getType() const { return m_type; }
   NetObj *getObject() const { return m_object.get(); }
   const TCHAR *getStatus() const { return m_status; }

   void startExecution() { _tcscpy(m_status, _T("started")); }
   void setStatus(const TCHAR *status) { _tcslcpy(m_status, status, 128); }
};

/**
 * Generic pollable class. Intended to be used only as secondary class in multi-inheritance with primary class derived from NetObj.
 */
class NXCORE_EXPORTABLE Pollable
{
public:
   static constexpr uint32_t NONE               = 0;
   static constexpr uint32_t STATUS             = 0x01;
   static constexpr uint32_t CONFIGURATION      = 0x02;
   static constexpr uint32_t INSTANCE_DISCOVERY = 0x04;
   static constexpr uint32_t TOPOLOGY           = 0x08;
   static constexpr uint32_t ROUTING_TABLE      = 0x10;
   static constexpr uint32_t DISCOVERY          = 0x20;
   static constexpr uint32_t ICMP               = 0x40;
   static constexpr uint32_t AUTOBIND           = 0x80;

protected:
   NetObj* m_this;

   uint32_t m_acceptablePolls;
   PollState m_statusPollState;
   PollState m_configurationPollState;
   PollState m_instancePollState;
   PollState m_discoveryPollState;
   PollState m_topologyPollState;
   PollState m_routingPollState;
   PollState m_icmpPollState;
   PollState m_autobindPollState;

   Mutex m_pollerMutex;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}
   virtual void instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}
   virtual void topologyPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}
   virtual void routingTablePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}
   virtual void icmpPoll(PollerInfo *poller) {}
   virtual void autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) {}

   virtual void startForcedStatusPoll() { m_statusPollState.manualStart(); }
   virtual void startForcedConfigurationPoll() { m_configurationPollState.manualStart(); }
   virtual void startForcedInstanceDiscoveryPoll() { m_instancePollState.manualStart(); }
   virtual void startForcedTopologyPoll() { m_topologyPollState.manualStart(); }
   virtual void startForcedRoutingTablePoll() { m_routingPollState.manualStart(); }
   virtual void startForcedDiscoveryPoll() { m_discoveryPollState.manualStart(); }
   virtual void startForcedAutobindPoll() { m_autobindPollState.manualStart(); }

   void _pollerLock() { m_pollerMutex.lock(); }
   void _pollerUnlock() { m_pollerMutex.unlock(); }

   bool loadFromDatabase(DB_HANDLE hdb, uint32_t id);

   void autoFillAssetProperties();

public:
   Pollable(NetObj *_this, uint32_t acceptablePolls);
   virtual ~Pollable();

   // Status poll
   bool isStatusPollAvailable() const { return (m_acceptablePolls & Pollable::STATUS) != 0; }
   void doForcedStatusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedStatusPoll(PollerInfo *poller) { doForcedStatusPoll(poller, nullptr, 0); }
   void doStatusPoll(PollerInfo *parentPoller, ClientSession *session, uint32_t rqId);
   void doStatusPoll(PollerInfo *poller);
   virtual bool lockForStatusPoll();

   // Configuration poll
   bool isConfigurationPollAvailable() const { return (m_acceptablePolls & Pollable::CONFIGURATION) != 0; }
   void doForcedConfigurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedConfigurationPoll(PollerInfo *poller) { doForcedConfigurationPoll(poller, nullptr, 0); }
   void doConfigurationPoll(PollerInfo *poller);
   virtual bool lockForConfigurationPoll();

   // Instance Discovery
   bool isInstanceDiscoveryPollAvailable() const { return (m_acceptablePolls & Pollable::INSTANCE_DISCOVERY) != 0; }
   void doForcedInstanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedInstanceDiscoveryPoll(PollerInfo *poller) { doForcedInstanceDiscoveryPoll(poller, nullptr, 0); }
   void doInstanceDiscoveryPoll(PollerInfo *poller);
   virtual bool lockForInstanceDiscoveryPoll() { return false; }

   // Topology
   bool isTopologyPollAvailable() const { return (m_acceptablePolls & Pollable::TOPOLOGY) != 0; }
   void doForcedTopologyPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedTopologyPoll(PollerInfo *poller) { doForcedTopologyPoll(poller, nullptr, 0); }
   void doTopologyPoll(PollerInfo *poller);
   virtual bool lockForTopologyPoll() { return false; }

   // Routing Table
   bool isRoutingTablePollAvailable() const { return (m_acceptablePolls & Pollable::ROUTING_TABLE) != 0; }
   void doForcedRoutingTablePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedRoutingTablePoll(PollerInfo *poller) { doForcedRoutingTablePoll(poller, nullptr, 0); }
   void doRoutingTablePoll(PollerInfo *poller);
   virtual bool lockForRoutingTablePoll() { return false; }

   // Network discovery
   bool isDiscoveryPollAvailable() const { return (m_acceptablePolls & Pollable::DISCOVERY) != 0; }
   void doForcedDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedDiscoveryPoll(PollerInfo *poller) { doForcedDiscoveryPoll(poller, nullptr, 0); }
   void doDiscoveryPoll(PollerInfo *poller);
   virtual bool lockForDiscoveryPoll() { return false; }

   // ICMP
   bool isIcmpPollAvailable() const { return (m_acceptablePolls & Pollable::ICMP) != 0; }
   void doIcmpPoll(PollerInfo *poller);
   virtual bool lockForIcmpPoll() { return false; }

   // Autobind
   bool isAutobindPollAvailable() const { return (m_acceptablePolls & Pollable::AUTOBIND) != 0; }
   void doForcedAutobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId);
   void doForcedAutobindPoll(PollerInfo *poller) { doForcedAutobindPoll(poller, nullptr, 0); }
   void doAutobindPoll(PollerInfo *poller);
   virtual bool lockForAutobindPoll();

   void resetPollTimers();

   DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size);
   bool saveToDatabase(DB_HANDLE hdb);
   void pollStateToMessage(NXCPMessage *msg);
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
   uint32_t getVersion() const { return m_version; }
};

/**
 * Number of filters in auto bind target
 */
#define MAX_AUTOBIND_TARGET_FILTERS    2

/**
 * Generic auto-bind target. Intended to be used only as secondary class in multi-inheritance with primary class derived from NetObj.
 */
class NXCORE_EXPORTABLE AutoBindTarget
{
private:
   NetObj *m_this;
   Mutex m_mutexProperties;

   void internalLock() const { m_mutexProperties.lock(); }
   void internalUnlock() const { m_mutexProperties.unlock(); }

protected:
   TCHAR *m_autoBindFilterSources[MAX_AUTOBIND_TARGET_FILTERS];
   NXSL_Program *m_autoBindFilters[MAX_AUTOBIND_TARGET_FILTERS];
   uint32_t m_autoBindFlags;

   void modifyFromMessage(const NXCPMessage& msg);
   void fillMessage(NXCPMessage* msg) const;

   bool loadFromDatabase(DB_HANDLE hdb, uint32_t objectId);
   bool saveToDatabase(DB_HANDLE hdb);
   bool deleteFromDatabase(DB_HANDLE hdb);
   void updateFromImport(const ConfigEntry& config, bool defaultAutoBindFlag);

   void toJson(json_t *root);
   void createExportRecord(StringBuffer &str);

public:
   AutoBindTarget(NetObj *_this);
   virtual ~AutoBindTarget();

   AutoBindDecision isApplicable(NXSL_VM **cachedFilterVM, const shared_ptr<NetObj>& object, const shared_ptr<DCObject>& dci, int filterNumber, NetObj *pollContext = nullptr) const;
   AutoBindDecision isApplicable(NXSL_VM **cachedFilterVM, const shared_ptr<NetObj>& object, NetObj *pollContext = nullptr) const
   {
      return isApplicable(cachedFilterVM, object, shared_ptr<DCObject>(), 0, pollContext);
   }

   uint32_t getAutoBindFlags() const { return m_autoBindFlags; }
   bool isAutoBindEnabled(int filterNumber = 0) const { return (filterNumber >= 0) && (filterNumber < MAX_AUTOBIND_TARGET_FILTERS) && (m_autoBindFlags & (AAF_AUTO_APPLY_1 << filterNumber * 2)); }
   bool isAutoUnbindEnabled(int filterNumber = 0) const { return isAutoBindEnabled(filterNumber) && (m_autoBindFlags & (AAF_AUTO_REMOVE_1 << filterNumber * 2)); }
   const TCHAR *getAutoBindFilterSource(int filterNumber = 0) const { return (filterNumber >= 0) && (filterNumber < MAX_AUTOBIND_TARGET_FILTERS) ? m_autoBindFilterSources[filterNumber] : nullptr; }

   void setAutoBindFilter(int filterNumber, const TCHAR *filter);
   void setAutoBindMode(int filterNumber, bool doBind, bool doUnbind);
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

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<DCObject>;
template class NXCORE_EXPORTABLE ObjectMemoryPool<shared_ptr<DCObject>>;
template class NXCORE_EXPORTABLE SharedObjectArray<DCObject>;
#endif

/**
 * Data collection owner class
 */
class NXCORE_EXPORTABLE DataCollectionOwner : public NetObj
{
private:
   typedef NetObj super;

protected:
   SharedObjectArray<DCObject> m_dcObjects;
   RWLock m_dciAccessLock;
   bool m_dciListModified;
   bool m_instanceDiscoveryChanges;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId) override;

   virtual void onDataCollectionLoad() { }
   virtual void onDataCollectionChange() { }
   virtual void onInstanceDiscoveryChange() { }

   void loadItemsFromDB(DB_HANDLE hdb);
   void destroyItems();
   void updateInstanceDiscoveryItems(DCObject *dci);

   void readLockDciAccess() const { m_dciAccessLock.readLock(); }
   void writeLockDciAccess() { m_dciAccessLock.writeLock(); }
   void unlockDciAccess() const { m_dciAccessLock.unlock(); }

   void deleteChildDCIs(uint32_t dcObjectId);
   void deleteDCObject(DCObject *object);

public:
   DataCollectionOwner();
   DataCollectionOwner(const TCHAR *name, const uuid& guid = uuid::NULL_UUID);
   virtual ~DataCollectionOwner();

   shared_ptr<DataCollectionOwner> self() { return static_pointer_cast<DataCollectionOwner>(NObject::self()); }
   shared_ptr<const DataCollectionOwner> self() const { return static_pointer_cast<const DataCollectionOwner>(NObject::self()); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual void updateFromImport(ConfigEntry *config);

   virtual void getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const;

   virtual json_t *toJson() override;

   int getItemCount() const { return m_dcObjects.size(); }
   bool addDCObject(DCObject *object, bool alreadyLocked = false, bool notify = true);
   bool updateDCObject(uint32_t dcObjectId, const NXCPMessage& msg, uint32_t *numMaps, uint32_t **mapIndex, uint32_t **mapId, uint32_t userId);
   bool deleteDCObject(uint32_t dcObjectId, bool needLock, uint32_t userId);
   bool setItemStatus(const IntegerArray<uint32_t>& dciList, int status, bool userChange = false);
   int updateMultipleDCObjects(const NXCPMessage& request, uint32_t userId);
   shared_ptr<DCObject> getDCObjectById(uint32_t itemId, uint32_t userId, bool lock = true) const;
   shared_ptr<DCObject> getDCObjectByGUID(const uuid& guid, uint32_t userId, bool lock = true) const;
   shared_ptr<DCObject> getDCObjectByTemplateId(uint32_t tmplItemId, uint32_t userId) const;
   shared_ptr<DCObject> getDCObjectByName(const TCHAR *name, uint32_t userId) const;
   shared_ptr<DCObject> getDCObjectByDescription(const TCHAR *description, uint32_t userId) const;
   unique_ptr<SharedObjectArray<DCObject>> getAllDCObjects() const;
   unique_ptr<SharedObjectArray<DCObject>> getDCObjectsByRegex(const TCHAR *regex, bool searchName, uint32_t userId) const;
   NXSL_Value *getAllDCObjectsForNXSL(NXSL_VM *vm, const TCHAR *name, const TCHAR *description, uint32_t userId) const;
   void setDCIModificationFlag() { lockProperties(); m_dciListModified = true; unlockProperties(); }
   void sendItemsToClient(ClientSession *session, uint32_t requestId) const;
   virtual HashSet<uint32_t> *getRelatedEventsList() const;
   unique_ptr<StringSet> getDCIScriptList() const;
   bool isDataCollectionSource(UINT32 nodeId) const;

   virtual void applyDCIChanges(bool forcedChange);
   virtual bool applyToTarget(const shared_ptr<DataCollectionTarget>& target);

   void queueUpdate();
   void queueRemoveFromTarget(uint32_t targetId, bool removeDCI);

   bool enumDCObjects(bool (*callback)(const shared_ptr<DCObject>&, uint32_t, void *), void *context) const;
   void associateItems();
};

#define EXPAND_MACRO 1

/**
 * Data for agent policy deployment task
 */
struct AgentPolicyDeploymentData
{
   shared_ptr<Node> node;
   bool forceInstall;
   uint32_t currVersion;
   uint8_t currHash[MD5_DIGEST_SIZE];
   bool newTypeFormat;
   TCHAR debugId[256];

   AgentPolicyDeploymentData(const shared_ptr<Node>& _node, bool _newTypeFormat) : node(_node)
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
   shared_ptr<Node> node;
   uuid guid;
   TCHAR policyType[MAX_POLICY_TYPE_LEN];
   bool newTypeFormat;
   TCHAR debugId[256];

   AgentPolicyRemovalData(const shared_ptr<Node>& _node, const uuid& _guid, const TCHAR *_type, bool _newTypeFormat) : node(_node), guid(_guid)
   {
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
   Mutex m_contentLock;

   virtual bool createDeploymentMessage(NXCPMessage *msg, char *content, bool newTypeFormatSupported);
   virtual void exportAdditionalData(StringBuffer &xml);
   virtual void importAdditionalData(const ConfigEntry *config);

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
   virtual bool isUsingEvent(uint32_t eventCode) const { return false; }

   virtual json_t *toJson();

   virtual void deploy(shared_ptr<AgentPolicyDeploymentData> data);
   virtual void validate();
};

/**
 * File delivery policy
 */
class NXCORE_EXPORTABLE FileDeliveryPolicy : public GenericAgentPolicy
{
protected:
   virtual void exportAdditionalData(StringBuffer &xml) override;
   virtual void importAdditionalData(const ConfigEntry *config) override;

public:
   FileDeliveryPolicy(const uuid& guid, uint32_t ownerId) : GenericAgentPolicy(guid, _T("FileDelivery"), ownerId) { }
   FileDeliveryPolicy(const TCHAR *name, uint32_t ownerId) : GenericAgentPolicy(name, _T("FileDelivery"), ownerId) { }
   virtual ~FileDeliveryPolicy() { }

   virtual uint32_t modifyFromMessage(const NXCPMessage& request) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual void deploy(shared_ptr<AgentPolicyDeploymentData> data) override;
   virtual void validate() override;
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
   virtual bool isUsingEvent(uint32_t eventCode) const override;
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<GenericAgentPolicy>;
template class NXCORE_EXPORTABLE ObjectMemoryPool<shared_ptr<GenericAgentPolicy>>;
template class NXCORE_EXPORTABLE SharedObjectArray<GenericAgentPolicy>;
#endif

/**
 * Data collection template class
 */
class NXCORE_EXPORTABLE Template : public DataCollectionOwner, public AutoBindTarget, public Pollable, public VersionableObject
{
private:
   typedef DataCollectionOwner super;

protected:
   SharedObjectArray<GenericAgentPolicy> m_policyList;
   SharedObjectArray<GenericAgentPolicy> m_deletedPolicyList;

   virtual void prepareForDeletion() override;
   virtual void onDataCollectionChange() override;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

   void forceDeployPolicies(const shared_ptr<Node>& target);

public:
   Template();
   Template(const TCHAR *name, const uuid& guid = uuid::NULL_UUID);
   ~Template();

   shared_ptr<Template> self() { return static_pointer_cast<Template>(NObject::self()); }
   shared_ptr<const Template> self() const { return static_pointer_cast<const Template>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_TEMPLATE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void applyDCIChanges(bool forcedChange) override;
   virtual bool applyToTarget(const shared_ptr<DataCollectionTarget>& target) override;

   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual void getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const override;

   virtual void updateFromImport(ConfigEntry *config) override;
   virtual json_t *toJson() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

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
   void initiatePolicyValidation();
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
   SharedString m_description;   // Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
   SharedString m_ifAlias;
   uint32_t m_type;
   uint32_t m_mtu;
   uint64_t m_speed;
   uint32_t m_bridgePortNumber;       // 802.1D port number
   InterfacePhysicalLocation m_physicalLocation;
   uint32_t m_peerNodeId;             // ID of peer node object, or 0 if unknown
   uint32_t m_peerInterfaceId;       // ID of peer interface object, or 0 if unknown
   LinkLayerProtocol m_peerDiscoveryProtocol;  // Protocol used to discover peer node
   int16_t m_adminState;             // interface administrative state
   int16_t m_operState;             // interface operational state
   int16_t m_pendingOperState;
   int16_t m_confirmedOperState;
   int16_t m_dot1xPaeAuthState;       // 802.1x port auth state
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
   int16_t m_lastKnownOperState;
   int16_t m_lastKnownAdminState;
   uint32_t m_ospfArea;
   OSPFInterfaceState m_ospfState;
   OSPFInterfaceType m_ospfType;

   void icmpStatusPoll(uint32_t rqId, uint32_t nodeIcmpProxy, Cluster *cluster, InterfaceAdminState *adminState, InterfaceOperState *operState);
   void paeStatusPoll(uint32_t rqId, SNMP_Transport *transport, Node *node);

protected:
   virtual void onObjectDelete(const NetObj& object) override;
   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   void setExpectedStateInternal(int state);

public:
   Interface();
   Interface(const InetAddressList& addrList, int32_t zoneUIN, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *description, uint32_t index, const InetAddressList& addrList, uint32_t ifType, int32_t zoneUIN);
   virtual ~Interface();

   shared_ptr<Interface> self() { return static_pointer_cast<Interface>(NObject::self()); }
   shared_ptr<const Interface> self() const { return static_pointer_cast<const Interface>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_INTERFACE; }
   virtual InetAddress getPrimaryIpAddress() const override { lockProperties(); auto a = m_ipAddressList.getFirstUnicastAddress(); unlockProperties(); return a; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

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
   uint32_t getOSPFArea() const { return m_ospfArea; }
   OSPFInterfaceType getOSPFType() const { return m_ospfType; }
   OSPFInterfaceState getOSPFState() const { return m_ospfState; }
   int getExpectedState() const { return (int)((m_flags & IF_EXPECTED_STATE_MASK) >> 28); }
   int getAdminState() const { return (int)m_adminState; }
   int getOperState() const { return (int)m_operState; }
   int getConfirmedOperState() const { return (int)m_confirmedOperState; }
   int getDot1xPaeAuthState() const { return (int)m_dot1xPaeAuthState; }
   int getDot1xBackendAuthState() const { return (int)m_dot1xBackendAuthState; }
   SharedString getDescription() const { return GetAttributeWithLock(m_description, m_mutexProperties); }
   SharedString getIfAlias() const { return GetAttributeWithLock(m_ifAlias, m_mutexProperties); }
   const MacAddress& getMacAddr() const { return m_macAddr; }
   int getIfTableSuffixLen() const { return m_ifTableSuffixLen; }
   const uint32_t *getIfTableSuffix() const { return m_ifTableSuffix; }
   bool isSyntheticMask() const { return (m_flags & IF_SYNTHETIC_MASK) != 0; }
   bool isPhysicalPort() const { return (m_flags & IF_PHYSICAL_PORT) != 0; }
   bool isLoopback() const { return (m_flags & IF_LOOPBACK) != 0; }
   bool isOSPF() const { return (m_flags & IF_OSPF_INTERFACE) != 0; }
   bool isManuallyCreated() const { return (m_flags & IF_CREATED_MANUALLY) != 0; }
   bool isExcludedFromTopology() const { return (m_flags & (IF_EXCLUDE_FROM_TOPOLOGY | IF_LOOPBACK)) != 0; }
   bool isFake() const
   {
      return (m_index == 1) &&
             (m_type == IFTYPE_OTHER) &&
             !_tcscmp(m_name, _T("unknown"));
   }
   bool isSubInterface() const { return m_parentInterfaceId != 0; }
   bool isPointToPoint() const;
   bool isIncludedInIcmpPoll() const { return (m_flags & IF_INCLUDE_IN_ICMP_POLL) != 0; }
   NXSL_Value *getVlanListForNXSL(NXSL_VM *vm);

   uint64_t getLastDownEventId() const { return m_lastDownEventId; }
   void setLastDownEventId(uint64_t id) { m_lastDownEventId = id; }

   void setMacAddr(const MacAddress& macAddr, bool updateMacDB);
   void setIpAddress(const InetAddress& addr);
   void addIpAddress(const InetAddress& addr);
   void deleteIpAddress(InetAddress addr);
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
      m_description = description;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setIfAlias(const TCHAR* ifAlias)
   {
      lockProperties();
      m_ifAlias = ifAlias;
      setModified(MODIFY_INTERFACE_PROPERTIES);
      unlockProperties();
   }
   void setExcludeFromTopology(bool excluded);
   void setIncludeInIcmpPoll(bool included);
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
   void setIfType(uint32_t type)
   {
      lockProperties();
      m_type = type;
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
   void setExpectedState(int state)
   {
      lockProperties();
      setExpectedStateInternal(state);
      unlockProperties();
   }

   void setOSPFInformation(const OSPFInterface& ospfInterface);
   void clearOSPFInformation();

   void expandName(const TCHAR *originalName, TCHAR *expandedName);

   void updateVlans(IntegerArray<uint32_t> *vlans);
   void updateZoneUIN();

   uint32_t wakeUp();

   void statusPoll(ClientSession *session, uint32_t rqId, ObjectQueue<Event> *eventQueue, Cluster *cluster, SNMP_Transport *snmpTransport, uint32_t nodeIcmpProxy);
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

   virtual void onObjectDelete(const NetObj& object) override;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

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

   void statusPoll(ClientSession *session, uint32_t rqId, const shared_ptr<Node>& pollerNode, ObjectQueue<Event> *eventQueue);

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

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

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
   if (__pollState->isSaveNeeded()) \
      setModified(MODIFY_POLL_TIMES, false);\
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
 * Geolocation control mode for objects
 */
enum GeoLocationControlMode
{
   GEOLOCATION_NO_CONTROL = 0,
   GEOLOCATION_RESTRICTED_AREAS = 1,
   GEOLOCATION_ALLOWED_AREAS = 2
};

/**
 * Common base class for all objects capable of collecting data
 */
class NXCORE_EXPORTABLE DataCollectionTarget : public DataCollectionOwner, public Pollable
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
   uint32_t m_webServiceProxy;
   atomic<double> m_proxyLoadFactor;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual void fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void onDataCollectionLoad() override;
   virtual void onDataCollectionChange() override;
   virtual void onInstanceDiscoveryChange() override;
   virtual bool isDataCollectionDisabled();

   virtual int getAdditionalMostCriticalStatus() override;

   virtual void instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual StringMap *getInstanceList(DCObject *dco);
   void doInstanceDiscovery(uint32_t requestId);
   bool updateInstances(DCObject *root, StringObjectMap<InstanceDiscoveryData> *instances, uint32_t requestId);

   void updateDataCollectionTimeIntervals();

   void updateGeoLocation(const GeoLocation& geoLocation);

   shared_ptr<NetObj> objectFromParameter(const TCHAR *param) const;

   NXSL_VM *runDataCollectionScript(const TCHAR *param, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo);

   void applyTemplates();
   void updateContainerMembership();

   DataCollectionError queryWebService(const TCHAR *param, WebServiceRequestType queryType, TCHAR *buffer, size_t bufSize, StringList *list);

   void getItemDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId);
   void getTableDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId);

   void addProxyDataCollectionElement(ProxyInfo *info, const DCObject *dco, uint32_t primaryProxyId);
   void addProxySnmpTarget(ProxyInfo *info, const Node *node);
   void calculateProxyLoad();

   virtual void collectProxyInfo(ProxyInfo *info);
   static void collectProxyInfoCallback(NetObj *object, void *data);

   bool saveDCIListForCleanup(DB_HANDLE hdb);
   void loadDCIListForCleanup(DB_HANDLE hdb);

public:
   DataCollectionTarget(uint32_t pollableFlags);
   DataCollectionTarget(const TCHAR *name, uint32_t pollableFlags);

   shared_ptr<DataCollectionTarget> self() { return static_pointer_cast<DataCollectionTarget>(NObject::self()); }
   shared_ptr<const DataCollectionTarget> self() const { return static_pointer_cast<const DataCollectionTarget>(NObject::self()); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool isDataCollectionTarget() const override;
   virtual bool isEventSource() const override;

   virtual void enterMaintenanceMode(uint32_t userId, const TCHAR *comments) override;
   virtual void leaveMaintenanceMode(uint32_t userId) override;

   virtual bool lockForInstanceDiscoveryPoll() override;

   virtual DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size);
   virtual DataCollectionError getInternalTable(const TCHAR *name, shared_ptr<Table> *result);

   DataCollectionError getMetricFromScript(const TCHAR *param, TCHAR *buffer, size_t bufSize, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo);
   DataCollectionError getTableFromScript(const TCHAR *param, shared_ptr<Table> *result, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo);
   DataCollectionError getListFromScript(const TCHAR *param, StringList **list, DataCollectionTarget *targetObject, const shared_ptr<DCObjectInfo>& dciInfo);
   DataCollectionError getStringMapFromScript(const TCHAR *param, StringMap **map, DataCollectionTarget *targetObject);

   DataCollectionError getMetricFromWebService(const TCHAR *param, TCHAR *buffer, size_t bufSize);
   DataCollectionError getListFromWebService(const TCHAR *param, StringList **list);

   virtual uint32_t getEffectiveSourceNode(DCObject *dco);

   uint32_t getEffectiveWebServiceProxy();

   NXSL_Array *getTemplatesForNXSL(NXSL_VM *vm);

   uint32_t getTableLastValue(uint32_t dciId, NXCPMessage *msg);
   uint32_t getDciLastValue(uint32_t dciId, NXCPMessage *msg);
   uint32_t getThresholdSummary(NXCPMessage *msg, uint32_t baseId, uint32_t userId);
   uint32_t getPerfTabDCIList(NXCPMessage *msg, uint32_t userId);
   void getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData, uint32_t userId);
   uint32_t getLastValues(NXCPMessage *msg, bool objectTooltipOnly, bool overviewOnly, bool includeNoValueObjects, uint32_t userId);
   void getTooltipLastValues(NXCPMessage *msg, uint32_t userId, uint32_t *index);
   double getProxyLoadFactor() const { return m_proxyLoadFactor.load(); }
   int getDciThreshold(uint32_t dciId);
   void findDcis(const SearchQuery &query, uint32_t userId, SharedObjectArray<DCObject> *result);

   uint64_t getCacheMemoryUsage();

   void updateDciCache();
   void updateDCItemCacheSize(uint32_t dciId);
   void reloadDCItemCache(uint32_t dciId);
   void cleanDCIData(DB_HANDLE hdb);
   void calculateDciCutoffTimes(time_t *cutoffTimeIData, time_t *cutoffTimeTData);
   void queueItemsForPolling();
   bool processNewDCValue(const shared_ptr<DCObject>& dco, time_t currTime, const TCHAR *itemValue, const shared_ptr<Table>& tableValue);
   void scheduleItemDataCleanup(uint32_t dciId);
   void scheduleTableDataCleanup(uint32_t dciId);
   void queuePredictionEngineTraining();

   bool applyTemplateItem(uint32_t templateId, DCObject *dcObject);
   void cleanDeletedTemplateItems(uint32_t templateId, const IntegerArray<uint32_t>& dciList);
   void removeTemplate(Template *templateObject, NetObj *pollTarget);
   void scheduleTemplateRemoval(Template *templateObject, NetObj *pollTarget);
   virtual void onTemplateRemove(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI);

   static void removeTemplate(const shared_ptr<ScheduledTaskParameters>& parameters);
};

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
   time_t timestamp;

   MobileDeviceStatus()
   {
      commProtocol = _T("UNKNOWN");
      altitude = 0;
      speed = -1;
      direction = -1;
      batteryLevel = -1;
      timestamp = 0;
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

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

public:
   MobileDevice();
   MobileDevice(const TCHAR *name, const TCHAR *deviceId);
   virtual ~MobileDevice();

   shared_ptr<MobileDevice> self() { return static_pointer_cast<MobileDevice>(NObject::self()); }
   shared_ptr<const MobileDevice> self() const { return static_pointer_cast<const MobileDevice>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_MOBILEDEVICE; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

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

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;

   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

public:
   AccessPoint();
   AccessPoint(const TCHAR *name, uint32_t index, const MacAddress& macAddr);
   virtual ~AccessPoint();

   shared_ptr<AccessPoint> self() { return static_pointer_cast<AccessPoint>(NObject::self()); }
   shared_ptr<const AccessPoint> self() const { return static_pointer_cast<const AccessPoint>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_ACCESSPOINT; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual int32_t getZoneUIN() const override;

   virtual json_t *toJson() override;

   void statusPollFromController(ClientSession *session, uint32_t rqId, ObjectQueue<Event> *eventQueue, Node *controller, SNMP_Transport *snmpTransport);

   uint32_t getIndex() const { return m_index; }
   const MacAddress& getMacAddr() const { return m_macAddr; }
   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
   bool isMyRadio(uint32_t rfIndex);
   bool isMyRadio(const BYTE *macAddr);
   void getRadioName(uint32_t rfIndex, TCHAR *buffer, size_t bufSize);
   AccessPointState getApState() const { return m_apState; }
   shared_ptr<Node> getParentNode() const;
   String getParentNodeName() const;
   uint32_t getParentNodeId() const { return m_nodeId; }
   bool isIncludedInIcmpPoll() const { return (m_flags & APF_INCLUDE_IN_ICMP_POLL) ? true : false; }
   const TCHAR *getVendor() const { return CHECK_NULL_EX(m_vendor); }
   const TCHAR *getModel() const { return CHECK_NULL_EX(m_model); }
   const TCHAR *getSerialNumber() const { return CHECK_NULL_EX(m_serialNumber); }

   void attachToNode(UINT32 nodeId);
   void setIpAddress(const InetAddress& addr) { lockProperties(); m_ipAddress = addr; setModified(MODIFY_OTHER); unlockProperties(); }
   void updateRadioInterfaces(const ObjectArray<RadioInterfaceInfo> *ri);
   void updateInfo(const TCHAR *vendor, const TCHAR *model, const TCHAR *serialNumber);
   void updateState(AccessPointState state);
};

/**
 * Cluster class
 */
class NXCORE_EXPORTABLE Cluster : public DataCollectionTarget, public AutoBindTarget
{
private:
   typedef DataCollectionTarget super;

protected:
   uint32_t m_clusterType;
   ObjectArray<InetAddress> m_syncNetworks;
   uint32_t m_dwNumResources;
   CLUSTER_RESOURCE *m_pResourceList;
   int32_t m_zoneUIN;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void onDataCollectionChange() override;
   uint32_t getResourceOwnerInternal(uint32_t id, const TCHAR *name);

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

public:
   Cluster();
   Cluster(const TCHAR *pszName, int32_t zoneUIN);
   virtual ~Cluster();

   shared_ptr<Cluster> self() { return static_pointer_cast<Cluster>(NObject::self()); }
   shared_ptr<const Cluster> self() const { return static_pointer_cast<const Cluster>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CLUSTER; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool showThresholdSummary() const override;

   virtual void enterMaintenanceMode(uint32_t userId, const TCHAR *comments) override;
   virtual void leaveMaintenanceMode(uint32_t userId) override;

   virtual void onTemplateRemove(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI) override;
   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;
   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }
   virtual json_t *toJson() override;

   bool isSyncAddr(const InetAddress& addr);
   bool isVirtualAddr(const InetAddress& addr);
   bool isResourceOnNode(uint32_t resourceId, uint32_t nodeId);
   uint32_t getResourceOwner(uint32_t resourceId) { return getResourceOwnerInternal(resourceId, nullptr); }
   uint32_t getResourceOwner(const TCHAR *resourceName) { return getResourceOwnerInternal(0, resourceName); }

   uint32_t collectAggregatedData(DCItem *item, TCHAR *buffer);
   uint32_t collectAggregatedData(DCTable *table, shared_ptr<Table> *result);

   NXSL_Value *getNodesForNXSL(NXSL_VM *vm);
   bool addNode(const shared_ptr<Node>& node);
   void removeNode(const shared_ptr<Node>& node);
   void changeZone(uint32_t newZoneUIN);
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

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void onDataCollectionChange() override;
   virtual void collectProxyInfo(ProxyInfo *info) override;

   void updateRackBinding();
   void updateControllerBinding();

public:
   Chassis();
   Chassis(const TCHAR *name, UINT32 controllerId);
   virtual ~Chassis();

   shared_ptr<Chassis> self() { return static_pointer_cast<Chassis>(NObject::self()); }
   shared_ptr<const Chassis> self() const { return static_pointer_cast<const Chassis>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CHASSIS; }
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;
   virtual bool showThresholdSummary() const override;
   virtual uint32_t getEffectiveSourceNode(DCObject *dco) override;

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
#define SENSOR_PROTO_LORAWAN  1
#define SENSOR_PROTO_DLMS     2

/**
 * Sensor device class
 */
#define SENSOR_CLASS_UNKNOWN        0
#define SENSOR_UPS                  1
#define SENSOR_WATER_METER          2
#define SENSOR_ELECTRICITY_METER    3

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
   int32_t m_signalStrenght; //+1 when no information(cannot be +)
   int32_t m_signalNoise; //*10 from origin number //MAX_INT32 when no value
   uint32_t m_frequency; //*10 from origin number // 0 when no value
   uint32_t m_proxyNodeId;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

   virtual StringMap *getInstanceList(DCObject *dco) override;

   void calculateStatus(BOOL bForcedRecalc = FALSE);

   static bool registerLoraDevice(Sensor *sensor);

   void buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies);

public:
   static shared_ptr<Sensor> create(const TCHAR *name, const NXCPMessage& request);

   Sensor();
   Sensor(const TCHAR *name, const NXCPMessage& request); // Intended for use by create() call only
   virtual ~Sensor();

   shared_ptr<Sensor> self() { return static_pointer_cast<Sensor>(NObject::self()); }
   shared_ptr<const Sensor> self() const { return static_pointer_cast<const Sensor>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_SENSOR; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual void prepareForDeletion() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

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
   DataCollectionError getTableFromAgent(const TCHAR *name, shared_ptr<Table> *table);

   void setProvisoned() { m_state |= SSF_PROVISIONED; }

   virtual json_t *toJson() override;

   shared_ptr<AgentConnectionEx> getAgentConnection();

   void checkDlmsConverterAccessibility();
   void prepareDlmsDciParameters(StringBuffer &parameter);
   void prepareLoraDciParameters(StringBuffer &parameter);

   unique_ptr<NetworkMapObjectList> buildInternalCommunicationTopology();
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

   void setLastConnectTime(time_t t) { m_lastConnect = t; }
   time_t getLastConnectTime() const { return m_lastConnect; }
};

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

   TCHAR *toString(TCHAR *buffer) const { return BinToStr(m_value, HARDWARE_ID_LENGTH, buffer); }
   String toString() const
   {
      TCHAR buffer[HARDWARE_ID_LENGTH * 2 + 1];
      return String(BinToStr(m_value, HARDWARE_ID_LENGTH, buffer));
   }
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<ComponentTree>;
template class NXCORE_EXPORTABLE shared_ptr<NetworkPath>;
template class NXCORE_EXPORTABLE shared_ptr<VlanList>;
template class NXCORE_EXPORTABLE StructArray<OSPFArea>;
template class NXCORE_EXPORTABLE StructArray<OSPFNeighbor>;
#endif

/**
 * Node
 */
class NXCORE_EXPORTABLE Node : public DataCollectionTarget
{
   friend void Sensor::buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology, bool checkAllProxies);

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
   uint32_t m_pollCountSSH;
   uint32_t m_pollCountEtherNetIP;
   uint32_t m_pollCountICMP;
   uint32_t m_requiredPollCount;
   uint32_t m_pollsAfterIpUpdate;
   int32_t m_zoneUIN;
   uint16_t m_agentPort;
   int16_t m_agentCacheMode;
   int16_t m_agentCompressionMode;  // agent compression mode (enabled/disabled/default)
   TCHAR m_agentSecret[MAX_SECRET_LENGTH];
   SNMP_Version m_snmpVersion;
   uint16_t m_snmpPort;
   uint16_t m_nUseIfXTable;
   SNMP_SecurityContext *m_snmpSecurity;
   char m_snmpCodepage[16];
   uuid m_agentId;
   TCHAR *m_agentCertSubject;
   TCHAR m_agentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_platformName[MAX_PLATFORM_NAME_LEN];
   NodeHardwareId m_hardwareId;
   TCHAR *m_snmpObjectId;
   TCHAR *m_sysDescription;   // Agent's System.Uname or SNMP sysDescr
   TCHAR *m_sysName;            // SNMP sysName
   TCHAR *m_sysLocation;      // SNMP sysLocation
   TCHAR *m_sysContact;       // SNMP sysContact
   uint32_t m_ospfRouterId;
   StructArray<OSPFArea> m_ospfAreas;
   StructArray<OSPFNeighbor> m_ospfNeighbors;
   TCHAR *m_lldpNodeId;         // lldpLocChassisId combined with lldpLocChassisIdSubtype, or nullptr for non-LLDP nodes
   ObjectArray<LLDP_LOCAL_PORT_INFO> *m_lldpLocalPortInfo;
   NetworkDeviceDriver *m_driver;
   DriverData *m_driverData;
   ObjectArray<AgentParameterDefinition> *m_agentParameters; // List of metrics supported by agent
   ObjectArray<AgentTableDefinition> *m_agentTables; // List of supported tables
   ObjectArray<AgentParameterDefinition> *m_driverParameters; // List of metrics supported by driver
   time_t m_failTimeAgent;
   time_t m_failTimeSNMP;
   time_t m_failTimeSSH;
   time_t m_failTimeEtherNetIP;
   time_t m_recoveryTime;
   time_t m_downSince;
   time_t m_savedDownSince;
   time_t m_bootTime;
   time_t m_agentUpTime;
   time_t m_lastAgentCommTime;
   time_t m_lastAgentConnectAttempt;
   time_t m_agentRestartTime;
   Mutex m_agentMutex;
   Mutex m_smclpMutex;
   Mutex m_routingTableMutex;
   Mutex m_topologyMutex;
   shared_ptr<AgentConnectionEx> m_agentConnection;
   ProxyAgentConnection *m_proxyConnections;
   VolatileCounter m_pendingDataConfigurationSync;
   SMCLP_Connection *m_smclpConnection;
   uint64_t m_lastAgentTrapId;        // ID of last received agent trap
   uint64_t m_lastAgentPushRequestId; // ID of last received agent push request
   uint32_t m_lastSNMPTrapId;
   uint64_t m_lastSyslogMessageId; // ID of last received syslog message
   uint64_t m_lastWindowsEventId;  // ID of last received Windows event
   uint32_t m_pollerNode;      // Node used for network service polling
   uint32_t m_agentProxy;      // Node used as proxy for agent connection
   uint32_t m_snmpProxy;       // Node used as proxy for SNMP requests
   uint32_t m_eipProxy;        // Node used as proxy for EtherNet/IP requests
   uint32_t m_mqttProxy;       // Node used as proxy for MQTT metrics
   uint32_t m_icmpProxy;       // Node used as proxy for ICMP ping
   uint64_t m_lastEvents[MAX_LAST_EVENTS];
   ObjectArray<RoutingLoopEvent> *m_routingLoopEvents;
   RoutingTable *m_routingTable;
   shared_ptr<NetworkPath> m_lastKnownNetworkPath;
   shared_ptr<ForwardingDatabase> m_fdb;
   shared_ptr<ArpCache> m_arpCache;
   shared_ptr<LinkLayerNeighbors> m_linkLayerNeighbors;
   shared_ptr<VlanList> m_vlans;
   VrrpInfo *m_vrrpInfo;
   ObjectArray<WirelessStationInfo> *m_wirelessStations;
   int m_adoptedApCount;
   int m_totalApCount;
   BYTE m_baseBridgeAddress[MAC_ADDR_LENGTH];   // Bridge base address (dot1dBaseBridgeAddress in bridge MIB)
   shared_ptr<NetworkMapObjectList> m_topology;
   time_t m_topologyRebuildTimestamp;
   shared_ptr<ComponentTree> m_components;      // Hardware components
   ObjectArray<SoftwarePackage> *m_softwarePackages;  // installed software packages
   ObjectArray<HardwareComponent> *m_hardwareComponents;  // installed hardware components
   ObjectArray<WinPerfObject> *m_winPerfObjects;  // Windows performance objects
   shared_ptr<AgentConnection> m_fileUpdateConnection;
   int16_t m_rackHeight;
   int16_t m_rackPosition;
   uint32_t m_physicalContainer;
   uuid m_rackImageFront;
   uuid m_rackImageRear;
   char m_syslogCodepage[16];
   int64_t m_syslogMessageCount;
   int64_t m_snmpTrapCount;
   int64_t m_snmpTrapLastTotal;
   time_t m_snmpTrapStormLastCheckTime;
   uint32_t m_snmpTrapStormActualDuration;
   SharedString m_sshLogin;
   SharedString m_sshPassword;
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
   uint8_t m_cipState;
   uint16_t m_cipVendorCode;

   virtual bool isDataCollectionDisabled() override;
   virtual void collectProxyInfo(ProxyInfo *info) override;

   virtual void prepareForDeletion() override;
   virtual void onObjectDelete(const NetObj& object) override;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;
   virtual void updateFlags(uint32_t flags, uint32_t mask) override;

   virtual void onDataCollectionChange() override;

   virtual StringMap *getInstanceList(DCObject *dco) override;

   virtual bool getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const override;

   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void topologyPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void routingTablePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void icmpPoll(PollerInfo *poller) override;

   virtual void startForcedConfigurationPoll() override;

   void agentLock() { m_agentMutex.lock(); }
   void agentUnlock() { m_agentMutex.unlock(); }

   void smclpLock() { m_smclpMutex.lock(); }
   void smclpUnlock() { m_smclpMutex.unlock(); }

   void routingTableLock() { m_routingTableMutex.lock(); }
   void routingTableUnlock() { m_routingTableMutex.unlock(); }

   void checkOSPFSupport(SNMP_Transport *snmp);
   void addVrrpInterfaces(InterfaceList *ifList);
   bool resolveName(bool useOnlyDNS, const TCHAR* *const facility);
   void setPrimaryIPAddress(const InetAddress& addr);

   bool setAgentProxy(AgentConnectionEx *conn);
   bool isAgentCompressionAllowed();

   bool isAgentRestarting() const { return m_agentRestartTime + g_agentRestartWaitTime >= time(nullptr); }
   bool isProxyAgentRestarting();

   uint32_t getInterfaceCount(Interface **lastInterface);
   void deleteInterface(Interface *iface);

   void checkInterfaceNames(InterfaceList *pIfList);
   shared_ptr<Interface> createInterfaceObject(InterfaceInfo *info, bool manuallyCreated, bool fakeInterface, bool syntheticMask);
   shared_ptr<Subnet> createSubnet(InetAddress& baseAddr, bool syntheticMask);
   void checkAgentPolicyBinding(const shared_ptr<AgentConnectionEx>& conn);
   void updatePrimaryIpAddr();
   bool confPollAgent(uint32_t requestId);
   bool confPollSnmp(uint32_t requestId);
   bool confPollSsh(uint32_t requestId);
   bool confPollEthernetIP(uint32_t requestId);
   NodeType detectNodeType(TCHAR *hypervisorType, TCHAR *hypervisorInfo);
   bool updateSystemHardwareInformation(PollerInfo *poller, uint32_t requestId);
   bool updateHardwareComponents(PollerInfo *poller, uint32_t requestId);
   bool updateSoftwarePackages(PollerInfo *poller, uint32_t requestId);
   bool querySnmpSysProperty(SNMP_Transport *snmp, const TCHAR *oid, const TCHAR *propName, TCHAR **value);
   void checkBridgeMib(SNMP_Transport *pTransport);
   void checkIfXTable(SNMP_Transport *pTransport);
   NetworkPathCheckResult checkNetworkPath(uint32_t requestId);
   NetworkPathCheckResult checkNetworkPathLayer2(uint32_t requestId, bool secondPass);
   NetworkPathCheckResult checkNetworkPathLayer3(uint32_t requestId, bool secondPass);
   NetworkPathCheckResult checkNetworkPathElement(uint32_t nodeId, const TCHAR *nodeType, bool isProxy, bool isSwitch, uint32_t requestId, bool secondPass);
   void icmpPollAddress(AgentConnection *conn, const TCHAR *target, const InetAddress& addr);

   bool checkSshConnection();

   void syncDataCollectionWithAgent(AgentConnectionEx *conn);

   bool updateInterfaceConfiguration(uint32_t requestId);
   bool deleteDuplicateInterfaces(uint32_t requestId);
   void executeInterfaceUpdateHook(Interface *iface);
   void updatePhysicalContainerBinding(uint32_t containerId);
   DuplicateCheckResult checkForDuplicates(shared_ptr<Node> *duplicate, TCHAR *reason, size_t size);
   bool isDuplicateOf(Node *node, TCHAR *reason, size_t size);
   void reconcileWithDuplicateNode(Node *node);

   bool connectToAgent(UINT32 *error = nullptr, UINT32 *socketError = nullptr, bool *newConnection = nullptr, bool forceConnect = false);
   void setLastAgentCommTime() { m_lastAgentCommTime = time(nullptr); }

   void buildInternalCommunicationTopologyInternal(NetworkMapObjectList *topology, uint32_t seedNode, bool agentConnectionOnly, bool checkAllProxies);
   bool checkProxyAndLink(NetworkMapObjectList *topology, uint32_t seedNode, uint32_t proxyId, uint32_t linkType, const TCHAR *linkName, bool checkAllProxies);

   void updateClusterMembership();

public:
   Node();
   Node(const NewNodeData *newNodeData, uint32_t flags);
   virtual ~Node();

   shared_ptr<Node> self() { return static_pointer_cast<Node>(NObject::self()); }
   shared_ptr<const Node> self() const { return static_pointer_cast<const Node>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_NODE; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool saveRuntimeData(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;
   virtual void cleanup() override;

   virtual bool lockForStatusPoll() override;
   virtual bool lockForConfigurationPoll() override;
   virtual bool lockForDiscoveryPoll() override;
   virtual bool lockForRoutingTablePoll() override;
   virtual bool lockForTopologyPoll() override;
   virtual bool lockForIcmpPoll() override;

   void completeDiscoveryPoll(INT64 elapsedTime) { m_discoveryPollState.complete(elapsedTime); }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   shared_ptr<Cluster> getMyCluster();

   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
   MacAddress getPrimaryMacAddress() const
   {
      shared_ptr<Interface> iface = findInterfaceByIP(getPrimaryIpAddress());
      return (iface != nullptr) ? iface->getMacAddr() : MacAddress();
   }

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

   uint32_t getCapabilities() const { return m_capabilities; }
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
   bool isSSHSupported() const { return m_capabilities & NC_IS_SSH ? true : false; }
   bool isNativeAgent() const { return m_capabilities & NC_IS_NATIVE_AGENT ? true : false; }
   bool isEthernetIPSupported() const { return m_capabilities & NC_IS_ETHERNET_IP ? true : false; }
   bool isModbusTCPSupported() const { return m_capabilities & NC_IS_MODBUS_TCP ? true : false; }
   bool isProfiNetSupported() const { return m_capabilities & NC_IS_PROFINET ? true : false; }
   bool isBridge() const { return m_capabilities & NC_IS_BRIDGE ? true : false; }
   bool isRouter() const { return m_capabilities & NC_IS_ROUTER ? true : false; }
   bool isOSPFSupported() const { return m_capabilities & NC_IS_OSPF ? true : false; }
   bool isLLDPSupported() const { return m_capabilities & NC_IS_LLDP ? true : false; }
   bool isLLDPV2MIBSupported() const { return m_capabilities & NC_LLDP_V2_MIB ? true : false; }
   bool isLocalManagement() const { return m_capabilities & NC_IS_LOCAL_MGMT ? true : false; }
   bool isPerVlanFdbSupported() const { return (m_driver != nullptr) ? m_driver->isPerVlanFdbSupported() : false; }
   bool isFdbUsingIfIndex() const { return (m_driver != nullptr) ? m_driver->isFdbUsingIfIndex(this, m_driverData) : false; }
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
   int16_t getRackHeight() const { return m_rackHeight; }
   int16_t getRackPosition() const { return m_rackPosition; }
   bool hasFileUpdateConnection() const { lockProperties(); bool result = (m_fileUpdateConnection != nullptr); unlockProperties(); return result; }
   uint32_t getIcmpProxy() const { return m_icmpProxy; }
   SharedString getSshLogin() const { return GetAttributeWithLock(m_sshLogin, m_mutexProperties); }
   SharedString getSshPassword() const { return GetAttributeWithLock(m_sshPassword, m_mutexProperties); }
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
   uint8_t getCipState() const { return m_cipState; }
   uint16_t getCipVendorCode() const { return m_cipVendorCode; }
   const char *getSyslogCodepage() const { return m_syslogCodepage; }
   const char *getSnmpCodepage() const { return m_snmpCodepage; }
   uint32_t getOSPFRouterId() const { return m_ospfRouterId; }
   StructArray<OSPFArea> getOSPFAreas() const { return GetAttributeWithLock(m_ospfAreas, m_mutexProperties); }
   StructArray<OSPFNeighbor> getOSPFNeighbors() const { return GetAttributeWithLock(m_ospfNeighbors, m_mutexProperties); }

   bool isDown() { return (m_state & DCSF_UNREACHABLE) ? true : false; }
   time_t getDownSince() const { return m_downSince; }

   void setNewTunnelBindFlag() { lockProperties(); m_runtimeFlags |= NDF_NEW_TUNNEL_BIND; unlockProperties(); }
   void clearNewTunnelBindFlag() { lockProperties(); m_runtimeFlags &= ~NDF_NEW_TUNNEL_BIND; unlockProperties(); }

   void setAgentRestartTime() { m_agentRestartTime = time(nullptr); }
   time_t getAgentRestartTime() { return m_agentRestartTime; }

   void addInterface(const shared_ptr<Interface>& iface) { addChild(iface); iface->addParent(self()); }
   shared_ptr<Interface> createNewInterface(InterfaceInfo *ifInfo, bool manuallyCreated, bool fakeInterface);
   shared_ptr<Interface> createNewInterface(const InetAddress& ipAddr, const MacAddress& macAddr, bool fakeInterface);

   void setPrimaryHostName(const TCHAR *name) { lockProperties(); m_primaryHostName = name; unlockProperties(); }
   void setAgentPort(uint16_t port) { m_agentPort = port; }
   void setSnmpPort(uint16_t port) { m_snmpPort = port; }
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

   shared_ptr<ArpCache> getArpCache(bool forceRead = false);
   InterfaceList *getInterfaceList();
   shared_ptr<Interface> findInterfaceByIndex(UINT32 ifIndex) const;
   shared_ptr<Interface> findInterfaceByName(const TCHAR *name) const;
   shared_ptr<Interface> findInterfaceByAlias(const TCHAR *alias) const;
   shared_ptr<Interface> findInterfaceByMAC(const MacAddress& macAddr) const;
   shared_ptr<Interface> findInterfaceByIP(const InetAddress& addr) const;
   shared_ptr<Interface> findInterfaceBySubnet(const InetAddress& subnet) const;
   shared_ptr<Interface> findInterfaceInSameSubnet(const InetAddress& addr) const;
   shared_ptr<Interface> findInterfaceByLocation(const InterfacePhysicalLocation& location) const;
   shared_ptr<Interface> findBridgePort(UINT32 bridgePortNumber) const;
   shared_ptr<AccessPoint> findAccessPointByMAC(const MacAddress& macAddr) const;
   shared_ptr<AccessPoint> findAccessPointByBSSID(const BYTE *bssid) const;
   shared_ptr<AccessPoint> findAccessPointByRadioId(uint32_t rfIndex) const;
   ObjectArray<WirelessStationInfo> *getWirelessStations() const;
   bool isMyIP(const InetAddress& addr) const;
   void getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, uint32_t index, int ifTableSuffixLen, uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState);
   void getInterfaceStatusFromAgent(UINT32 dwIndex, InterfaceAdminState *adminState, InterfaceOperState *operState);
   RoutingTable *getRoutingTable();
   RoutingTable *getCachedRoutingTable() { return m_routingTable; }
   shared_ptr<NetworkPath> getLastKnownNetworkPath() const { return GetAttributeWithLock(m_lastKnownNetworkPath, m_mutexProperties); }
   shared_ptr<LinkLayerNeighbors> getLinkLayerNeighbors() const { return GetAttributeWithLock(m_linkLayerNeighbors, m_topologyMutex); }
   shared_ptr<VlanList> getVlans() const { return GetAttributeWithLock(m_vlans, m_topologyMutex); }
   shared_ptr<ComponentTree> getComponents();

   bool getNextHop(const InetAddress& srcAddr, const InetAddress& destAddr, InetAddress *nextHop, InetAddress *route, uint32_t *ifIndex, bool *isVpn, TCHAR *name);
   bool getOutwardInterface(const InetAddress& destAddr, InetAddress *srcAddr, uint32_t *srcIfIndex);

   bool getLldpLocalPortInfo(uint32_t idType, BYTE *id, size_t idLen, LLDP_LOCAL_PORT_INFO *port);
   void showLLDPInfo(ServerConsole *console);

   void setRecheckCapsFlag() { lockProperties(); m_runtimeFlags |= NDF_RECHECK_CAPABILITIES; unlockProperties(); }
   void resolveVlanPorts(VlanList *vlanList);
   void updateInterfaceNames(ClientSession *pSession, UINT32 dwRqId);
   void checkSubnetBinding();
   AccessPointState getAccessPointState(AccessPoint *ap, SNMP_Transport *snmpTransport, const ObjectArray<RadioInterfaceInfo> *radioInterfaces);

   void forceConfigurationPoll() { lockProperties(); m_runtimeFlags |= ODF_FORCE_CONFIGURATION_POLL; unlockProperties(); }

   virtual bool setMgmtStatus(bool isManaged) override;
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   bool checkAgentTrapId(uint64_t id);
   bool checkSNMPTrapId(uint32_t id);
   bool checkSyslogMessageId(uint64_t id);
   bool checkWindowsEventId(uint64_t id);
   bool checkAgentPushRequestId(UINT64 id);

   bool connectToSMCLP();

   virtual DataCollectionError getInternalMetric(const TCHAR *name, TCHAR *buffer, size_t size) override;
   virtual DataCollectionError getInternalTable(const TCHAR *name, shared_ptr<Table> *result) override;

   DataCollectionError getMetricFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *name, TCHAR *buffer, size_t size, int interpretRawValue);
   DataCollectionError getTableFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, const ObjectArray<DCTableColumn> &columns, shared_ptr<Table> *table);
   DataCollectionError getListFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, StringList **list);
   DataCollectionError getOIDSuffixListFromSNMP(uint16_t port, SNMP_Version version, const TCHAR *oid, StringMap **values);
   DataCollectionError getMetricFromAgent(const TCHAR *name, TCHAR *buffer, size_t size);
   DataCollectionError getTableFromAgent(const TCHAR *name, shared_ptr<Table> *table);
   DataCollectionError getListFromAgent(const TCHAR *name, StringList **list);
   DataCollectionError getMetricFromSMCLP(const TCHAR *name, TCHAR *buffer, size_t size);
   DataCollectionError getMetricFromDeviceDriver(const TCHAR *name, TCHAR *buffer, size_t size);

   double getMetricFromAgentAsDouble(const TCHAR *name, double defaultValue = 0);
   int32_t getMetricFromAgentAsInt32(const TCHAR *name, int32_t defaultValue = 0);
   uint32_t getMetricFromAgentAsUInt32(const TCHAR *name, uint32_t defaultValue = 0);
   int64_t getMetricFromAgentAsInt64(const TCHAR *name, int64_t defaultValue = 0);
   uint64_t getMetricFromAgentAsUInt64(const TCHAR *name, uint64_t defaultValue = 0);

   uint32_t getMetricForClient(int origin, uint32_t userId, const TCHAR *name, TCHAR *buffer, size_t size);
   uint32_t getTableForClient(const TCHAR *name, shared_ptr<Table> *table);

   virtual NXSL_Value *getParentsForNXSL(NXSL_VM *vm) override;
   NXSL_Value *getInterfacesForNXSL(NXSL_VM *vm);
   NXSL_Value *getHardwareComponentsForNXSL(NXSL_VM* vm);
   NXSL_Value* getSoftwarePackagesForNXSL(NXSL_VM* vm);
   NXSL_Value *getOSPFAreasForNXSL(NXSL_VM *vm);
   NXSL_Value *getOSPFNeighborsForNXSL(NXSL_VM *vm);

   ObjectArray<AgentParameterDefinition> *openParamList(int origin);
   void closeParamList() { unlockProperties(); }

   ObjectArray<AgentTableDefinition> *openTableList();
   void closeTableList() { unlockProperties(); }

   shared_ptr<AgentConnectionEx> createAgentConnection(bool sendServerId = false);
   shared_ptr<AgentConnectionEx> getAgentConnection(bool forcePrimary = false);
   shared_ptr<AgentConnectionEx> acquireProxyConnection(ProxyType type, bool validate = false);
   SNMP_Transport *createSnmpTransport(uint16_t port = 0, SNMP_Version version = SNMP_VERSION_DEFAULT, const char *context = nullptr, const char *community = nullptr);
   SNMP_SecurityContext *getSnmpSecurityContext() const;

   uint32_t getEffectiveSnmpProxy(bool backup = false);
   uint32_t getEffectiveEtherNetIPProxy(bool backup = false);
   uint32_t getEffectiveMqttProxy();
   uint32_t getEffectiveSshProxy();
   uint32_t getEffectiveIcmpProxy();
   uint32_t getEffectiveAgentProxy();
   uint32_t getEffectiveTcpProxy();

   void writeParamListToMessage(NXCPMessage *pMsg, int origin, WORD flags);
   void writeWinPerfObjectsToMessage(NXCPMessage *msg);
   void writePackageListToMessage(NXCPMessage *msg);
   void writeWsListToMessage(NXCPMessage *msg);
   void writeHardwareListToMessage(NXCPMessage *msg);
   void writeOSPFDataToMessage(NXCPMessage *msg);

   shared_ptr<Table> wsListAsTable();

   uint32_t wakeUp();

   void addService(const shared_ptr<NetworkService>& service) { addChild(service); service->addParent(self()); }
   UINT32 checkNetworkService(UINT32 *pdwStatus, const InetAddress& ipAddr, int iServiceType, WORD wPort = 0,
                              WORD wProto = 0, TCHAR *pszRequest = nullptr, TCHAR *pszResponse = nullptr, UINT32 *responseTime = nullptr);

   uint64_t getLastEventId(int index) const { return ((index >= 0) && (index < MAX_LAST_EVENTS)) ? m_lastEvents[index] : 0; }
   void setLastEventId(int index, uint64_t eventId) { if ((index >= 0) && (index < MAX_LAST_EVENTS)) m_lastEvents[index] = eventId; }
   void setRoutingLoopEvent(const InetAddress& address, uint32_t nodeId, uint64_t eventId);

   uint32_t callSnmpEnumerate(const TCHAR *rootOid, uint32_t (*handler)(SNMP_Variable*, SNMP_Transport*, void*), void *callerData,
         const char *context = nullptr, bool failOnShutdown = false);
   template<typename T> uint32_t callSnmpEnumerate(const TCHAR *rootOid, uint32_t (*handler)(SNMP_Variable*, SNMP_Transport*, T*),
         T *callerData, const char *context = nullptr, bool failOnShutdown = false)
   {
      return callSnmpEnumerate(rootOid, reinterpret_cast<uint32_t (*)(SNMP_Variable*, SNMP_Transport*, void*)>(handler), callerData, context, failOnShutdown);
   }

   shared_ptr<NetworkMapObjectList> getL2Topology();
   shared_ptr<NetworkMapObjectList> buildL2Topology(uint32_t *status, int radius, bool includeEndNodes, bool useL1Topology);
   shared_ptr<ForwardingDatabase> getSwitchForwardingDatabase() const { return GetAttributeWithLock(m_fdb, m_topologyMutex); }
   shared_ptr<NetObj> findConnectionPoint(UINT32 *localIfId, BYTE *localMacAddr, int *type);
   void addHostConnections(LinkLayerNeighbors *nbs);
   void addExistingConnections(LinkLayerNeighbors *nbs);

   unique_ptr<NetworkMapObjectList> buildInternalCommunicationTopology();

   bool getIcmpStatistics(const TCHAR *target, uint32_t *last, uint32_t *min, uint32_t *max, uint32_t *avg, uint32_t *loss) const;
   DataCollectionError getIcmpStatistic(const TCHAR *param, IcmpStatFunction function, TCHAR *value) const;
   StringList *getIcmpStatCollectors() const;
   void updateIcmpStatisticPeriod(uint32_t period);

   NetworkDeviceDriver *getDriver() const { return m_driver; }
   DriverData *getDriverData() { return m_driverData; }
   void setDriverData(DriverData *data) { m_driverData = data; }

   void incSyslogMessageCount();
   void incSnmpTrapCount();
   bool checkTrapShouldBeProcessed();

   static const TCHAR *typeName(NodeType type);
};

/**
 * Subnet
 */
class NXCORE_EXPORTABLE Subnet : public NetObj
{
private:
   typedef NetObj super;

protected:
   InetAddress m_ipAddress;
   int32_t m_zoneUIN;

   virtual void prepareForDeletion() override;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;

public:
   Subnet();
   Subnet(const InetAddress& addr, int32_t zoneUIN, bool bSyntheticMask);
   Subnet(const TCHAR *name, const InetAddress& addr, int32_t zoneUIN);
   virtual ~Subnet();

   shared_ptr<Subnet> self() { return static_pointer_cast<Subnet>(NObject::self()); }
   shared_ptr<const Subnet> self() const { return static_pointer_cast<const Subnet>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_SUBNET; }
   virtual InetAddress getPrimaryIpAddress() const override { return getIpAddress(); }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual int32_t getZoneUIN() const override { return m_zoneUIN; }

   virtual json_t *toJson() override;

   void addNode(const shared_ptr<Node>& node) { addChild(node); node->addParent(self()); calculateCompoundStatus(true); }

   virtual bool showThresholdSummary() const override;

   InetAddress getIpAddress() const { lockProperties(); auto a = m_ipAddress; unlockProperties(); return a; }
   bool isSyntheticMask() const { return (m_flags & SF_SYNTETIC_MASK) > 0; }
   bool isManuallyCreated() const { return (m_flags & SF_MANUALLY_CREATED) > 0; }
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

   virtual int getObjectClass() const override { return OBJECT_TEMPLATEROOT; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
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

public:
   AbstractContainer();
   AbstractContainer(const TCHAR *pszName, UINT32 dwCategory);
   virtual ~AbstractContainer();

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual void linkObjects() override;

   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual json_t *toJson() override;

   void linkObject(const shared_ptr<NetObj>& object) { addChild(object); object->addParent(self()); }
};

/**
 * Object container
 */
class NXCORE_EXPORTABLE Container : public AbstractContainer, public AutoBindTarget, public Pollable
{
protected:
   typedef AbstractContainer super;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;
   virtual void autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

public:
   Container() : super(), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND) {}
   Container(const TCHAR *pszName, UINT32 dwCategory) : super(pszName, dwCategory), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND) {}
   virtual ~Container() {}

   shared_ptr<Container> self() { return static_pointer_cast<Container>(NObject::self()); }
   shared_ptr<const Container> self() const { return static_pointer_cast<const Container>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CONTAINER; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool showThresholdSummary() const override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;
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
   TemplateGroup(const TCHAR *name) : super(name, 0) { m_status = STATUS_NORMAL; }
   virtual ~TemplateGroup() { }

   virtual int getObjectClass() const override { return OBJECT_TEMPLATEGROUP; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual bool showThresholdSummary() const override;
};

/**
 * Rack element type
 */
enum class RackElementType
{
   PATCH_PANEL = 0,
   FILLER_PANEL = 1,
   ORGANISER = 2,
   PDU = 3
};

/**
 * Passive rack element
 */
class NXCORE_EXPORTABLE RackPassiveElement
{
private:
   uint32_t m_id;
   TCHAR *m_name;
   RackElementType m_type;
   uint16_t m_position;
   uint16_t m_height;
   RackOrientation m_orientation;
   uint32_t m_portCount;
   uuid m_imageFront;
   uuid m_imageRear;

public:
   RackPassiveElement(DB_RESULT hResult, int row);
   RackPassiveElement(const NXCPMessage& request, uint32_t base);
   ~RackPassiveElement()
   {
      MemFree(m_name);
   }

   bool deleteChildren(DB_HANDLE hdb, uint32_t parentId);

   bool saveToDatabase(DB_HANDLE hdb, uint32_t parentId) const;
   void fillMessage(NXCPMessage *pMsg, uint32_t base) const;
   json_t *toJson() const;

   uint32_t getId() const { return m_id; }
   RackElementType getType() const { return m_type; }

   String toString() const;
};

/**
 * Rack object
 */
class NXCORE_EXPORTABLE Rack : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

protected:
   int m_height;   // Rack height in units
   bool m_topBottomNumbering;
   ObjectArray<RackPassiveElement> *m_passiveElements;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void prepareForDeletion() override;

public:
   Rack();
   Rack(const TCHAR *name, int height);
   virtual ~Rack();

   shared_ptr<Rack> self() { return static_pointer_cast<Rack>(NObject::self()); }
   shared_ptr<const Rack> self() const { return static_pointer_cast<const Rack>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_RACK; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   String getRackPasiveElementDescription(uint32_t id);

   virtual json_t *toJson() override;
};

/**
 * Zone proxy information
 */
struct ZoneProxy
{
   uint32_t nodeId;
   bool isAvailable;    // True if proxy is available
   VolatileCounter assignments;  // Number of objects where this proxy is assigned
   int64_t rawDataSenderLoad;
   double cpuLoad;
   double dataCollectorLoad;
   double dataSenderLoad;
   double dataSenderLoadTrend;
   time_t loadBalanceTimestamp;

   ZoneProxy(uint32_t _nodeId)
   {
      nodeId = _nodeId;
      isAvailable = false;
      assignments = 0;
      rawDataSenderLoad = 0;
      cpuLoad = 0;
      dataCollectorLoad = 0;
      dataSenderLoad = 0;
      dataSenderLoadTrend = 0;
      loadBalanceTimestamp = TIMESTAMP_NEVER;
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

#ifdef _WIN32
template class NXCORE_EXPORTABLE SynchronizedObjectMemoryPool<shared_ptr<ZoneProxy>>;
template class NXCORE_EXPORTABLE SharedPointerIndex<ZoneProxy>;
#endif

/**
 * Zone object
 */
class NXCORE_EXPORTABLE Zone : public NetObj, public Pollable
{
protected:
   typedef NetObj super;

protected:
   int32_t m_uin;
   SharedPointerIndex<ZoneProxy> m_proxyNodes;
   BYTE m_proxyAuthKey[ZONE_PROXY_KEY_LENGTH];
   InetAddressIndex *m_idxNodeByAddr;
   InetAddressIndex *m_idxInterfaceByAddr;
   InetAddressIndex *m_idxSubnetByAddr;
   time_t m_lastHealthCheck;
   bool m_lockedForHealthCheck;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual void fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;
   virtual uint32_t modifyFromMessageInternalStage2(const NXCPMessage& msg) override;

   void updateProxyLoadData(shared_ptr<Node> node);
   void migrateProxyLoad(ZoneProxy *source, ZoneProxy *target);
   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

public:
   Zone();
   Zone(int32_t uin, const TCHAR *name);
   virtual ~Zone();

   shared_ptr<Zone> self() { return static_pointer_cast<Zone>(NObject::self()); }
   shared_ptr<const Zone> self() const { return static_pointer_cast<const Zone>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_ZONE; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual bool showThresholdSummary() const override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   virtual json_t *toJson() override;

   int32_t getUIN() const { return m_uin; }

   uint32_t getProxyNodeId(NetObj *object, bool backup = false);
   bool isProxyNode(uint32_t nodeId) const { return m_proxyNodes.contains(nodeId); }
   uint32_t getProxyNodeAssignments(uint32_t nodeId) const;
   bool isProxyNodeAvailable(uint32_t nodeId) const;
   IntegerArray<uint32_t> getAllProxyNodes() const;
   void fillAgentConfigurationMessage(NXCPMessage *msg) const;

   shared_ptr<AgentConnectionEx> acquireConnectionToProxy(bool validate = false);

   void addProxy(const Node& node);
   void updateProxyStatus(const shared_ptr<Node>& node, bool activeMode);

   virtual bool lockForStatusPoll() override;

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
   unique_ptr<SharedObjectArray<NetObj>> getSubnets() const { return m_idxSubnetByAddr->getObjects(); }

   void dumpState(ServerConsole *console) const;
   void dumpInterfaceIndex(ServerConsole *console) const;
   void dumpNodeIndex(ServerConsole *console) const;
   void dumpSubnetIndex(ServerConsole *console) const;
};

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
class NXCORE_EXPORTABLE ConditionObject : public NetObj, public Pollable
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

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual void fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;
   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   void check();

public:
   ConditionObject();
   ConditionObject(bool hidden);
   virtual ~ConditionObject();

   shared_ptr<ConditionObject> self() { return static_pointer_cast<ConditionObject>(NObject::self()); }
   shared_ptr<const ConditionObject> self() const { return static_pointer_cast<const ConditionObject>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_CONDITION; }

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual json_t *toJson() override;

   virtual bool lockForStatusPoll() override;
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   int getCacheSizeForDCI(uint32_t itemId);

   bool isUsingEvent(uint32_t eventCode) const { return (eventCode == m_activationEventCode || eventCode == m_deactivationEventCode); }
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

   virtual int getObjectClass() const override { return OBJECT_NETWORKMAPROOT; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
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
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual bool showThresholdSummary() const override;
};

/**
 * Network map object location
 */
struct NetworkMapObjectLocation
{
   uint32_t objectId;
   int32_t posX;
   int32_t posY;
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE ObjectArray<NetworkMapElement>;
template class NXCORE_EXPORTABLE ObjectArray<NetworkMapLink>;
template class NXCORE_EXPORTABLE StructArray<NetworkMapObjectLocation>;
#endif

/**
 * Network map object
 */
class NXCORE_EXPORTABLE NetworkMap : public NetObj
{
protected:
   typedef NetObj super;

protected:
   int m_mapType;
   IntegerArray<uint32_t> m_seedObjects;
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
   TCHAR *m_filterSource;
   NXSL_VM *m_filter;
   uint32_t m_nextElementId;
   uint32_t m_nextLinkId;
   ObjectArray<NetworkMapElement> m_elements;
   ObjectArray<NetworkMapLink> m_links;
   StructArray<NetworkMapObjectLocation> m_deletedObjects;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   bool updateContent(const shared_ptr<Node>& seed, NetworkMapObjectList *objects);
   void updateObjects(NetworkMapObjectList *objects);
   void updateLinks();
   uint32_t objectIdFromElementId(uint32_t eid);
   uint32_t elementIdFromObjectId(uint32_t eid);

   void setFilter(const TCHAR *filter);
   bool isAllowedOnMap(const shared_ptr<NetObj>& object);

   static bool objectFilter(uint32_t objectId, NetworkMap *map);

public:
   NetworkMap();
   NetworkMap(const NetworkMap &src);
   NetworkMap(int type, const IntegerArray<uint32_t>& seeds);
   virtual ~NetworkMap();

   virtual int getObjectClass() const override { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;
   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;

   virtual void onObjectDelete(const NetObj& object) override;

   virtual json_t *toJson() override;

   void updateContent();
   void clone(const TCHAR *name, const TCHAR *alias);

   int getBackgroundColor() { return m_backgroundColor; }
   void setBackgroundColor(int color) { m_backgroundColor = color; }
};

/**
 * Asset representation
 */
class NXCORE_EXPORTABLE Asset : public NetObj
{
private:
   typedef NetObj super;

   StringMap m_properties;
   uint32_t m_linkedObjectId;

protected:
   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

public:
   Asset();
   Asset(const TCHAR *name, const StringMap& properties);
   virtual ~Asset();

   shared_ptr<Asset> self() { return static_pointer_cast<Asset>(NObject::self()); }
   shared_ptr<const Asset> self() const { return static_pointer_cast<const Asset>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_ASSET; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;
   virtual json_t *toJson() override;

   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
   virtual void prepareForDeletion() override;

   void autoFillProperties();

   void setLinkedObjectId(uint32_t objectId)  { lockProperties(); m_linkedObjectId = objectId; setModified(MODIFY_ASSET_PROPERTIES); unlockProperties(); }
   uint32_t getLinkedObjectId() const { return m_linkedObjectId; }
   SharedString getProperty(const TCHAR *attr);

   std::pair<uint32_t, String> setProperty(const TCHAR *attr, const TCHAR *value);
   uint32_t deleteProperty(const TCHAR *attr);
   void deleteCachedProperty(const TCHAR *attr);

   bool isSamePropertyValue(const TCHAR *attr, const TCHAR *value) const;
   NXSL_Value *getPropertyValueForNXSL(NXSL_VM *vm, const TCHAR *name) const;
   void dumpProperties(ServerConsole *console) const;
};

/**
 * Asset group object
 */
class NXCORE_EXPORTABLE AssetGroup : public AbstractContainer
{
protected:
   typedef AbstractContainer super;

public:
   AssetGroup() : super() { }
   AssetGroup(const TCHAR *name) : super(name, 0) { }

   virtual int getObjectClass() const override { return OBJECT_ASSETGROUP; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual bool showThresholdSummary() const override;
};

/**
 * Asset tree root
 */
class NXCORE_EXPORTABLE AssetRoot : public UniversalRoot
{
protected:
   typedef UniversalRoot super;

public:
   AssetRoot();

   virtual int getObjectClass() const override { return OBJECT_ASSETROOT; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
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

   virtual int getObjectClass() const override { return OBJECT_DASHBOARDROOT; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
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

   DashboardElement()
   {
      m_data = nullptr;
      m_layout = nullptr;
   }
   ~DashboardElement()
   {
      MemFree(m_data);
      MemFree(m_layout);
   }

   json_t *toJson()
   {
      json_t *root = json_object();
      json_object_set_new(root, "type", json_integer(m_type));
      json_object_set_new(root, "data", json_string_t(m_data));
      json_object_set_new(root, "layout", json_string_t(m_layout));
      return root;
   }
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE ObjectArray<DashboardElement>;
#endif

/**
 * Dashboard object
 */
class NXCORE_EXPORTABLE Dashboard : public AbstractContainer, AutoBindTarget, Pollable
{
protected:
   typedef AbstractContainer super;

protected:
   int m_numColumns;
   int m_displayPriority;
   ObjectArray<DashboardElement> m_elements;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

public:
   Dashboard();
   Dashboard(const TCHAR *name);

   shared_ptr<Dashboard> self() { return static_pointer_cast<Dashboard>(NObject::self()); }
   shared_ptr<const Dashboard> self() const { return static_pointer_cast<const Dashboard>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

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

   virtual int getObjectClass() const override { return OBJECT_DASHBOARDGROUP; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;

   virtual bool showThresholdSummary() const override;
};

/**
 * Business service ticket data
 */
struct BusinessServiceTicketData
{
   uint32_t ticketId;
   uint32_t checkId;
   uint32_t serviceId;
   time_t timestamp;
   TCHAR reason[256];
   TCHAR description[1024];
};

/**
 * Business service check type
 */
enum class BusinessServiceCheckType
{
   NONE = 0,
   SCRIPT = 1,
   DCI = 2,
   OBJECT = 3
};

/**
 * Get business service check type from integer
 */
static inline BusinessServiceCheckType BusinessServiceCheckTypeFromInt(int i)
{
   switch(i)
   {
      case 1:
         return BusinessServiceCheckType::SCRIPT;
      case 2:
         return BusinessServiceCheckType::DCI;
      case 3:
         return BusinessServiceCheckType::OBJECT;
      default:
         return BusinessServiceCheckType::NONE;
   }
}

/**
 * Business service check object
 */
class NXCORE_EXPORTABLE BusinessServiceCheck
{
protected:
   uint32_t m_id;
   uint32_t m_serviceId;
   uint32_t m_prototypeServiceId;
   uint32_t m_prototypeCheckId;
   SharedString m_description;
   BusinessServiceCheckType m_type;
   uint32_t m_relatedObject;
   uint32_t m_relatedDCI;
   int m_state;
   int m_statusThreshold;
   TCHAR *m_script;
   NXSL_Program *m_compiledScript;
   TCHAR m_reason[256];
   uint32_t m_currentTicket;
   Mutex m_mutex;

   void insertTicket(const shared_ptr<BusinessServiceTicketData>& ticket);
   void closeTicket();
   void compileScript();

   void lock() const { m_mutex.lock(); }
   void unlock() const { m_mutex.unlock(); }

public:
   BusinessServiceCheck(uint32_t serviceId);
   BusinessServiceCheck(uint32_t serviceId, BusinessServiceCheckType type, uint32_t relatedObject, uint32_t relatedDCI, const TCHAR* description, int threshhold);
   BusinessServiceCheck(uint32_t serviceId, const BusinessServiceCheck& prototype);
   BusinessServiceCheck(DB_RESULT hResult, int row);
   virtual ~BusinessServiceCheck();

   uint32_t getId() const { return m_id; }
   uint32_t getPrototypeServiceId() const { return m_prototypeServiceId; }
   uint32_t getPrototypeCheckId() const { return m_prototypeCheckId; }
   BusinessServiceCheckType getType() const { return m_type; }
   int getState() const { return m_state; }
   const TCHAR *getFailureReason() const { return m_reason; }
   uint32_t getCurrentTicket() const { return m_currentTicket; }
   uint32_t getRelatedObject() const { return m_relatedObject; }
   uint32_t getRelatedDCI() const { return m_relatedDCI; }
   SharedString getDescription() const { return GetStringAttributeWithLock(m_description, m_mutex); }

   int execute(const shared_ptr<BusinessServiceTicketData>& ticket);

   void modifyFromMessage(const NXCPMessage& request);
   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   bool saveToDatabase(DB_HANDLE hdb) const;

   void updateFromPrototype(const BusinessServiceCheck& prototype);
   void setThreshold(int statusThreshold) { m_statusThreshold = statusThreshold; }
};

/**
 * Business service root
 */
class NXCORE_EXPORTABLE BusinessServiceRoot : public UniversalRoot
{
protected:
  typedef UniversalRoot super;

public:
   BusinessServiceRoot();

   virtual int getObjectClass() const override { return OBJECT_BUSINESSSERVICEROOT; }
   virtual void calculateCompoundStatus(bool forcedRecalc = false) override;
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<BusinessServiceCheck>;
template class NXCORE_EXPORTABLE ObjectMemoryPool<shared_ptr<BusinessServiceCheck>>;
template class NXCORE_EXPORTABLE SharedObjectArray<BusinessServiceCheck>;
#endif

/**
 * Business service base class
 */
class NXCORE_EXPORTABLE BaseBusinessService : public AbstractContainer, public AutoBindTarget
{
  typedef AbstractContainer super;

protected:
   SharedObjectArray<BusinessServiceCheck> m_checks;
   IntegerArray<uint32_t> m_deletedChecks;
   bool m_pollingDisabled;
   uint32_t m_objectStatusThreshhold;
   uint32_t m_dciStatusThreshhold;
   Mutex m_checkMutex;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void onCheckModify(const shared_ptr<BusinessServiceCheck>& check);
   virtual void onCheckDelete(uint32_t checkId);

   void checksLock() const { m_checkMutex.lock(); }
   void checksUnlock() const { m_checkMutex.unlock(); }

   bool loadChecksFromDatabase(DB_HANDLE hdb);

public:
   BaseBusinessService(const TCHAR* name);
   BaseBusinessService(const BaseBusinessService& prototype, const TCHAR *name);
   BaseBusinessService();
   virtual ~BaseBusinessService();

   shared_ptr<BaseBusinessService> self() { return static_pointer_cast<BaseBusinessService>(NObject::self()); }
   shared_ptr<const BaseBusinessService> self() const { return static_pointer_cast<const BaseBusinessService>(NObject::self()); }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   unique_ptr<SharedObjectArray<BusinessServiceCheck>> getChecks() const;
   uint32_t getObjectStatusThreshhold() const { return m_objectStatusThreshhold; }
   uint32_t getDciStatusThreshhold() const { return m_dciStatusThreshhold; }

   uint32_t modifyCheckFromMessage(const NXCPMessage& request);
   uint32_t deleteCheck(uint32_t checkId);
};

class BusinessService;

/**
 * Business service prototype
 */
class NXCORE_EXPORTABLE BusinessServicePrototype : public BaseBusinessService, public Pollable
{
   typedef BaseBusinessService super;

protected:
   uint32_t m_instanceDiscoveryMethod;
   uint32_t m_instanceSource;
   TCHAR* m_instanceDiscoveryData;
   TCHAR* m_instanceDiscoveryFilter;
   NXSL_Program *m_compiledInstanceDiscoveryFilter;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;
   virtual uint32_t modifyFromMessageInternalStage2(const NXCPMessage& msg) override;

   virtual void instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

   virtual void onCheckModify(const shared_ptr<BusinessServiceCheck>& check) override;
   virtual void onCheckDelete(uint32_t checkId) override;

   void compileInstanceDiscoveryFilterScript();
   unique_ptr<StringMap> getInstancesFromAgentList();
   unique_ptr<StringMap> getInstancesFromAgentTable();
   unique_ptr<StringMap> getInstancesFromScript();
   unique_ptr<StringMap> getInstances();
   unique_ptr<SharedObjectArray<BusinessService>> getServices();
   void processRelatedServices(std::function<void (BusinessServicePrototype*, BusinessService*)> callback);

public:
   BusinessServicePrototype();
   BusinessServicePrototype(const TCHAR *name, uint32_t method);
   virtual ~BusinessServicePrototype();

   shared_ptr<BusinessServicePrototype> self() { return static_pointer_cast<BusinessServicePrototype>(NObject::self()); }
   shared_ptr<const BusinessServicePrototype> self() const { return static_pointer_cast<const BusinessServicePrototype>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_BUSINESSSERVICEPROTO; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual bool lockForInstanceDiscoveryPoll() override;
};

/**
 * Business service object
 */
class NXCORE_EXPORTABLE BusinessService : public BaseBusinessService, public Pollable
{
   typedef BaseBusinessService super;

protected:
   int m_serviceState; // State of service - operational/degraded/failed
   uint32_t m_prototypeId;
   TCHAR *m_instance;
   Mutex m_stateChangeMutex;

   virtual void fillMessageInternal(NXCPMessage *msg, uint32_t userId) override;
   virtual uint32_t modifyFromMessageInternal(const NXCPMessage& msg) override;

   virtual void configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;
   virtual void statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId) override;

   virtual int getAdditionalMostCriticalStatus() override;

   virtual void prepareForDeletion() override;

   void changeState(int newState);
   void onChildStateChange();

   int getMostCriticalCheckState();

   void validateAutomaticObjectChecks();
   void validateAutomaticDCIChecks();

   void addTicketToParents(shared_ptr<BusinessServiceTicketData> data);
   void addChildTicket(DB_STATEMENT hStmt, const shared_ptr<BusinessServiceTicketData>& data);

   void updateThresholds(int statusThreshold, BusinessServiceCheckType type);

public:
   BusinessService();
   BusinessService(const TCHAR *name);
   BusinessService(const BaseBusinessService& prototype, const TCHAR *name, const TCHAR *instance);
   virtual ~BusinessService();

   shared_ptr<BusinessService> self() { return static_pointer_cast<BusinessService>(NObject::self()); }
   shared_ptr<const BusinessService> self() const { return static_pointer_cast<const BusinessService>(NObject::self()); }

   virtual int getObjectClass() const override { return OBJECT_BUSINESSSERVICE; }

   virtual bool loadFromDatabase(DB_HANDLE hdb, UINT32 id) override;
   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual bool lockForStatusPoll() override;
   virtual bool lockForConfigurationPoll() override;

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;

   int getServiceState() const { return m_serviceState; }
   uint32_t getPrototypeId() const { return m_prototypeId; }
   const TCHAR* getInstance() const { return m_instance; }
   shared_ptr<BusinessService> getParentService() const;

   void updateFromPrototype(const BusinessServicePrototype& prototype);
   void updateCheckFromPrototype(const BusinessServiceCheck& prototype);
   void deleteCheckFromPrototype(uint32_t prototypeCheckId);
};

/**
 * Object index
 */
class NXCORE_EXPORTABLE ObjectIndex : public SharedPointerIndex<NetObj>
{
public:
   ObjectIndex() : SharedPointerIndex<NetObj>() { }
   ObjectIndex(const ObjectIndex& src) = delete;

   unique_ptr<SharedObjectArray<NetObj>> getObjects(bool (*filter)(NetObj*, void*) = nullptr, void *context = nullptr);

   template<typename C>
   unique_ptr<SharedObjectArray<NetObj>> getObjects(bool (*filter)(NetObj*, C*), C *context)
   {
      return getObjects(reinterpret_cast<bool (*)(NetObj*, void*)>(filter), context);
   }

   unique_ptr<SharedObjectArray<NetObj>> getObjects(int classFilter)
   {
      return getObjects([] (NetObj *o, void *c) -> bool
         {
            return o->getObjectClass() == *static_cast<int*>(c);
         }, &classFilter);
   }

   void getObjects(SharedObjectArray<NetObj> *destination, bool (*filter)(NetObj*, void*) = nullptr, void *context = nullptr);

   template<typename C>
   void getObjects(SharedObjectArray<NetObj> *destination, bool (*filter)(NetObj*, C*), C *context)
   {
      getObjects(destination, reinterpret_cast<bool (*)(NetObj*, void*)>(filter), context);
   }
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
   uint32_t nodeId;
   uint32_t dependencyType;
};

/**
 * Input field
 */
struct InputField
{
   TCHAR name[32];
   TCHAR displayName[128];
   uint32_t flags;
   int16_t type;
   int16_t orderNumber;
};

/**
 * Object query result
 */
struct ObjectQueryResult
{
   shared_ptr<NetObj> object;
   StringMap *values;

   ObjectQueryResult(const shared_ptr<NetObj>& _object, StringMap *_values) : object(_object)
   {
      values = _values;
   }

   ~ObjectQueryResult()
   {
      delete values;
   }
};

/**
 * Predefined object query
 */
class ObjectQuery
{
private:
   uint32_t m_id;
   uuid m_guid;
   TCHAR m_name[MAX_OBJECT_NAME];
   String m_description;
   String m_source;
   NXSL_Program *m_script;
   StructArray<InputField> m_inputFields;

   void compile();

public:
   ObjectQuery(const NXCPMessage& msg);
   ObjectQuery(DB_HANDLE hdb, DB_RESULT hResult, int row);
   ~ObjectQuery()
   {
      delete m_script;
   }

   bool saveToDatabase(DB_HANDLE hdb) const;
   bool deleteFromDatabase(DB_HANDLE hdb);

   uint32_t fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   uint32_t getId() const { return m_id; }
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
   const TCHAR *getMessage() const { return m_message; }

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
void NXCORE_EXPORTABLE MacDbRemoveAccessPoint(const AccessPoint& ap);
void NXCORE_EXPORTABLE MacDbRemoveInterface(const Interface& iface);
void NXCORE_EXPORTABLE MacDbRemoveObject(const MacAddress& macAddr, const uint32_t objectId);
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const BYTE *macAddr);
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const MacAddress& macAddr);
const TCHAR * FindVendorByMac(const MacAddress& macAddr);
void FindVendorByMacList(const NXCPMessage& request, NXCPMessage* response);

shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectById(uint32_t id, int objectClassHint = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByName(const TCHAR *name, int objectClassHint = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObjectByGUID(const uuid& guid, int objectClassHint = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(bool (*comparator)(NetObj*, void*), void *context, int objectClassHint = -1);
shared_ptr<NetObj> NXCORE_EXPORTABLE FindObject(std::function<bool (NetObj*)> comparator, int objectClassHint = -1);
unique_ptr<SharedObjectArray<NetObj>> NXCORE_EXPORTABLE FindObjectsByRegex(const TCHAR *regex, int objectClassHint = -1);
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
unique_ptr<SharedObjectArray<NetObj>> NXCORE_EXPORTABLE FindNodesByHostname(int32_t zoneUIN, const TCHAR *hostname);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByIP(int32_t zoneUIN, const InetAddress& ipAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const BYTE *macAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByMAC(const MacAddress& macAddr);
shared_ptr<Interface> NXCORE_EXPORTABLE FindInterfaceByDescription(const TCHAR *description);
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetByIP(int32_t zoneUIN, const InetAddress& ipAddr);
shared_ptr<Subnet> NXCORE_EXPORTABLE FindSubnetForNode(int32_t zoneUIN, const InetAddress& nodeAddr);
bool AdjustSubnetBaseAddress(InetAddress& baseAddr, int32_t zoneUIN);
shared_ptr<MobileDevice> NXCORE_EXPORTABLE FindMobileDeviceByDeviceID(const TCHAR *deviceId);
shared_ptr<AccessPoint> NXCORE_EXPORTABLE FindAccessPointByMAC(const MacAddress& macAddr);
uint32_t NXCORE_EXPORTABLE FindLocalMgmtNode();
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByUIN(int32_t zoneUIN);
shared_ptr<Zone> NXCORE_EXPORTABLE FindZoneByProxyId(uint32_t proxyId);
int32_t FindUnusedZoneUIN();
bool NXCORE_EXPORTABLE IsClusterIP(int32_t zoneUIN, const InetAddress& ipAddr);
bool NXCORE_EXPORTABLE IsParentObject(uint32_t object1, uint32_t object2);
unique_ptr<StructArray<DependentNode>> GetNodeDependencies(uint32_t nodeId);
IntegerArray<uint32_t> CheckSubnetOverlap(const InetAddress &addr, int32_t uin);

unique_ptr<ObjectArray<ObjectQueryResult>> QueryObjects(const TCHAR *query, uint32_t userId, TCHAR *errorMessage, size_t errorMessageLen,
         bool readAllComputedFields = false, const StringList *fields = nullptr, const StringList *orderBy = nullptr,
         const StringMap *inputFields = nullptr, uint32_t limit = 0);
uint32_t GetObjectQueries(NXCPMessage *msg);
uint32_t ModifyObjectQuery(const NXCPMessage& msg, uint32_t *queryId);
uint32_t DeleteObjectQuery(uint32_t queryId);

bool LoadObjects();
void DumpObjects(ServerConsole *console, const TCHAR *filter);

bool NXCORE_EXPORTABLE CreateObjectAccessSnapshot(uint32_t userId, int objClass);

void DeleteUserFromAllObjects(uint32_t userId);

bool IsValidParentClass(int childClass, int parentClass);
bool IsEventSource(int objectClass);
bool IsValidAssetLinkTargetClass(int objectClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);

PollerInfo *RegisterPoller(PollerType type, const shared_ptr<NetObj>& object);
void ShowPollers(ServerConsole *console);

void InitUserAgentNotifications();
void DeleteExpiredUserAgentNotifications(DB_HANDLE hdb,UINT32 retentionTime);
void FillUserAgentNotificationsAll(NXCPMessage *msg, Node *node);
UserAgentNotificationItem *CreateNewUserAgentNotification(const TCHAR *message, const IntegerArray<uint32_t>& objects, time_t startTime, time_t endTime, bool onStartup, uint32_t userId);

void DeleteObjectFromPhysicalLinks(uint32_t id);
void DeletePatchPanelFromPhysicalLinks(uint32_t rackId, uint32_t patchPanelId);
ObjectArray<L1_NEIGHBOR_INFO> GetL1Neighbors(const Node *root);

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
void CreateOrEditSshKey(const NXCPMessage& request);
void DeleteSshKey(NXCPMessage *response, uint32_t id, bool forceDelete);
void FindSshKeyById(uint32_t id, NXCPMessage *msg);
void LoadSshKeys();

double GetServiceUptime(uint32_t serviceId, time_t from, time_t to);
void GetServiceTickets(uint32_t serviceId, time_t from, time_t to, NXCPMessage* msg);

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
extern bool g_modificationsLocked;
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
extern ObjectIndex NXCORE_EXPORTABLE g_idxBusinessServicesById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxSensorById;

//User agent messages
extern Mutex g_userAgentNotificationListMutex;
extern ObjectArray<UserAgentNotificationItem> g_userAgentNotificationList;

#endif   /* _nms_objects_h_ */
