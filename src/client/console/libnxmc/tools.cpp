/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
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
** File: tools.cpp
**
**/

#include "libnxmc.h"


//
// Format time stamp
//

TCHAR LIBNXMC_EXPORTABLE *NXMCFormatTimeStamp(time_t timeStamp, TCHAR *buffer, int type)
{
   struct tm *pt;
   static TCHAR *format[] = { _T("%d-%b-%Y %H:%M:%S"), _T("%H:%M:%S"), _T("%b/%d"), _T("%b") };

   pt = localtime(&timeStamp);
	if (pt != NULL)
		_tcsftime(buffer, 32, format[type], pt);
	else
		_tcscpy(buffer, _T("(null)"));
   return buffer;
}


//
// Translate given code to text
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCCodeToText(int code, CODE_TO_TEXT *translator, const TCHAR *defaultText)
{
   int i;

   for(i = 0; translator[i].text != NULL; i++)
      if (translator[i].code == code)
         return translator[i].text;
   return defaultText;
}


//
// Load resource file
//

bool LIBNXMC_EXPORTABLE NXMCLoadResources(const TCHAR *name, NXMC_LIB_INSTANCE instance, TCHAR *resName)
{
	bool success = false;
#ifdef _WIN32
	HRSRC hRes;
	HGLOBAL hMem;
	void *data;

	hRes = FindResource(instance, resName, _T("XRS"));
	if (hRes != NULL)
	{
		hMem = LoadResource(instance, hRes);
		if (hMem != NULL)
		{
			data = LockResource(hMem);
			if (data != NULL)
			{
				wxMemoryFSHandler::AddFile(name, data, SizeofResource(instance, hRes));
				UnlockResource(hMem);
			}
			FreeResource(hMem);
		}
		else
		{
			wxLogWarning(_T("NXMCLoadResources: cannot load resource %s from executable"), name);
			return false;
		}
	}
	else
	{
		wxLogWarning(_T("NXMCLoadResources: cannot find resource %s in executable module"), name);
		return false;
	}
	wxString xrsFile = _T("memory:");
#else
	wxString xrsFile = wxStandardPaths::Get().GetResourcesDir();
	xrsFile += FS_PATH_SEPARATOR;
#endif
	xrsFile += name;
	wxLogDebug(_T("Loading resource file %s"), xrsFile.c_str());
	return wxXmlResource::Get()->Load(xrsFile);
}


//
// Save dimensions of all list control columns into config
//

void LIBNXMC_EXPORTABLE NXMCSaveListCtrlColumns(wxConfigBase *cfg, wxListCtrl &wndListCtrl, const TCHAR *prefix)
{
   int i, count;
	wxString path = cfg->GetPath();
   TCHAR item[64];

	cfg->SetPath(prefix);
	count = wndListCtrl.GetColumnCount();
	cfg->Write(_T("ColumnCount"), count);
	for(i = 0; i < count; i++)
   {
      _stprintf(item, _T("Column%d"), i);
		cfg->Write(item, wndListCtrl.GetColumnWidth(i));
   }
	cfg->SetPath(path);
}


//
// Load and set dimensions of all list control columns
//

void LIBNXMC_EXPORTABLE NXMCLoadListCtrlColumns(wxConfigBase *cfg, wxListCtrl &wndListCtrl, const TCHAR *prefix)
{
   long i, count, width;
	wxString path = cfg->GetPath();
   TCHAR item[64];

	cfg->SetPath(prefix);
	cfg->Read(_T("ColumnCount"), &count, 0);

   for(i = 0; i < count; i++)
   {
      _stprintf(item, _T("Column%d"), i);
		cfg->Read(item, &width, 50);
      wndListCtrl.SetColumnWidth(i, width);
   }

	cfg->SetPath(path);
}
