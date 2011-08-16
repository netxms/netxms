/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: nms_cscp.h
**
**/

#ifndef _nms_cscp_h_
#define _nms_cscp_h_


//
// Constants
//

#define NXCP_VERSION                   2

#define SERVER_LISTEN_PORT             4701
#define MAX_DCI_STRING_VALUE           256
#define CLIENT_CHALLENGE_SIZE				256
#define CSCP_HEADER_SIZE               16
#define CSCP_ENCRYPTION_HEADER_SIZE    16
#define CSCP_EH_UNENCRYPTED_BYTES      8
#define CSCP_EH_ENCRYPTED_BYTES        (CSCP_ENCRYPTION_HEADER_SIZE - CSCP_EH_UNENCRYPTED_BYTES)
#ifdef __64BIT__
#define PROXY_ENCRYPTION_CTX           ((CSCP_ENCRYPTION_CONTEXT *)_ULL(0xFFFFFFFFFFFFFFFF))
#else
#define PROXY_ENCRYPTION_CTX           ((CSCP_ENCRYPTION_CONTEXT *)0xFFFFFFFF)
#endif

#ifndef EVP_MAX_IV_LENGTH
#define EVP_MAX_IV_LENGTH              16
#endif

#define RECORD_ORDER_NORMAL            0
#define RECORD_ORDER_REVERSED          1

#define CSCP_TEMP_BUF_SIZE             65536


//
// Ciphers
//

#define CSCP_CIPHER_AES_256      0
#define CSCP_CIPHER_BLOWFISH     1
#define CSCP_CIPHER_IDEA         2
#define CSCP_CIPHER_3DES         3

#define CSCP_SUPPORT_AES_256     0x01
#define CSCP_SUPPORT_BLOWFISH    0x02
#define CSCP_SUPPORT_IDEA        0x04
#define CSCP_SUPPORT_3DES        0x08


//
// Data field structure
//

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

typedef struct
{
   DWORD dwVarId;       // Variable identifier
   BYTE  bType;         // Data type
   BYTE  bPadding;      // Padding
   WORD wInt16;
   union
   {
      DWORD dwInteger;
      QWORD qwInt64;
      double dFloat;
      struct
      {
         DWORD dwLen;
         WORD szValue[1];
      } string;
   } data;
} CSCP_DF;

#define df_int16  wInt16
#define df_int32  data.dwInteger
#define df_int64  data.qwInt64
#define df_real   data.dFloat
#define df_string data.string


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
// Encrypted payload header
//

typedef struct
{
   DWORD dwChecksum;
   DWORD dwReserved; // Align to 8-byte boundary
} CSCP_ENCRYPTED_PAYLOAD_HEADER;


//
// Encrypted message structure
//

typedef struct
{
   WORD wCode;       // Should be CMD_ENCRYPTED_MESSAGE
   BYTE nPadding;    // Number of bytes added to the end of message
   BYTE nReserved;
   DWORD dwSize;     // Size of encrypted message (including encryption header and padding)
   BYTE data[1];     // Encrypted payload
} CSCP_ENCRYPTED_MESSAGE;


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
      UCS2CHAR szString[MAX_DCI_STRING_VALUE];
   } value;
} DCI_DATA_ROW;

#ifdef __HP_aCC
#pragma pack
#else
#pragma pack()
#endif


//
// CSCP encryption context
//

typedef struct
{
   int nCipher;            // Encryption algorithm
   BYTE *pSessionKey;      // Current session key
   int nKeyLen;            // Session key length in bytes
   BYTE iv[EVP_MAX_IV_LENGTH];   // Current IV
} CSCP_ENCRYPTION_CONTEXT;


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

#define MF_BINARY          0x0001
#define MF_END_OF_FILE     0x0002
#define MF_DONT_ENCRYPT    0x0004
#define MF_END_OF_SEQUENCE 0x0008
#define MF_REVERSE_ORDER   0x0010
#define MF_CONTROL         0x0020


//
// Message (command) codes
//

