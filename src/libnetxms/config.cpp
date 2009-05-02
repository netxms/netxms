/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
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
** File: config.cpp
**
**/

#include "libnetxms.h"
#include <nxconfig.h>


//
// Constructor for config entry
//

ConfigEntry::ConfigEntry(const TCHAR *name, ConfigEntry *parent, const TCHAR *file, int line)
{
	m_name = _tcsdup(CHECK_NULL(name));
	m_childs = NULL;
	m_next = NULL;
	if (parent != NULL)
		parent->addEntry(this);
	m_valueCount = 0;
	m_values = NULL;
	m_file = _tcsdup(CHECK_NULL(file));
	m_line = line;
}


//
// Destructor for config entry
//

ConfigEntry::~ConfigEntry()
{
	ConfigEntry *entry, *next;

	for(entry = m_childs; entry != NULL; entry = next)
	{
		next = entry->getNext();
		delete entry;
	}
	safe_free(m_name);

	for(int i = 0; i < m_valueCount; i++)
		safe_free(m_values[i]);
	safe_free(m_values);
}


//
// Find entry by name
//

ConfigEntry *ConfigEntry::findEntry(const TCHAR *name)
{
	ConfigEntry *e;

	for(e = m_childs; e != NULL; e = e->getNext())
		if (!_tcsicmp(e->getName(), name))
			return e;
	return NULL;
}


//
// Get value
//

const TCHAR *ConfigEntry::getValue(int index)
{
	if ((index < 0) || (index >= m_valueCount))
		return NULL;
	return m_values[index];
}


//
// Set value
//

void ConfigEntry::setValue(const TCHAR *value)
{
	for(int i = 0; i < m_valueCount; i++)
		safe_free(m_values[i]);
	m_valueCount = 1;
	m_values = (TCHAR **)realloc(m_values, sizeof(TCHAR *));
	m_values[0] = _tcsdup(value);
}


//
// Add value
//

void ConfigEntry::addValue(const TCHAR *value)
{
	m_values = (TCHAR **)realloc(m_values, sizeof(TCHAR *) * (m_valueCount + 1));
	m_values[m_valueCount] = _tcsdup(value);
	m_valueCount++;
}


//
// Get summary length of all values as if they was concatenated with separator character
//

int ConfigEntry::getConcatenatedValuesLength()
{
	int i, len;

	if (m_valueCount < 1)
		return 0;

	for(i = 0, len = 0; i < m_valueCount; i++)
		len += _tcslen(m_values[i]);
	return len + (m_valueCount - 1);
}


//
// Constructor for config
//

Config::Config()
{
	m_root = new ConfigEntry(_T("[root]"), NULL, NULL, 0);
	m_errorCount = 0;
}


//
// Destructor
//

Config::~Config()
{
	delete m_root;
}


//
// Default error handler
//

void Config::onError(const TCHAR *errorMessage)
{
	_ftprintf(stderr, _T("%s\n"), errorMessage);
}


//
// Report error
//

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


//
// Bind parameters to variables: simulation of old NxLoadConfig() API
//

