/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: event_forwarder.cpp
**
** Generic pluggable event forwarding framework. Server modules register named driver
** factories; each configured forwarder instance owns an asynchronous queue and a worker
** thread that delivers events to its driver. The built-in "isc" driver forwards events
** to another NetXMS server over ISC/NXCP.
**
**/

#include "nxcore.h"
#include <efdrv.h>
#include <netxms_isc.h>

#define DEBUG_TAG L"event.forwarder"

#define EF_THREAD_KEY      L"EventForwarder"
#define EF_MAX_RETRY       3      // Inline delivery retries before an event is dropped
#define EF_RETRY_DELAY     10     // Seconds between delivery retries

/**
 * Queued event waiting to be forwarded
 */
class ForwardedEvent
{
private:
   Event *m_event;
   String m_recipient;
   shared_ptr<NetObj> m_source;

public:
   ForwardedEvent(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source) : m_recipient(recipient), m_source(source)
   {
      m_event = new Event(event);
   }
   ~ForwardedEvent()
   {
      delete m_event;
   }

   const Event& getEvent() const { return *m_event; }
   const TCHAR *getRecipient() const { return m_recipient.cstr(); }
   const shared_ptr<NetObj>& getSource() const { return m_source; }
};

/**
 * Last known status of forwarding operation
 */
enum class EFSendStatus
{
   UNKNOWN = 0,
   SUCCESS = 1,
   FAILED = 2
};

/**
 * Configured event forwarder instance
 */
class EventForwarder
{
private:
   EventForwarderDriver *m_driver;
   wchar_t m_name[MAX_OBJECT_NAME];
   wchar_t m_description[MAX_NC_DESCRIPTION];
   wchar_t m_driverName[MAX_OBJECT_NAME];
   json_t *m_configuration;
   Mutex m_driverLock;
   THREAD m_workerThread;
   ObjectQueue<ForwardedEvent> m_queue;
   EFSendStatus m_sendStatus;
   bool m_healthCheckStatus;
   bool m_queueOverflow;
   time_t m_lastMessageTime;
   uint32_t m_messageCount;
   uint32_t m_failureCount;
   uint32_t m_droppedCount;
   wchar_t m_errorMessage[MAX_NC_ERROR_MESSAGE];

   void workerThread();

   void setError(const wchar_t *message)
   {
      if ((m_sendStatus != EFSendStatus::FAILED) || (wcscmp(m_errorMessage, message) != 0))
      {
         m_sendStatus = EFSendStatus::FAILED;
         wcslcpy(m_errorMessage, message, MAX_NC_ERROR_MESSAGE);
         NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
      }
   }

   void clearError()
   {
      if (m_sendStatus != EFSendStatus::SUCCESS)
      {
         m_sendStatus = EFSendStatus::SUCCESS;
         m_errorMessage[0] = 0;
         NotifyClientSessions(NX_NOTIFY_EVENT_FORWARDER_CHANGED, 0);
      }
   }

public:
   EventForwarder(EventForwarderDriver *driver, const wchar_t *name, const wchar_t *description,
            const wchar_t *driverName, json_t *configuration, const wchar_t *errorMessage);
   ~EventForwarder();

   void enqueue(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source);

   const wchar_t *getName() const { return m_name; }
   const wchar_t *getDescription() const { return m_description; }
   const wchar_t *getDriverName() const { return m_driverName; }

   void fillMessage(NXCPMessage *msg, uint32_t base) const;
   json_t *toJson(bool includeSensitiveData) const;

   void update(const wchar_t *description, const wchar_t *driverName, json_t *configuration);
   void updateName(const wchar_t *newName) { wcslcpy(m_name, newName, MAX_OBJECT_NAME); }
   void saveToDatabase();
   void checkHealth();

   void requestShutdown() { m_queue.setShutdownMode(); }
   void joinWorker() { ThreadJoin(m_workerThread); m_workerThread = INVALID_THREAD_HANDLE; }
};

/**
 * Event forwarder driver descriptor
 */
struct EventForwarderDriverDescriptor
{
   EventForwarderDriverFactory factory;
   wchar_t name[MAX_OBJECT_NAME];
};

/**
 * Registered drivers and configured forwarders
 */
static StringObjectMap<EventForwarderDriverDescriptor> s_driverList(Ownership::True);
static SharedStringObjectMap<EventForwarder> s_forwarderList;
static Mutex s_forwarderListLock;

/**
 * Event forwarder constructor
 */
