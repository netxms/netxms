/*
** NetXMS - Network Management System
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
** File: location.cpp
**
** Location context of exported objects (facility, room, rack, zone, geolocation, power domains,
** cooling zone) as OTLP resource attributes. Shared by the metric exporter and the event forwarder.
**
**/

#include "otlp.h"
#include <algorithm>

#define MAX_ANCESTOR_DEPTH   16

/**
 * Location structure of an object: which facility, room, rack, power domains and cooling zone
 * apply, and which object's rack placement (position and height) refers to that rack. Only
 * structure is cached - names, rack position, zone, geolocation and power domain properties are
 * read live when attributes are built, so renames and property changes need no invalidation.
 * The cache is dropped as a whole whenever any parent list in the object tree changes.
 */
struct LocationContext
{
   weak_ptr<NetObj> facility;
   weak_ptr<NetObj> room;
   weak_ptr<NetObj> rack;
   weak_ptr<NetObj> placement;   // node or chassis mounted in the rack
   std::vector<weak_ptr<NetObj>> powerDomains;   // ordered by object ID
   weak_ptr<NetObj> coolingZone;
};

static Mutex s_cacheLock(MutexType::FAST);
static std::unordered_map<uint32_t, shared_ptr<LocationContext>> s_cache;
static uint32_t s_cacheVersion = 0;

/**
 * Keep candidate with lowest object ID (deterministic choice between ancestors at the same distance)
 */
static inline void SelectCandidate(shared_ptr<NetObj> *candidate, const shared_ptr<NetObj>& object)
{
   if ((*candidate == nullptr) || (object->getId() < (*candidate)->getId()))
      *candidate = object;
}

/**
 * Find object (node or chassis) whose rack placement refers to given rack
 */
static shared_ptr<NetObj> FindRackPlacementObject(const shared_ptr<NetObj>& object, uint32_t rackId)
{
   shared_ptr<NetObj> base = object;
   if (base->getObjectClass() == OBJECT_INTERFACE)
   {
      base = static_cast<Interface&>(*object).getParentNode();
      if (base == nullptr)
         return shared_ptr<NetObj>();
   }

   if (base->getObjectClass() == OBJECT_NODE)
   {
      uint32_t containerId = static_cast<Node&>(*base).getPhysicalContainerId();
      if (containerId == rackId)
         return base;
      shared_ptr<NetObj> chassis = FindObjectById(containerId, OBJECT_CHASSIS);
      return ((chassis != nullptr) && (static_cast<Chassis&>(*chassis).getRackId() == rackId)) ? chassis : shared_ptr<NetObj>();
   }

   if ((base->getObjectClass() == OBJECT_CHASSIS) && (static_cast<Chassis&>(*base).getRackId() == rackId))
      return base;

   return shared_ptr<NetObj>();
}

/**
 * Resolve location structure of given object by breadth-first walk over its ancestors (object itself included).
 * Nearest ancestor of each class wins; ties at the same distance are broken by lowest object ID. For power
 * domains, all domains at the nearest distance are taken (a dual-corded device has two), so a device bound
 * to power domains directly does not inherit the domains of its rack, and upstream domains of the power
 * chain are never included.
 */
static shared_ptr<LocationContext> ResolveLocationContext(const shared_ptr<NetObj>& object)
{
   shared_ptr<NetObj> facility, room, rack, coolingZone;
   std::vector<shared_ptr<NetObj>> powerDomains;

   std::vector<shared_ptr<NetObj>> level, nextLevel;
   HashSet<uint32_t> visited;
   level.push_back(object);
   visited.put(object->getId());
   for(int depth = 0; (depth < MAX_ANCESTOR_DEPTH) && !level.empty() &&
       ((facility == nullptr) || (room == nullptr) || (rack == nullptr) || powerDomains.empty() || (coolingZone == nullptr)); depth++)
   {
      shared_ptr<NetObj> facilityCandidate, roomCandidate, rackCandidate, coolingZoneCandidate;
      std::vector<shared_ptr<NetObj>> powerDomainCandidates;
      for(const shared_ptr<NetObj>& current : level)
      {
         switch(current->getObjectClass())
         {
            case OBJECT_FACILITY:
               SelectCandidate(&facilityCandidate, current);
               continue;   // Nothing location-related above facility
            case OBJECT_ROOM:
               SelectCandidate(&roomCandidate, current);
               break;
            case OBJECT_RACK:
               SelectCandidate(&rackCandidate, current);
               break;
            case OBJECT_POWERDOMAIN:
               powerDomainCandidates.push_back(current);
               break;
            case OBJECT_COOLINGZONE:
               SelectCandidate(&coolingZoneCandidate, current);
               break;
         }

         unique_ptr<SharedObjectArray<NetObj>> parents = current->getParents();
         for(int i = 0; i < parents->size(); i++)
         {
            const shared_ptr<NetObj>& parent = parents->getShared(i);
            if (!visited.contains(parent->getId()))
            {
               visited.put(parent->getId());
               nextLevel.push_back(parent);
            }
         }
      }

      if (facility == nullptr)
         facility = facilityCandidate;
      if (room == nullptr)
         room = roomCandidate;
      if (rack == nullptr)
         rack = rackCandidate;
      if (coolingZone == nullptr)
         coolingZone = coolingZoneCandidate;
      if (powerDomains.empty())
         powerDomains.swap(powerDomainCandidates);

      level.swap(nextLevel);
      nextLevel.clear();
   }

   auto context = make_shared<LocationContext>();
   context->facility = facility;
   context->room = room;
   context->rack = rack;
   if (rack != nullptr)
      context->placement = FindRackPlacementObject(object, rack->getId());
   context->coolingZone = coolingZone;
   std::sort(powerDomains.begin(), powerDomains.end(),
      [] (const shared_ptr<NetObj>& a, const shared_ptr<NetObj>& b) { return a->getId() < b->getId(); });
   context->powerDomains.assign(powerDomains.begin(), powerDomains.end());

   nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"Resolved location context for %s [%u]: facility=%u room=%u rack=%u coolingZone=%u powerDomains=%d",
      object->getName(), object->getId(), (facility != nullptr) ? facility->getId() : 0, (room != nullptr) ? room->getId() : 0,
      (rack != nullptr) ? rack->getId() : 0, (coolingZone != nullptr) ? coolingZone->getId() : 0, static_cast<int>(powerDomains.size()));
   return context;
}

