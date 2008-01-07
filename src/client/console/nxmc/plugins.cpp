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
// Static data
//

static nxmcArrayOfPlugins s_pluginList;
static int s_freeBaseId = wxID_PLUGIN_RANGE_START;


//
// nxmcPlugin class implementation
//

nxmcPlugin::nxmcPlugin(HMODULE hmod, NXMC_PLUGIN_INFO *info, int baseId, void (*fpCmdHandler)(int))
{
	m_moduleHandle = hmod;
	m_info = (NXMC_PLUGIN_INFO *)nx_memdup(info, sizeof(NXMC_PLUGIN_INFO));
	m_baseId = baseId;
	m_fpCommandHandler = fpCmdHandler;
}

nxmcPlugin::~nxmcPlugin()
{
	safe_free(m_info);
}

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(nxmcArrayOfPlugins);


//
// Load plugin
//

static bool LoadPlugin(wxString &file)
{
	HMODULE module;
	TCHAR errorText[256];
	bool status = false;
	void (*fpReg)(NXMC_PLUGIN_INFO *);
	bool (*fpInit)(NXMC_PLUGIN_HANDLE);
	void (*fpCmdHandler)(int);
	NXMC_PLUGIN_INFO info;
	nxmcPlugin *plugin;
	size_t i;

	wxLogDebug(_T("Loading plugin %s..."), file.c_str());
	module = DLOpen(file.c_str(), errorText);
	if (module != NULL)
	{
		fpReg = (void (*)(NXMC_PLUGIN_INFO *))DLGetSymbolAddr(module, _T("nxmcRegisterPlugin"), NULL);
		fpInit = (bool (*)(NXMC_PLUGIN_HANDLE))DLGetSymbolAddr(module, _T("nxmcInitializePlugin"), NULL);
		fpCmdHandler = (void (*)(int))DLGetSymbolAddr(module, _T("nxmcCommandHandler"), NULL);
		if ((fpReg != NULL) && (fpInit != NULL))
		{
			fpReg(&info);

			// Check for duplicate plugin names
			for(i = 0; i < s_pluginList.GetCount(); i++)
				if (!_tcsicmp(s_pluginList[i].GetName(), info.name))
				{
					wxLogWarning(_T("Plugin with name %s already loaded"), info.name);
					break;
				}

			if (i == s_pluginList.GetCount())
			{
				plugin = new nxmcPlugin(module, &info, s_freeBaseId, fpCmdHandler);
				if (fpInit(plugin))
				{
					s_pluginList.Add(plugin);
					s_freeBaseId += NXMC_PLUGIN_ID_LIMIT;
					status = true;
					wxLogMessage(_T("Plugin %s successfully loaded from %s"), info.name, file.c_str());
				}
				else
				{
					delete plugin;
					wxLogWarning(_T("Plugin %s loaded from %s failed to initialize itself"), info.name, file.c_str());
				}
			}

			if (!status)
				DLClose(module);
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


//
// Call plugin's command handler
//

void CallPluginCommandHandler(int cmd)
{
	size_t index;

	index = (cmd - wxID_PLUGIN_RANGE_START) / NXMC_PLUGIN_ID_LIMIT;
	if (index < s_pluginList.GetCount())
		s_pluginList[index].CallCommandHandler(cmd - s_pluginList[index].GetBaseId());
}
