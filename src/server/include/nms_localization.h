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
** File: nms_localization.h
**
**/

#ifndef _nms_localization_h_
#define _nms_localization_h_

/**
 * Entity class identifiers used as the entity_class column in localized_strings.
 * Once shipped, a value must never change — DB rows reference it by string.
 */
#define LOCSTR_CLASS_OBJECT_TOOL        L"object_tool"
#define LOCSTR_CLASS_EVENT_TEMPLATE     L"event_template"
#define LOCSTR_CLASS_ALARM_CATEGORY     L"alarm_category"
#define LOCSTR_CLASS_DCI_THRESHOLD      L"dci_threshold"

/**
 * Common field tags. Entities may also define their own tag constants
 * for fields that are unique to them.
 */
#define LOCSTR_TAG_NAME                 L"name"
#define LOCSTR_TAG_DESCRIPTION          L"description"
#define LOCSTR_TAG_MESSAGE              L"message"
#define LOCSTR_TAG_CONFIRMATION_TEXT    L"confirmation_text"
#define LOCSTR_TAG_COMMAND_NAME         L"command_name"
#define LOCSTR_TAG_COMMAND_SHORT_NAME   L"command_short_name"

/**
 * Set of localized strings for one entity instance: (field_tag, language) -> value.
 * Storage is flat with composite keys; runtime path uses resolve() for direct lookup,
 * editor path uses fillMessage()/loadFromMessage() to round-trip the whole set.
 *
 * NXCP sub-block layout, written by fillMessage()/read by loadFromMessage():
 *   countFieldId            uint32  number of entries
 *   baseFieldId + i*3 + 0   string  field_tag
 *   baseFieldId + i*3 + 1   string  language
 *   baseFieldId + i*3 + 2   string  value
 * The caller chooses where to place countFieldId and baseFieldId inside the
 * owning entity's NXCP layout.
 */
class NXCORE_EXPORTABLE LocalizedStringSet
{
   friend bool SaveLocalizedStrings(const wchar_t *entityClass, uint32_t entityId, const LocalizedStringSet& strings);

private:
   StringMap m_strings;

public:
   LocalizedStringSet() : m_strings() { }

   /**
    * Return the best translation for (fieldTag, language) following the fallback chain:
    * exact match (e.g. "pt-BR") -> language-only ("pt") -> fallback. Empty strings in
    * the table are treated as absent so admins can clear a translation.
    */
   const wchar_t *resolve(const wchar_t *fieldTag, const wchar_t *language, const wchar_t *fallback) const;

   void set(const wchar_t *fieldTag, const wchar_t *language, const wchar_t *value);
   void remove(const wchar_t *fieldTag, const wchar_t *language);
   bool isEmpty() const { return m_strings.size() == 0; }
   size_t size() const { return m_strings.size(); }

   void fillMessage(NXCPMessage *msg, uint32_t countFieldId, uint32_t baseFieldId) const;
   void loadFromMessage(const NXCPMessage& msg, uint32_t countFieldId, uint32_t baseFieldId);

   void forEach(std::function<void (const wchar_t *fieldTag, const wchar_t *language, const wchar_t *value)> cb) const;
};

/**
 * Load all translations for a single entity instance.
 */
bool NXCORE_EXPORTABLE LoadLocalizedStrings(const wchar_t *entityClass, uint32_t entityId, LocalizedStringSet *out);

/**
 * Bulk-load all translations for an entity class, keyed by entity id.
 * Entries are created on demand for ids that have at least one translation.
 */
bool NXCORE_EXPORTABLE LoadLocalizedStringsForClass(const wchar_t *entityClass, HashMap<uint32_t, LocalizedStringSet> *out);

/**
 * Replace all translations for an entity instance with the given set
 * (delete-then-insert, run inside a transaction by the caller if needed).
 */
bool NXCORE_EXPORTABLE SaveLocalizedStrings(const wchar_t *entityClass, uint32_t entityId, const LocalizedStringSet& strings);

/**
 * Remove all translations for an entity instance. Each entity-class owner is
 * responsible for calling this from its own delete path.
 */
bool NXCORE_EXPORTABLE DeleteLocalizedStrings(const wchar_t *entityClass, uint32_t entityId);

#endif
