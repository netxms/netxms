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
// Global data
//

Queue g_statusPollQueue;
Queue g_configPollQueue;


//
// Status poll thread
//

static THREAD_RESULT THREAD_CALL StatusPoller(void *arg)
{
   Node *pNode;

   while(!ShutdownInProgress())
   {
      pNode = (Node *)g_statusPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      pNode->StatusPoll(NULL, 0);
      pNode->DecRefCount();
   }
   return THREAD_OK;
}


//
// Configuration poll thread
//

static THREAD_RESULT THREAD_CALL ConfigurationPoller(void *arg)
{
   Node *pNode;

   while(!ShutdownInProgress())
   {
      pNode = (Node *)g_configPollQueue.GetOrBlock();
      if (pNode == INVALID_POINTER_VALUE)
         break;   // Shutdown indicator

      pNode->ConfigurationPoll(NULL, 0);
      pNode->DecRefCount();
   }
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

   // Start status pollers
   iNumStatusPollers = ConfigReadInt("NumberOfStatusPollers", 10);
   for(i = 0; i < iNumStatusPollers; i++)
      ThreadCreate(StatusPoller, 0, NULL);

   // Start configuration pollers
   iNumConfigPollers = ConfigReadInt("NumberOfConfigurationPollers", 4);
   for(i = 0; i < iNumConfigPollers; i++)
      ThreadCreate(ConfigurationPoller, 0, NULL);

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
            pNode->LockForStatusPoll();
            g_configPollQueue.Put(pNode);
         }
         if (pNode->ReadyForStatusPoll())
         {
            pNode->IncRefCount();
            pNode->LockForConfigurationPoll();
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

   return THREAD_OK;
}
