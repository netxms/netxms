#ifndef __zeromq__h__
#define __zeromq__h__

#include <zmq.h>

#define ZMQ_DCI_ID_INVALID (-1)
#define ZMQ_DCI_ID_ANY (0)

namespace zmq {
   enum SubscriptionType {
      EVENT,
      DATA
   };

   class Subscription
   {
      private:
         UINT32 objectId;
         bool ignoreItems;
         IntegerArray<UINT32> *items;

      public:
         Subscription(UINT32 objectId, UINT32 dciId = ZMQ_DCI_ID_INVALID);
         ~Subscription();
         UINT32 getObjectId() { return objectId; }
         IntegerArray<UINT32> *getItems() { return items; }
         bool isIgnoreItems() { return ignoreItems; }
         void addItem(UINT32 dciId);
         bool removeItem(UINT32 dciId);
         bool match(UINT32 dciId);
   };
}

void StartZMQConnector();
void StopZMQConnector();
void ZmqPublishEvent(const Event *event);
void ZmqPublishData(UINT32 objectId, UINT32 dciId, const TCHAR *dciName, const TCHAR *value);
bool ZmqSubscribeEvent(UINT32 objectId, UINT32 eventCode, UINT32 dciId);
bool ZmqUnsubscribeEvent(UINT32 objectId, UINT32 eventCode, UINT32 dciId);
bool ZmqSubscribeData(UINT32 objectId, UINT32 dciId = 0);
bool ZmqUnsubscribeData(UINT32 objectId, UINT32 dciId = 0);
void ZmqFillSubscriptionListMessage(NXCPMessage *msg, zmq::SubscriptionType);

#endif
