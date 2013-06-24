/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2012 Victor Kirhenshtein
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
** File: check.cpp
**
**/

#include "nxdbmgr.h"


//
// Static data
//

static int m_iNumErrors = 0;
static int m_iNumFixes = 0;
static int m_iStageErrors;
static int m_iStageFixes;
static TCHAR *m_pszStageMsg = NULL;


//
// Start stage
//

static void StartStage(const TCHAR *pszMsg)
{
   if (pszMsg != NULL)
   {
      safe_free(m_pszStageMsg);
      m_pszStageMsg = _tcsdup(pszMsg);
   }
   WriteToTerminalEx(_T("\x1b[1m*\x1b[0m %-67s"), m_pszStageMsg, stdout);
#ifndef _WIN32
   fflush(stdout);
#endif
   m_iStageErrors = m_iNumErrors;
   m_iStageFixes = m_iNumFixes;
}


//
// End stage
//

static void EndStage()
{
   static const TCHAR *pszStatus[] = { _T("PASSED"), _T("FIXED "), _T("ERROR ") };
   static int nColor[] = { 32, 33, 31 };
   int nCode, nErrors;

   nErrors = m_iNumErrors - m_iStageErrors;
   if (nErrors > 0)
   {
      nCode = (m_iNumFixes - m_iStageFixes == nErrors) ? 1 : 2;
      StartStage(NULL); // redisplay stage message
   }
   else
   {
      nCode = 0;
   }
   WriteToTerminalEx(_T(" \x1b[37;1m[\x1b[%d;1m%s\x1b[37;1m]\x1b[0m\n"), nColor[nCode], pszStatus[nCode]);
}


//
// Get object name from object_properties table
//

static TCHAR *GetObjectName(DWORD dwId, TCHAR *pszBuffer)
{
	TCHAR szQuery[256];
	DB_RESULT hResult;

	_sntprintf(szQuery, 256, _T("SELECT name FROM object_properties WHERE object_id=%d"), dwId);
	hResult = SQLSelect(szQuery);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			DBGetField(hResult, 0, 0, pszBuffer, MAX_OBJECT_NAME);
		}
		else
		{
			_tcscpy(pszBuffer, _T("<unknown>"));
		}
	}
	else
	{
		_tcscpy(pszBuffer, _T("<unknown>"));
	}
	return pszBuffer;
}


//
// Check that given node is inside at least one container or cluster
//

static bool NodeInContainer(DWORD id)
{
	TCHAR query[256];
	DB_RESULT hResult;
	bool result = false;

	_sntprintf(query, 256, _T("SELECT container_id FROM container_members WHERE object_id=%d"), id);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		result = (DBGetNumRows(hResult) > 0);
		DBFreeResult(hResult);
	}

	if (!result)
	{
		_sntprintf(query, 256, _T("SELECT cluster_id FROM cluster_members WHERE node_id=%d"), id);
		hResult = SQLSelect(query);
		if (hResult != NULL)
		{
			result = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
	}

	return result;
}


//
// Find subnet for unlinked node
//

static BOOL FindSubnetForNode(DWORD id, const TCHAR *name)
{
	DB_RESULT hResult, hResult2;
	TCHAR query[256], buffer[32];
	int i, count;
	DWORD addr, mask, subnet;
	BOOL success = FALSE;

	// Read list of interfaces of given node
	_sntprintf(query, 256, _T("SELECT ip_addr,ip_netmask FROM interfaces WHERE node_id=%d"), id);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		count = DBGetNumRows(hResult);
		for(i = 0; i < count; i++)
		{
			addr = DBGetFieldIPAddr(hResult, i, 0);
			mask = DBGetFieldIPAddr(hResult, i, 1);
			subnet = addr & mask;
			
			_sntprintf(query, 256, _T("SELECT id FROM subnets WHERE ip_addr='%s'"), IpToStr(subnet, buffer));
			hResult2 = SQLSelect(query);
			if (hResult2 != NULL)
			{
				if (DBGetNumRows(hResult2) > 0)
				{
					subnet = DBGetFieldULong(hResult2, 0, 0);
					m_iNumErrors++;
					if (GetYesNo(_T("\rUnlinked node object %d (\"%s\") can be linked to subnet %d (%s). Link?"), id, name, subnet, buffer))
					{
						_sntprintf(query, 256, _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), subnet, id);
						if (SQLQuery(query))
						{
							success = TRUE;
							m_iNumFixes++;
							break;
						}
						else
						{
							// Node remains unlinked, so error count will be
							// incremented again by node deletion code or next iteration
							m_iNumErrors--;
						}
					}
					else
					{
						// Node remains unlinked, so error count will be
						// incremented again by node deletion code
						m_iNumErrors--;
					}
				}
				DBFreeResult(hResult2);
			}
		}
		DBFreeResult(hResult);
	}
	return success;
}


