/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("dc.push")

/**
 * Request ID
 */
static uint64_t s_requestIdHigh = static_cast<uint64_t>(time(nullptr)) << 32;
static VolatileCounter s_requestIdLow = 0;

/**
 * Push parameter's data
 */
bool PushData(const TCHAR *parameter, const TCHAR *value, uint32_t objectId, Timestamp timestamp)
{
	bool success = false;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("PushData: \"%s\" = \"%s\""), parameter, value);

	NXCPMessage msg(CMD_PUSH_DCI_DATA, 0, 4); // Use version 4 to avoid compatibility issues
	msg.setField(VID_NAME, parameter);
	msg.setField(VID_VALUE, value);
   msg.setField(VID_OBJECT_ID, objectId);
   msg.setField(VID_TIMESTAMP_MS, timestamp);
   msg.setFieldFromTime(VID_TIMESTAMP, timestamp.asTime());   // for compatibility with older servers
   msg.setField(VID_REQUEST_ID, s_requestIdHigh | static_cast<uint64_t>(InterlockedIncrement(&s_requestIdLow)));

   if (g_dwFlags & AF_SUBAGENT_LOADER)
   {
      success = SendMessageToMasterAgent(&msg);
   }
   else
   {
      g_sessionLock.lock();
      for(int i = 0; i < g_sessions.size(); i++)
      {
         CommSession *session = g_sessions.get(i);
         if (session->canAcceptTraps())
         {
            session->sendMessage(&msg);
            success = true;
         }
      }
      g_sessionLock.unlock();
   }
	return success;
}

/**
 * Pushed value
 */
struct PushDataEntry
{
   Timestamp timestamp;
   TCHAR value[MAX_RESULT_LENGTH];

   PushDataEntry(Timestamp t, const TCHAR *v) : timestamp(t)
   {
      _tcscpy(value, v);
   }
};

/**
 * Local cache
 */
static ObjectMemoryPool<PushDataEntry> s_localCachePool;
static StringObjectMap<PushDataEntry> s_localCache(Ownership::False);
static Mutex s_localCacheLock(MutexType::FAST);

/**
 * Store pushed data locally
 */
static void StoreLocalData(const TCHAR *parameter, const TCHAR *value, uint32_t objectId, Timestamp timestamp)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("StoreLocalData: \"%s\" = \"%s\""), parameter, value);
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

	nxlog_debug_tag(DEBUG_TAG, 5, _T("ProcessPushRequest: connection established"));
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
	while(true)
	{
      MessageReceiverResult result;
		NXCPMessage *msg = receiver.readMessage(5000, &result);
		if (msg == nullptr)
			break;
		nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessPushRequest: received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
		if (msg->getCode() == CMD_PUSH_DCI_DATA)
		{
         uint32_t objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
         uint32_t count = msg->getFieldAsUInt32(VID_NUM_ITEMS);
         Timestamp timestamp = msg->getFieldAsTimestamp(VID_TIMESTAMP_MS);
         if (timestamp.isNull())
            timestamp = Timestamp::fromTime(msg->getFieldAsTime(VID_TIMESTAMP));
         bool localCache = msg->getFieldAsBoolean(VID_LOCAL_CACHE);
         uint32_t fieldId = VID_PUSH_DCI_DATA_BASE;
			for(uint32_t i = 0; i < count; i++)
			{
				TCHAR name[MAX_RUNTIME_PARAM_NAME], value[MAX_RESULT_LENGTH];
				msg->getFieldAsString(fieldId++, name, MAX_RUNTIME_PARAM_NAME);
				msg->getFieldAsString(fieldId++, value, MAX_RESULT_LENGTH);
				if (localCache)
				   StoreLocalData(name, value, objectId, timestamp);
				else
				   PushData(name, value, objectId, timestamp);
			}
		}
		delete msg;
	}
	nxlog_debug_tag(DEBUG_TAG, 5, _T("ProcessPushRequest: connection by user %s closed"), pipe->user());
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
   shared_ptr<Config> config = g_config;
   const TCHAR *user = config->getValue(_T("/%agent/PushUser"), _T("*"));
   s_listener = NamedPipeListener::create(_T("nxagentd.push"), ProcessPushRequest, nullptr,
            (user != nullptr) && (user[0] != 0) && _tcscmp(user, _T("*")) ? user : nullptr);
   if (s_listener != nullptr)
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
   if (v != nullptr)
      _tcscpy(value, v->value);
   s_localCacheLock.unlock();
   return (v != nullptr) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Get list of all available push values
 */
LONG H_PushValues(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   s_localCacheLock.lock();
   value->addAll(s_localCache.keys());
   s_localCacheLock.unlock();
   return SYSINFO_RC_SUCCESS;
}
