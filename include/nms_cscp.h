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
** $module: nms_cscp.h
**
**/

#ifndef _nms_cscp_h_
#define _nms_cscp_h_


//
// Constants
//

#define SERVER_LISTEN_PORT       4701


//
// Data field structure
//

#pragma pack(1)

typedef struct
{
   DWORD dwVarId;       // Variable identifier
   BYTE  bType;         // Data type
   BYTE  bReserved;     // Padding
   union
   {
      struct
      {
         WORD wLen;
         char szValue[1];
      } string;
      struct
      {
         WORD wReserved1;
         DWORD dwInteger;
      } integer;
      struct
      {
         WORD wReserverd2;
         QWORD qwInt64;
      } int64;
      WORD wInt16;
   } data;
} CSCP_DF;


//
// Message structure
//

typedef struct
{
   WORD wCode;    // Message (command) code (lower 12 bits) and flags (higher 4 bits)
   WORD wSize;    // Message size (including header) in bytes
   DWORD dwId;    // Unique message identifier
   CSCP_DF df[1]; // Data fields
} CSCP_MESSAGE;

#pragma pack()


//
// Data types
//

#define DT_INTEGER   0
#define DT_STRING    1
#define DT_INT64     2
#define DT_INT16     3
#define DT_BINARY    4


//
// Message flags
//

#define MF_BINARY                   0x1000


//
// Message (command) codes
//

#define CMD_LOGIN                   0x0001
#define CMD_LOGIN_RESP              0x0002
#define CMD_KEEPALIVE               0x0003
#define CMD_EVENT                   0x0004
#define CMD_GET_OBJECTS             0x0005
#define CMD_OBJECT                  0x0006
#define CMD_DELETE_OBJECT           0x0007
#define CMD_MODIFY_OBJECT           0x0008
#define CMD_OBJECT_LIST_END         0x0009
#define CMD_OBJECT_UPDATE           0x000A
#define CMD_GET_EVENTS              0x000B
#define CMD_EVENT_LIST_END          0x000C
#define CMD_GET_CONFIG_VARLIST      0x000D
#define CMD_SET_CONFIG_VARIABLE     0x000E
#define CMD_CONFIG_VARIABLE         0x000F
#define CMD_CONFIG_VARLIST_END      0x0010
#define CMD_DELETE_CONFIG_VARIABLE  0x0011
#define CMD_NOTIFY                  0x0012
#define CMD_OPEN_EPP                0x0013
#define CMD_CLOSE_EPP               0x0014
#define CMD_INSTALL_EPP             0x0015
#define CMD_SAVE_EPP                0x0016
#define CMD_EPP_RECORD              0x0017
#define CMD_OPEN_EVENT_DB           0x0018
#define CMD_CLOSE_EVENT_DB          0x0019
#define CMD_SET_EVENT_INFO          0x001A
#define CMD_EVENT_DB_RECORD         0x001B
#define CMD_EVENT_DB_EOF            0x001C
#define CMD_REQUEST_COMPLETED       0x001D
#define CMD_LOAD_USER_DB            0x001E
#define CMD_USER_DATA               0x001F
#define CMD_GROUP_DATA              0x0020
#define CMD_USER_DB_EOF             0x0021
#define CMD_UPDATE_USER             0x0022
#define CMD_DELETE_USER             0x0023
#define CMD_CREATE_USER             0x0024
#define CMD_LOCK_USER_DB            0x0025
#define CMD_UNLOCK_USER_DB          0x0026
#define CMD_USER_DB_UPDATE          0x0027
#define CMD_SET_PASSWORD            0x0028
#define CMD_GET_NODE_DCI_LIST       0x0029
#define CMD_NODE_DCI                0x002A
#define CMD_NODE_DCI_LIST_END       0x002B
#define CMD_DELETE_NODE_DCI         0x002C
#define CMD_MODIFY_NODE_DCI         0x002D
#define CMD_UNLOCK_NODE_DCI_LIST    0x002E
#define CMD_SET_OBJECT_MGMT_STATUS  0x002F
#define CMD_CREATE_NEW_DCI          0x0030
#define CMD_GET_DCI_DATA            0x0031


//
// Variable identifiers
//

