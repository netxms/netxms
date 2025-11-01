/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2025 Raden Solutions
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
 ** File: config.cpp
 **
 **/

#include "libnetxms.h"
#include <nxconfig.h>
#include <expat.h>
#include <nxstat.h>

#if !HAVE_ISATTY
#define _isatty(x) (true)
#endif

/**
 * Expand value
 */
static StringBuffer ExpandValue(const TCHAR *src, bool xmlFormat, bool expandEnv)
{
   if (xmlFormat && !expandEnv)
      return StringBuffer(src);

   StringBuffer out;

   const TCHAR *in = src;
   bool squotes = false;
   bool dquotes = false;
   if ((*in == _T('"')) && !xmlFormat)
   {
      dquotes = true;
      in++;
   }
   else if ((*in == _T('\'')) && !xmlFormat)
   {
      squotes = true;
      in++;
   }

   for(; *in != 0; in++)
   {
      if (squotes && (*in == _T('\'')))
      {
         // Single quoting characters are ignored in quoted string
         if (*(in + 1) == _T('\''))
         {
            in++;
            out.append(_T('\''));
         }
      }
      else if (dquotes && (*in == _T('"')))
      {
         // Double quoting characters are ignored in quoted string
         if (*(in + 1) == _T('"'))
         {
            in++;
            out.append(_T('"'));
         }
      }
      else if (!squotes && expandEnv && (*in == _T('$')))
      {
         if (*(in + 1) == _T('{'))  // environment string expansion
         {
            const TCHAR *end = _tcschr(in, _T('}'));
            if (end != nullptr)
            {
               in += 2;

               TCHAR name[256];
               size_t nameLen = end - in;
               if (nameLen >= 256)
                  nameLen = 255;
               memcpy(name, in, nameLen * sizeof(TCHAR));
               name[nameLen] = 0;
               String env = GetEnvironmentVariableEx(name);
               if (!env.isEmpty())
               {
                  out.append(env);
               }
               in = end;
            }
            else
            {
               // unexpected end of line, ignore anything after ${
               break;
            }
         }
         else
         {
            out.append(*in);
         }
      }
      else
      {
         out.append(*in);
      }
   }
   return out;
}

/**
 * Constructor for config entry
 */
ConfigEntry::ConfigEntry(const TCHAR *name, ConfigEntry *parent, const Config *owner, const TCHAR *file, int line, int id)
{
   m_name = MemCopyString(CHECK_NULL(name));
   m_first = nullptr;
   m_last = nullptr;
   m_next = nullptr;
   m_parent = nullptr;
   if (parent != nullptr)
      parent->addEntry(this);
   m_file = MemCopyString(CHECK_NULL(file));
   m_line = line;
   m_id = id;
   m_owner = owner;
}

/**
 * Copy constructor (do not copy sub-entries)
 */
ConfigEntry::ConfigEntry(const ConfigEntry *src, const Config *owner)
{
   m_name = MemCopyString(src->m_name);
   m_first = nullptr;
   m_last = nullptr;
   m_next = nullptr;
   m_parent = nullptr;
   m_values.addAll(&src->m_values);
   m_attributes.addAll(&src->m_attributes);
   m_file = MemCopyString(src->m_file);
   m_line = src->m_line;
   m_id = src->m_id;
   m_owner = owner;
}

/**
 * Destructor for config entry
 */
ConfigEntry::~ConfigEntry()
{
   ConfigEntry *entry, *next;
   for(entry = m_first; entry != nullptr; entry = next)
   {
      next = entry->getNext();
      delete entry;
   }
   MemFree(m_name);
   MemFree(m_file);
}

/**
 * Set entry's name
 */
void ConfigEntry::setName(const TCHAR *name)
{
   MemFree(m_name);
   m_name = MemCopyString(CHECK_NULL(name));
}

/**
 * Find entry by name
 */
ConfigEntry* ConfigEntry::findEntry(const TCHAR *name) const
{
   const TCHAR *realName;
   if (name[0] == _T('%'))
   {
      const TCHAR *alias = m_owner->getAlias(&name[1]);
      if (alias == nullptr)
         return nullptr;
      realName = alias;
   }
   else
   {
      realName = name;
   }
   for(ConfigEntry *e = m_first; e != nullptr; e = e->getNext())
      if (!_tcsicmp(e->getName(), realName))
         return e;
   return nullptr;
}

/**
 * Create (or find existing) subentry
 */
ConfigEntry* ConfigEntry::findOrCreateEntry(const TCHAR *name)
{
   const TCHAR *realName;
   if (name[0] == _T('%'))
   {
      const TCHAR *alias = m_owner->getAlias(&name[1]);
      realName = (alias != nullptr) ? alias : &name[1];
   }
   else
   {
      realName = name;
   }

   for(ConfigEntry *e = m_first; e != nullptr; e = e->getNext())
      if (!_tcsicmp(e->getName(), realName))
         return e;

   return new ConfigEntry(realName, this, m_owner, _T("<memory>"), 0, 0);
}

/**
 * Add sub-entry
 */
void ConfigEntry::addEntry(ConfigEntry *entry)
{
   entry->m_parent = this;
   entry->m_next = nullptr;
   if (m_last != nullptr)
      m_last->m_next = entry;
   m_last = entry;
   if (m_first == nullptr)
      m_first = entry;
}

/**
 * Unlink subentry
 */
void ConfigEntry::unlinkEntry(ConfigEntry *entry)
{
   ConfigEntry *curr, *prev;
   for(curr = m_first, prev = nullptr; curr != nullptr; curr = curr->m_next)
   {
      if (curr == entry)
      {
         if (prev != nullptr)
         {
            prev->m_next = curr->m_next;
         }
         else
         {
            m_first = curr->m_next;
         }
         if (m_last == curr)
            m_last = prev;
         curr->m_next = nullptr;
         break;
      }
      prev = curr;
   }
}

/**
 * Get all subentries with names matched to mask.
 * Returned list ordered by ID
 */