#define CMD_LOGIN                   0x0001
#define CMD_LOGIN_RESP              0x0002
#define CMD_KEEPALIVE               0x0003
#define CMD_SET_ALARM_HD_STATE      0x0004
#define CMD_GET_OBJECTS             0x0005
#define CMD_OBJECT                  0x0006
#define CMD_DELETE_OBJECT           0x0007
#define CMD_MODIFY_OBJECT           0x0008
#define CMD_OBJECT_LIST_END         0x0009
#define CMD_OBJECT_UPDATE           0x000A
#define CMD_GET_EVENTS              0x000B
#define CMD_EVENTLOG_RECORDS        0x000C
#define CMD_GET_CONFIG_VARLIST      0x000D
#define CMD_SET_CONFIG_VARIABLE     0x000E
#define CMD_GET_OBJECT_TOOLS        0x000F
#define CMD_EXECUTE_ACTION          0x0010
#define CMD_DELETE_CONFIG_VARIABLE  0x0011
#define CMD_NOTIFY                  0x0012
#define CMD_TRAP                    0x0013
#define CMD_OPEN_EPP                0x0014
#define CMD_CLOSE_EPP               0x0015
#define CMD_SAVE_EPP                0x0016
#define CMD_EPP_RECORD              0x0017
#define CMD_EVENT_DB_UPDATE         0x0018
#define CMD_TRAP_CFG_UPDATE         0x0019
#define CMD_SET_EVENT_INFO          0x001A
#define CMD_EVENT_DB_RECORD         0x001B
#define CMD_LOAD_EVENT_DB           0x001C
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
#define CMD_GET_LOG_DATA            0x002B
#define CMD_DELETE_NODE_DCI         0x002C
#define CMD_MODIFY_NODE_DCI         0x002D
#define CMD_UNLOCK_NODE_DCI_LIST    0x002E
#define CMD_SET_OBJECT_MGMT_STATUS  0x002F
#define CMD_CREATE_NEW_DCI          0x0030
#define CMD_GET_DCI_DATA            0x0031
#define CMD_DCI_DATA                0x0032
#define CMD_GET_MIB_TIMESTAMP       0x0033
#define CMD_GET_MIB                 0x0034
#define CMD_TEST_DCI_TRANSFORMATION 0x0035
#define CMD_GET_JOB_LIST            0x0036
#define CMD_CREATE_OBJECT           0x0037
#define CMD_GET_EVENT_NAMES         0x0038
#define CMD_EVENT_NAME_LIST         0x0039
#define CMD_BIND_OBJECT             0x003A
#define CMD_UNBIND_OBJECT           0x003B
#define CMD_UNINSTALL_AGENT_POLICY  0x003C
#define CMD_OPEN_SERVER_LOG         0x003D
#define CMD_CLOSE_SERVER_LOG        0x003E
#define CMD_QUERY_LOG               0x003F
#define CMD_AUTHENTICATE            0x0040
#define CMD_GET_PARAMETER           0x0041
#define CMD_GET_LIST                0x0042
#define CMD_ACTION                  0x0043
#define CMD_GET_CURRENT_USER_ATTR   0x0044
#define CMD_SET_CURRENT_USER_ATTR   0x0045
#define CMD_GET_ALL_ALARMS          0x0046
#define CMD_GET_ALARM               0x0047
#define CMD_ACK_ALARM               0x0048
#define CMD_ALARM_UPDATE            0x0049
#define CMD_ALARM_DATA              0x004A
#define CMD_DELETE_ALARM            0x004B
#define CMD_ADD_CLUSTER_NODE        0x004C
#define CMD_GET_POLICY_INVENTORY    0x004D
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
#define CMD_POLL_NODE               0x0059
#define CMD_POLLING_INFO            0x005A
#define CMD_COPY_DCI                0x005B
#define CMD_WAKEUP_NODE             0x005C
#define CMD_DELETE_EVENT_TEMPLATE   0x005D
#define CMD_GENERATE_EVENT_CODE     0x005E
#define CMD_FIND_NODE_CONNECTION    0x005F
#define CMD_FIND_MAC_LOCATION       0x0060
#define CMD_CREATE_TRAP             0x0061
#define CMD_MODIFY_TRAP             0x0062
#define CMD_DELETE_TRAP             0x0063
#define CMD_LOAD_TRAP_CFG           0x0064
#define CMD_TRAP_CFG_RECORD         0x0065
#define CMD_QUERY_PARAMETER         0x0066
#define CMD_GET_SERVER_INFO         0x0067
#define CMD_SET_DCI_STATUS          0x0068
#define CMD_FILE_DATA               0x0069
#define CMD_TRANSFER_FILE           0x006A
#define CMD_UPGRADE_AGENT           0x006B
#define CMD_GET_PACKAGE_LIST        0x006C
#define CMD_PACKAGE_INFO            0x006D
#define CMD_REMOVE_PACKAGE          0x006E
#define CMD_INSTALL_PACKAGE         0x006F
#define CMD_LOCK_PACKAGE_DB         0x0070
#define CMD_UNLOCK_PACKAGE_DB       0x0071
#define CMD_ABORT_FILE_TRANSFER     0x0072
#define CMD_CHECK_NETWORK_SERVICE   0x0073
#define CMD_GET_AGENT_CONFIG        0x0074
#define CMD_UPDATE_AGENT_CONFIG     0x0075
#define CMD_GET_PARAMETER_LIST      0x0076
#define CMD_DEPLOY_PACKAGE          0x0077
#define CMD_INSTALLER_INFO          0x0078
#define CMD_GET_LAST_VALUES         0x0079
#define CMD_APPLY_TEMPLATE          0x007A
#define CMD_SET_USER_VARIABLE       0x007B
#define CMD_GET_USER_VARIABLE       0x007C
#define CMD_ENUM_USER_VARIABLES     0x007D
#define CMD_DELETE_USER_VARIABLE    0x007E
#define CMD_ADM_MESSAGE             0x007F
#define CMD_ADM_REQUEST             0x0080
#define CMD_CHANGE_IP_ADDR          0x0081
#define CMD_REQUEST_SESSION_KEY     0x0082
#define CMD_ENCRYPTED_MESSAGE       0x0083
#define CMD_SESSION_KEY             0x0084
#define CMD_REQUEST_ENCRYPTION      0x0085
#define CMD_GET_ROUTING_TABLE       0x0086
#define CMD_EXEC_TABLE_TOOL         0x0087
#define CMD_TABLE_DATA              0x0088
#define CMD_CANCEL_JOB              0x0089
#define CMD_CHANGE_SUBSCRIPTION     0x008A
#define CMD_GET_SYSLOG              0x008B
#define CMD_SYSLOG_RECORDS          0x008C
#define CMD_JOB_CHANGE_NOTIFICATION 0x008D
#define CMD_DEPLOY_AGENT_POLICY     0x008E
#define CMD_LOG_DATA                0x008F
#define CMD_GET_OBJECT_TOOL_DETAILS 0x0090
#define CMD_EXECUTE_SERVER_COMMAND  0x0091
#define CMD_UPLOAD_FILE_TO_AGENT    0x0092
#define CMD_UPDATE_OBJECT_TOOL      0x0093
#define CMD_DELETE_OBJECT_TOOL      0x0094
#define CMD_SETUP_PROXY_CONNECTION  0x0095
#define CMD_GENERATE_OBJECT_TOOL_ID 0x0096
#define CMD_GET_SERVER_STATS        0x0097
#define CMD_GET_SCRIPT_LIST         0x0098
#define CMD_GET_SCRIPT              0x0099
#define CMD_UPDATE_SCRIPT           0x009A
#define CMD_DELETE_SCRIPT           0x009B
#define CMD_RENAME_SCRIPT           0x009C
#define CMD_GET_SESSION_LIST        0x009D
#define CMD_KILL_SESSION            0x009E
#define CMD_GET_TRAP_LOG            0x009F
#define CMD_TRAP_LOG_RECORDS        0x00A0
#define CMD_START_SNMP_WALK         0x00A1
#define CMD_SNMP_WALK_DATA          0x00A2
#define CMD_GET_MAP_LIST            0x00A3
#define CMD_LOAD_MAP                0x00A4
#define CMD_SAVE_MAP                0x00A5
#define CMD_DELETE_MAP              0x00A6
#define CMD_RESOLVE_MAP_NAME        0x00A7
#define CMD_SUBMAP_DATA             0x00A8
#define CMD_UPLOAD_SUBMAP_BK_IMAGE  0x00A9
#define CMD_GET_SUBMAP_BK_IMAGE     0x00AA
#define CMD_GET_MODULE_LIST			0x00AB
#define CMD_UPDATE_MODULE_INFO		0x00AC
#define CMD_COPY_USER_VARIABLE      0x00AD
#define CMD_RESOLVE_DCI_NAMES       0x00AE
#define CMD_GET_MY_CONFIG           0x00AF
#define CMD_GET_AGENT_CFG_LIST      0x00B0
#define CMD_OPEN_AGENT_CONFIG       0x00B1
#define CMD_SAVE_AGENT_CONFIG       0x00B2
#define CMD_DELETE_AGENT_CONFIG     0x00B3
#define CMD_SWAP_AGENT_CONFIGS      0x00B4
#define CMD_TERMINATE_ALARM         0x00B5
#define CMD_GET_NXCP_CAPS           0x00B6
#define CMD_NXCP_CAPS               0x00B7
#define CMD_GET_OBJECT_COMMENTS     0x00B8
#define CMD_UPDATE_OBJECT_COMMENTS  0x00B9
#define CMD_ENABLE_AGENT_TRAPS      0x00BA
#define CMD_PUSH_DCI_DATA           0x00BB
#define CMD_GET_ADDR_LIST           0x00BC
#define CMD_SET_ADDR_LIST           0x00BD
#define CMD_RESET_COMPONENT         0x00BE
#define CMD_GET_DCI_EVENTS_LIST     0x00BF
#define CMD_EXPORT_CONFIGURATION    0x00C0
#define CMD_IMPORT_CONFIGURATION    0x00C1
#define CMD_GET_TRAP_CFG_RO			0x00C2
#define CMD_SNMP_REQUEST				0x00C3
#define CMD_GET_DCI_INFO				0x00C4
#define CMD_GET_GRAPH_LIST				0x00C5
#define CMD_DEFINE_GRAPH				0x00C6
#define CMD_DELETE_GRAPH				0x00C7
#define CMD_GET_PERFTAB_DCI_LIST    0x00C8
#define CMD_ADD_CA_CERTIFICATE		0x00C9
#define CMD_DELETE_CERTIFICATE		0x00CA
#define CMD_GET_CERT_LIST				0x00CB
#define CMD_UPDATE_CERT_COMMENTS		0x00CC
#define CMD_QUERY_L2_TOPOLOGY			0x00CD
#define CMD_AUDIT_RECORD            0x00CE
#define CMD_GET_AUDIT_LOG           0x00CF
#define CMD_SEND_SMS                0x00D0
#define CMD_GET_COMMUNITY_LIST      0x00D1
#define CMD_UPDATE_COMMUNITY_LIST   0x00D2
#define CMD_GET_SITUATION_LIST      0x00D3
#define CMD_DELETE_SITUATION        0x00D4
#define CMD_CREATE_SITUATION        0x00D5
#define CMD_DEL_SITUATION_INSTANCE  0x00D6
#define CMD_UPDATE_SITUATION        0x00D7
#define CMD_SITUATION_DATA          0x00D8
#define CMD_SITUATION_CHANGE        0x00D9
#define CMD_CREATE_MAP              0x00DA
#define CMD_UPLOAD_FILE             0x00DB
#define CMD_DELETE_FILE             0x00DC
//#define CMD_GET_REPORT_RESULTS      0x00DD
#define CMD_RENDER_REPORT           0x00DE
#define CMD_EXECUTE_REPORT          0x00DF
#define CMD_GET_REPORT_RESULTS      0x00E0
#define CMD_CONFIG_SET_CLOB         0x00E1
#define CMD_CONFIG_GET_CLOB         0x00E2
#define CMD_RENAME_MAP              0x00E3
#define CMD_CLEAR_DCI_DATA          0x00E4
#define CMD_GET_LICENSE             0x00E5
#define CMD_CHECK_LICENSE           0x00E6
#define CMD_RELEASE_LICENSE         0x00E7
#define CMD_ISC_CONNECT_TO_SERVICE  0x00E8
#define CMD_REGISTER_AGENT          0x00E9
#define CMD_GET_SERVER_FILE         0x00EA
#define CMD_FORWARD_EVENT           0x00EB
#define CMD_GET_USM_CREDENTIALS     0x00EC
#define CMD_UPDATE_USM_CREDENTIALS  0x00ED
#define CMD_GET_DCI_THRESHOLDS      0x00EE
#define CMD_GET_IMAGE               0x00EF
#define CMD_CREATE_IMAGE            0x00F0
#define CMD_DELETE_IMAGE            0x00F1
#define CMD_MODIFY_IMAGE            0x00F2
#define CMD_LIST_IMAGES             0x00F3
#define CMD_LIST_SERVER_FILES       0x00F4
#define CMD_GET_TABLE               0x00F5
#define CMD_QUERY_TABLE             0x00F6
#define CMD_OPEN_CONSOLE            0x00F7
#define CMD_CLOSE_CONSOLE           0x00F8
#define CMD_GET_SELECTED_OBJECTS    0x00F9
#define CMD_GET_VLANS               0x00FA
#define CMD_HOLD_JOB                0x00FB
#define CMD_UNHOLD_JOB              0x00FC
#define CMD_CHANGE_ZONE             0x00FD


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
#define VID_CREATION_FLAGS          ((DWORD)14)
#define VID_AGENT_PORT              ((DWORD)15)
#define VID_AUTH_METHOD             ((DWORD)16)
#define VID_SHARED_SECRET           ((DWORD)17)
#define VID_SNMP_AUTH_OBJECT        ((DWORD)18)
#define VID_SNMP_OID                ((DWORD)19)
#define VID_NAME                    ((DWORD)20)
#define VID_VALUE                   ((DWORD)21)
#define VID_PEER_GATEWAY            ((DWORD)22)
#define VID_NOTIFICATION_CODE       ((DWORD)23)
#define VID_EVENT_CODE              ((DWORD)24)
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
#define VID_PROPERTIES              ((DWORD)62)
#define VID_ALARM_SEVERITY          ((DWORD)63)
#define VID_ALARM_KEY               ((DWORD)64)
#define VID_ALARM_TIMEOUT           ((DWORD)65)
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
#define VID_SNMP_PORT               ((DWORD)80)
#define VID_CONFIG_FILE_DATA        ((DWORD)81)
#define VID_COMMENTS                ((DWORD)82)
#define VID_POLICY_ID               ((DWORD)83)
#define VID_SNMP_USM_METHODS        ((DWORD)84)
#define VID_PARAMETER               ((DWORD)85)
#define VID_NUM_STRINGS             ((DWORD)86)
#define VID_ACTION_NAME             ((DWORD)87)
#define VID_NUM_ARGS                ((DWORD)88)
#define VID_SNMP_AUTH_PASSWORD      ((DWORD)89)
#define VID_CLASS_ID_LIST           ((DWORD)90)
#define VID_SNMP_PRIV_PASSWORD      ((DWORD)91)
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
#define VID_POLL_TYPE               ((DWORD)107)
#define VID_POLLER_MESSAGE          ((DWORD)108)
#define VID_SOURCE_OBJECT_ID        ((DWORD)109)
#define VID_DESTINATION_OBJECT_ID   ((DWORD)110)
#define VID_NUM_ITEMS               ((DWORD)111)
#define VID_ITEM_LIST               ((DWORD)112)
#define VID_MAC_ADDR                ((DWORD)113)
#define VID_TEMPLATE_VERSION        ((DWORD)114)
#define VID_NODE_TYPE               ((DWORD)115)
#define VID_INSTANCE                ((DWORD)116)
#define VID_TRAP_ID                 ((DWORD)117)
#define VID_TRAP_OID                ((DWORD)118)
#define VID_TRAP_OID_LEN            ((DWORD)119)
#define VID_TRAP_NUM_MAPS           ((DWORD)120)
#define VID_SERVER_VERSION          ((DWORD)121)
#define VID_SUPPORTED_ENCRYPTION    ((DWORD)122)
#define VID_EVENT_ID                ((DWORD)123)
#define VID_AGENT_VERSION           ((DWORD)124)
#define VID_FILE_NAME               ((DWORD)125)
#define VID_PACKAGE_ID              ((DWORD)126)
#define VID_PACKAGE_VERSION         ((DWORD)127)
#define VID_PLATFORM_NAME           ((DWORD)128)
#define VID_PACKAGE_NAME            ((DWORD)129)
#define VID_SERVICE_TYPE            ((DWORD)130)
#define VID_IP_PROTO                ((DWORD)131)
#define VID_IP_PORT                 ((DWORD)132)
#define VID_SERVICE_REQUEST         ((DWORD)133)
#define VID_SERVICE_RESPONSE        ((DWORD)134)
#define VID_POLLER_NODE_ID          ((DWORD)135)
#define VID_SERVICE_STATUS          ((DWORD)136)
#define VID_NUM_PARAMETERS          ((DWORD)137)
#define VID_NUM_OBJECTS             ((DWORD)138)
#define VID_OBJECT_LIST             ((DWORD)139)
#define VID_DEPLOYMENT_STATUS       ((DWORD)140)
#define VID_ERROR_MESSAGE           ((DWORD)141)
#define VID_SERVER_ID               ((DWORD)142)
#define VID_SEARCH_PATTERN          ((DWORD)143)
#define VID_NUM_VARIABLES           ((DWORD)144)
#define VID_COMMAND                 ((DWORD)145)
#define VID_PROTOCOL_VERSION        ((DWORD)146)
#define VID_ZONE_ID                 ((DWORD)147)
#define VID_ZONING_ENABLED          ((DWORD)148)
#define VID_ICMP_PROXY              ((DWORD)149)
#define VID_ADDR_LIST_SIZE          ((DWORD)150)
#define VID_IP_ADDR_LIST            ((DWORD)151)
#define VID_REMOVE_DCI              ((DWORD)152)
#define VID_TEMPLATE_ID             ((DWORD)153)
#define VID_PUBLIC_KEY              ((DWORD)154)
#define VID_SESSION_KEY             ((DWORD)155)
#define VID_CIPHER                  ((DWORD)156)
#define VID_KEY_LENGTH              ((DWORD)157)
#define VID_SESSION_IV              ((DWORD)158)
#define VID_CONFIG_FILE             ((DWORD)159)
#define VID_STATUS_CALCULATION_ALG  ((DWORD)160)
#define VID_NUM_LOCAL_NETS          ((DWORD)161)
#define VID_NUM_REMOTE_NETS         ((DWORD)162)
#define VID_APPLY_FLAG              ((DWORD)163)
#define VID_NUM_TOOLS               ((DWORD)164)
#define VID_TOOL_ID                 ((DWORD)165)
#define VID_NUM_COLUMNS             ((DWORD)166)
#define VID_NUM_ROWS                ((DWORD)167)
#define VID_TABLE_TITLE             ((DWORD)168)
#define VID_EVENT_NAME              ((DWORD)169)
#define VID_AUTO_APPLY              ((DWORD)170)
#define VID_LOG_NAME                ((DWORD)171)
#define VID_OPERATION               ((DWORD)172)
#define VID_MAX_RECORDS             ((DWORD)173)
#define VID_NUM_RECORDS             ((DWORD)174)
#define VID_CLIENT_INFO             ((DWORD)175)
#define VID_OS_INFO                 ((DWORD)176)
#define VID_LIBNXCL_VERSION         ((DWORD)177)
#define VID_VERSION                 ((DWORD)178)
#define VID_NUM_NODES               ((DWORD)179)
#define VID_LOG_FILE                ((DWORD)180)
#define VID_ADV_SCHEDULE            ((DWORD)181)
#define VID_NUM_SCHEDULES           ((DWORD)182)
#define VID_STATUS_PROPAGATION_ALG  ((DWORD)183)
#define VID_FIXED_STATUS            ((DWORD)184)
#define VID_STATUS_SHIFT            ((DWORD)185)
#define VID_STATUS_TRANSLATION_1    ((DWORD)186)
#define VID_STATUS_TRANSLATION_2    ((DWORD)187)
#define VID_STATUS_TRANSLATION_3    ((DWORD)188)
#define VID_STATUS_TRANSLATION_4    ((DWORD)189)
#define VID_STATUS_SINGLE_THRESHOLD ((DWORD)190)
#define VID_STATUS_THRESHOLD_1      ((DWORD)191)
#define VID_STATUS_THRESHOLD_2      ((DWORD)192)
#define VID_STATUS_THRESHOLD_3      ((DWORD)193)
#define VID_STATUS_THRESHOLD_4      ((DWORD)194)
#define VID_AGENT_PROXY             ((DWORD)195)
#define VID_TOOL_TYPE               ((DWORD)196)
#define VID_TOOL_DATA               ((DWORD)197)
#define VID_ACL                     ((DWORD)198)
#define VID_TOOL_OID                ((DWORD)199)
#define VID_SERVER_UPTIME           ((DWORD)200)
#define VID_NUM_ALARMS              ((DWORD)201)
#define VID_ALARMS_BY_SEVERITY      ((DWORD)202)
#define VID_NETXMSD_PROCESS_WKSET   ((DWORD)203)
#define VID_NETXMSD_PROCESS_VMSIZE  ((DWORD)204)
#define VID_NUM_SESSIONS            ((DWORD)205)
#define VID_NUM_SCRIPTS             ((DWORD)206)
#define VID_SCRIPT_ID               ((DWORD)207)
#define VID_SCRIPT_CODE             ((DWORD)208)
#define VID_SESSION_ID              ((DWORD)209)
#define VID_RECORDS_ORDER           ((DWORD)210)
#define VID_NUM_SUBMAPS             ((DWORD)211)
#define VID_SUBMAP_LIST             ((DWORD)212)
#define VID_SUBMAP_ATTR             ((DWORD)213)
#define VID_NUM_LINKS               ((DWORD)214)
#define VID_LINK_LIST               ((DWORD)215)
#define VID_MAP_ID                  ((DWORD)216)
#define VID_NUM_MAPS                ((DWORD)217)
#define VID_NUM_MODULES             ((DWORD)218)
#define VID_DST_USER_ID             ((DWORD)219)
#define VID_MOVE_FLAG               ((DWORD)220)
#define VID_CHANGE_PASSWD_FLAG      ((DWORD)221)
#define VID_GUID                    ((DWORD)222)
#define VID_ACTIVATION_EVENT        ((DWORD)223)
#define VID_DEACTIVATION_EVENT      ((DWORD)224)
#define VID_SOURCE_OBJECT           ((DWORD)225)
#define VID_ACTIVE_STATUS           ((DWORD)226)
#define VID_INACTIVE_STATUS         ((DWORD)227)
#define VID_SCRIPT                  ((DWORD)228)
#define VID_NODE_LIST               ((DWORD)229)
#define VID_DCI_LIST                ((DWORD)230)
#define VID_CONFIG_ID               ((DWORD)231)
#define VID_FILTER                  ((DWORD)232)
#define VID_SEQUENCE_NUMBER         ((DWORD)233)
#define VID_VERSION_MAJOR           ((DWORD)234)
#define VID_VERSION_MINOR           ((DWORD)235)
#define VID_VERSION_RELEASE         ((DWORD)236)
#define VID_CONFIG_ID_2             ((DWORD)237)
#define VID_IV_LENGTH               ((DWORD)238)
#define VID_DBCONN_STATUS           ((DWORD)239)
#define VID_CREATION_TIME           ((DWORD)240)
#define VID_LAST_CHANGE_TIME        ((DWORD)241)
#define VID_TERMINATED_BY_USER      ((DWORD)242)
#define VID_STATE                   ((DWORD)243)
#define VID_CURRENT_SEVERITY        ((DWORD)244)
#define VID_ORIGINAL_SEVERITY       ((DWORD)245)
#define VID_HELPDESK_STATE          ((DWORD)246)
#define VID_HELPDESK_REF            ((DWORD)247)
#define VID_REPEAT_COUNT            ((DWORD)248)
#define VID_ALL_THRESHOLDS          ((DWORD)249)
#define VID_CONFIRMATION_TEXT       ((DWORD)250)
#define VID_FAILED_DCI_INDEX        ((DWORD)251)
#define VID_ADDR_LIST_TYPE          ((DWORD)252)
#define VID_COMPONENT_ID            ((DWORD)253)
#define VID_SYNC_COMMENTS           ((DWORD)254)
#define VID_EVENT_LIST              ((DWORD)255)
#define VID_NUM_TRAPS               ((DWORD)256)
#define VID_TRAP_LIST               ((DWORD)257)
#define VID_NXMP_CONTENT            ((DWORD)258)
#define VID_ERROR_TEXT              ((DWORD)259)
#define VID_COMPONENT               ((DWORD)260)
#define VID_CONSOLE_UPGRADE_URL		((DWORD)261)
#define VID_CLUSTER_TYPE				((DWORD)262)
#define VID_NUM_SYNC_SUBNETS			((DWORD)263)
#define VID_SYNC_SUBNETS				((DWORD)264)
#define VID_NUM_RESOURCES				((DWORD)265)
#define VID_RESOURCE_ID					((DWORD)266)
#define VID_SNMP_PROXY					((DWORD)267)
#define VID_PORT							((DWORD)268)
#define VID_PDU							((DWORD)269)
#define VID_PDU_SIZE						((DWORD)270)
#define VID_IS_SYSTEM					((DWORD)271)
#define VID_GRAPH_CONFIG				((DWORD)272)
#define VID_NUM_GRAPHS					((DWORD)273)
#define VID_GRAPH_ID						((DWORD)274)
#define VID_AUTH_TYPE					((DWORD)275)
#define VID_CERTIFICATE					((DWORD)276)
#define VID_SIGNATURE					((DWORD)277)
#define VID_CHALLENGE					((DWORD)278)
#define VID_CERT_MAPPING_METHOD		((DWORD)279)
#define VID_CERT_MAPPING_DATA       ((DWORD)280)
#define VID_CERTIFICATE_ID				((DWORD)281)
#define VID_NUM_CERTIFICATES        ((DWORD)282)
#define VID_ALARM_TIMEOUT_EVENT     ((DWORD)283)
#define VID_NUM_GROUPS              ((DWORD)284)
#define VID_QSIZE_CONDITION_POLLER  ((DWORD)285)
#define VID_QSIZE_CONF_POLLER       ((DWORD)286)
#define VID_QSIZE_DCI_POLLER        ((DWORD)287)
#define VID_QSIZE_DBWRITER          ((DWORD)288)
#define VID_QSIZE_EVENT             ((DWORD)289)
#define VID_QSIZE_DISCOVERY         ((DWORD)290)
#define VID_QSIZE_NODE_POLLER       ((DWORD)291)
#define VID_QSIZE_ROUTE_POLLER      ((DWORD)292)
#define VID_QSIZE_STATUS_POLLER     ((DWORD)293)
#define VID_SYNTHETIC_MASK          ((DWORD)294)
#define VID_SUBSYSTEM               ((DWORD)295)
#define VID_SUCCESS_AUDIT           ((DWORD)296)
#define VID_WORKSTATION             ((DWORD)297)
#define VID_USER_TAG                ((DWORD)298)
#define VID_REQUIRED_POLLS          ((DWORD)299)
#define VID_SYS_DESCRIPTION         ((DWORD)300)
#define VID_SITUATION_ID            ((DWORD)301)
#define VID_SITUATION_INSTANCE      ((DWORD)302)
#define VID_SITUATION_NUM_ATTRS     ((DWORD)303)
#define VID_INSTANCE_COUNT          ((DWORD)304)
#define VID_SITUATION_COUNT         ((DWORD)305)
#define VID_NUM_TRUSTED_NODES       ((DWORD)306)
#define VID_TRUSTED_NODES           ((DWORD)307)
#define VID_TIMEZONE                ((DWORD)308)
#define VID_NUM_CUSTOM_ATTRIBUTES   ((DWORD)309)
#define VID_MAP_DATA                ((DWORD)310)
#define VID_PRODUCT_ID              ((DWORD)311)
#define VID_CLIENT_ID               ((DWORD)312)
#define VID_LICENSE_DATA            ((DWORD)313)
#define VID_TOKEN                   ((DWORD)314)
#define VID_SERVICE_ID              ((DWORD)315)
#define VID_TOKEN_SOFTLIMIT         ((DWORD)316)
#define VID_TOKEN_HARDLIMIT         ((DWORD)317)
#define VID_USE_IFXTABLE            ((DWORD)318)
#define VID_APPLY_FILTER            ((DWORD)319)
#define VID_ENABLE_AUTO_BIND        ((DWORD)320)
#define VID_AUTO_BIND_FILTER        ((DWORD)321)
#define VID_BASE_UNITS              ((DWORD)322)
#define VID_MULTIPLIER              ((DWORD)323)
#define VID_CUSTOM_UNITS_NAME       ((DWORD)324)
#define VID_PERFTAB_SETTINGS        ((DWORD)325)
#define VID_EXECUTION_STATUS        ((DWORD)326)
#define VID_EXECUTION_RESULT        ((DWORD)327)
#define VID_TABLE_NUM_ROWS          ((DWORD)328)
#define VID_TABLE_NUM_COLS          ((DWORD)329)
#define VID_JOB_COUNT               ((DWORD)330)
#define VID_JOB_ID                  ((DWORD)331)
#define VID_JOB_TYPE                ((DWORD)332)
#define VID_JOB_STATUS              ((DWORD)333)
#define VID_JOB_PROGRESS            ((DWORD)334)
#define VID_FAILURE_MESSAGE         ((DWORD)335)
#define VID_POLICY_TYPE             ((DWORD)336)
#define VID_FIELDS                  ((DWORD)337)
#define VID_LOG_HANDLE              ((DWORD)338)
#define VID_START_ROW               ((DWORD)339)
#define VID_TABLE_OFFSET            ((DWORD)340)
#define VID_NUM_FILTERS             ((DWORD)341)
#define VID_GEOLOCATION_TYPE        ((DWORD)342)
#define VID_LATITUDE                ((DWORD)343)
#define VID_LONGITUDE               ((DWORD)344)
#define VID_NUM_ORDERING_COLUMNS    ((DWORD)345)
#define VID_SYSTEM_TAG              ((DWORD)346)
#define VID_NUM_ENUMS               ((DWORD)347)
#define VID_NUM_PUSH_PARAMETERS     ((DWORD)348) 
#define VID_OLD_PASSWORD            ((DWORD)349)
#define VID_MIN_PASSWORD_LENGTH     ((DWORD)350)
#define VID_LAST_LOGIN              ((DWORD)351)
#define VID_LAST_PASSWORD_CHANGE    ((DWORD)352)
#define VID_DISABLED_UNTIL          ((DWORD)353)
#define VID_AUTH_FAILURES           ((DWORD)354)
#define VID_RUNTIME_FLAGS           ((DWORD)355)
#define VID_FILE_SIZE               ((DWORD)356)
#define VID_MAP_TYPE                ((DWORD)357)
#define VID_LAYOUT                  ((DWORD)358)
#define VID_SEED_OBJECT             ((DWORD)359)
#define VID_BACKGROUND              ((DWORD)360)
#define VID_NUM_ELEMENTS            ((DWORD)361)
#define VID_INTERFACE_ID            ((DWORD)362)
#define VID_LOCAL_INTERFACE_ID      ((DWORD)363)
#define VID_LOCAL_NODE_ID           ((DWORD)364)
#define VID_SYS_NAME                ((DWORD)365)
#define VID_LLDP_NODE_ID            ((DWORD)366)
#define VID_IF_SLOT                 ((DWORD)367)
#define VID_IF_PORT                 ((DWORD)368)
#define VID_IMAGE_DATA              ((DWORD)369)
#define VID_IMAGE_PROTECTED         ((DWORD)370)
#define VID_NUM_IMAGES              ((DWORD)371)
#define VID_IMAGE_MIMETYPE          ((DWORD)372)
#define VID_PEER_NODE_ID            ((DWORD)373)
#define VID_PEER_INTERFACE_ID       ((DWORD)374)
#define VID_VRRP_VERSION            ((DWORD)375)
#define VID_VRRP_VR_COUNT           ((DWORD)376)
#define VID_DESTINATION_FILE_NAME   ((DWORD)377)
#define VID_NUM_TABLES              ((DWORD)378)
#define VID_IMAGE                   ((DWORD)379)
#define VID_DRIVER_NAME             ((DWORD)380)
#define VID_DRIVER_VERSION          ((DWORD)381)
#define VID_NUM_VLANS               ((DWORD)382)
#define VID_CREATE_JOB_ON_HOLD      ((DWORD)383)
#define VID_TILE_SERVER_URL         ((DWORD)384)
#define VID_BACKGROUND_LATITUDE     ((DWORD)385)
#define VID_BACKGROUND_LONGITUDE    ((DWORD)386)
#define VID_BACKGROUND_ZOOM         ((DWORD)387)
#define VID_BRIDGE_BASE_ADDRESS     ((DWORD)388)
#define VID_SUBMAP_ID               ((DWORD)389)
#define VID_REPORT_DEFINITION       ((DWORD)390)

