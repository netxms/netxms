/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: inline.cpp
**
**/

#define __libnxcl_inline_c__
#include "libnxcl.h"


//
// All functions that implemented as inline for C++
//

extern "C"
{
   DWORD LIBNXCL_EXPORTABLE NXCSyncObjects(void) { return NXCRequest(NXC_OP_SYNC_OBJECTS); }
   DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(void) { return NXCRequest(NXC_OP_SYNC_EVENTS); }
   DWORD LIBNXCL_EXPORTABLE NXCOpenEventDB(void) { return NXCRequest(NXC_OP_OPEN_EVENT_DB); }
   DWORD LIBNXCL_EXPORTABLE NXCCloseEventDB(BOOL bSaveChanges) { return NXCRequest(NXC_OP_CLOSE_EVENT_DB, bSaveChanges); }
   DWORD LIBNXCL_EXPORTABLE NXCModifyObject(NXC_OBJECT_UPDATE *pData) { return NXCRequest(NXC_OP_MODIFY_OBJECT, pData); }
   DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(void) { return NXCRequest(NXC_OP_LOAD_USER_DB); }
   DWORD LIBNXCL_EXPORTABLE NXCCreateUser(char *pszName) { return NXCRequest(NXC_OP_CREATE_USER, FALSE, pszName); }
   DWORD LIBNXCL_EXPORTABLE NXCCreateUserGroup(char *pszName) { return NXCRequest(NXC_OP_CREATE_USER, TRUE, pszName); }
   DWORD LIBNXCL_EXPORTABLE NXCDeleteUser(DWORD dwId) { return NXCRequest(NXC_OP_DELETE_USER, dwId); }
   DWORD LIBNXCL_EXPORTABLE NXCLockUserDB(void) { return NXCRequest(NXC_OP_LOCK_USER_DB, TRUE); }
   DWORD LIBNXCL_EXPORTABLE NXCUnlockUserDB(void) { return NXCRequest(NXC_OP_LOCK_USER_DB, FALSE); }
}
