/*
** NetXMS NETCONF subagent
** Copyright (C) 2026 Raden Solutions
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
** File: cache.cpp
**/

#include "netconf_subagent.h"

/**
 * Cached NETCONF document (content of data element from get/get-config reply)
 */
class CachedDocument
{
private:
   Mutex m_mutex;
   time_t m_timestamp;
   pugi::xml_document m_document;

public:
   CachedDocument() : m_mutex(MutexType::NORMAL)
   {
      m_timestamp = 0;
   }

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   bool isExpired(uint32_t retentionTime) const { return (time(nullptr) - m_timestamp) >= retentionTime; }
   time_t getTimestamp() const { return m_timestamp; }

   /**
    * Update document from raw rpc-reply. Content of data element is re-rooted so XPath
    * expressions address it as /data/... regardless of rpc-reply level structure.
    */
   uint32_t update(const char *reply, size_t size)
   {
      NETCONF_Response response;
      if (!response.parse(reply, size))
         return ERR_MALFORMED_RESPONSE;
      if (!response.isSuccess())
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("CachedDocument::update: device returned error (%s)"), response.getErrorText().cstr());
         return ERR_EXEC_FAILED;
      }

      pugi::xml_node data = response.getData();
      if (!data)
         return ERR_MALFORMED_RESPONSE;

      m_document.reset();
      pugi::xml_node root = m_document.append_child("data");
      for(pugi::xml_node child = data.first_child(); child; child = child.next_sibling())
         root.append_copy(child);
      m_timestamp = time(nullptr);
      return ERR_SUCCESS;
   }

   String getValueByXPath(const char *xpath) const
   {
      return NETCONF_GetValueByXPath(m_document, xpath);
   }
};

/**
 * Document cache
 */
static SharedStringObjectMap<CachedDocument> s_documentCache;
static Mutex s_documentCacheLock(MutexType::FAST);

/**
 * Remove stale entries from document cache (called from housekeeping thread)
 */
void CleanDocumentCache()
{
   s_documentCacheLock.lock();
   s_documentCache.filterElements(
      [] (const TCHAR *key, const void *value, void *context) -> bool
      {
         const shared_ptr<CachedDocument>& document = *static_cast<const shared_ptr<CachedDocument>*>(value);
         if (!document->isExpired(g_netconfCacheExpirationTime))
            return true;
         nxlog_debug_tag(DEBUG_TAG, 7, _T("CleanDocumentCache: cache entry \"%s\" removed"), key);
         return false;
      }, nullptr);
   s_documentCacheLock.unlock();
}

/**
 * Process capability request within CMD_NETCONF_QUERY (connects to device and returns
 * capability list announced in hello message)
 */
