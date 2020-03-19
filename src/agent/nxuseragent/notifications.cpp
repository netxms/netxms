#include "nxuseragent.h"

/**
 * List of active user agent messages
 */
static HashMap<ServerObjectKey, UserAgentNotification> s_notifications(Ownership::True);
static Mutex s_notificationLock;

/**
 * Delete read mark in registry
 */
static void DeleteReadMark(const ServerObjectKey& id)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\UserAgent\\Notifications"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("%u:%I64u"), id.objectId, id.serverId);
      RegDeleteValue(hKey, name);
      RegCloseKey(hKey);
   }
}

/**
 * Update read flag
 */
static void MarkAsRead(UserAgentNotification *n)
{
   n->setRead();

   HKEY hKey;
   if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\UserAgent\\Notifications"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("%u:%I64u"), n->getId().objectId, n->getId().serverId);
      DWORD t = (DWORD)time(NULL);
      RegSetValueEx(hKey, name, 0, REG_DWORD, (BYTE *)&t, sizeof(DWORD));
      RegCloseKey(hKey);
   }
}

/**
 * Mark notifications in given list as read. Provided list is a copy of original objects.
 */
void MarkNotificationAsRead(const ServerObjectKey& id)
{
   s_notificationLock.lock();

   UserAgentNotification *n = s_notifications.get(id);
   if (n != nullptr)
   {
      if (n->isInstant())  // Delete instant messages immediately after read
      {
         DeleteReadMark(id);
         s_notifications.remove(id);
      }
      else
      {
         MarkAsRead(n);
      }
   }

   s_notificationLock.unlock();
}

/**
 * Check if marked as read in registry
 */
static bool IsMarkedAsRead(const ServerObjectKey& id)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\NetXMS\\UserAgent\\Notifications"), 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
      return false;

   TCHAR name[64];
   _sntprintf(name, 64, _T("%u:%I64u"), id.objectId, id.serverId);
   DWORD data, size = sizeof(DWORD);
   bool result = false;
   if (RegQueryValueEx(hKey, name, NULL, NULL, reinterpret_cast<BYTE*>(&data), &size) == ERROR_SUCCESS)
   {
      result = (data != 0);
   }
   RegCloseKey(hKey);
   return result;
}

/**
 * Update notifications for user
 */
void UpdateNotifications(const NXCPMessage *request)
{
   s_notificationLock.lock();

   s_notifications.clear();
   int count = request->getFieldAsInt32(VID_UA_NOTIFICATION_COUNT);
   int base = VID_UA_NOTIFICATION_BASE;
   for (int i = 0; i < count; i++, base += 10)
   {
      UserAgentNotification *n = new UserAgentNotification(request, base);
      if (IsMarkedAsRead(n->getId()))
         n->setRead();
      s_notifications.set(n->getId(), n);
      nxlog_debug(7, _T("Notification [%u]: %s"), n->getId().objectId, n->getMessage());
   }

   s_notificationLock.unlock();

   PostThreadMessage(g_mainThreadId, NXUA_MSG_SHOW_NOTIFICATIONS, 1, 0);
}

/**
 * Add new notification
 */
void AddNotification(const NXCPMessage *request)
{
   UserAgentNotification *n = new UserAgentNotification(request, VID_UA_NOTIFICATION_BASE);
   nxlog_debug(7, _T("Add notification [%u]: %s"), n->getId().objectId, n->getMessage());

   s_notificationLock.lock();
   s_notifications.set(n->getId(), n);
   s_notificationLock.unlock();

   if (n->isInstant())
   {
      PostThreadMessage(g_mainThreadId, NXUA_MSG_SHOW_NOTIFICATIONS, 0, 0);
   }
   else
   {
      if (IsMarkedAsRead(n->getId()))
         n->setRead();
   }
}

/**
 * Remove notification
 */
void RemoveNotification(const NXCPMessage *request)
{
   ServerObjectKey id = ServerObjectKey(request->getFieldAsUInt64(VID_UA_NOTIFICATION_BASE + 1),
         request->getFieldAsUInt32(VID_UA_NOTIFICATION_BASE));
   nxlog_debug(7, _T("remove notification [%u]"), id.objectId);

   s_notificationLock.lock();
   s_notifications.remove(id);
   s_notificationLock.unlock();

   DeleteReadMark(id);
}

/**
 * Selector for notifications
 */
static EnumerationCallbackResult NotificationSelector(const void *key, const void *value, void *context)
{
   const UserAgentNotification *n = static_cast<const UserAgentNotification*>(value);
   time_t now = time(nullptr);
   if (!n->isRead() && !n->isStartup() && (n->isInstant() || ((n->getStartTime() <= now) && (n->getEndTime() >= now))))
   {
      static_cast<ObjectArray<UserAgentNotification>*>(context)->add(new UserAgentNotification(n));
   }
   return _CONTINUE;
}

/**
 * Selector for startup notifications
 */
static EnumerationCallbackResult StartupNotificationSelector(const void *key, const void *value, void *context)
{
   const UserAgentNotification *n = static_cast<const UserAgentNotification*>(value);
   time_t now = time(nullptr);
   if (n->isStartup() && (n->getStartTime() <= now) && (n->getEndTime() >= now))
   {
      static_cast<ObjectArray<UserAgentNotification>*>(context)->add(new UserAgentNotification(n));
   }
   return _CONTINUE;
}

/**
 * Get list of notifications to be displayed (should be destroyed by caller).
 */
ObjectArray<UserAgentNotification> *GetNotificationsForDisplay(bool startup)
{
   ObjectArray<UserAgentNotification> *list = new ObjectArray<UserAgentNotification>(64, 64, Ownership::True);
   time_t now = time(NULL);
   s_notificationLock.lock();
   s_notifications.forEach(startup ? StartupNotificationSelector : NotificationSelector, list);
   s_notificationLock.unlock();
   return list;
}

/**
 * Callback for checking pnding notifications
 */
static EnumerationCallbackResult CheckPendingNotifications(const void *key, const void *value, void *context)
{
   const UserAgentNotification *n = static_cast<const UserAgentNotification*>(value);
   time_t now = time(nullptr);
   if (!n->isRead() && !n->isStartup() && (n->isInstant() || ((n->getStartTime() <= now) && (n->getEndTime() >= now))))
   {
      *static_cast<bool*>(context) = true;
      return _STOP;
   }
   return _CONTINUE;
}

/**
 * Notification manager
 */
THREAD_RESULT THREAD_CALL NotificationManager(void *arg)
{
   nxlog_write(NXLOG_INFO, _T("Notification manager thread started"));
   while (!ConditionWait(g_shutdownCondition, 10000))
   {
      bool pendingNotifications = false;
      s_notificationLock.lock();
      s_notifications.forEach(CheckPendingNotifications, &pendingNotifications);
      s_notificationLock.unlock();
      if (pendingNotifications)
      {
         nxlog_debug(4, _T("NotificationManager: pending notifications found"));
         PostThreadMessage(g_mainThreadId, NXUA_MSG_SHOW_NOTIFICATIONS, 0, 0);
      }
   }
   nxlog_write(NXLOG_INFO, _T("Notification manager thread stopped"));
   return THREAD_OK;
}
