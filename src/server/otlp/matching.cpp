/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: matching.cpp
**
**/

#include "otlp.h"
#include <uthash.h>

/**
 * Matching rules loaded from database
 */
static ObjectArray<OTLPMatchingRule> s_matchingRules(16, 16, Ownership::True);
static Mutex s_matchingRulesLock;

/**
 * Match cache: hash of resource attributes -> node ID
 */
struct MatchCacheEntry
{
   UT_hash_handle hh;
   BYTE key[SHA256_DIGEST_SIZE];
   uint32_t nodeId;
   time_t timestamp;
};
static MatchCacheEntry *s_matchCache = nullptr;
static Mutex s_matchCacheLock;

/**
 * Set new entry in match cache
 */
static void SetMatchCacheEntry(const BYTE *key, uint32_t nodeId)
{
   MatchCacheEntry *entry;
   HASH_FIND(hh, s_matchCache, key, SHA256_DIGEST_SIZE, entry);
   if (entry == nullptr)
   {
      entry = MemAllocStruct<MatchCacheEntry>();
      memcpy(entry->key, key, SHA256_DIGEST_SIZE);
      HASH_ADD_KEYPTR(hh, s_matchCache, entry->key, SHA256_DIGEST_SIZE, entry);
   }
   entry->nodeId = nodeId;
   entry->timestamp = time(nullptr);
}

/**
 * Get match from cache
 */
static MatchCacheEntry *GetMatchCacheEntry(const BYTE *key)
{
   MatchCacheEntry *entry;
   HASH_FIND(hh, s_matchCache, key, SHA256_DIGEST_SIZE, entry);
   return entry;
}

/**
 * Remove entry from match cache
 */
static void RemoveMatchCacheEntry(const BYTE *key)
{
   MatchCacheEntry *entry;
   HASH_FIND(hh, s_matchCache, key, SHA256_DIGEST_SIZE, entry);
   if (entry != nullptr)
   {
      HASH_DEL(s_matchCache, entry);
      MemFree(entry);
   }
}

/**
 * Clear match cache
 */
static void ClearMatchCache()
{
   MatchCacheEntry *entry, *tmp;
   HASH_ITER(hh, s_matchCache, entry, tmp)
   {
      HASH_DEL(s_matchCache, entry);
      MemFree(entry);
   }
}

/**
 * Comparison function for sorting matching rules by sequence number
 */
static int CompareMatchingRules(const OTLPMatchingRule **a, const OTLPMatchingRule **b)
{
   return (*a)->sequenceNumber - (*b)->sequenceNumber;
}

/**
 * Load matching rules from database
 */
static void LoadMatchingRules()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, L"SELECT id,sequence_number,otel_attribute,regex_transform,node_property,custom_attr_name FROM otlp_matching_rules ORDER BY sequence_number");
   if (hResult != nullptr)
   {
      s_matchingRulesLock.lock();
      s_matchingRules.clear();

      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         auto rule = new OTLPMatchingRule();
         rule->id = DBGetFieldULong(hResult, i, 0);
         rule->sequenceNumber = DBGetFieldLong(hResult, i, 1);
         DBGetFieldA(hResult, i, 2, rule->otelAttribute, sizeof(rule->otelAttribute));
         DBGetFieldA(hResult, i, 3, rule->regexTransform, sizeof(rule->regexTransform));
         rule->nodeProperty = static_cast<OTLPNodeProperty>(DBGetFieldLong(hResult, i, 4));
         DBGetField(hResult, i, 5, rule->customAttrName, sizeof(rule->customAttrName) / sizeof(TCHAR));
         s_matchingRules.add(rule);
      }

      s_matchingRules.sort(CompareMatchingRules);
      s_matchingRulesLock.unlock();

      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG_OTLP, 3, L"Loaded %d OTLP matching rules", count);
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_OTLP, L"Failed to load OTLP matching rules from database");
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Compute cache key from resource attributes
 */
static void ComputeCacheKey(const std::map<std::string, std::string>& attributes, BYTE *key)
{
   SHA256_STATE state;
   SHA256Init(&state);
   for (auto it = attributes.begin(); it != attributes.end(); ++it)
   {
      SHA256Update(&state, it->first.c_str(), it->first.length());
      SHA256Update(&state, "=", 1);
      SHA256Update(&state, it->second.c_str(), it->second.length());
      SHA256Update(&state, "\n", 1);
   }
   SHA256Final(&state, key);
}

/**
 * Try to match a node by name
 */
static shared_ptr<Node> MatchByNodeName(const char *value)
{
   wchar_t wvalue[256];
   utf8_to_wchar(value, -1, wvalue, 256);
   shared_ptr<NetObj> object = FindObjectByName(wvalue, OBJECT_NODE);
   return (object != nullptr) ? static_pointer_cast<Node>(object) : shared_ptr<Node>();
}

/**
 * Try to match a node by primary DNS name
 */
static shared_ptr<Node> MatchByDnsName(const char *value)
{
   wchar_t wvalue[256];
   utf8_to_wchar(value, -1, wvalue, 256);
   shared_ptr<NetObj> object = g_idxNodeById.find(
      [wvalue](NetObj *o) -> bool
      {
         return !wcsicmp(static_cast<Node*>(o)->getPrimaryHostName().cstr(), wvalue);
      });
   return (object != nullptr) ? static_pointer_cast<Node>(object) : shared_ptr<Node>();
}

