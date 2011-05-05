static DWORD GenerateObjectId()
{
	DB_RESULT hResult = SQLSelect(_T("SELECT max(object_id) FROM object_properties"));
	if (hResult == NULL)
		return 0;

	DWORD id = DBGetFieldULong(hResult, 0, 0) + 1;
	DBFreeResult(hResult);
	return id;
}

static DWORD CreateMapObject(const TCHAR *name, const TCHAR *description)
{
	TCHAR query[4096];

	DWORD id = GenerateObjectId();
	if (id == 0)
		return 0;

	_sntprintf(query, 4096, _T("INSERT INTO network_maps (id,map_type,layout,seed,background) VALUES (%d,0,32767,0,'')"), (int)id);
	if (!SQLQuery(query))
		return FALSE;

	uuid_t guid;
	TCHAR guidText[128];

	uuid_generate(guid);
   _sntprintf(query, 4096, 
              _T("INSERT INTO object_properties (object_id,guid,name,")
              _T("status,is_deleted,is_system,inherit_access_rights,")
              _T("last_modified,status_calc_alg,status_prop_alg,")
              _T("status_fixed_val,status_shift,status_translation,")
              _T("status_single_threshold,status_thresholds,location_type,")
				  _T("latitude,longitude,image,comments) VALUES ")
              _T("(%d,'%s',%s,0,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
				  _T("'0.000000','0.000000','00000000-0000-0000-0000-000000000000',%s)"),
				  (int)id, uuid_to_string(guid, guidText), 
				  DBPrepareString(g_hCoreDB, name), 
				  TIME_T_FCAST(time(NULL)),
				  DBPrepareString(g_hCoreDB, description, 2048));
	if (!SQLQuery(query))
		return FALSE;


}


BOOL MigrateMaps()
{
	DB_RESULT hResult = SQLSelect(_T("SELECT map_id,map_name,description,root_object_id FROM maps"));
	if (hResult == NULL)
		return FALSE;

	BOOL success = FALSE;
	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		TCHAR name[256];

		DBGetField(hResult, i, 1, name, 256);
		DWORD newId = CreateMapObject(name);
		if (newId == 0)
			goto cleanup;
	}

	success = TRUE;

cleanup:
	DBFreeResult(hResult);
	return success;
}
