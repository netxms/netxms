/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: nxevent.h
**
**/

#ifndef _nxevent_h_
#define _nxevent_h_

//
// Event flags
//

#define EF_LOG                   0x00000001


//
// Event flags used by client library (has no meaning for server)
//

#define EF_MODIFIED              0x01000000
#define EF_DELETED               0x02000000

#define SERVER_FLAGS_BITS        0x00FFFFFF


//
// Event severity codes
//

#define EVENT_SEVERITY_INFO      0
#define EVENT_SEVERITY_WARNING   1
#define EVENT_SEVERITY_MINOR     2
#define EVENT_SEVERITY_MAJOR     3
#define EVENT_SEVERITY_CRITICAL  4


//
// System-defined events
//

#define EVENT_NODE_ADDED         1
#define EVENT_SUBNET_ADDED       2
#define EVENT_INTERFACE_ADDED    3
#define EVENT_INTERFACE_UP       4
#define EVENT_INTERFACE_DOWN     5
#define EVENT_NODE_NORMAL        6
#define EVENT_NODE_WARNING       7
#define EVENT_NODE_MINOR         8
#define EVENT_NODE_MAJOR         9
#define EVENT_NODE_CRITICAL      10
#define EVENT_NODE_UNKNOWN       11
#define EVENT_NODE_UNMANAGED     12
#define EVENT_NODE_FLAGS_CHANGED 13
#define EVENT_SNMP_FAIL          14
#define EVENT_AGENT_FAIL         15
#define EVENT_INTERFACE_DELETED  16
#define EVENT_THRESHOLD_REACHED  17
#define EVENT_THRESHOLD_REARMED  18
#define EVENT_SUBNET_DELETED     19
#define EVENT_THREAD_HANGS       20

#endif
