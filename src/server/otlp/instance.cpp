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
** File: instance.cpp
**
**/

#include "otlp.h"
#include <uthash.h>

/**
 * Short-circuit cache entry
 */
struct InstanceCacheEntry
{
   UT_hash_handle hh;
   BYTE key[SHA256_DIGEST_SIZE];
   uint32_t dciId;
   uint32_t nodeId;
};

static InstanceCacheEntry *s_instanceCache = nullptr;
static Mutex s_instanceCacheLock;

/**
 * Initialize instance discovery engine
 */
void InitInstanceDiscovery()
{
   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP instance discovery engine initialized");
}

/**
 * Shutdown instance discovery engine
 */
void ShutdownInstanceDiscovery()
{
   s_instanceCacheLock.lock();
   InstanceCacheEntry *entry, *tmp;
   HASH_ITER(hh, s_instanceCache, entry, tmp)
   {
      HASH_DEL(s_instanceCache, entry);
      MemFree(entry);
   }
   s_instanceCacheLock.unlock();
   nxlog_debug_tag(DEBUG_TAG_OTLP, 1, L"OTLP instance discovery engine shut down");
}

/**
 * Compute cache key from node ID, metric name, and instance key
 */
static void ComputeInstanceCacheKey(uint32_t nodeId, const char *metricName, const wchar_t *instanceKey, BYTE *key)
{
   SHA256_STATE ctx;
   SHA256Init(&ctx);
   SHA256Update(&ctx, &nodeId, sizeof(nodeId));
   SHA256Update(&ctx, metricName, strlen(metricName));
   size_t ikLen = wcslen(instanceKey) * sizeof(wchar_t);
   SHA256Update(&ctx, instanceKey, ikLen);
   SHA256Final(&ctx, key);
}

/**
 * Invalidate instance cache entries for a given node
 */
void InvalidateInstanceCache(uint32_t nodeId)
{
   s_instanceCacheLock.lock();
   InstanceCacheEntry *entry, *tmp;
   HASH_ITER(hh, s_instanceCache, entry, tmp)
   {
      if (entry->nodeId == nodeId)
      {
         HASH_DEL(s_instanceCache, entry);
         MemFree(entry);
      }
   }
   s_instanceCacheLock.unlock();
   nxlog_debug_tag(DEBUG_TAG_OTLP, 5, L"Invalidated instance cache for node [%u]", nodeId);
}

/**
 * Parse OTLP discovery config from instanceDiscoveryData string.
 * Format: metric_pattern[;key_attr1,key_attr2[;name_format]]
 * Key attributes and name format are optional. Metric name is always
 * implicitly included as the first component of the instance key.
 */
static bool ParseOTLPDiscoveryConfig(const wchar_t *data, OTLPDiscoveryConfig *config)
{
   if ((data == nullptr) || (*data == 0))
      return false;

   // Split on ';'
   StringList parts(data, L";");

   // Part 1: metric pattern (required)
   wcsncpy(config->metricPattern, parts.get(0), 511);
   config->metricPattern[511] = 0;

   // Part 2: key attributes (optional, comma-separated)
   config->keyAttributes.clear();
   if (parts.size() >= 2)
   {
      StringList attrs(parts.get(1), L",");
      for (int i = 0; i < attrs.size(); i++)
      {
         const wchar_t *attr = attrs.get(i);
         if (*attr != 0)
            config->keyAttributes.add(attr);
      }
   }

   // Part 3: name format (optional)
   if (parts.size() >= 3)
   {
      wcsncpy(config->nameFormat, parts.get(2), 511);
      config->nameFormat[511] = 0;
   }
   else
   {
      config->nameFormat[0] = 0;
   }

   return true;
}

/**
 * Build instance key from metric name and data point attributes.
 * Metric name is always the first component, followed by colon-separated attribute values.
 * When key attributes are specified, only those are used. When empty, all data point
 * attributes are included (sorted by name) to avoid collapsing distinct data points.
 * Example: "jvm.memory.used:heap:G1 Eden Space" or just "jvm.threads.count" (no attributes).
 */
