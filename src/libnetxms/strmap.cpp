/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: strmap.cpp
**
**/

#include "libnetxms.h"
#include "strmap-internal.h"
#include <nxcpapi.h>

/**
 * Copy constructor
 */
StringMap::StringMap(const StringMap &src) : StringMapBase(Ownership::True)
{
	m_objectOwner = src.m_objectOwner;
   m_ignoreCase = src.m_ignoreCase;
   m_objectDestructor = src.m_objectDestructor;

   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src.m_data, entry, tmp)
   {
      setObject(MemCopyString(m_ignoreCase ? entry->originalKey : entry->key), MemCopyString((TCHAR *)entry->value), true);
   }
}

/**
 * Create string map from NXCP message
 */
StringMap::StringMap(const NXCPMessage& msg, uint32_t baseFieldId, uint32_t sizeFieldId) : StringMapBase(Ownership::True)
{
   addAllFromMessage(msg, baseFieldId, sizeFieldId);
}

/**
 * Create string map from JSON document
 */
StringMap::StringMap(json_t *json) : StringMapBase(Ownership::True)
{
   addAllFromJson(json);
}

/**
 * Assignment
 */
StringMap& StringMap::operator =(const StringMap &src)
{
	clear();
	m_objectOwner = src.m_objectOwner;
   m_ignoreCase = src.m_ignoreCase;
   m_objectDestructor = src.m_objectDestructor;

   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src.m_data, entry, tmp)
   {
      setObject(MemCopyString(m_ignoreCase ? entry->originalKey : entry->key), MemCopyString((TCHAR *)entry->value), true);
   }
	return *this;
}

/**
 * Add all values from another string map
 */
void StringMap::addAll(const StringMap *src, bool (*filter)(const TCHAR *, const TCHAR *, void *), void *context)
{
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, src->m_data, entry, tmp)
   {
      const TCHAR *k = src->m_ignoreCase ? entry->originalKey : entry->key;
      if ((filter == NULL) || filter(k, static_cast<TCHAR*>(entry->value), context))
      {
         setObject(MemCopyString(k), MemCopyString(static_cast<TCHAR*>(entry->value)), true);
      }
   }
}

/**
 * Set value from INT32
 */
StringMap& StringMap::set(const TCHAR *key, int32_t value)
{
   TCHAR buffer[32];
   set(key, IntegerToString(value, buffer));
   return *this;
}

/**
 * Set value from UINT32
 */
StringMap& StringMap::set(const TCHAR *key, uint32_t value)
{
	TCHAR buffer[32];
   set(key, IntegerToString(value, buffer));
   return *this;
}

/**
 * Set value from INT64
 */
StringMap& StringMap::set(const TCHAR *key, int64_t value)
{
   TCHAR buffer[64];
   set(key, IntegerToString(value, buffer));
   return *this;
}

/**
 * Set value from UINT64
 */
StringMap& StringMap::set(const TCHAR *key, uint64_t value)
{
   TCHAR buffer[64];
   set(key, IntegerToString(value, buffer));
   return *this;
}

/**
 * Get value by key as INT32
 */
int32_t StringMap::getInt32(const TCHAR *key, int32_t defaultValue) const
{
	const TCHAR *value = get(key);
	if (value == nullptr)
		return defaultValue;
	return _tcstol(value, nullptr, 0);
}

/**
 * Get value by key as UINT32
 */
uint32_t StringMap::getUInt32(const TCHAR *key, uint32_t defaultValue) const
{
   const TCHAR *value = get(key);
   if (value == nullptr)
      return defaultValue;
   return _tcstoul(value, nullptr, 0);
}

/**
 * Get value by key as INT64
 */
int64_t StringMap::getInt64(const TCHAR *key, int64_t defaultValue) const
{
   const TCHAR *value = get(key);
   if (value == nullptr)
      return defaultValue;
   return _tcstoll(value, nullptr, 0);
}

/**
 * Get value by key as UINT64
 */
uint64_t StringMap::getUInt64(const TCHAR *key, uint64_t defaultValue) const
{
   const TCHAR *value = get(key);
   if (value == nullptr)
      return defaultValue;
   return _tcstoull(value, nullptr, 0);
}

/**
 * Get value by key as double
 */
double StringMap::getDouble(const TCHAR *key, double defaultValue) const
{
   const TCHAR *value = get(key);
   if (value == nullptr)
      return defaultValue;
   return _tcstod(value, nullptr);
}

/**
 * Get value by key as boolean
 */
bool StringMap::getBoolean(const TCHAR *key, bool defaultValue) const
{
	const TCHAR *value = get(key);
	if (value == nullptr)
		return defaultValue;
	if (!_tcsicmp(value, _T("false")))
		return false;
	if (!_tcsicmp(value, _T("true")))
		return true;
	return _tcstoul(value, nullptr, 0) != 0;
}

/**
 * Fill NXCP message with map data
 */
void StringMap::fillMessage(NXCPMessage *msg, uint32_t baseFieldId, uint32_t sizeFieldId) const
{
   msg->setField(sizeFieldId, static_cast<uint32_t>(size()));
   uint32_t id = baseFieldId;
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      msg->setField(id++, m_ignoreCase ? entry->originalKey : entry->key);
      msg->setField(id++, static_cast<TCHAR*>(entry->value));
   }
}

/**
 * Load data from NXCP message
 */
void StringMap::addAllFromMessage(const NXCPMessage& msg, uint32_t baseFieldId, uint32_t sizeFieldId)
{
   int count = msg.getFieldAsInt32(sizeFieldId);
   uint32_t id = baseFieldId;
   for(int i = 0; i < count; i++)
   {
      TCHAR *key = msg.getFieldAsString(id++);
      TCHAR *value = msg.getFieldAsString(id++);
      setPreallocated(key, value);
   }
}

/**
 * Load data from JSON document
 */
void StringMap::addAllFromJson(json_t *json)
{
   if (!json_is_object(json))
      return;

   const char *key;
   json_t *value;
   json_object_foreach(json, key, value)
   {
      if (json_is_string(value))
      {
#ifdef UNICODE
         setPreallocated(WideStringFromUTF8String(key), WideStringFromUTF8String(json_string_value(value)));
#else
         set(key, json_string_value(value));
#endif
      }
      else if (json_is_integer(value))
      {
         char buffer[32];
         IntegerToString(static_cast<int64_t>(json_integer_value(value)), buffer);
#ifdef UNICODE
         setPreallocated(WideStringFromUTF8String(key), WideStringFromUTF8String(buffer));
#else
         set(key, buffer);
#endif
      }
      else if (json_is_real(value))
      {
         char buffer[32];
         snprintf(buffer, 32, "%f", json_real_value(value));
#ifdef UNICODE
         setPreallocated(WideStringFromUTF8String(key), WideStringFromUTF8String(buffer));
#else
         set(key, buffer);
#endif
      }
      else if (json_is_boolean(value))
      {
#ifdef UNICODE
         setPreallocated(WideStringFromUTF8String(key), MemCopyString(BooleanToString(json_is_true(value))));
#else
         set(key, BooleanToString(json_is_true(value)));
#endif
      }
   }
}

/**
 * Serialize as JSON
 */
json_t *StringMap::toJson() const
{
   json_t *root = json_object();
   StringMapEntry *entry, *tmp;
   HASH_ITER(hh, m_data, entry, tmp)
   {
      char *key = UTF8StringFromTString(m_ignoreCase ? entry->originalKey : entry->key);
      json_object_set_new(root, key, json_string_t(static_cast<TCHAR*>(entry->value)));
      MemFree(key);
   }
   return root;
}
