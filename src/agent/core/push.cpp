/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: push.cpp
**
**/

#include "nxagentd.h"

/**
 * Request ID
 */
static UINT64 s_requestIdHigh = (UINT64)time(NULL) << 32;
static VolatileCounter s_requestIdLow = 0;

/**
 * Push parameter's data
 */
bool PushData(const TCHAR *parameter, const TCHAR *value, UINT32 objectId, time_t timestamp)
{
	bool success = false;

	nxlog_debug(6, _T("PushData: \"%s\" = \"%s\""), parameter, value);

	NXCPMessage msg(CMD_PUSH_DCI_DATA, 0, 4); // Use version 4 to avoid compatibility issues
	msg.setField(VID_NAME, parameter);
	msg.setField(VID_VALUE, value);
   msg.setField(VID_OBJECT_ID, objectId);
   msg.setFieldFromTime(VID_TIMESTAMP, timestamp);
   msg.setField(VID_REQUEST_ID, s_requestIdHigh | (UINT64)InterlockedIncrement(&s_requestIdLow)); 

   if (g_dwFlags & AF_SUBAGENT_LOADER)
   {
      success = SendMessageToMasterAgent(&msg);
   }
   else
   {
      MutexLock(g_hSessionListAccess);
      for(DWORD i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
            if (g_pSessionList[i]->canAcceptTraps())
            {
               g_pSessionList[i]->sendMessage(&msg);
               success = true;
            }
      MutexUnlock(g_hSessionListAccess);
   }
	return success;
}

/**
 * Pushed value
 */
struct PushDataEntry
{
   time_t timestamp;
   TCHAR value[MAX_RESULT_LENGTH];

   PushDataEntry(time_t t, const TCHAR *v)
   {
      timestamp = t;
      _tcscpy(value, v);
   }
};

/**
 * Local cache
 */
static ObjectMemoryPool<PushDataEntry> s_localCachePool;
static StringObjectMap<PushDataEntry> s_localCache(false);
static Mutex s_localCacheLock;

/**
 * Store pushed data locally
 */
static void StoreLocalData(const TCHAR *parameter, const TCHAR *value, UINT32 objectId, time_t timestamp)
{
   nxlog_debug(6, _T("StoreLocalData: \"%s\" = \"%s\""), parameter, value);
   s_localCacheLock.lock();
   s_localCache.set(parameter, new(s_localCachePool.allocate()) PushDataEntry(timestamp, value));
   s_localCacheLock.unlock();
}

/**
 * Process push request
 */
static void ProcessPushRequest(NamedPipe *pipe, void *arg)
{
	TCHAR buffer[256];

	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection established"));
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
	while(true)
	{
      MessageReceiverResult result;
		NXCPMessage *msg = receiver.readMessage(5000, &result);
		if (msg == NULL)
			break;
		AgentWriteDebugLog(6, _T("ProcessPushRequest: received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
		if (msg->getCode() == CMD_PUSH_DCI_DATA)
		{
         UINT32 objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
			UINT32 count = msg->getFieldAsUInt32(VID_NUM_ITEMS);
         time_t timestamp = msg->getFieldAsTime(VID_TIMESTAMP);
         bool localCache = msg->getFieldAsBoolean(VID_LOCAL_CACHE);
			UINT32 varId = VID_PUSH_DCI_DATA_BASE;
			for(DWORD i = 0; i < count; i++)
			{
				TCHAR name[MAX_RUNTIME_PARAM_NAME], value[MAX_RESULT_LENGTH];
				msg->getFieldAsString(varId++, name, MAX_RUNTIME_PARAM_NAME);
				msg->getFieldAsString(varId++, value, MAX_RESULT_LENGTH);
				if (localCache)
				   StoreLocalData(name, value, objectId, timestamp);
				else
				   PushData(name, value, objectId, timestamp);
			}
		}
		delete msg;
	}
	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection by user %s closed"), pipe->user());
}

/**
 * Pipe listener for push requests
 */
static NamedPipeListener *s_listener;

/**
 * Start push connector
 */
void StartPushConnector()
{
   if (!(g_dwFlags & AF_ENABLE_PUSH_CONNECTOR))
   {
      nxlog_write(NXLOG_INFO, _T("Push connector is disabled"));
      return;
   }

   const TCHAR *user = g_config->getValue(_T("/%agent/PushUser"), _T("*"));
   s_listener = NamedPipeListener::create(_T("nxagentd.push"), ProcessPushRequest, NULL,
            (user != NULL) && (user[0] != 0) && _tcscmp(user, _T("*")) ? user : NULL);
   if (s_listener != NULL)
      s_listener->start();
}

/**
 * Retrieve push value from local cache
 */
LONG H_PushValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR key[MAX_RUNTIME_PARAM_NAME];
   if (!AgentGetParameterArg(cmd, 1, key, MAX_RUNTIME_PARAM_NAME))
      return SYSINFO_RC_UNSUPPORTED;

   s_localCacheLock.lock();
   const PushDataEntry *v = s_localCache.get(key);
   if (v != NULL)
      _tcscpy(value, v->value);
   s_localCacheLock.unlock();
   return (v != NULL) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Get list of all available push values
 */
LONG H_PushValues(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   s_localCacheLock.lock();
   StringList *keys = s_localCache.keys();
   value->addAll(keys);
   s_localCacheLock.unlock();
   delete keys;
   return SYSINFO_RC_SUCCESS;
}