EventForwarder::EventForwarder(EventForwarderDriver *driver, const wchar_t *name, const wchar_t *description,
         const wchar_t *driverName, json_t *configuration, const wchar_t *errorMessage) :
            m_driverLock(MutexType::FAST), m_queue(64, Ownership::True)
{
   m_driver = driver;
   wcslcpy(m_name, name, MAX_OBJECT_NAME);
   wcslcpy(m_description, description, MAX_NC_DESCRIPTION);
   wcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
   m_configuration = (configuration != nullptr) ? configuration : json_object();
   wcslcpy(m_errorMessage, CHECK_NULL_EX(errorMessage), MAX_NC_ERROR_MESSAGE);
   m_sendStatus = EFSendStatus::UNKNOWN;
   m_healthCheckStatus = false;
   m_queueOverflow = false;
   m_lastMessageTime = 0;
   m_messageCount = 0;
   m_failureCount = 0;
   m_droppedCount = 0;
   m_workerThread = ThreadCreateEx(this, &EventForwarder::workerThread);
}

/**
 * Event forwarder destructor
 */
EventForwarder::~EventForwarder()
{
   if (m_workerThread != INVALID_THREAD_HANDLE)
   {
      m_queue.setShutdownMode();
      ThreadJoin(m_workerThread);
      m_workerThread = INVALID_THREAD_HANDLE;
   }
   delete m_driver;
   json_decref(m_configuration);
}

/**
 * Event forwarding worker thread
 */
void EventForwarder::workerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Worker thread for event forwarder \"%s\" started", m_name);
   while(true)
   {
      ForwardedEvent *fe = m_queue.getOrBlock();
      if (fe == INVALID_POINTER_VALUE)
         break;

      if (IsShutdownInProgress())
      {
         delete fe;
         continue;
      }

      m_messageCount++;
      m_lastMessageTime = time(nullptr);

      bool success = false;
      for(int attempt = 0; attempt <= EF_MAX_RETRY; attempt++)
      {
         m_driverLock.lock();
         success = (m_driver != nullptr) ? m_driver->forward(fe->getEvent(), fe->getRecipient(), fe->getSource()) : false;
         m_driverLock.unlock();

         if (success)
            break;

         if (m_driver == nullptr)
         {
            setError(L"Driver not initialized");
            break;
         }

         if (attempt < EF_MAX_RETRY)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Delivery failure on event forwarder \"%s\", retrying in %d seconds (attempt %d of %d)",
               m_name, EF_RETRY_DELAY, attempt + 1, EF_MAX_RETRY);
            if (SleepAndCheckForShutdown(EF_RETRY_DELAY))
               break;
         }
      }

      if (success)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"Event successfully forwarded via \"%s\"", m_name);
         clearError();
      }
      else
      {
         m_failureCount++;
         m_droppedCount++;
         if (m_driver != nullptr)
            setError(L"Delivery failure");
         nxlog_debug_tag(DEBUG_TAG, 4, L"Event dropped by forwarder \"%s\" after %d failed attempts (total dropped=%u)",
            m_name, EF_MAX_RETRY + 1, m_droppedCount);
      }

      delete fe;
   }
   nxlog_debug_tag(DEBUG_TAG, 2, L"Worker thread for event forwarder \"%s\" stopped", m_name);
}

/**
 * Enqueue event for forwarding
 */
void EventForwarder::enqueue(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source)
{
   uint32_t maxQueueSize = ConfigReadULong(L"EventForwarders.MaxQueueSize", 1000);
   if ((maxQueueSize > 0) && (m_queue.size() >= maxQueueSize))
   {
      m_droppedCount++;
      if (!m_queueOverflow)
      {
         m_queueOverflow = true;
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
            L"Event forwarder \"%s\" queue size exceeds threshold (size=%u, max=%u), new events will be dropped",
            m_name, static_cast<uint32_t>(m_queue.size()), maxQueueSize);
      }
      return;
   }

   if (m_queueOverflow)
   {
      m_queueOverflow = false;
      nxlog_debug_tag(DEBUG_TAG, 4, L"Event forwarder \"%s\" queue size is below threshold", m_name);
   }

   m_queue.put(new ForwardedEvent(event, recipient, source));
}

/**
 * Fill NXCPMessage with event forwarder data
 */
