/**
 * NetXMS - open source network management system
 * Copyright (C) 2013 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "db2_subagent.h"

DB_DRIVER g_drvHandle = NULL;
CONDITION g_sdownCondition = NULL;
PTHREAD_INFO g_threads = NULL;;
UINT32 g_numOfThreads = 0;

static NETXMS_SUBAGENT_PARAM m_agentParams[] =
{
   {
      _T("DB2.Instance.Version(*)"), GetParameter, _D(DCI_DBMS_VERSION),
      DCI_DT_STRING, _T("DB2/Instance: DBMS Version")
   },
   {
      _T("DB2.Table.Available(*)"), GetParameter, _D(DCI_NUM_AVAILABLE),
      DCI_DT_INT, _T("DB2/Table: Number of available tables")
   },
   {
      _T("DB2.Table.Unavailable(*)"), GetParameter, _D(DCI_NUM_UNAVAILABLE),
      DCI_DT_INT, _T("DB2/Table: Number of unavailable tables")
   },
   {
      _T("DB2.Table.Data.LogicalSize(*)"), GetParameter, _D(DCI_DATA_L_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Data: Data object logical size in kilobytes")
   },
   {
      _T("DB2.Table.Data.PhysicalSize(*)"), GetParameter, _D(DCI_DATA_P_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Data: Data object physical size in kilobytes")
   },
   {
      _T("DB2.Table.Index.LogicalSize(*)"), GetParameter, _D(DCI_INDEX_L_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Index: Index object logical size in kilobytes")
   },
   {
      _T("DB2.Table.Index.PhysicalSize(*)"), GetParameter, _D(DCI_INDEX_P_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Index: Index object physical size in kilobytes")
   },
   {
      _T("DB2.Table.Long.LogicalSize(*)"), GetParameter, _D(DCI_LONG_L_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Long: Long object logical size in kilobytes")
   },
   {
      _T("DB2.Table.Long.PhysicalSize(*)"), GetParameter, _D(DCI_LONG_P_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Long: Long object physical size in kilobytes")
   },
   {
      _T("DB2.Table.Lob.LogicalSize(*)"), GetParameter, _D(DCI_LOB_L_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Lob: LOB object logical size in kilobytes")
   },
   {
      _T("DB2.Table.Lob.PhysicalSize(*)"), GetParameter, _D(DCI_LOB_P_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Lob: LOB object physical size in kilobytes")
   },
   {
      _T("DB2.Table.Xml.LogicalSize(*)"), GetParameter, _D(DCI_XML_L_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Xml: XML object logical size in kilobytes")
   },
   {
      _T("DB2.Table.Xml.PhysicalSize(*)"), GetParameter, _D(DCI_XML_P_SIZE),
      DCI_DT_INT64, _T("DB2/Table/Xml: XML object physical size in kilobytes")
   },
   {
      _T("DB2.Table.Index.Type1(*)"), GetParameter, _D(DCI_INDEX_TYPE1),
      DCI_DT_INT, _T("DB2/Table/Index: Number of tables using type-1 indexes")
   },
   {
      _T("DB2.Table.Index.Type2(*)"), GetParameter, _D(DCI_INDEX_TYPE2),
      DCI_DT_INT, _T("DB2/Table/Index: Number of tables using type-2 indexes")
   },
   {
      _T("DB2.Table.Reorg.Pending(*)"), GetParameter, _D(DCI_REORG_PENDING),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of tables pending reorganization")
   },
   {
      _T("DB2.Table.Reorg.Aborted(*)"), GetParameter, _D(DCI_REORG_ABORTED),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of tables in aborted reorganization state")
   },
   {
      _T("DB2.Table.Reorg.Executing(*)"), GetParameter, _D(DCI_REORG_EXECUTING),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of tables in executing reorganization state")
   },
   {
      _T("DB2.Table.Reorg.Null(*)"), GetParameter, _D(DCI_REORG_NULL),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of tables in null reorganization state")
   },
   {
      _T("DB2.Table.Reorg.Paused(*)"), GetParameter, _D(DCI_REORG_PAUSED),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of tables in paused reorganization state")
   },
   {
      _T("DB2.Table.Reorg.Alters(*)"), GetParameter, _D(DCI_NUM_REORG_REC_ALTERS),
      DCI_DT_INT, _T("DB2/Table/Reorg: Number of reorg recommend alter operations")
   },
   {
      _T("DB2.Table.Load.InProgress(*)"), GetParameter, _D(DCI_LOAD_IN_PROGRESS),
      DCI_DT_INT, _T("DB2/Table/Load: Number of tables with load in progress status")
   },
   {
      _T("DB2.Table.Load.Pending(*)"), GetParameter, _D(DCI_LOAD_PENDING),
      DCI_DT_INT, _T("DB2/Table/Load: Number of tables with load pending status")
   },
   {
      _T("DB2.Table.Load.Null(*)"), GetParameter, _D(DCI_LOAD_NULL),
      DCI_DT_INT, _T("DB2/Table/Load: Number of tables with load status neither in progress nor pending")
   },
   {
      _T("DB2.Table.Readonly(*)"), GetParameter, _D(DCI_ACCESS_RO),
      DCI_DT_INT, _T("DB2/Table: Number of tables in Read Access Only state")
   },
   {
      _T("DB2.Table.NoLoadRestart(*)"), GetParameter, _D(DCI_NO_LOAD_RESTART),
      DCI_DT_INT, _T("DB2/Table: Number of tables in a state that won't allow a load restart")
   },
   {
      _T("DB2.Table.Index.Rebuild(*)"), GetParameter, _D(DCI_INDEX_REQUIRE_REBUILD),
      DCI_DT_INT, _T("DB2/Table/Index: Number of tables with indexes that require rebuild")
   },
   {
      _T("DB2.Table.Rid.Large(*)"), GetParameter, _D(DCI_LARGE_RIDS),
      DCI_DT_INT, _T("DB2/Table/Rid: Number of tables that use large row IDs")
   },
   {
      _T("DB2.Table.Rid.Usual(*)"), GetParameter, _D(DCI_NO_LARGE_RIDS),
      DCI_DT_INT, _T("DB2/Table/Rid: Number of tables that don't use large row IDs")
   },
   {
      _T("DB2.Table.Rid.Pending(*)"), GetParameter, _D(DCI_LARGE_RIDS_PENDING),
      DCI_DT_INT, _T("DB2/Table/Rid: Number of tables that use large row Ids but not all indexes have been rebuilt yet")
   },
   {
      _T("DB2.Table.Slot.Large(*)"), GetParameter, _D(DCI_LARGE_SLOTS),
      DCI_DT_INT, _T("DB2/Table/Slot: Number of tables that use large slots")
   },
   {
      _T("DB2.Table.Slot.Usual(*)"), GetParameter, _D(DCI_NO_LARGE_SLOTS),
      DCI_DT_INT, _T("DB2/Table/Slot: Number of tables that don't use large slots")
   },
   {
      _T("DB2.Table.Slot.Pending(*)"), GetParameter, _D(DCI_LARGE_SLOTS_PENDING),
      DCI_DT_INT,
      _T("DB2/Table/Slot: Number of tables that use large slots but there has not yet been an offline table reorganization")
      _T(" or table truncation operation")
   },
   {
      _T("DB2.Table.DictSize(*)"), GetParameter, _D(DCI_DICTIONARY_SIZE),
      DCI_DT_INT64, _T("DB2/Table: Size of the dictionary in bytes")
   }
};

static NETXMS_SUBAGENT_INFO m_agentInfo =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   SUBAGENT_NAME,
   NETXMS_VERSION_STRING,
   DB2Init, DB2Shutdown, DB2CommandHandler,
   (sizeof(m_agentParams) / sizeof(NETXMS_SUBAGENT_PARAM)), m_agentParams,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL
};

static QUERY g_queries[] =
{
   { { DCI_DBMS_VERSION }, _T("SELECT service_level FROM TABLE (sysproc.env_get_inst_info())") },
   { { DCI_NUM_AVAILABLE }, _T("SELECT count(available) FROM sysibmadm.admintabinfo WHERE available = 'Y'") },
   { { DCI_NUM_UNAVAILABLE }, _T("SELECT count(available) FROM sysibmadm.admintabinfo WHERE available = 'N'") },
   {
      {
         DCI_DATA_L_SIZE, DCI_DATA_P_SIZE, DCI_INDEX_L_SIZE, DCI_INDEX_P_SIZE, DCI_LONG_L_SIZE, DCI_LONG_P_SIZE,
         DCI_LOB_L_SIZE, DCI_LOB_P_SIZE, DCI_XML_L_SIZE, DCI_XML_P_SIZE, DCI_DICTIONARY_SIZE, DCI_NUM_REORG_REC_ALTERS
      },
      _T("SELECT sum(data_object_l_size), sum(data_object_p_size), sum(index_object_l_size), sum(index_object_p_size),")
      _T("sum(long_object_l_size), sum(long_object_p_size), sum(lob_object_l_size), sum(lob_object_p_size),")
      _T("sum(xml_object_l_size), sum(xml_object_p_size), sum(dictionary_size), sum(num_reorg_rec_alters) FROM sysibmadm.admintabinfo")
   },
   { { DCI_INDEX_TYPE1 }, _T("SELECT count(index_type) FROM sysibmadm.admintabinfo WHERE index_type = 1") },
   { { DCI_INDEX_TYPE2 }, _T("SELECT count(index_type) FROM sysibmadm.admintabinfo WHERE index_type = 2") },
   { { DCI_REORG_PENDING }, _T("SELECT count(reorg_pending) FROM sysibmadm.admintabinfo WHERE reorg_pending = 'Y'") },
   { { DCI_REORG_ABORTED }, _T("SELECT count(inplace_reorg_status) FROM sysibmadm.admintabinfo WHERE inplace_reorg_status = 'ABORTED'") },
   { { DCI_REORG_EXECUTING }, _T("SELECT count(inplace_reorg_status) FROM sysibmadm.admintabinfo WHERE inplace_reorg_status = 'EXECUTING'") },
   { { DCI_REORG_PAUSED }, _T("SELECT count(inplace_reorg_status) FROM sysibmadm.admintabinfo WHERE inplace_reorg_status = 'PAUSED'") },
   { { DCI_REORG_NULL }, _T("SELECT count(inplace_reorg_status) FROM sysibmadm.admintabinfo WHERE inplace_reorg_status = 'NULL'") },
   { { DCI_LOAD_IN_PROGRESS }, _T("SELECT count(load_status) FROM sysibmadm.admintabinfo WHERE load_status = 'IN_PROGRESS'") },
   { { DCI_LOAD_PENDING }, _T("SELECT count(load_status) FROM sysibmadm.admintabinfo WHERE load_status = 'PENDING'") },
   { { DCI_LOAD_NULL }, _T("SELECT count(load_status) FROM sysibmadm.admintabinfo WHERE load_status = 'NULL'") },
   { { DCI_ACCESS_RO }, _T("SELECT count(read_access_only) FROM sysibmadm.admintabinfo WHERE read_access_only = 'Y'") },
   { { DCI_NO_LOAD_RESTART }, _T("SELECT count(no_load_restart) FROM sysibmadm.admintabinfo WHERE no_load_restart = 'Y'") },
   { { DCI_INDEX_REQUIRE_REBUILD }, _T("SELECT count(indexes_require_rebuild) FROM sysibmadm.admintabinfo WHERE indexes_require_rebuild = 'Y'") },
   { { DCI_LARGE_RIDS }, _T("SELECT count(large_rids) FROM sysibmadm.admintabinfo WHERE large_rids = 'Y'") },
   { { DCI_NO_LARGE_RIDS }, _T("SELECT count(large_rids) FROM sysibmadm.admintabinfo WHERE large_rids = 'N'") },
   { { DCI_LARGE_RIDS_PENDING }, _T("SELECT count(large_rids) FROM sysibmadm.admintabinfo WHERE large_rids = 'P'") },
   { { DCI_LARGE_SLOTS }, _T("SELECT count(large_slots) FROM sysibmadm.admintabinfo WHERE large_slots = 'Y'") },
   { { DCI_NO_LARGE_SLOTS }, _T("SELECT count(large_slots) FROM sysibmadm.admintabinfo WHERE large_slots = 'N'") },
   { { DCI_LARGE_SLOTS_PENDING }, _T("SELECT count(large_slots) FROM sysibmadm.admintabinfo WHERE large_slots = 'P'") },
   { { DCI_NULL }, _T("\0") }
};

static BOOL DB2Init(Config* config)
{
   AgentWriteDebugLog(7, _T("%s: initializing"), SUBAGENT_NAME);

#ifdef _WIN32
   g_drvHandle = DBLoadDriver(_T("db2.ddr"), NULL, TRUE, NULL, NULL);
#else
   g_drvHandle = DBLoadDriver(LIBDIR _T("/netxms/dbdrv/db2.ddr"), NULL, TRUE, NULL, NULL);
#endif

   if (g_drvHandle == NULL)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load the database driver"), SUBAGENT_NAME);
      return FALSE;
   }

   AgentWriteDebugLog(7, _T("%s: loaded the database driver"), SUBAGENT_NAME);

   ConfigEntry* db2IniEntry = config->getEntry(_T("/db2"));

   if (db2IniEntry == NULL)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: no entries found in the configuration file"), SUBAGENT_NAME);
      return FALSE;
   }

   PDB2_INFO* arrDb2Info;
   int numOfPossibleThreads = 0;

   ConfigEntry* xmlFile = db2IniEntry->findEntry(_T("ConfigFile"));
   if (xmlFile == NULL)
   {
      AgentWriteDebugLog(7, _T("%s: processing configuration entries in section '%s'"), SUBAGENT_NAME, db2IniEntry->getName());

      const PDB2_INFO db2Info = GetConfigs(config, db2IniEntry);
      if (db2Info == NULL)
      {
         return FALSE;
      }

      g_numOfThreads = 1;
      arrDb2Info = new PDB2_INFO[1];
      arrDb2Info[0] = db2Info;
   }
   else
   {
      const TCHAR* pathToXml = xmlFile->getValue();
      AgentWriteDebugLog(7, _T("%s: processing configuration file '%s'"), SUBAGENT_NAME, pathToXml);

      if (!config->loadXmlConfig(pathToXml))
      {
        AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: '%s' is not a valid configuration file"), SUBAGENT_NAME, pathToXml);
        return FALSE;
      }

      ConfigEntry* db2SubXmlEntry = config->getEntry(_T("/db2sub"));
      if (db2SubXmlEntry == NULL)
      {
         AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: '%s' doesn't contain the db2 configuration entry"), SUBAGENT_NAME, pathToXml);
         return FALSE;
      }

      ConfigEntryList* db2SubXmlSubEntries = db2SubXmlEntry->getSubEntries(_T("db2#*"));
      numOfPossibleThreads = db2SubXmlSubEntries->getSize();
      AgentWriteDebugLog(7, _T("%s: '%s' loaded with number of db2 entries: %d"), SUBAGENT_NAME, pathToXml, numOfPossibleThreads);
      arrDb2Info = new PDB2_INFO[numOfPossibleThreads];

      for(int i = 0; i < numOfPossibleThreads; i++)
      {
         ConfigEntry* entry = db2SubXmlSubEntries->getEntry(i);
         const PDB2_INFO db2Info = GetConfigs(config, entry);
         if (db2Info == NULL)
         {
            continue;
         }

         arrDb2Info[g_numOfThreads] = db2Info;
         g_numOfThreads++;
      }
   }

   if(g_numOfThreads > 0)
   {
      AgentWriteDebugLog(7, _T("%s: loaded %d configuration section(s)"), SUBAGENT_NAME, g_numOfThreads);
      g_threads = new THREAD_INFO[g_numOfThreads];
      g_sdownCondition = ConditionCreate(TRUE);
   }

   for(int i = 0; i < g_numOfThreads; i++)
   {
      //g_threads[i] = new THREAD_INFO;
      g_threads[i].mutex = MutexCreate();
      g_threads[i].db2Info = arrDb2Info[i];
      g_threads[i].threadHandle = ThreadCreateEx(RunMonitorThread, 0, (void*) &g_threads[i]);
   }

   delete[] arrDb2Info;

   if (g_numOfThreads > 0)
   {
      AgentWriteDebugLog(7, _T("%s: starting with %d query thread(s)"), SUBAGENT_NAME, g_numOfThreads);
      return TRUE;
   }

   return FALSE;
}

static void DB2Shutdown()
{
   AgentWriteDebugLog(9, _T("%s: terminating"), SUBAGENT_NAME);
   ConditionSet(g_sdownCondition);

   for(int i = 0; i < g_numOfThreads; i++)
   {
      ThreadJoin(g_threads[i].threadHandle);
      MutexDestroy(g_threads[i].mutex);
      delete g_threads[i].db2Info;
   }
   delete[] g_threads;

   DBUnloadDriver(g_drvHandle);
   ConditionDestroy(g_sdownCondition);

   g_drvHandle = NULL;
   g_sdownCondition = NULL;
   g_threads = NULL;

   AgentWriteDebugLog(7, _T("%s: terminated"), SUBAGENT_NAME);
}

static BOOL DB2CommandHandler(UINT32 dwCommand, CSCPMessage* pRequest, CSCPMessage* pResponse, void* session)
{
   return FALSE;
}

static THREAD_RESULT THREAD_CALL RunMonitorThread(void* info)
{
   PTHREAD_INFO threadInfo = (PTHREAD_INFO) info;
   PDB2_INFO db2Info = threadInfo->db2Info;
   DB_HANDLE dbHandle = NULL;
   TCHAR connectError[DBDRV_MAX_ERROR_TEXT];
   DWORD reconnectInterval = (DWORD) db2Info->db2ReconnectInterval;

   while(TRUE)
   {
      dbHandle = DBConnect(
         g_drvHandle, db2Info->db2DbAlias, db2Info->db2DbName, db2Info->db2UName, db2Info->db2UPass, NULL, connectError);

      if (dbHandle == NULL)
      {
         AgentWriteLog(
            EVENTLOG_ERROR_TYPE,
            _T("%s: failed to connect to the database \"%s\" (%s), reconnecting in %ds"),
            SUBAGENT_NAME, db2Info->db2DbName, connectError, reconnectInterval);

         if (ConditionWait(g_sdownCondition, (reconnectInterval * 1000)))
         {
            break;
         }
         else
         {
            continue;
         }
      }

      threadInfo->hDb = dbHandle;

      AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("%s: connected to database '%s'"), SUBAGENT_NAME, db2Info->db2DbName);

      if (PerformQueries(threadInfo))
      {
         break;
      }
   }

   if(dbHandle != NULL)
   {
      DBDisconnect(dbHandle);
   }

   return THREAD_OK;
}

static BOOL PerformQueries(const PTHREAD_INFO threadInfo)
{
   PDB2_INFO db2Info = threadInfo->db2Info;
   DWORD queryInterval = (DWORD) db2Info->db2QueryInterval;

   while(TRUE)
   {
      MutexLock(threadInfo->mutex);

      DB_RESULT hResult;
      Dci* dciList;

      int queryId = 0;
      while(*(dciList = g_queries[queryId].dciList) != DCI_NULL)
      {
         hResult = DBSelect(threadInfo->hDb, g_queries[queryId].query);

         if (hResult == NULL)
         {
            AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: query '%s' failed"), SUBAGENT_NAME, g_queries[queryId].query);
            queryId++;
            continue;
         }

         int numCols = DBGetColumnCount(hResult);

         for(int i = 0; i < numCols; i++)
         {
            TCHAR* dciVal = threadInfo->db2Params[dciList[i]];

            DBGetField(hResult, 0, i, dciVal, STR_MAX);

            AgentWriteDebugLog(9, _T("%s: got '%s'"), SUBAGENT_NAME, dciVal);
         }

         DBFreeResult(hResult);

         queryId++;
      }

      MutexUnlock(threadInfo->mutex);

      if (ConditionWait(g_sdownCondition, queryInterval * 1000))
      {
         break;
      }
   }

   return TRUE;
}

static LONG GetParameter(const TCHAR* parameter, const TCHAR* arg, TCHAR* value)
{
   Dci dci = StringToDci(arg);

   if (dci == DCI_NULL)
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   TCHAR dbIdString[DB_ID_DIGITS_MAX];
   unsigned long dbId;

   AgentGetParameterArg(parameter, 1, dbIdString, DB_ID_DIGITS_MAX);
   dbId = _tcstoul(dbIdString, NULL, 0);

   if (dbId == 0)
   {
      AgentWriteDebugLog(7, _T("%s: id '%s' is invalid"), SUBAGENT_NAME, dbIdString);
      return SYSINFO_RC_UNSUPPORTED;
   }

   THREAD_INFO threadInfo;

   for(int i = 0; i < g_numOfThreads; i++)
   {
      if (g_threads[i].db2Info->db2Id == dbId)
      {
         threadInfo = g_threads[i];
         break;
      }

      AgentWriteDebugLog(7, _T("%s: no database with id '%d' found"), SUBAGENT_NAME, dbId);
      return SYSINFO_RC_UNSUPPORTED;
   }

   ret_string(value, threadInfo.db2Params[dci]);

   return SYSINFO_RC_SUCCESS;
}

static const PDB2_INFO GetConfigs(Config* config, ConfigEntry* configEntry)
{
   ConfigEntryList* entryList = configEntry->getSubEntries(_T("*"));
   TCHAR entryName[STR_MAX] = _T("db2sub/");

   _tcscat(entryName, configEntry->getName());

   if (entryList->getSize() == 0)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: entry '%s' contained no values"), SUBAGENT_NAME, entryName);
      return NULL;
   }

   const PDB2_INFO db2Info = new DB2_INFO();
   BOOL noErr = TRUE;

   db2Info->db2Id = configEntry->getId();

   NX_CFG_TEMPLATE cfgTemplate[] =
   {
      { _T("DBName"),            CT_STRING,      0, 0, DB2_DB_MAX_NAME,   0, db2Info->db2DbName },
      { _T("DBAlias"),           CT_STRING,      0, 0, DB2_DB_MAX_NAME,   0, db2Info->db2DbAlias },
      { _T("UserName"),          CT_STRING,      0, 0, DB2_MAX_USER_NAME, 0, db2Info->db2UName },
      { _T("Password"),          CT_STRING,      0, 0, STR_MAX,           0, db2Info->db2UPass },
      { _T("ReconnectInterval"), CT_LONG,        0, 0, sizeof(LONG),      0, &(db2Info->db2ReconnectInterval) },
      { _T("QueryInterval"),     CT_LONG,        0, 0, sizeof(LONG),      0, &(db2Info->db2QueryInterval) },
      { _T(""),                  CT_END_OF_LIST, 0, 0, 0,                 0, NULL }
   };

   if (!config->parseTemplate(entryName, cfgTemplate))
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to process entry '%s'"), SUBAGENT_NAME, entryName);
      noErr = FALSE;
   }

   if (noErr)
   {
      if (db2Info->db2DbName[0] == '\0')
      {
         AgentWriteDebugLog(7, _T("%s: no DBName in '%s'"), SUBAGENT_NAME, entryName);
         noErr = FALSE;
      }
      else
      {
         AgentWriteDebugLog(9, _T("%s: got DBName entry with value '%s'"), SUBAGENT_NAME, db2Info->db2DbName);
      }

      if (db2Info->db2DbAlias[0] == '\0')
      {
         AgentWriteDebugLog(7, _T("%s: no DBAlias in '%s'"), SUBAGENT_NAME, entryName);
         noErr = FALSE;
      }
      else
      {
         AgentWriteDebugLog(9, _T("%s: got DBAlias entry with value '%s'"), SUBAGENT_NAME, db2Info->db2DbAlias);
      }

      if (db2Info->db2UName[0] == '\0')
      {
         AgentWriteDebugLog(7, _T("%s: no UserName in '%s'"), SUBAGENT_NAME, entryName);
         noErr = FALSE;
      }
      else
      {
         AgentWriteDebugLog(9, _T("%s: got UserName entry with value '%s'"), SUBAGENT_NAME, db2Info->db2UName);
      }

      if (db2Info->db2UPass[0] == '\0')
      {
         AgentWriteDebugLog(7, _T("%s: no Password in '%s'"), SUBAGENT_NAME, entryName);
         noErr = FALSE;
      }
      else
      {
         AgentWriteDebugLog(9, _T("%s: got Password entry with value '%s'"), SUBAGENT_NAME, db2Info->db2UPass);
      }

      if (db2Info->db2ReconnectInterval == 0)
      {
         db2Info->db2ReconnectInterval = INTERVAL_RECONNECT_SECONDS;
      }

      if (db2Info->db2QueryInterval == 0)
      {
         db2Info->db2QueryInterval = INTERVAL_QUERY_SECONDS;
      }
   }

   if (!noErr)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to process entry '%s'"), SUBAGENT_NAME, entryName);
      delete db2Info;
      return NULL;
   }

   return db2Info;
}

DECLARE_SUBAGENT_ENTRY_POINT(DB2)
{
   AgentWriteDebugLog(7, _T("%s: started"), SUBAGENT_NAME);
   *ppInfo = &m_agentInfo;
   return TRUE;
}

/**
 * DLL entry point
 */

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH)
   {
      DisableThreadLibraryCalls(hinstDll);
   }
   return TRUE;
}

#endif