/**
 * Get location structure of given object from cache, resolving it on cache miss
 */
static shared_ptr<LocationContext> GetLocationContext(const shared_ptr<NetObj>& object)
{
   uint32_t version = NObject::getParentListChangeCount();

   s_cacheLock.lock();
   if (s_cacheVersion != version)
   {
      s_cache.clear();
      s_cacheVersion = version;
   }
   auto it = s_cache.find(object->getId());
   if (it != s_cache.end())
   {
      shared_ptr<LocationContext> context = it->second;
      s_cacheLock.unlock();
      return context;
   }
   s_cacheLock.unlock();

   shared_ptr<LocationContext> context = ResolveLocationContext(object);

   // Do not cache result if relations changed while it was being resolved
   s_cacheLock.lock();
   if ((s_cacheVersion == version) && (NObject::getParentListChangeCount() == version))
      s_cache.emplace(object->getId(), context);
   s_cacheLock.unlock();
   return context;
}

/**
 * Check if objects of given class belong to a zone
 */
static inline bool IsZonedObjectClass(int objectClass)
{
   return (objectClass == OBJECT_NODE) || (objectClass == OBJECT_INTERFACE) || (objectClass == OBJECT_SUBNET) ||
          (objectClass == OBJECT_ACCESSPOINT) || (objectClass == OBJECT_CLUSTER) ||
          (objectClass == OBJECT_TRAFFICOBSERVER) || (objectClass == OBJECT_OBSERVATIONPOINT);
}

/**
 * Add name and GUID attributes for location object (if it still exists)
 */
static void AddLocationObjectAttributes(std::vector<OtlpExportAttribute> *attributes, const shared_ptr<NetObj>& object, const char *nameKey, const char *idKey)
{
   if (object == nullptr)
      return;
   attributes->emplace_back(nameKey, object->getName());
   attributes->emplace_back(idKey, object->getGuid());
}

/**
 * Check if string is an ISO 3166-1 alpha-2 country code
 */
static inline bool IsCountryCode(const wchar_t *s)
{
   return iswalpha(s[0]) && iswalpha(s[1]) && (s[2] == 0) && (s[0] < 128) && (s[1] < 128);
}

/**
 * Check if string is an ISO 3166-2 subdivision code (country code, hyphen, 1 to 3 alphanumeric characters)
 */
static bool IsSubdivisionCode(const wchar_t *s)
{
   if (!iswalpha(s[0]) || !iswalpha(s[1]) || (s[0] >= 128) || (s[1] >= 128) || (s[2] != L'-'))
      return false;
   int len = 0;
   for(const wchar_t *p = &s[3]; *p != 0; p++, len++)
      if ((*p >= 128) || !iswalnum(*p))
         return false;
   return (len >= 1) && (len <= 3);
}

/**
 * Convert ASCII code to upper case UTF-8 string
 */
static std::string CodeToUpper(const wchar_t *code)
{
   std::string result;
   for(const wchar_t *p = code; *p != 0; p++)
      result.push_back(static_cast<char>(towupper(*p)));
   return result;
}

/**
 * Add geo.* attributes: coordinates and postal address are taken independently from the first object in the chain
 * (object itself, then rack, room and facility) that has them
 */
