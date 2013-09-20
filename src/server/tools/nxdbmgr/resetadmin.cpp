/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2013 Alex Kirhenshtein
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
**/

#include "nxdbmgr.h"

//
// Reset password to default
//

void ResetAdmin()
{
	WriteToTerminal(_T("\n\n\x1b[1mWARNING!!!\x1b[0m\n"));
	if (!GetYesNo(_T("This operation will unlock admin user and change it password to default (\"netxms\").\nAre you sure?")))
   {
		return;
   }

	if (!ValidateDatabase())
   {
		return;
   }

   SQLQuery(_T("UPDATE users SET password='3A445C0072CD69D9030CC6644020E5C4576051B1', flags=8, grace_logins=5, auth_method=0, auth_failures=0, disabled_until=0 WHERE id=0"));

	_tprintf(_T("All done.\n"));
}
