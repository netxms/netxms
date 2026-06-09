/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: localization.cpp
**
**/

#include "nxcore.h"
#include "nms_localization.h"

#define DEBUG_TAG L"localization"

// Composite-key separator: ASCII unit separator, not a legal character in tag or language strings
#define KEY_SEP L"\x1F"

/**
 * Build "fieldTag\x1Flanguage" composite key for the in-memory string map
 */
static inline StringBuffer BuildKey(const wchar_t *fieldTag, const wchar_t *language)
{
   StringBuffer key(fieldTag);
   key.append(KEY_SEP);
   key.append(language);
   return key;
}

/**
 * Resolve a (fieldTag, language) lookup with fallback chain:
 *   exact -> language-only (strip region suffix after '-') -> fallback
 * Empty string in storage is treated as absent so the editor can clear an entry.
 */
const wchar_t *LocalizedStringSet::resolve(const wchar_t *fieldTag, const wchar_t *language, const wchar_t *fallback) const
{
   if ((language == nullptr) || (*language == 0))
      return fallback;

   const wchar_t *value = m_strings.get(BuildKey(fieldTag, language));
   if ((value != nullptr) && (*value != 0))
      return value;

   const wchar_t *region = wcschr(language, L'-');
   if (region != nullptr)
   {
      wchar_t baseLang[8];
      size_t len = std::min(static_cast<size_t>(region - language), static_cast<size_t>(7));
      memcpy(baseLang, language, len * sizeof(wchar_t));
      baseLang[len] = 0;
      value = m_strings.get(BuildKey(fieldTag, baseLang));
      if ((value != nullptr) && (*value != 0))
         return value;
   }

   return fallback;
}

void LocalizedStringSet::set(const wchar_t *fieldTag, const wchar_t *language, const wchar_t *value)
{
   if ((fieldTag == nullptr) || (*fieldTag == 0) || (language == nullptr) || (*language == 0))
      return;
   m_strings.set(BuildKey(fieldTag, language), CHECK_NULL_EX(value));
}

void LocalizedStringSet::remove(const wchar_t *fieldTag, const wchar_t *language)
{
   if ((fieldTag == nullptr) || (language == nullptr))
      return;
   m_strings.remove(BuildKey(fieldTag, language));
}

/**
 * Serialize all entries into the message starting at baseId using the sub-block
 * layout defined in nms_localization.h.
 */
void LocalizedStringSet::fillMessage(NXCPMessage *msg, uint32_t countFieldId, uint32_t baseFieldId) const
{
   uint32_t count = 0;
   uint32_t fieldId = baseFieldId;
   for(KeyValuePair<const wchar_t> *e : m_strings)
   {
      const wchar_t *sep = wcschr(e->key, L'\x1F');
      if (sep == nullptr)
         continue;
      String fieldTag(e->key, sep - e->key);
      msg->setField(fieldId, fieldTag);
      msg->setField(fieldId + 1, sep + 1);
      msg->setField(fieldId + 2, e->value);
      fieldId += 3;
      count++;
   }
   msg->setField(countFieldId, count);
}

void LocalizedStringSet::forEach(std::function<void (const wchar_t *, const wchar_t *, const wchar_t *)> cb) const
{
   for(KeyValuePair<const wchar_t> *e : m_strings)
   {
      const wchar_t *sep = wcschr(e->key, L'\x1F');
      if (sep == nullptr)
         continue;
      String fieldTag(e->key, sep - e->key);
      cb(fieldTag, sep + 1, e->value);
   }
}

void LocalizedStringSet::loadFromMessage(const NXCPMessage& msg, uint32_t countFieldId, uint32_t baseFieldId)
{
   m_strings.clear();
   uint32_t count = msg.getFieldAsUInt32(countFieldId);
   uint32_t fieldId = baseFieldId;
   for(uint32_t i = 0; i < count; i++)
   {
      wchar_t fieldTag[32], language[8];
      msg.getFieldAsString(fieldId, fieldTag, 32);
      msg.getFieldAsString(fieldId + 1, language, 8);
      wchar_t *value = msg.getFieldAsString(fieldId + 2);
      set(fieldTag, language, CHECK_NULL_EX(value));
      MemFree(value);
      fieldId += 3;
   }
}

