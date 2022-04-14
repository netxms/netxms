/* 
** NetXMS - Network Management System
** Local administration tool
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
** $module: nxadm.h
**
**/

#ifndef _nxadm_h_
#define _nxadm_h_

#include <stdio.h>
#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <local_admin.h>

bool Connect();
void Disconnect();
void SendMsg(const NXCPMessage& msg);
NXCPMessage *RecvMsg(void);

extern SOCKET g_socket;
extern uint32_t g_requestId;

#endif