static String BuildInstanceKey(const char *metricName, const OTLPDiscoveryConfig& config, const std::map<std::string, std::string>& dpAttributes)
{
   StringBuffer key;

   // Metric name is always the first component
   wchar_t wmetric[256];
   utf8_to_wchar(metricName, -1, wmetric, 256);
   key.append(wmetric);

   if (config.keyAttributes.size() > 0)
   {
      // Use only specified key attributes
      for (int i = 0; i < config.keyAttributes.size(); i++)
      {
         key.append(L':');

         char attrName[256];
         wchar_to_utf8(config.keyAttributes.get(i), -1, attrName, 256);

         auto it = dpAttributes.find(attrName);
         if (it != dpAttributes.end())
         {
            wchar_t wval[256];
            utf8_to_wchar(it->second.c_str(), -1, wval, 256);
            key.append(wval);
         }
      }
   }
   else
   {
      // No key attributes configured - include all data point attributes
      // std::map is already sorted by key name
      for (const auto& kv : dpAttributes)
      {
         key.append(L':');
         wchar_t wval[256];
         utf8_to_wchar(kv.second.c_str(), -1, wval, 256);
         key.append(wval);
      }
   }
   return key;
}

/**
 * Format instance display name from format string and data point attributes.
 * Replaces %{attr_name} placeholders with actual attribute values.
 * If format is empty, returns the instance key.
 */
static String FormatInstanceName(const wchar_t *format, const std::map<std::string, std::string>& dpAttributes, const wchar_t *instanceKey)
{
   if ((format == nullptr) || (*format == 0))
      return String(instanceKey);

   StringBuffer result(format);

   for (const auto& kv : dpAttributes)
   {
      wchar_t placeholder[280];
      wchar_t wkey[256];
      utf8_to_wchar(kv.first.c_str(), -1, wkey, 256);
      nx_swprintf(placeholder, 280, L"%%{%s}", wkey);

      wchar_t wval[256];
      utf8_to_wchar(kv.second.c_str(), -1, wval, 256);
      result.replace(placeholder, wval);
   }

   return result;
}

/**
 * Resolve an OTel data point to an instance DCI
 */