//
// Check zone objects
//

static void CheckZones()
{
   DB_RESULT hResult, hResult2;
   DWORD i, dwNumObjects, dwId;
   TCHAR szQuery[1024], szName[MAX_OBJECT_NAME];
   BOOL bIsDeleted;

   StartStage(_T("Checking zone objects..."));
   hResult = SQLSelect(_T("SELECT id FROM zones"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);

         // Check appropriate record in object_properties table
         _sntprintf(szQuery, 256, _T("SELECT name,is_deleted FROM object_properties WHERE object_id=%d"), (int)dwId);
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if ((DBGetNumRows(hResult2) == 0) && (dwId != 4))	// Properties for built-in zone can be missing
            {
               m_iNumErrors++;
               if (GetYesNo(_T("\rMissing zone object %d properties. Create?"), dwId))
               {
						uuid_t guid;
						TCHAR guidText[128];

						uuid_generate(guid);
                  _sntprintf(szQuery, 1024, 
                             _T("INSERT INTO object_properties (object_id,guid,name,")
                             _T("status,is_deleted,is_system,inherit_access_rights,")
                             _T("last_modified,status_calc_alg,status_prop_alg,")
                             _T("status_fixed_val,status_shift,status_translation,")
                             _T("status_single_threshold,status_thresholds,location_type,")
									  _T("latitude,longitude,image) VALUES ")
                             _T("(%d,'%s','lost_zone_%d',5,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
									  _T("'0.000000','0.000000','00000000-0000-0000-0000-000000000000')"),
									  (int)dwId, uuid_to_string(guid, guidText), (int)dwId, TIME_T_FCAST(time(NULL)));
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
               }
            }
            else
            {
               DBGetField(hResult2, 0, 0, szName, MAX_OBJECT_NAME);
               bIsDeleted = DBGetFieldLong(hResult2, 0, 1) ? TRUE : FALSE;
            }
            DBFreeResult(hResult2);
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}


//
// Check node objects
//

static void CheckNodes()
{
   DB_RESULT hResult, hResult2;
   DWORD i, dwNumObjects, dwId;
   TCHAR szQuery[1024], szName[MAX_OBJECT_NAME];
   BOOL bResult, bIsDeleted;

   StartStage(_T("Checking node objects..."));
   hResult = SQLSelect(_T("SELECT id,primary_ip FROM nodes"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);

         // Check appropriate record in object_properties table
         _sntprintf(szQuery, 256, _T("SELECT name,is_deleted FROM object_properties WHERE object_id=%d"), dwId);
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               if (GetYesNo(_T("\rMissing node object %d properties. Create?"), dwId))
               {
						uuid_t guid;
						TCHAR guidText[128];

						uuid_generate(guid);
                  _sntprintf(szQuery, 1024, 
                             _T("INSERT INTO object_properties (object_id,guid,name,")
                             _T("status,is_deleted,is_system,inherit_access_rights,")
                             _T("last_modified,status_calc_alg,status_prop_alg,")
                             _T("status_fixed_val,status_shift,status_translation,")
                             _T("status_single_threshold,status_thresholds,location_type,")
									  _T("latitude,longitude,location_accuracy,location_timestamp,image,submap_id) VALUES ")
                             _T("(%d,'%s','lost_node_%d',5,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
									  _T("'0.000000','0.000000',0,0,'00000000-0000-0000-0000-000000000000',0)"),
									  (int)dwId, uuid_to_string(guid, guidText), (int)dwId, TIME_T_FCAST(time(NULL)));
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
               }
            }
            else
            {
               DBGetField(hResult2, 0, 0, szName, MAX_OBJECT_NAME);
               bIsDeleted = DBGetFieldLong(hResult2, 0, 1) ? TRUE : FALSE;
            }
            DBFreeResult(hResult2);
         }

         if (!bIsDeleted)
         {
            _sntprintf(szQuery, 1024, _T("SELECT subnet_id FROM nsmap WHERE node_id=%d"), dwId);
            hResult2 = SQLSelect(szQuery);
            if (hResult2 != NULL)
            {
               if ((DBGetNumRows(hResult2) == 0) && (!NodeInContainer(dwId)))
               {
						if ((DBGetFieldIPAddr(hResult, i, 1) == 0) || (!FindSubnetForNode(dwId, szName)))
						{
							m_iNumErrors++;
							if (GetYesNo(_T("\rUnlinked node object %d (\"%s\"). Delete it?"), dwId, szName))
							{
								_sntprintf(szQuery, 1024, _T("DELETE FROM nodes WHERE id=%d"), dwId);
								bResult = SQLQuery(szQuery);
								_sntprintf(szQuery, 1024, _T("DELETE FROM acl WHERE object_id=%d"), dwId);
								bResult = bResult && SQLQuery(szQuery);
								_sntprintf(szQuery, 1024, _T("DELETE FROM object_properties WHERE object_id=%d"), dwId);
								if (SQLQuery(szQuery) && bResult)
									m_iNumFixes++;
							}
						}
               }
               DBFreeResult(hResult2);
            }
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}


//
// Check if node exists
//

BOOL IsNodeExist(DWORD dwId)
{
	TCHAR szQuery[256];
	DB_RESULT hResult;
	BOOL bRet = FALSE;

   _sntprintf(szQuery, 256, _T("SELECT id FROM nodes WHERE id=%d"), dwId);
   hResult = SQLSelect(szQuery);
   if (hResult != NULL)
   {
      bRet = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
	return bRet;
}


//
// Check node component objects
//

static void CheckComponents(const TCHAR *pszDisplayName, const TCHAR *pszTable)
{
   DB_RESULT hResult, hResult2;
   DWORD i, dwNumObjects, dwId;
   TCHAR szQuery[1024], szName[MAX_OBJECT_NAME];
   BOOL bIsDeleted;

   _sntprintf(szQuery, 1024, _T("Checking %s objects..."), pszDisplayName);
   StartStage(szQuery);

   _sntprintf(szQuery, 1024, _T("SELECT id,node_id FROM %s"), pszTable);
   hResult = SQLSelect(szQuery);
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);

         // Check appropriate record in object_properties table
         _sntprintf(szQuery, 1024, _T("SELECT name,is_deleted FROM object_properties WHERE object_id=%d"), dwId);
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               if (GetYesNo(_T("\rMissing %s object %d properties. Create?"), pszDisplayName, dwId))
               {
						uuid_t guid;
						TCHAR guidText[128];

						uuid_generate(guid);
                  _sntprintf(szQuery, 1024, 
                             _T("INSERT INTO object_properties (object_id,guid,name,")
                             _T("status,is_deleted,is_system,inherit_access_rights,")
                             _T("last_modified,status_calc_alg,status_prop_alg,")
                             _T("status_fixed_val,status_shift,status_translation,")
                             _T("status_single_threshold,status_thresholds,location_type,")
									  _T("latitude,longitude,image) VALUES ")
                             _T("(%d,'%s','lost_%s_%d',5,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
									  _T("'0.000000','0.000000','00000000-0000-0000-0000-000000000000')"),
									  (int)dwId, uuid_to_string(guid, guidText), pszDisplayName, (int)dwId, TIME_T_FCAST(time(NULL)));
                  if (SQLQuery(szQuery))
                     m_iNumFixes++;
                  szName[0] = 0;
               }
            }
            else
            {
               DBGetField(hResult2, 0, 0, szName, MAX_OBJECT_NAME);
               bIsDeleted = DBGetFieldLong(hResult2, 0, 1) ? TRUE : FALSE;
            }
            DBFreeResult(hResult2);
         }
         else
         {
            szName[0] = 0;
         }

         // Check if referred node exists
         _sntprintf(szQuery, 256, _T("SELECT name FROM object_properties WHERE object_id=%d AND is_deleted=0"),
                    DBGetFieldULong(hResult, i, 1));
         hResult2 = SQLSelect(szQuery);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               dwId = DBGetFieldULong(hResult, i, 0);
               if (GetYesNo(_T("\rUnlinked %s object %d (\"%s\"). Delete it?"), pszDisplayName, dwId, szName))
               {
                  _sntprintf(szQuery, 256, _T("DELETE FROM %s WHERE id=%d"), pszTable, dwId);
                  if (SQLQuery(szQuery))
                  {
                     _sntprintf(szQuery, 256, _T("DELETE FROM object_properties WHERE object_id=%d"), dwId);
                     SQLQuery(szQuery);
                     m_iNumFixes++;
                  }
               }
            }
            DBFreeResult(hResult2);
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}


//
// Check common object properties
//

static void CheckObjectProperties()
{
   DB_RESULT hResult;
   TCHAR szQuery[1024];
   DWORD i, dwNumRows, dwObjectId;

   StartStage(_T("Checking object properties..."));
   hResult = SQLSelect(_T("SELECT object_id,name,last_modified FROM object_properties"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);

         // Check last change time
         if (DBGetFieldULong(hResult, i, 2) == 0)
         {
            m_iNumErrors++;
            if (GetYesNo(_T("\rObject %d [%s] has invalid timestamp. Fix it?"),
				             dwObjectId, DBGetField(hResult, i, 1, szQuery, 1024)))
            {
               _sntprintf(szQuery, 1024, _T("UPDATE object_properties SET last_modified=") TIME_T_FMT _T(" WHERE object_id=%d"),
                          TIME_T_FCAST(time(NULL)), (int)dwObjectId);
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}


//
// Check cluster objects
//

static void CheckClusters()
{
   DB_RESULT hResult;
   TCHAR szQuery[256], szName[MAX_OBJECT_NAME];
   DWORD i, dwNumRows, dwObjectId, dwId;

   StartStage(_T("Checking cluster objects..."));
   hResult = SQLSelect(_T("SELECT cluster_id,node_id FROM cluster_members"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 1);
			if (!IsNodeExist(dwObjectId))
			{
            m_iNumErrors++;
            dwId = DBGetFieldULong(hResult, i, 0);
            if (GetYesNo(_T("\rCluster object %s [%d] refers to non-existing node %d. Dereference?"),
				             GetObjectName(dwId, szName), dwId, dwObjectId))
            {
               _sntprintf(szQuery, 256, _T("DELETE FROM cluster_members WHERE cluster_id=%d AND node_id=%d"),dwId, dwObjectId);
               if (SQLQuery(szQuery))
               {
                  m_iNumFixes++;
               }
            }
			}
      }
      DBFreeResult(hResult);
   }
   EndStage();
}


//
// Returns TRUE if SELECT returns non-empty set
//

static BOOL CheckResultSet(TCHAR *pszQuery)
{
   DB_RESULT hResult;
   BOOL bResult = FALSE;

   hResult = SQLSelect(pszQuery);
   if (hResult != NULL)
   {
      bResult = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   return bResult;
}


//
// Check event processing policy
//

static void CheckEPP()
{
   DB_RESULT hResult;
   TCHAR szQuery[1024];
   int i, iNumRows;
   DWORD dwId;

   StartStage(_T("Checking event processing policy..."));
   
   // Check source object ID's
   hResult = SQLSelect(_T("SELECT object_id FROM policy_source_list"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         _sntprintf(szQuery, 1024, _T("SELECT object_id FROM object_properties WHERE object_id=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            if (GetYesNo(_T("\rInvalid object ID %d used in policy. Delete it from policy?"), dwId))
            {
               _sntprintf(szQuery, 1024, _T("DELETE FROM policy_source_list WHERE object_id=%d"), dwId);
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   // Check event ID's
   hResult = SQLSelect(_T("SELECT event_code FROM policy_event_list"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         if (dwId & GROUP_FLAG)
            _sntprintf(szQuery, 1024, _T("SELECT id FROM event_groups WHERE id=%d"), dwId);
         else
            _sntprintf(szQuery, 1024, _T("SELECT event_code FROM event_cfg WHERE event_code=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            if (GetYesNo(_T("\rInvalid event%s ID 0x%08X referenced in policy. Delete this reference?"), (dwId & GROUP_FLAG) ? _T(" group") : _T(""), dwId))
            {
               _sntprintf(szQuery, 1024, _T("DELETE FROM policy_event_list WHERE event_code=%d"), dwId);
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   // Check action ID's
   hResult = SQLSelect(_T("SELECT action_id FROM policy_action_list"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         _sntprintf(szQuery, 1024, _T("SELECT action_id FROM actions WHERE action_id=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            if (GetYesNo(_T("\rInvalid action ID %d referenced in policy. Delete this reference?"), dwId))
            {
               _sntprintf(szQuery, 1024, _T("DELETE FROM policy_action_list WHERE action_id=%d"), dwId);
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Create idata_xx table
 */
BOOL CreateIDataTable(DWORD nodeId)
{
   TCHAR szQuery[256], szQueryTemplate[256];
   DWORD i;

   MetaDataReadStr(_T("IDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
   _sntprintf(szQuery, 256, szQueryTemplate, nodeId);
   if (!SQLQuery(szQuery))
		return FALSE;

   for(i = 0; i < 10; i++)
   {
      _sntprintf(szQuery, 256, _T("IDataIndexCreationCommand_%d"), i);
      MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
      if (szQueryTemplate[0] != 0)
      {
         _sntprintf(szQuery, 256, szQueryTemplate, nodeId, nodeId);
         if (!SQLQuery(szQuery))
				return FALSE;
      }
   }

	return TRUE;
}

/**
 * Create tdata_xx table - pre V281 version
 */
BOOL CreateTDataTable_preV281(DWORD nodeId)
{
   TCHAR szQuery[256], szQueryTemplate[256];
   DWORD i;

   MetaDataReadStr(_T("TDataTableCreationCommand"), szQueryTemplate, 255, _T(""));
   _sntprintf(szQuery, 256, szQueryTemplate, nodeId);
   if (!SQLQuery(szQuery))
		return FALSE;

   for(i = 0; i < 10; i++)
   {
      _sntprintf(szQuery, 256, _T("TDataIndexCreationCommand_%d"), i);
      MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
      if (szQueryTemplate[0] != 0)
      {
         _sntprintf(szQuery, 256, szQueryTemplate, nodeId, nodeId);
         if (!SQLQuery(szQuery))
				return FALSE;
      }
   }

	return TRUE;
}

/**
 * Create tdata_*_xx tables
 */
BOOL CreateTDataTables(DWORD nodeId)
{
   TCHAR szQuery[256], szQueryTemplate[256];
   DWORD i;

   for(i = 0; i < 10; i++)
   {
      _sntprintf(szQuery, 256, _T("TDataTableCreationCommand_%d"), i);
      MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
      if (szQueryTemplate[0] != 0)
      {
         _sntprintf(szQuery, 256, szQueryTemplate, nodeId);
         if (!SQLQuery(szQuery))
		      return FALSE;
      }
   }

   for(i = 0; i < 10; i++)
   {
      _sntprintf(szQuery, 256, _T("TDataIndexCreationCommand_%d"), i);
      MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
      if (szQueryTemplate[0] != 0)
      {
         _sntprintf(szQuery, 256, szQueryTemplate, nodeId, nodeId);
         if (!SQLQuery(szQuery))
				return FALSE;
      }
   }

	return TRUE;
}

/**
 * Check collected data
 */
static void CheckIData()
{
	int i, nodeCount;
	time_t now;
	DWORD nodeId;
	TCHAR query[1024];
	DB_RESULT hResultNodes, hResult;

   StartStage(_T("Checking collected data..."));
   
	now = time(NULL);
   hResultNodes = SQLSelect(_T("SELECT id FROM nodes"));
   if (hResultNodes != NULL)
   {
		nodeCount = DBGetNumRows(hResultNodes);
		for(i = 0; i < nodeCount; i++)
		{
			nodeId = DBGetFieldULong(hResultNodes, i, 0);
			_sntprintf(query, 1024, _T("SELECT count(*) FROM idata_%d WHERE idata_timestamp>") TIME_T_FMT, nodeId, TIME_T_FCAST(now));
			hResult = SQLSelect(query);
			if (hResult != NULL)
			{
				if (DBGetFieldLong(hResult, 0, 0) > 0)
				{
					m_iNumErrors++;
					if (GetYesNo(_T("\rFound collected data for node [%d] with timestamp in the future. Delete them?"), nodeId))
					{
						_sntprintf(query, 1024, _T("DELETE FROM idata_%d WHERE idata_timestamp>") TIME_T_FMT, nodeId, TIME_T_FCAST(now));
						if (SQLQuery(query))
							m_iNumFixes++;
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
/*				m_iNumErrors++;
				_tprintf(_T("\rData collection table for node [%d] not found. Create? (Y/N) "), nodeId);
				if (GetYesNo())
				{
					if (CreateIDataTable(nodeId))
						m_iNumFixes++;
				}*/
			}
		}
		DBFreeResult(hResultNodes);
	}

	_sntprintf(query, 1024, _T("SELECT count(*) FROM raw_dci_values WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now));
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		if (DBGetFieldLong(hResult, 0, 0) > 0)
		{
			m_iNumErrors++;
			if (GetYesNo(_T("\rFound DCIs with last poll timestamp in the future. Fix it?")))
			{
				_sntprintf(query, 1024, _T("UPDATE raw_dci_values SET last_poll_time=") TIME_T_FMT _T(" WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now), TIME_T_FCAST(now));
				if (SQLQuery(query))
					m_iNumFixes++;
			}
		}
		DBFreeResult(hResult);
	}

	EndStage();
}

/**
 * Check template to node mapping
 */
static void CheckTemplateNodeMapping()
{
   DB_RESULT hResult;
   TCHAR name[256], query[256];
   DWORD i, dwNumRows, dwTemplateId, dwNodeId;

   StartStage(_T("Checking template to node mapping..."));
   hResult = SQLSelect(_T("SELECT template_id,node_id FROM dct_node_map ORDER BY template_id"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         dwTemplateId = DBGetFieldULong(hResult, i, 0);
         dwNodeId = DBGetFieldULong(hResult, i, 1);

         // Check node existence
         if (!IsNodeExist(dwNodeId))
         {
            m_iNumErrors++;
				GetObjectName(dwTemplateId, name);
            if (GetYesNo(_T("\rTemplate %d [%s] mapped to non-existent node %d. Delete this mapping?"), dwTemplateId, name, dwNodeId))
            {
               _sntprintf(query, 256, _T("DELETE FROM dct_node_map WHERE template_id=%d AND node_id=%d"),
                          dwTemplateId, dwNodeId);
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check database for errors
 */
void CheckDatabase()
{
   DB_RESULT hResult;
   LONG iVersion = 0;
   BOOL bCompleted = FALSE;

	_tprintf(_T("Checking database (%s collected data):\n"), g_checkData ? _T("including") : _T("excluding"));

   // Get database format version
   iVersion = DBGetSchemaVersion(g_hCoreDB);
   if (iVersion < DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               iVersion, DB_FORMAT_VERSION);
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n")
                _T("You need to upgrade your server before using this database.\n"),
                iVersion, DB_FORMAT_VERSION);

   }
   else
   {
      TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];
      BOOL bLocked;

      // Check if database is locked
      hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, szLockStatus, MAX_DB_STRING);
            DecodeSQLString(szLockStatus);
            bLocked = _tcscmp(szLockStatus, _T("UNLOCKED"));
         }
         DBFreeResult(hResult);

         if (bLocked)
         {
            hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockInfo'"));
            if (hResult != NULL)
            {
               if (DBGetNumRows(hResult) > 0)
               {
                  DBGetField(hResult, 0, 0, szLockInfo, MAX_DB_STRING);
                  DecodeSQLString(szLockInfo);
               }
               DBFreeResult(hResult);
            }
         }

			if (bLocked)
			{
				if (GetYesNo(_T("Database is locked by server %s [%s]\nDo you wish to force database unlock?"), szLockStatus, szLockInfo))
				{
					if (SQLQuery(_T("UPDATE config SET var_value='UNLOCKED' where var_name='DBLockStatus'")))
					{
						bLocked = FALSE;
						_tprintf(_T("Database lock removed\n"));
					}
				}
			}

			if (!bLocked)
			{
				DBBegin(g_hCoreDB);

				CheckZones();
				CheckNodes();
				CheckComponents(_T("interface"), _T("interfaces"));
				CheckComponents(_T("network service"), _T("network_services"));
				CheckClusters();
				CheckTemplateNodeMapping();
				CheckObjectProperties();
				CheckEPP();
				if (g_checkData)
					CheckIData();

				if (m_iNumErrors == 0)
				{
					_tprintf(_T("Database doesn't contain any errors\n"));
					DBCommit(g_hCoreDB);
				}
				else
				{
					_tprintf(_T("%d errors was found, %d errors was corrected\n"), m_iNumErrors, m_iNumFixes);
					if (m_iNumFixes == m_iNumErrors)
						_tprintf(_T("All errors in database was fixed\n"));
					else
						_tprintf(_T("Database still contain errors\n"));
					if (m_iNumFixes > 0)
					{
						if (GetYesNo(_T("Commit changes?")))
						{
							_tprintf(_T("Committing changes...\n"));
							if (DBCommit(g_hCoreDB))
								_tprintf(_T("Changes was successfully committed to database\n"));
						}
						else
						{
							_tprintf(_T("Rolling back changes...\n"));
							if (DBRollback(g_hCoreDB))
								_tprintf(_T("All changes made to database was cancelled\n"));
						}
					}
					else
					{
						DBRollback(g_hCoreDB);
					}
				}
				bCompleted = TRUE;
			}
      }
		else
		{
			_tprintf(_T("Unable to get database lock status\n"));
		}
   }

   _tprintf(_T("Database check %s\n"), bCompleted ? _T("completed") : _T("aborted"));
}
