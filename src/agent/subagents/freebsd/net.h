/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2008 Mark Ibell
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

#ifndef __NET_H__
#define __NET_H__

enum InterfaceMetrict
{
	IF_INFO_ADMIN_STATUS,
	IF_INFO_OPER_STATUS,
	IF_INFO_BYTES_IN,
	IF_INFO_BYTES_IN_64,
	IF_INFO_BYTES_OUT,
	IF_INFO_BYTES_OUT_64,
	IF_INFO_DESCRIPTION,
	IF_INFO_IN_ERRORS,
	IF_INFO_IN_ERRORS_64,
	IF_INFO_OUT_ERRORS,
	IF_INFO_OUT_ERRORS_64,
	IF_INFO_PACKETS_IN,
	IF_INFO_PACKETS_IN_64,
	IF_INFO_PACKETS_OUT,
	IF_INFO_PACKETS_OUT_64,
	IF_INFO_SPEED
};

LONG H_NetIpForwarding(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetIfAdminStatus(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetIfOperStatus(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetArpCache(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetIfList(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetRoutingTable(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
LONG H_NetIfInfoFromKVM(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
LONG H_NetInterface64bitSupport(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);

#endif // __NET_H__
