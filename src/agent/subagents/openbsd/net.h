/* $Id: net.h,v 1.1 2006-03-07 09:42:48 alk Exp $ */

/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
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

LONG H_NetIpForwarding(char *, char *, char *);
LONG H_NetIfAdmStatus(char *, char *, char *);
LONG H_NetIfLink(char *, char *, char *);
LONG H_NetArpCache(char *, char *, NETXMS_VALUES_LIST *);
LONG H_NetIfList(char *, char *, NETXMS_VALUES_LIST *);
LONG H_NetRoutingTable(char *, char *, NETXMS_VALUES_LIST *);

#endif // __NET_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2005/08/22 23:00:05  alk
Net.IP.RoutingTable added

Revision 1.2  2005/03/10 19:04:07  alk
implemented:
	Net.Interface.AdminStatus(*)
	Net.Interface.Link(*)

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

Revision 1.1  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
