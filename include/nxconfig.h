/*
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: nxconfig.h
**
**/

#ifndef _nxconfig_h_
#define _nxconfig_h_

#include <nms_common.h>
#include <nms_util.h>
#include <uuid.h>

class Config;

/**
 * Config entry
 */
class LIBNETXMS_EXPORTABLE ConfigEntry
{
private:
	TCHAR *m_name;
	ConfigEntry *m_parent;
	ConfigEntry *m_next;
	ConfigEntry *m_first;
   ConfigEntry *m_last;
	StringList m_values;
   StringMap m_attributes;
   TCHAR *m_file;
	int m_line;
	int m_id;
   const Config *m_owner;

	void addEntry(ConfigEntry *entry);
	void linkEntry(ConfigEntry *entry) { entry->m_next = m_next; m_next = entry; }

   ConfigEntry(const ConfigEntry *src, const Config *owner);

public:
	ConfigEntry(const TCHAR *name, ConfigEntry *parent, const Config *owner, const TCHAR *file, int line, int id);
	ConfigEntry(const ConfigEntry& src) = delete;
	~ConfigEntry();

	ConfigEntry *getNext() const { return m_next; }
	ConfigEntry *getParent() const { return m_parent; }

	const TCHAR *getName() const { return m_name; }
	int getId() const { return m_id; }
	int getValueCount() const { return m_values.size(); }

   String getConcatenatedValues(const TCHAR *separator = nullptr) const;
	int getConcatenatedValuesLength() const;

   const TCHAR *getValue(int index = 0) const { return m_values.get(index); }
	int32_t getValueAsInt(int index = 0, int32_t defaultValue = 0) const;
	uint32_t getValueAsUInt(int index = 0, uint32_t defaultValue = 0) const;
	int64_t getValueAsInt64(int index = 0, int64_t defaultValue = 0) const;
	uint64_t getValueAsUInt64(int index = 0, uint64_t defaultValue = 0) const;
	bool getValueAsBoolean(int index = 0, bool defaultValue = false) const;
	uuid getValueAsUUID(int index) const;

   void addValue(const TCHAR *value) { m_values.add(value); }
   void addValuePreallocated(TCHAR *value) { m_values.addPreallocated(value); }
	void setValue(const TCHAR*value);

	const TCHAR *getSubEntryValue(const TCHAR *name, int index = 0, const TCHAR *defaultValue = nullptr) const;
	int32_t getSubEntryValueAsInt(const TCHAR *name, int index = 0, int32_t defaultValue = 0) const;
	uint32_t getSubEntryValueAsUInt(const TCHAR *name, int index = 0, uint32_t defaultValue = 0) const;
	int64_t getSubEntryValueAsInt64(const TCHAR *name, int index = 0, int64_t defaultValue = 0) const;
	uint64_t getSubEntryValueAsUInt64(const TCHAR *name, int index = 0, uint64_t defaultValue = 0) const;
	bool getSubEntryValueAsBoolean(const TCHAR *name, int index = 0, bool defaultValue = false) const;
	uuid getSubEntryValueAsUUID(const TCHAR *name, int index = 0) const;

   const TCHAR *getAttribute(const TCHAR *name)  const { return m_attributes.get(name); }
	int32_t getAttributeAsInt(const TCHAR *name, int32_t defaultValue = 0) const;
	uint32_t getAttributeAsUInt(const TCHAR *name, uint32_t defaultValue = 0) const;
	int64_t getAttributeAsInt64(const TCHAR *name, int64_t defaultValue = 0) const;
	uint64_t getAttributeAsUInt64(const TCHAR *name, uint64_t defaultValue = 0) const;
	bool getAttributeAsBoolean(const TCHAR *name, bool defaultValue = false) const;

   void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); }
   void setAttributePreallocated(TCHAR *name, TCHAR *value) { m_attributes.setPreallocated(name, value); }
   void setAttribute(const TCHAR *name, int32_t value);
   void setAttribute(const TCHAR *name, uint32_t value);
   void setAttribute(const TCHAR *name, int64_t value);
   void setAttribute(const TCHAR *name, uint64_t value);
   void setAttribute(const TCHAR *name, bool value);

	const TCHAR *getFile() const { return m_file; }
	int getLine() const { return m_line; }

	void setName(const TCHAR *name);

	ConfigEntry *findOrCreateEntry(const TCHAR *name);
	ConfigEntry *findEntry(const TCHAR *name) const;
   unique_ptr<ObjectArray<ConfigEntry>> getSubEntries(const TCHAR *mask = nullptr) const;
   unique_ptr<ObjectArray<ConfigEntry>> getOrderedSubEntries(const TCHAR *mask = nullptr) const;
   void unlinkEntry(ConfigEntry *entry);

   void addSubTree(const ConfigEntry *root, bool merge);

	void print(FILE *file, StringList *slist, int level, TCHAR *prefix) const;
	void createXml(StringBuffer &xml, int level = 0) const;
};

