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
** File: export.cpp
**
**/

#include "nxdbmgr.h"


//
// Export single database table
//

static void ExportTable(const char *name, ...)
{
	va_list args;
	String query1, query2;
	const char *field;

	query1 = "SELECT ";
	query2.AddFormattedString("CREATE TABLE %s (", name); 
	va_start(args, name);
	
	while((field = va_arg(args, const char *)) != NULL)
	{
		query += field;
		query += ",";
	}

	va_end(args);
}


//
// Export database
//

void ExportDatabase(void)
{
	if (!ValidateDatabase())
		return;

	ExportTable("config_clob", "var_name", "var_value", NULL);

	_tprintf(_T("Database export complete.\n"));
}
