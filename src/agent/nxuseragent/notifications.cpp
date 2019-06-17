#include "nxuseragent.h"

/**
 * List of active user agent messages
 */
static HashMap<ServerObjectKey, UserAgentNotification> s_notifications(true);
static Mutex s_notificationLock;

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

   if (n->getStartTime() == 0)
   {
      ShowTrayNotification(n->getMessage());
   }
   else
   {
      s_notificationLock.lock();
      s_notifications.set(n->getId(), n);
      s_notificationLock.unlock();
   }
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

   DeleteReadMark(id);
}
