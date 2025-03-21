/*
** NetXMS SSH subagent
** Copyright (C) 2021-2025 Raden Solutions
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

static SharedStringObjectMap<KeyPair> s_keyList;
static Mutex s_mutex;

/**
 * Constructor for key pair
 */
KeyPair::KeyPair(char *privateKey, char *publicKey)
{
   pubKeySource = publicKey;
   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey: public key %hs"), pubKeySource);
   char *tmp = strchr(pubKeySource, ' ');
   if (tmp != nullptr)
   {
      *tmp = 0;
      type = ssh_key_type_from_name(pubKeySource);
      this->publicKey = tmp + 1;
   }

   tmp = strchr(publicKey, ' ');
   if (tmp != nullptr)
   {
      *tmp = 0;
   }

   this->privateKey = privateKey;
   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey: private key %hs"), privateKey);
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
 * Gets from cache or loads form server SSH keys
 */
shared_ptr<KeyPair> GetSshKey(AbstractCommSession *session, uint32_t id)
{
   if (id == 0)
      return shared_ptr<KeyPair>();

   TCHAR dataKey[64];
   _sntprintf(dataKey, 64, UINT64X_FMT(_T("016")) _T("_%d"), session->getServerId(), id);
   nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey(%d): find key"), id);

   s_mutex.lock();
   shared_ptr<KeyPair> keys = s_keyList.getShared(dataKey);
   s_mutex.unlock();
   if (keys == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 9, _T("GetSshKey(%d): request key"), id);
      NXCPMessage request(session->getProtocolVersion());
      request.setId(session->generateRequestId());
      request.setCode(CMD_GET_SSH_KEYS);
      request.setField(VID_SSH_KEY_ID, id);
      session->sendMessage(&request);
      NXCPMessage *response = session->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 5000);
      if(response != nullptr)
      {
         uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            keys = make_shared<KeyPair>(response->getFieldAsMBString(VID_PRIVATE_KEY, nullptr, 0),
                  response->getFieldAsMBString(VID_PUBLIC_KEY, nullptr, 0));
            s_mutex.lock();
            s_keyList.set(dataKey, keys);
            s_mutex.unlock();
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey: failed to load keys with ID %u (RCC = %u)"), id, rcc);
         }
         delete response;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("GetSshKey: failed to load keys with ID %u"), id);
      }
   }

   return keys;
}