void EventForwarder::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base, m_name);
   msg->setField(base + 1, m_description);
   msg->setField(base + 2, m_driverName);
   msg->setField(base + 3, m_configuration, true);
   msg->setField(base + 4, m_driver != nullptr);
   msg->setField(base + 5, m_errorMessage);
   msg->setField(base + 6, static_cast<int16_t>(m_sendStatus));
   msg->setField(base + 7, m_healthCheckStatus);
   msg->setFieldFromTime(base + 8, m_lastMessageTime);
   msg->setField(base + 9, m_messageCount);
   msg->setField(base + 10, m_failureCount);
   msg->setField(base + 11, static_cast<uint32_t>(m_queue.size()));
   msg->setField(base + 12, m_droppedCount);
}

/**
 * Convert event forwarder to JSON
 */
json_t *EventForwarder::toJson(bool includeSensitiveData) const
{
   json_t *root = json_object();
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "driverName", json_string_t(m_driverName));
   if (includeSensitiveData)
      json_object_set(root, "configuration", m_configuration);
   json_object_set_new(root, "driverInitialized", json_boolean(m_driver != nullptr));
   json_object_set_new(root, "errorMessage", json_string_t(m_errorMessage));
   json_object_set_new(root, "sendStatus", json_integer(static_cast<int16_t>(m_sendStatus)));
   json_object_set_new(root, "healthCheckStatus", json_boolean(m_healthCheckStatus));
   json_object_set_new(root, "lastMessageTime", json_time_string(m_lastMessageTime));
   json_object_set_new(root, "messageCount", json_integer(m_messageCount));
   json_object_set_new(root, "failureCount", json_integer(m_failureCount));
   json_object_set_new(root, "queueSize", json_integer(m_queue.size()));
   json_object_set_new(root, "droppedCount", json_integer(m_droppedCount));
   return root;
}

/**
 * Create driver instance for given configuration
 */
static EventForwarderDriver *CreateDriverInstance(const wchar_t *name, const wchar_t *driverName, json_t *configuration, StringBuffer *errorMessage)
{
   EventForwarderDriverDescriptor *dd = s_driverList.get(driverName);
   if (dd == nullptr)
   {
      errorMessage->append(L"Cannot find driver ");
      errorMessage->append(driverName);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot find event forwarder driver %s", driverName);
      return nullptr;
   }

   EventForwarderDriver *driver = dd->factory(configuration);
   if (driver == nullptr)
   {
      errorMessage->append(L"Unable to create instance of driver ");
      errorMessage->append(driverName);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Unable to create instance of event forwarder driver %s for forwarder \"%s\"", driverName, name);
   }
   return driver;
}

/**
 * Update event forwarder
 */
void EventForwarder::update(const wchar_t *description, const wchar_t *driverName, json_t *configuration)
{
   wcslcpy(m_description, description, MAX_NC_DESCRIPTION);

   if (configuration == nullptr)
      configuration = json_object();

   if (wcscmp(m_driverName, driverName) || !json_equal(m_configuration, configuration))
   {
      StringBuffer errorMessage;
      EventForwarderDriver *driver = CreateDriverInstance(m_name, driverName, configuration, &errorMessage);
      if (driver != nullptr)
         clearError();
      else
         setError(errorMessage.isEmpty() ? L"Driver not initialized" : errorMessage.cstr());

      m_driverLock.lock();
      delete m_driver;
      m_driver = driver;
      m_driverLock.unlock();

      wcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
      json_decref(m_configuration);
      m_configuration = configuration;  // take ownership
   }
   else
   {
      json_decref(configuration);  // unchanged - drop the passed copy
   }

   saveToDatabase();
}

/**
 * Save event forwarder configuration to database
 */
void EventForwarder::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const wchar_t *columns[] = { L"driver_name", L"description", L"configuration", nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"event_forwarders", L"name", m_name, columns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_driverName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, m_configuration, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Check event forwarder health
 */
void EventForwarder::checkHealth()
{
   bool status = false;
   m_driverLock.lock();
   if (m_driver != nullptr)
      status = m_driver->checkHealth();
   m_driverLock.unlock();
   m_healthCheckStatus = status;
}

/**
 * Built-in ISC event forwarder driver - forwards events to another NetXMS server
 */
class IscEventForwarderDriver : public EventForwarderDriver
{
private:
   wchar_t m_hostname[MAX_DNS_NAME];

public:
   IscEventForwarderDriver(const wchar_t *hostname)
   {
      wcslcpy(m_hostname, hostname, MAX_DNS_NAME);
   }

   virtual bool forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source) override;
};

