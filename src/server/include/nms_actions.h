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

/**
 * Action configuration
 */
struct Action
{
   UINT32 id;
   uuid guid;
   int type;
   bool isDisabled;
   TCHAR name[MAX_OBJECT_NAME];
   TCHAR rcptAddr[MAX_RCPT_ADDR_LEN];
   TCHAR emailSubject[MAX_EMAIL_SUBJECT_LEN];
   TCHAR *data;

   Action(const TCHAR *name);
   Action(DB_RESULT hResult, int row);
   ~Action();

   void fillMessage(NXCPMessage *msg) const;
   void saveToDatabase() const;
};

//
// Functions
//
BOOL InitActions();
void CleanupActions();
BOOL ExecuteAction(UINT32 dwActionId, Event *pEvent, const TCHAR *alarmMsg, const TCHAR *alarmKey);
UINT32 CreateAction(const TCHAR *pszName, UINT32 *pdwId);
UINT32 DeleteAction(UINT32 dwActionId);
UINT32 ModifyActionFromMessage(NXCPMessage *pMsg);
void SendActionsToClient(ClientSession *pSession, UINT32 dwRqId);
void CreateActionExportRecord(String &xml, UINT32 id);
bool ImportAction(ConfigEntry *config);
bool IsValidActionId(UINT32 id);
uuid GetActionGUID(UINT32 id);
UINT32 FindActionByGUID(const uuid& guid);

#endif   /* _nms_actions_ */
