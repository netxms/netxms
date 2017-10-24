/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: nms_actions.h
**
**/

#ifndef _nms_actions_h_
#define _nms_actions_h_


//
// Functions
//

BOOL InitActions();
void CleanupActions();
BOOL ExecuteAction(UINT32 dwActionId, Event *pEvent, const TCHAR *alarmMsg, const TCHAR *alarmKey);
UINT32 CreateNewAction(const TCHAR *pszName, UINT32 *pdwId);
UINT32 DeleteActionFromDB(UINT32 dwActionId);
UINT32 ModifyActionFromMessage(NXCPMessage *pMsg);
void FillActionInfoMessage(NXCPMessage *pMsg, NXC_ACTION *pAction);
void SendActionsToClient(ClientSession *pSession, UINT32 dwRqId);
void CreateActionExportRecord(String &xml, UINT32 id);
bool ImportAction(ConfigEntry *config);

#endif   /* _nms_actions_ */