// Map elements list base
#define VID_ELEMENT_LIST_BASE       ((DWORD)0x10000000)
#define VID_LINK_LIST_BASE          ((DWORD)0x40000000)

// Variable ranges for object's ACL
#define VID_ACL_USER_BASE           ((DWORD)0x00001000)
#define VID_ACL_USER_LAST           ((DWORD)0x00001FFF)
#define VID_ACL_RIGHTS_BASE         ((DWORD)0x00002000)
#define VID_ACL_RIGHTS_LAST         ((DWORD)0x00002FFF)

// Variable range for user group members
#define VID_GROUP_MEMBER_BASE       ((DWORD)0x00004000)
#define VID_GROUP_MEMBER_LAST       ((DWORD)0x00004FFF)

// Variable range for data collection thresholds
#define VID_DCI_THRESHOLD_BASE      ((DWORD)0x20000000)
#define VID_DCI_SCHEDULE_BASE       ((DWORD)0x10000000)

// Variable range for event argument list
#define VID_EVENT_ARG_BASE          ((DWORD)0x00008000)
#define VID_EVENT_ARG_LAST          ((DWORD)0x00008FFF)

// Variable range for trap parameter list
#define VID_TRAP_PLEN_BASE          ((DWORD)0x00009000)
#define VID_TRAP_PLEN_LAST          ((DWORD)0x000093FF)
#define VID_TRAP_PNAME_BASE         ((DWORD)0x00009400)
#define VID_TRAP_PNAME_LAST         ((DWORD)0x000097FF)
#define VID_TRAP_PDESCR_BASE        ((DWORD)0x00009800)
#define VID_TRAP_PDESCR_LAST        ((DWORD)0x00009BFF)

