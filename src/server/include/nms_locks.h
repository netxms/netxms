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


#define UNLOCKED           ((UINT32)0xFFFFFFFF)

/**
 * Component identifiers used for locking
 */
#define CID_EPP 0
#define CID_USER_DB 1
#define CID_PACKAGE_DB 2
#define CID_CUSTOM_1 3
#define CID_CUSTOM_2 4
#define CID_CUSTOM_3 5
#define CID_CUSTOM_4 6
#define CID_CUSTOM_5 7
#define CID_CUSTOM_6 8
#define CID_CUSTOM_7 9
#define CID_CUSTOM_8 10

/*** Functions ***/
#ifndef _NETXMS_DB_SCHEMA_

BOOL InitLocks(UINT32 *pdwIpAddr, TCHAR *pszInfo);
BOOL LockComponent(UINT32 dwId, int sessionId, const TCHAR *pszOwnerInfo, UINT32 *pdwCurrentOwner, TCHAR *pszCurrentOwnerInfo);
void UnlockComponent(UINT32 dwId);
void RemoveAllSessionLocks(int sessionId);
BOOL LockLPP(UINT32 dwPolicyId, int sessionId);
void UnlockLPP(UINT32 dwPolicyId, int sessionId);
void NXCORE_EXPORTABLE UnlockDB();

#endif

#endif
