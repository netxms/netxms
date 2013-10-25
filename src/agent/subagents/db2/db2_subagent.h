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
#define DB_MAX_COUNT 32
#define INTERVAL_QUERY_SECONDS 60
#define INTERVAL_RECONNECT_SECONDS 30

#ifdef _WIN32
#define DB2_MAX_USER_NAME 32 + 1
#else
#define DB2_MAX_USER_NAME 8 + 1
#endif

typedef struct
{
   TCHAR db2NodeId[STR_MAX];
   TCHAR db2UName[DB2_MAX_USER_NAME];
   TCHAR db2UPass[STR_MAX];
   LONG db2ReconnectInterval;
   LONG db2QueryInterval;
} DB2_INFO, *PDB2_INFO;

typedef struct
{
   THREAD threadHandle;
   MUTEX mutex;
   PDB2_INFO db2Info;
} THREAD_INFO, *PTHREAD_INFO;

inline BOOL GetConfigs(Config* config, const TCHAR* section, const PDB2_INFO db2Info)
{
   NX_CFG_TEMPLATE cfgTemplate[] =
   {
      { _T("NodeId"),            CT_STRING,      0, 0, STR_MAX,           0, db2Info->db2NodeId },
      { _T("UserName"),          CT_STRING,      0, 0, DB2_MAX_USER_NAME, 0, db2Info->db2UName },
      { _T("Password"),          CT_STRING,      0, 0, STR_MAX,           0, db2Info->db2UPass },
      { _T("ReconnectInterval"), CT_LONG,        0, 0, sizeof(LONG),      0, &(db2Info->db2ReconnectInterval) },
      { _T("QueryInterval"),     CT_LONG,        0, 0, sizeof(LONG),      0, &(db2Info->db2QueryInterval) },
      { _T(""),                  CT_END_OF_LIST, 0, 0, 0,                 0, NULL }
   };

   if (!config->parseTemplate(section, cfgTemplate))
   {
      return FALSE;
   }

   BOOL noErr = TRUE;

   if (_tcslen(db2Info->db2NodeId) == 0)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: no NodeId in %s"), SUBAGENT_NAME, section);
      noErr = FALSE;
   }

   if (db2Info->db2ReconnectInterval == 0)
   {
      db2Info->db2ReconnectInterval = INTERVAL_RECONNECT_SECONDS;
   }

   if (db2Info->db2QueryInterval == 0)
   {
      db2Info->db2QueryInterval = INTERVAL_QUERY_SECONDS;
   }

   //return noErr;
   return FALSE;
}

static THREAD_RESULT THREAD_CALL RunMonitorThread(void* info);
static BOOL PerformQueries(const PTHREAD_INFO);

#endif /* DB2_SUBAGENT_H_ */
