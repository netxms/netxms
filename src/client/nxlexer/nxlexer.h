/* 
** NetXMS - Network Management System
** Lexer for Scintilla
** Copyright (C) 2006 Victor Kirhenshtein
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
** $module: nxlexer.h
**
**/

#ifndef _nxlexer_h_
#define _nxlexer_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxlexer_styles.h>

#include <Scintilla.h>
#include <Platform.h>
#include <PropSet.h>
#include <Accessor.h>
#include <WindowAccessor.h>


//
// Functions
//

BOOL IsKeyword(char *pszList, char *pszWord);


#endif
