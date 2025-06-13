/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: macdb.cpp
**
**/

#include "nxcore.h"
#include <uthash.h>

#define DEBUG_TAG _T("macdb")

/**
 * Organizationally Unique Identifier mapping (24 bit identifiers)
 */
typedef BYTE oui24_t[3];
static HashMap<oui24_t, String> s_ouiMap(Ownership::True);

/**
 * Organizationally Unique Identifier mapping (28 bit identifiers)
 */
typedef BYTE oui28_t[4];
static HashMap<oui28_t, String> s_ouiMap28(Ownership::True);

/**
 * Organizationally Unique Identifier mapping (36 bit identifiers)
 */
typedef BYTE oui36_t[5];
static HashMap<oui36_t, String> s_ouiMap36(Ownership::True);

/**
 * MAC database entry
 */
struct MacDbEntry
{
   UT_hash_handle hh;
   MacAddress macAddr;
   SharedObjectArray<NetObj> objects;

   MacDbEntry()
   {
      memset(&hh, 0, sizeof(UT_hash_handle));
   }
};

/**
 * Root
 */
static MacDbEntry *s_data = nullptr;

/**
 * Access lock
 */
static RWLock s_lock;

/**
 * Get parent object
 */
static inline uint32_t GetParentId(const NetObj& object)
{
   if (object.getObjectClass() == OBJECT_INTERFACE)
      return static_cast<const Interface&>(object).getParentNodeId();
   return 0;
}

/**
 * Get parent object name for diagnostic messages
 */
static inline String GetParentName(const NetObj& object)
{
   if (object.getObjectClass() == OBJECT_INTERFACE)
      return static_cast<const Interface&>(object).getParentNodeName();
   return String();
}

/**
 * Add interface
 */
void NXCORE_EXPORTABLE MacDbAddObject(const MacAddress& macAddr, const shared_ptr<NetObj>& object)
{
   // Ignore non-unique or non-Ethernet addresses
   if (!macAddr.isValid() || macAddr.isBroadcast() || macAddr.isMulticast() ||
       (macAddr.length() != MAC_ADDR_LENGTH) ||
       !memcmp(macAddr.value(), "\x00\x00\x5E\x00\x01", 5) || // VRRP
       !memcmp(macAddr.value(), "\x00\x00\x0C\x07\xAC", 5) || // HSRP
       (!memcmp(macAddr.value(), "\x00\x00\x0C\x9F", 4) && ((macAddr.value()[4] & 0xF0) == 0xF0)) || // HSRP
       (!memcmp(macAddr.value(), "\x00\x05\x73\xA0", 4) && ((macAddr.value()[4] & 0xF0) == 0x00))) // HSRP IPv6
      return;

   s_lock.writeLock();
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr.value(), MAC_ADDR_LENGTH, entry);
   if (entry == nullptr)
   {
      entry = new MacDbEntry();
      entry->macAddr = macAddr;
      HASH_ADD_KEYPTR(hh, s_data, entry->macAddr.value(), entry->macAddr.length(), entry);
      entry->objects.add(object);
   }
   else
   {
      bool found = false;
      for(int i  = 0; i < entry->objects.size(); i++)
      {
         if (entry->objects.get(i)->getId() == object->getId())
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         // Only generate event if duplicate MAC address is not from already added node
         uint32_t nodeId = GetParentId(*object);
         if (nodeId != 0)
         {
            bool nodeAlreadyKnown = false;
            for(int i = 0; i < entry->objects.size(); i++)
               if (GetParentId(*entry->objects.get(i)) == nodeId)
               {
                  nodeAlreadyKnown = true;
                  break;
               }
            entry->objects.add(object);

            if (!nodeAlreadyKnown)
            {
               StringBuffer objects;
               for(int i  = 0; i < entry->objects.size(); i++)
               {
                  NetObj *o = entry->objects.get(i);
                  objects
                     .append(GetParentName(*o))
                     .append(_T("/"))
                     .append(o->getName())
                     .append(_T(", "));
               }
               objects.shrink(2);

               TCHAR macAddrStr[64];
               nxlog_debug_tag(DEBUG_TAG, 5, _T("MacDbAddObject: MAC address %s is known on multiple interfaces (%s)"), macAddr.toString(macAddrStr), objects.cstr());

               if (g_flags & AF_SERVER_INITIALIZED)
               {
                  EventBuilder(EVENT_DUPLICATE_MAC_ADDRESS, nodeId)
                     .param(_T("macAddress"), macAddr)
                     .param(_T("listOfInterfaces"), objects.cstr())
                     .post();
               }
            }
         }
         else
         {
            entry->objects.add(object);

            TCHAR macAddrStr[64];
            nxlog_debug_tag(DEBUG_TAG, 4, _T("MacDbAddObject: cannot get parent node ID for object \"%s\" [%u] with MAC address %s"), object->getName(), object->getId(), macAddr.toString(macAddrStr));
         }
      }
   }
   s_lock.unlock();
}