static void AddGeoAttributes(std::vector<OtlpExportAttribute> *attributes, const shared_ptr<NetObj> *chain, size_t chainSize)
{
   for(size_t i = 0; i < chainSize; i++)
   {
      if (chain[i] == nullptr)
         continue;
      GeoLocation location = chain[i]->getGeoLocation();
      if (location.isValid() && (location.getType() != GL_UNSET))
      {
         attributes->emplace_back("geo.location.lat", location.getLatitude());
         attributes->emplace_back("geo.location.lon", location.getLongitude());
         break;
      }
   }

   for(size_t i = 0; i < chainSize; i++)
   {
      if (chain[i] == nullptr)
         continue;
      PostalAddress address = chain[i]->getPostalAddress();
      const wchar_t *country = address.getCountry();
      const wchar_t *region = address.getRegion();
      const wchar_t *city = address.getCity();
      const wchar_t *postcode = address.getPostCode();
      if ((*country == 0) && (*region == 0) && (*city == 0) && (*postcode == 0))
         continue;

      if (IsCountryCode(country))
         attributes->emplace_back("geo.country.iso_code", CodeToUpper(country));
      if (IsSubdivisionCode(region))
         attributes->emplace_back("geo.region.iso_code", CodeToUpper(region));
      if (*city != 0)
         attributes->emplace_back("geo.locality.name", city);
      if (*postcode != 0)
         attributes->emplace_back("geo.postal_code", postcode);
      break;
   }
}

/**
 * Add power domain attributes. Domain names, GUIDs, feed tags and types are parallel arrays (element N of each
 * describes the same domain); a domain without feed tag contributes an empty string.
 */
static void AddPowerDomainAttributes(std::vector<OtlpExportAttribute> *attributes, const std::vector<weak_ptr<NetObj>>& domains)
{
   std::vector<std::string> names, ids, feeds, types;
   for(const weak_ptr<NetObj>& d : domains)
   {
      shared_ptr<NetObj> object = d.lock();
      if (object == nullptr)
         continue;
      const PowerDomain& domain = static_cast<const PowerDomain&>(*object);
      names.push_back(WideToUtf8(domain.getName()));
      char guid[64];
      ids.push_back(domain.getGuid().toStringA(guid));
      feeds.push_back(WideToUtf8(domain.getFeedTag()));
      types.push_back(PowerDomainTypeName(domain.getDomainType()));
   }
   if (names.empty())
      return;
   attributes->emplace_back("netxms.power.domains", std::move(names));
   attributes->emplace_back("netxms.power.domain_ids", std::move(ids));
   attributes->emplace_back("netxms.power.feeds", std::move(feeds));
   attributes->emplace_back("netxms.power.domain_types", std::move(types));
}

/**
 * Get location attributes (facility, room, rack, rack placement, zone, geolocation, power domains,
 * cooling zone) for OTLP resource representing given object. Returns empty list if object no longer exists.
 */
std::vector<OtlpExportAttribute> GetLocationAttributes(uint32_t objectId)
{
   std::vector<OtlpExportAttribute> attributes;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return attributes;

   shared_ptr<LocationContext> context = GetLocationContext(object);
   shared_ptr<NetObj> facility = context->facility.lock();
   shared_ptr<NetObj> room = context->room.lock();
   shared_ptr<NetObj> rack = context->rack.lock();

   AddLocationObjectAttributes(&attributes, facility, "netxms.facility", "netxms.facility.id");
   AddLocationObjectAttributes(&attributes, room, "netxms.room", "netxms.room.id");
   AddLocationObjectAttributes(&attributes, rack, "netxms.rack", "netxms.rack.id");

   // Rack position is the top unit occupied by the device, in the rack's own unit numbering (as shown in the rack view)
   shared_ptr<NetObj> placement = context->placement.lock();
   if ((rack != nullptr) && (placement != nullptr))
   {
      int16_t position = 0, height = 0;
      if (placement->getObjectClass() == OBJECT_NODE)
      {
         const Node& node = static_cast<const Node&>(*placement);
         if (node.getPhysicalContainerId() == rack->getId())
         {
            position = node.getRackPosition();
            height = node.getRackHeight();
         }
      }
      else if (placement->getObjectClass() == OBJECT_CHASSIS)
      {
         const Chassis& chassis = static_cast<const Chassis&>(*placement);
         if (chassis.getRackId() == rack->getId())
         {
            position = chassis.getRackPosition();
            height = chassis.getRackHeight();
         }
      }
      if (position > 0)
      {
         attributes.emplace_back("netxms.rack.position", static_cast<int64_t>(position));
         if (height > 0)
            attributes.emplace_back("netxms.rack.units", static_cast<int64_t>(height));
      }
   }

   if (IsZoningEnabled() && IsZonedObjectClass(object->getObjectClass()))
   {
      int32_t zoneUIN = object->getZoneUIN();
      attributes.emplace_back("netxms.zone.uin", static_cast<int64_t>(zoneUIN));
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zone != nullptr)
         attributes.emplace_back("netxms.zone", zone->getName());
   }

   shared_ptr<NetObj> chain[] = { object, rack, room, facility };
   AddGeoAttributes(&attributes, chain, sizeof(chain) / sizeof(chain[0]));

   AddPowerDomainAttributes(&attributes, context->powerDomains);
   AddLocationObjectAttributes(&attributes, context->coolingZone.lock(), "netxms.cooling.zone", "netxms.cooling.zone.id");

   return attributes;
}
