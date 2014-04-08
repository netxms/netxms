/* 
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2013 Raden Solutions
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

/**
 * Constructor for config entry
 */
ConfigEntry::ConfigEntry(const TCHAR *name, ConfigEntry *parent, const TCHAR *file, int line, int id)
{
   m_name = _tcsdup(CHECK_NULL(name));
   m_first = NULL;
   m_last = NULL;
   m_next = NULL;
   if (parent != NULL)
      parent->addEntry(this);
   m_valueCount = 0;
   m_values = NULL;
   m_file = _tcsdup(CHECK_NULL(file));
   m_line = line;
   m_id = id;
}

/**
 * Destructor for config entry
 */
ConfigEntry::~ConfigEntry()
{
   ConfigEntry *entry, *next;

   for(entry = m_first; entry != NULL; entry = next)
   {
      next = entry->getNext();
      delete entry;
   }
   safe_free(m_name);
   safe_free(m_file);

   for(int i = 0; i < m_valueCount; i++)
      safe_free(m_values[i]);
   safe_free(m_values);
}

/**
 * Set entry's name
 */
void ConfigEntry::setName(const TCHAR *name)
{
   safe_free(m_name);
   m_name = _tcsdup(CHECK_NULL(name));
}

/**
 * Find entry by name
 */
ConfigEntry* ConfigEntry::findEntry(const TCHAR *name)
{
   ConfigEntry *e;

   for(e = m_first; e != NULL; e = e->getNext())
      if (!_tcsicmp(e->getName(), name))
         return e;
   return NULL;
}

/**
 * Create (or find existing) subentry
 */
ConfigEntry* ConfigEntry::createEntry(const TCHAR *name)
{
   ConfigEntry *e;

   for(e = m_first; e != NULL; e = e->getNext())
      if (!_tcsicmp(e->getName(), name))
         return e;

   return new ConfigEntry(name, this, _T("<memory>"), 0, 0);
}

/**
 * Add sub-entry
 */
void ConfigEntry::addEntry(ConfigEntry *entry)
{
   entry->m_parent = this;
   entry->m_next = NULL;
   if (m_last != NULL)
      m_last->m_next = entry;
   m_last = entry;
   if (m_first == NULL)
      m_first = entry;
}

/**
 * Unlink subentry
 */
void ConfigEntry::unlinkEntry(ConfigEntry *entry)
{
   ConfigEntry *curr, *prev;

   for(curr = m_first, prev = NULL; curr != NULL; curr = curr->m_next)
   {
      if (curr == entry)
      {
         if (prev != NULL)
         {
            prev->m_next = curr->m_next;
         }
         else
         {
            m_first = curr->m_next;
         }
         if (m_last == curr)
            m_last = prev;
         curr->m_next = NULL;
         break;
      }
      prev = curr;
   }
}

/**
 * Get all subentries with names matched to mask
 */
ObjectArray<ConfigEntry> *ConfigEntry::getSubEntries(const TCHAR *mask)
{
   ObjectArray<ConfigEntry> *list = new ObjectArray<ConfigEntry>(16, 16, false);
   for(ConfigEntry *e = m_first; e != NULL; e = e->getNext())
      if ((mask == NULL) || MatchString(mask, e->getName(), FALSE))
      {
         list->add(e);
      }
   return list;
}

/**
 * Comparator for ConfigEntryList::sortById()
 */
static int CompareById(const void *p1, const void *p2)
{
   ConfigEntry *e1 = *((ConfigEntry **)p1);
   ConfigEntry *e2 = *((ConfigEntry **)p2);
   return e1->getId() - e2->getId();
}

/**
 * Get all subentries with names matched to mask ordered by id
 */
ObjectArray<ConfigEntry> *ConfigEntry::getOrderedSubEntries(const TCHAR *mask)
{
   ObjectArray<ConfigEntry> *list = getSubEntries(mask);
   list->sort(CompareById);
   return list;
}

/**
 * Get entry value
 */
const TCHAR* ConfigEntry::getValue(int index)
{
   if ((index < 0) || (index >= m_valueCount))
      return NULL;
   return m_values[index];
}

/**
 * Get entry value as integer
 */