/**
 * Forward event over ISC. The recipient is not used by this driver - events are always
 * forwarded to the server configured in the driver's "hostname" setting.
 */
bool IscEventForwarderDriver::forward(const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source)
{
   InetAddress addr = InetAddress::resolveHostName(m_hostname);
   if (!addr.isValidUnicast())
   {
      nxlog_debug_tag(DEBUG_TAG, 2, L"ISC forwarder: host name %s is invalid or cannot be resolved", m_hostname);
      return false;
   }

   ISC isc(addr);
   uint32_t rcc = isc.connect(ISC_SERVICE_EVENT_FORWARDER);
   if (rcc == ISC_ERR_SUCCESS)
   {
      NXCPMessage msg(CMD_FORWARD_EVENT, 1);
      if ((source != nullptr) && (source->getObjectClass() == OBJECT_NODE))
         msg.setField(VID_IP_ADDRESS, static_cast<Node*>(source.get())->getIpAddress());
      msg.setField(VID_EVENT_CODE, event.getCode());
      msg.setField(VID_EVENT_NAME, event.getName());
      msg.setField(VID_TAGS, event.getTagsAsList());
      msg.setField(VID_NUM_ARGS, static_cast<uint16_t>(event.getParametersCount()));
      for(int i = 0; i < event.getParametersCount(); i++)
         msg.setField(VID_EVENT_ARG_BASE + i, event.getParameter(i));
      const StringList *names = event.getParameterNames();
      for(int i = 0; i < names->size(); i++)
         msg.setField(VID_EVENT_ARG_NAMES_BASE + i, names->get(i));

      if (isc.sendMessage(&msg))
         rcc = isc.waitForRCC(1, 10000);
      else
         rcc = ISC_ERR_CONNECTION_BROKEN;
      isc.disconnect();
   }

   if (rcc != ISC_ERR_SUCCESS)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Failed to forward event to server %s (%s)", m_hostname, ISCErrorCodeToText(rcc));
      return false;
   }
   return true;
}

/**
 * Built-in ISC driver factory
 */
static EventForwarderDriver *CreateIscEventForwarderDriver(json_t *configuration)
{
   wchar_t *hostname = json_object_get_string_w(configuration, "hostname", L"");
   if (hostname[0] == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"ISC event forwarder driver requires \"hostname\" in configuration");
      MemFree(hostname);
      return nullptr;
   }

   EventForwarderDriver *driver = new IscEventForwarderDriver(hostname);
   MemFree(hostname);
   return driver;
}

/**
 * Register event forwarder driver (called by server modules at initialization)
 */
void NXCORE_EXPORTABLE RegisterEventForwarderDriver(const wchar_t *name, EventForwarderDriverFactory factory)
{
   EventForwarderDriverDescriptor *dd = new EventForwarderDriverDescriptor();
   wcslcpy(dd->name, name, MAX_OBJECT_NAME);
   dd->factory = factory;
   s_driverList.set(name, dd);
   nxlog_debug_tag(DEBUG_TAG, 4, L"Event forwarder driver \"%s\" registered", name);
}

/**
 * Check if event forwarder with given name exists
 */
bool NXCORE_EXPORTABLE IsEventForwarderExists(const wchar_t *name)
{
   s_forwarderListLock.lock();
   bool exists = s_forwarderList.contains(name);
   s_forwarderListLock.unlock();
   return exists;
}

/**
 * Create event forwarder object instance. Takes ownership of configuration.
 */
static shared_ptr<EventForwarder> CreateEventForwarderObject(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, json_t *configuration)
{
   if (configuration == nullptr)
      configuration = json_object();
   StringBuffer errorMessage;
   EventForwarderDriver *driver = CreateDriverInstance(name, driverName, configuration, &errorMessage);
   return make_shared<EventForwarder>(driver, name, description, driverName, configuration, errorMessage);
}

/**
 * Create new event forwarder. Takes ownership of configuration.
 */
void NXCORE_EXPORTABLE CreateEventForwarder(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, json_t *configuration)
{
   shared_ptr<EventForwarder> ef = CreateEventForwarderObject(name, description, driverName, configuration);
   s_forwarderListLock.lock();
   s_forwarderList.set(name, ef);
   ef->saveToDatabase();
   s_forwarderListLock.unlock();
}

/**
 * Update existing event forwarder. Takes ownership of configuration.
 */
