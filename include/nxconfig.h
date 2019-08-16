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
   DISABLE_COPY_CTOR(ConfigEntry)

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
	~ConfigEntry();

	ConfigEntry *getNext() const { return m_next; }
	ConfigEntry *getParent() const { return m_parent; }

	const TCHAR *getName() const { return m_name; }
	int getId() const { return m_id; }
	int getValueCount() const { return m_values.size(); }
	int getConcatenatedValuesLength();

   const TCHAR *getValue(int index = 0) const { return m_values.get(index); }
	INT32 getValueAsInt(int index = 0, INT32 defaultValue = 0);
	UINT32 getValueAsUInt(int index = 0, UINT32 defaultValue = 0);
	INT64 getValueAsInt64(int index = 0, INT64 defaultValue = 0);
	UINT64 getValueAsUInt64(int index = 0, UINT64 defaultValue = 0);
	bool getValueAsBoolean(int index = 0, bool defaultValue = false);
	uuid getValueAsUUID(int index);

   void addValue(const TCHAR *value) { m_values.add(value); }
   void addValuePreallocated(TCHAR *value) { m_values.addPreallocated(value); }
	void setValue(const TCHAR*value);

	const TCHAR *getSubEntryValue(const TCHAR *name, int index = 0, const TCHAR *defaultValue = NULL) const;
	INT32 getSubEntryValueAsInt(const TCHAR *name, int index = 0, INT32 defaultValue = 0) const;
	UINT32 getSubEntryValueAsUInt(const TCHAR *name, int index = 0, UINT32 defaultValue = 0) const;
	INT64 getSubEntryValueAsInt64(const TCHAR *name, int index = 0, INT64 defaultValue = 0) const;
	UINT64 getSubEntryValueAsUInt64(const TCHAR *name, int index = 0, UINT64 defaultValue = 0) const;
	bool getSubEntryValueAsBoolean(const TCHAR *name, int index = 0, bool defaultValue = false) const;
	uuid getSubEntryValueAsUUID(const TCHAR *name, int index = 0) const;

   const TCHAR *getAttribute(const TCHAR *name)  const { return m_attributes.get(name); }
	INT32 getAttributeAsInt(const TCHAR *name, INT32 defaultValue = 0) const;
	UINT32 getAttributeAsUInt(const TCHAR *name, UINT32 defaultValue = 0) const;
	INT64 getAttributeAsInt64(const TCHAR *name, INT64 defaultValue = 0) const;
	UINT64 getAttributeAsUInt64(const TCHAR *name, UINT64 defaultValue = 0) const;
	bool getAttributeAsBoolean(const TCHAR *name, bool defaultValue = false) const;

   void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); }
   void setAttributePreallocated(TCHAR *name, TCHAR *value) { m_attributes.setPreallocated(name, value); }
   void setAttribute(const TCHAR *name, INT32 value);
   void setAttribute(const TCHAR *name, UINT32 value);
   void setAttribute(const TCHAR *name, INT64 value);
   void setAttribute(const TCHAR *name, UINT64 value);
   void setAttribute(const TCHAR *name, bool value);

	const TCHAR *getFile() const { return m_file; }
	int getLine() const { return m_line; }

	void setName(const TCHAR *name);

	ConfigEntry *createEntry(const TCHAR *name);
	ConfigEntry *findEntry(const TCHAR *name) const;
	ObjectArray<ConfigEntry> *getSubEntries(const TCHAR *mask) const;
	ObjectArray<ConfigEntry> *getOrderedSubEntries(const TCHAR *mask) const;
	void unlinkEntry(ConfigEntry *entry);

   void addSubTree(const ConfigEntry *root, bool merge);

	void print(FILE *file, int level, TCHAR *prefix);
	void createXml(String &xml, int level = 0);
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
   DISABLE_COPY_CTOR(Config)

private:
	ConfigEntry *m_root;
	int m_errorCount;
	MUTEX m_mutex;
   StringMap m_aliases;
   bool m_allowMacroExpansion;
   ConfigMergeStrategy m_mergeStrategy;

protected:
	virtual void onError(const TCHAR *errorMessage);

	void error(const TCHAR *format, ...);
	ConfigEntry *createEntry(const TCHAR *path);


public:
	Config(bool allowMacroExpansion = true);
   virtual ~Config();

	void lock() { MutexLock(m_mutex); }
	void unlock() { MutexUnlock(m_mutex); }

	void setTopLevelTag(const TCHAR *topLevelTag) { m_root->setName(topLevelTag); }
   
   void setMergeStrategy(ConfigMergeStrategy s) { m_mergeStrategy = s; }
   ConfigMergeStrategy getMergeStrategy() const { return m_mergeStrategy; }

	bool loadXmlConfig(const TCHAR *file, const char *topLevelTag = NULL, bool merge = true);
	bool loadXmlConfigFromMemory(const char *xml, int xmlSize, const TCHAR *name = NULL, const char *topLevelTag = NULL, bool merge = true);
	bool loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection, bool ignoreErrors = true);
   bool loadIniConfigFromMemory(const char *content, size_t length, const TCHAR *fileName, const TCHAR *defaultIniSection, bool ignoreErrors = true);
	bool loadConfig(const TCHAR *file, const TCHAR *defaultIniSection, const char *topLevelTag = NULL, bool ignoreErrors = true, bool merge = true);
	bool loadConfigFromMemory(const char *xml, int xmlSize, const TCHAR *defaultIniSection, const char *topLevelTag, bool ignoreErrors, bool merge);

	bool loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection, const char *topLevelTag = NULL, bool ignoreErrors = true, bool merge = true);

   void addSubTree(const TCHAR *path, const ConfigEntry *root, bool merge = true);

	void deleteEntry(const TCHAR *path);

	ConfigEntry *getEntry(const TCHAR *path);
	const TCHAR *getValue(const TCHAR *path, const TCHAR *defaultValue = NULL);
	INT32 getValueAsInt(const TCHAR *path, INT32 defaultValue);
	UINT32 getValueAsUInt(const TCHAR *path, UINT32 defaultValue);
	INT64 getValueAsInt64(const TCHAR *path, INT64 defaultValue);
	UINT64 getValueAsUInt64(const TCHAR *path, UINT64 defaultValue);
	bool getValueAsBoolean(const TCHAR *path, bool defaultValue);
	uuid getValueAsUUID(const TCHAR *path);
	ObjectArray<ConfigEntry> *getSubEntries(const TCHAR *path, const TCHAR *mask);
	ObjectArray<ConfigEntry> *getOrderedSubEntries(const TCHAR *path, const TCHAR *mask);

	bool setValue(const TCHAR *path, const TCHAR *value);
	bool setValue(const TCHAR *path, INT32 value);
	bool setValue(const TCHAR *path, UINT32 value);
	bool setValue(const TCHAR *path, INT64 value);
	bool setValue(const TCHAR *path, UINT64 value);
	bool setValue(const TCHAR *path, double value);
	bool setValue(const TCHAR *path, const uuid& value);

	bool parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate);

	int getErrorCount() const { return m_errorCount; }

	void print(FILE *file);
	String createXml();

   void setAlias(const TCHAR *alias, const TCHAR *value) { if (alias != NULL) m_aliases.set(alias, value); else m_aliases.remove(alias); }
   const TCHAR *getAlias(const TCHAR *alias) const { return m_aliases.get(alias); }

   bool isExpansionAllowed() { return m_allowMacroExpansion; }
};


#endif
