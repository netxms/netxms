/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: nxconfig.h
**
**/

#ifndef _nxconfig_h_
#define _nxconfig_h_

#include <nms_common.h>
#include <nms_util.h>


//
// Config entry
//

class LIBNETXMS_EXPORTABLE ConfigEntry
{
private:
	TCHAR *m_name;
	ConfigEntry *m_parent;
	ConfigEntry *m_next;
	ConfigEntry *m_childs;
	int m_valueCount;
	TCHAR **m_values;
	TCHAR *m_file;
	int m_line;

	void linkEntry(ConfigEntry *entry) { entry->m_next = m_next; m_next = entry; }
	void addEntry(ConfigEntry *entry) { entry->m_parent = this; entry->m_next = m_childs; m_childs = entry; }

public:
	ConfigEntry(const TCHAR *name, ConfigEntry *parent, const TCHAR *file, int line);
	~ConfigEntry();

	ConfigEntry *getNext() { return m_next; }

	const TCHAR *getName() { return m_name; }
	const TCHAR *getValue(int index = 0);
	int getValueCount() { return m_valueCount; }
	int getConcatenatedValuesLength();

	const TCHAR *getFile() { return m_file; }
	int getLine() { return m_line; }

	void addValue(const TCHAR *value);
	void setValue(const TCHAR*value);

	ConfigEntry *findEntry(const TCHAR *name);

	void print(FILE *file, int level);
};


//
// Config class
//

class LIBNETXMS_EXPORTABLE Config
{
private:
	ConfigEntry *m_root;
	int m_errorCount;

protected:
	virtual void onError(const TCHAR *errorMessage);
	
	void error(const TCHAR *format, ...);

public:
	Config();
	~Config();

	bool loadXmlConfig(const TCHAR *file);
	bool loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection);
	bool loadConfig(const TCHAR *file, const TCHAR *defaultIniSection);

	bool loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection);

	ConfigEntry *getEntry(const TCHAR *path);
	const TCHAR *getValue(const TCHAR *path);

	bool parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate);

	int getErrorCount() { return m_errorCount; }

	void print(FILE *file);
};


#endif
