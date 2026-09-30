/*
** NetXMS - Network Management System
** NetXMS shell
** Copyright (C) 2025-2026 Raden Solutions
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
** File: navigation.cpp
**
**/

#include "nxshell.h"
#include <algorithm>

/**
 * Lifetime of cached object listing in milliseconds
 */
#define LISTING_CACHE_LIFETIME   5000

/**
 * Maximum number of cached object listings
 */
#define LISTING_CACHE_SIZE       64

/**
 * Maximum depth of location built from object's parents
 */
#define MAX_LOCATION_DEPTH       64

/**
 * Maximum number of objects offered for selection
 */
#define MAX_SELECTION_SIZE       20

/**
 * Object status names and colors
 */
static const struct
{
   const char *name;
   const char *attributes;
} s_objectStatus[] =
{
   { "Normal", "32" },
   { "Warning", "36" },
   { "Minor", "33" },
   { "Major", "33;1" },
   { "Critical", "31;1" },
   { "Unknown", "90" },
   { "Unmanaged", "90" },
   { "Disabled", "90" },
   { "Testing", "35" }
};

/**
 * Compare object names ignoring case
 */
static bool NameEquals(const std::string& name1, const std::string& name2)
{
   if (name1 == name2)
      return true;

   WCHAR *wname1 = WideStringFromUTF8String(name1.c_str());
   WCHAR *wname2 = WideStringFromUTF8String(name2.c_str());
   bool equal = (wcsicmp(wname1, wname2) == 0);
   MemFree(wname1);
   MemFree(wname2);
   return equal;
}

/**
 * Get path to current location
 */
std::string Shell::getLocationPath() const
{
   if (m_location.empty())
      return "/";

   std::string path;
   for(size_t i = 0; i < m_location.size(); i++)
      path.append("/").append(EscapePathElement(m_location[i].name));
   return path;
}

/**
 * Get direct children of given object (objects without accessible parents if parent ID is 0).
 * Listings are cached for short time to make path resolution and completion responsive.
 */
bool Shell::getChildObjects(uint32_t parentId, const std::vector<ObjectInfo> **objects)
{
   int64_t now = GetCurrentTimeMs();
   auto it = m_listingCache.find(parentId);
   if ((it != m_listingCache.end()) && (now - it->second.timestamp < LISTING_CACHE_LIFETIME))
   {
      *objects = &it->second.objects;
      return true;
   }

   if (m_listingCache.size() >= LISTING_CACHE_SIZE)
      m_listingCache.clear();

   ObjectListing& listing = m_listingCache[parentId];
   if (!m_client->getChildObjects(parentId, &listing.objects))
   {
      m_listingCache.erase(parentId);
      return false;
   }

   std::sort(listing.objects.begin(), listing.objects.end(),
      [] (const ObjectInfo& o1, const ObjectInfo& o2) -> bool
      {
         int rc = stricmp(o1.name.c_str(), o2.name.c_str());
         return (rc != 0) ? (rc < 0) : (o1.id < o2.id);
      });
   listing.timestamp = now;
   *objects = &listing.objects;
   return true;
}

/**
 * Resolve path to location. Relative path is resolved from current location.
 */
bool Shell::resolvePath(const std::string& text, std::vector<ObjectInfo> *location, bool reportErrors)
{
   ObjectPath path;
   ParseObjectPath(text, &path);

   if (path.absolute)
      location->clear();
   else
      *location = m_location;

   for(size_t i = 0; i < path.elements.size(); i++)
   {
      const std::string& element = path.elements[i];
      if (element == ".")
         continue;
      if (element == "..")
      {
         if (!location->empty())
            location->pop_back();
         continue;
      }

      const std::vector<ObjectInfo> *objects;
      if (!getChildObjects(location->empty() ? 0 : location->back().id, &objects))
      {
         if (reportErrors)
            PrintError("%s", m_client->getErrorText());
         return false;
      }

      // Name match with exact case is preferred
      std::vector<const ObjectInfo*> matches;
      const ObjectInfo *exactMatch = nullptr;
      int exactMatchCount = 0;
      for(size_t j = 0; j < objects->size(); j++)
      {
         const ObjectInfo *object = &objects->at(j);
         if (object->name == element)
         {
            exactMatch = object;
            exactMatchCount++;
         }
         if (NameEquals(object->name, element))
            matches.push_back(object);
      }

      if (exactMatchCount == 1)
      {
         location->push_back(*exactMatch);
      }
      else if (matches.size() == 1)
      {
         location->push_back(*matches[0]);
      }
      else
      {
         if (reportErrors)
         {
            if (matches.empty())
            {
               PrintError("object \"%s\" not found", element.c_str());
            }
            else
            {
               PrintError("name \"%s\" is ambiguous, use \"cd #<id>\" to select one of:", element.c_str());
               for(size_t j = 0; j < matches.size(); j++)
                  PrintStatus("   #%u  %s (%s)", matches[j]->id, matches[j]->name.c_str(), matches[j]->className.c_str());
            }
         }
         return false;
      }
   }
   return true;
}

