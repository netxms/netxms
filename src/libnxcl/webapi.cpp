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
** $module: webapi.cpp
**
**/

#include "libnxcl.h"


//
// Attach current session to existing WEB session
//

DWORD LIBNXCL_EXPORTABLE NXCAttachWebSession(QWORD qwSID)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_WEBAPI_ATTACH);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_WEBAPI_SID, qwSID);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}