unique_ptr<ObjectArray<ConfigEntry>> ConfigEntry::getSubEntries(const TCHAR *mask) const
{
   auto list = make_unique<ObjectArray<ConfigEntry>>(16, 16, Ownership::False);
   for(ConfigEntry *e = m_first; e != nullptr; e = e->getNext())
      if ((mask == nullptr) || MatchString(mask, e->getName(), FALSE))
      {
         list->add(e);
      }
   return list;
}

/**
 * Get all subentries with names matched to mask ordered by id
 */
unique_ptr<ObjectArray<ConfigEntry>> ConfigEntry::getOrderedSubEntries(const TCHAR *mask) const
{
   unique_ptr<ObjectArray<ConfigEntry>> list = getSubEntries(mask);
   list->sort([] (const ConfigEntry **e1, const ConfigEntry **e2) -> int { return (*e1)->getId() - (*e2)->getId(); });
   return list;
}

/**
 * Get entry value as integer
 */
int32_t ConfigEntry::getValueAsInt(int index, int32_t defaultValue) const
{
   const TCHAR *value = getValue(index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

/**
 * Get entry value as unsigned integer
 */
uint32_t ConfigEntry::getValueAsUInt(int index, uint32_t defaultValue) const
{
   const TCHAR *value = getValue(index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get entry value as 64 bit integer
 */
int64_t ConfigEntry::getValueAsInt64(int index, int64_t defaultValue) const
{
   const TCHAR *value = getValue(index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

/**
 * Get entry value as 64 bit unsigned integer
 */
uint64_t ConfigEntry::getValueAsUInt64(int index, uint64_t defaultValue) const
{
   const TCHAR *value = getValue(index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get entry value as boolean
 * (consider non-zero numerical value or strings "yes", "true", "on" as true)
 */
bool ConfigEntry::getValueAsBoolean(int index, bool defaultValue) const
{
   const TCHAR *value = getValue(index);
   if (value != nullptr)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
         || (_tcstol(value, nullptr, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Get entry value as GUID
 */
uuid ConfigEntry::getValueAsUUID(int index) const
{
   const TCHAR *value = getValue(index);
   if (value != nullptr)
   {
      return uuid::parse(value);
   }
   return uuid::NULL_UUID;
}

/**
 * Set value (replace all existing values)
 */
void ConfigEntry::setValue(const TCHAR *value)
{
   m_values.clear();
   m_values.add(value);
}

/**
 * Get all values concatenated
 */
String ConfigEntry::getConcatenatedValues(const TCHAR *separator) const
{
   if (m_values.isEmpty())
      return String();

   StringBuffer result(m_values.get(0));
   for(int i = 1; i < m_values.size(); i++)
   {
      result.append(separator);
      result.append(m_values.get(i));
   }
   return result;
}

/**
 * Get summary length of all values as if they was concatenated with separator character
 */
int ConfigEntry::getConcatenatedValuesLength() const
{
   if (m_values.isEmpty())
      return 0;

   int len = 0;
   for(int i = 0; i < m_values.size(); i++)
      len += (int)_tcslen(m_values.get(i));
   return len + m_values.size();
}

/**
 * Get value for given subentry
 *
 * @param name sub-entry name
 * @param index sub-entry index (0 if omited)
 * @param defaultValue value to be returned if requested sub-entry is missing (nullptr if omited)
 * @return sub-entry value or default value if requested sub-entry does not exist
 */
const TCHAR* ConfigEntry::getSubEntryValue(const TCHAR *name, int index, const TCHAR *defaultValue) const
{
   ConfigEntry *e = findEntry(name);
   if (e == nullptr)
      return defaultValue;
   const TCHAR *value = e->getValue(index);
   return (value != nullptr) ? value : defaultValue;
}

int32_t ConfigEntry::getSubEntryValueAsInt(const TCHAR *name, int index, int32_t defaultValue) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

uint32_t ConfigEntry::getSubEntryValueAsUInt(const TCHAR *name, int index, uint32_t defaultValue) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

int64_t ConfigEntry::getSubEntryValueAsInt64(const TCHAR *name, int index, int64_t defaultValue) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

uint64_t ConfigEntry::getSubEntryValueAsUInt64(const TCHAR *name, int index, uint64_t defaultValue) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get sub-entry value as boolean
 * (consider non-zero numerical value or strings "yes", "true", "on" as true)
 */
bool ConfigEntry::getSubEntryValueAsBoolean(const TCHAR *name, int index, bool defaultValue) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   if (value != nullptr)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on")) || (_tcstol(value, nullptr, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Get sub-entry value as UUID
 */
uuid ConfigEntry::getSubEntryValueAsUUID(const TCHAR *name, int index) const
{
   const TCHAR *value = getSubEntryValue(name, index);
   if (value != nullptr)
   {
      return uuid::parse(value);
   }
   else
   {
      return uuid::NULL_UUID;
   }
}

/**
 * Get attribute as integer
 */
int32_t ConfigEntry::getAttributeAsInt(const TCHAR *name, int32_t defaultValue) const
{
   const TCHAR *value = getAttribute(name);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

/**
 * Get attribute as unsigned integer
 */
uint32_t ConfigEntry::getAttributeAsUInt(const TCHAR *name, uint32_t defaultValue) const
{
   const TCHAR *value = getAttribute(name);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get attribute as 64 bit integer
 */
int64_t ConfigEntry::getAttributeAsInt64(const TCHAR *name, int64_t defaultValue) const
{
   const TCHAR *value = getAttribute(name);
   return (value != nullptr) ? _tcstoll(value, nullptr, 0) : defaultValue;
}

/**
 * Get attribute as unsigned 64 bit integer
 */
uint64_t ConfigEntry::getAttributeAsUInt64(const TCHAR *name, uint64_t defaultValue) const
{
   const TCHAR *value = getAttribute(name);
   return (value != nullptr) ? _tcstoull(value, nullptr, 0) : defaultValue;
}

/**
 * Get attribute as boolean
 * (consider non-zero numerical value or strings "yes", "true", "on" as true)
 */
bool ConfigEntry::getAttributeAsBoolean(const TCHAR *name, bool defaultValue) const
{
   const TCHAR *value = getAttribute(name);
   if (value != nullptr)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on")) || (_tcstol(value, nullptr, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Set attribute
 */
void ConfigEntry::setAttribute(const TCHAR *name, int32_t value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%d"), (int)value);
   setAttribute(name, buffer);
}

/**
 * Set attribute
 */
void ConfigEntry::setAttribute(const TCHAR *name, uint32_t value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%u"), (unsigned int)value);
   setAttribute(name, buffer);
}

/**
 * Set attribute
 */
void ConfigEntry::setAttribute(const TCHAR *name, int64_t value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, INT64_FMT, value);
   setAttribute(name, buffer);
}

/**
 * Set attribute
 */
void ConfigEntry::setAttribute(const TCHAR *name, uint64_t value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, UINT64_FMT, value);
   setAttribute(name, buffer);
}

/**
 * Set attribute
 */
void ConfigEntry::setAttribute(const TCHAR *name, bool value)
{
   setAttribute(name, value ? _T("true") : _T("false"));
}

/**
 * Add subtree to this entry
 */
void ConfigEntry::addSubTree(const ConfigEntry *root, bool merge)
{
   for(ConfigEntry *src = root->m_first; src != nullptr; src = src->m_next)
   {
      ConfigEntry *dst;
      if (merge)
      {
         if (m_owner->getMergeStrategy() != nullptr)
         {
            dst = m_owner->getMergeStrategy()(this, src->m_name);
         }
         else
         {
            dst = findEntry(src->m_name);
         }
         if (dst != nullptr)
         {
            dst->m_values.addAll(&src->m_values);
         }
         else
         {
            dst = new ConfigEntry(src, m_owner);
            addEntry(dst);
         }
      }
      else
      {
         dst = new ConfigEntry(src, m_owner);
         addEntry(dst);
      }
      dst->addSubTree(src, merge);
   }
}

/**
 * Print config entry
 */
void ConfigEntry::print(FILE *file, StringList *slist, int level, TCHAR *prefix) const
{
   bool maskValue;
   if (file != nullptr)
   {
      if (_isatty(_fileno(file)))
         WriteToTerminalEx(_T("%s\x1b[32;1m%s\x1b[0m\n"), prefix, m_name);
      else
         _tprintf(_T("%s%s\n"), prefix, m_name);
      maskValue = false;
   }
   else if (slist != nullptr)
   {
      StringBuffer sb(prefix);
      sb.append(m_name);
      slist->add(sb);

      TCHAR name[256];
      _tcslcpy(name, m_name, 256);
      _tcslwr(name);
      maskValue = (_tcsstr(name, _T("password")) != nullptr) || (_tcsstr(name, _T("secret")) != nullptr) || (_tcsstr(name, _T("token")) != nullptr);
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, _T("config"), _T("%s%s"), prefix, m_name);

      TCHAR name[256];
      _tcslcpy(name, m_name, 256);
      _tcslwr(name);
      maskValue = (_tcsstr(name, _T("password")) != nullptr) || (_tcsstr(name, _T("secret")) != nullptr) || (_tcsstr(name, _T("token")) != nullptr);
   }

   if (level > 0)
   {
      prefix[(level - 1) * 4 + 1] = (m_next == nullptr) ? _T(' ') : _T('|');
      prefix[(level - 1) * 4 + 2] = _T(' ');
   }

   // Do not print empty values for non-leaf nodes
   if ((m_first == nullptr) || (!m_values.isEmpty() && (*m_values.get(0) != 0)))
   {
      for(int i = 0; i < m_values.size(); i++)
      {
         if (file != nullptr)
         {
            if (_isatty(_fileno(file)))
               WriteToTerminalEx(_T("%s  value: \x1b[1m%s\x1b[0m\n"), prefix, m_values.get(i));
            else
               _tprintf(_T("%s  value: %s\n"), prefix, m_values.get(i));
         }
         else if (slist != nullptr)
         {
            StringBuffer sb(prefix);
            sb.append(_T("  value: "));
            sb.append(maskValue ? _T("********") : m_values.get(i)); // Mask values likely containing passwords
            slist->add(sb);
         }
         else
         {
            // Mask values likely containing passwords
            if (maskValue)
               nxlog_write_tag(NXLOG_INFO, _T("config"), _T("%s  value: ********"), prefix);
            else
               nxlog_write_tag(NXLOG_INFO, _T("config"), _T("%s  value: %s"), prefix, m_values.get(i));
         }
      }
   }

   for(ConfigEntry *e = m_first; e != nullptr; e = e->getNext())
   {
      _tcscat(prefix, _T(" +- "));
      e->print(file, slist, level + 1, prefix);
      prefix[level * 4] = 0;
   }
}

/**
 * Add attribute
 */
static EnumerationCallbackResult AddAttribute(const TCHAR *key, const void *value, void *userData)
{
   if (_tcscmp(key, _T("id")))
      static_cast<StringBuffer*>(userData)->appendFormattedString(_T(" %s=\"%s\""), key, static_cast<const TCHAR*>(value));
   return _CONTINUE;
}

/**
 * Create XML element(s) from config entry
 */
void ConfigEntry::createXml(StringBuffer &xml, int level) const
{
   TCHAR *name = MemCopyString(m_name);
   TCHAR *ptr = _tcschr(name, _T('#'));
   if (ptr != nullptr)
      *ptr = 0;

   if (m_id == 0)
      xml.appendFormattedString(_T("%*s<%s"), level * 4, _T(""), name);
   else
      xml.appendFormattedString(_T("%*s<%s id=\"%d\""), level * 4, _T(""), name, m_id);
   m_attributes.forEach(AddAttribute, &xml);
   xml += _T(">");

   if (m_first != nullptr)
   {
      xml += _T("\n");
      for(ConfigEntry *e = m_first; e != nullptr; e = e->getNext())
         e->createXml(xml, level + 1);
      xml.appendFormattedString(_T("%*s"), level * 4, _T(""));
   }

   if (!m_values.isEmpty())
      xml.appendPreallocated(EscapeStringForXML(m_values.get(0), -1));
   xml.appendFormattedString(_T("</%s>\n"), name);

   for(int i = 1; i < m_values.size(); i++)
   {
      if ((*m_values.get(i) == 0) && (m_first != nullptr))
         continue;   // Skip empty values for non-leaf elements

      if (m_id == 0)
         xml.appendFormattedString(_T("%*s<%s>"), level * 4, _T(""), name);
      else
         xml.appendFormattedString(_T("%*s<%s id=\"%d\">"), level * 4, _T(""), name, m_id);
      xml.appendPreallocated(EscapeStringForXML(m_values.get(i), -1));
      xml.appendFormattedString(_T("</%s>\n"), name);
   }

   MemFree(name);
}

/**
 * Constructor for config
 */
Config::Config(bool allowMacroExpansion)
{
   m_root = new ConfigEntry(_T("config"), nullptr, this, nullptr, 0, 0);
   m_errorCount = 0;
   m_allowMacroExpansion = allowMacroExpansion;
   m_mergeStrategy = nullptr;
   m_logErrors = true;
}

/**
 * Destructor
 */
Config::~Config()
{
   delete m_root;
}

/**
 * Default error handler
 */
void Config::onError(const TCHAR *errorMessage)
{
   nxlog_write_tag(NXLOG_ERROR, _T("config"), _T("Configuration loading error: %s"), errorMessage);
}

/**
 * Report error
 */
void Config::error(const TCHAR *format, ...)
{
   if (!m_logErrors)
      return;

   va_list args;
   TCHAR buffer[4096];

   m_errorCount++;
   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);
   onError(buffer);
}

/**
 * Parse size specification (with K, M, G, or T suffixes)
 */
static uint64_t ParseSize(const TCHAR *s, uint64_t multiplier)
{
   TCHAR *eptr;
   uint64_t value = _tcstoull(s, &eptr, 0);
   while(*eptr == ' ')
      eptr++;
   if ((*eptr == 'K') || (*eptr == 'k'))
      return value * multiplier;
   if ((*eptr == 'M') || (*eptr == 'm'))
      return value * multiplier * multiplier;
   if ((*eptr == 'G') || (*eptr == 'g'))
      return value * multiplier * multiplier * multiplier;
   if ((*eptr == 'T') || (*eptr == 't'))
      return value * multiplier * multiplier * multiplier * multiplier;
   return value;
}

/**
 * Parse configuration template (emulation of old NxLoadConfig() API)
 */
bool Config::parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate)
{
   TCHAR name[MAX_PATH], *curr, *eptr;
   int i, pos, initialErrorCount = m_errorCount;
   ConfigEntry *entry;

   name[0] = _T('/');
   _tcslcpy(&name[1], section, MAX_PATH - 2);
   _tcscat(name, _T("/"));
   pos = (int)_tcslen(name);

   for(i = 0; cfgTemplate[i].type != CT_END_OF_LIST; i++)
   {
      _tcslcpy(&name[pos], cfgTemplate[i].token, MAX_PATH - pos);
      entry = getEntry(name);
      if (entry != nullptr)
      {
         const TCHAR *value = CHECK_NULL(entry->getValue(entry->getValueCount() - 1));
         switch(cfgTemplate[i].type)
         {
            case CT_LONG:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((int32_t *)cfgTemplate[i].overrideIndicator) != NXCONFIG_UNINITIALIZED_VALUE))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               *((int32_t *)cfgTemplate[i].buffer) = _tcstol(value, &eptr, 0);
               if (*eptr != 0)
               {
                  error(_T("Invalid number '%s' in configuration file %s at line %d\n"), value, entry->getFile(), entry->getLine());
               }
               break;
            case CT_WORD:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((INT16 *)cfgTemplate[i].overrideIndicator) != NXCONFIG_UNINITIALIZED_VALUE))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               *((UINT16 *)cfgTemplate[i].buffer) = (UINT16)_tcstoul(value, &eptr, 0);
               if (*eptr != 0)
               {
                  error(_T("Invalid number '%s' in configuration file %s at line %d\n"), value, entry->getFile(), entry->getLine());
               }
               break;
            case CT_BOOLEAN_FLAG_32:
               if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
                  || !_tcsicmp(value, _T("1")))
               {
                  *static_cast<uint32_t*>(cfgTemplate[i].buffer) |= (uint32_t)cfgTemplate[i].bufferSize;
               }
               else
               {
                  *static_cast<uint32_t*>(cfgTemplate[i].buffer) &= ~((uint32_t)cfgTemplate[i].bufferSize);
               }
               break;
            case CT_BOOLEAN_FLAG_64:
               if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
                  || !_tcsicmp(value, _T("1")))
               {
                  *static_cast<uint64_t*>(cfgTemplate[i].buffer) |= cfgTemplate[i].bufferSize;
               }
               else
               {
                  *static_cast<uint64_t*>(cfgTemplate[i].buffer) &= ~(cfgTemplate[i].bufferSize);
               }
               break;
            case CT_BOOLEAN:
               if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
                  || !_tcsicmp(value, _T("1")))
               {
                  *static_cast<bool*>(cfgTemplate[i].buffer) = true;
               }
               else
               {
                  *static_cast<bool*>(cfgTemplate[i].buffer) = false;
               }
               break;
            case CT_STRING:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((TCHAR *)cfgTemplate[i].overrideIndicator) != 0))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               _tcslcpy((TCHAR *)cfgTemplate[i].buffer, value, (size_t)cfgTemplate[i].bufferSize);
               break;
            case CT_MB_STRING:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((char *)cfgTemplate[i].overrideIndicator) != 0))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
#ifdef UNICODE
               memset(cfgTemplate[i].buffer, 0, (size_t)cfgTemplate[i].bufferSize);
               wchar_to_mb(value, -1, (char *)cfgTemplate[i].buffer, (int)cfgTemplate[i].bufferSize - 1);
#else
               strlcpy((TCHAR *)cfgTemplate[i].buffer, value, (size_t)cfgTemplate[i].bufferSize);
#endif
               break;
            case CT_STRING_CONCAT:
               if (*static_cast<TCHAR**>(cfgTemplate[i].buffer) != nullptr)
               {
                  *static_cast<TCHAR**>(cfgTemplate[i].buffer) = MemRealloc(*static_cast<TCHAR**>(cfgTemplate[i].buffer), (_tcslen(*static_cast<TCHAR**>(cfgTemplate[i].buffer)) + entry->getConcatenatedValuesLength() + 1) * sizeof(TCHAR));
                  curr = *static_cast<TCHAR**>(cfgTemplate[i].buffer) + _tcslen(*static_cast<TCHAR**>(cfgTemplate[i].buffer));
               }
               else
               {
                  *static_cast<TCHAR**>(cfgTemplate[i].buffer) = MemAllocString(entry->getConcatenatedValuesLength() + 1);
                  curr = *static_cast<TCHAR**>(cfgTemplate[i].buffer);
               }
               for(int j = 0; j < entry->getValueCount(); j++)
               {
                  _tcscpy(curr, entry->getValue(j));
                  curr += _tcslen(curr);
                  *curr = cfgTemplate[i].separator;
                  curr++;
               }
               *curr = 0;
               break;
            case CT_STRING_SET:
               for (int j = 0; j < entry->getValueCount(); j++)
                  static_cast<StringSet*>(cfgTemplate[i].buffer)->add(entry->getValue(j));
               break;
            case CT_STRING_LIST:
               for (int j = 0; j < entry->getValueCount(); j++)
                  static_cast<StringList*>(cfgTemplate[i].buffer)->add(entry->getValue(j));
               break;
            case CT_SIZE_BYTES:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((int32_t *)cfgTemplate[i].overrideIndicator) != NXCONFIG_UNINITIALIZED_VALUE))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               *((uint64_t *)cfgTemplate[i].buffer) = ParseSize(value, 1024);
               break;
            case CT_SIZE_UNITS:
               if ((cfgTemplate[i].overrideIndicator != nullptr) &&
                   (*((int32_t *)cfgTemplate[i].overrideIndicator) != NXCONFIG_UNINITIALIZED_VALUE))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               *((uint64_t *)cfgTemplate[i].buffer) = ParseSize(value, 1000);
               break;
            case CT_IGNORE:
               break;
            default:
               break;
         }
      }
   }

   return (m_errorCount - initialErrorCount) == 0;
}

