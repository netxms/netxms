/*
** NetXMS - Network Management System
** Copyright (C) 2015 Raden Solutions
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
** File: zeromq.cpp
**
**/

#include "nxcore.h"

#ifdef WITH_ZMQ
#include "zeromq.h"
#include <jansson.h>

#define DB_TYPE_EVENT _T("E")
#define DB_TYPE_DATA _T("D")
#define DB_ITEM_SEPARATOR _T(",")

using namespace zmq;

static void *m_context = NULL;
static void *m_socket = NULL;
static MUTEX m_socketLock = MutexCreate();
static HashMap<UINT32, Subscription> m_eventSubscription(true);
static HashMap<UINT32, Subscription> m_dataSubscription(true);
static MUTEX m_eventSubscriptionLock = MutexCreate();
static MUTEX m_dataSubscriptionLock = MutexCreate();


/******************************************************************************
 * INTERNAL                                                                   *
 *****************************************************************************/

/**
 *
 */
Subscription::Subscription(UINT32 objectId, UINT32 dciId)
{
   this->objectId = objectId;
   ignoreItems = false;
   items = new IntegerArray<UINT32>();

   if (dciId != ZMQ_DCI_ID_INVALID)
   {
      addItem(dciId);
   }
}


/**
 *
 */
Subscription::~Subscription()
{
   delete this->items;
}


/**
 *
 */
void Subscription::addItem(UINT32 dciId)
{
   if (dciId == ZMQ_DCI_ID_ANY)
   {
      ignoreItems = true;
   }
   else
   {
      items->add(dciId);
   }
}


/**
 *
 */
bool Subscription::removeItem(UINT32 dciId)
{
   if (items->contains(dciId))
   {
      items->remove(items->indexOf(dciId));
   }
   if (dciId == ZMQ_DCI_ID_ANY)
   {
      ignoreItems = false;
   }

   return !ignoreItems && items->size() == 0;
}

/**
 *
 */
bool Subscription::match(UINT32 dciId)
{
   return ignoreItems || items->contains(dciId);
}

/**
 * Load subscriptions from database
 */
static void LoadSubscriptions()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
	DB_RESULT result = DBSelect(db, _T("SELECT object_id, subscription_type, ignore_items, items FROM zmq_subscription"));
   if (result != NULL)
   {
      MutexLock(m_eventSubscriptionLock);
      MutexLock(m_dataSubscriptionLock);

      int count = DBGetNumRows(result);
      TCHAR type[2];
      for (int i = 0; i < count; i++)
      {
         UINT32 objectId = DBGetFieldULong(result, i, 0);
         DBGetField(result, i, 1, type, sizeof(type) / sizeof(type[0]));
         bool ignoreItems = DBGetFieldLong(result, i, 2);
         TCHAR *items = DBGetField(result, i, 3, NULL, 0);

         HashMap<UINT32, Subscription> *map = NULL;
         switch (type[0])
         {
            case 'E':
               DbgPrintf(6, _T("ZeroMQ: event subscription loaded (objectId=%d, ignoreItems=%s, items=\"%s\")"), objectId, ignoreItems ? _T("YES") : _T("NO"), items);
               map = &m_eventSubscription;
               break;
            case 'D':
               DbgPrintf(6, _T("ZeroMQ: data subscription loaded (objectId=%d, ignoreItems=%s, items=\"%s\")"), objectId, ignoreItems ? _T("YES") : _T("NO"), items);
               map = &m_dataSubscription;
               break;
            default:
               DbgPrintf(1, _T("ZeroMQ: Invalid subscription found in database (type=%s)"), type);
               break;
         }

         if (map != NULL)
         {
            Subscription *sub;
            if (map->contains(objectId))
            {
               sub = map->get(objectId);
            }
            else
            {
               sub = new Subscription(objectId);
               map->set(objectId, sub);
            }

            if (ignoreItems)
            {
               sub->addItem(ZMQ_DCI_ID_ANY);
            }
            StringList *itemList = new StringList(items, DB_ITEM_SEPARATOR);
            int count = itemList->size();
            for (int j = 0; j < count; j++)
            {
               UINT32 dciId = _tcstol(itemList->get(j), NULL, 0);
               if (dciId > 0)
               {
                  sub->addItem(dciId);
               }
            }

            delete itemList;
         }
         free(items);
      }

      MutexUnlock(m_eventSubscriptionLock);
      MutexUnlock(m_dataSubscriptionLock);

      DBFreeResult(result);
   }
   else
   {
      DbgPrintf(1, _T("Cannot load ZMQ subscriptions"));
   }
   DBConnectionPoolReleaseConnection(db);
}


