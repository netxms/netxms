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
PTHREAD_INFO g_threads;
int g_numToWatch = 0;

static BOOL DB2Init(Config* config)
{
#ifdef _WIN32
   g_drvHandle = DBLoadDriver(_T("db2.ddr"), NULL, TRUE, NULL, NULL);
#else
   g_drvHandle = DBLoadDriver(LIBDIR _T("/netxms/dbdrv/db2.ddr"), NULL, TRUE, NULL, NULL);
#endif

   if (g_driverHandle == NULL)
   {
      AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load the database driver"), SUBAGENT_NAME);
      return FALSE;
   }

   for(int i = 0; i < DB_MAX; i++)
   {
      if (!GetConfigs(config))
      {
         AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: error parsing configuration data"), SUBAGENT_NAME);
         continue;
      }

      g_numToWatch++;
   }

   if(g_numToWatch > 0)
   {
      g_threads = new THREAD_INFO[g_numToWatch];
      g_sdownCondition = ConditionCreate(TRUE);
   }

   for(int i = 0; i < g_numToWatch; i++)
   {
      g_threads[i] = new THREAD_INFO();
      g_threads[i].threadHandle = ThreadCreateEx(RunMonitorThread, 0, (void*) g_threads[i]);
      g_threads[i].mutex = MutexCreate();
   }

   return TRUE;
}

static void DB2Shutdown()
{
   for(int i = 0; i < g_numToWatch; i++)
   {
      ThreadJoin(g_threads[i].threadHandle);
      MutexDestroy(g_threads[i].mutex);
   }
   ConditionDestroy(g_sdownCondition);
}

static BOOL DB2CommandHandler(INT32 dwCommand, CSCPMessage *pRequest, CSCPMessage *pResponse, void *session)
{
   return FALSE;
}

THREAD_RESULT THREAD_CALL RunMonitorThread(void * info)
{
   PTHREAD_INFO threadInfo = (PTHREAD_INFO) info;

   return THREAD_OK;
}

static NETXMS_SUBAGENT_PARAM* m_agentParams = {

};

static NETXMS_SUBAGENT_INFO* m_agentInfo =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   SUBAGENT_NAME,
   NETXMS_VERSION_STRING,
   DB2Init, DB2Shutdown, DB2CommandHandler,
   (sizeof(*m_agentParams) / sizeof(NETXMS_SUBAGENT_PARAM)), m_agentParams,
   0, NULL,
   0, NULL,
   0, NULL,
   0, NULL
};

DECLARE_SUBAGENT_ENTRY_POINT(DB2)
{
   *ppInfo = m_agentInfo;
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

