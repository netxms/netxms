/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
static int s_stageErrorsUpdate;
static int m_iStageFixes;
static TCHAR *m_pszStageMsg = NULL;
static int s_stageWorkTotal = 0;
static int s_stageWorkDone = 0;

/**
 * Start stage
 */
static void StartStage(const TCHAR *pszMsg, int workTotal = 1)
{
   if (pszMsg != NULL)
   {
      free(m_pszStageMsg);
      m_pszStageMsg = _tcsdup(pszMsg);
      m_iStageErrors = m_iNumErrors;
      s_stageErrorsUpdate = m_iNumErrors;
      m_iStageFixes = m_iNumFixes;
      s_stageWorkTotal = workTotal;
      s_stageWorkDone = 0;
      ResetBulkYesNo();
   }
   WriteToTerminalEx(_T("\x1b[1m*\x1b[0m %-67s  \x1b[37;1m[\x1b[0m   0%% \x1b[37;1m]\x1b[0m\b\b\b"), m_pszStageMsg);
#ifndef _WIN32
   fflush(stdout);
#endif
   SetOperationInProgress(true);
}

/**
 * Set total work for stage
 */
static void SetStageWorkTotal(int workTotal)
{
   s_stageWorkTotal = workTotal;
   if (s_stageWorkDone > s_stageWorkTotal)
      s_stageWorkDone = s_stageWorkTotal;
}

/**
 * Update stage progress
 */
static void UpdateStageProgress(int installment)
{
   if (m_iNumErrors - s_stageErrorsUpdate > 0)
   {
      StartStage(NULL); // redisplay stage message
      s_stageErrorsUpdate = m_iNumErrors;
   }

   s_stageWorkDone += installment;
   if (s_stageWorkDone > s_stageWorkTotal)
      s_stageWorkDone = s_stageWorkTotal;
   WriteToTerminalEx(_T("\b\b\b%3d"), s_stageWorkDone * 100 / s_stageWorkTotal);
#ifndef _WIN32
   fflush(stdout);
#endif
}

/**
 * End stage
 */
static void EndStage()
{
   static const TCHAR *pszStatus[] = { _T("PASSED"), _T("FIXED "), _T("ERROR ") };
   static int nColor[] = { 32, 33, 31 };
   int nCode, nErrors;

   nErrors = m_iNumErrors - m_iStageErrors;
   if (nErrors > 0)
   {
      nCode = (m_iNumFixes - m_iStageFixes == nErrors) ? 1 : 2;
      if (m_iNumErrors - s_stageErrorsUpdate > 0)
         StartStage(NULL); // redisplay stage message
   }
   else
   {
      nCode = 0;
   }
   WriteToTerminalEx(_T("\b\b\b\b\b\x1b[37;1m[\x1b[%d;1m%s\x1b[37;1m]\x1b[0m\n"), nColor[nCode], pszStatus[nCode]);
   SetOperationInProgress(false);
   ResetBulkYesNo();
}

/**
 * Get object name from object_properties table
 */
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

/**
 * Check that given node is inside at least one container or cluster
 */
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

/**
 * Find subnet for unlinked node
 */
