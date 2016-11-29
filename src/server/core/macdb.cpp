/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
   BYTE macAddr[MAC_ADDR_LENGTH];
   NetObj *object;
};

/**
 * Root
 */
static MacDbEntry *s_data = NULL;

/**
 * Access lock
 */
static RWLOCK s_lock = RWLockCreate();

/**
 * Add interface
 */
void NXCORE_EXPORTABLE MacDbAddObject(const BYTE *macAddr, NetObj *object)
{
   // Ignore non-unique addresses
   if (!memcmp(macAddr, "\x00\x00\x00\x00\x00\x00", 6) ||
       !memcmp(macAddr, "\x00\x00\x5E\x00\x01", 5) || // VRRP
       !memcmp(macAddr, "\x00\x00\x0C\x07\xAC", 5) || // HSRP
       (!memcmp(macAddr, "\x00\x00\x0C\x9F", 4) && ((macAddr[4] & 0xF0) == 0xF0)) || // HSRP
       (!memcmp(macAddr, "\x00\x05\x73\xA0", 4) && ((macAddr[4] & 0xF0) == 0x00)) || // HSRP IPv6
       ((macAddr[0] & 0x01) != 0))  // multicast
      return;

   object->incRefCount();
   RWLockWriteLock(s_lock, INFINITE);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   if (entry == NULL)
   {
      entry = (MacDbEntry *)malloc(sizeof(MacDbEntry));
      memcpy(entry->macAddr, macAddr, MAC_ADDR_LENGTH);
      HASH_ADD_KEYPTR(hh, s_data, entry->macAddr, MAC_ADDR_LENGTH, entry);
   }
   else
   {
      if (entry->object->getId() != object->getId())
      {
         TCHAR macAddrStr[32];
         nxlog_debug(2, _T("MacDbAddObject: MAC address %s already known (%s [%d] -> %s [%d])"),
                     MACToStr(macAddr, macAddrStr), entry->object->getName(), entry->object->getId(),
                     object->getName(), object->getId());
      }
      entry->object->decRefCount();
   }
   entry->object = object;
   RWLockUnlock(s_lock);
}

/**
 * Add interface
 */
void NXCORE_EXPORTABLE MacDbAddInterface(Interface *iface)
{
   MacDbAddObject(iface->getMacAddr(), iface);
}

/**
 * Add access point
 */
void NXCORE_EXPORTABLE MacDbAddAccessPoint(AccessPoint *ap)
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

   RWLockWriteLock(s_lock, INFINITE);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   if (entry != NULL)
   {
      entry->object->decRefCount();
      HASH_DEL(s_data, entry);
      free(entry);
   }
   RWLockUnlock(s_lock);
}

/**
 * Find interface by MAC address
 */
NetObj NXCORE_EXPORTABLE *MacDbFind(const BYTE *macAddr)
{
   RWLockReadLock(s_lock, INFINITE);
   MacDbEntry *entry;
   HASH_FIND(hh, s_data, macAddr, MAC_ADDR_LENGTH, entry);
   NetObj *object = (entry != NULL) ? entry->object : NULL;
   RWLockUnlock(s_lock);
   return object;
}
