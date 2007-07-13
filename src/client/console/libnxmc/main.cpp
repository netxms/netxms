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
** File: main.cpp
**
**/

#include "libnxmc.h"


//
// Static data
//

static bool s_isUiInitialized = false;
static nxmcArrayOfRegItems s_regItemList;


//
// Registration item class implementation
//

nxmcItemRegistration::nxmcItemRegistration(const TCHAR *name, int id, int type, void (*fpHandler)(int))
{
	m_name = _tcsdup(name);
	m_id = id;
	m_type = type;
	m_fpHandler = fpHandler;
}

nxmcItemRegistration::~nxmcItemRegistration()
{
	safe_free(m_name);
}

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(nxmcArrayOfRegItems);


//
// Add Control Panel item
//

bool LIBNXMC_EXPORTABLE NXMCAddControlPanelItem(const TCHAR *name, int id, void (*fpHandler)(int))
{
	if (s_isUiInitialized)
		return false;	// Currently this registration is not allowed after initialization

	s_regItemList.Add(new nxmcItemRegistration(name, id, REGITEM_CONTROL_PANEL, fpHandler));
	return true;
}


//
// Add View menu item
//

bool LIBNXMC_EXPORTABLE NXMCAddViewMenuItem(const TCHAR *name, int id, void (*fpHandler)(int))
{
	if (s_isUiInitialized)
		return false;	// Currently this registration is not allowed after initialization

	s_regItemList.Add(new nxmcItemRegistration(name, id, REGITEM_VIEW_MENU, fpHandler));
	return true;
}


//
// Change UI initialization flag
//

void LIBNXMC_EXPORTABLE NXMCInitializationComplete(void)
{
	s_isUiInitialized = true;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