/**
 * Try to match a node by IP address
 */
static shared_ptr<Node> MatchByIpAddress(const char *value)
{
   InetAddress addr = InetAddress::parse(value);
   if (!addr.isValid())
      return shared_ptr<Node>();
   return FindNodeByIP(ALL_ZONES, addr);
}

/**
 * Try to match a node by custom attribute
 */
static shared_ptr<Node> MatchByCustomAttribute(const char *value, const TCHAR *attrName)
{
   wchar_t wvalue[256];
   utf8_to_wchar(value, -1, wvalue, 256);
   shared_ptr<NetObj> object = g_idxNodeById.find(
      [wvalue, attrName](NetObj *o) -> bool
      {
         SharedString av = o->getCustomAttribute(attrName);
         return !_tcsicmp(av.cstr(), wvalue);
      });
   return (object != nullptr) ? static_pointer_cast<Node>(object) : shared_ptr<Node>();
}

/**
 * Initialize matching engine
 */
void InitMatchingEngine()
{
   LoadMatchingRules();
}

/**
 * Shutdown matching engine
 */
void ShutdownMatchingEngine()
{
   s_matchCacheLock.lock();
   ClearMatchCache();
   s_matchCacheLock.unlock();
}

/**
 * Reload matching rules from database
 */
void ReloadMatchingRules()
{
   LoadMatchingRules();

   // Invalidate match cache when rules change
   s_matchCacheLock.lock();
   ClearMatchCache();
   s_matchCacheLock.unlock();

   nxlog_debug_tag(DEBUG_TAG_OTLP, 3, L"OTLP matching rules reloaded, match cache cleared");
}

/**
 * Match OTel resource attributes to a NetXMS node
 */
shared_ptr<Node> MatchResourceToNode(const std::map<std::string, std::string>& attributes)
{
   // Check match cache first
   BYTE cacheKey[SHA256_DIGEST_SIZE];
   ComputeCacheKey(attributes, cacheKey);
   s_matchCacheLock.lock();
   MatchCacheEntry *cached = GetMatchCacheEntry(cacheKey);
   if (cached != nullptr)
   {
      uint32_t ttl = ConfigReadULong(L"OTLP.MatchCacheTTL", 300);
      if (static_cast<uint32_t>(time(nullptr) - cached->timestamp) < ttl)
      {
         uint32_t nodeId = cached->nodeId;
         s_matchCacheLock.unlock();
         return static_pointer_cast<Node>(FindObjectById(nodeId, OBJECT_NODE));
      }
      // TTL expired, remove entry
      RemoveMatchCacheEntry(cacheKey);
   }
   s_matchCacheLock.unlock();

   // Try matching rules
   shared_ptr<Node> node;
   s_matchingRulesLock.lock();
   for (int i = 0; i < s_matchingRules.size(); i++)
   {
      OTLPMatchingRule *rule = s_matchingRules.get(i);

      // Get attribute value from resource
      auto it = attributes.find(rule->otelAttribute);
      if (it == attributes.end())
         continue;
      const std::string& value = it->second;

      // TODO: Apply regex transform if configured (rule->regexTransform)

      switch (rule->nodeProperty)
      {
         case OTLP_MATCH_NODE_NAME:
            node = MatchByNodeName(value.c_str());
            break;
         case OTLP_MATCH_DNS_NAME:
            node = MatchByDnsName(value.c_str());
            break;
         case OTLP_MATCH_IP_ADDRESS:
            node = MatchByIpAddress(value.c_str());
            break;
         case OTLP_MATCH_CUSTOM_ATTRIBUTE:
            node = MatchByCustomAttribute(value.c_str(), rule->customAttrName);
            break;
      }

      if (node != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Matched OTel resource to node %s [%u] via rule %u (attribute %hs -> %s)",
            node->getName(), node->getId(), rule->id, rule->otelAttribute,
            (rule->nodeProperty == OTLP_MATCH_NODE_NAME) ? L"node name" :
            (rule->nodeProperty == OTLP_MATCH_DNS_NAME) ? L"DNS name" :
            (rule->nodeProperty == OTLP_MATCH_IP_ADDRESS) ? L"IP address" : L"custom attribute");
         break;
      }
   }
   s_matchingRulesLock.unlock();

   // Update cache
   if (node != nullptr)
   {
      s_matchCacheLock.lock();
      SetMatchCacheEntry(cacheKey,  node->getId());
      s_matchCacheLock.unlock();
   }
   else if (ConfigReadBoolean(L"OTLP.LogUnmatchedResources", false) && (nxlog_get_debug_level_tag(DEBUG_TAG_OTLP) >= 3))
   {
      std::string attrList;
      for (const auto& kv : attributes)
      {
         if (!attrList.empty())
            attrList += ", ";
         attrList += kv.first + "=" + kv.second;
      }
      nxlog_debug_tag(DEBUG_TAG_OTLP, 3, L"No matching node found for OTel resource %hs", attrList.c_str());
   }

   return node;
}
