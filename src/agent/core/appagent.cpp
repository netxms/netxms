/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: appagent.cpp
**
**/

#include "nxagentd.h"
#include <appagent.h>

/**
 * Application agent info
 */
class ApplicationAgent
{
public:
   TCHAR *m_name;
   HPIPE m_handle;
   MUTEX m_mutex;

   ApplicationAgent(const TCHAR *name)
   {
      m_name = _tcsdup(name);
      m_handle = INVALID_PIPE_HANDLE;
      m_mutex = MutexCreate();
   }

   ~ApplicationAgent()
   {
      safe_free(m_name);
      MutexDestroy(m_mutex);
   }

   void lock()
   {
      MutexLock(m_mutex);
   }

   void unlock()
   {
      MutexUnlock(m_mutex);
   }
};

/**
 * Application agent list
 */
static ObjectArray<ApplicationAgent> s_appAgents;

/**
 * Register application agent
 */
void RegisterApplicationAgent(const TCHAR *name)
{
   ApplicationAgent *agent = new ApplicationAgent(name);
   if (AppAgentConnect(name, &agent->m_handle))
   {
      AgentWriteDebugLog(3, _T("Application agent %s connected"), name);
   }
   else
   {
      AgentWriteDebugLog(3, _T("Cannot connect to application agent %s"), name);
   }
   s_appAgents.add(agent);
}

/**
 * Get value from application agent
 *
 * @return agent error code
 */
UINT32 GetParameterValueFromAppAgent(const TCHAR *name, TCHAR *buffer)
{
	UINT32 rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_appAgents.size(); i++)
	{
      ApplicationAgent *agent = s_appAgents.get(i);
      agent->lock();

reconnect:
		if (agent->m_handle == INVALID_PIPE_HANDLE)
		{
         if (AppAgentConnect(agent->m_name, &agent->m_handle))
         {
            AgentWriteDebugLog(3, _T("Application agent %s connected"), agent->m_name);
         }
         else
         {
            AgentWriteDebugLog(3, _T("Cannot connect to application agent %s"), agent->m_name);
            agent->unlock();
            continue;
         }
      }

      int apprc = AppAgentGetMetric(agent->m_handle, name, buffer, MAX_RESULT_LENGTH);
      if (apprc == APPAGENT_RCC_SUCCESS)
      {
         rc = ERR_SUCCESS;
         agent->unlock();
         break;
      }
      if (apprc == APPAGENT_RCC_NO_SUCH_INSTANCE)
      {
         rc = ERR_UNKNOWN_PARAMETER;
         agent->unlock();
         break;
      }
      if (apprc == APPAGENT_RCC_METRIC_READ_ERROR)
      {
         rc = ERR_INTERNAL_ERROR;
         agent->unlock();
         break;
      }
      if (apprc == APPAGENT_RCC_COMM_FAILURE)
      {
         // try reconnect
         AppAgentDisconnect(agent->m_handle);
         agent->m_handle = INVALID_PIPE_HANDLE;
         goto reconnect;
      }
      agent->unlock();
	}
	return rc;
}