/**
 * Add interface
 */
void NXCORE_EXPORTABLE MacDbAddInterface(const shared_ptr<Interface>& iface)
{
   MacDbAddObject(iface->getMacAddress(), iface);
}

/**
 * Add access point
 */
void NXCORE_EXPORTABLE MacDbAddAccessPoint(const shared_ptr<AccessPoint>& ap)
{
   MacDbAddObject(ap->getMacAddress(), ap);
   std::vector<MacAddress> bssids = ap->getRadioBSSID();
   for(const MacAddress& m : bssids)
      MacDbAddObject(m, ap);
}

/**
 * Delete entry
 */
void NXCORE_EXPORTABLE MacDbRemoveObject(const MacAddress& macAddr, const uint32_t objectId)
{
   if (!macAddr.isValid() || macAddr.isBroadcast() || macAddr.isMulticast() || (macAddr.length() != MAC_ADDR_LENGTH))
      return;

   if (!memcmp(macAddr.value(), "\x00\x00\x00\x00\x00\x00", 6))
      return;

   s_lock.writeLock();
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr.value(), MAC_ADDR_LENGTH, entry);
   if (entry != nullptr)
   {
      for(int i = 0; i < entry->objects.size(); i++)
      {
         uint32_t id = entry->objects.get(i)->getId();
         if(id == objectId)
         {
            entry->objects.remove(i);
            break;
         }
      }
      if (entry->objects.isEmpty())
      {
         HASH_DEL(s_data, entry);
         delete entry;
      }
   }
   s_lock.unlock();
}

/**
 * Remove access point
 */
void NXCORE_EXPORTABLE MacDbRemoveAccessPoint(const AccessPoint& ap)
{
   MacDbRemoveObject(ap.getMacAddress(), ap.getId());
   std::vector<MacAddress> bssids = ap.getRadioBSSID();
   for(const MacAddress& m : bssids)
      MacDbRemoveObject(m, ap.getId());
}

/**
 * Remove interface
 */
void NXCORE_EXPORTABLE MacDbRemoveInterface(const Interface& iface)
{
   MacDbRemoveObject(iface.getMacAddress(), iface.getId());
}

/**
 * Find object by MAC address
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const BYTE *macAddr)
{
   s_lock.readLock();
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   shared_ptr<NetObj> object = (entry != nullptr) ? entry->objects.getShared(entry->objects.size() - 1) : shared_ptr<NetObj>();
   s_lock.unlock();
   return object;
}

/**
 * Find object by MAC address
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const MacAddress& macAddr)
{
   if (!macAddr.isValid() || macAddr.isBroadcast() || macAddr.isMulticast() || (macAddr.length() != MAC_ADDR_LENGTH))
      return shared_ptr<NetObj>();
   return MacDbFind(macAddr.value());
}

/**
 * Find vendor by MAC address (internal)
 */
static std::pair<const TCHAR*, uint32_t> FindVendorByMacInternal(const MacAddress& macAddr)
{
   uint32_t numBytes;
   oui24_t bytes;
   memcpy(bytes, macAddr.value(), 3);
   const String *name = s_ouiMap.get(bytes);
   numBytes = 3;

   if (name == nullptr)
   {
      oui28_t value;
      memcpy(value, macAddr.value(), 4);
      value[3] &= 0xF0;
      name = s_ouiMap28.get(value);
      numBytes = 4;
   }

   if (name == nullptr)
   {
      oui36_t value;
      memcpy(value, macAddr.value(), 5);
      value[4] &= 0xF0;
      name = s_ouiMap36.get(value);
      numBytes = 5;
   }

   return std::pair<const TCHAR *, uint32_t>((name != nullptr) ? name->cstr() : nullptr, numBytes);
}