/**
 * Build location for object given by ID. Path is built by following first accessible parent
 * up to an object without accessible parents.
 */
bool Shell::buildLocation(uint32_t objectId, std::vector<ObjectInfo> *location)
{
   ObjectInfo object;
   if (!m_client->getObject(objectId, &object))
   {
      if (m_client->getHttpStatus() == 404)
         PrintError("object with ID %u does not exist", objectId);
      else if (m_client->getHttpStatus() == 403)
         PrintError("access denied to object with ID %u", objectId);
      else
         PrintError("%s", m_client->getErrorText());
      return false;
   }

   location->clear();
   location->push_back(object);
   while(location->size() < MAX_LOCATION_DEPTH)
   {
      std::vector<uint32_t> parents = location->back().parents;
      bool found = false;
      for(size_t i = 0; (i < parents.size()) && !found; i++)
      {
         bool visited = false;
         for(size_t j = 0; j < location->size(); j++)
         {
            if (location->at(j).id == parents[i])
               visited = true;
         }

         ObjectInfo parent;
         if (!visited && m_client->getObject(parents[i], &parent))
         {
            location->push_back(parent);
            found = true;
         }
      }
      if (!found)
         break;
   }
   std::reverse(location->begin(), location->end());
   return true;
}

/**
 * Select object by name. If multiple objects match and input is read from terminal, user is
 * asked to select one of them.
 */
bool Shell::selectObject(const std::string& name, uint32_t *objectId)
{
   std::vector<ObjectInfo> objects;
   if (!m_client->findObjects(name.c_str(), &objects))
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }

   if (objects.empty())
   {
      PrintError("object \"%s\" not found", name.c_str());
      return false;
   }

   // Search matches part of the name, so single object with exactly matching name is preferred
   const ObjectInfo *match = nullptr;
   int matchCount = 0;
   for(size_t i = 0; i < objects.size(); i++)
   {
      if (NameEquals(objects[i].name, name))
      {
         match = &objects[i];
         matchCount++;
      }
   }
   if ((matchCount == 1) || (objects.size() == 1))
   {
      *objectId = (matchCount == 1) ? match->id : objects[0].id;
      return true;
   }

   size_t count = std::min(objects.size(), static_cast<size_t>(MAX_SELECTION_SIZE));
   if (!m_terminalInput)
   {
      PrintError("name \"%s\" is ambiguous, use \"cd #<id>\" to select one of:", name.c_str());
      for(size_t i = 0; i < count; i++)
         PrintStatus("   #%u  %s (%s)", objects[i].id, objects[i].name.c_str(), objects[i].className.c_str());
      if (objects.size() > count)
         PrintStatus("   (%d more objects not shown)", static_cast<int>(objects.size() - count));
      return false;
   }

   std::string text;
   for(size_t i = 0; i < count; i++)
   {
      char line[512];
      snprintf(line, sizeof(line), "  %2d. %s (%s) #%u\n", static_cast<int>(i) + 1, objects[i].name.c_str(), objects[i].className.c_str(), objects[i].id);
      text.append(line);
   }
   WriteToTerminalUtf8(text.c_str());
   if (objects.size() > count)
      PrintStatus("%d more objects not shown", static_cast<int>(objects.size() - count));

   char prompt[64];
   snprintf(prompt, sizeof(prompt), "Select object (1-%d): ", static_cast<int>(count));
   std::string answer;
   if (!ReadInputLine(prompt, &answer))
   {
      WriteToTerminalUtf8("\n");
      return false;
   }
   TrimString(&answer);
   if (answer.empty())
      return false;

   char *eptr;
   long choice = strtol(answer.c_str(), &eptr, 10);
   if ((*eptr != 0) || (choice < 1) || (choice > static_cast<long>(count)))
   {
      PrintError("invalid selection");
      return false;
   }
   *objectId = objects[choice - 1].id;
   return true;
}

