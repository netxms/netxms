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
** File: nms_locks.h
**
**/

#ifndef _nms_locks_h_
#define _nms_locks_h_


#define UNLOCKED           ((session_id_t)-1)

/*** Functions ***/
#ifndef _NETXMS_DB_SCHEMA_

BOOL LockEPP(session_id_t sessionId, const TCHAR *pszOwnerInfo, session_id_t *currentOwner, TCHAR *pszCurrentOwnerInfo);
void UnlockEPP();
void RemoveAllSessionLocks(session_id_t sessionId);

#endif

#endif
