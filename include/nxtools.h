/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

/**
 * Tool types.
 * Value 0 is reserved: it was used by the "internal" tool type, whose only implementation
 * was Wake-on-LAN. That tool is now a server script tool calling Node::wakeUp(). Value 0
 * must not be reused - it may still be present in databases not yet upgraded.
 */
#define TOOL_TYPE_ACTION            1
#define TOOL_TYPE_SNMP_TABLE        2
#define TOOL_TYPE_AGENT_LIST        3
#define TOOL_TYPE_URL               4
#define TOOL_TYPE_COMMAND           5
#define TOOL_TYPE_SERVER_COMMAND    6
#define TOOL_TYPE_FILE_DOWNLOAD     7
#define TOOL_TYPE_SERVER_SCRIPT     8
#define TOOL_TYPE_AGENT_TABLE       9
#define TOOL_TYPE_SSH_COMMAND      10

/**
 * Applicable object classes for object tools (bitmask).
 * Not a raw OBJECT_* bit position; translated via ObjectClassToToolMask().
 */
#define TOOL_APPLICABLE_NODE          0x00000001
#define TOOL_APPLICABLE_INTERFACE     0x00000002
#define TOOL_APPLICABLE_SENSOR        0x00000004
#define TOOL_APPLICABLE_ACCESS_POINT  0x00000008
#define TOOL_APPLICABLE_ALL_VALID     0x0000000F

/**
 * Object tool flags
 */
#define TF_ASK_CONFIRMATION         0x00000001
#define TF_GENERATES_OUTPUT         0x00000002
#define TF_DISABLED                 0x00000004
#define TF_SHOW_IN_COMMANDS         0x00000008
#define TF_SNMP_INDEXED_BY_VALUE    0x00000010
#define TF_RUN_IN_CONTAINER_CONTEXT 0x00000020
#define TF_SUPPRESS_SUCCESS_MESSAGE 0x00000040
#define TF_SETUP_TCP_TUNNEL         0x00000080

/**
 * Column formats
 */
#define CFMT_STRING     0
#define CFMT_INTEGER    1
#define CFMT_FLOAT      2
#define CFMT_IP_ADDR    3
#define CFMT_MAC_ADDR   4
#define CFMT_IFINDEX    5

#endif
