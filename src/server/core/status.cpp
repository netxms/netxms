/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: status.cpp
**
**/

#include "nxcore.h"


//
// Poller state structure
//

struct __poller_state
{
   int iType;
   time_t timestamp;
   char szMsg[128];
   THREAD handle;
};


//
// Global data
//

Queue g_statusPollQueue;
Queue g_configPollQueue;


//
// Static data
//

static __poller_state *m_pPollerState = NULL;
static int m_iNumPollers = 0;


//
// Set poller's state
//

static void SetPollerState(int nIdx, char *pszMsg)
{
   strncpy(m_pPollerState[nIdx].szMsg, pszMsg, 128);
   m_pPollerState[nIdx].timestamp = time(NULL);
}


//
// Display current poller threads state
//

void ShowPollerState(CONSOLE_CTX pCtx)
{
   int i;
   char szTime[64];
   struct tm *ltm;

   ConsolePrintf(pCtx, "PT  TIME                   STATE\n");
   for(i = 0; i < m_iNumPollers; i++)
   {
      ltm = localtime(&m_pPollerState[i].timestamp);
      strftime(szTime, 64, "%d/%b/%Y %H:%M:%S", ltm);
      ConsolePrintf(pCtx, "%c   %s   %s\n", m_pPollerState[i].iType, szTime, m_pPollerState[i].szMsg);
   }
   ConsolePrintf(pCtx, "\n");
}


//
// Status poll thread
//

static THREAD_RESULT THREAD_CALL StatusPoller(void *arg)
{
   Node *pNode;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(int)arg].iType = 'S';
   SetPollerState((int)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((int)arg, "wait");
      pNode = (Node *)g_statusPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%ld]",
               pNode->Name(), pNode->Id());
      SetPollerState((int)arg, szBuffer);
      pNode->StatusPoll(NULL, 0);
      pNode->DecRefCount();
   }
   SetPollerState((int)arg, "finished");
   return THREAD_OK;
}


//
// Configuration poll thread
//

static THREAD_RESULT THREAD_CALL ConfigurationPoller(void *arg)
{
   Node *pNode;
   char szBuffer[MAX_OBJECT_NAME + 64];

   // Initialize state info
   m_pPollerState[(int)arg].iType = 'C';
   SetPollerState((int)arg, "init");

   // Main loop
   while(!ShutdownInProgress())
   {
      SetPollerState((int)arg, "wait");
      pNode = (Node *)g_configPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      snprintf(szBuffer, MAX_OBJECT_NAME + 64, "poll: %s [%ld]",
               pNode->Name(), pNode->Id());
      SetPollerState((int)arg, szBuffer);
      pNode->ConfigurationPoll(NULL, 0);
      pNode->DecRefCount();
   }
   SetPollerState((int)arg, "finished");
   return THREAD_OK;
}


//
// Node queuing thread
//

THREAD_RESULT THREAD_CALL NodePollManager(void *pArg)
{
   Node *pNode;
   DWORD dwWatchdogId;
   int i, iNumStatusPollers, iNumConfigPollers;

   // Read configuration
   iNumStatusPollers = ConfigReadInt("NumberOfStatusPollers", 10);
   iNumConfigPollers = ConfigReadInt("NumberOfConfigurationPollers", 4);
   m_iNumPollers = iNumStatusPollers + iNumConfigPollers;

   // Prepare static data
   m_pPollerState = (__poller_state *)malloc(sizeof(__poller_state) * m_iNumPollers);

   // Start status pollers
   for(i = 0; i < iNumStatusPollers; i++)
      m_pPollerState[i].handle = ThreadCreateEx(StatusPoller, 0, (void *)i);

   // Start configuration pollers
   for(i = 0; i < iNumConfigPollers; i++)
      m_pPollerState[i + iNumStatusPollers].handle = ThreadCreateEx(ConfigurationPoller, 0, (void *)(i + iNumStatusPollers));

   dwWatchdogId = WatchdogAddThread("Node Poll Manager", 60);

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(5))
         break;      // Shutdown has arrived
      WatchdogNotify(dwWatchdogId);

      // Walk through nodes and queue them for status 
      // and/or configuration poll
      RWLockReadLock(g_rwlockNodeIndex, INFINITE);
      for(DWORD i = 0; i < g_dwNodeAddrIndexSize; i++)
      {
         pNode = (Node *)g_pNodeIndexByAddr[i].pObject;
         if (pNode->ReadyForConfigurationPoll())
         {
            pNode->IncRefCount();
            pNode->LockForConfigurationPoll();
            g_configPollQueue.Put(pNode);
         }
         if (pNode->ReadyForStatusPoll())
         {
            pNode->IncRefCount();
            pNode->LockForStatusPoll();
            g_statusPollQueue.Put(pNode);
         }
      }
      RWLockUnlock(g_rwlockNodeIndex);
   }

   // Send stop signal to all pollers
   g_statusPollQueue.Clear();
   g_configPollQueue.Clear();
   for(i = 0; i < iNumStatusPollers; i++)
      g_statusPollQueue.Put(INVALID_POINTER_VALUE);
   for(i = 0; i < iNumConfigPollers; i++)
      g_configPollQueue.Put(INVALID_POINTER_VALUE);

   // Wait for all pollers
   for(i = 0; i < m_iNumPollers; i++)
      ThreadJoin(m_pPollerState[i].handle);

   m_iNumPollers = 0;
   safe_free(m_pPollerState);

   return THREAD_OK;
}
