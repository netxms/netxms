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
** File: nxtools.h
**
**/

#ifndef _nxtools_h
#define _nxtools_h


//
// Tool types
//

#define TOOL_TYPE_INTERNAL          0
#define TOOL_TYPE_ACTION            1
#define TOOL_TYPE_TABLE_SNMP        2
#define TOOL_TYPE_TABLE_AGENT       3
#define TOOL_TYPE_URL               4
#define TOOL_TYPE_COMMAND           5
#define TOOL_TYPE_SERVER_COMMAND    6


//
// Object tool flags
//

#define TF_REQUIRES_SNMP            ((UINT32)0x00000001)
#define TF_REQUIRES_AGENT           ((UINT32)0x00000002)
#define TF_REQUIRES_OID_MATCH       ((UINT32)0x00000004)
#define TF_ASK_CONFIRMATION         ((UINT32)0x00000008)
#define TF_GENERATES_OUTPUT         ((UINT32)0x00000010)
#define TF_DISABLED                 ((UINT32)0x00000020)
#define TF_SHOW_IN_COMMANDS         ((UINT32)0x00000040)
#define TF_SNMP_INDEXED_BY_VALUE    ((UINT32)0x00010000)


//
// Column formats
//

#define CFMT_STRING     0
#define CFMT_INTEGER    1
#define CFMT_FLOAT      2
#define CFMT_IP_ADDR    3
#define CFMT_MAC_ADDR   4
#define CFMT_IFINDEX    5


#endif
