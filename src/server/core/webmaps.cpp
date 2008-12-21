/* $Id$ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: webmaps.cpp
**
**/

#include "nxcore.h"


//
// Check existence of webmap
//

static BOOL IsWebMapExist(DWORD id)
{
	TCHAR query[256];
	DB_RESULT hResult;
	BOOL isExist = FALSE;

	_sntprintf(query, 256, _T("SELECT id FROM web_maps WHERE id=%d"), id);
	hResult = DBSelect(g_hCoreDB, query);
	if (hResult != NULL)
	{
		isExist = (DBGetNumRows(hResult) > 0);
		DBFreeResult(hResult);
	}
	return isExist;
}


//
// Create webmap. Returns RCC.
//

DWORD CreateWebMap(const TCHAR *name, const TCHAR *props, DWORD *id)
{
	TCHAR *escProps, *query;
	int len;
	DWORD mapId, rcc;

	escProps = EncodeSQLString(CHECK_NULL_EX(props));
	len = _tcslen(escProps) + 256;
	query = (TCHAR *)malloc(len * sizeof(TCHAR));
	mapId = CreateUniqueId(IDG_MAP);
	_sntprintf(query, len, _T("INSERT INTO web_maps (id,title,properties,data) VALUES (%d,'%s','%s','#00')"),
			     mapId, name, escProps);
	free(escProps);
	rcc = DBQuery(g_hCoreDB, query) ? RCC_SUCCESS : RCC_DB_FAILURE;
	*id = mapId;
	free(query);
	return rcc;
}


//
// Delete webmap. Returns RCC.
//

DWORD DeleteWebMap(DWORD id)
{
	DWORD rcc;
	TCHAR query[256];

	if (IsWebMapExist(id))
	{
		_sntprintf(query, 256, _T("DELETE FROM web_maps WHERE id=%d"), id);
		rcc = DBQuery(g_hCoreDB, query) ? RCC_SUCCESS : RCC_DB_FAILURE;
	}
	else
	{
		rcc = RCC_INVALID_MAP_ID;
	}
	return rcc;
}


//
// Update webmap's data. Returns RCC.
//

DWORD UpdateWebMapData(DWORD mapId, const TCHAR *data)
{
	DWORD rcc;
	int len;
	TCHAR *query, *escData;

	if (IsWebMapExist(mapId))
	{
		escData = EncodeSQLString(CHECK_NULL_EX(data));
		len = _tcslen(escData) + 256;
		query = (TCHAR *)malloc(len * sizeof(TCHAR));
		_sntprintf(query, len, _T("UPDATE web_maps SET data='%s' WHERE id=%d"), escData, mapId);
		free(escData);
		rcc = DBQuery(g_hCoreDB, query) ? RCC_SUCCESS : RCC_DB_FAILURE;
		free(query);
	}
	else
	{
		rcc = RCC_INVALID_MAP_ID;
	}
	return rcc;
}


//
// Update webmap's properties. Returns RCC.
//

DWORD UpdateWebMapProperties(DWORD mapId, const TCHAR *name, const TCHAR *props)
{
	DWORD rcc;
	int len;
	TCHAR *query, *escProps;

	if (IsWebMapExist(mapId))
	{
		escProps = EncodeSQLString(CHECK_NULL_EX(props));
		len = _tcslen(escProps) + 256;
		query = (TCHAR *)malloc(len * sizeof(TCHAR));
		_sntprintf(query, len, _T("UPDATE web_maps SET title='%s',properties='%s' WHERE id=%d"),
		           name, escProps, mapId);
		free(escProps);
		rcc = DBQuery(g_hCoreDB, query) ? RCC_SUCCESS : RCC_DB_FAILURE;
		free(query);
	}
	else
	{
		rcc = RCC_INVALID_MAP_ID;
	}
	return rcc;
}