void NXCORE_EXPORTABLE UpdateEventForwarder(const wchar_t *name, const wchar_t *description, const wchar_t *driverName, json_t *configuration)
{
   s_forwarderListLock.lock();
   shared_ptr<EventForwarder> ef = s_forwarderList.getShared(name);
   s_forwarderListLock.unlock();
   if (ef != nullptr)
      ef->update(description, driverName, configuration);
   else
      json_decref(configuration);
}

/**
 * Delete event forwarder (executed in separate thread)
 */
static void DeleteEventForwarderInternal(wchar_t *name)
{
   s_forwarderListLock.lock();
   shared_ptr<EventForwarder> ef = s_forwarderList.unlink(name);
   s_forwarderListLock.unlock();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"DELETE FROM event_forwarders WHERE name=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   MemFree(name);
}

/**
 * Delete event forwarder
 */
bool NXCORE_EXPORTABLE DeleteEventForwarder(const wchar_t *name)
{
   s_forwarderListLock.lock();
   bool contains = s_forwarderList.contains(name);
   s_forwarderListLock.unlock();
   if (contains)
      ThreadPoolExecuteSerialized(g_mainThreadPool, EF_THREAD_KEY, DeleteEventForwarderInternal, MemCopyString(name));
   return contains;
}

/**
 * Rename event forwarder in database (executed in separate thread)
 */
static void RenameEventForwarderInDB(std::pair<wchar_t*, wchar_t*> *names)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = false;
   DBBegin(hdb);

   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE event_forwarders SET name=? WHERE name=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, names->second, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, names->first, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   if (success)
   {
      hStmt = DBPrepare(hdb, L"UPDATE actions SET channel_name=? WHERE channel_name=? AND action_type=4");
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, names->second, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, names->first, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
   DBConnectionPoolReleaseConnection(hdb);

   MemFree(names->first);
   MemFree(names->second);
   delete names;
}

/**
 * Rename event forwarder
 */
void NXCORE_EXPORTABLE RenameEventForwarder(wchar_t *name, wchar_t *newName)
{
   s_forwarderListLock.lock();
   shared_ptr<EventForwarder> ef = s_forwarderList.unlink(name);
   if (ef != nullptr)
   {
      ef->updateName(newName);
      s_forwarderList.set(newName, ef);
      auto pair = new std::pair<wchar_t*, wchar_t*>(name, newName);
      UpdateForwarderNameInActions(pair);
      ThreadPoolExecuteSerialized(g_mainThreadPool, EF_THREAD_KEY, RenameEventForwarderInDB, pair);
   }
   else
   {
      MemFree(name);
      MemFree(newName);
   }
   s_forwarderListLock.unlock();
}

/**
 * Get event forwarder list
 */
void NXCORE_EXPORTABLE GetEventForwarders(NXCPMessage *msg)
{
   s_forwarderListLock.lock();
   msg->setField(VID_CHANNEL_COUNT, s_forwarderList.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   auto it = s_forwarderList.begin();
   while(it.hasNext())
   {
      shared_ptr<EventForwarder> ef = *it.next()->value;
      ef->fillMessage(msg, fieldId);
      fieldId += 20;
   }
   s_forwarderListLock.unlock();
}

/**
 * Get event forwarder list as JSON array
 */
json_t NXCORE_EXPORTABLE *GetEventForwarders(bool basicInfoOnly, bool includeSensitiveData)
{
   json_t *forwarders = json_array();
   s_forwarderListLock.lock();
   auto it = s_forwarderList.begin();
   while(it.hasNext())
   {
      shared_ptr<EventForwarder> ef = *it.next()->value;
      if (basicInfoOnly)
      {
         json_t *json = json_object();
         json_object_set_new(json, "name", json_string_t(ef->getName()));
         json_object_set_new(json, "description", json_string_t(ef->getDescription()));
         json_object_set_new(json, "driverName", json_string_t(ef->getDriverName()));
         json_array_append_new(forwarders, json);
      }
      else
      {
         json_array_append_new(forwarders, ef->toJson(includeSensitiveData));
      }
   }
   s_forwarderListLock.unlock();
   return forwarders;
}

/**
 * Get single event forwarder as JSON by name
 */
json_t NXCORE_EXPORTABLE *GetEventForwarderByName(const wchar_t *name, bool includeSensitiveData)
{
   json_t *result = nullptr;
   s_forwarderListLock.lock();
   EventForwarder *ef = s_forwarderList.get(name);
   if (ef != nullptr)
      result = ef->toJson(includeSensitiveData);
   s_forwarderListLock.unlock();
   return result;
}

/**
 * Get event forwarder driver list as JSON array
 */
json_t NXCORE_EXPORTABLE *GetEventForwarderDriversAsJson()
{
   json_t *drivers = json_array();
   StringList driverNames = s_driverList.keys();
   for(int i = 0; i < driverNames.size(); i++)
      json_array_append_new(drivers, json_string_t(driverNames.get(i)));
   return drivers;
}

/**
 * Get event forwarder driver list
 */
void NXCORE_EXPORTABLE GetEventForwarderDrivers(NXCPMessage *msg)
{
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   StringList driverNames = s_driverList.keys();
   for(int i = 0; i < driverNames.size(); i++)
   {
      msg->setField(fieldId++, driverNames.get(i));
   }
   msg->setField(VID_DRIVER_COUNT, s_driverList.size());
}

/**
 * Forward event using named forwarder instance
 */
void NXCORE_EXPORTABLE ForwardEventToForwarder(const wchar_t *name, const Event& event, const TCHAR *recipient, const shared_ptr<NetObj>& source)
{
   s_forwarderListLock.lock();
   shared_ptr<EventForwarder> ef = s_forwarderList.getShared(name);
   s_forwarderListLock.unlock();
   if (ef != nullptr)
   {
      ef->enqueue(event, recipient, source);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"ForwardEventToForwarder: event forwarder \"%s\" not found", name);
   }
}