#define VID_LOGIN_NAME              ((DWORD)1)
#define VID_PASSWORD                ((DWORD)2)
#define VID_OBJECT_ID               ((DWORD)3)
#define VID_OBJECT_NAME             ((DWORD)4)
#define VID_OBJECT_CLASS            ((DWORD)5)
#define VID_SNMP_VERSION            ((DWORD)6)
#define VID_PARENT_CNT              ((DWORD)7)
#define VID_IP_ADDRESS              ((DWORD)8)
#define VID_IP_NETMASK              ((DWORD)9)
#define VID_OBJECT_STATUS           ((DWORD)10)
#define VID_IF_INDEX                ((DWORD)11)
#define VID_IF_TYPE                 ((DWORD)12)
#define VID_FLAGS                   ((DWORD)13)
#define VID_DISCOVERY_FLAGS         ((DWORD)14)
#define VID_AGENT_PORT              ((DWORD)15)
#define VID_AUTH_METHOD             ((DWORD)16)
#define VID_SHARED_SECRET           ((DWORD)17)
#define VID_COMMUNITY_STRING        ((DWORD)18)
#define VID_SNMP_OID                ((DWORD)19)
#define VID_NAME                    ((DWORD)20)
#define VID_VALUE                   ((DWORD)21)
#define VID_ERROR                   ((DWORD)22)
#define VID_NOTIFICATION_CODE       ((DWORD)23)
#define VID_EVENT_ID                ((DWORD)24)
#define VID_SEVERITY                ((DWORD)25)
#define VID_MESSAGE                 ((DWORD)26)
#define VID_DESCRIPTION             ((DWORD)27)
#define VID_RCC                     ((DWORD)28)    /* RCC == Request Completion Code */
#define VID_LOCKED_BY               ((DWORD)29)
#define VID_IS_DELETED              ((DWORD)30)
#define VID_CHILD_CNT               ((DWORD)31)
#define VID_ACL_SIZE                ((DWORD)32)
#define VID_INHERIT_RIGHTS          ((DWORD)33)
#define VID_USER_NAME               ((DWORD)34)
#define VID_USER_ID                 ((DWORD)35)
#define VID_USER_SYS_RIGHTS         ((DWORD)36)
#define VID_USER_FLAGS              ((DWORD)37)
#define VID_NUM_MEMBERS             ((DWORD)38)    /* Number of members in users group */
#define VID_IS_GROUP                ((DWORD)39)
#define VID_USER_FULL_NAME          ((DWORD)40)
#define VID_USER_DESCRIPTION        ((DWORD)41)
#define VID_UPDATE_TYPE             ((DWORD)42)
#define VID_DCI_ID                  ((DWORD)43)
#define VID_POLLING_INTERVAL        ((DWORD)44)
#define VID_RETENTION_TIME          ((DWORD)45)
#define VID_DCI_SOURCE_TYPE         ((DWORD)46)
#define VID_DCI_DATA_TYPE           ((DWORD)47)
#define VID_DCI_STATUS              ((DWORD)48)
#define VID_MGMT_STATUS             ((DWORD)49)

// Variable ranges for object's ACL
#define VID_ACL_USER_BASE           ((DWORD)0x00001000)
#define VID_ACL_USER_LAST           ((DWORD)0x00001FFF)
#define VID_ACL_RIGHTS_BASE         ((DWORD)0x00002000)
#define VID_ACL_RIGHTS_LAST         ((DWORD)0x00002FFF)

// Variable range for user group members
#define VID_GROUP_MEMBER_BASE       ((DWORD)0x00004000)
#define VID_GROUP_MEMBER_LAST       ((DWORD)0x00004FFF)

// Object information can contain variable number of parent and child objects' ids.
// Because each variable in message have to have unique identifier,
// we reserver a two range ids for this variables.
#define VID_PARENT_ID_BASE          ((DWORD)0x00003000)
#define VID_PARENT_ID_LAST          ((DWORD)0x00003FFF)

// Reservation of 0x7FFFFFFF ids for child object's list
#define VID_CHILD_ID_BASE           ((DWORD)0x80000000)
#define VID_CHILD_ID_LAST           ((DWORD)0xFFFFFFFE)


//
// Inline functions
//

inline BOOL IsBinaryMsg(CSCP_MESSAGE *pMsg)
{
   return ntohs(pMsg->wCode) & MF_BINARY;
}


#endif   /* _nms_cscp_h_ */