// Object information can contain variable number of parent and child objects' ids.
// Because each variable in message have to have unique identifier,
// we reserver a two range ids for this variables.
#define VID_PARENT_ID_BASE          ((DWORD)0x00003000)
#define VID_PARENT_ID_LAST          ((DWORD)0x00003FFF)

// Reservation of 0x7FFFFFFF ids for child object's list
#define VID_CHILD_ID_BASE           ((DWORD)0x80000000)
#define VID_CHILD_ID_LAST           ((DWORD)0xFFFFFFFE)

// Base value for custom attributes
#define VID_CUSTOM_ATTRIBUTES_BASE  ((DWORD)0x70000000)

// Base value for cluster resource list
#define VID_RESOURCE_LIST_BASE      ((DWORD)0x20000000)

// Base value for agent's enum values
#define VID_ENUM_VALUE_BASE         ((DWORD)0x10000000)

// Base value for agent's action arguments
#define VID_ACTION_ARG_BASE         ((DWORD)0x10000000)

// Base value for agent's parameter list
#define VID_PARAM_LIST_BASE         ((DWORD)0x10000000)
#define VID_ENUM_LIST_BASE          ((DWORD)0x20000000)
#define VID_PUSHPARAM_LIST_BASE     ((DWORD)0x30000000)
#define VID_TABLE_LIST_BASE         ((DWORD)0x40000000)

