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
#define MAX_DCI_STRING_VALUE     256
#define CSCP_HEADER_SIZE         16


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
         WORD wReserved1;
         DWORD dwLen;
         char szValue[1];
      } string;
      struct
      {
         WORD wReserved2;
         DWORD dwInteger;
      } integer;
      struct
      {
         WORD wReserverd3;
         QWORD qwInt64;
      } int64;
      struct
      {
         WORD wReserverd4;
         double dFloat;
      } fp;
      WORD wInt16;
   } data;
} CSCP_DF;


//
// Message structure
//

typedef struct
{
   WORD wCode;       // Message (command) code
   WORD wFlags;      // Message flags
   DWORD dwSize;     // Message size (including header) in bytes
   DWORD dwId;       // Unique message identifier
   DWORD dwNumVars;  // Number of variables in message
   CSCP_DF df[1];    // Data fields
} CSCP_MESSAGE;


//
// DCI data header structure
//

typedef struct
{
   DWORD dwItemId;
   DWORD dwNumRows;
   DWORD dwDataType;
} DCI_DATA_HEADER;


//
// DCI data row structure
//

typedef struct
{
   DWORD dwTimeStamp;
   union
   {
      DWORD dwInteger;
      QWORD qwInt64;
      double dFloat;
      char szString[MAX_DCI_STRING_VALUE];
   } value;
} DCI_DATA_ROW;


//
// DCI threshold structure
//

typedef struct
{
   DWORD dwId;
   DWORD dwEvent;
   DWORD dwArg1;
   DWORD dwArg2;
   union
   {
      DWORD dwInt32;
      INT64 qwInt64;
      double dFloat;
      char szString[MAX_DCI_STRING_VALUE];
   } value;
   WORD wFunction;
   WORD wOperation;
} DCI_THRESHOLD;

#pragma pack()


//
// Data types
//

#define CSCP_DT_INTEGER    0
#define CSCP_DT_STRING     1
#define CSCP_DT_INT64      2
#define CSCP_DT_INT16      3
#define CSCP_DT_BINARY     4
#define CSCP_DT_FLOAT      5


//
// Message flags
//

#define MF_BINARY    0x0001


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
#define CMD_TRAP                    0x0013
#define CMD_OPEN_EPP                0x0014
#define CMD_CLOSE_EPP               0x0015
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
#define CMD_DCI_DATA                0x0032
#define CMD_GET_MIB_LIST            0x0033
#define CMD_GET_MIB                 0x0034
#define CMD_MIB_LIST                0x0035
#define CMD_MIB                     0x0036
#define CMD_CREATE_OBJECT           0x0037
#define CMD_GET_EVENT_NAMES         0x0038
#define CMD_EVENT_NAME_LIST         0x0039
#define CMD_BIND_OBJECT             0x003A
#define CMD_UNBIND_OBJECT           0x003B
#define CMD_GET_IMAGE_LIST          0x003C
#define CMD_LOAD_IMAGE_FILE         0x003D
#define CMD_IMAGE_LIST              0x003E
#define CMD_IMAGE_FILE              0x003F
#define CMD_AUTHENTICATE            0x0040
#define CMD_GET_PARAMETER           0x0041
#define CMD_GET_LIST                0x0042
#define CMD_ACTION                  0x0043
#define CMD_GET_DEFAULT_IMAGE_LIST  0x0044
#define CMD_DEFAULT_IMAGE_LIST      0x0045
#define CMD_GET_ALL_ALARMS          0x0046
#define CMD_GET_ALARM               0x0047
#define CMD_ACK_ALARM               0x0048
#define CMD_ALARM_UPDATE            0x0049
#define CMD_ALARM_DATA              0x004A
#define CMD_DELETE_ALARM            0x004B
#define CMD_LOCK_ACTION_DB          0x004C
#define CMD_UNLOCK_ACTION_DB        0x004D
#define CMD_LOAD_ACTIONS            0x004E
#define CMD_ACTION_DB_UPDATE        0x004F
#define CMD_MODIFY_ACTION           0x0050
#define CMD_CREATE_ACTION           0x0051
#define CMD_DELETE_ACTION           0x0052
#define CMD_ACTION_DATA             0x0053
#define CMD_GET_CONTAINER_CAT_LIST  0x0054
#define CMD_CONTAINER_CAT_DATA      0x0055
#define CMD_DELETE_CONTAINER_CAT    0x0056
#define CMD_CREATE_CONTAINER_CAT    0x0057
#define CMD_MODIFY_CONTAINER_CAT    0x0058


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
#define VID_MAX_ROWS                ((DWORD)50)
#define VID_TIME_FROM               ((DWORD)51)
#define VID_TIME_TO                 ((DWORD)52)
#define VID_DCI_DATA                ((DWORD)53)
#define VID_NUM_THRESHOLDS          ((DWORD)54)
#define VID_DCI_NUM_MAPS            ((DWORD)55)
#define VID_DCI_MAP_IDS             ((DWORD)56)
#define VID_DCI_MAP_INDEXES         ((DWORD)57)
#define VID_NUM_MIBS                ((DWORD)58)
#define VID_MIB_NAME                ((DWORD)59)
#define VID_MIB_FILE_SIZE           ((DWORD)60)
#define VID_MIB_FILE                ((DWORD)61)
#define VID_COMMENT                 ((DWORD)62)
#define VID_ALARM_SEVERITY          ((DWORD)63)
#define VID_ALARM_KEY               ((DWORD)64)
#define VID_ALARM_ACK_KEY           ((DWORD)65)
#define VID_ALARM_MESSAGE           ((DWORD)66)
#define VID_RULE_ID                 ((DWORD)67)
#define VID_NUM_SOURCES             ((DWORD)68)
#define VID_NUM_EVENTS              ((DWORD)69)
#define VID_NUM_ACTIONS             ((DWORD)70)
#define VID_RULE_SOURCES            ((DWORD)71)
#define VID_RULE_EVENTS             ((DWORD)72)
#define VID_RULE_ACTIONS            ((DWORD)73)
#define VID_NUM_RULES               ((DWORD)74)
#define VID_CATEGORY                ((DWORD)75)
#define VID_UPDATED_CHILD_LIST      ((DWORD)76)
#define VID_EVENT_NAME_TABLE        ((DWORD)77)
#define VID_PARENT_ID               ((DWORD)78)
#define VID_CHILD_ID                ((DWORD)79)
#define VID_IMAGE_ID                ((DWORD)80)
#define VID_NUM_IMAGES              ((DWORD)81)
#define VID_IMAGE_LIST              ((DWORD)82)
#define VID_IMAGE_FILE_SIZE         ((DWORD)83)
#define VID_IMAGE_FILE              ((DWORD)84)
#define VID_PARAMETER               ((DWORD)85)
#define VID_NUM_STRINGS             ((DWORD)86)
#define VID_ACTION_NAME             ((DWORD)87)
#define VID_NUM_ARGS                ((DWORD)88)
#define VID_IMAGE_ID_LIST           ((DWORD)89)
#define VID_CLASS_ID_LIST           ((DWORD)90)
#define VID_IMAGE_FORMAT            ((DWORD)91)
#define VID_NOTIFICATION_DATA       ((DWORD)92)
#define VID_ALARM_ID                ((DWORD)93)
#define VID_TIMESTAMP               ((DWORD)94)
#define VID_ACK_BY_USER             ((DWORD)95)
#define VID_IS_ACK                  ((DWORD)96)
#define VID_ACTION_ID               ((DWORD)97)
#define VID_IS_DISABLED             ((DWORD)98)
#define VID_ACTION_TYPE             ((DWORD)99)
#define VID_ACTION_DATA             ((DWORD)100)
#define VID_EMAIL_SUBJECT           ((DWORD)101)
#define VID_RCPT_ADDR               ((DWORD)102)
#define VID_CATEGORY_NAME           ((DWORD)103)
#define VID_CATEGORY_ID             ((DWORD)104)
#define VID_DCI_DELTA_CALCULATION   ((DWORD)105)
#define VID_DCI_FORMULA             ((DWORD)106)

