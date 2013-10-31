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

#include "db2dci.h"

#define SUBAGENT_NAME _T("DB2")
#define STR_MAX 256
#define INTERVAL_QUERY_SECONDS 60
#define INTERVAL_RECONNECT_SECONDS 30
#define QUERY_MAX 512
#define DB_ID_DIGITS_MAX 10 + 1
#define DCI_LIST_SIZE 12

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
   int db2Id;
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
   TCHAR db2Params[NUM_OF_DCI][STR_MAX];
} THREAD_INFO, *PTHREAD_INFO;

typedef struct
{
   Dci dciList[DCI_LIST_SIZE];
   TCHAR query[QUERY_MAX];
} QUERY;

static BOOL DB2Init(Config* config);
static void DB2Shutdown();
static BOOL DB2CommandHandler(UINT32 dwCommand, CSCPMessage* pRequest, CSCPMessage* pResponse, void* session);

static THREAD_RESULT THREAD_CALL RunMonitorThread(void* info);
static BOOL PerformQueries(const PTHREAD_INFO);
static LONG GetParameter(const TCHAR* parameter, const TCHAR* arg, TCHAR* value);
static const PDB2_INFO GetConfigs(Config* config, ConfigEntry* configEntry);

#endif /* DB2_SUBAGENT_H_ */