INT32 ConfigEntry::getValueInt(int index, INT32 defaultValue)
{
   const TCHAR *value = getValue(index);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

/**
 * Get entry value as unsigned integer
 */
UINT32 ConfigEntry::getValueUInt(int index, UINT32 defaultValue)
{
   const TCHAR *value = getValue(index);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

/**
 * Get entry value as 64 bit integer
 */
INT64 ConfigEntry::getValueInt64(int index, INT64 defaultValue)
{
   const TCHAR *value = getValue(index);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

/**
 * Get entry value as 64 bit unsigned integer
 */
UINT64 ConfigEntry::getValueUInt64(int index, UINT64 defaultValue)
{
   const TCHAR *value = getValue(index);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

/**
 * Get entry value as boolean
 * (consider non-zero numerical value or strings "yes", "true", "on" as true)
 */
bool ConfigEntry::getValueBoolean(int index, bool defaultValue)
{
   const TCHAR *value = getValue(index);
   if (value != NULL)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
         || (_tcstol(value, NULL, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Get entry value as GUID
 */
bool ConfigEntry::getValueUUID(int index, uuid_t uuid)
{
   const TCHAR *value = getValue(index);
   if (value != NULL)
   {
      return uuid_parse(value, uuid) == 0;
   }
   return false;
}

/**
 * Set value (replace all existing values)
 */
void ConfigEntry::setValue(const TCHAR *value)
{
   for(int i = 0; i < m_valueCount; i++)
      safe_free(m_values[i]);
   m_valueCount = 1;
   m_values = (TCHAR **) realloc(m_values, sizeof(TCHAR *));
   m_values[0] = _tcsdup(value);
}

/**
 * Add value
 */
void ConfigEntry::addValue(const TCHAR *value)
{
   m_values = (TCHAR **) realloc(m_values, sizeof(TCHAR *) * (m_valueCount + 1));
   m_values[m_valueCount] = _tcsdup(value);
   m_valueCount++;
}

/**
 * Get summary length of all values as if they was concatenated with separator character
 */
int ConfigEntry::getConcatenatedValuesLength()
{
   int i, len;

   if (m_valueCount < 1)
      return 0;

   for(i = 0, len = 0; i < m_valueCount; i++)
      len += (int) _tcslen(m_values[i]);
   return len + m_valueCount;
}

/**
 * Get value for given subentry
 *
 * @param name sub-entry name
 * @param index sub-entry index (0 if omited)
 * @param defaultValue value to be returned if requested sub-entry is missing (NULL if omited)
 * @return sub-entry value or default value if requested sub-entry does not exist
 */
const TCHAR* ConfigEntry::getSubEntryValue(const TCHAR *name, int index, const TCHAR *defaultValue)
{
   ConfigEntry *e = findEntry(name);
   if (e == NULL)
      return defaultValue;
   const TCHAR *value = e->getValue(index);
   return (value != NULL) ? value : defaultValue;
}

INT32 ConfigEntry::getSubEntryValueInt(const TCHAR *name, int index, INT32 defaultValue)
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

UINT32 ConfigEntry::getSubEntryValueUInt(const TCHAR *name, int index, UINT32 defaultValue)
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

INT64 ConfigEntry::getSubEntryValueInt64(const TCHAR *name, int index, INT64 defaultValue)
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

UINT64 ConfigEntry::getSubEntryValueUInt64(const TCHAR *name, int index, UINT64 defaultValue)
{
   const TCHAR *value = getSubEntryValue(name, index);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

bool ConfigEntry::getSubEntryValueBoolean(const TCHAR *name, int index, bool defaultValue)
{
   const TCHAR *value = getSubEntryValue(name, index);
   if (value != NULL)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
         || (_tcstol(value, NULL, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

bool ConfigEntry::getSubEntryValueUUID(const TCHAR *name, uuid_t uuid, int index)
{
   const TCHAR *value = getSubEntryValue(name, index);
   if (value != NULL)
   {
      return uuid_parse(value, uuid) == 0;
   }
   else
   {
      return false;
   }
}

/**
 * Print config entry
 */
void ConfigEntry::print(FILE *file, int level)
{
   _tprintf(_T("%*s%s\n"), level * 4, _T(""), m_name);
   for(ConfigEntry *e = m_first; e != NULL; e = e->getNext())
      e->print(file, level + 1);

   for(int i = 0, len = 0; i < m_valueCount; i++)
      _tprintf(_T("%*svalue: %s\n"), level * 4 + 2, _T(""), m_values[i]);
}

/**
 * Create XML element(s) from config entry
 */
void ConfigEntry::createXml(String &xml, int level)
{
   TCHAR *name = _tcsdup(m_name);
   TCHAR *ptr = _tcschr(name, _T('#'));
   if (ptr != NULL)
      *ptr = 0;

   if (m_id == 0)
      xml.addFormattedString(_T("%*s<%s"), level * 4, _T(""), name);
   else
      xml.addFormattedString(_T("%*s<%s id=\"%d\""), level * 4, _T(""), name, m_id);
   for(UINT32 j = 0; j < m_attributes.getSize(); j++)
   {
      if (_tcscmp(m_attributes.getKeyByIndex(j), _T("id")))
         xml.addFormattedString(_T(" %s=\"%s\""), m_attributes.getKeyByIndex(j), m_attributes.getValueByIndex(j));
   }
   xml += _T(">");

   if (m_first != NULL)
   {
      xml += _T("\n");
      for(ConfigEntry *e = m_first; e != NULL; e = e->getNext())
         e->createXml(xml, level + 1);
      xml.addFormattedString(_T("%*s"), level * 4, _T(""));
   }

   if (m_valueCount > 0)
      xml.addDynamicString(EscapeStringForXML(m_values[0], -1));
   xml.addFormattedString(_T("</%s>\n"), name);

   for(int i = 1, len = 0; i < m_valueCount; i++)
   {
      if (m_id == 0)
         xml.addFormattedString(_T("%*s<%s>"), level * 4, _T(""), name);
      else
         xml.addFormattedString(_T("%*s<%s id=\"%d\">"), level * 4, _T(""), name, m_id);
      xml.addDynamicString(EscapeStringForXML(m_values[i], -1));
      xml.addFormattedString(_T("</%s>\n"), name);
   }

   free(name);
}

/**
 * Constructor for config
 */
Config::Config()
{
   m_root = new ConfigEntry(_T("[root]"), NULL, NULL, 0, 0);
   m_errorCount = 0;
   m_mutex = MutexCreate();
}

/**
 * Destructor
 */
Config::~Config()
{
   delete m_root;
   MutexDestroy(m_mutex);
}

/**
 * Default error handler
 */
void Config::onError(const TCHAR *errorMessage)
{
   _ftprintf(stderr, _T("%s\n"), errorMessage);
}

/**
 * Report error
 */
void Config::error(const TCHAR *format, ...)
{
   va_list args;
   TCHAR buffer[4096];

   m_errorCount++;
   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);
   onError(buffer);
}

/**
 * Parse configuration template (emulation of old NxLoadConfig() API)
 */
bool Config::parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate)
{
   TCHAR name[MAX_PATH], *curr, *eptr;
   int i, j, pos, initialErrorCount = m_errorCount;
   ConfigEntry *entry;

   name[0] = _T('/');
   nx_strncpy(&name[1], section, MAX_PATH - 2);
   _tcscat(name, _T("/"));
   pos = (int) _tcslen(name);

   for(i = 0; cfgTemplate[i].type != CT_END_OF_LIST; i++)
   {
      nx_strncpy(&name[pos], cfgTemplate[i].token, MAX_PATH - pos);
      entry = getEntry(name);
      if (entry != NULL)
      {
         const TCHAR *value = CHECK_NULL(entry->getValue(entry->getValueCount() - 1));
         switch (cfgTemplate[i].type)
         {
            case CT_LONG:
               if ((cfgTemplate[i].overrideIndicator != NULL) &&
                   (*((INT32 *)cfgTemplate[i].overrideIndicator) != NXCONFIG_UNINITIALIZED_VALUE))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               *((INT32 *)cfgTemplate[i].buffer) = _tcstol(value, &eptr, 0);
               if (*eptr != 0)
               {
                  error(_T("Invalid number '%s' in configuration file %s at line %d\n"), value, entry->getFile(), entry->getLine());
               }
               break;
            case CT_WORD:
               if ((cfgTemplate[i].overrideIndicator != NULL) &&
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
            case CT_BOOLEAN:
               if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
                  || !_tcsicmp(value, _T("1")))
               {
                  *((UINT32 *)cfgTemplate[i].buffer) |= cfgTemplate[i].bufferSize;
               }
               else
               {
                  *((UINT32 *)cfgTemplate[i].buffer) &= ~(cfgTemplate[i].bufferSize);
               }
               break;
            case CT_STRING:
               if ((cfgTemplate[i].overrideIndicator != NULL) &&
                   (*((TCHAR *)cfgTemplate[i].overrideIndicator) != 0))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
               nx_strncpy((TCHAR *)cfgTemplate[i].buffer, value, cfgTemplate[i].bufferSize);
               break;
            case CT_MB_STRING:
               if ((cfgTemplate[i].overrideIndicator != NULL) &&
                   (*((char *)cfgTemplate[i].overrideIndicator) != 0))
               {
                  break;   // this parameter was already initialized, and override from config is forbidden
               }
#ifdef UNICODE
               memset(cfgTemplate[i].buffer, 0, cfgTemplate[i].bufferSize);
               WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, value, -1, (char *)cfgTemplate[i].buffer, cfgTemplate[i].bufferSize - 1, NULL, NULL);
#else
               nx_strncpy((TCHAR *)cfgTemplate[i].buffer, value, cfgTemplate[i].bufferSize);
#endif
               break;
            case CT_STRING_LIST:
               *((TCHAR **)cfgTemplate[i].buffer) = (TCHAR *)malloc(sizeof(TCHAR) * (entry->getConcatenatedValuesLength() + 1));
               for(j = 0, curr = *((TCHAR **) cfgTemplate[i].buffer); j < entry->getValueCount(); j++)
               {
                  _tcscpy(curr, entry->getValue(j));
                  curr += _tcslen(curr);
                  *curr = cfgTemplate[i].separator;
                  curr++;
               }
               *curr = 0;
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
const TCHAR *Config::getValue(const TCHAR *path, const TCHAR *defaultValue)
{
   const TCHAR *value;
   ConfigEntry *entry = getEntry(path);
   if (entry != NULL)
   {
      value = entry->getValue();
      if (value == NULL)
         value = defaultValue;
   }
   else
   {
      value = defaultValue;
   }
   return value;
}

INT32 Config::getValueInt(const TCHAR *path, INT32 defaultValue)
{
   const TCHAR *value = getValue(path);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

UINT32 Config::getValueUInt(const TCHAR *path, UINT32 defaultValue)
{
   const TCHAR *value = getValue(path);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

INT64 Config::getValueInt64(const TCHAR *path, INT64 defaultValue)
{
   const TCHAR *value = getValue(path);
   return (value != NULL) ? _tcstol(value, NULL, 0) : defaultValue;
}

UINT64 Config::getValueUInt64(const TCHAR *path, UINT64 defaultValue)
{
   const TCHAR *value = getValue(path);
   return (value != NULL) ? _tcstoul(value, NULL, 0) : defaultValue;
}

bool Config::getValueBoolean(const TCHAR *path, bool defaultValue)
{
   const TCHAR *value = getValue(path);
   if (value != NULL)
   {
      return !_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
         || (_tcstol(value, NULL, 0) != 0);
   }
   else
   {
      return defaultValue;
   }
}

/**
 * Get value at given path as UUID
 */
bool Config::getValueUUID(const TCHAR *path, uuid_t uuid)
{
   const TCHAR *value = getValue(path);
   if (value != NULL)
   {
      return uuid_parse(value, uuid) == 0;
   }
   else
   {
      return false;
   }
}

/**
 * Get subentries
 */
ObjectArray<ConfigEntry> *Config::getSubEntries(const TCHAR *path, const TCHAR *mask)
{
   ConfigEntry *entry = getEntry(path);
   return (entry != NULL) ? entry->getSubEntries(mask) : NULL;
}

/**
 * Get subentries ordered by id
 */
ObjectArray<ConfigEntry> *Config::getOrderedSubEntries(const TCHAR *path, const TCHAR *mask)
{
   ConfigEntry *entry = getEntry(path);
   return (entry != NULL) ? entry->getOrderedSubEntries(mask) : NULL;
}

/**
 * Get entry
 */
ConfigEntry *Config::getEntry(const TCHAR *path)
{
   const TCHAR *curr, *end;
   TCHAR name[256];
   ConfigEntry *entry = m_root;

   if ((path == NULL) || (*path != _T('/')))
      return NULL;

   if (!_tcscmp(path, _T("/")))
      return m_root;

   curr = path + 1;
   while(entry != NULL)
   {
      end = _tcschr(curr, _T('/'));
      if (end != NULL)
      {
         int len = min((int )(end - curr), 255);
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
   return NULL;
}

/*
 * Create entry if does not exist, or return existing
 * Will return NULL on error
 */

ConfigEntry *Config::createEntry(const TCHAR *path)
{
   const TCHAR *curr, *end;
   TCHAR name[256];
   ConfigEntry *entry, *parent;

   if ((path == NULL) || (*path != _T('/')))
      return NULL;

   if (!_tcscmp(path, _T("/")))
      return m_root;

   curr = path + 1;
   parent = m_root;
   do
   {
      end = _tcschr(curr, _T('/'));
      if (end != NULL)
      {
         int len = min((int )(end - curr), 255);
         _tcsncpy(name, curr, len);
         name[len] = 0;
         entry = parent->findEntry(name);
         curr = end + 1;
         if (entry == NULL)
            entry = new ConfigEntry(name, parent, _T("<memory>"), 0, 0);
         parent = entry;
      }
      else
      {
         entry = parent->findEntry(curr);
         if (entry == NULL)
            entry = new ConfigEntry(curr, parent, _T("<memory>"), 0, 0);
      }
   }
   while(end != NULL);
   return entry;
}

/**
 * Delete entry
 */
void Config::deleteEntry(const TCHAR *path)
{
   ConfigEntry *entry = getEntry(path);
   if (entry == NULL)
      return;

   ConfigEntry *parent = entry->getParent();
   if (parent == NULL)	// root entry
      return;

   parent->unlinkEntry(entry);
   delete entry;
}

/*
 * Set value
 * Returns false on error (usually caused by incorrect path)
 */

bool Config::setValue(const TCHAR *path, const TCHAR *value)
{
   ConfigEntry *entry = createEntry(path);
   if (entry == NULL)
      return false;
   entry->setValue(value);
   return true;
}

bool Config::setValue(const TCHAR *path, INT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%ld"), (long) value);
   return setValue(path, buffer);
}

bool Config::setValue(const TCHAR *path, UINT32 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%lu"), (unsigned long) value);
   return setValue(path, buffer);
}

bool Config::setValue(const TCHAR *path, INT64 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, INT64_FMT, value);
   return setValue(path, buffer);
}

bool Config::setValue(const TCHAR *path, UINT64 value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, UINT64_FMT, value);
   return setValue(path, buffer);
}

bool Config::setValue(const TCHAR *path, double value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%f"), value);
   return setValue(path, buffer);
}

bool Config::setValue(const TCHAR *path, uuid_t value)
{
   TCHAR buffer[64];
   uuid_to_string(value, buffer);
   return setValue(path, buffer);
}

/**
 * Find comment start in INI style config line
 * Comment starts with # character, characters within double quotes ignored
 */
static TCHAR* FindComment(TCHAR *str)
{
   TCHAR *curr;
   bool quotes;

   for(curr = str, quotes = false; *curr != 0; curr++)
   {
      if (*curr == _T('"'))
      {
         quotes = !quotes;
      }
      else if ((*curr == _T('#')) && !quotes)
      {
         return curr;
      }
   }
   return NULL;
}

/**
 * Load INI-style config
 */
bool Config::loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection, bool ignoreErrors)
{
   FILE *cfg;
   TCHAR buffer[4096], *ptr;
   ConfigEntry *currentSection;
   int sourceLine = 0;
   bool validConfig = true;

   cfg = _tfopen(file, _T("r"));
   if (cfg == NULL)
   {
      error(_T("Cannot open file %s"), file);
      return false;
   }

   currentSection = m_root->findEntry(defaultIniSection);
   if (currentSection == NULL)
   {
      currentSection = new ConfigEntry(defaultIniSection, m_root, file, 0, 0);
   }

   while(!feof(cfg))
   {
      // Read line from file
      buffer[0] = 0;
      if (_fgetts(buffer, 4095, cfg) == NULL)
         break;	// EOF or error
      sourceLine++;
      ptr = _tcschr(buffer, _T('\n'));
      if (ptr != NULL)
      {
         if (ptr != buffer)
         {
            if (*(ptr - 1) == '\r')
               ptr--;
         }
         *ptr = 0;
      }
      ptr = FindComment(buffer);
      if (ptr != NULL)
         *ptr = 0;

      StrStrip(buffer);
      if (buffer[0] == 0)
         continue;

      // Check if it's a section name
      if ((buffer[0] == _T('*')) || (buffer[0] == _T('[')))
      {
         if (buffer[0] == _T('['))
         {
            TCHAR *end = _tcschr(buffer, _T(']'));
            if (end != NULL)
               *end = 0;
         }
         currentSection = m_root->findEntry(&buffer[1]);
         if (currentSection == NULL)
         {
            currentSection = new ConfigEntry(&buffer[1], m_root, file, sourceLine, 0);
         }
      }
      else
      {
         // Divide on two parts at = sign
         ptr = _tcschr(buffer, _T('='));
         if (ptr == NULL)
         {
            error(_T("Syntax error in configuration file %s at line %d"), file, sourceLine);
            validConfig = false;
            continue;
         }
         *ptr = 0;
         ptr++;
         StrStrip(buffer);
         StrStrip(ptr);

         ConfigEntry *entry = currentSection->findEntry(buffer);
         if (entry == NULL)
         {
            entry = new ConfigEntry(buffer, currentSection, file, sourceLine, 0);
         }
         entry->addValue(ptr);
      }
   }
   fclose(cfg);
   return ignoreErrors || validConfig;
}

/**
 * Max stack depth for XML config
 */
#define MAX_STACK_DEPTH		256

/**
 * State information for XML parser
 */
typedef struct
{
   const char *topLevelTag;
   XML_Parser parser;
   Config *config;
   const TCHAR *file;
   int level;
   ConfigEntry *stack[MAX_STACK_DEPTH];
   String charData[MAX_STACK_DEPTH];
   bool trimValue[MAX_STACK_DEPTH];
} XML_PARSER_STATE;

/**
 * Element start handler for XML parser
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *) userData;

   if (ps->level == 0)
   {
      if (!stricmp(name, ps->topLevelTag))
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

         UINT32 id = XMLGetAttrUINT32(attrs, "id", 0);
#ifdef UNICODE
         if (id != 0)
         {
            WCHAR wname[MAX_PATH];

            MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_PATH);
            wname[MAX_PATH - 1] = 0;
#ifdef _WIN32
            _snwprintf(entryName, MAX_PATH, L"%s#%u", wname, (unsigned int)id);
#else
            swprintf(entryName, MAX_PATH, L"%S#%u", wname, (unsigned int)id);
#endif
         }
         else
         {
            MultiByteToWideChar(CP_UTF8, 0, name, -1, entryName, MAX_PATH);
            entryName[MAX_PATH - 1] = 0;
         }
#else
         if (id != 0)
            snprintf(entryName, MAX_PATH, "%s#%u", name, (unsigned int) id);
         else
            nx_strncpy(entryName, name, MAX_PATH);
#endif
         // Only do merge on top level by default, otherwise add sub-entry with same name
         bool merge = XMLGetAttrBoolean(attrs, "merge", (ps->level == 1));
         ps->stack[ps->level] = merge ? ps->stack[ps->level - 1]->findEntry(entryName) : NULL;
         if (ps->stack[ps->level] == NULL)
         {
            ConfigEntry *e = new ConfigEntry(entryName, ps->stack[ps->level - 1], ps->file, XML_GetCurrentLineNumber(ps->parser), (int)id);
            ps->stack[ps->level] = e;
            // add all attributes to the entry
            for(int i = 0; attrs[i] != NULL; i += 2)
            {
#ifdef UNICODE
               e->setAttributePreallocated(WideStringFromMBString(attrs[i]), WideStringFromMBString(attrs[i + 1]));
#else
               e->setAttribute(attrs[i], attrs[i + 1]);
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
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

   if (ps->level > MAX_STACK_DEPTH)
   {
      ps->level--;
   }
   else if (ps->level > 0)
   {
      ps->level--;
      if (ps->trimValue[ps->level])
         ps->charData[ps->level].trim();
      ps->stack[ps->level]->addValue(ps->charData[ps->level]);
   }
}

/**
 * Data handler for XML parser
 */
static void CharData(void *userData, const XML_Char *s, int len)
{
   XML_PARSER_STATE *ps = (XML_PARSER_STATE *) userData;

   if ((ps->level > 0) && (ps->level <= MAX_STACK_DEPTH))
      ps->charData[ps->level - 1].addMultiByteString(s, len, CP_UTF8);
}

/**
 * Load config from XML in memory
 */
bool Config::loadXmlConfigFromMemory(const char *xml, int xmlSize, const TCHAR *name, const char *topLevelTag)
{
   XML_PARSER_STATE state;

   XML_Parser parser = XML_ParserCreate(NULL);
   XML_SetUserData(parser, &state);
   XML_SetElementHandler(parser, StartElement, EndElement);
   XML_SetCharacterDataHandler(parser, CharData);

   state.topLevelTag = (topLevelTag != NULL) ? topLevelTag : "config";
   state.config = this;
   state.level = 0;
   state.parser = parser;
   state.file = (name != NULL) ? name : _T("<mem>");

   bool success = (XML_Parse(parser, xml, xmlSize, TRUE) != XML_STATUS_ERROR);
   if (!success)
   {
      error(_T("%hs at line %d"), XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
   }
   XML_ParserFree(parser);
   return success;
}

/**
 * Load config from XML file
 */
bool Config::loadXmlConfig(const TCHAR *file, const char *topLevelTag)
{
   BYTE *xml;
   UINT32 size;
   bool success;

   xml = LoadFile(file, &size);
   if (xml != NULL)
   {
      success = loadXmlConfigFromMemory((char *) xml, (int) size, file, topLevelTag);
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
bool Config::loadConfig(const TCHAR *file, const TCHAR *defaultIniSection, bool ignoreErrors)
{
   NX_STAT_STRUCT fileStats;
   int ret = CALL_STAT(file, &fileStats);
   if (ret != 0)
   {
      error(_T("Could not process \"%s\"!"), file);
      return false;
   }

   if (!S_ISREG(fileStats.st_mode))
   {
      error(_T("\"%s\" is not a file!"), file);
      return false;
   }

   FILE *f = _tfopen(file, _T("r"));
   if (f == NULL)
   {
      error(_T("Cannot open file %s"), file);
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
   return (ch == '<') ? loadXmlConfig(file) : loadIniConfig(file, defaultIniSection, ignoreErrors);
}

/*
 * Load all files in given directory
 */

bool Config::loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection, bool ignoreErrors)
{
   _TDIR *dir;
   struct _tdirent *file;
   TCHAR fileName[MAX_PATH];
   bool success;

   dir = _topendir(path);
   if (dir != NULL)
   {
      success = true;
      while(1)
      {
         file = _treaddir(dir);
         if (file == NULL)
            break;

         if (!_tcscmp(file->d_name, _T(".")) || !_tcscmp(file->d_name, _T("..")))
            continue;

         size_t len = _tcslen(path) + _tcslen(file->d_name) + 2;
         if (len > MAX_PATH)
            continue;	// Full file name is too long

         _tcscpy(fileName, path);
         _tcscat(fileName, FS_PATH_SEPARATOR);
         _tcscat(fileName, file->d_name);

         if (!loadConfig(fileName, defaultIniSection, ignoreErrors))
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

/*
 * Print config to output stream
 */

void Config::print(FILE *file)
{
   if (m_root != NULL)
      m_root->print(file, 0);
}

/**
 * Create XML from config
 */
String Config::createXml()
{
   String xml;
   m_root->createXml(xml);
   return xml;
}