/**
 *
 */
static bool SaveSubscriptionInternal(DB_STATEMENT statement, HashMap<UINT32, Subscription> *subscriptions, const TCHAR *type)
{
   Iterator<Subscription> *it = subscriptions->iterator();
   while (it->hasNext())
   {
      Subscription *sub = it->next();

      StringList *itemList = new StringList();
      IntegerArray<UINT32> *items = sub->getItems();
      for (int i =  0; i < items->size(); i++)
      {
         itemList->add(items->get(i));
      }

      DBBind(statement, 1, DB_SQLTYPE_VARCHAR, sub->getObjectId());
      DBBind(statement, 2, DB_SQLTYPE_VARCHAR, type, DB_BIND_STATIC);
      DBBind(statement, 3, DB_SQLTYPE_INTEGER, (UINT32)sub->isIgnoreItems());
      DBBind(statement, 4, DB_SQLTYPE_VARCHAR, itemList->join(DB_ITEM_SEPARATOR), DB_BIND_DYNAMIC);

      delete itemList;

      if (!DBExecute(statement))
      {
         return false;
      }
   }

   return true;
}

/**
 * Save subscriptions to database
 */
static bool SaveSubscriptions()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   bool success = false;

   if (DBBegin(db))
   {
      DBQuery(db, _T("DELETE FROM zmq_subscription"));
      DB_STATEMENT statement = DBPrepare(db, _T("INSERT INTO zmq_subscription (object_id, subscription_type, ignore_items, items) VALUES (?, ?, ?, ?)"));
      if (statement != NULL)
      {
         if (SaveSubscriptionInternal(statement, &m_eventSubscription, DB_TYPE_EVENT))
         {
            if (SaveSubscriptionInternal(statement, &m_dataSubscription, DB_TYPE_DATA))
            {
               success = true;
            }
         }

         DBFreeStatement(statement);
      }

      if (success)
      {
         DBCommit(db);
      }
      else
      {
         DBRollback(db);
      }
   }

   DBConnectionPoolReleaseConnection(db);
   return success;
}

/**
 * Add dynamic string to JSON
 */
inline json_t *json_string_dynamic(char *s)
{
   json_t *json = json_string(s);
   free(s);
   return json;
}

/**
 * Convert event object to JSON representation
 */
static char *EventToJson(const Event *event, const NetObj *object)
{
   json_t *root = json_object();
   json_t *args = json_object();
   json_t *source = json_object();

   json_object_set_new(root, "source", source);
   json_object_set_new(root, "arguments", args);

   json_object_set_new(root, "id", json_integer(event->getId()));
   json_object_set_new(root, "time", json_integer(event->getTimeStamp()));
   json_object_set_new(root, "code", json_integer(event->getCode()));
   json_object_set_new(root, "severity", json_integer(event->getSeverity()));
   json_object_set_new(root, "message", json_string_dynamic(UTF8StringFromTString(event->getMessage())));

   json_object_set_new(source, "id", json_integer(event->getSourceId()));
   int objectType = object->getObjectClass();
   json_object_set_new(source, "type", json_integer(objectType));
   json_object_set_new(source, "name", json_string_dynamic(UTF8StringFromTString(object->getName())));
   if (objectType == OBJECT_NODE)
   {
      InetAddress ip = ((Node *)object)->getIpAddress();
      json_object_set_new(source, "primary-ip", json_string(ip.toStringA(NULL)));
   }

   int count = event->getParametersCount();
   json_object_set_new(args, "count", json_integer(count));
   for (int i = 0; i < count; i++)
   {
      char name[16];
      sprintf(name, "p%d", i+1);
      json_object_set_new(args, name, json_string_dynamic(UTF8StringFromTString(event->getParameter(i))));
   }

   char *message = json_dumps(root, 0);
   json_decref(root);
   return message;
}