// Variable ranges for object's ACL
#define VID_ACL_USER_BASE           ((DWORD)0x00001000)
#define VID_ACL_USER_LAST           ((DWORD)0x00001FFF)
#define VID_ACL_RIGHTS_BASE         ((DWORD)0x00002000)
#define VID_ACL_RIGHTS_LAST         ((DWORD)0x00002FFF)

// Variable range for user group members
#define VID_GROUP_MEMBER_BASE       ((DWORD)0x00004000)
#define VID_GROUP_MEMBER_LAST       ((DWORD)0x00004FFF)

// Variable range for data collection thresholds
#define VID_DCI_THRESHOLD_BASE      ((DWORD)0x00005000)
#define VID_DCI_THRESHOLD_LAST      ((DWORD)0x00005FFF)

// Variable range for MIB list
#define VID_MIB_NAME_BASE           ((DWORD)0x00006000)
#define VID_MIB_NAME_LAST           ((DWORD)0x00006FFF)
#define VID_MIB_HASH_BASE           ((DWORD)0x00007000)
#define VID_MIB_HASH_LAST           ((DWORD)0x00007FFF)

// Object information can contain variable number of parent and child objects' ids.
// Because each variable in message have to have unique identifier,
// we reserver a two range ids for this variables.
#define VID_PARENT_ID_BASE          ((DWORD)0x00003000)
#define VID_PARENT_ID_LAST          ((DWORD)0x00003FFF)

// Reservation of 0x7FFFFFFF ids for child object's list
#define VID_CHILD_ID_BASE           ((DWORD)0x80000000)
#define VID_CHILD_ID_LAST           ((DWORD)0xFFFFFFFE)

// Base value for agent's enum values
#define VID_ENUM_VALUE_BASE         ((DWORD)0x10000000)

// Base value for agent's action arguments
#define VID_ACTION_ARG_BASE         ((DWORD)0x10000000)


//
// Inline functions
//

inline BOOL IsBinaryMsg(CSCP_MESSAGE *pMsg)
{
   return ntohs(pMsg->wFlags) & MF_BINARY;
}


#endif   /* _nms_cscp_h_ */
