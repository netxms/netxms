/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: audit.cpp
**
**/

#include "nxcore.h"


//
// Write audit record
//

void WriteAuditLog(DWORD dwCategory, DWORD dwUserId, DWORD dwObjectId, TCHAR *pszText)
{
	TCHAR *pszQuery, *pszEscText;

	if (dwCategory & g_dwAuditFlags)
	{
		pszEscText = EncodeSQLString(pszText);
		pszQuery = (TCHAR *)malloc((_tcslen(pszText) + 256) * sizeof(TCHAR));
		_stprintf(pszQuery, _T("INSERT INTO audit_log (timestamp,user_id,object_id,message) VALUES(%d,%d,%d,'%s')"),
		          time(NULL), dwUserId, dwObjectId, pszEscText);
		free(pszEscText);
		QueueSQLRequest(pszQuery);
		free(pszQuery);
	}
}