static void ProcessCapabilityRequest(const InetAddress& addr, uint16_t port, const TCHAR *user, const TCHAR *password,
      const shared_ptr<KeyPair>& keys, NXCPMessage *response)
{
   NETCONFSession *netconfSession = AcquireSession(addr, port, user, password, keys);
   if (netconfSession == nullptr)
   {
      response->setField(VID_RCC, ERR_REMOTE_CONNECT_FAILED);
      return;
   }

   netconfSession->getPeerCapabilities().getAll().fillMessage(response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
   response->setField(VID_SESSION_ID, netconfSession->getPeerSessionId());
   response->setField(VID_RCC, ERR_SUCCESS);
   ReleaseSession(netconfSession);
}

/**
 * Process CMD_NETCONF_QUERY request. Extracts values by XPath from device document
 * retrieved with get (datastore not set) or get-config, using agent-side document cache
 * keyed by target + datastore + filter with per-request retention time. Capability
 * requests (VID_REQUEST_TYPE = 1) bypass the cache and return device capability list.
 */
void QueryNetconfDocument(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   InetAddress addr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
   uint16_t port = request.getFieldAsUInt16(VID_PORT);
   if (port == 0)
      port = NETCONF_DEFAULT_PORT;

   TCHAR user[256], password[256];
   request.getFieldAsString(VID_USER_NAME, user, 256);
   request.getFieldAsString(VID_PASSWORD, password, 256);

   uint32_t keyId = request.getFieldAsUInt32(VID_SSH_KEY_ID);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   if (request.getFieldAsUInt16(VID_REQUEST_TYPE) == static_cast<uint16_t>(NetconfRequestType::CAPABILITIES))
   {
      ProcessCapabilityRequest(addr, port, user, password, keys, response);
      return;
   }

   uint32_t timeout = request.getFieldAsUInt32(VID_TIMEOUT);
   if (timeout == 0)
      timeout = g_netconfRequestTimeout;
   uint32_t retentionTime = request.getFieldAsUInt32(VID_RETENTION_TIME);
   int datastore = request.getFieldAsInt16(VID_DATASTORE);
   int filterType = request.getFieldAsInt16(VID_FILTER_TYPE);
   char *filter = request.getFieldAsUtf8String(VID_FILTER);

   if ((filterType < 0) || (filterType > 2) || (datastore < 0) || (datastore > 3))
   {
      MemFree(filter);
      response->setField(VID_RCC, ERR_MALFORMED_COMMAND);
      return;
   }

   // Cache key: target + datastore + hash of filter
   TCHAR ipAddrText[64];
   BYTE filterHash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(CHECK_NULL_EX_A(filter), (filter != nullptr) ? strlen(filter) : 0, filterHash);
   char filterHashText[SHA1_DIGEST_SIZE * 2 + 1];
   BinToStrA(filterHash, SHA1_DIGEST_SIZE, filterHashText);
   TCHAR key[256];
   _sntprintf(key, 256, _T("%s:%u/%s/%d/%d/%hs"), addr.toString(ipAddrText), port, user, datastore, filterType, filterHashText);

   s_documentCacheLock.lock();
   shared_ptr<CachedDocument> document = s_documentCache.getShared(key);
   if (document == nullptr)
   {
      document = make_shared<CachedDocument>();
      s_documentCache.set(key, document);
   }
   s_documentCacheLock.unlock();

   document->lock();

   uint32_t rcc = ERR_SUCCESS;
   if (document->isExpired(retentionTime))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("QueryNetconfDocument: refreshing document \"%s\""), key);
      NETCONFSession *netconfSession = AcquireSession(addr, port, user, password, keys);
      if (netconfSession != nullptr)
      {
         size_t replySize;
         char *reply = netconfSession->executeGetRequest(datastore - 1, static_cast<NetconfFilterType>(filterType), filter, timeout, &replySize);
         if (reply != nullptr)
         {
            rcc = document->update(reply, replySize);
            MemFree(reply);
            ReleaseSession(netconfSession);
         }
         else
         {
            rcc = ERR_EXEC_FAILED;
            ReleaseSession(netconfSession, true);
         }
      }
      else
      {
         rcc = ERR_REMOTE_CONNECT_FAILED;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("QueryNetconfDocument: using cached document \"%s\" (timestamp %u)"), key, static_cast<uint32_t>(document->getTimestamp()));
   }

   if (rcc == ERR_SUCCESS)
   {
      // Extract values for requested XPath expressions; only successfully extracted
      // values are returned as (path, value) pairs
      int count = request.getFieldAsInt32(VID_NUM_PARAMETERS);
      uint32_t fieldId = VID_PARAM_LIST_BASE;
      int resultCount = 0;
      for(int i = 0; i < count; i++)
      {
         char xpath[1024];
         request.getFieldAsUtf8String(VID_PARAM_LIST_BASE + i, xpath, 1024);
         String value = document->getValueByXPath(xpath);
         if (!value.isEmpty())
         {
            response->setFieldFromUtf8String(fieldId++, xpath);
            response->setField(fieldId++, value.cstr());
            resultCount++;
         }
      }
      response->setField(VID_NUM_PARAMETERS, static_cast<int32_t>(resultCount));
   }

   document->unlock();
   MemFree(filter);
   response->setField(VID_RCC, rcc);
}
