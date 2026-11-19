/*
** NetXMS SSH subagent
** Copyright (C) 2021-2026 Raden Solutions
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
** File: keys.cpp
**/

#include "ssh_subagent.h"

/**
 * Time in seconds after which cached key is re-validated with server
 */
#define KEY_CACHE_TTL               300

/**
 * Time in seconds before next attempt to re-validate cached key after failed or in-progress attempt
 */
#define KEY_REFRESH_RETRY_INTERVAL  60

/**
 * Key cache entry
 */
struct KeyCacheEntry
{
   shared_ptr<KeyPair> keys;
   time_t refreshTime;

   KeyCacheEntry(const shared_ptr<KeyPair>& _keys, time_t _refreshTime) : keys(_keys), refreshTime(_refreshTime) { }
};

/**
 * Key cache, indexed by server ID and key ID
 */
static StringObjectMap<KeyCacheEntry> s_keyCache(Ownership::True);
static Mutex s_mutex;

/**
 * Constructor for key pair
 */
KeyPair::KeyPair(char *privateKey, char *publicKey)
{
   this->privateKey = privateKey;
   this->publicKey = nullptr;
   pubKeySource = publicKey;
   type = SSH_KEYTYPE_UNKNOWN;

   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey: public key %hs"), CHECK_NULL_A(pubKeySource));
   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey: private key %s"), (privateKey != nullptr) ? _T("present") : _T("missing"));

   // Public key is expected in OpenSSH one-line format: "<type> <base64 body> [comment]".
   // Type and body are addressed in place within pubKeySource; comment is cut off.
   if (pubKeySource == nullptr)
      return;

   char *keyBody = strchr(pubKeySource, ' ');
   if (keyBody == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey: public key has no type prefix, key authentication will not be attempted"));
      return;
   }
   *keyBody++ = 0;
   type = ssh_key_type_from_name(pubKeySource);

   char *comment = strchr(keyBody, ' ');
   if (comment != nullptr)
      *comment = 0;
   this->publicKey = keyBody;
}

/**
 * Destructor for key pair
 */
KeyPair::~KeyPair()
{
   MemFree(pubKeySource);
   MemFree(privateKey);
}

/**
 * Request SSH key from server over given session. Returns false if request cannot be sent over this session.
 * On successful send response is set to server's response or nullptr on timeout.
 */
static bool SendKeyRequest(AbstractCommSession *session, uint32_t id, NXCPMessage **response)
{
   NXCPMessage request(CMD_GET_SSH_KEYS, session->generateRequestId(), session->getProtocolVersion());
   request.setField(VID_SSH_KEY_ID, id);
   if (!session->sendMessage(&request))
      return false;
   *response = session->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 5000);
   return true;
}

/**
 * Request SSH key from server. Session used for locally cached data collection cannot carry requests,
 * so request is sent over any live session with same server in that case. Returns nullptr if server is not reachable.
 */
static NXCPMessage *RequestKeyFromServer(AbstractCommSession *session, uint32_t id)
{
   NXCPMessage *response = nullptr;
   if (!SendKeyRequest(session, id, &response) && (session->getServerId() != 0))
   {
      shared_ptr<AbstractCommSession> serverSession = AgentFindServerSession(session->getServerId());
      if ((serverSession != nullptr) && (serverSession.get() != session))
         SendKeyRequest(serverSession.get(), id, &response);
   }
   return response;
}

/**
 * Get SSH key from cache or load it from server. Cached key is re-validated with server after KEY_CACHE_TTL seconds;
 * key deleted on server is removed from cache, and cached copy is kept while server cannot be reached.
 */
shared_ptr<KeyPair> GetSshKey(AbstractCommSession *session, uint32_t id)
{
   if (id == 0)
      return shared_ptr<KeyPair>();

   TCHAR cacheKey[64];
   _sntprintf(cacheKey, 64, UINT64X_FMT(_T("016")) _T("_%u"), session->getServerId(), id);

   shared_ptr<KeyPair> keys;
   s_mutex.lock();
   KeyCacheEntry *entry = s_keyCache.get(cacheKey);
   if (entry != nullptr)
   {
      keys = entry->keys;
      time_t now = time(nullptr);
      if (entry->refreshTime > now)
      {
         s_mutex.unlock();
         return keys;
      }
      // Postpone next attempt so that concurrent callers use cached copy while this one refreshes it,
      // and failed refresh is not retried on every call
      entry->refreshTime = now + KEY_REFRESH_RETRY_INTERVAL;
   }
   s_mutex.unlock();

   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey(%u): request key from server"), id);
   NXCPMessage *response = RequestKeyFromServer(session, id);
   if (response == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey(%u): server is not reachable (%s)"), id, (keys != nullptr) ? _T("using cached key") : _T("key not available"));
      return keys;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      keys = make_shared<KeyPair>(response->getFieldAsMBString(VID_PRIVATE_KEY, nullptr, 0),
            response->getFieldAsMBString(VID_PUBLIC_KEY, nullptr, 0));
      s_mutex.lock();
      s_keyCache.set(cacheKey, new KeyCacheEntry(keys, time(nullptr) + KEY_CACHE_TTL));
      s_mutex.unlock();
   }
   else if (rcc == ERR_INVALID_SSH_KEY_ID)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey(%u): key does not exist on server"), id);
      keys.reset();
      s_mutex.lock();
      s_keyCache.remove(cacheKey);
      s_mutex.unlock();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey(%u): failed to load key (RCC = %u, %s)"), id, rcc, (keys != nullptr) ? _T("using cached key") : _T("key not available"));
   }
   delete response;
   return keys;
}