/**
 * Set current location to object with given ID
 */
bool Shell::setLocationByObjectId(uint32_t objectId)
{
   std::vector<ObjectInfo> location;
   if (!buildLocation(objectId, &location))
      return false;

   m_previousLocation = m_location;
   m_location = location;
   return true;
}

/**
 * Set current location to first object matching given name
 */
bool Shell::setLocationByObjectName(const char *name)
{
   std::vector<ObjectInfo> objects;
   if (!m_client->findObjects(name, &objects))
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }
   if (objects.empty())
   {
      PrintError("object \"%s\" not found", name);
      return false;
   }
   return setLocationByObjectId(objects[0].id);
}

/**
 * Execute "cd" command
 */
bool Shell::changeLocation(const std::string& arguments)
{
   if (arguments == "-")
   {
      std::swap(m_location, m_previousLocation);
      return true;
   }

   if (!arguments.empty() && (arguments[0] == '#'))
   {
      char *eptr;
      uint32_t objectId = strtoul(arguments.c_str() + 1, &eptr, 10);
      if ((*eptr != 0) || (objectId == 0))
      {
         PrintError("invalid object ID \"%s\"", arguments.c_str() + 1);
         return false;
      }
      return setLocationByObjectId(objectId);
   }

   if (!arguments.empty() && (arguments[0] == '@'))
   {
      std::string name = arguments.substr(1);
      TrimString(&name);
      uint32_t objectId;
      if (name.empty())
      {
         PrintError("usage: cd @<name>");
         return false;
      }
      return selectObject(name, &objectId) && setLocationByObjectId(objectId);
   }

   std::vector<ObjectInfo> location;
   if (!arguments.empty() && !resolvePath(arguments, &location, true))
      return false;

   m_previousLocation = m_location;
   m_location = location;
   return true;
}

/**
 * Execute "ls" command
 */
bool Shell::listObjects(const std::string& arguments)
{
   std::vector<ObjectInfo> location;
   if (!resolvePath(arguments, &location, true))
      return false;

   const std::vector<ObjectInfo> *objects;
   if (!getChildObjects(location.empty() ? 0 : location.back().id, &objects))
   {
      PrintError("%s", m_client->getErrorText());
      return false;
   }

   size_t classWidth = 5;
   for(size_t i = 0; i < objects->size(); i++)
      classWidth = std::max(classWidth, objects->at(i).className.length());

   std::string text;
   for(size_t i = 0; i < objects->size(); i++)
   {
      const ObjectInfo& object = objects->at(i);

      char id[32];
      snprintf(id, sizeof(id), "%8u  ", object.id);
      text.append(id);

      text.append(object.className).append(classWidth - object.className.length() + 2, ' ');

      bool validStatus = (object.status >= 0) && (object.status < static_cast<int>(sizeof(s_objectStatus) / sizeof(s_objectStatus[0])));
      const char *statusName = validStatus ? s_objectStatus[object.status].name : "Unknown";
      AppendHighlightedText(&text, validStatus ? s_objectStatus[object.status].attributes : "90", statusName);
      text.append(11 - strlen(statusName), ' ');

      text.append(object.name).append("\n");
   }
   WriteToTerminalUtf8(text.c_str());
   return true;
}

/**
 * Validate current location after server reported that current object does not exist. Location is
 * changed to the nearest ancestor that still exists.
 */
void Shell::validateLocation()
{
   m_listingCache.clear();

   std::vector<ObjectInfo> location = m_location;
   while(!location.empty())
   {
      ObjectInfo object;
      if (m_client->getObject(location.back().id, &object))
         break;
      if ((m_client->getHttpStatus() != 404) && (m_client->getHttpStatus() != 403))
         return;   // Cannot check, keep current location
      location.pop_back();
   }

   if (location.size() == m_location.size())
      return;

   m_location = location;
   PrintWarning("current object is no longer available, location changed to %s", getLocationPath().c_str());
}
