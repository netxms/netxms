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
// Constants
//

#define MAX_EMAIL_ADDR_LEN       256
#define MAX_EMAIL_SUBJECT_LEN    256


//
// Action types
//

#define ACTION_EXEC           1
#define ACTION_SEND_EMAIL     2
#define ACTION_REMOTE         3


//
// Action structure
//

struct NMS_ACTION
{
   DWORD dwId;
   int iType;
   char szEmailAddr[MAX_EMAIL_ADDR_LEN];
   char szEmailSubject[MAX_EMAIL_SUBJECT_LEN];
   char *pszData;
};


//
// Functions
//

BOOL LoadActions(void);
BOOL ExecuteAction(DWORD dwActionId, Event *pEvent);
void DestroyActionList(void);

#endif   /* _nms_actions_ */
