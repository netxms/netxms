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
#ifndef DB2_SUBAGENT_H_
#define DB2_SUBAGENT_H_

#include <nms_agent.h>
#include <nxdbapi.h>
#include <nms_util.h>
#include <nms_common.h>


#define SUBAGENT_NAME _T("DB2")
#define STR_MAX 256
#define INTERVAL_QUERY_SECONDS 60
#define INTERVAL_RECONNECT_SECONDS 30

/**
 * DB2 constants
 */
#define DB2_DB_MAX_NAME 8 + 1
#ifdef _WIN32
#define DB2_MAX_USER_NAME 32 + 1
#else
#define DB2_MAX_USER_NAME 8 + 1
#endif

typedef struct
{
   TCHAR db2DbName[DB2_DB_MAX_NAME];
   TCHAR db2DbAlias[DB2_DB_MAX_NAME];
   TCHAR db2UName[DB2_MAX_USER_NAME];
   TCHAR db2UPass[STR_MAX];
   LONG db2ReconnectInterval;
   LONG db2QueryInterval;
} DB2_INFO, *PDB2_INFO;

typedef struct
{
   THREAD threadHandle;
   MUTEX mutex;
   DB_HANDLE hDb;
   PDB2_INFO db2Info;
} THREAD_INFO, *PTHREAD_INFO;

enum dci
{
   DCI_DBMS_VERSION
};

inline const PDB2_INFO GetConfigs(Config* config, ConfigEntry* configEntry)
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

static THREAD_RESULT THREAD_CALL RunMonitorThread(void* info);
static BOOL PerformQueries(const PTHREAD_INFO);

#endif /* DB2_SUBAGENT_H_ */
