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
** File: object_browser.h
**
**/

#ifndef _object_browser_h_
#define _object_browser_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxmc_api.h>


//
// Object view class
//

class nxObjectView : public wxWindow
{
public:
	nxObjectView(wxWindow *parent);
};


//
// Object browser class
//

class nxObjectBrowser : public nxView
{
private:
	wxSplitterWindow *m_wndSplitter;

public:
	nxObjectBrowser();

	// Event handlers
protected:
	void OnSize(wxSizeEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
