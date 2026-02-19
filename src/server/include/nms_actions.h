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
   wchar_t name[MAX_OBJECT_NAME];
   wchar_t rcptAddr[MAX_RCPT_ADDR_LEN];
   wchar_t emailSubject[MAX_EMAIL_SUBJECT_LEN];
   wchar_t *data;
   wchar_t channelName[MAX_OBJECT_NAME];

   Action(const wchar_t *name);
   Action(DB_RESULT hResult, int row);
   Action(const Action& src);

   ~Action()
   {
      MemFree(data);
   }

   void fillMessage(NXCPMessage *msg) const;
   void saveToDatabase() const;
   json_t *toJson() const;
};

class ImportContext;

//
// Functions
//
bool LoadActions();
void CleanupActions();
void ExecuteAction(uint32_t actionId, const Event& event, const Alarm *alarm, const uuid& ruleId);
uint32_t NXCORE_EXPORTABLE CreateAction(const TCHAR *name, uint32_t *id);
uint32_t NXCORE_EXPORTABLE DeleteAction(uint32_t actionId);
uint32_t ModifyActionFromMessage(const NXCPMessage& msg);
uint32_t NXCORE_EXPORTABLE ModifyActionFromJson(uint32_t actionId, json_t *json, json_t **oldValue, json_t **newValue);
void NXCORE_EXPORTABLE UpdateChannelNameInActions(std::pair<TCHAR*, TCHAR*> *names);
bool NXCORE_EXPORTABLE CheckChannelIsUsedInAction(TCHAR *name);
void SendActionsToClient(ClientSession *session, uint32_t requestId);
json_t *CreateActionExportRecord(uint32_t id);
bool ImportAction(ConfigEntry *config, bool overwrite, ImportContext *context);
bool ImportAction(json_t *action, bool overwrite, ImportContext *context);
json_t NXCORE_EXPORTABLE *GetActions();
json_t NXCORE_EXPORTABLE *GetActionById(uint32_t id);
bool NXCORE_EXPORTABLE IsValidActionId(uint32_t id);
uuid NXCORE_EXPORTABLE GetActionGUID(uint32_t id);
uint32_t NXCORE_EXPORTABLE FindActionByGUID(const uuid& guid);

#endif   /* _nms_actions_ */
