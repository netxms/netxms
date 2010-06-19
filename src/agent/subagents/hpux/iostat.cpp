/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
** Copyright (C) 2010 Victor Kirhenshtein
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
*/

#include <nms_common.h>
#include <nms_agent.h>
#include <sys/pstat.h>


//
// Process stats for single disk
//

static void ProcessDiskStats(struct pst_diskinfo *di)
{
}


//
// I/O stat collector
//

THREAD_RESULT THREAD_CALL IOStatCollector(void *arg)
{
	struct pst_diskinfo di[64];

	AgentWriteDebugLog(1, "HPUX: I/O stat collector thread started");

	while(!g_bShutdown)
	{
		for(index = 0, count = 64; count == 64; index += 64)
		{
			count = pstat_getdisk(di, sizeof(struct pst_diskinfo), 64, index);
			for(int i = 0; i < count; i++)
			{
				ProcessDiskStats(&di[i]);
			}
		}
		CalculateTotals();
		s_currSlot++;
		if (s_currSlot == HISTORY_SIZE)
			s_currSlot = 0;
		ThreadSleepMs(1000);
	}

	AgentWriteDebugLog(1, "HPUX: I/O stat collector thread stopped");
	return THREAD_OK;
}
