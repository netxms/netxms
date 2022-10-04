/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2022 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published
 ** by the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 ** File: calltbl.cpp
 **
 **/

#include "libnetxms.h"
#include <nxcall.h>
#include <uthash.h>
#include <vector>

/**
 * Call table entry
 */
struct CallTableEntry
{
   UT_hash_handle hh;
   char name[64];
   CallHandler handler;
};

/**
 * Call table lock
 */
static RWLock s_callTableLock;

/**
 * Call table
 */
static CallTableEntry *s_callTable = nullptr;

/**
 * Register call handler
 */
void LIBNETXMS_EXPORTABLE RegisterCallHandler(const char *name, CallHandler handler)
{
   auto keyLen = static_cast<unsigned int>(strlen(name));
   s_callTableLock.writeLock();
   CallTableEntry *entry;
   HASH_FIND(hh, s_callTable, name, keyLen, entry);
   if (entry == nullptr)
   {
      entry = MemAllocStruct<CallTableEntry>();
      strlcpy(entry->name, name, 64);
      HASH_ADD_KEYPTR(hh, s_callTable, entry->name, keyLen, entry);
   }
   entry->handler = handler;
   s_callTableLock.unlock();
}

/**
 * Unregister call handler
 */
void LIBNETXMS_EXPORTABLE UnregisterCallHandler(const char *name)
{
   s_callTableLock.writeLock();
   CallTableEntry *entry;
   HASH_FIND(hh, s_callTable, name, static_cast<unsigned int>(strlen(name)), entry);
   if (entry != nullptr)
   {
      HASH_DEL(s_callTable, entry);
      MemFree(entry);
   }
   s_callTableLock.unlock();
}

/**
 * Call named function
 */
int LIBNETXMS_EXPORTABLE CallNamedFunction(const char *name, const void *input, void *output)
{
   int rc;
   s_callTableLock.readLock();
   CallTableEntry *entry;
   HASH_FIND(hh, s_callTable, name, static_cast<unsigned int>(strlen(name)), entry);
   if (entry != nullptr)
   {
      rc = entry->handler(input, output);
   }
   else
   {
      rc = -1;
   }
   s_callTableLock.unlock();
   return rc;
}

/**
 * Hook table entry
 */
struct HookTableEntry
{
   UT_hash_handle hh;
   char name[64];
   std::vector<std::pair<std::function<void (void*)>, uint32_t>> hooks;
};

/**
 * Hook table lock
 */
static RWLock s_hookTableLock;

/**
 * Hook table
 */
static HookTableEntry *s_hookTable = nullptr;

/**
 * Next hook id
 */
static uint32_t s_nextHookId = 0;

/**
 * Register hook
 */
uint32_t LIBNETXMS_EXPORTABLE RegisterHook(const char *name, std::function<void (void*)> hook)
{
   auto keyLen = static_cast<unsigned int>(strlen(name));
   s_hookTableLock.writeLock();
   HookTableEntry *entry;
   HASH_FIND(hh, s_hookTable, name, keyLen, entry);
   if (entry == nullptr)
   {
      entry = new HookTableEntry();
      strlcpy(entry->name, name, 64);
      HASH_ADD_KEYPTR(hh, s_hookTable, entry->name, keyLen, entry);
   }
   uint32_t id = ++s_nextHookId;
   entry->hooks.push_back(std::pair<std::function<void (void*)>, uint32_t>(hook, id));
   s_hookTableLock.unlock();
   return id;
}

/**
 * Unregister hook
 */
void LIBNETXMS_EXPORTABLE UnregisterHook(uint32_t hookId)
{
   s_hookTableLock.writeLock();
   bool found = false;
   HookTableEntry *entry, *tmp;
   HASH_ITER(hh, s_hookTable, entry, tmp)
   {
      for(auto it = entry->hooks.begin(); it != entry->hooks.end(); ++it)
      {
         if ((*it).second == hookId)
         {
            entry->hooks.erase(it);
            found = true;
            break;
         }
      }
      if (found)
      {
         if (entry->hooks.empty())
         {
            HASH_DEL(s_hookTable, entry);
            delete entry;
         }
         break;
      }
   }
   s_hookTableLock.unlock();
}

/**
 * Call named hook
 */
void LIBNETXMS_EXPORTABLE CallHook(const char *name, void *data)
{
   s_hookTableLock.readLock();
   HookTableEntry *entry;
   HASH_FIND(hh, s_hookTable, name, static_cast<unsigned int>(strlen(name)), entry);
   if (entry != nullptr)
   {
      for(auto it = entry->hooks.begin(); it != entry->hooks.end(); ++it)
      {
         (*it).first(data);
      }
   }
   s_hookTableLock.unlock();
}