/**
 * Convert DCI value to JSON representation
 */
static char *DataToJson(const NetObj *object, UINT32 dciId, const TCHAR *dciName, const TCHAR *value)
{
   json_t *root = json_object();
   json_t *data = json_array();

   json_object_set_new(root, "data", data);

   json_object_set_new(root, "id", json_integer(object->getId()));

   json_t *record = json_object();
   json_object_set_new(record, "id", json_integer(dciId));
   char *utf8name = UTF8StringFromTString(dciName);
   json_object_set_new(record, utf8name, json_string_dynamic(UTF8StringFromTString(value)));
   free(utf8name);
   json_array_append(data, record);

   char *message = json_dumps(root, 0);
   json_decref(root);
   return message;
}

/**
 * Subscribe
 */
static bool Subscribe(HashMap<UINT32, Subscription> *map, MUTEX mutex, UINT32 objectId, UINT32 dciId)
{
   MutexLock(mutex);
   if (map->contains(objectId))
   {
      Subscription *sub = map->get(objectId);
      sub->addItem(dciId);
   }
   else
   {
      map->set(objectId, new Subscription(objectId, dciId));
   }
   MutexUnlock(mutex);

   SaveSubscriptions();

   return true;
}


/**
 *
 */
static bool Unsubscribe(HashMap<UINT32, Subscription> *map, MUTEX mutex, UINT32 objectId, UINT32 dciId)
{
   bool ret = true;

   MutexLock(mutex);
   if (map->contains(objectId))
   {
      Subscription *sub = map->get(objectId);
      if (sub->removeItem(dciId))
      {
         map->remove(objectId);
      }
   }
   else
   {
      ret = false;
   }
   MutexUnlock(mutex);

   SaveSubscriptions();

   return ret;
}


/******************************************************************************
 * PUBLIC                                                                     *
 *****************************************************************************/

/**
 *
 */
void StartZMQConnector()
{
   char endpoint[MAX_CONFIG_VALUE];
   ConfigReadStrUTF8(_T("ZeroMQEndpoint"), endpoint, MAX_CONFIG_VALUE, "");
   if (endpoint[0] == 0)
   {
      DbgPrintf(1, _T("ZeroMQ: endpoint not configured, integration disabled (expected parameter: ZeroMQEndpoint)"));
      return;
   }

   m_context = zmq_ctx_new();
   if (m_context != NULL)
   {
      m_socket = zmq_socket(m_context, ZMQ_PUSH);
      if (m_socket != NULL)
      {
         MutexLock(m_socketLock);
         if (zmq_connect(m_socket, endpoint) == 0)
         {
            LoadSubscriptions();
            DbgPrintf(1, _T("ZeroMQ: connector initialised"));
         }
         else
         {
            DbgPrintf(1, _T("ZeroMQ: bind failed, integration disabled"));
            zmq_ctx_term(m_context);
            m_context = NULL;
            zmq_close(m_socket);
            m_socket = NULL;
         }
         MutexUnlock(m_socketLock);
      }
      else
      {
         DbgPrintf(1, _T("ZeroMQ: cannot create socket, integration disabled"));
         zmq_ctx_term(m_context);
         m_context = NULL;
      }
   }
   else
   {
         DbgPrintf(1, _T("ZeroMQ: cannot initalise context, integration disabled"));
   }
}


/**
 *
 */
void StopZMQConnector()
{
   DbgPrintf(6, _T("ZeroMQ: shutdown initiated"));
   if (m_socket != NULL)
   {
      MutexLock(m_socketLock);
      zmq_close(m_socket);
      m_socket = NULL;
      MutexUnlock(m_socketLock);
   }
   DbgPrintf(6, _T("ZeroMQ: socket closed"));
   if (m_context != NULL)
   {
      zmq_ctx_term(m_context);
      m_context = NULL;
   }
   DbgPrintf(6, _T("ZeroMQ: context destroyed"));
}


/**
 *
 */
