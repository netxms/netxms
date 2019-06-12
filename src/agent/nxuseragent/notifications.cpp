#include "nxuseragent.h"

/**
 * List of active user agent messages
 */
static HashMap<ServerObjectKey, UserNotification> s_notifications(true);
static Mutex s_notificationLock;

/**
 * Update notifications for user
 */
void UpdateNotifications(NXCPMessage *request)
{
   s_notificationLock.lock();

   s_notifications.clear();
   int count = request->getFieldAsInt32(VID_USER_AGENT_MESSAGE_COUNT);
   int base = VID_USER_AGENT_MESSAGE_BASE;
   for (int i = 0; i < count; i++, base += 10)
   {
      UserNotification *n = new UserNotification(request, base);
      s_notifications.set(n->getId(), n);
      nxlog_debug(7, _T("User notification [%u]: %s"), n->getId().objectId, n->getMessage());
   }

   s_notificationLock.unlock();
}