/**
 * Event forwarder driver health checking thread
 */
static void CheckEventForwarderDriversHealth()
{
   while(true)
   {
      SharedObjectArray<EventForwarder> forwarders;
      s_forwarderListLock.lock();
      for(auto f : s_forwarderList)
         forwarders.add(*f->value);
      s_forwarderListLock.unlock();

      for(int i = 0; i < forwarders.size(); i++)
         forwarders.getShared(i)->checkHealth();

      if (SleepAndCheckForShutdown(60))
         break;
   }
}

/**
 * Health check thread handle
 */
static THREAD s_healthCheckThread = INVALID_THREAD_HANDLE;

/**
 * Load event forwarders from database on server start
 */
void LoadEventForwarders()
{
   // Register built-in drivers (modules register their own drivers during initialization)
   RegisterEventForwarderDriver(L"isc", CreateIscEventForwarderDriver);
   RegisterEventForwarderDriver(L"snmptrap", CreateSnmpTrapEventForwarderDriver);

   int count = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, L"SELECT name,driver_name,description,configuration FROM event_forwarders");
   if (hResult != nullptr)
   {
      int numRows = DBGetNumRows(hResult);
      for(int i = 0; i < numRows; i++)
      {
         wchar_t name[MAX_OBJECT_NAME], driverName[MAX_OBJECT_NAME], description[MAX_NC_DESCRIPTION];
         DBGetField(hResult, i, 0, name, MAX_OBJECT_NAME);
         DBGetField(hResult, i, 1, driverName, MAX_OBJECT_NAME);
         DBGetField(hResult, i, 2, description, MAX_NC_DESCRIPTION);
         json_t *configuration = DBGetFieldJson(hResult, i, 3);
         shared_ptr<EventForwarder> ef = CreateEventForwarderObject(name, description, driverName, configuration);
         s_forwarderListLock.lock();
         s_forwarderList.set(name, ef);
         s_forwarderListLock.unlock();
         count++;
         nxlog_debug_tag(DEBUG_TAG, 4, L"Event forwarder \"%s\" successfully created", name);
      }
      DBFreeResult(hResult);
   }
   nxlog_debug_tag(DEBUG_TAG, 1, L"%d event forwarders loaded", count);

   s_healthCheckThread = ThreadCreateEx(CheckEventForwarderDriversHealth);

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Shutdown all event forwarders
 */
void ShutdownEventForwarders()
{
   s_forwarderListLock.lock();

   auto sit = s_forwarderList.begin();
   while(sit.hasNext())
      (*sit.next()->value)->requestShutdown();

   auto jit = s_forwarderList.begin();
   while(jit.hasNext())
   {
      EventForwarder *ef = jit.next()->value->get();
      nxlog_debug_tag(DEBUG_TAG, 4, L"Waiting for event forwarder \"%s\" worker thread to stop", ef->getName());
      ef->joinWorker();
   }

   s_forwarderList.clear();
   s_forwarderListLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 3, L"All event forwarder worker threads stopped");
}
