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
   shared_ptr<NetObj> object;

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
   }
   else
   {
      if (entry->object->getId() != object->getId())
      {
         TCHAR macAddrStr[64];
         nxlog_debug(5, _T("MacDbAddObject: MAC address %s already known (%s [%d] -> %s [%d])"),
                     macAddr.toString(macAddrStr), entry->object->getName(), entry->object->getId(),
                     object->getName(), object->getId());
      }
   }
   entry->object = object;
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
void NXCORE_EXPORTABLE MacDbRemove(const BYTE *macAddr)
{
   if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6))
      return;

   RWLockWriteLock(s_lock);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   if (entry != nullptr)
   {
      HASH_DEL(s_data, entry);
      delete entry;
   }
   RWLockUnlock(s_lock);
}

/**
 * Delete entry
 */
void NXCORE_EXPORTABLE MacDbRemove(const MacAddress& macAddr)
{
   if (!macAddr.isValid() || macAddr.isBroadcast() || macAddr.isMulticast() || (macAddr.length() != MAC_ADDR_LENGTH))
      return;
   MacDbRemove(macAddr.value());
}

/**
 * Find object by MAC address
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE MacDbFind(const BYTE *macAddr)
{
   RWLockReadLock(s_lock);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   shared_ptr<NetObj> object = (entry != nullptr) ? entry->object : shared_ptr<NetObj>();
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
