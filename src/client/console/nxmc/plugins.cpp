/* 
** NetXMS - Network Management System
** Portable management console
** Copyright (C) 2007 Victor Kirhenshtein
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
** File: plugins.cpp
**
**/

#include "nxmc.h"


//
// Load plugin
//

static bool LoadPlugin(wxString &file)
{
	HMODULE module;
	TCHAR errorText[256];
	bool status = false;
	void (*fpReg)(NXMC_PLUGIN_INFO *);
	bool (*fpInit)();
	NXMC_PLUGIN_INFO info;

	wxLogDebug(_T("Loading plugin %s..."), file.c_str());
	module = DLOpen(file.c_str(), errorText);
	if (module != NULL)
	{
		fpReg = (void (*)(NXMC_PLUGIN_INFO *))DLGetSymbolAddr(module, _T("nxmcRegisterPlugin"), NULL);
		fpInit = (bool (*)())DLGetSymbolAddr(module, _T("nxmcInitializePlugin"), NULL);
		if ((fpReg != NULL) && (fpInit != NULL))
		{
			fpReg(&info);

			// Check for duplicate plugin names

			status = true;
			wxLogMessage(_T("Plugin %s successfully loaded form %s"), info.name, file.c_str());
		}
		else
		{
			wxLogWarning(_T("Cannot load plugin module %s: unable to find required entry points"), file.c_str());
		}
	}
	else
	{
		wxLogWarning(_T("Cannot load plugin module %s: %s"), file.c_str(), errorText);
	}
	return status;
}


//
// Load plugins
//

void LoadPlugins(void)
{
	TCHAR *p;
	wxString fname, path;
	bool success;

	// Determine plugin path
	p = _tgetenv(_T("NXMC_PLUGIN_PATH"));
	if (p != NULL)
	{
		path = p;
		if (path.Last() != FS_PATH_SEPARATOR_CHAR)
			path.Append(FS_PATH_SEPARATOR_CHAR, 1);
	}
	else
	{
		// Search plugins in <install_dir>/lib/nxmc directory
		path = wxStandardPaths::Get().GetPluginsDir();
#ifdef _WIN32
		// On Windows, GetPluginsDir() will return bin directory, change it to lib\nxmc
		int idx = path.Find(FS_PATH_SEPARATOR_CHAR, true);
		if (idx != wxNOT_FOUND)
		{
			path.Truncate(idx);
			path.Append(_T("\\lib\\nxmc\\"));
		}
#else
		if (path.Last() != FS_PATH_SEPARATOR_CHAR)
			path.Append(FS_PATH_SEPARATOR_CHAR, 1);
#endif
	}

	// Find and load plugins
	wxDir dir(path);
	if (dir.IsOpened())
	{
		wxString fullName;

		success = dir.GetFirst(&fname, _T("*.so"));
		while(success)
		{
			fullName = path + fname;
			LoadPlugin(fullName);
			success = dir.GetNext(&fname);
		}
	}
}