/**
 * Load all translations belonging to one entity instance
 */
bool LoadLocalizedStrings(const wchar_t *entityClass, uint32_t entityId, LocalizedStringSet *out)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"SELECT field_tag,language,value FROM localized_strings WHERE entity_class=? AND entity_id=?");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, entityClass, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, entityId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool success = (hResult != nullptr);
   if (success)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t fieldTag[32], language[8];
         DBGetField(hResult, i, 0, fieldTag, 32);
         DBGetField(hResult, i, 1, language, 8);
         wchar_t *value = DBGetField(hResult, i, 2, nullptr, 0);
         out->set(fieldTag, language, CHECK_NULL_EX(value));
         MemFree(value);
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Bulk-load all translations for one entity class, keyed by entity id.
 * Used by an entity-class owner at startup so it can attach a LocalizedStringSet
 * to each in-memory record in a single query.
 */
bool LoadLocalizedStringsForClass(const wchar_t *entityClass, HashMap<uint32_t, LocalizedStringSet> *out)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"SELECT entity_id,field_tag,language,value FROM localized_strings WHERE entity_class=?");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, entityClass, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool success = (hResult != nullptr);
   if (success)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t entityId = DBGetFieldULong(hResult, i, 0);
         wchar_t fieldTag[32], language[8];
         DBGetField(hResult, i, 1, fieldTag, 32);
         DBGetField(hResult, i, 2, language, 8);
         wchar_t *value = DBGetField(hResult, i, 3, nullptr, 0);

         LocalizedStringSet *set = out->get(entityId);
         if (set == nullptr)
         {
            set = new LocalizedStringSet();
            out->set(entityId, set);
         }
         set->set(fieldTag, language, CHECK_NULL_EX(value));
         MemFree(value);
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Replace all translations for an entity instance: delete-all then insert
 */
bool SaveLocalizedStrings(const wchar_t *entityClass, uint32_t entityId, const LocalizedStringSet& strings)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hDelete = DBPrepare(hdb,
      L"DELETE FROM localized_strings WHERE entity_class=? AND entity_id=?");
   if (hDelete == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }
   DBBind(hDelete, 1, DB_SQLTYPE_VARCHAR, entityClass, DB_BIND_STATIC);
   DBBind(hDelete, 2, DB_SQLTYPE_INTEGER, entityId);
   bool success = DBExecute(hDelete);
   DBFreeStatement(hDelete);

   if (success && !strings.isEmpty())
   {
      DB_STATEMENT hInsert = DBPrepare(hdb,
         L"INSERT INTO localized_strings (entity_class,entity_id,field_tag,language,value) VALUES (?,?,?,?,?)",
         strings.size() > 1);
      if (hInsert != nullptr)
      {
         for(KeyValuePair<const wchar_t> *e : strings.m_strings)
         {
            const wchar_t *sep = wcschr(e->key, L'\x1F');
            if (sep == nullptr)
               continue;
            String fieldTag(e->key, sep - e->key);
            const wchar_t *language = sep + 1;

            DBBind(hInsert, 1, DB_SQLTYPE_VARCHAR, entityClass, DB_BIND_STATIC);
            DBBind(hInsert, 2, DB_SQLTYPE_INTEGER, entityId);
            DBBind(hInsert, 3, DB_SQLTYPE_VARCHAR, fieldTag, DB_BIND_TRANSIENT);
            DBBind(hInsert, 4, DB_SQLTYPE_VARCHAR, language, DB_BIND_STATIC);
            DBBind(hInsert, 5, DB_SQLTYPE_TEXT, e->value, DB_BIND_STATIC);
            if (!DBExecute(hInsert))
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(hInsert);
      }
      else
      {
         success = false;
      }
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Remove all translations for one entity instance. Entity-class owners must call
 * this from their delete paths -- the table has no FK to the owner.
 */
bool DeleteLocalizedStrings(const wchar_t *entityClass, uint32_t entityId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      L"DELETE FROM localized_strings WHERE entity_class=? AND entity_id=?");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, entityClass, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, entityId);
   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}
