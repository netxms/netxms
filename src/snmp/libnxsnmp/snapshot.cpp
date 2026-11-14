/*
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: snapshot.cpp
**
**/

#include "libnxsnmp.h"

#undef uthash_malloc
#define uthash_malloc(sz) m_pool.allocate(sz)
#undef uthash_free
#define uthash_free(ptr,sz) do { } while(0)

#include <uthash.h>

/**
 * Snapshot index entry
 */
struct SNMP_SnapshotIndexEntry
{
   UT_hash_handle hh;
   SNMP_Variable *var;
   int pos;
};

/**
 * Constructor
 */
SNMP_Snapshot::SNMP_Snapshot() : m_values(0, 256, Ownership::True), m_pool(8192)
{
   m_index = nullptr;
   m_indexData = nullptr;
}

/**
 * Build OID index. Expected to be called only once.
 */
void SNMP_Snapshot::buildIndex()
{
   if (m_values.isEmpty())
      return;

   m_indexData = MemAllocArray<SNMP_SnapshotIndexEntry>(m_values.size());
   for(int i = 0; i < m_values.size(); i++)
   {
      SNMP_Variable *v = m_values.get(i);
      SNMP_SnapshotIndexEntry *entry = &m_indexData[i];
      entry->var = v;
      entry->pos = i;
      HASH_ADD_KEYPTR(hh, m_index, entry->var->getName().value(), (unsigned int)(entry->var->getName().length() * sizeof(uint32_t)), entry);
   }
}

/**
 * Find OID index entry by OID
 */
SNMP_SnapshotIndexEntry *SNMP_Snapshot::find(const uint32_t *oid, size_t oidLen) const
{
   SNMP_SnapshotIndexEntry *entry;
   HASH_FIND(hh, m_index, oid, (unsigned int)(oidLen * sizeof(uint32_t)), entry);
   return entry;
}

/**
 * Find OID index entry by OID
 */
SNMP_SnapshotIndexEntry *SNMP_Snapshot::find(const TCHAR *oid) const
{
   uint32_t binOid[MAX_OID_LEN];
   size_t oidLen = SnmpParseOID(oid, binOid, MAX_OID_LEN);
   if (oidLen == 0)
      return nullptr;
   return find(binOid, oidLen);
}

/**
 * Create snapshot (using text OID)
 */
SNMP_Snapshot *SNMP_Snapshot::create(SNMP_Transport *transport, const TCHAR *baseOid, size_t limit)
{
   SNMP_Snapshot *s = new SNMP_Snapshot();

   uint32_t rc = SnmpWalk(transport, baseOid,
      [s, limit] (SNMP_Variable *var) -> uint32_t
      {
         if ((limit != 0) && (s->m_values.size() >= limit))
            return SNMP_ERR_SNAPSHOT_TOO_BIG;
         s->m_values.add(new SNMP_Variable(std::move(*var)));
         return SNMP_ERR_SUCCESS;
      });

   if (rc == SNMP_ERR_SUCCESS)
      s->buildIndex();
   else
      delete_and_null(s);
   return s;
}

/**
 * Create snapshot (using binary OID)
 */
SNMP_Snapshot *SNMP_Snapshot::create(SNMP_Transport *transport, const uint32_t *baseOid, size_t oidLen, size_t limit)
{
   SNMP_Snapshot *s = new SNMP_Snapshot();

   uint32_t rc = SnmpWalk(transport, baseOid, oidLen,
      [s, limit] (SNMP_Variable *var) -> uint32_t
      {
         if ((limit != 0) && (s->m_values.size() >= limit))
            return SNMP_ERR_SNAPSHOT_TOO_BIG;
         s->m_values.add(new SNMP_Variable(std::move(*var)));
         return SNMP_ERR_SUCCESS;
      });

   if (rc == SNMP_ERR_SUCCESS)
      s->buildIndex();
   else
      delete_and_null(s);
   return s;
}

/**
 * Get varbind for given OID
 */
const SNMP_Variable *SNMP_Snapshot::get(const TCHAR *oid) const
{
   SNMP_SnapshotIndexEntry *entry = find(oid);
   return (entry != nullptr) ? entry->var : nullptr;
}

/**
 * Get varbind for given OID
 */
const SNMP_Variable *SNMP_Snapshot::get(const uint32_t *oid, size_t oidLen) const
{
   SNMP_SnapshotIndexEntry *entry = find(oid, oidLen);
   return (entry != nullptr) ? entry->var : nullptr;
}

/**
 * Get next variable for given OID
 */
const SNMP_Variable *SNMP_Snapshot::getNext(const TCHAR *oid) const
{
   uint32_t binOid[MAX_OID_LEN];
   size_t oidLen = SnmpParseOID(oid, binOid, MAX_OID_LEN);
   if (oidLen == 0)
      return nullptr;
   return getNext(binOid, oidLen);
}

/**
 * Get next variable for given OID
 */
const SNMP_Variable *SNMP_Snapshot::getNext(const uint32_t *oid, size_t oidLen) const
{
   SNMP_SnapshotIndexEntry *entry = find(oid, oidLen);
   if (entry != nullptr)
      return m_values.get(entry->pos + 1);

   for(int i = 0; i < m_values.size(); i++)
   {
      SNMP_Variable *v = m_values.get(i);
      int c = v->getName().compare(oid, oidLen);
      if ((c == OID_FOLLOWING) || (c == OID_LONGER))
         return v;
   }
   return nullptr;
}

/**
 * Walk part of snapshot
 */
EnumerationCallbackResult SNMP_Snapshot::walk(const TCHAR *baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const
{
   uint32_t binOid[MAX_OID_LEN];
   size_t oidLen = SnmpParseOID(baseOid, binOid, MAX_OID_LEN);
   if (oidLen == 0)
      return _CONTINUE;
   return walk(binOid, oidLen, handler, userArg);
}

/**
 * Walk part of snapshot
 */
EnumerationCallbackResult SNMP_Snapshot::walk(const uint32_t *baseOid, size_t baseOidLen, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const
{
   EnumerationCallbackResult result = _CONTINUE;
   const SNMP_Variable *curr = getNext(baseOid, baseOidLen);
   while((curr != nullptr) && (curr->getName().compare(baseOid, baseOidLen) == OID_LONGER))
   {
      result = handler(curr, this, userArg);
      if (result == _STOP)
         break;
      curr = getNext(curr->getName().value(), curr->getName().length());
   }
   return result;
}

/**
 * Walk part of snapshot
 */
EnumerationCallbackResult SNMP_Snapshot::walk(const TCHAR *baseOid, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const
{
   uint32_t binOid[MAX_OID_LEN];
   size_t oidLen = SnmpParseOID(baseOid, binOid, MAX_OID_LEN);
   if (oidLen == 0)
      return _CONTINUE;
   return walk(binOid, oidLen, callback);
}

/**
 * Walk part of snapshot
 */
EnumerationCallbackResult SNMP_Snapshot::walk(const uint32_t *baseOid, size_t baseOidLen, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const
{
   EnumerationCallbackResult result = _CONTINUE;
   const SNMP_Variable *curr = getNext(baseOid, baseOidLen);
   while((curr != nullptr) && (curr->getName().compare(baseOid, baseOidLen) == OID_LONGER))
   {
      result = callback(curr);
      if (result == _STOP)
         break;
      curr = getNext(curr->getName().value(), curr->getName().length());
   }
   return result;
}
