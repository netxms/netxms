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
#include <expat.h>


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
// Get all subentries with names matched to mask
//

ConfigEntryList *ConfigEntry::getSubEntries(const TCHAR *mask)
{
	ConfigEntry *e, **list = NULL;
	int count = 0, allocated = 0;

	for(e = m_childs; e != NULL; e = e->getNext())
		if (MatchString(mask, e->getName(), FALSE))
		{
			if (count == allocated)
			{
				allocated += 10;
				list = (ConfigEntry **)realloc(list, sizeof(ConfigEntry *) * allocated);
			}
			list[count++] = e;
		}
	return new ConfigEntryList(list, count);
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
// Print config entry
//

void ConfigEntry::print(FILE *file, int level)
{
	_tprintf(_T("%*s%s\n"), level * 4, _T(""), m_name);
	for(ConfigEntry *e = m_childs; e != NULL; e = e->getNext())
		e->print(file, level + 1);

	for(int i = 0, len = 0; i < m_valueCount; i++)
		_tprintf(_T("%*svalue: %s\n"), level * 4 + 2, _T(""), m_values[i]);
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
// Simulation of old NxLoadConfig() API
//

bool Config::parseTemplate(const TCHAR *section, NX_CFG_TEMPLATE *cfgTemplate)
{
	TCHAR name[MAX_PATH], *curr, *eptr;
	int i, j, pos, initialErrorCount = m_errorCount;
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

	return (m_errorCount - initialErrorCount) == 0;
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
// Get subentries
//

ConfigEntryList *Config::getSubEntries(const TCHAR *path, const TCHAR *mask)
{
	ConfigEntry *entry = getEntry(path);
	return (entry != NULL) ? entry->getSubEntries(mask) : NULL;
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

	if (!_tcscmp(path, _T("/")))
		return m_root;

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


//
// Load INI-style config
//

bool Config::loadIniConfig(const TCHAR *file, const TCHAR *defaultIniSection)
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

	currentSection = m_root->findEntry(defaultIniSection);
	if (currentSection == NULL)
	{
		currentSection = new ConfigEntry(defaultIniSection, m_root, file, 0);
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
			{
				entry = new ConfigEntry(buffer, currentSection, file, sourceLine);
			}
			entry->addValue(ptr);
      }
   }
   fclose(cfg);
	return true;
}


//
// Load config from XML file
//

#define MAX_STACK_DEPTH		256

typedef struct
{
	XML_Parser parser;
	Config *config;
	const TCHAR *file;
	int level;
	ConfigEntry *stack[MAX_STACK_DEPTH];
	String charData[MAX_STACK_DEPTH];
	bool trimValue[MAX_STACK_DEPTH];
} XML_PARSER_STATE;

static void StartElement(void *userData, const char *name, const char **attrs)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if (ps->level == 0)
	{
		if (!stricmp(name, "config"))
		{
			ps->stack[ps->level++] = ps->config->getEntry(_T("/"));
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

			DWORD id = XMLGetAttrDWORD(attrs, "id", 0);
#ifdef UNICODE
			if (id != 0)
			{
				WCHAR wname[MAX_PATH];

				MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_PATH);
				wname[MAX_PATH - 1] = 0;
				_snwprintf(entryName, MAX_PATH, L"%s#%u", wname, id);
			}
			else
			{
				MultiByteToWideChar(CP_UTF8, 0, name, -1, entryName, MAX_PATH);
				entryName[MAX_PATH - 1] = 0;
			}
#else
			if (id != 0)
				snprintf(entryName, MAX_PATH, "%s#%u", name, id);
			else
				nx_strncpy(entryName, name, MAX_PATH);
#endif
			ps->stack[ps->level] = ps->stack[ps->level - 1]->findEntry(entryName);
			if (ps->stack[ps->level] == NULL)
				ps->stack[ps->level] = new ConfigEntry(entryName, ps->stack[ps->level - 1], ps->file, XML_GetCurrentLineNumber(ps->parser));
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

static void CharData(void *userData, const XML_Char *s, int len)
{
	XML_PARSER_STATE *ps = (XML_PARSER_STATE *)userData;

	if ((ps->level > 0) && (ps->level <= MAX_STACK_DEPTH))
		ps->charData[ps->level - 1].AddMultiByteString(s, len, CP_UTF8);
}

bool Config::loadXmlConfig(const TCHAR *file)
{
	BYTE *xml;
	DWORD size;
	bool success;

	xml = LoadFile(file, &size);
	if (xml != NULL)
	{
		XML_PARSER_STATE state;

		XML_Parser parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, &state);
		XML_SetElementHandler(parser, StartElement, EndElement);
		XML_SetCharacterDataHandler(parser, CharData);

		state.config = this;
		state.level = 0;
		state.parser = parser;
		state.file = file;

		success = (XML_Parse(parser, (char *)xml, size, TRUE) != XML_STATUS_ERROR);
		if (!success)
		{
			error(_T("%s at line %d"), XML_ErrorString(XML_GetErrorCode(parser)),
               XML_GetCurrentLineNumber(parser));
		}
	}
	else
	{
		success = false;
	}

	return success;
}


//
// Load config file with format auto detection
//

bool Config::loadConfig(const TCHAR *file, const TCHAR *defaultIniSection)
{
	FILE *f;
	int ch;

	f = _tfopen(file, _T("r"));
	if (f == NULL)
	{
		error(_T("Cannot open file %s"), file);
		return false;
	}

	// Skip all space/newline characters
	do
	{
		ch = fgetc(f);
	} while(isspace(ch));

	fclose(f);

	// If first non-space character is < assume XML format, otherwise assume INI format
	return (ch == '<') ? loadXmlConfig(file) : loadIniConfig(file, defaultIniSection);
}


//
// Load all files in given directory
//

bool Config::loadConfigDirectory(const TCHAR *path, const TCHAR *defaultIniSection)
{
	DIR *dir;
   struct dirent *file;
	TCHAR fileName[MAX_PATH];
	bool success;

#ifdef UNICODE
	char mbpath[MAX_PATH];
	WCHAR wname[MAX_PATH];

	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, path, -1, mbpath, MAX_PATH, NULL, NULL);
	mbpath[MAX_PATH - 1] = 0;
   dir = opendir(mbpath);
#else
   dir = opendir(path);
#endif
   if (dir != NULL)
   {
		success = true;
      while(1)
      {
         file = readdir(dir);
         if (file == NULL)
            break;

         if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
            continue;
         
			size_t len = _tcslen(path) + strlen(file->d_name) + 2;
			if (len > MAX_PATH)
				continue;	// Full file name is too long

         _tcscpy(fileName, path);
         _tcscat(fileName, FS_PATH_SEPARATOR);
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, file->d_name, -1, wname, MAX_PATH);
         wcscat(fileName, wname);
#else
         strcat(fileName, file->d_name);
#endif

			success = success && loadConfig(fileName, defaultIniSection);
      }
      closedir(dir);
   }
	else
	{
		success = false;
	}

	return success;
}


//
// Print config to output stream
//

void Config::print(FILE *file)
{
	if (m_root != NULL)
		m_root->print(file, 0);
}
