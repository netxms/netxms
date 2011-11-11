/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2009 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxagentd.cpp
**
**/

#include "nxagentd.h"


//
// Push parameter's data
//

BOOL PushData(const TCHAR *parameter, const TCHAR *value)
{
	CSCPMessage msg;
	BOOL success = FALSE;

	msg.SetCode(CMD_PUSH_DCI_DATA);
	msg.SetVariable(VID_NAME, parameter);
	msg.SetVariable(VID_VALUE, value);

   MutexLock(g_hSessionListAccess);
   for(DWORD i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         if (g_pSessionList[i]->canAcceptTraps())
         {
            g_pSessionList[i]->sendMessage(&msg);
            success = TRUE;
         }
   MutexUnlock(g_hSessionListAccess);

	return success;
}
