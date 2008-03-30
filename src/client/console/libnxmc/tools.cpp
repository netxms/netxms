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
      _sntprintf(item, 64, _T("Column%d"), i);
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
      _sntprintf(item, 64, _T("Column%d"), i);
		cfg->Read(item, &width, 50);
      wndListCtrl.SetColumnWidth(i, width);
   }

	cfg->SetPath(path);
}


//
// Show client library error
//

void LIBNXMC_EXPORTABLE NXMCShowClientError(DWORD rcc, const TCHAR *msgTemplate)
{
	TCHAR msg[4096];

	_sntprintf(msg, 4096, msgTemplate, NXCGetErrorText(rcc));
	wxLogMessage(msg);
	wxMessageBox(msg, _T("NetXMS Console - Error"), wxOK | wxICON_ERROR, g_appMainFrame);
}


//
// Adjust list view column
//

void LIBNXMC_EXPORTABLE NXMCAdjustListColumn(wxListCtrl *listCtrl, int col)
{
	int i, count, w, h, width;
	wxListItem item;

	count = listCtrl->GetItemCount();
	for(i = 0, width = 0; i < count; i++)
	{
		item.SetId(i);
		item.SetColumn(col);
		item.SetMask(wxLIST_MASK_TEXT);
		listCtrl->GetItem(item);
		listCtrl->GetTextExtent(item.GetText(), &w, &h);
		if (width < w)
			width = w;
	}
	listCtrl->SetColumnWidth(col, width + 20);
}


//
// Compare two DWORDs
//

int LIBNXMC_EXPORTABLE CompareDwords(DWORD dw1, DWORD dw2)
{
	return COMPARE_NUMBERS(dw1, dw2);
}


//
// Constructors for DCIInfo class
//

DCIInfo::DCIInfo()
{
   m_dwNodeId = 0;
   m_dwItemId = 0;
   m_iSource = 0;
   m_iDataType = 0;
   m_pszParameter = NULL;
   m_pszDescription = NULL;
}

DCIInfo::DCIInfo(DCIInfo *pSrc)
{
   m_dwNodeId = pSrc->m_dwNodeId;
   m_dwItemId = pSrc->m_dwItemId;
   m_iSource = pSrc->m_iSource;
   m_iDataType = pSrc->m_iDataType;
   m_pszParameter = _tcsdup(pSrc->m_pszParameter);
   m_pszDescription = _tcsdup(pSrc->m_pszDescription);
}

DCIInfo::DCIInfo(DWORD dwNodeId, NXC_DCI *pItem)
{
   m_dwNodeId = dwNodeId;
   m_dwItemId = pItem->dwId;
   m_iSource = pItem->iSource;
   m_iDataType = pItem->iDataType;
   m_pszParameter = _tcsdup(pItem->szName);
   m_pszDescription = _tcsdup(pItem->szDescription);
}


//
// Destructor for class DCIInfo
//

DCIInfo::~DCIInfo()
{
   safe_free(m_pszParameter);
   safe_free(m_pszDescription);
}
