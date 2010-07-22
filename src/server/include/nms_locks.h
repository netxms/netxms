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
** $module: nms_locks.h
**
**/

#ifndef _nms_locks_h_
#define _nms_locks_h_


#define UNLOCKED           ((DWORD)0xFFFFFFFF)


//
// Component identifiers used for locking
//

#define CID_EPP               0
#define CID_USER_DB           1
//deprecated: #define CID_EVENT_DB          2
//deprecated: #define CID_ACTION_DB         3
#define CID_TRAP_CFG          4
#define CID_PACKAGE_DB        5
#define CID_OBJECT_TOOLS      6


//
// Functions
//

#ifndef _NETXMS_DB_SCHEMA_

BOOL InitLocks(DWORD *pdwIpAddr, char *pszInfo);
BOOL LockComponent(DWORD dwId, DWORD dwLockBy, const char *pszOwnerInfo, DWORD *pdwCurrentOwner, char *pszCurrentOwnerInfo);
void UnlockComponent(DWORD dwId);
void RemoveAllSessionLocks(DWORD dwSessionId);
BOOL LockLPP(DWORD dwPolicyId, DWORD dwSessionId);
void UnlockLPP(DWORD dwPolicyId, DWORD dwSessionId);
void NXCORE_EXPORTABLE UnlockDB(void);

#endif

#endif
