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

#ifdef _WIN32
#define DB2_MAX_USER_NAME 32 + 1
#else
#define DB2_MAX_USER_NAME 8 + 1
#endif

typedef struct
{
   TCHAR db2Id[STR_MAX];
} DB2_INFO, *PDB2_INFO;

typedef struct
{
   THREAD threadHandle;
   MUTEX mutex;
   PDB2_INFO db2Info;
} THREAD_INFO, *PTHREAD_INFO;

inline BOOL GetConfigs(Config* config, const TCHAR* section, const PDB2_INFO db2Info)
{
   NX_CFG_TEMPLATE* cfgTemplate =
   {
      { _T("Id"), CT_STRING, 0, 0, STR_MAX, 0, db2Info->db2Id },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
   };

   return config->parseTemplate(section, cfgTemplate);
}

THREAD_RESULT THREAD_CALL RunMonitorThread(void* info);

#endif /* DB2_SUBAGENT_H_ */
