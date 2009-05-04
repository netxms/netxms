/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: jobmgr.cpp
**
**/

#include "nxcore.h"


//
// Add job
//

bool NXCORE_EXPORTABLE AddJob(ServerJob *job)
{
	bool success = false;

	NetObj *object = FindObjectById(job->getRemoteNode());
	if ((object != NULL) && (object->Type() == OBJECT_NODE))
	{
		ServerJobQueue *queue = ((Node *)object)->getJobQueue();
		queue->add(job);
		success = true;
	}
	return success;
}


//
// Get job list
//

void GetJobList(CSCPMessage *msg)
{
	DWORD i, id, count;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0, id = VID_JOB_LIST_BASE, count = 0; i < g_dwIdIndexSize; i++)
   {
      if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
      {
			ServerJobQueue *queue = ((Node *)g_pIndexById[i].pObject)->getJobQueue();
			count += queue->fillMessage(msg, &id);
      }
   }
   RWLockUnlock(g_rwlockIdIndex);
	msg->SetVariable(VID_JOB_COUNT, count);
}