bool Config::bindParameters(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate)
{
	TCHAR name[MAX_PATH], *curr, *eptr;
	int i, j, pos;
	ConfigEntry *entry;

	name[0] = _T('/');
	nx_strncpy(&name[1], section, MAX_PATH - 2);
	_tcscat(name, _T("/"));
	pos = _tcslen(name);
	
	for(i = 0; cfgTemplate[i].iType != CT_END_OF_LIST; i++)
	{
		nx_strncpy(&name[pos], cfgTemplate[i].szToken, MAX_PATH - pos);
		entry = getEntry(name);
		if (entry != NULL)
		{
			const TCHAR *value = CHECK_NULL(entry->getValue());
         switch(cfgTemplate[i].iType)
         {
            case CT_LONG:
					*((LONG *)cfgTemplate[i].pBuffer) = _tcstol(value, &eptr, 0);
               if (*eptr != 0)
               {
						error(_T("Invalid number '%s' in configuration file %s at line %d\n"), value, entry->getFile(), entry->getLine());
               }
               break;
            case CT_WORD:
               *((WORD *)cfgTemplate[i].pBuffer) = (WORD)_tcstoul(value, &eptr, 0);
               if (*eptr != 0)
               {
						error(_T("Invalid number '%s' in configuration file %s at line %d\n"), value, entry->getFile(), entry->getLine());
               }
               break;
            case CT_BOOLEAN:
               if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) ||
                   !_tcsicmp(value, _T("on")) || !_tcsicmp(value, _T("1")))
               {
                  *((DWORD *)cfgTemplate[i].pBuffer) |= cfgTemplate[i].dwBufferSize;
               }
               else
               {
                  *((DWORD *)cfgTemplate[i].pBuffer) &= ~(cfgTemplate[i].dwBufferSize);
               }
               break;
            case CT_STRING:
               nx_strncpy((TCHAR *)cfgTemplate[i].pBuffer, value, cfgTemplate[i].dwBufferSize);
               break;
            case CT_STRING_LIST:
					*((TCHAR **)cfgTemplate[i].pBuffer) = (TCHAR *)malloc(sizeof(TCHAR) * (entry->getConcatenatedValuesLength() + 1));
					for(j = 0, curr = *((TCHAR **)cfgTemplate[i].pBuffer); j < entry->getValueCount(); j++)
					{
						_tcscpy(curr, entry->getValue(j));
						curr += _tcslen(curr);
						*curr = cfgTemplate[i].cSeparator;
						curr++;
					}
					if (j > 0)
						curr--;
					*curr = 0;
               break;
            case CT_IGNORE:
               break;
            default:
               break;
         }
		}
	}

	return m_errorCount == 0;
}


//
// Load INI-style config
//

bool Config::loadIniConfig(const TCHAR *file, const TCHAR *defaultSectionName)
{
	FILE *cfg;
	TCHAR buffer[4096], *ptr;
	ConfigEntry *currentSection;
	int sourceLine = 0;

   cfg = _tfopen(file, _T("r"));
   if (cfg == NULL)
	{
		error(_T("Cannot open file %s"), file);
		return false;
	}

	currentSection = m_root->findEntry(defaultSectionName);
	if (currentSection == NULL)
	{
		currentSection = new ConfigEntry(defaultSectionName, m_root, file, 0);
	}

   while(!feof(cfg))
   {
      // Read line from file
      buffer[0] = 0;
      _fgetts(buffer, 4095, cfg);
      sourceLine++;
      ptr = _tcschr(buffer, _T('\n'));
      if (ptr != NULL)
         *ptr = 0;
      ptr = _tcschr(buffer, _T('#'));
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
				currentSection = new ConfigEntry(&buffer[1], m_root, file, sourceLine);
			}
      }
      else
      {
         // Divide on two parts at = sign
         ptr = _tcschr(buffer, _T('='));
         if (ptr == NULL)
         {
            error(_T("Syntax error in configuration file %s at line %d"), file, sourceLine);
            continue;
         }
         *ptr = 0;
         ptr++;
         StrStrip(buffer);
         StrStrip(ptr);

			ConfigEntry *entry = currentSection->findEntry(buffer);
			if (entry == NULL)
				entry = new ConfigEntry(buffer, currentSection, file, sourceLine);
			entry->addValue(ptr);
      }
   }
   fclose(cfg);
	return true;
}


//
// Get value
//

const TCHAR *Config::getValue(const TCHAR *path)
{
	ConfigEntry *entry = getEntry(path);
	return (entry != NULL) ? entry->getValue() : NULL;
}


//
// Get entry
//

ConfigEntry *Config::getEntry(const TCHAR *path)
{
	const TCHAR *curr, *end;
	TCHAR name[256];
	ConfigEntry *entry = m_root;

	if ((path == NULL) || (*path != _T('/')))
		return NULL;

	curr = path + 1;
	while(entry != NULL)
	{
		end = _tcschr(curr, _T('/'));
		if (end != NULL)
		{
			int len = min((int)(end - curr), 255);
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
