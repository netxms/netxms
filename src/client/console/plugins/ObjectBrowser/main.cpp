/* 
** NetXMS - Network Management System
** Portable management console - Object Browser plugin
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
** File: main.cpp
**
**/

#include "object_browser.h"

#ifdef _WIN32
#include "resource.h"
#endif


//
// Static data
//

#ifdef _WIN32
static HINSTANCE s_libInstance;
#endif


//
// Create object browser
//

extern "C" void NXMC_PLUGIN_EXPORT nxmcCommandHandler(int cmd)
{
	nxView *view;
	
	if ((view = FindUniqueView(_T("objectbrowser"))) == NULL)
	{
		NXMCCreateView(new nxObjectBrowser, VIEWAREA_MAIN);
	}
	else
	{
		ActivateView(view);
	}
}


//
// Registration function
//

NXMC_IMPLEMENT_PLUGIN_REGISTRATION(_T("ObjectBrowser"), NETXMS_VERSION_STRING, NXMC_IP_MAIN_MENU)


//
// Initialization function
//

extern "C" bool NXMC_PLUGIN_EXPORT nxmcInitializePlugin(NXMC_PLUGIN_HANDLE handle)
{
	if (!NXMCLoadResources(_T("ObjectBrowser.xrs"), NXMC_LIB_INSTANCE_ARG(s_libInstance), wxMAKEINTRESOURCE(IDR_XRS)))
		wxLogWarning(_T("ObjectBrowser: cannot load resource file"));
	NXMCAddViewMenuItem(handle, _T("&Object Browser\tF3"), 0);
	return true;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	s_libInstance = hInstance;
	return TRUE;
}

#endif