// Base value for DCI last values
#define VID_DCI_VALUES_BASE         ((DWORD)0x10000000)

// Base value for variable names
#define VID_VARLIST_BASE            ((DWORD)0x10000000)

// Base value for network list
#define VID_VPN_NETWORK_BASE        ((DWORD)0x10000000)

// Base value for network list
#define VID_OBJECT_TOOLS_BASE       ((DWORD)0x10000000)

// Base values for table data
#define VID_COLUMN_INFO_BASE        ((DWORD)0x10000000)
#define VID_COLUMN_NAME_BASE        ((DWORD)0x10000000)
#define VID_COLUMN_FMT_BASE         ((DWORD)0x20000000)
#define VID_ROW_DATA_BASE           ((DWORD)0x30000000)

// Base value for event log records
#define VID_EVENTLOG_MSG_BASE       ((DWORD)0x10000000)

// Base value for syslog records
#define VID_SYSLOG_MSG_BASE         ((DWORD)0x10000000)

// Base value for trap log records
#define VID_TRAP_LOG_MSG_BASE       ((DWORD)0x10000000)

// Base value for script list
#define VID_SCRIPT_LIST_BASE        ((DWORD)0x10000000)

// Base value for session data
#define VID_SESSION_DATA_BASE       ((DWORD)0x10000000)