static BOOL FindSubnetForNode(DWORD id, const TCHAR *name)
{
	DB_RESULT hResult, hResult2;
	TCHAR query[256], buffer[32];
	int i, count;
	BOOL success = FALSE;

	// Read list of interfaces of given node
	_sntprintf(query, 256, _T("SELECT l.ip_addr,l.ip_netmask FROM interfaces i INNER JOIN interface_address_list l ON l.iface_id = i.id WHERE node_id=%d"), id);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		count = DBGetNumRows(hResult);
		for(i = 0; i < count; i++)
		{
			InetAddress addr = DBGetFieldInetAddr(hResult, i, 0);
         addr.setMaskBits(DBGetFieldLong(hResult, i, 1));
         InetAddress subnet = addr.getSubnetAddress();

         _sntprintf(query, 256, _T("SELECT id FROM subnets WHERE ip_addr='%s'"), subnet.toString(buffer));
			hResult2 = SQLSelect(query);
			if (hResult2 != NULL)
			{
				if (DBGetNumRows(hResult2) > 0)
				{
					UINT32 subnetId = DBGetFieldULong(hResult2, 0, 0);
					m_iNumErrors++;
					if (GetYesNoEx(_T("Unlinked node object %d (\"%s\") can be linked to subnet %d (%s). Link?"), id, name, subnetId, buffer))
					{
						_sntprintf(query, 256, _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), subnetId, id);
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

/**
 * Check missing object properties
 */
static void CheckMissingObjectProperties(const TCHAR *table, const TCHAR *className, UINT32 builtinObjectId)
{
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT o.id FROM %s o LEFT OUTER JOIN object_properties p ON p.object_id = o.id WHERE p.name IS NULL"), table);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      UINT32 id = DBGetFieldULong(hResult, i, 0);
      if (id == builtinObjectId)
         continue;
      m_iNumErrors++;
      if (GetYesNoEx(_T("Missing %s object %d properties. Create?"), className, id))
      {
         uuid_t guid;
         _uuid_generate(guid);

         TCHAR guidText[128];
         _sntprintf(query, 1024,
                    _T("INSERT INTO object_properties (object_id,guid,name,")
                    _T("status,is_deleted,is_system,inherit_access_rights,")
                    _T("last_modified,status_calc_alg,status_prop_alg,")
                    _T("status_fixed_val,status_shift,status_translation,")
                    _T("status_single_threshold,status_thresholds,location_type,")
                    _T("latitude,longitude,location_accuracy,location_timestamp,image,submap_id,maint_mode,maint_event_id) VALUES ")
                    _T("(%d,'%s','lost_%s_%d',5,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
                    _T("'0.000000','0.000000',0,0,'00000000-0000-0000-0000-000000000000',0,'0',0)"),
                    (int)id, _uuid_to_string(guid, guidText), className, (int)id, TIME_T_FCAST(time(NULL)));
         if (SQLQuery(query))
            m_iNumFixes++;
      }
   }
   DBFreeResult(hResult);
}

/**
 * Check zone objects
 */
static void CheckZones()
{
   StartStage(_T("Checking zone object properties"));
   CheckMissingObjectProperties(_T("zones"), _T("zone"), 4);
   EndStage();
}

/**
 * Check node objects
 */
static void CheckNodes()
{
   StartStage(_T("Checking node object properties"));
   CheckMissingObjectProperties(_T("nodes"), _T("node"), 0);
   EndStage();

   StartStage(_T("Checking node to subnet bindings"));
   DB_RESULT hResult = SQLSelect(_T("SELECT n.id,p.name FROM nodes n INNER JOIN object_properties p ON p.object_id = n.id WHERE p.is_deleted=0"));
   int count = DBGetNumRows(hResult);
   SetStageWorkTotal(count);
   for(int i = 0; i < count; i++)
   {
      UINT32 nodeId = DBGetFieldULong(hResult, i, 0);

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT subnet_id FROM nsmap WHERE node_id=%d"), nodeId);
      DB_RESULT hResult2 = SQLSelect(query);
      if (hResult2 != NULL)
      {
         if ((DBGetNumRows(hResult2) == 0) && (!NodeInContainer(nodeId)))
         {
            TCHAR nodeName[MAX_OBJECT_NAME];
            DBGetField(hResult, i, 1, nodeName, MAX_OBJECT_NAME);
            if ((DBGetFieldIPAddr(hResult, i, 1) == 0) || (!FindSubnetForNode(nodeId, nodeName)))
            {
               m_iNumErrors++;
               if (GetYesNoEx(_T("Unlinked node object %d (\"%s\"). Delete it?"), nodeId, nodeName))
               {
                  _sntprintf(query, 1024, _T("DELETE FROM nodes WHERE id=%d"), nodeId);
                  bool success = SQLQuery(query);
                  _sntprintf(query, 1024, _T("DELETE FROM acl WHERE object_id=%d"), nodeId);
                  success = success && SQLQuery(query);
                  _sntprintf(query, 1024, _T("DELETE FROM object_properties WHERE object_id=%d"), nodeId);
                  if (success && SQLQuery(query))
                     m_iNumFixes++;
               }
            }
         }
         DBFreeResult(hResult2);
      }
      UpdateStageProgress(1);
   }
   DBFreeResult(hResult);
   EndStage();
}

/**
 * Check node component objects
 */
static void CheckComponents(const TCHAR *pszDisplayName, const TCHAR *pszTable)
{
   TCHAR stageName[256];
   _sntprintf(stageName, 256, _T("Checking %s object properties"), pszDisplayName);
   StartStage(stageName);
   CheckMissingObjectProperties(pszTable, pszDisplayName, 0);
   EndStage();

   _sntprintf(stageName, 256, _T("Checking %s bindings"), pszDisplayName);
   StartStage(stageName);

   TCHAR query[256];
   _sntprintf(query, 1024, _T("SELECT id,node_id FROM %s"), pszTable);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         UINT32 objectId = DBGetFieldULong(hResult, i, 0);

         // Check if referred node exists
         _sntprintf(query, 256, _T("SELECT name FROM object_properties WHERE object_id=%d AND is_deleted=0"),
                    DBGetFieldULong(hResult, i, 1));
         DB_RESULT hResult2 = SQLSelect(query);
         if (hResult2 != NULL)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               m_iNumErrors++;
               TCHAR objectName[MAX_OBJECT_NAME];
               if (GetYesNoEx(_T("Unlinked %s object %d (\"%s\"). Delete it?"), pszDisplayName, objectId, GetObjectName(objectId, objectName)))
               {
                  _sntprintf(query, 256, _T("DELETE FROM %s WHERE id=%d"), pszTable, objectId);
                  if (SQLQuery(query))
                  {
                     _sntprintf(query, 256, _T("DELETE FROM object_properties WHERE object_id=%d"), objectId);
                     SQLQuery(query);
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

/**
 * Check common object properties
 */
static void CheckObjectProperties()
{
   StartStage(_T("Checking object properties"));
   DB_RESULT hResult = SQLSelect(_T("SELECT object_id,name,last_modified FROM object_properties"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         UINT32 objectId = DBGetFieldULong(hResult, i, 0);

         // Check last change time
         if (DBGetFieldULong(hResult, i, 2) == 0)
         {
            m_iNumErrors++;
            TCHAR objectName[MAX_OBJECT_NAME];
            if (GetYesNoEx(_T("Object %d [%s] has invalid timestamp. Fix it?"),
				                objectId, DBGetField(hResult, i, 1, objectName, MAX_OBJECT_NAME)))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE object_properties SET last_modified=") TIME_T_FMT _T(" WHERE object_id=%d"),
                          TIME_T_FCAST(time(NULL)), (int)objectId);
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check container membership
 */
static void CheckContainerMembership()
{
   StartStage(_T("Checking container membership"));
   DB_RESULT containerList = SQLSelect(_T("SELECT object_id,container_id FROM container_members"));
   DB_RESULT objectList = SQLSelect(_T("SELECT object_id FROM object_properties"));
   if (containerList != NULL && objectList != NULL)
   {
      int numContainers = DBGetNumRows(containerList);
      int numObjects = DBGetNumRows(objectList);
      bool match = false;
      TCHAR szQuery[1024];

      SetStageWorkTotal(numContainers);
      for(int i = 0; i < numContainers; i++)
      {
         for(int n = 0; n < numObjects; n++)
         {
            if (DBGetFieldULong(containerList, i, 0) == DBGetFieldULong(objectList, n, 0))
            {
               match = true;
               break;
            }
         }
         if (!match)
         {
            m_iNumErrors++;
            if (GetYesNoEx(_T("Container %d contains non-existing child %d. Fix it?"),
                           DBGetFieldULong(containerList, i, 1), DBGetFieldULong(containerList, i, 0)))
            {
               _sntprintf(szQuery, 1024, _T("DELETE FROM container_members WHERE object_id=%d AND container_id=%d"),
                           DBGetFieldULong(containerList, i, 0), DBGetFieldULong(containerList, i, 1));
               if (SQLQuery(szQuery))
                  m_iNumFixes++;
            }
         }
         match = false;
         UpdateStageProgress(1);
      }
      DBFreeResult(containerList);
      DBFreeResult(objectList);
   }
   EndStage();
}

/**
 * Check cluster objects
 */
static void CheckClusters()
{
   StartStage(_T("Checking cluster object properties"));
   CheckMissingObjectProperties(_T("clusters"), _T("cluster"), 0);
   EndStage();

   StartStage(_T("Checking cluster member nodes"));
   DB_RESULT hResult = SQLSelect(_T("SELECT cluster_id,node_id FROM cluster_members"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         UINT32 nodeId = DBGetFieldULong(hResult, i, 1);
			if (!IsDatabaseRecordExist(_T("nodes"), _T("id"), nodeId))
			{
            m_iNumErrors++;
            UINT32 clusterId = DBGetFieldULong(hResult, i, 0);
            TCHAR name[MAX_OBJECT_NAME];
            if (GetYesNoEx(_T("Cluster object %s [%d] refers to non-existing node %d. Dereference?"),
				               GetObjectName(clusterId, name), clusterId, nodeId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM cluster_members WHERE cluster_id=%d AND node_id=%d"), clusterId, nodeId);
               if (SQLQuery(query))
               {
                  m_iNumFixes++;
               }
            }
			}
			UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Returns TRUE if SELECT returns non-empty set
 */
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

/**
 * Check event processing policy
 */
static void CheckEPP()
{
   DB_RESULT hResult;
   TCHAR szQuery[1024];
   int i, iNumRows;
   DWORD dwId;

   StartStage(_T("Checking event processing policy"));

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
            if (GetYesNoEx(_T("Invalid object ID %d used in policy. Delete it from policy?"), dwId))
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
   ResetBulkYesNo();
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
            if (GetYesNoEx(_T("Invalid event%s ID 0x%08X referenced in policy. Delete this reference?"), (dwId & GROUP_FLAG) ? _T(" group") : _T(""), dwId))
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
   ResetBulkYesNo();
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
            if (GetYesNoEx(_T("Invalid action ID %d referenced in policy. Delete this reference?"), dwId))
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
 * Check data tables for given object class
 */
static void CollectObjectIdentifiers(const TCHAR *className, IntegerArray<UINT32> *list)
{
   TCHAR query[1024];
   _sntprintf(query, 256, _T("SELECT id FROM %s"), className);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         list->add(DBGetFieldULong(hResult, i, 0));
      }
      DBFreeResult(hResult);
   }
}

/**
 * Get all data collection targets
 */
IntegerArray<UINT32> *GetDataCollectionTargets()
{
   IntegerArray<UINT32> *list = new IntegerArray<UINT32>(128, 128);
   CollectObjectIdentifiers(_T("nodes"), list);
   CollectObjectIdentifiers(_T("clusters"), list);
   CollectObjectIdentifiers(_T("mobile_devices"), list);
   CollectObjectIdentifiers(_T("access_points"), list);
   CollectObjectIdentifiers(_T("chassis"), list);
   return list;
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
 * Create tdata_xx table
 */
BOOL CreateTDataTable(DWORD nodeId)
{
   TCHAR szQuery[256], szQueryTemplate[256];
   DWORD i;

   for(i = 0; i < 10; i++)
   {
      _sntprintf(szQuery, 256, _T("TDataTableCreationCommand_%d"), i);
      MetaDataReadStr(szQuery, szQueryTemplate, 255, _T(""));
      if (szQueryTemplate[0] != 0)
      {
         _sntprintf(szQuery, 256, szQueryTemplate, nodeId, nodeId);
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
 * Check if DCI exists
 */
static bool IsDciExists(UINT32 dciId, UINT32 nodeId, bool isTable)
{
   TCHAR query[256];
   if (nodeId != 0)
      _sntprintf(query, 256, _T("SELECT count(*) FROM %s WHERE item_id=%d AND node_id=%d"), isTable ? _T("dc_tables") : _T("items"), dciId, nodeId);
   else
      _sntprintf(query, 256, _T("SELECT count(*) FROM %s WHERE item_id=%d"), isTable ? _T("dc_tables") : _T("items"), dciId);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;

   int count = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);
   return count != 0;
}

/**
 * Check collected data
 */
static void CheckCollectedData(bool isTable)
{
   StartStage(isTable ? _T("Checking table DCI history records") : _T("Checking DCI history records"));

	time_t now = time(NULL);
	IntegerArray<UINT32> *targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets->size());
	for(int i = 0; i < targets->size(); i++)
   {
      UINT32 objectId = targets->get(i);
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT count(*) FROM %s_%d WHERE %s_timestamp>") TIME_T_FMT,
               isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         if (DBGetFieldLong(hResult, 0, 0) > 0)
         {
            m_iNumErrors++;
            if (GetYesNoEx(_T("Found collected data for node [%d] with timestamp in the future. Delete invalid records?"), objectId))
            {
               _sntprintf(query, 1024, _T("DELETE FROM %s_%d WHERE %s_timestamp>") TIME_T_FMT,
                        isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         DBFreeResult(hResult);
      }
   }

	ResetBulkYesNo();

   for(int i = 0; i < targets->size(); i++)
   {
      UINT32 objectId = targets->get(i);
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT distinct(item_id) FROM %s_%d"), isTable ? _T("tdata") : _T("idata"), objectId);
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            UINT32 id = DBGetFieldLong(hResult, i, 0);
            if (!IsDciExists(id, objectId, isTable))
            {
               m_iNumErrors++;
               if (GetYesNoEx(_T("Found collected data for non-existing DCI [%d] on node [%d]. Delete invalid records?"), id, objectId))
               {
                  _sntprintf(query, 1024, _T("DELETE FROM %s_%d WHERE item_id=%d"), isTable ? _T("tdata") : _T("idata"), objectId, id);
                  if (SQLQuery(query))
                     m_iNumFixes++;
               }
            }
         }
         DBFreeResult(hResult);
      }

      UpdateStageProgress(1);
   }
	delete targets;

	EndStage();
}

/**
 * Check raw DCI values
 */
static void CheckRawDciValues()
{
   StartStage(_T("Checking raw DCI values table"));

   time_t now = time(NULL);

   DB_RESULT hResult = SQLSelect(_T("SELECT item_id FROM raw_dci_values"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count + 1);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldLong(hResult, i, 0);
         if (!IsDciExists(id, 0, false))
         {
            m_iNumErrors++;
            if (GetYesNoEx(_T("Found raw value record for non-existing DCI [%d]. Delete it?"), id))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM raw_dci_values WHERE item_id=%d"), id);
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   ResetBulkYesNo();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT count(*) FROM raw_dci_values WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now));
   hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         m_iNumErrors++;
         if (GetYesNoEx(_T("Found DCIs with last poll timestamp in the future. Fix it?")))
         {
            _sntprintf(query, 1024, _T("UPDATE raw_dci_values SET last_poll_time=") TIME_T_FMT _T(" WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now), TIME_T_FCAST(now));
            if (SQLQuery(query))
               m_iNumFixes++;
         }
      }
      DBFreeResult(hResult);
   }
   UpdateStageProgress(1);

   EndStage();
}

/**
 * Check thresholds
 */
static void CheckThresholds()
{
   StartStage(_T("Checking DCI thresholds"));

   DB_RESULT hResult = SQLSelect(_T("SELECT threshold_id,item_id FROM thresholds"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         UINT32 dciId = DBGetFieldULong(hResult, i, 1);
         if (!IsDciExists(dciId, 0, false))
         {
            m_iNumErrors++;
            if (GetYesNoEx(_T("Found threshold configuration for non-existing DCI [%d]. Delete?"), dciId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM thresholds WHERE threshold_id=%d AND item_id=%d"), DBGetFieldLong(hResult, i, 0), dciId);
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check thresholds
 */
static void CheckTableThresholds()
{
   StartStage(_T("Checking table DCI thresholds"));

   DB_RESULT hResult = SQLSelect(_T("SELECT id,table_id FROM dct_thresholds"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         UINT32 dciId = DBGetFieldULong(hResult, i, 1);
         if (!IsDciExists(dciId, 0, true))
         {
            m_iNumErrors++;
            if (GetYesNoEx(_T("Found threshold configuration for non-existing table DCI [%d]. Delete?"), dciId))
            {
               UINT32 id = DBGetFieldLong(hResult, i, 0);

               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM dct_threshold_instances WHERE threshold_id=%d"), id);
               if (SQLQuery(query))
               {
                  _sntprintf(query, 256, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=%d"), id);
                  if (SQLQuery(query))
                  {
                     _sntprintf(query, 256, _T("DELETE FROM dct_thresholds WHERE id=%d"), id);
                     if (SQLQuery(query))
                        m_iNumFixes++;
                  }
               }
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check if given data table exist
 */
bool IsDataTableExist(const TCHAR *format, UINT32 id)
{
   TCHAR table[256];
   _sntprintf(table, 256, format, id);
   int rc = DBIsTableExist(g_hCoreDB, table);
   if (rc == DBIsTableExist_Failure)
   {
      _tprintf(_T("WARNING: call to DBIsTableExist(\"%s\") failed\n"), table);
   }
   return rc != DBIsTableExist_NotFound;
}

/**
 * Check data tables
 */
static void CheckDataTables()
{
   StartStage(_T("Checking data tables"));

	IntegerArray<UINT32> *targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets->size());
	for(int i = 0; i < targets->size(); i++)
   {
      UINT32 objectId = targets->get(i);

      // IDATA
      if (!IsDataTableExist(_T("idata_%d"), objectId))
      {
			m_iNumErrors++;

         TCHAR objectName[MAX_OBJECT_NAME];
         GetObjectName(objectId, objectName);
			if (GetYesNoEx(_T("Data collection table (IDATA) for object %s [%d] not found. Create? (Y/N) "), objectName, objectId))
			{
				if (CreateIDataTable(objectId))
					m_iNumFixes++;
			}
      }

      // TDATA
      if (!IsDataTableExist(_T("tdata_%d"), objectId))
      {
			m_iNumErrors++;

         TCHAR objectName[MAX_OBJECT_NAME];
         GetObjectName(objectId, objectName);
			if (GetYesNoEx(_T("Data collection table (TDATA) for %s [%d] not found. Create? (Y/N) "), objectName, objectId))
			{
				if (CreateTDataTable(objectId))
					m_iNumFixes++;
			}
      }

      UpdateStageProgress(1);
   }

   delete targets;
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

   StartStage(_T("Checking template to node mapping"));
   hResult = SQLSelect(_T("SELECT template_id,node_id FROM dct_node_map ORDER BY template_id"));
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      SetStageWorkTotal(dwNumRows);
      for(i = 0; i < dwNumRows; i++)
      {
         dwTemplateId = DBGetFieldULong(hResult, i, 0);
         dwNodeId = DBGetFieldULong(hResult, i, 1);

         // Check node existence
         if (!IsDatabaseRecordExist(_T("nodes"), _T("id"), dwNodeId) &&
             !IsDatabaseRecordExist(_T("clusters"), _T("id"), dwNodeId) &&
             !IsDatabaseRecordExist(_T("mobile_devices"), _T("id"), dwNodeId))
         {
            m_iNumErrors++;
				GetObjectName(dwTemplateId, name);
            if (GetYesNoEx(_T("Template %d [%s] mapped to non-existent node %d. Delete this mapping?"), dwTemplateId, name, dwNodeId))
            {
               _sntprintf(query, 256, _T("DELETE FROM dct_node_map WHERE template_id=%d AND node_id=%d"),
                          dwTemplateId, dwNodeId);
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check network map links
 */
static void CheckMapLinks()
{
   StartStage(_T("Checking network map links"));

   for(int pass = 1; pass <= 2; pass++)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
         _T("SELECT network_map_links.map_id,network_map_links.element1,network_map_links.element2 ")
         _T("FROM network_map_links ")
         _T("LEFT OUTER JOIN network_map_elements ON ")
         _T("   network_map_links.map_id = network_map_elements.map_id AND ")
         _T("   network_map_links.element%d = network_map_elements.element_id ")
         _T("WHERE network_map_elements.element_id IS NULL"), pass);

      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            m_iNumErrors++;
            DWORD mapId = DBGetFieldULong(hResult, i, 0);
            TCHAR name[MAX_OBJECT_NAME];
				GetObjectName(mapId, name);
            if (GetYesNoEx(_T("Invalid link on network map %s [%d]. Delete?"), name, mapId))
            {
               _sntprintf(query, 256, _T("DELETE FROM network_map_links WHERE map_id=%d AND element1=%d AND element2=%d"),
                          mapId, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));
               if (SQLQuery(query))
                  m_iNumFixes++;
            }
         }
         DBFreeResult(hResult);
      }
   }
   EndStage();
}

/**
 * Check database for errors
 */
void CheckDatabase()
{
   if (g_checkDataTablesOnly)
	   _tprintf(_T("Checking database (data tables only):\n"));
   else
	   _tprintf(_T("Checking database (%s collected data):\n"), g_checkData ? _T("including") : _T("excluding"));

   // Get database format version
   INT32 major, minor;
   if (!DBGetSchemaVersion(g_hCoreDB, &major, &minor))
   {
      _tprintf(_T("Unable to determine database schema version\n"));
      _tprintf(_T("Database check aborted\n"));
      return;
   }
   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
       _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       _tprintf(_T("Database check aborted\n"));
       return;
   }
   if ((major < DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR)))
   {
      _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
      _tprintf(_T("Database check aborted\n"));
      return;
   }

   TCHAR szLockStatus[MAX_DB_STRING], szLockInfo[MAX_DB_STRING];
   BOOL bLocked = FALSE;
   BOOL bCompleted = FALSE;

   // Check if database is locked
   DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
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

         if (g_checkDataTablesOnly)
         {
            CheckDataTables();
         }
         else
         {
            CheckZones();
            CheckNodes();
            CheckComponents(_T("interface"), _T("interfaces"));
            CheckComponents(_T("network service"), _T("network_services"));
            CheckClusters();
            CheckTemplateNodeMapping();
            CheckObjectProperties();
            CheckContainerMembership();
            CheckEPP();
            CheckMapLinks();
            CheckDataTables();
            CheckRawDciValues();
            CheckThresholds();
            CheckTableThresholds();
            if (g_checkData)
            {
               CheckCollectedData(false);
               CheckCollectedData(true);
            }
         }

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

   _tprintf(_T("Database check %s\n"), bCompleted ? _T("completed") : _T("aborted"));
}
