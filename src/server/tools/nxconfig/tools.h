/* 
** NetXMS - Network Management System
** Server Configurator for Windows
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
** $module: tools.h
**
**/


#ifndef _tools_h_
#define _tools_h_


//
// Functions
//

void EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable);
BOOL GetEditBoxValueULong(CDialog *pWnd, int nCtrl, DWORD *pdwValue,
                          DWORD dwMin = 0, DWORD dwMax = 0xFFFFFFFF);


#endif
