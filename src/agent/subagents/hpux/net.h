/* $Id$ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#ifndef __NET_H__
#define __NET_H__

enum
{
	IF_INFO_ADMIN_STATUS,
	IF_INFO_OPER_STATUS,
	IF_INFO_BYTES_IN,
	IF_INFO_BYTES_OUT,
	IF_INFO_DESCRIPTION,
	IF_INFO_IN_ERRORS,
	IF_INFO_OUT_ERRORS,
	IF_INFO_PACKETS_IN,
	IF_INFO_PACKETS_OUT,
	IF_INFO_SPEED,
	IF_INFO_MTU
};


LONG H_NetIfInfo(const char *, const char *, char *, AbstractCommSession *);
LONG H_NetIpForwarding(const char *, const char *, char *, AbstractCommSession *);

LONG H_NetArpCache(const char *, const char *, StringList *, AbstractCommSession *);
LONG H_NetRoutingTable(const char *, const char *, StringList *, AbstractCommSession *);
LONG H_NetIfList(const char *, const char *, StringList *, AbstractCommSession *);

#endif // __NET_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