shared_ptr<DCObject> ResolveOTelInstance(const shared_ptr<Node>& node,
   const char *metricName,
   const std::map<std::string, std::string>& dpAttributes)
{
   // Find all IDM_OTLP prototypes on this node
   SharedObjectArray<DCObject> prototypes = node->getDCObjectsByFilter(
      [](DCObject *dco) -> bool {
         return (dco->getInstanceDiscoveryMethod() == IDM_OTLP) && (dco->getStatus() != ITEM_STATUS_DISABLED);
      });

   if (prototypes.isEmpty())
      return shared_ptr<DCObject>();

   wchar_t wmetric[256];
   utf8_to_wchar(metricName, -1, wmetric, 256);

   for (int i = 0; i < prototypes.size(); i++)
   {
      DCObject *proto = prototypes.get(i);

      // Parse config
      OTLPDiscoveryConfig config;
      SharedString instData = proto->getInstanceDiscoveryData();
      if (!ParseOTLPDiscoveryConfig(instData.cstr(), &config))
         continue;

      // Match metric name against pattern
      if (!MatchStringW(config.metricPattern, wmetric, false))
         continue;

      // Build instance key from metric name and data point attributes
      String instanceKey = BuildInstanceKey(metricName, config, dpAttributes);

      // Check short-circuit cache
      BYTE cacheKey[SHA256_DIGEST_SIZE];
      ComputeInstanceCacheKey(node->getId(), metricName, instanceKey.cstr(), cacheKey);

      s_instanceCacheLock.lock();
      InstanceCacheEntry *cached = nullptr;
      HASH_FIND(hh, s_instanceCache, cacheKey, SHA256_DIGEST_SIZE, cached);
      if (cached != nullptr)
      {
         uint32_t dciId = cached->dciId;
         s_instanceCacheLock.unlock();

         shared_ptr<DCObject> dco = node->getDCObjectById(dciId, 0);
         if (dco != nullptr && dco->getStatus() == ITEM_STATUS_ACTIVE)
         {
            nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"Instance cache hit for metric %hs instance \"%s\" -> DCI %u",
               metricName, instanceKey.cstr(), dciId);
            return dco;
         }
         // DCI was deleted or disabled, remove stale cache entry
         s_instanceCacheLock.lock();
         HASH_FIND(hh, s_instanceCache, cacheKey, SHA256_DIGEST_SIZE, cached);
         if (cached != nullptr)
         {
            HASH_DEL(s_instanceCache, cached);
            MemFree(cached);
         }
         s_instanceCacheLock.unlock();
      }
      else
      {
         s_instanceCacheLock.unlock();
      }

      // Check if instance DCI already exists
      uint32_t protoId = proto->getId();
      const wchar_t *ik = instanceKey.cstr();
      shared_ptr<DCObject> existing = node->getDCObjectByFilter(
         [protoId, ik](DCObject *dco) -> bool {
            return (dco->getTemplateItemId() == protoId) &&
                   (dco->getInstanceDiscoveryMethod() == IDM_NONE) &&
                   !wcscmp(dco->getInstanceDiscoveryData(), ik);
         });

      if (existing != nullptr)
      {
         // Add to cache and return
         s_instanceCacheLock.lock();
         InstanceCacheEntry *entry = MemAllocStruct<InstanceCacheEntry>();
         memcpy(entry->key, cacheKey, SHA256_DIGEST_SIZE);
         entry->dciId = existing->getId();
         entry->nodeId = node->getId();
         HASH_ADD(hh, s_instanceCache, key, SHA256_DIGEST_SIZE, entry);
         s_instanceCacheLock.unlock();

         nxlog_debug_tag(DEBUG_TAG_OTLP, 6, L"Found existing instance DCI %u for metric %hs instance \"%s\" on node %s [%u]",
            existing->getId(), metricName, instanceKey.cstr(), node->getName(), node->getId());
         return existing;
      }

      // Format instance name
      String instanceName = FormatInstanceName(config.nameFormat, dpAttributes, instanceKey.cstr());

      // Run instance filter
      StringMap instances;
      instances.set(instanceKey.cstr(), instanceName.cstr());
      StringObjectMap<InstanceDiscoveryData> *filtered = proto->filterInstanceList(&instances);
      if ((filtered == nullptr) || filtered->isEmpty())
      {
         delete filtered;
         nxlog_debug_tag(DEBUG_TAG_OTLP, 5, L"Instance \"%s\" filtered out for OTel prototype DCI %u on node %s [%u]",
            instanceKey.cstr(), proto->getId(), node->getName(), node->getId());
         continue;
      }

      // Create new instance DCI
      StringList filteredKeys = filtered->keys();
      const InstanceDiscoveryData *idd = filtered->get(filteredKeys.get(0));

      DCObject *newDco = proto->clone();
      newDco->setTemplateId(node->getId(), proto->getId());
      newDco->setInstanceName(idd->getInstanceName());
      newDco->setInstanceDiscoveryMethod(IDM_NONE);
      newDco->setInstanceDiscoveryData(instanceKey.cstr());
      newDco->setInstanceFilter(nullptr);
      newDco->expandInstance();
      newDco->setRelatedObject(idd->getRelatedObject());
      newDco->changeBinding(CreateUniqueId(IDG_ITEM), static_pointer_cast<DataCollectionOwner>(node), false);
      node->addDCObject(newDco, false, true);

      delete filtered;

      // Get shared_ptr for newly added DCI and cache it
      shared_ptr<DCObject> result = node->getDCObjectById(newDco->getId(), 0);
      if (result != nullptr)
      {
         s_instanceCacheLock.lock();
         InstanceCacheEntry *entry = MemAllocStruct<InstanceCacheEntry>();
         memcpy(entry->key, cacheKey, SHA256_DIGEST_SIZE);
         entry->dciId = result->getId();
         entry->nodeId = node->getId();
         HASH_ADD(hh, s_instanceCache, key, SHA256_DIGEST_SIZE, entry);
         s_instanceCacheLock.unlock();

         nxlog_debug_tag(DEBUG_TAG_OTLP, 4, L"Created OTel instance DCI %u (instance=\"%s\", name=\"%s\") from prototype %u on node %s [%u]",
            result->getId(), instanceKey.cstr(), result->getName().cstr(), proto->getId(), node->getName(), node->getId());
      }

      return result;
   }

   return shared_ptr<DCObject>();
}
