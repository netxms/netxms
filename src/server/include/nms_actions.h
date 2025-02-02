/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
   uint32_t id;
   uuid guid;
   ServerActionType type;
   bool isDisabled;
   TCHAR name[MAX_OBJECT_NAME];
   TCHAR rcptAddr[MAX_RCPT_ADDR_LEN];
   TCHAR emailSubject[MAX_EMAIL_SUBJECT_LEN];
   TCHAR *data;
   TCHAR channelName[MAX_OBJECT_NAME];

   Action(const TCHAR *name);
   Action(DB_RESULT hResult, int row);
   Action(const Action& src);

   ~Action()
   {
      MemFree(data);
   }

   void fillMessage(NXCPMessage *msg) const;
   void saveToDatabase() const;
};

class ImportContext;

//
// Functions
//
bool LoadActions();
void CleanupActions();
void ExecuteAction(uint32_t actionId, const Event& event, const Alarm *alarm, const uuid& ruleId);
uint32_t CreateAction(const TCHAR *name, uint32_t *id);
uint32_t DeleteAction(uint32_t actionId);
uint32_t ModifyActionFromMessage(const NXCPMessage& msg);
void UpdateChannelNameInActions(std::pair<TCHAR*, TCHAR*> *names);
bool CheckChannelIsUsedInAction(TCHAR *name);
void SendActionsToClient(ClientSession *session, uint32_t requestId);
void CreateActionExportRecord(TextFileWriter& xml, uint32_t id);
bool ImportAction(ConfigEntry *config, bool overwrite, ImportContext *context);
bool IsValidActionId(uint32_t id);
uuid GetActionGUID(uint32_t id);
uint32_t FindActionByGUID(const uuid& guid);

#endif   /* _nms_actions_ */
