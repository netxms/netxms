/* 
** NetXMS - Network Management System
** Ethernet/IP support library
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: libetherip.h
**
**/

#ifndef _libetherip_h_
#define _libetherip_h_

#include <nms_common.h>
#include <nms_util.h>
#include <ethernet_ip.h>

#define LIBETHERIP_DEBUG_TAG   _T("etherip.lib")

EIP_Message *EIP_DoRequest(SOCKET s, const EIP_Message &request, uint32_t timeout, EIP_Status *status);

#endif   /* _libetherip_h_ */
