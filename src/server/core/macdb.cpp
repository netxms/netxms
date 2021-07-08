/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

/**
 * Entry
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
static RWLOCK s_lock = RWLockCreate();

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

   RWLockWriteLock(s_lock);
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
         if(entry->objects.get(i)->getId() == object->getId())
         {
            found = true;
            break;
         }
      }
      if(!found)
      {
         StringBuffer objects;
         for(int i  = 0; i < entry->objects.size(); i++)
         {
            objects.appendFormattedString(_T("%s [%d]%s "), entry->objects.get(i)->getName(), entry->objects.get(i)->getId(), (i+1) == entry->objects.size() ? _T("") : _T(","));
         }
         entry->objects.add(object);
         TCHAR macAddrStr[64];
         nxlog_debug(5, _T("MacDbAddObject: MAC address %s already known (%s -> %s [%d])"),
                     macAddr.toString(macAddrStr), objects.getBuffer(),
                     object->getName(), object->getId());

         if(g_flags & AF_SERVER_INITIALIZED)
         {
            PostSystemEvent(EVENT_DUPLICATE_MAC_ADDRESS, object->getId(), "H", &macAddr);
         }
      }
   }
   RWLockUnlock(s_lock);
}

/**
 * Add interface
 */
void NXCORE_EXPORTABLE MacDbAddInterface(const shared_ptr<Interface>& iface)
{
   MacDbAddObject(iface->getMacAddr(), iface);
}

/**
 * Add access point
 */
void NXCORE_EXPORTABLE MacDbAddAccessPoint(const shared_ptr<AccessPoint>& ap)
{
   MacDbAddObject(ap->getMacAddr(), ap);
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

   RWLockWriteLock(s_lock);
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
      if(entry->objects.size() == 0)
      {
         HASH_DEL(s_data, entry);
         delete entry;
      }
   }
   RWLockUnlock(s_lock);
}

/**
 * Find object by MAC address
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const BYTE *macAddr)
{
   RWLockReadLock(s_lock);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   shared_ptr<NetObj> object = (entry != nullptr) ? entry->objects.getShared(entry->objects.size() - 1) : shared_ptr<NetObj>();
   RWLockUnlock(s_lock);
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