/**
 * Merge strategy
 */
typedef ConfigEntry *(*ConfigMergeStrategy)(ConfigEntry *parent, const TCHAR *name);

/**
 * Hierarchical config
 */
class LIBNETXMS_EXPORTABLE Config
{
private:
	ConfigEntry *m_root;
	int m_errorCount;
	Mutex m_mutex;
   StringMap m_aliases;
   bool m_allowMacroExpansion;
   ConfigMergeStrategy m_mergeStrategy;
   bool m_logErrors;

protected:
	virtual void onError(const TCHAR *errorMessage);

	void error(const TCHAR *format, ...);

public:
	Config(bool allowMacroExpansion = true);
	Config(const Config& src) = delete;
   virtual ~Config();

	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

	void setTopLevelTag(const TCHAR *topLevelTag) { m_root->setName(topLevelTag); }

   void setMergeStrategy(ConfigMergeStrategy s) { m_mergeStrategy = s; }
   ConfigMergeStrategy getMergeStrategy() const { return m_mergeStrategy; }

	bool loadXmlConfig(const TCHAR *file, const char *topLevelTag = nullptr, bool merge = true);
	bool loadXmlConfigFromMemory(const char *xml, size_t xmlSize, const TCHAR *name = nullptr, const char *topLevelTag = nullptr, bool merge = true);
	bool loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection, bool ignoreErrors = true);
   bool loadIniConfigFromMemory(const char *content, size_t length, const TCHAR *fileName, const TCHAR *defaultIniSection, bool ignoreErrors = true);
	bool loadConfig(const TCHAR *file, const TCHAR *defaultIniSection, const char *topLevelTag = nullptr, bool ignoreErrors = true, bool merge = true);
	bool loadConfigFromMemory(const char *xml, size_t xmlSize, const TCHAR *defaultIniSection, const char *topLevelTag, bool ignoreErrors, bool merge);

	bool loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection, const char *topLevelTag = nullptr, bool ignoreErrors = true, bool merge = true);

   void addSubTree(const TCHAR *path, const ConfigEntry *root, bool merge = true);

   ConfigEntry *getOrCreateEntry(const TCHAR *path);
	void deleteEntry(const TCHAR *path);

	ConfigEntry *getEntry(const TCHAR *path) const;
	const TCHAR *getValue(const TCHAR *path, const TCHAR *defaultValue = nullptr, int index = 0) const;
   const TCHAR *getFirstNonEmptyValue(const TCHAR *path) const;
   int32_t getValueAsInt(const TCHAR *path, int32_t defaultValue, int index = 0) const;
	uint32_t getValueAsUInt(const TCHAR *path, uint32_t defaultValue, int index = 0) const;
	int64_t getValueAsInt64(const TCHAR *path, int64_t defaultValue, int index = 0) const;
	uint64_t getValueAsUInt64(const TCHAR *path, uint64_t defaultValue, int index = 0) const;
	bool getValueAsBoolean(const TCHAR *path, bool defaultValue, int index = 0) const;
	uuid getValueAsUUID(const TCHAR *path, int index = 0) const;
   unique_ptr<ObjectArray<ConfigEntry>> getSubEntries(const TCHAR *path, const TCHAR *mask = nullptr) const;
   unique_ptr<ObjectArray<ConfigEntry>> getOrderedSubEntries(const TCHAR *path, const TCHAR *mask = nullptr) const;

	bool setValue(const TCHAR *path, const TCHAR *value);
	bool setValue(const TCHAR *path, int32_t value);
	bool setValue(const TCHAR *path, uint32_t value);
	bool setValue(const TCHAR *path, int64_t value);
	bool setValue(const TCHAR *path, uint64_t value);
	bool setValue(const TCHAR *path, double value);
	bool setValue(const TCHAR *path, const uuid& value);

	void setLogErrors(bool logErrors) { m_logErrors = logErrors; }

	bool parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate);

	int getErrorCount() const { return m_errorCount; }

   void print() const;
	void print(FILE *file) const;
   void print(StringList *slist) const;
	String createXml() const;

   void setAlias(const TCHAR *alias, const TCHAR *value) { if (alias != nullptr) m_aliases.set(alias, value); else m_aliases.remove(alias); }
   const TCHAR *getAlias(const TCHAR *alias) const { return m_aliases.get(alias); }

   bool isExpansionAllowed() const { return m_allowMacroExpansion; }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE shared_ptr<Config>;
#endif

#endif