// Base value for SNMP walker data
#define VID_SNMP_WALKER_DATA_BASE   ((DWORD)0x10000000)

// Base value for map list
#define VID_MAP_LIST_BASE           ((DWORD)0x10000000)

// Base value for module list
#define VID_MODULE_LIST_BASE        ((DWORD)0x10000000)

// Base value for agent configs list
#define VID_AGENT_CFG_LIST_BASE     ((DWORD)0x10000000)

// Base and last values for condition's DCI list
#define VID_DCI_LIST_BASE           ((DWORD)0x40000000)
#define VID_DCI_LIST_LAST           ((DWORD)0x4FFFFFFF)

// Base value for DCI push data
#define VID_PUSH_DCI_DATA_BASE      ((DWORD)0x10000000)

// Base value for address list
#define VID_ADDR_LIST_BASE          ((DWORD)0x10000000)

// Base value for trap configuration records
#define VID_TRAP_INFO_BASE          ((DWORD)0x10000000)

// Base value for graph list
#define VID_GRAPH_LIST_BASE         ((DWORD)0x10000000)
#define VID_GRAPH_ACL_BASE				((DWORD)0x20000000)

// Base value for system DCI list
#define VID_SYSDCI_LIST_BASE			((DWORD)0x10000000)

// Base value for certificate list
#define VID_CERT_LIST_BASE 			((DWORD)0x10000000)

