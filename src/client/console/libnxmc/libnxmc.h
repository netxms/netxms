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
** File: libnxmc.h
**
**/

#ifndef _libnxmc_h_
#define _libnxmc_h_

#define WXUSINGDLL

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>

#ifdef _WIN32
#include <wx/msw/winundef.h>
#endif

#include <wx/wx.h>

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/fs_mem.h>
#include <wx/aui/aui.h>
#include <wx/dir.h>
#endif

#include <nxmc_api.h>


//
// Hash map types
//

WX_DECLARE_STRING_HASH_MAP(nxView*, nxViewHash);


#endif
