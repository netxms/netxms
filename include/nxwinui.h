/* 
** NetXMS - Network Management System
** Windows UI Library
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: nxwinui.h
**
**/

#ifndef _nxwinui_h_
#define _nxwinui_h_

#ifdef NXUILIB_EXPORTS
#define NXUILIB_EXPORTABLE __declspec(dllexport)
#else
#define NXUILIB_EXPORTABLE __declspec(dllimport)
#endif


//
// Exportable classes
//

#include "../src/console/nxuilib/resource.h"
#include "../src/console/nxuilib/LoginDialog.h"


//
// Functions
//

void NXUILIB_EXPORTABLE EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable);

#endif
