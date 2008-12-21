/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
#ifdef _WIN32
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);
   _puttc(_T('*'), stdout);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
   _tprintf(_T(" %-68s"), m_pszStageMsg, stdout);
#else
   _tprintf(_T("* %-68s"), m_pszStageMsg, stdout);
   fflush(stdout);
#endif
   m_iStageErrors = m_iNumErrors;
   m_iStageFixes = m_iNumFixes;
}


//
// End stage
//

static void EndStage(void)
{
   static const TCHAR *pszStatus[] = { _T("PASSED"), _T("FIXED "), _T("ERROR ") };
   static int nColor[] = { 0x0A, 0x0E, 0x0C };
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
#ifdef _WIN32
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);
   _puttc(_T('['), stdout);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), nColor[nCode]);
   _tprintf(_T("%s"), pszStatus[nCode]);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);
   _puttc(_T(']'), stdout);
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
   _tprintf("\n");
#else
   _tprintf(_T("  [%s]\n"), pszStatus[nCode]);
#endif
}


//
// Get object name from object_properties table
//

static TCHAR *GetObjectName(DWORD dwId, TCHAR *pszBuffer)
{
	TCHAR szQuery[256];
	DB_RESULT hResult;

	_stprintf(szQuery, _T("SELECT name FROM object_properties WHERE object_id=%d"), dwId);
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
// Check node objects
//

static void CheckNodes(void)
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
               _tprintf(_T("\rMissing node object %d properties. Create? (Y/N) "), dwId);
               if (GetYesNo())
               {
                  _sntprintf(szQuery, 1024, 
                             _T("INSERT INTO object_properties (object_id,name,"
                                "status,is_deleted,image_id,inherit_access_rights,"
                                "last_modified,status_calc_alg,status_prop_alg,"
                                "status_fixed_val,status_shift,status_translation,"
                                "status_single_threshold,status_thresholds) VALUES "
                                "(%d,'lost_node_%d',5,0,0,1,0,0,0,0,0,0,0,'00000000')"),
                                dwId, dwId);
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
               if ((DBGetNumRows(hResult2) == 0) && (DBGetFieldIPAddr(hResult, i, 1) != 0))
               {
                  m_iNumErrors++;
                  _tprintf(_T("\rUnlinked node object %d (\"%s\"). Delete? (Y/N) "),
                           dwId, szName);
                  if (GetYesNo())
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

static BOOL IsNodeExist(DWORD dwId)
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

   _stprintf(szQuery, _T("Checking %s objects..."), pszDisplayName);
   StartStage(szQuery);

   _stprintf(szQuery, _T("SELECT id,node_id FROM %s"), pszTable);
   hResult = SQLSelect(szQuery);
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
               _tprintf(_T("\rMissing %s object %d properties. Create? (Y/N) "),
                        pszDisplayName, dwId);
               if (GetYesNo())
               {
                  _sntprintf(szQuery, 1024, 
                             _T("INSERT INTO object_properties (object_id,name,"
                                "status,is_deleted,image_id,inherit_access_rights,"
                                "last_modified,status_calc_alg,status_prop_alg,"
                                "status_fixed_val,status_shift,status_translation,"
                                "status_single_threshold,status_thresholds) VALUES "
                                "(%d,'lost_%s_%d',5,0,0,1,0,0,0,0,0,0,0,'00000000')"),
                             dwId, pszDisplayName, dwId);
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
               _tprintf(_T("\rUnlinked %s object %d (\"%s\"). Delete? (Y/N) "),
                        pszDisplayName, dwId, szName);
               if (GetYesNo())
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

static void CheckObjectProperties(void)
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
            _tprintf(_T("\rObject %d [%s] has invalid timestamp. Correct? (Y/N) "),
                     dwObjectId, DBGetField(hResult, i, 1, szQuery, 1024));
            if (GetYesNo())
            {
               _sntprintf(szQuery, 1024, _T("UPDATE object_properties SET last_modified=%ld WHERE object_id=%d"),
                          time(NULL), dwObjectId);
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

static void CheckClusters(void)
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
            _tprintf(_T("\rCluster object %s [%d] refers to non-existing node %d. Dereference? (Y/N) "),
                     GetObjectName(dwId, szName), dwId, dwObjectId);
            if (GetYesNo())
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

static void CheckEPP(void)
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
         _stprintf(szQuery, _T("SELECT object_id FROM object_properties WHERE object_id=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            _tprintf(_T("\rInvalid object ID %d used. Correct? (Y/N) "), dwId);
            if (GetYesNo())
            {
               _stprintf(szQuery, _T("DELETE FROM policy_source_list WHERE object_id=%d"), dwId);
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
            _stprintf(szQuery, _T("SELECT id FROM event_groups WHERE id=%d"), dwId);
         else
            _stprintf(szQuery, _T("SELECT event_code FROM event_cfg WHERE event_code=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            _tprintf(_T("\rInvalid event%s ID 0x%08X used. Correct? (Y/N) "),
                     (dwId & GROUP_FLAG) ? _T(" group") : _T(""), dwId);
            if (GetYesNo())
            {
               _stprintf(szQuery, _T("DELETE FROM policy_event_list WHERE event_code=%d"), dwId);
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
         _stprintf(szQuery, _T("SELECT action_id FROM actions WHERE action_id=%d"), dwId);
         if (!CheckResultSet(szQuery))
         {
            m_iNumErrors++;
            _tprintf(_T("\rInvalid action ID %d used. Correct? (Y/N) "), dwId);
            if (GetYesNo())
            {
               _stprintf(szQuery, _T("DELETE FROM policy_action_list WHERE action_id=%d"), dwId);
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
// Create idata_xx table
//

static BOOL CreateIDataTable(DWORD nodeId)
{
   char szQuery[256], szQueryTemplate[256];
   DWORD i;

   ConfigReadStr("IDataTableCreationCommand", szQueryTemplate, 255, "");
   sprintf(szQuery, szQueryTemplate, nodeId);
   if (!SQLQuery(szQuery))
		return FALSE;

   for(i = 0; i < 10; i++)
   {
      sprintf(szQuery, "IDataIndexCreationCommand_%d", i);
      ConfigReadStr(szQuery, szQueryTemplate, 255, "");
      if (szQueryTemplate[0] != 0)
      {
         sprintf(szQuery, szQueryTemplate, nodeId, nodeId);
         if (!SQLQuery(szQuery))
				return FALSE;
      }
   }

	return TRUE;
}


//
// Check collected data
//

static void CheckIData(void)
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
			_sntprintf(query, 1024, _T("SELECT count(*) FROM idata_%d WHERE idata_timestamp>%d"), nodeId, now);
//			hResult = DBSelect(g_hCoreDB, query);
			hResult = SQLSelect(query);
			if (hResult != NULL)
			{
				if (DBGetFieldLong(hResult, 0, 0) > 0)
				{
					m_iNumErrors++;
					_tprintf(_T("\rFound collected data for node [%d] with timestamp in the future. Correct? (Y/N) "), nodeId);
					if (GetYesNo())
					{
						_sntprintf(query, 1024, _T("DELETE FROM idata_%d WHERE idata_timestamp>%d"), nodeId, now);
						if (SQLQuery(query))
							m_iNumFixes++;
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
/*				m_iNumErrors++;
				_tprintf(_T("\rData collection table for node [%d] not found. Correct? (Y/N) "), nodeId);
				if (GetYesNo())
				{
					if (CreateIDataTable(nodeId))
						m_iNumFixes++;
				}*/
			}
		}
		DBFreeResult(hResultNodes);
	}

	_sntprintf(query, 1024, _T("SELECT count(*) FROM raw_dci_values WHERE last_poll_time>%d"), now);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		if (DBGetFieldLong(hResult, 0, 0) > 0)
		{
			m_iNumErrors++;
			_tprintf(_T("\rFound DCIs with last poll timestamp in the future. Correct? (Y/N) "));
			if (GetYesNo())
			{
				_sntprintf(query, 1024, _T("UPDATE raw_dci_values SET last_poll_time=%d WHERE last_poll_time>%d"), now, now);
				if (SQLQuery(query))
					m_iNumFixes++;
			}
		}
		DBFreeResult(hResult);
	}

	EndStage();
}


//
// Check template to node mapping
//

static void CheckTemplateNodeMapping(void)
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
            _tprintf(_T("\rTemplate %d [%s] mapped to non-existent node %d. Correct? (Y/N) "),
                     dwTemplateId, name, dwNodeId);
            if (GetYesNo())
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


//
// Check database for errors
//

void CheckDatabase(void)
{
   DB_RESULT hResult;
   LONG iVersion = 0;
   BOOL bCompleted = FALSE;

   _tprintf(_T("Checking database:\n"));

   // Get database format version
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         iVersion = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   if (iVersion < DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               iVersion, DB_FORMAT_VERSION);
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
                   "You need to upgrade your server before using this database.\n"),
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
      }

      if (bLocked)
      {
         _tprintf(_T("Database is locked by server %s [%s]\n"
                     "Do you wish to force database unlock? (Y/N) "),
                  szLockStatus, szLockInfo);
         if (GetYesNo())
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

         CheckNodes();
         CheckComponents(_T("interface"), _T("interfaces"));
         CheckComponents(_T("network service"), _T("network_services"));
			CheckClusters();
         CheckTemplateNodeMapping();
         CheckObjectProperties();
         CheckEPP();
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
               _tprintf(_T("Commit changes (Y/N) "));
               if (GetYesNo())
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

   _tprintf(_T("Database check %s\n"), bCompleted ? _T("completed") : _T("aborted"));
}