void ZmqPublishEvent(const Event *event)
{
   if (m_socket == NULL)
   {
      return;
   }

   UINT32 objectId = event->getSourceId();
   NetObj *object = FindObjectById(objectId);
   if (object == NULL)
   {
      return;
   }

   DbgPrintf(7, _T("ZeroMQ: publish event: %s(%d) from %s(%d)"), event->getName(), event->getCode(), object->getName(), event->getSourceId());

   bool doSend = false;
   MutexLock(m_eventSubscriptionLock);
   if (m_eventSubscription.contains(objectId))
   {
      Subscription *sub = m_eventSubscription.get(objectId);
      doSend = sub->match(event->getDciId());
   }
   MutexUnlock(m_eventSubscriptionLock);

   if (doSend)
   {
      char *message = EventToJson(event, object);
      if (message != NULL)
      {
         MutexLock(m_socketLock);
         if (zmq_send(m_socket, message, strlen(message), ZMQ_DONTWAIT) != 0)
         {
            DbgPrintf(7, _T("ZeroMQ: publish event failed (%d)"), errno);
         }
         MutexUnlock(m_socketLock);
         free(message);
      }
   }
}


/**
 *
 */
void ZmqPublishData(UINT32 objectId, UINT32 dciId, const TCHAR *dciName, const TCHAR *value)
{
   if (m_socket == NULL)
   {
      return;
   }

   NetObj *object = FindObjectById(objectId);
   if (object == NULL)
   {
      return;
   }

   DbgPrintf(7, _T("ZeroMQ: publish collected data on %s(%d): %s"), object->getName(), objectId, value);

   bool doSend = false;
   MutexLock(m_dataSubscriptionLock);
   if (m_dataSubscription.contains(objectId))
   {
      Subscription *sub = m_dataSubscription.get(objectId);
      doSend = sub->match(dciId);
   }
   MutexUnlock(m_dataSubscriptionLock);

   if (doSend)
   {
      char *message = DataToJson(object, dciId, dciName, value);
      if (message != NULL)
      {
         MutexLock(m_socketLock);
         if (zmq_send(m_socket, message, strlen(message), ZMQ_DONTWAIT) != 0)
         {
            DbgPrintf(7, _T("ZeroMQ: publish collected data failed (%d)"), errno);
         }
         MutexUnlock(m_socketLock);
         free(message);
      }
   }
}


/**
 *
 */
bool ZmqSubscribeEvent(UINT32 objectId, UINT32 eventCode, UINT32 dciId)
{
   DbgPrintf(5, _T("ZeroMQ: subscribe event forwarding from %d (dciId=%d)"), objectId, dciId);
   return Subscribe(&m_eventSubscription, m_eventSubscriptionLock, objectId, dciId);
}


/**
 *
 */
bool ZmqUnsubscribeEvent(UINT32 objectId, UINT32 eventCode, UINT32 dciId)
{
   DbgPrintf(5, _T("ZeroMQ: unsubscribe event forwarding from %d (dciId=%d)"), objectId, dciId);
   return Unsubscribe(&m_eventSubscription, m_eventSubscriptionLock, objectId, dciId);
}


/**
 *
 */
bool ZmqSubscribeData(UINT32 objectId, UINT32 dciId)
{
   DbgPrintf(5, _T("ZeroMQ: subscribe DCI %d on %d"), dciId, objectId);
   return Subscribe(&m_dataSubscription, m_dataSubscriptionLock, objectId, dciId);
}


/**
 *
 */
bool ZmqUnsubscribeData(UINT32 objectId, UINT32 dciId)
{
   DbgPrintf(5, _T("ZeroMQ: unsubscribe DCI %d on %d"), dciId, objectId);
   return Unsubscribe(&m_dataSubscription, m_dataSubscriptionLock, objectId, dciId);
}


/**
 *
 */
void ZmqFillSubscriptionListMessage(NXCPMessage *msg, zmq::SubscriptionType type)
{
   Iterator<Subscription> *it;
   if (type == zmq::EVENT)
   {
      it = m_eventSubscription.iterator();
   }
   else
   {
      it = m_dataSubscription.iterator();
   }

   int baseId = VID_ZMQ_SUBSCRIPTION_BASE;
   while (it->hasNext())
   {
      Subscription *sub = it->next();
      msg->setField(baseId, sub->getObjectId());
      msg->setField(baseId + 1, sub->isIgnoreItems());
      msg->setFieldFromInt32Array(baseId + 2, sub->getItems());

      baseId += 10;
   }
}

#endif // WITH_ZMQ
