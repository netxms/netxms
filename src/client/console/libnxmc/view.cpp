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
** File: view.cpp
**
**/

#include "libnxmc.h"


//
// Static data
//

static nxViewCreatorsHash s_viewCreators;


//
// Create view in given area
//

bool LIBNXMC_EXPORTABLE NXMCCreateView(nxView *view, int area)
{
	if ((g_auiManager == NULL) || (g_auiNotebook == NULL))
		return false;

	switch(area)
	{
		case VIEWAREA_MAIN:
			view->Reparent(g_auiNotebook);
			g_auiNotebook->AddPage(view, view->GetLabel(), true, view->GetIcon());
			break;
		case VIEWAREA_DOCKED:
			g_auiManager->AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()));
			g_auiManager->Update();
			break;
		case VIEWAREA_FLOATING:
			g_auiManager->AddPane(view, wxAuiPaneInfo().Name(view->GetName()).Caption(view->GetLabel()).Float());
			g_auiManager->Update();
			break;
		default:
			wxLogWarning(_T("INTERNAL ERROR: invalid area code %d passed to NXMCCreateView()"), area);
			break;
	}
	return true;
}


//
// Register view creator
//

void LIBNXMC_EXPORTABLE NXMCRegisterViewCreator(const TCHAR *viewClass, nxViewCreator func)
{
	s_viewCreators[viewClass] = func;
}


//
// Create view provided by plugin or main app by class name
//

nxView LIBNXMC_EXPORTABLE *NXMCCreateViewByClass(const TCHAR *viewClass, wxWindow *parent,
                                                 const TCHAR *context, NXC_OBJECT *object,
                                                 void *userData)
{
	nxViewCreatorsHash::iterator it;
		
	it = s_viewCreators.find(viewClass);
	return (it == s_viewCreators.end()) ? NULL : it->second(parent, context, object, userData);
}

