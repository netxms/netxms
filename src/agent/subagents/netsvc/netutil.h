/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
**/

#ifndef __netutil__h__
#define __netutil__h__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>

SOCKET NetConnectTCP(const char* hostname, const InetAddress& addr, uint16_t port, uint32_t dwTimeout);
bool NetWrite(SOCKET hSocket, const void* data, size_t size);
ssize_t NetRead(SOCKET hSocket, void* buffer, size_t size);
void NetClose(SOCKET nSocket);

extern uint32_t g_netsvcTimeout;

#endif // __netsvc__h__
