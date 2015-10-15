/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2011 Victor Kirhenshtein
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
** File: mm.cpp
** Purpose: Map Migration
**
**/

#include "nxdbmgr.h"
#include <netxms_maps.h>


/**
 * Generate new object ID
 *
 * @return new unique object ID
 */
static DWORD GenerateObjectId()
{
	DB_RESULT hResult = SQLSelect(_T("SELECT max(object_id) FROM object_properties"));
	if (hResult == NULL)
		return 0;

	DWORD id = DBGetFieldULong(hResult, 0, 0) + 1;
	if (id < 10)
		id = 10;
	DBFreeResult(hResult);
	return id;
}

/**
 * Create new network map object
 *
 * @param name map name
 * @param description map description (will be placed into comments)
 * @return ID of created network map object or 0 in case of failure
 */
static DWORD CreateMapObject(const TCHAR *name, const TCHAR *description)
{
	TCHAR query[4096];

	DWORD id = GenerateObjectId();
	if (id == 0)
		return 0;

	_sntprintf(query, 4096, _T("INSERT INTO network_maps (id,map_type,layout,seed,background) VALUES (%d,0,32767,0,'00000000-0000-0000-0000-000000000000')"), (int)id);
	if (!SQLQuery(query))
		return FALSE;

   _sntprintf(query, 4096, 
              _T("INSERT INTO object_properties (object_id,guid,name,")
              _T("status,is_deleted,is_system,inherit_access_rights,")
              _T("last_modified,status_calc_alg,status_prop_alg,")
              _T("status_fixed_val,status_shift,status_translation,")
              _T("status_single_threshold,status_thresholds,location_type,")
				  _T("latitude,longitude,image,comments) VALUES ")
              _T("(%d,'%s',%s,0,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
				  _T("'0.000000','0.000000','00000000-0000-0000-0000-000000000000',%s)"),
              (int)id, (const TCHAR *)uuid::generate().toString(),
				  (const TCHAR *)DBPrepareString(g_hCoreDB, name), 
				  TIME_T_FCAST(time(NULL)),
				  (const TCHAR *)DBPrepareString(g_hCoreDB, description, 2048));
	if (!SQLQuery(query))
		return FALSE;

	_sntprintf(query, 4096, _T("INSERT INTO container_members (container_id,object_id) VALUES (6,%d)"), (int)id);
	return SQLQuery(query) ? id : 0;
}

/**
 * Migrate objects
 */
static BOOL MigrateObjects(DWORD mapId, DWORD submapId, DWORD mapObjectId)
{
	TCHAR query[8192];

	_sntprintf(query, 8192, _T("SELECT object_id,x,y FROM submap_object_positions WHERE map_id=%d AND submap_id=%d"), (int)mapId, (int)submapId);
	DB_RESULT hResult = SQLSelect(query);
	if (hResult == NULL)
		return FALSE;

	BOOL success = FALSE;

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		DWORD id = DBGetFieldULong(hResult, i, 0);

		NetworkMapObject *object = new NetworkMapObject(id, id);
		object->setPosition(DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));

		Config *config = new Config();
		config->setTopLevelTag(_T("element"));
		object->updateConfig(config);
		String data = DBPrepareString(g_hCoreDB, config->createXml());
		delete config;
		delete object;

		_sntprintf(query, 8192, _T("INSERT INTO network_map_elements (map_id,element_id,element_type,element_data) VALUES (%d,%d,1,%s)"),
		           (int)mapObjectId, (int)id, (const TCHAR *)data);
		if (!SQLQuery(query))
			goto cleanup;
	}

	success = TRUE;

cleanup:
	DBFreeResult(hResult);
	return success;
}

/**
 * Migrate links between objects
 */
static BOOL MigrateLinks(DWORD mapId, DWORD submapId, DWORD mapObjectId)
{
	TCHAR query[8192];

	_sntprintf(query, 8192, _T("SELECT object_id1,object_id2,port1,port2,link_type FROM submap_links WHERE map_id=%d AND submap_id=%d"), (int)mapId, (int)submapId);
	DB_RESULT hResult = SQLSelect(query);
	if (hResult == NULL)
		return FALSE;

	BOOL success = FALSE;

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		DWORD id1 = DBGetFieldULong(hResult, i, 0);
		DWORD id2 = DBGetFieldULong(hResult, i, 1);
		int type = (int)DBGetFieldLong(hResult, i, 4);

		TCHAR cname1[256], cname2[256];
		DBGetField(hResult, i, 2, cname1, 256);
		DBGetField(hResult, i, 3, cname2, 256);
		DecodeSQLString(cname1);
		DecodeSQLString(cname2);

		_sntprintf(query, 8192, _T("INSERT INTO network_map_links (map_id,element1,element2,link_type,link_name,connector_name1,connector_name2) VALUES (%d,%d,%d,%d,'',%s,%s)"),
		           (int)mapObjectId, (int)id1, (int)id2, type, 
					  (const TCHAR *)DBPrepareString(g_hCoreDB, cname1),
					  (const TCHAR *)DBPrepareString(g_hCoreDB, cname2));
		if (!SQLQuery(query))
			goto cleanup;
	}

	success = TRUE;

cleanup:
	DBFreeResult(hResult);
	return success;
}

/**
 * Migrate 1.0.x maps to 1.1.x network map objects
 */
BOOL MigrateMaps()
{
	DB_RESULT hResult = SQLSelect(_T("SELECT map_id,map_name,description,root_object_id FROM maps"));
	if (hResult == NULL)
		return FALSE;

	BOOL success = FALSE;
	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		TCHAR name[256], description[2048];

		DBGetField(hResult, i, 1, name, 256);
		DecodeSQLString(name);
		DBGetField(hResult, i, 2, description, 2048);
		DecodeSQLString(description);
		DWORD mapObjectId = CreateMapObject(name, description);
		if (mapObjectId == 0)
			goto cleanup;

		DWORD mapId = DBGetFieldULong(hResult, i, 0);
		DWORD submapId = DBGetFieldULong(hResult, i, 3);
		if (!MigrateObjects(mapId, submapId,  mapObjectId))
			goto cleanup;
		if (!MigrateLinks(mapId, submapId, mapObjectId))
			goto cleanup;
	}

	success = TRUE;

cleanup:
	DBFreeResult(hResult);
	return success;
}