/**
 * Get value
 */
const TCHAR *Config::getValue(const TCHAR *path, const TCHAR *defaultValue, int index) const
{
   const TCHAR *value;
   ConfigEntry *entry = getEntry(path);
   if (entry != nullptr)
   {
      value = entry->getValue(index);
      if (value == nullptr)
         value = defaultValue;
   }
   else
   {
      value = defaultValue;
   }
   return value;
}

/**
 * Get first non-empty value
 */
const TCHAR *Config::getFirstNonEmptyValue(const TCHAR *path) const
{
   const TCHAR *value = nullptr;
   ConfigEntry *entry = getEntry(path);
   if (entry != nullptr)
   {
      for (int i = 0; i < entry->getValueCount(); i++)
      {
         const TCHAR *v = entry->getValue(i);
         if ((v != nullptr) && (*v != 0))
         {
            value = v;
            break;
         }
      }
   }
   return value;
}

/**
 * Get value as 32 bit integer
 */
int32_t Config::getValueAsInt(const TCHAR *path, int32_t defaultValue, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

/**
 * Get value as unsigned 32 bit integer
 */
uint32_t Config::getValueAsUInt(const TCHAR *path, uint32_t defaultValue, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get value as 64 bit integer
 */
int64_t Config::getValueAsInt64(const TCHAR *path, int64_t defaultValue, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
}

/**
 * Get value as unsigned 64 bit integer
 */
uint64_t Config::getValueAsUInt64(const TCHAR *path, uint64_t defaultValue, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
}

/**
 * Get value as boolean
 */
bool Config::getValueAsBoolean(const TCHAR *path, bool defaultValue, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   if (value != nullptr)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
         || (_tcstol(value, nullptr, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Get value at given path as UUID
 */
uuid Config::getValueAsUUID(const TCHAR *path, int index) const
{
   const TCHAR *value = getValue(path, nullptr, index);
   if (value != nullptr)
   {
      return uuid::parse(value);
   }
   else
   {
      return uuid::NULL_UUID;
   }
}

/**
 * Get subentries
 */
unique_ptr<ObjectArray<ConfigEntry>> Config::getSubEntries(const TCHAR *path, const TCHAR *mask) const
{
   ConfigEntry *entry = getEntry(path);
   return (entry != nullptr) ? entry->getSubEntries(mask) : nullptr;
}

/**
 * Get subentries ordered by id
 */
unique_ptr<ObjectArray<ConfigEntry>> Config::getOrderedSubEntries(const TCHAR *path, const TCHAR *mask) const
{
   ConfigEntry *entry = getEntry(path);
   return (entry != nullptr) ? entry->getOrderedSubEntries(mask) : nullptr;
}

/**
 * Get entry
 */
ConfigEntry *Config::getEntry(const TCHAR *path) const
{
   if ((path == nullptr) || (*path != _T('/')))
      return nullptr;

   if (!_tcscmp(path, _T("/")))
      return m_root;

   TCHAR name[256];
   const TCHAR *curr, *end;
   ConfigEntry *entry = m_root;

   curr = path + 1;
   while(entry != nullptr)
   {
      end = _tcschr(curr, _T('/'));
      if (end != nullptr)
      {
         int len = std::min((int)(end - curr), 255);
         _tcsncpy(name, curr, len);
         name[len] = 0;
         entry = entry->findEntry(name);
         curr = end + 1;
      }
      else
      {
         return entry->findEntry(curr);
      }
   }
   return nullptr;
}

/**
 * Create entry if does not exist, or return existing
 * Will return nullptr on error
 */
ConfigEntry *Config::getOrCreateEntry(const TCHAR *path)
{
   const TCHAR *curr, *end;
   TCHAR name[256];
   ConfigEntry *entry, *parent;

   if ((path == nullptr) || (*path != _T('/')))
      return nullptr;

   if (!_tcscmp(path, _T("/")))
      return m_root;

   curr = path + 1;
   parent = m_root;
   do
   {
      end = _tcschr(curr, _T('/'));
      if (end != nullptr)
      {
         int len = std::min((int)(end - curr), 255);
         _tcsncpy(name, curr, len);
         name[len] = 0;
         entry = parent->findEntry(name);
         curr = end + 1;
         if (entry == nullptr)
            entry = new ConfigEntry(name, parent, this, _T("<memory>"), 0, 0);
         parent = entry;
      }
      else
      {
         entry = parent->findEntry(curr);
         if (entry == nullptr)
            entry = new ConfigEntry(curr, parent, this, _T("<memory>"), 0, 0);
      }
   }
   while(end != nullptr);
   return entry;
}

/**
 * Delete entry
 */
void Config::deleteEntry(const TCHAR *path)
{
   ConfigEntry *entry = getEntry(path);
   if (entry == nullptr)
      return;

   ConfigEntry *parent = entry->getParent();
   if (parent == nullptr)	// root entry
      return;

   parent->unlinkEntry(entry);
   delete entry;
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, const TCHAR *value)
{
   ConfigEntry *entry = getOrCreateEntry(path);
   if (entry == nullptr)
      return false;
   entry->setValue(value);
   return true;
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, int32_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%d"), value);
   return setValue(path, buffer);
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, uint32_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%u"), value);
   return setValue(path, buffer);
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, int64_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, INT64_FMT, value);
   return setValue(path, buffer);
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, uint64_t value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, UINT64_FMT, value);
   return setValue(path, buffer);
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, double value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%f"), value);
   return setValue(path, buffer);
}

/**
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */
bool Config::setValue(const TCHAR *path, const uuid& value)
{
   TCHAR buffer[64];
   value.toString(buffer);
   return setValue(path, buffer);
}

/**
 * Find comment start in INI style config line
 * Comment starts with # character, characters within single or double quotes ignored
 */
static TCHAR *FindComment(TCHAR *str)
{
   TCHAR *curr = str;
   while(_istspace(*curr))
      curr++;

   if (*curr == _T('['))
   {
      curr = _tcschr(curr, _T(']'));
      if (curr == nullptr)
         return nullptr;
   }

   bool squotes = false, dquotes = false;
   for(; *curr != 0; curr++)
   {
      if (*curr == _T('"'))
      {
         if (!squotes)
            dquotes = !dquotes;
      }
      else if (*curr == _T('\''))
      {
         if (!dquotes)
            squotes = !squotes;
      }
      else if ((*curr == _T('#')) && !squotes && !dquotes)
      {
         return curr;
      }
   }
   return nullptr;
}

/**
 * Load INI-style config
 */
bool Config::loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection, bool ignoreErrors)
{
   bool success;
   size_t size;
   BYTE *content = LoadFile(file, &size);
   if (content != nullptr)
   {
      success = loadIniConfigFromMemory(reinterpret_cast<char*>(content), size, file, defaultIniSection, ignoreErrors);
      MemFree(content);
   }
   else
   {
      success = false;
   }
   return success;
}

/**
 * Load INI-style config
 */
bool Config::loadIniConfigFromMemory(const char *content, size_t length, const TCHAR *fileName, const TCHAR *defaultIniSection, bool ignoreErrors)
{
   TCHAR buffer[4096], *ptr;
   ConfigEntry *currentSection;
   int sourceLine = 0;
   bool validConfig = true;

   currentSection = m_root->findEntry(defaultIniSection);
   if (currentSection == nullptr)
   {
      currentSection = new ConfigEntry(defaultIniSection, m_root, this, fileName, 0, 0);
   }

   const char *curr = content;
   const char *next = curr;
   while(next != nullptr)
   {
      // Read line from file
      next = static_cast<const char*>(memchr(curr, '\n', length - (curr - content)));
      size_t llen = (next != nullptr) ? next - curr : length - (curr - content);
#ifdef UNICODE
      llen = utf8_to_wchar(curr, static_cast<ssize_t>(llen), buffer, 4095);
#else
      llen = MIN(llen, 4095);
      memcpy(buffer, curr, llen);
#endif
      buffer[llen] = 0;
      curr = (next != nullptr) ? next + 1 : nullptr;

      sourceLine++;
      ptr = _tcschr(buffer, _T('\r'));
      if (ptr != nullptr)
         *ptr = 0;
      ptr = FindComment(buffer);
      if (ptr != nullptr)
         *ptr = 0;

      Trim(buffer);
      if (buffer[0] == 0)
         continue;

      // Check if it's a section name
      if ((buffer[0] == _T('*')) || (buffer[0] == _T('[')))
      {
         if (buffer[0] == _T('['))
         {
            TCHAR *end = _tcschr(buffer, _T(']'));
            if (end != nullptr)
               *end = 0;
         }

         ConfigEntry *parent = m_root;
         TCHAR *curr = &buffer[1];
         TCHAR *s;
         do
         {
            s = _tcschr(curr, _T('/'));
            if (s != nullptr)
               *s = 0;
            if (*curr == _T('@'))
            {
               // @name indicates no merge entry
               currentSection = new ConfigEntry(curr + 1, parent, this, fileName, sourceLine, 0);
            }
            else
            {
               currentSection = parent->findEntry(curr);
               if (currentSection == nullptr)
               {
                  currentSection = new ConfigEntry(curr, parent, this, fileName, sourceLine, 0);
               }
            }
            curr = s + 1;
            parent = currentSection;
         } while(s != nullptr);
      }
      else
      {
         // Divide on two parts at = sign
         ptr = _tcschr(buffer, _T('='));
         if (ptr == nullptr)
         {
            error(_T("Syntax error in configuration file %s at line %d"), fileName, sourceLine);
            validConfig = false;
            continue;
         }
         *ptr = 0;
         ptr++;
         Trim(buffer);
         Trim(ptr);

         ConfigEntry *entry = currentSection->findEntry(buffer);
         if (entry == nullptr)
         {
            entry = new ConfigEntry(buffer, currentSection, this, fileName, sourceLine, 0);
         }
         entry->addValue(ExpandValue(ptr, false, m_allowMacroExpansion));
      }
   }
   return ignoreErrors || validConfig;
}

/**
 * Max stack depth for XML config
 */
#define MAX_STACK_DEPTH		256

/**
 * State information for XML parser
 */
struct Config_XmlParserState
{
   const char *topLevelTag;
   XML_Parser parser;
   Config *config;
   const TCHAR *file;
   int level;
   ConfigEntry *stack[MAX_STACK_DEPTH];
   StringBuffer charData[MAX_STACK_DEPTH];
   bool trimValue[MAX_STACK_DEPTH];
   bool merge;
};

/**
 * Element start handler for XML parser
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
   Config_XmlParserState *ps = static_cast<Config_XmlParserState*>(userData);

   if (ps->level == 0)
   {
      if (!stricmp(ps->topLevelTag, "*"))
      {
#ifdef UNICODE
         WCHAR wname[MAX_PATH];
         utf8_to_wchar(name, -1, wname, MAX_PATH);
         wname[MAX_PATH - 1] = 0;
         ConfigEntry *e = new ConfigEntry(wname, ps->config->getEntry(_T("/")), ps->config, ps->file, XML_GetCurrentLineNumber(ps->parser), 0);
#else
         ConfigEntry *e = new ConfigEntry(name, ps->config->getEntry(_T("/")), ps->config, ps->file, XML_GetCurrentLineNumber(ps->parser), 0);
#endif
         ps->stack[ps->level] = e;
         ps->charData[ps->level] = _T("");
         ps->trimValue[ps->level] = XMLGetAttrBoolean(attrs, "trim", true);
         ps->level++;
      }
      else if (!stricmp(name, ps->topLevelTag))
      {
         ps->stack[ps->level] = ps->config->getEntry(_T("/"));
         ps->charData[ps->level] = _T("");
         ps->trimValue[ps->level] = XMLGetAttrBoolean(attrs, "trim", true);
         ps->level++;
      }
      else
      {
         ps->level = -1;
      }
   }
   else if (ps->level > 0)
   {
      if (ps->level < MAX_STACK_DEPTH)
      {
         TCHAR entryName[MAX_PATH];

         uint32_t id = XMLGetAttrUInt32(attrs, "id", 0);
#ifdef UNICODE
         if (id != 0)
         {
            WCHAR wname[MAX_PATH];
            utf8_to_wchar(name, -1, wname, MAX_PATH);
            wname[MAX_PATH - 1] = 0;
#ifdef _WIN32
            _snwprintf(entryName, MAX_PATH, L"%s#%u", wname, (unsigned int)id);
#else
            swprintf(entryName, MAX_PATH, L"%S#%u", wname, (unsigned int)id);
#endif
         }
         else
         {
            utf8_to_wchar(name, -1, entryName, MAX_PATH);
            entryName[MAX_PATH - 1] = 0;
         }
#else
         if (id != 0)
            snprintf(entryName, MAX_PATH, "%s#%u", name, (unsigned int) id);
         else
            strlcpy(entryName, name, MAX_PATH);
#endif
         bool merge = XMLGetAttrBoolean(attrs, "merge", ps->merge);
         if (merge)
         {
            if (ps->config->getMergeStrategy() != nullptr)
            {
               ps->stack[ps->level] = ps->config->getMergeStrategy()(ps->stack[ps->level - 1], entryName);
            }
            else
            {
               ps->stack[ps->level] = ps->stack[ps->level - 1]->findEntry(entryName);
            }
         }
         else
         {
            ps->stack[ps->level] = nullptr;
         }
         if (ps->stack[ps->level] == nullptr)
         {
            ConfigEntry *e = new ConfigEntry(entryName, ps->stack[ps->level - 1], ps->config, ps->file, XML_GetCurrentLineNumber(ps->parser), (int)id);
            ps->stack[ps->level] = e;
            // add all attributes to the entry
            for(int i = 0; attrs[i] != nullptr; i += 2)
            {
#ifdef UNICODE
               e->setAttributePreallocated(WideStringFromUTF8String(attrs[i]), WideStringFromUTF8String(attrs[i + 1]));
#else
               e->setAttributePreallocated(MBStringFromUTF8String(attrs[i]), MBStringFromUTF8String(attrs[i + 1]));
#endif
            }
         }
         ps->charData[ps->level] = _T("");
         ps->trimValue[ps->level] = XMLGetAttrBoolean(attrs, "trim", true);
         ps->level++;
      }
      else
      {
         ps->level++;
      }
   }
}

/**
 * Element end handler for XML parser
 */
static void EndElement(void *userData, const char *name)
{
   Config_XmlParserState *ps = static_cast<Config_XmlParserState*>(userData);
   if (ps->level > MAX_STACK_DEPTH)
   {
      ps->level--;
   }
   else if (ps->level > 0)
   {
      ps->level--;
      if (ps->trimValue[ps->level])
         ps->charData[ps->level].trim();
      ps->stack[ps->level]->addValue(ExpandValue(ps->charData[ps->level], true, ps->config->isExpansionAllowed()));
   }
}

/**
 * Data handler for XML parser
 */
static void CharData(void *userData, const XML_Char *s, int len)
{
   Config_XmlParserState *ps = static_cast<Config_XmlParserState*>(userData);
   if ((ps->level > 0) && (ps->level <= MAX_STACK_DEPTH))
      ps->charData[ps->level - 1].appendUtf8String(s, len);
}

/**
 * Load config from XML or INI in memory
 */
bool Config::loadConfigFromMemory(const char *xml, size_t xmlSize, const TCHAR *defaultIniSection, const char *topLevelTag, bool ignoreErrors, bool merge)
{
   bool success;
   int ch;
   int i = 0;
   do
   {
      ch = xml[i];
      i++;
   }
   while(isspace(ch));

   if (ch == '<')
   {
      success = loadXmlConfigFromMemory(xml, xmlSize, nullptr, topLevelTag, merge);
   }
   else
   {
      success = loadIniConfigFromMemory(xml, xmlSize, _T(":memory:"), defaultIniSection, ignoreErrors);
   }

   return success;
}

/**
 * Load config from XML in memory
 */
bool Config::loadXmlConfigFromMemory(const char *xml, size_t xmlSize, const TCHAR *name, const char *topLevelTag, bool merge)
{
   Config_XmlParserState state;

   XML_Parser parser = XML_ParserCreate(nullptr);
   XML_SetUserData(parser, &state);
   XML_SetElementHandler(parser, StartElement, EndElement);
   XML_SetCharacterDataHandler(parser, CharData);

   state.topLevelTag = (topLevelTag != nullptr) ? topLevelTag : "config";
   state.config = this;
   state.level = 0;
   state.parser = parser;
   state.file = (name != nullptr) ? name : _T("<mem>");
   state.merge = merge;

   bool success = (XML_Parse(parser, xml, static_cast<int>(xmlSize), TRUE) != XML_STATUS_ERROR);
   if (!success)
   {
      error(_T("XML parser error in file \"%s\" at line %d (%hs)"),
         ((name != nullptr) && (*name != 0)) ? name : _T(":memory:"), XML_GetCurrentLineNumber(parser), XML_ErrorString(XML_GetErrorCode(parser)));
   }
   XML_ParserFree(parser);
   return success;
}

/**
 * Load config from XML file
 */
bool Config::loadXmlConfig(const TCHAR *file, const char *topLevelTag, bool merge)
{
   bool success;
   size_t size;
   BYTE *xml = LoadFile(file, &size);
   if (xml != nullptr)
   {
      success = loadXmlConfigFromMemory((char *)xml, size, file, topLevelTag, merge);
      MemFree(xml);
   }
   else
   {
      success = false;
   }
   return success;
}

/**
 * Load config file with format auto detection
 */
bool Config::loadConfig(const TCHAR *file, const TCHAR *defaultIniSection, const char *topLevelTag, bool ignoreErrors, bool merge)
{
   NX_STAT_STRUCT fileStats;
   int ret = CALL_STAT_FOLLOW_SYMLINK(file, &fileStats);
   if (ret != 0)
   {
      error(_T("Cannot access file \"%s\" (%s)"), file, _tcserror(errno));
      return false;
   }

   if (!S_ISREG(fileStats.st_mode))
   {
      error(_T("\"%s\" is not a file"), file);
      return false;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == nullptr)
   {
      error(_T("Cannot open file \"%s\" (%s)"), file, _tcserror(errno));
      return false;
   }

   // Skip all space/newline characters
   int ch;
   do
   {
      ch = fgetc(f);
   }
   while(isspace(ch));

   fclose(f);

   // If first non-space character is < assume XML format, otherwise assume INI format
   return (ch == '<') ? loadXmlConfig(file, topLevelTag, merge) : loadIniConfig(file, defaultIniSection, ignoreErrors);
}

/**
 * Load all files in given directory
 */
bool Config::loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection, const char *topLevelTag, bool ignoreErrors, bool merge)
{
   _TDIR *dir;
   struct _tdirent *file;
   TCHAR fileName[MAX_PATH];
   bool success;

   dir = _topendir(path);
   if (dir != nullptr)
   {
      success = true;
      bool trailingSeparator = (path[_tcslen(path) - 1] == FS_PATH_SEPARATOR_CHAR);
      while(true)
      {
         file = _treaddir(dir);
         if (file == nullptr)
            break;

         if (!_tcscmp(file->d_name, _T(".")) || !_tcscmp(file->d_name, _T("..")))
            continue;

         size_t len = _tcslen(path) + _tcslen(file->d_name) + 2;
         if (len > MAX_PATH)
            continue;	// Full file name is too long

         _tcscpy(fileName, path);
         if (!trailingSeparator)
            _tcscat(fileName, FS_PATH_SEPARATOR);
         _tcscat(fileName, file->d_name);

         if (!loadConfig(fileName, defaultIniSection, topLevelTag, ignoreErrors, merge))
         {
            success = false;
         }
      }
      _tclosedir(dir);
   }
   else
   {
      success = false;
   }

   return success;
}

/**
 * Add given configuration subtree into this configuration tree
 */
void Config::addSubTree(const TCHAR *path, const ConfigEntry *root, bool merge)
{
   ConfigEntry *entry = getEntry(path);
   if (entry != nullptr)
      entry->addSubTree(root, merge);
}

/**
 * Print config to log
 */
void Config::print() const
{
   TCHAR prefix[256] = _T("");
   if (m_root != nullptr)
      m_root->print(nullptr, nullptr, 0, prefix);
}

/**
 * Print config to output stream
 */
void Config::print(FILE *file) const
{
   TCHAR prefix[256] = _T("");
   if (m_root != nullptr)
      m_root->print(file, nullptr, 0, prefix);
}

/**
 * Print config to string list
 */
void Config::print(StringList *slist) const
{
   TCHAR prefix[256] = _T("");
   if (m_root != nullptr)
      m_root->print(nullptr, slist, 0, prefix);
}

/**
 * Create XML from config
 */
String Config::createXml() const
{
   StringBuffer xml;
   m_root->createXml(xml);
   return xml;
}
