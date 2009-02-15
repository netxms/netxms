/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
** File: clear.cpp
**
**/

#include "nxdbmgr.h"


//
// Clear database
//

BOOL ClearDatabase()
{
	printf("WARNING!!!\nThis operation will clear all configuration and collected data from database.\nAre you sure (y/N)?");
	if (!GetYesNo())
		return FALSE;

	return TRUE;
}
