#include "nxuseragent.h"

/**
 * List of active user agent messages
 */
static HashMap<ServerObjectKey, UserAgentNotification> s_notifications(true);
static Mutex s_notificationLock;

/**
 * Update notifications for user
 */
void UpdateNotifications(NXCPMessage *request)
{
   s_notificationLock.lock();

   s_notifications.clear();
   int count = request->getFieldAsInt32(VID_UA_NOTIFICATION_COUNT);
   int base = VID_UA_NOTIFICATION_BASE;
   for (int i = 0; i < count; i++, base += 10)
   {
      UserAgentNotification *n = new UserAgentNotification(request, base);
      s_notifications.set(n->getId(), n);
      nxlog_debug(7, _T("Notification [%u]: %s"), n->getId().objectId, n->getMessage());
   }

   s_notificationLock.unlock();
}

/**
 * Add new notification
 */
void AddNotification(NXCPMessage *request)
{
   UserAgentNotification *n = new UserAgentNotification(request, VID_UA_NOTIFICATION_BASE);
   nxlog_debug(7, _T("Add notification [%u]: %s"), n->getId().objectId, n->getMessage());

   s_notificationLock.lock();
   s_notifications.set(n->getId(), n);
   s_notificationLock.unlock();
}

/**
 * Remove notification
 */
void RemoveNotification(NXCPMessage *request)
{
   ServerObjectKey id = ServerObjectKey(request->getFieldAsUInt64(VID_UA_NOTIFICATION_BASE + 1),
         request->getFieldAsUInt32(VID_UA_NOTIFICATION_BASE));
   nxlog_debug(7, _T("remove notification [%u]"), id.objectId);

   s_notificationLock.lock();
   s_notifications.remove(id);
   s_notificationLock.unlock();
}
