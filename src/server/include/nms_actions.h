/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nms_actions.h
**
**/

#ifndef _nms_actions_h_
#define _nms_actions_h_


//
// Functions
//

BOOL InitActions();
void CleanupActions();
BOOL ExecuteAction(UINT32 dwActionId, Event *pEvent, TCHAR *pszAlarmMsg);
UINT32 CreateNewAction(const TCHAR *pszName, UINT32 *pdwId);
UINT32 DeleteActionFromDB(UINT32 dwActionId);
UINT32 ModifyActionFromMessage(CSCPMessage *pMsg);
void FillActionInfoMessage(CSCPMessage *pMsg, NXC_ACTION *pAction);
void SendActionsToClient(ClientSession *pSession, UINT32 dwRqId);

#endif   /* _nms_actions_ */
