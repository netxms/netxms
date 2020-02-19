/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: wtsproc.cpp
**
**/

#include "libnxagent.h"
#include <wtsapi32.h>

/**
 * Get process list for given user session
 */
ObjectArray<ProcessInformation> LIBNXAGENT_EXPORTABLE *GetProcessListForUserSession(DWORD sessionId)
{
   WTS_PROCESS_INFO *processes;
   DWORD count;
   if (!WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &processes, &count))
      return nullptr;

   auto sessionProcesses = new ObjectArray<ProcessInformation>(64, 64, Ownership::True);
   for (DWORD i = 0; i < count; i++)
   {
      if (processes[i].SessionId == sessionId)
         sessionProcesses->add(new ProcessInformation(processes[i].ProcessId, processes[i].pProcessName));
   }
   WTSFreeMemory(processes);
   return sessionProcesses;
}

/**
 * Check if given process is running in given session (current process is not checked)
 */
bool LIBNXAGENT_EXPORTABLE CheckProcessPresenseInSession(DWORD sessionId, const TCHAR *processName)
{
   ObjectArray<ProcessInformation> *processList = GetProcessListForUserSession(sessionId);
   if (processList == nullptr)
      return false;

   bool found = false;
   for (int i = 0; i < processList->size(); i++)
   {
      if (!_tcscmp(processList->get(i)->name, processName) && (processList->get(i)->pid != GetCurrentProcessId()))
      {
         found = true;
         break;
      }
   }

   delete processList;
   return found;
}