// Base value for various string lists
#define VID_STRING_LIST_BASE 			((DWORD)0x10000000)

// Base values for situation lists
#define VID_SITUATION_ATTR_LIST_BASE ((DWORD)0x10000000)
#define VID_INSTANCE_LIST_BASE      ((DWORD)0x20000000)

// Base value for object links list
#define VID_OBJECT_LINKS_BASE			((DWORD)0x10000000)
#define VID_SUBMAP_LINK_NAMES_BASE  ((DWORD)0x20000000)

#define VID_TABLE_COLUMN_INFO_BASE  ((DWORD)0x10000000)
#define VID_TABLE_DATA_BASE         ((DWORD)0x20000000)

#define VID_JOB_LIST_BASE           ((DWORD)0x10000000)

#define VID_COLUMN_FILTERS_BASE     ((DWORD)0x10000000)
#define VID_ORDERING_COLUMNS_BASE   ((DWORD)0x40000000)

#define VID_USM_CRED_LIST_BASE      ((DWORD)0x10000000)

#define VID_IMAGE_LIST_BASE         ((DWORD)0x10000000)

#define VID_VLAN_LIST_BASE          ((DWORD)0x10000000)


//
// Inline functions
//

#ifdef __cplusplus

inline BOOL IsBinaryMsg(CSCP_MESSAGE *pMsg)
{
   return ntohs(pMsg->wFlags) & MF_BINARY;
}

#endif


#endif   /* _nms_cscp_h_ */