/**
 * Find vendor name by MAC address
 */
const TCHAR *FindVendorByMac(const MacAddress& macAddr)
{
   return FindVendorByMacInternal(macAddr).first;
}

/**
 * Find vendor name by MAC address list. Read MAC addresses from incoming NXCP message and write results to response NXCP message.
 */
void FindVendorByMacList(const NXCPMessage& request, NXCPMessage* response)
{
   uint32_t numElements = request.getFieldAsUInt32(VID_NUM_ELEMENTS);
   uint32_t inputBase = VID_ELEMENT_LIST_BASE;
   uint32_t outputBase = VID_ELEMENT_LIST_BASE;
   for (uint32_t i = 0; i < numElements; i++)
   {
      MacAddress addr = request.getFieldAsMacAddress(inputBase++);
      std::pair<const TCHAR *, uint32_t> result = FindVendorByMacInternal(addr);
      oui36_t value;
      memcpy(value, addr.value(), result.second);
      if (result.second > 3)
         value[result.second - 1] &= 0xF0;
      response->setField(outputBase, value, result.second);
      response->setField(outputBase + 1, result.first);
      outputBase += 2;
   }
   response->setField(VID_NUM_ELEMENTS, numElements);
}

/**
 * Load OUI database file
 */
template<size_t MaxLen, typename T> static bool LoadOUIDatabaseFile(const wchar_t *fileName, HashMap<T, String> *map)
{
   wchar_t path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   wcscat(path, FS_PATH_SEPARATOR);
   wcscat(path, L"oui");
   wcscat(path, FS_PATH_SEPARATOR);
   wcscat(path, fileName);

   char *fileContent = LoadFileAsUTF8String(path);
   if (fileContent == nullptr)
   {
      if ((errno != ENOENT) || wcsncmp(fileName, L"custom-", 7))
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot open OUI database file \"%s\" (%s)", path, _wcserror(errno));
      return false;
   }

   wchar_t *content = WideStringFromUTF8String(fileContent);
   MemFree(fileContent);

   Table *table = Table::createFromCSV(content, L',');
   MemFree(content);
   if (table == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing OUI database file \"%s\""), fileName);
      return false;
   }

   for (int i = 0; i < table->getNumRows(); i++)
   {
      T oui;
      StringBuffer ouiText = table->getAsString(i, 1, L"");
      if (ouiText.length() % 2 != 0)
         ouiText.append(_T("0"));
      StrToBin(ouiText, oui, MaxLen);
      map->set(oui, new String(table->getAsString(i, 2, L"")));
   }

   delete table;
   return true;
}

/**
 * Load OUI database
 */
void LoadOUIDatabase()
{
   if (LoadOUIDatabaseFile<3, oui24_t>(L"oui.csv", &s_ouiMap))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("OUI-24 database loaded (%d entries)"), s_ouiMap.size());

   if (LoadOUIDatabaseFile<4, oui28_t>(L"oui28.csv", &s_ouiMap28))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("OUI-28 database loaded (%d entries)"), s_ouiMap28.size());

   if (LoadOUIDatabaseFile<5, oui36_t>(L"oui36.csv", &s_ouiMap36))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("OUI-36 database loaded (%d entries)"), s_ouiMap36.size());

   int size = s_ouiMap.size();
   if (LoadOUIDatabaseFile<3, oui24_t>(L"custom-oui.csv", &s_ouiMap))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Custom OUI-24 database loaded (%d entries)"), s_ouiMap.size() - size);

   size = s_ouiMap28.size();
   if (LoadOUIDatabaseFile<4, oui28_t>(L"custom-oui28.csv", &s_ouiMap28))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Custom OUI-28 database loaded (%d entries)"), s_ouiMap28.size() - size);

   size = s_ouiMap36.size();
   if (LoadOUIDatabaseFile<5, oui36_t>(L"custom-oui36.csv", &s_ouiMap36))
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Custom OUI-36 database loaded (%d entries)"), s_ouiMap36.size() - size);
}
