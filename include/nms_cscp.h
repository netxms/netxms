/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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

/**
 * Constants
 */
#define NXCP_VERSION                   4

#define SERVER_LISTEN_PORT_FOR_CLIENTS 4701
#define SERVER_LISTEN_PORT_FOR_MOBILES 4747
#define MAX_DCI_STRING_VALUE           256
#define CLIENT_CHALLENGE_SIZE				256
#define NXCP_HEADER_SIZE               16
#define NXCP_ENCRYPTION_HEADER_SIZE    16
#define NXCP_EH_UNENCRYPTED_BYTES      8
#define NXCP_EH_ENCRYPTED_BYTES        (NXCP_ENCRYPTION_HEADER_SIZE - NXCP_EH_UNENCRYPTED_BYTES)
#ifdef __64BIT__
#define PROXY_ENCRYPTION_CTX           ((NXCPEncryptionContext *)_ULL(0xFFFFFFFFFFFFFFFF))
#else
#define PROXY_ENCRYPTION_CTX           ((NXCPEncryptionContext *)0xFFFFFFFF)
#endif

#ifndef EVP_MAX_IV_LENGTH
#define EVP_MAX_IV_LENGTH              16
#endif

#define RECORD_ORDER_NORMAL            0
#define RECORD_ORDER_REVERSED          1

#define NXCP_TEMP_BUF_SIZE             65536

/**
 * Ciphers
 */
#define NXCP_CIPHER_AES_256       0
#define NXCP_CIPHER_BLOWFISH_256  1
#define NXCP_CIPHER_IDEA          2
#define NXCP_CIPHER_3DES          3
#define NXCP_CIPHER_AES_128       4
#define NXCP_CIPHER_BLOWFISH_128  5

#define NXCP_SUPPORT_AES_256      0x01
#define NXCP_SUPPORT_BLOWFISH_256 0x02
#define NXCP_SUPPORT_IDEA         0x04
#define NXCP_SUPPORT_3DES         0x08
#define NXCP_SUPPORT_AES_128      0x10
#define NXCP_SUPPORT_BLOWFISH_128 0x20

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

/**
 * Message field flags
 */
#define NXCP_MFF_SIGNED   0x01

/**
 * Address family ID for NXCP
 */
#define NXCP_AF_INET    0
#define NXCP_AF_INET6   1
#define NXCP_AF_UNSPEC  2

/**
 * NXCP data field structure
 */
typedef struct
{
   UINT32 fieldId;  // Field identifier
   BYTE type;       // Data type
   BYTE flags;      // flags (may by type-dependent)
   UINT16 int16;
   union
   {
      INT32 int32;
      INT64 int64;
      UINT32 uint32;
      UINT64 uint64;
      double real;
      struct
      {
         UINT32 length;
         UINT16 value[1]; // actual size depends on length value
      } string;
      struct
      {
         UINT32 length;
         BYTE value[1]; // actual size depends on length value
      } binary;
      struct
      {
         union
         {
            UINT32 v4;
            BYTE v6[16];
         } addr;
         BYTE family;
         BYTE maskBits;
         BYTE padding[6];
      } inetaddr;
   } data;
} NXCP_MESSAGE_FIELD;

#define df_int16    int16
#define df_int32    data.int32
#define df_uint32   data.uint32
#define df_int64    data.int64
#define df_uint64   data.uint64
#define df_real     data.real
#define df_string   data.string
#define df_binary   data.binary
#define df_inetaddr data.inetaddr

/**
 * Message structure
 */
typedef struct
{
   UINT16 code;      // Message (command) code
   UINT16 flags;     // Message flags
   UINT32 size;      // Message size (including header) in bytes
   UINT32 id;        // Unique message identifier
   UINT32 numFields; // Number of fields in message
   NXCP_MESSAGE_FIELD fields[1];    // Data fields - actual length depends on value in numFields
} NXCP_MESSAGE;

/**
 * Encrypted payload header
 */
typedef struct
{
   UINT32 dwChecksum;
   UINT32 dwReserved; // Align to 8-byte boundary
} NXCP_ENCRYPTED_PAYLOAD_HEADER;

/**
 * Encrypted message structure
 */
typedef struct
{
   WORD code;       // Should be CMD_ENCRYPTED_MESSAGE
   BYTE padding;    // Number of bytes added to the end of message
   BYTE reserved;
   UINT32 size;    // Size of encrypted message (including encryption header and padding)
   BYTE data[1];     // Encrypted payload
} NXCP_ENCRYPTED_MESSAGE;

/**
 * DCI data header structure
 */
typedef struct
{
   UINT32 dciId;
   UINT32 numRows;
   UINT32 dataType;
   UINT32 padding;
} DCI_DATA_HEADER;

/**
 * DCI data row structure
 */
typedef struct
{
   UINT32 timeStamp;
   union
   {
      UINT32 int32;
      UCS2CHAR string[MAX_DCI_STRING_VALUE];
      struct
      {
         UINT32 padding;
         union
         {
            UINT64 int64;
            double real;
         } v64;
      } ext;
   } value;
} DCI_DATA_ROW;

#if defined(__HP_aCC)
#pragma pack
#elif defined(_AIX) && !defined(__GNUC__)
#pragma pack(pop)
#else
#pragma pack()
#endif

/**
 * Data types
 */
#define NXCP_DT_INT32      0
#define NXCP_DT_STRING     1
#define NXCP_DT_INT64      2
#define NXCP_DT_INT16      3
#define NXCP_DT_BINARY     4
#define NXCP_DT_FLOAT      5
#define NXCP_DT_INETADDR   6

/**
 * Message flags
 */
#define MF_BINARY             0x0001
#define MF_END_OF_FILE        0x0002
#define MF_DONT_ENCRYPT       0x0004
#define MF_END_OF_SEQUENCE    0x0008
#define MF_REVERSE_ORDER      0x0010
#define MF_CONTROL            0x0020
#define MF_COMPRESSED         0x0040
#define MF_STREAM             0x0080

/**
 * Message (command) codes
 */
#define CMD_LOGIN                         0x0001
#define CMD_LOGIN_RESP                    0x0002
#define CMD_KEEPALIVE                     0x0003
#define CMD_OPEN_HELPDESK_ISSUE           0x0004
#define CMD_GET_OBJECTS                   0x0005
#define CMD_OBJECT                        0x0006
#define CMD_DELETE_OBJECT                 0x0007
#define CMD_MODIFY_OBJECT                 0x0008
#define CMD_OBJECT_LIST_END               0x0009
#define CMD_OBJECT_UPDATE                 0x000A
#define CMD_RECALCULATE_DCI_VALUES        0x000B
#define CMD_EVENTLOG_RECORDS              0x000C
#define CMD_GET_CONFIG_VARLIST            0x000D
#define CMD_SET_CONFIG_VARIABLE           0x000E
#define CMD_GET_OBJECT_TOOLS              0x000F
#define CMD_EXECUTE_ACTION                0x0010
#define CMD_DELETE_CONFIG_VARIABLE        0x0011
#define CMD_NOTIFY                        0x0012
#define CMD_TRAP                          0x0013
#define CMD_OPEN_EPP                      0x0014
#define CMD_CLOSE_EPP                     0x0015
#define CMD_SAVE_EPP                      0x0016
#define CMD_EPP_RECORD                    0x0017
#define CMD_EVENT_DB_UPDATE               0x0018
#define CMD_TRAP_CFG_UPDATE               0x0019
#define CMD_SET_EVENT_INFO                0x001A
#define CMD_EVENT_DB_RECORD               0x001B
#define CMD_LOAD_EVENT_DB                 0x001C
#define CMD_REQUEST_COMPLETED             0x001D
#define CMD_LOAD_USER_DB                  0x001E
#define CMD_USER_DATA                     0x001F
#define CMD_GROUP_DATA                    0x0020
#define CMD_USER_DB_EOF                   0x0021
#define CMD_UPDATE_USER                   0x0022
#define CMD_DELETE_USER                   0x0023
#define CMD_CREATE_USER                   0x0024
#define CMD_LOCK_USER_DB                  0x0025
#define CMD_UNLOCK_USER_DB                0x0026
#define CMD_USER_DB_UPDATE                0x0027
#define CMD_SET_PASSWORD                  0x0028
#define CMD_GET_NODE_DCI_LIST             0x0029
#define CMD_NODE_DCI                      0x002A
#define CMD_GET_LOG_DATA                  0x002B
#define CMD_DELETE_NODE_DCI               0x002C
#define CMD_MODIFY_NODE_DCI               0x002D
#define CMD_UNLOCK_NODE_DCI_LIST          0x002E
#define CMD_SET_OBJECT_MGMT_STATUS        0x002F
//#define CMD_CREATE_NEW_DCI                0x0030
#define CMD_GET_DCI_DATA                  0x0031
#define CMD_DCI_DATA                      0x0032
#define CMD_GET_MIB_TIMESTAMP             0x0033
#define CMD_GET_MIB                       0x0034
#define CMD_TEST_DCI_TRANSFORMATION       0x0035
#define CMD_GET_JOB_LIST                  0x0036
#define CMD_CREATE_OBJECT                 0x0037
#define CMD_GET_EVENT_NAMES               0x0038
#define CMD_EVENT_NAME_LIST               0x0039
#define CMD_BIND_OBJECT                   0x003A
#define CMD_UNBIND_OBJECT                 0x003B
#define CMD_UNINSTALL_AGENT_POLICY        0x003C
#define CMD_OPEN_SERVER_LOG               0x003D
#define CMD_CLOSE_SERVER_LOG              0x003E
#define CMD_QUERY_LOG                     0x003F
#define CMD_AUTHENTICATE                  0x0040
#define CMD_GET_PARAMETER                 0x0041
#define CMD_GET_LIST                      0x0042
#define CMD_ACTION                        0x0043
#define CMD_GET_CURRENT_USER_ATTR         0x0044
#define CMD_SET_CURRENT_USER_ATTR         0x0045
#define CMD_GET_ALL_ALARMS                0x0046
#define CMD_GET_ALARM_COMMENTS            0x0047
#define CMD_ACK_ALARM                     0x0048
#define CMD_ALARM_UPDATE                  0x0049
#define CMD_ALARM_DATA                    0x004A
#define CMD_DELETE_ALARM                  0x004B
#define CMD_ADD_CLUSTER_NODE              0x004C
#define CMD_GET_POLICY_INVENTORY          0x004D
#define CMD_LOAD_ACTIONS                  0x004E
#define CMD_ACTION_DB_UPDATE              0x004F
#define CMD_MODIFY_ACTION                 0x0050
#define CMD_CREATE_ACTION                 0x0051
#define CMD_DELETE_ACTION                 0x0052
#define CMD_ACTION_DATA                   0x0053
#define CMD_SETUP_AGENT_TUNNEL            0x0054
#define CMD_EXECUTE_LIBRARY_SCRIPT        0x0055
#define CMD_GET_PREDICTION_ENGINES        0x0056
#define CMD_GET_PREDICTED_DATA            0x0057
#define CMD_STOP_SERVER_COMMAND           0x0058
#define CMD_POLL_NODE                     0x0059
#define CMD_POLLING_INFO                  0x005A
#define CMD_COPY_DCI                      0x005B
#define CMD_WAKEUP_NODE                   0x005C
#define CMD_DELETE_EVENT_TEMPLATE         0x005D
#define CMD_GENERATE_EVENT_CODE           0x005E
#define CMD_FIND_NODE_CONNECTION          0x005F
#define CMD_FIND_MAC_LOCATION             0x0060
#define CMD_CREATE_TRAP                   0x0061
#define CMD_MODIFY_TRAP                   0x0062
#define CMD_DELETE_TRAP                   0x0063
#define CMD_LOAD_TRAP_CFG                 0x0064
#define CMD_TRAP_CFG_RECORD               0x0065
#define CMD_QUERY_PARAMETER               0x0066
#define CMD_GET_SERVER_INFO               0x0067
#define CMD_SET_DCI_STATUS                0x0068
#define CMD_FILE_DATA                     0x0069
#define CMD_TRANSFER_FILE                 0x006A
#define CMD_UPGRADE_AGENT                 0x006B
#define CMD_GET_PACKAGE_LIST              0x006C
#define CMD_PACKAGE_INFO                  0x006D
#define CMD_REMOVE_PACKAGE                0x006E
#define CMD_INSTALL_PACKAGE               0x006F
/*UNUSED:#define CMD_LOCK_PACKAGE_DB               0x0070*/
/*UNUSED:#define CMD_UNLOCK_PACKAGE_DB             0x0071*/
#define CMD_ABORT_FILE_TRANSFER           0x0072
#define CMD_CHECK_NETWORK_SERVICE         0x0073
#define CMD_GET_AGENT_CONFIG              0x0074
#define CMD_UPDATE_AGENT_CONFIG           0x0075
#define CMD_GET_PARAMETER_LIST            0x0076
#define CMD_DEPLOY_PACKAGE                0x0077
#define CMD_INSTALLER_INFO                0x0078
#define CMD_GET_LAST_VALUES               0x0079
#define CMD_APPLY_TEMPLATE                0x007A
#define CMD_SET_USER_VARIABLE             0x007B
#define CMD_GET_USER_VARIABLE             0x007C
#define CMD_ENUM_USER_VARIABLES           0x007D
#define CMD_DELETE_USER_VARIABLE          0x007E
#define CMD_ADM_MESSAGE                   0x007F
#define CMD_ADM_REQUEST                   0x0080
#define CMD_GET_NETWORK_PATH              0x0081
#define CMD_REQUEST_SESSION_KEY           0x0082
#define CMD_ENCRYPTED_MESSAGE             0x0083
#define CMD_SESSION_KEY                   0x0084
#define CMD_REQUEST_ENCRYPTION            0x0085
#define CMD_GET_ROUTING_TABLE             0x0086
#define CMD_EXEC_TABLE_TOOL               0x0087
#define CMD_TABLE_DATA                    0x0088
#define CMD_CANCEL_JOB                    0x0089
#define CMD_CHANGE_SUBSCRIPTION           0x008A
#define CMD_SET_CONFIG_TO_DEFAULT         0x008B
#define CMD_SYSLOG_RECORDS                0x008C
#define CMD_JOB_CHANGE_NOTIFICATION       0x008D
#define CMD_DEPLOY_AGENT_POLICY           0x008E
#define CMD_LOG_DATA                      0x008F
#define CMD_GET_OBJECT_TOOL_DETAILS       0x0090
#define CMD_EXECUTE_SERVER_COMMAND        0x0091
#define CMD_UPLOAD_FILE_TO_AGENT          0x0092
#define CMD_UPDATE_OBJECT_TOOL            0x0093
#define CMD_DELETE_OBJECT_TOOL            0x0094
#define CMD_SETUP_PROXY_CONNECTION        0x0095
#define CMD_GENERATE_OBJECT_TOOL_ID       0x0096
#define CMD_GET_SERVER_STATS              0x0097
#define CMD_GET_SCRIPT_LIST               0x0098
#define CMD_GET_SCRIPT                    0x0099
#define CMD_UPDATE_SCRIPT                 0x009A
#define CMD_DELETE_SCRIPT                 0x009B
#define CMD_RENAME_SCRIPT                 0x009C
#define CMD_GET_SESSION_LIST              0x009D
#define CMD_KILL_SESSION                  0x009E
#define CMD_SET_DB_PASSWORD               0x009F
#define CMD_TRAP_LOG_RECORDS              0x00A0
#define CMD_START_SNMP_WALK               0x00A1
#define CMD_SNMP_WALK_DATA                0x00A2
#define CMD_GET_MAP_LIST                  0x00A3
#define CMD_LOAD_MAP                      0x00A4
#define CMD_SAVE_MAP                      0x00A5
#define CMD_DELETE_MAP                    0x00A6
#define CMD_RESOLVE_MAP_NAME              0x00A7
#define CMD_SUBMAP_DATA                   0x00A8
#define CMD_UPLOAD_SUBMAP_BK_IMAGE        0x00A9
#define CMD_GET_SUBMAP_BK_IMAGE           0x00AA
#define CMD_GET_MODULE_LIST			      0x00AB
#define CMD_UPDATE_MODULE_INFO		      0x00AC
#define CMD_COPY_USER_VARIABLE            0x00AD
#define CMD_RESOLVE_DCI_NAMES             0x00AE
#define CMD_GET_MY_CONFIG                 0x00AF
#define CMD_GET_AGENT_CFG_LIST            0x00B0
#define CMD_OPEN_AGENT_CONFIG             0x00B1
#define CMD_SAVE_AGENT_CONFIG             0x00B2
#define CMD_DELETE_AGENT_CONFIG           0x00B3
#define CMD_SWAP_AGENT_CONFIGS            0x00B4
#define CMD_TERMINATE_ALARM               0x00B5
#define CMD_GET_NXCP_CAPS                 0x00B6
#define CMD_NXCP_CAPS                     0x00B7
#define CMD_GET_OBJECT_COMMENTS           0x00B8
#define CMD_UPDATE_OBJECT_COMMENTS        0x00B9
#define CMD_ENABLE_AGENT_TRAPS            0x00BA
#define CMD_PUSH_DCI_DATA                 0x00BB
#define CMD_GET_ADDR_LIST                 0x00BC
#define CMD_SET_ADDR_LIST                 0x00BD
#define CMD_RESET_COMPONENT               0x00BE
#define CMD_GET_DCI_EVENTS_LIST           0x00BF
#define CMD_EXPORT_CONFIGURATION          0x00C0
#define CMD_IMPORT_CONFIGURATION          0x00C1
#define CMD_GET_TRAP_CFG_RO			      0x00C2
#define CMD_SNMP_REQUEST				      0x00C3
#define CMD_GET_DCI_INFO				      0x00C4
#define CMD_GET_GRAPH_LIST				      0x00C5
#define CMD_SAVE_GRAPH		   		      0x00C6
#define CMD_DELETE_GRAPH				      0x00C7
#define CMD_GET_PERFTAB_DCI_LIST          0x00C8
#define CMD_ADD_CA_CERTIFICATE		      0x00C9
#define CMD_DELETE_CERTIFICATE		      0x00CA
#define CMD_GET_CERT_LIST				      0x00CB
#define CMD_UPDATE_CERT_COMMENTS		      0x00CC
#define CMD_QUERY_L2_TOPOLOGY			      0x00CD
#define CMD_AUDIT_RECORD                  0x00CE
#define CMD_GET_AUDIT_LOG                 0x00CF
#define CMD_SEND_SMS                      0x00D0
#define CMD_GET_COMMUNITY_LIST            0x00D1
#define CMD_UPDATE_COMMUNITY_LIST         0x00D2
#define CMD_GET_PERSISTENT_STORAGE        0x00D3
#define CMD_DELETE_PSTORAGE_VALUE         0x00D4
#define CMD_SET_PSTORAGE_VALUE            0x00D5
#define CMD_GET_AGENT_TUNNELS             0x00D6
#define CMD_BIND_AGENT_TUNNEL             0x00D7
#define CMD_REQUEST_CERTIFICATE           0x00D8
#define CMD_NEW_CERTIFICATE               0x00D9
#define CMD_CREATE_MAP                    0x00DA
#define CMD_UPLOAD_FILE                   0x00DB
#define CMD_DELETE_FILE                   0x00DC
#define CMD_CONFIG_SET_CLOB               0x00E1
#define CMD_CONFIG_GET_CLOB               0x00E2
#define CMD_RENAME_MAP                    0x00E3
#define CMD_CLEAR_DCI_DATA                0x00E4
#define CMD_GET_LICENSE                   0x00E5
#define CMD_CHECK_LICENSE                 0x00E6
#define CMD_RELEASE_LICENSE               0x00E7
#define CMD_ISC_CONNECT_TO_SERVICE        0x00E8
#define CMD_REGISTER_AGENT                0x00E9
#define CMD_GET_SERVER_FILE               0x00EA
#define CMD_FORWARD_EVENT                 0x00EB
#define CMD_GET_USM_CREDENTIALS           0x00EC
#define CMD_UPDATE_USM_CREDENTIALS        0x00ED
#define CMD_GET_DCI_THRESHOLDS            0x00EE
#define CMD_GET_IMAGE                     0x00EF
#define CMD_CREATE_IMAGE                  0x00F0
#define CMD_DELETE_IMAGE                  0x00F1
#define CMD_MODIFY_IMAGE                  0x00F2
#define CMD_LIST_IMAGES                   0x00F3
#define CMD_LIST_SERVER_FILES             0x00F4
#define CMD_GET_TABLE                     0x00F5
#define CMD_QUERY_TABLE                   0x00F6
#define CMD_OPEN_CONSOLE                  0x00F7
#define CMD_CLOSE_CONSOLE                 0x00F8
#define CMD_GET_SELECTED_OBJECTS          0x00F9
#define CMD_GET_VLANS                     0x00FA
#define CMD_HOLD_JOB                      0x00FB
#define CMD_UNHOLD_JOB                    0x00FC
#define CMD_CHANGE_ZONE                   0x00FD
#define CMD_GET_AGENT_FILE                0x00FE
#define CMD_GET_FILE_DETAILS              0x00FF
#define CMD_IMAGE_LIBRARY_UPDATE          0x0100
#define CMD_GET_NODE_COMPONENTS           0x0101
#define CMD_UPDATE_ALARM_COMMENT          0x0102
#define CMD_GET_ALARM                     0x0103
#define CMD_GET_TABLE_LAST_VALUES         0x0104
#define CMD_GET_TABLE_DCI_DATA            0x0105
#define CMD_GET_THRESHOLD_SUMMARY         0x0106
#define CMD_RESOLVE_ALARM                 0x0107
#define CMD_FIND_IP_LOCATION              0x0108
#define CMD_REPORT_DEVICE_STATUS          0x0109
#define CMD_REPORT_DEVICE_INFO            0x010A
#define CMD_GET_ALARM_EVENTS              0x010B
#define CMD_GET_ENUM_LIST                 0x010C
#define CMD_GET_TABLE_LIST                0x010D
#define CMD_GET_MAPPING_TABLE             0x010E
#define CMD_UPDATE_MAPPING_TABLE          0x010F
#define CMD_DELETE_MAPPING_TABLE          0x0110
#define CMD_LIST_MAPPING_TABLES           0x0111
#define CMD_GET_NODE_SOFTWARE             0x0112
#define CMD_GET_WINPERF_OBJECTS           0x0113
#define CMD_GET_WIRELESS_STATIONS         0x0114
#define CMD_GET_SUMMARY_TABLES            0x0115
#define CMD_MODIFY_SUMMARY_TABLE          0x0116
#define CMD_DELETE_SUMMARY_TABLE          0x0117
#define CMD_GET_SUMMARY_TABLE_DETAILS     0x0118
#define CMD_QUERY_SUMMARY_TABLE           0x0119
#define CMD_SHUTDOWN                      0x011A
#define CMD_SNMP_TRAP                     0x011B
#define CMD_GET_SUBNET_ADDRESS_MAP        0x011C
#define CMD_FILE_MONITORING               0x011D
#define CMD_CANCEL_FILE_MONITORING        0x011E
#define CMD_CHANGE_OBJECT_TOOL_STATUS     0x011F
#define CMD_SET_ALARM_STATUS_FLOW         0x0120
#define CMD_DELETE_ALARM_COMMENT          0x0121
#define CMD_GET_EFFECTIVE_RIGHTS          0x0122
#define CMD_GET_DCI_VALUES                0x0123
#define CMD_GET_HELPDESK_URL              0x0124
#define CMD_UNLINK_HELPDESK_ISSUE         0x0125
#define CMD_GET_FOLDER_CONTENT            0x0126
#define CMD_FILEMGR_DELETE_FILE           0x0127
#define CMD_FILEMGR_RENAME_FILE           0x0128
#define CMD_FILEMGR_MOVE_FILE             0x0129
#define CMD_FILEMGR_UPLOAD                0x012A
#define CMD_GET_SWITCH_FDB                0x012B
#define CMD_COMMAND_OUTPUT                0x012C
#define CMD_GET_LOC_HISTORY               0x012D
#define CMD_TAKE_SCREENSHOT               0x012E
#define CMD_EXECUTE_SCRIPT                0x012F
#define CMD_EXECUTE_SCRIPT_UPDATE         0x0130
#define CMD_FILEMGR_CREATE_FOLDER         0x0131
#define CMD_QUERY_ADHOC_SUMMARY_TABLE     0x0132
#define CMD_GRAPH_UPDATE                  0x0133
#define CMD_SET_SERVER_CAPABILITIES       0x0134
#define CMD_FORCE_DCI_POLL                0x0135
#define CMD_GET_DCI_SCRIPT_LIST           0x0136
#define CMD_DATA_COLLECTION_CONFIG        0x0137
#define CMD_SET_SERVER_ID                 0x0138
#define CMD_GET_PUBLIC_CONFIG_VAR         0x0139
#define CMD_ENABLE_FILE_UPDATES           0x013A
#define CMD_DETACH_LDAP_USER              0x013B
#define CMD_VALIDATE_PASSWORD             0x013C
#define CMD_COMPILE_SCRIPT                0x013D
#define CMD_CLEAN_AGENT_DCI_CONF          0x013E
#define CMD_RESYNC_AGENT_DCI_CONF         0x013F
#define CMD_LIST_SCHEDULE_CALLBACKS       0x0140
#define CMD_LIST_SCHEDULES                0x0141
#define CMD_ADD_SCHEDULE                  0x0142
#define CMD_UPDATE_SCHEDULE               0x0143
#define CMD_REMOVE_SCHEDULE               0x0144
#define CMD_ENTER_MAINT_MODE              0x0145
#define CMD_LEAVE_MAINT_MODE              0x0146
#define CMD_JOIN_CLUSTER                  0x0147
#define CMD_CLUSTER_NOTIFY                0x0148
#define CMD_ZMQ_SUBSCRIBE_EVENT           0x0149
#define CMD_ZMQ_UNSUBSCRIBE_EVENT         0x014A
#define CMD_ZMQ_SUBSCRIBE_DATA            0x014B
#define CMD_ZMQ_UNSUBSCRIBE_DATA          0x014C
#define CMD_ZMQ_GET_EVT_SUBSCRIPTIONS     0x014D
#define CMD_ZMQ_GET_DATA_SUBSCRIPTIONS    0x014E
#define CMD_GET_REPOSITORIES              0x014F
#define CMD_ADD_REPOSITORY                0x0150
#define CMD_MODIFY_REPOSITORY             0x0151
#define CMD_DELETE_REPOSITORY             0x0152
#define CMD_GET_ALARM_CATEGORIES          0x0153
#define CMD_MODIFY_ALARM_CATEGORY         0x0154
#define CMD_DELETE_ALARM_CATEGORY         0x0155
#define CMD_ALARM_CATEGORY_UPDATE         0x0156
#define CMD_BULK_TERMINATE_ALARMS         0x0157
#define CMD_BULK_RESOLVE_ALARMS           0x0158
#define CMD_BULK_ALARM_STATE_CHANGE       0x0159
#define CMD_GET_FOLDER_SIZE               0x015A
#define CMD_FIND_HOSTNAME_LOCATION        0x015B
#define CMD_RESET_TUNNEL                  0x015C
#define CMD_CREATE_CHANNEL                0x015D
#define CMD_CHANNEL_DATA                  0x015E
#define CMD_CLOSE_CHANNEL                 0x015F
#define CMD_CREATE_OBJECT_ACCESS_SNAPSHOT 0x0160
#define CMD_UNBIND_AGENT_TUNNEL           0x0161
#define CMD_RESTART                       0x0162
#define CMD_REGISTER_LORAWAN_SENSOR       0x0163
#define CMD_UNREGISTER_LORAWAN_SENSOR     0x0164
#define CMD_EXPAND_MACROS                 0x0165
#define CMD_EXECUTE_ACTION_WITH_EXPANSION 0x0166
#define CMD_HOST_BY_IP                    0x0167
#define CMD_CANCEL_FILE_DOWNLOAD          0x0168
#define CMD_FILEMGR_COPY_FILE             0x0169
#define CMD_QUERY_OBJECTS                 0x016A
#define CMD_QUERY_OBJECT_DETAILS          0x016B
#define CMD_SETUP_TCP_PROXY               0x016C
#define CMD_TCP_PROXY_DATA                0x016D
#define CMD_CLOSE_TCP_PROXY               0x016E
#define CMD_GET_DEPENDENT_NODES           0x016F
#define CMD_DELETE_DCI_ENTRY              0x0170
#define CMD_GET_ACTIVE_THRESHOLDS         0x0171
#define CMD_QUERY_INTERNAL_TOPOLOGY       0x0172
#define CMD_GET_ACTION_LIST               0x0173
#define CMD_PROXY_MESSAGE                 0x0174
#define CMD_GET_GRAPH                     0x0175
#define CMD_UPDATE_AGENT_POLICY           0x0176
#define CMD_DELETE_AGENT_POLICY           0x0177
#define CMD_GET_AGENT_POLICY              0x0178
#define CMD_POLICY_EDITOR_CLOSED          0x0179
#define CMD_POLICY_FORCE_APPLY            0x017A
#define CMD_GET_NODE_HARDWARE             0x017B
#define CMD_GET_MATCHING_DCI              0x017C

#define CMD_RS_LIST_REPORTS            0x1100
#define CMD_RS_GET_REPORT              0x1101
#define CMD_RS_SCHEDULE_EXECUTION      0x1102
#define CMD_RS_LIST_RESULTS            0x1103
#define CMD_RS_RENDER_RESULT           0x1104
#define CMD_RS_DELETE_RESULT           0x1105
#define CMD_RS_LIST_SCHEDULES          0x1106
#define CMD_RS_DELETE_SCHEDULE         0x1107
#define CMD_RS_NOTIFY                  0x1108
#define CMD_RS_ADD_REPORT_NOTIFY       0x1109

/**
 * Variable identifiers
 */
#define VID_LOGIN_NAME              ((UINT32)1)
#define VID_PASSWORD                ((UINT32)2)
#define VID_OBJECT_ID               ((UINT32)3)
#define VID_OBJECT_NAME             ((UINT32)4)
#define VID_OBJECT_CLASS            ((UINT32)5)
#define VID_SNMP_VERSION            ((UINT32)6)
#define VID_PARENT_CNT              ((UINT32)7)
#define VID_IP_ADDRESS              ((UINT32)8)
#define VID_IP_NETMASK              ((UINT32)9)
#define VID_OBJECT_STATUS           ((UINT32)10)
#define VID_IF_INDEX                ((UINT32)11)
#define VID_IF_TYPE                 ((UINT32)12)
#define VID_FLAGS                   ((UINT32)13)
#define VID_CREATION_FLAGS          ((UINT32)14)
#define VID_AGENT_PORT              ((UINT32)15)
#define VID_AUTH_METHOD             ((UINT32)16)
#define VID_SHARED_SECRET           ((UINT32)17)
#define VID_SNMP_AUTH_OBJECT        ((UINT32)18)
#define VID_SNMP_OID                ((UINT32)19)
#define VID_NAME                    ((UINT32)20)
#define VID_VALUE                   ((UINT32)21)
#define VID_PEER_GATEWAY            ((UINT32)22)
#define VID_NOTIFICATION_CODE       ((UINT32)23)
#define VID_EVENT_CODE              ((UINT32)24)
#define VID_SEVERITY                ((UINT32)25)
#define VID_MESSAGE                 ((UINT32)26)
#define VID_DESCRIPTION             ((UINT32)27)
#define VID_RCC                     ((UINT32)28)    /* RCC == Request Completion Code */
#define VID_LOCKED_BY               ((UINT32)29)
#define VID_IS_DELETED              ((UINT32)30)
#define VID_CHILD_CNT               ((UINT32)31)
#define VID_ACL_SIZE                ((UINT32)32)
#define VID_INHERIT_RIGHTS          ((UINT32)33)
#define VID_USER_NAME               ((UINT32)34)
#define VID_USER_ID                 ((UINT32)35)
#define VID_USER_SYS_RIGHTS         ((UINT32)36)
#define VID_USER_FLAGS              ((UINT32)37)
#define VID_NUM_MEMBERS             ((UINT32)38)    /* Number of members in users group */
#define VID_IS_GROUP                ((UINT32)39)
#define VID_USER_FULL_NAME          ((UINT32)40)
#define VID_USER_DESCRIPTION        ((UINT32)41)
#define VID_UPDATE_TYPE             ((UINT32)42)
#define VID_DCI_ID                  ((UINT32)43)
#define VID_POLLING_INTERVAL        ((UINT32)44)
#define VID_RETENTION_TIME          ((UINT32)45)
#define VID_DCI_SOURCE_TYPE         ((UINT32)46)
#define VID_DCI_DATA_TYPE           ((UINT32)47)
#define VID_DCI_STATUS              ((UINT32)48)
#define VID_MGMT_STATUS             ((UINT32)49)
#define VID_MAX_ROWS                ((UINT32)50)
#define VID_TIME_FROM               ((UINT32)51)
#define VID_TIME_TO                 ((UINT32)52)
#define VID_DCI_DATA                ((UINT32)53)
#define VID_NUM_THRESHOLDS          ((UINT32)54)
#define VID_DCI_NUM_MAPS            ((UINT32)55)
#define VID_DCI_MAP_IDS             ((UINT32)56)
#define VID_DCI_MAP_INDEXES         ((UINT32)57)
#define VID_NUM_MIBS                ((UINT32)58)
#define VID_MIB_NAME                ((UINT32)59)
#define VID_MIB_FILE_SIZE           ((UINT32)60)
#define VID_MIB_FILE                ((UINT32)61)
#define VID_PROPERTIES              ((UINT32)62)
#define VID_ALARM_SEVERITY          ((UINT32)63)
#define VID_ALARM_KEY               ((UINT32)64)
#define VID_ALARM_TIMEOUT           ((UINT32)65)
#define VID_ALARM_MESSAGE           ((UINT32)66)
#define VID_RULE_ID                 ((UINT32)67)
#define VID_NUM_SOURCES             ((UINT32)68)
#define VID_NUM_EVENTS              ((UINT32)69)
#define VID_NUM_ACTIONS             ((UINT32)70)
#define VID_RULE_SOURCES            ((UINT32)71)
#define VID_RULE_EVENTS             ((UINT32)72)
#define VID_RULE_ACTIONS            ((UINT32)73)
#define VID_NUM_RULES               ((UINT32)74)
#define VID_CATEGORY                ((UINT32)75)
#define VID_UPDATED_CHILD_LIST      ((UINT32)76)
#define VID_EVENT_NAME_TABLE        ((UINT32)77)
#define VID_PARENT_ID               ((UINT32)78)
#define VID_CHILD_ID                ((UINT32)79)
#define VID_SNMP_PORT               ((UINT32)80)
#define VID_CONFIG_FILE_DATA        ((UINT32)81)
#define VID_COMMENTS                ((UINT32)82)
#define VID_POLICY_ID               ((UINT32)83)
#define VID_SNMP_USM_METHODS        ((UINT32)84)
#define VID_PARAMETER               ((UINT32)85)
#define VID_NUM_STRINGS             ((UINT32)86)
#define VID_ACTION_NAME             ((UINT32)87)
#define VID_NUM_ARGS                ((UINT32)88)
#define VID_SNMP_AUTH_PASSWORD      ((UINT32)89)
#define VID_CLASS_ID_LIST           ((UINT32)90)
#define VID_SNMP_PRIV_PASSWORD      ((UINT32)91)
#define VID_NOTIFICATION_DATA       ((UINT32)92)
#define VID_ALARM_ID                ((UINT32)93)
#define VID_TIMESTAMP               ((UINT32)94)
#define VID_ACK_BY_USER             ((UINT32)95)
#define VID_IS_ACK                  ((UINT32)96)
#define VID_ACTION_ID               ((UINT32)97)
#define VID_IS_DISABLED             ((UINT32)98)
#define VID_ACTION_TYPE             ((UINT32)99)
#define VID_ACTION_DATA             ((UINT32)100)
#define VID_EMAIL_SUBJECT           ((UINT32)101)
#define VID_RCPT_ADDR               ((UINT32)102)
#define VID_NPE_NAME                ((UINT32)103)
#define VID_CATEGORY_ID             ((UINT32)104)
#define VID_DCI_DELTA_CALCULATION   ((UINT32)105)
#define VID_TRANSFORMATION_SCRIPT   ((UINT32)106)
#define VID_POLL_TYPE               ((UINT32)107)
#define VID_POLLER_MESSAGE          ((UINT32)108)
#define VID_SOURCE_OBJECT_ID        ((UINT32)109)
#define VID_DESTINATION_OBJECT_ID   ((UINT32)110)
#define VID_NUM_ITEMS               ((UINT32)111)
#define VID_ITEM_LIST               ((UINT32)112)
#define VID_MAC_ADDR                ((UINT32)113)
#define VID_TEMPLATE_VERSION        ((UINT32)114)
#define VID_NODE_TYPE               ((UINT32)115)
#define VID_INSTANCE                ((UINT32)116)
#define VID_TRAP_ID                 ((UINT32)117)
#define VID_TRAP_OID                ((UINT32)118)
#define VID_TRAP_OID_LEN            ((UINT32)119)
#define VID_TRAP_NUM_MAPS           ((UINT32)120)
#define VID_SERVER_VERSION          ((UINT32)121)
#define VID_SUPPORTED_ENCRYPTION    ((UINT32)122)
#define VID_EVENT_ID                ((UINT32)123)
#define VID_AGENT_VERSION           ((UINT32)124)
#define VID_FILE_NAME               ((UINT32)125)
#define VID_PACKAGE_ID              ((UINT32)126)
#define VID_PACKAGE_VERSION         ((UINT32)127)
#define VID_PLATFORM_NAME           ((UINT32)128)
#define VID_PACKAGE_NAME            ((UINT32)129)
#define VID_SERVICE_TYPE            ((UINT32)130)
#define VID_IP_PROTO                ((UINT32)131)
#define VID_IP_PORT                 ((UINT32)132)
#define VID_SERVICE_REQUEST         ((UINT32)133)
#define VID_SERVICE_RESPONSE        ((UINT32)134)
#define VID_POLLER_NODE_ID          ((UINT32)135)
#define VID_SERVICE_STATUS          ((UINT32)136)
#define VID_NUM_PARAMETERS          ((UINT32)137)
#define VID_NUM_OBJECTS             ((UINT32)138)
#define VID_OBJECT_LIST             ((UINT32)139)
#define VID_DEPLOYMENT_STATUS       ((UINT32)140)
#define VID_ERROR_MESSAGE           ((UINT32)141)
#define VID_SERVER_ID               ((UINT32)142)
#define VID_SEARCH_PATTERN          ((UINT32)143)
#define VID_NUM_VARIABLES           ((UINT32)144)
#define VID_COMMAND                 ((UINT32)145)
#define VID_PROTOCOL_VERSION        ((UINT32)146)
#define VID_ZONE_UIN                ((UINT32)147)
#define VID_ZONING_ENABLED          ((UINT32)148)
#define VID_ICMP_PROXY              ((UINT32)149)
#define VID_IP_ADDRESS_COUNT        ((UINT32)150)
#define VID_ENABLED                 ((UINT32)151)
#define VID_REMOVE_DCI              ((UINT32)152)
#define VID_TEMPLATE_ID             ((UINT32)153)
#define VID_PUBLIC_KEY              ((UINT32)154)
#define VID_SESSION_KEY             ((UINT32)155)
#define VID_CIPHER                  ((UINT32)156)
#define VID_KEY_LENGTH              ((UINT32)157)
#define VID_SESSION_IV              ((UINT32)158)
#define VID_CONFIG_FILE             ((UINT32)159)
#define VID_STATUS_CALCULATION_ALG  ((UINT32)160)
#define VID_NUM_LOCAL_NETS          ((UINT32)161)
#define VID_NUM_REMOTE_NETS         ((UINT32)162)
#define VID_APPLY_FLAG              ((UINT32)163)
#define VID_NUM_TOOLS               ((UINT32)164)
#define VID_TOOL_ID                 ((UINT32)165)
#define VID_NUM_COLUMNS             ((UINT32)166)
#define VID_NUM_ROWS                ((UINT32)167)
#define VID_TABLE_TITLE             ((UINT32)168)
#define VID_EVENT_NAME              ((UINT32)169)
#define VID_CLIENT_TYPE             ((UINT32)170)
#define VID_LOG_NAME                ((UINT32)171)
#define VID_OPERATION               ((UINT32)172)
#define VID_MAX_RECORDS             ((UINT32)173)
#define VID_NUM_RECORDS             ((UINT32)174)
#define VID_CLIENT_INFO             ((UINT32)175)
#define VID_OS_INFO                 ((UINT32)176)
#define VID_LIBNXCL_VERSION         ((UINT32)177)
#define VID_VERSION                 ((UINT32)178)
#define VID_NUM_NODES               ((UINT32)179)
#define VID_LOG_FILE                ((UINT32)180)
#define VID_HOP_COUNT               ((UINT32)181)
#define VID_NUM_SCHEDULES           ((UINT32)182)
#define VID_STATUS_PROPAGATION_ALG  ((UINT32)183)
#define VID_FIXED_STATUS            ((UINT32)184)
#define VID_STATUS_SHIFT            ((UINT32)185)
#define VID_STATUS_TRANSLATION_1    ((UINT32)186)
#define VID_STATUS_TRANSLATION_2    ((UINT32)187)
#define VID_STATUS_TRANSLATION_3    ((UINT32)188)
#define VID_STATUS_TRANSLATION_4    ((UINT32)189)
#define VID_STATUS_SINGLE_THRESHOLD ((UINT32)190)
#define VID_STATUS_THRESHOLD_1      ((UINT32)191)
#define VID_STATUS_THRESHOLD_2      ((UINT32)192)
#define VID_STATUS_THRESHOLD_3      ((UINT32)193)
#define VID_STATUS_THRESHOLD_4      ((UINT32)194)
#define VID_AGENT_PROXY             ((UINT32)195)
#define VID_TOOL_TYPE               ((UINT32)196)
#define VID_TOOL_DATA               ((UINT32)197)
#define VID_ACL                     ((UINT32)198)
#define VID_TOOL_FILTER             ((UINT32)199)
#define VID_SERVER_UPTIME           ((UINT32)200)
#define VID_NUM_ALARMS              ((UINT32)201)
#define VID_ALARMS_BY_SEVERITY      ((UINT32)202)
#define VID_NETXMSD_PROCESS_WKSET   ((UINT32)203)
#define VID_NETXMSD_PROCESS_VMSIZE  ((UINT32)204)
#define VID_NUM_SESSIONS            ((UINT32)205)
#define VID_NUM_SCRIPTS             ((UINT32)206)
#define VID_SCRIPT_ID               ((UINT32)207)
#define VID_SCRIPT_CODE             ((UINT32)208)
#define VID_SESSION_ID              ((UINT32)209)
#define VID_RECORDS_ORDER           ((UINT32)210)
#define VID_NUM_SUBMAPS             ((UINT32)211)
#define VID_SUBMAP_LIST             ((UINT32)212)
#define VID_SUBMAP_ATTR             ((UINT32)213)
#define VID_NUM_LINKS               ((UINT32)214)
#define VID_LINK_LIST               ((UINT32)215)
#define VID_MAP_ID                  ((UINT32)216)
#define VID_NUM_MAPS                ((UINT32)217)
#define VID_NUM_MODULES             ((UINT32)218)
#define VID_DST_USER_ID             ((UINT32)219)
#define VID_MOVE_FLAG               ((UINT32)220)
#define VID_CHANGE_PASSWD_FLAG      ((UINT32)221)
#define VID_GUID                    ((UINT32)222)
#define VID_ACTIVATION_EVENT        ((UINT32)223)
#define VID_DEACTIVATION_EVENT      ((UINT32)224)
#define VID_SOURCE_OBJECT           ((UINT32)225)
#define VID_ACTIVE_STATUS           ((UINT32)226)
#define VID_INACTIVE_STATUS         ((UINT32)227)
#define VID_SCRIPT                  ((UINT32)228)
#define VID_NODE_LIST               ((UINT32)229)
#define VID_DCI_LIST                ((UINT32)230)
#define VID_CONFIG_ID               ((UINT32)231)
#define VID_FILTER                  ((UINT32)232)
#define VID_SEQUENCE_NUMBER         ((UINT32)233)
#define VID_VERSION_MAJOR           ((UINT32)234)
#define VID_VERSION_MINOR           ((UINT32)235)
#define VID_VERSION_RELEASE         ((UINT32)236)
#define VID_CONFIG_ID_2             ((UINT32)237)
#define VID_IV_LENGTH               ((UINT32)238)
#define VID_DBCONN_STATUS           ((UINT32)239)
#define VID_CREATION_TIME           ((UINT32)240)
#define VID_LAST_CHANGE_TIME        ((UINT32)241)
#define VID_TERMINATED_BY_USER      ((UINT32)242)
#define VID_STATE                   ((UINT32)243)
#define VID_CURRENT_SEVERITY        ((UINT32)244)
#define VID_ORIGINAL_SEVERITY       ((UINT32)245)
#define VID_HELPDESK_STATE          ((UINT32)246)
#define VID_HELPDESK_REF            ((UINT32)247)
#define VID_REPEAT_COUNT            ((UINT32)248)
#define VID_SNMP_RAW_VALUE_TYPE     ((UINT32)249)
#define VID_CONFIRMATION_TEXT       ((UINT32)250)
#define VID_FAILED_DCI_INDEX        ((UINT32)251)
#define VID_ADDR_LIST_TYPE          ((UINT32)252)
#define VID_COMPONENT_ID            ((UINT32)253)
#define VID_SYNC_COMMENTS           ((UINT32)254)
#define VID_EVENT_LIST              ((UINT32)255)
#define VID_NUM_TRAPS               ((UINT32)256)
#define VID_TRAP_LIST               ((UINT32)257)
#define VID_NXMP_CONTENT            ((UINT32)258)
#define VID_ERROR_TEXT              ((UINT32)259)
#define VID_COMPONENT               ((UINT32)260)
#define VID_CONSOLE_UPGRADE_URL		((UINT32)261)
#define VID_CLUSTER_TYPE				((UINT32)262)
#define VID_NUM_SYNC_SUBNETS			((UINT32)263)
#define VID_SYNC_SUBNETS				((UINT32)264)
#define VID_NUM_RESOURCES				((UINT32)265)
#define VID_RESOURCE_ID					((UINT32)266)
#define VID_SNMP_PROXY					((UINT32)267)
#define VID_PORT							((UINT32)268)
#define VID_PDU							((UINT32)269)
#define VID_PDU_SIZE						((UINT32)270)
#define VID_IS_SYSTEM					((UINT32)271)
#define VID_GRAPH_CONFIG				((UINT32)272)
#define VID_NUM_GRAPHS					((UINT32)273)
#define VID_GRAPH_ID						((UINT32)274)
#define VID_AUTH_TYPE					((UINT32)275)
#define VID_CERTIFICATE					((UINT32)276)
#define VID_SIGNATURE					((UINT32)277)
#define VID_CHALLENGE					((UINT32)278)
#define VID_CERT_MAPPING_METHOD		((UINT32)279)
#define VID_CERT_MAPPING_DATA       ((UINT32)280)
#define VID_CERTIFICATE_ID				((UINT32)281)
#define VID_NUM_CERTIFICATES        ((UINT32)282)
#define VID_ALARM_TIMEOUT_EVENT     ((UINT32)283)
#define VID_NUM_GROUPS              ((UINT32)284)
#define VID_QSIZE_CONDITION_POLLER  ((UINT32)285)
#define VID_QSIZE_CONF_POLLER       ((UINT32)286)
#define VID_QSIZE_DCI_POLLER        ((UINT32)287)
#define VID_QSIZE_DBWRITER          ((UINT32)288)
#define VID_QSIZE_EVENT             ((UINT32)289)
#define VID_QSIZE_DISCOVERY         ((UINT32)290)
#define VID_QSIZE_NODE_POLLER       ((UINT32)291)
#define VID_QSIZE_ROUTE_POLLER      ((UINT32)292)
#define VID_QSIZE_STATUS_POLLER     ((UINT32)293)
#define VID_SYNTHETIC_MASK          ((UINT32)294)
#define VID_SUBSYSTEM               ((UINT32)295)
#define VID_SUCCESS_AUDIT           ((UINT32)296)
#define VID_WORKSTATION             ((UINT32)297)
#define VID_USER_TAG                ((UINT32)298)
#define VID_REQUIRED_POLLS          ((UINT32)299)
#define VID_SYS_DESCRIPTION         ((UINT32)300)
#define VID_PSTORAGE_KEY            ((UINT32)301)
#define VID_PSTORAGE_VALUE          ((UINT32)302)
#define VID_NUM_DELETE_PSTORAGE     ((UINT32)303)
#define VID_INSTANCE_COUNT          ((UINT32)304)
#define VID_NUM_SET_PSTORAGE        ((UINT32)305)
#define VID_NUM_TRUSTED_NODES       ((UINT32)306)
#define VID_TRUSTED_NODES           ((UINT32)307)
#define VID_TIMEZONE                ((UINT32)308)
#define VID_NUM_CUSTOM_ATTRIBUTES   ((UINT32)309)
#define VID_MAP_DATA                ((UINT32)310)
#define VID_PRODUCT_ID              ((UINT32)311)
#define VID_CLIENT_ID               ((UINT32)312)
#define VID_LICENSE_DATA            ((UINT32)313)
#define VID_TOKEN                   ((UINT32)314)
#define VID_SERVICE_ID              ((UINT32)315)
#define VID_TOKEN_SOFTLIMIT         ((UINT32)316)
#define VID_TOKEN_HARDLIMIT         ((UINT32)317)
#define VID_USE_IFXTABLE            ((UINT32)318)
#define VID_USE_X509_KEY_FORMAT     ((UINT32)319)
#define VID_STICKY_FLAG             ((UINT32)320)
#define VID_AUTOBIND_FILTER         ((UINT32)321)
#define VID_BASE_UNITS              ((UINT32)322)
#define VID_MULTIPLIER              ((UINT32)323)
#define VID_CUSTOM_UNITS_NAME       ((UINT32)324)
#define VID_PERFTAB_SETTINGS        ((UINT32)325)
#define VID_EXECUTION_STATUS        ((UINT32)326)
#define VID_EXECUTION_RESULT        ((UINT32)327)
#define VID_TABLE_NUM_ROWS          ((UINT32)328)
#define VID_TABLE_NUM_COLS          ((UINT32)329)
#define VID_JOB_COUNT               ((UINT32)330)
#define VID_JOB_ID                  ((UINT32)331)
#define VID_JOB_TYPE                ((UINT32)332)
#define VID_JOB_STATUS              ((UINT32)333)
#define VID_JOB_PROGRESS            ((UINT32)334)
#define VID_FAILURE_MESSAGE         ((UINT32)335)
#define VID_POLICY_TYPE             ((UINT32)336)
#define VID_FIELDS                  ((UINT32)337)
#define VID_LOG_HANDLE              ((UINT32)338)
#define VID_START_ROW               ((UINT32)339)
#define VID_TABLE_OFFSET            ((UINT32)340)
#define VID_NUM_FILTERS             ((UINT32)341)
#define VID_GEOLOCATION_TYPE        ((UINT32)342)
#define VID_LATITUDE                ((UINT32)343)
#define VID_LONGITUDE               ((UINT32)344)
#define VID_NUM_ORDERING_COLUMNS    ((UINT32)345)
#define VID_SYSTEM_TAG              ((UINT32)346)
#define VID_NUM_ENUMS               ((UINT32)347)
#define VID_NUM_PUSH_PARAMETERS     ((UINT32)348)
#define VID_OLD_PASSWORD            ((UINT32)349)
#define VID_MIN_PASSWORD_LENGTH     ((UINT32)350)
#define VID_LAST_LOGIN              ((UINT32)351)
#define VID_LAST_PASSWORD_CHANGE    ((UINT32)352)
#define VID_DISABLED_UNTIL          ((UINT32)353)
#define VID_AUTH_FAILURES           ((UINT32)354)
#define VID_RUNTIME_FLAGS           ((UINT32)355)
#define VID_FILE_SIZE               ((UINT32)356)
#define VID_MAP_TYPE                ((UINT32)357)
#define VID_LAYOUT                  ((UINT32)358)
#define VID_SEED_OBJECTS            ((UINT32)359)
#define VID_BACKGROUND              ((UINT32)360)
#define VID_NUM_ELEMENTS            ((UINT32)361)
#define VID_INTERFACE_ID            ((UINT32)362)
#define VID_LOCAL_INTERFACE_ID      ((UINT32)363)
#define VID_LOCAL_NODE_ID           ((UINT32)364)
#define VID_SYS_NAME                ((UINT32)365)
#define VID_LLDP_NODE_ID            ((UINT32)366)
#define VID_IF_SLOT                 ((UINT32)367)
#define VID_IF_PORT                 ((UINT32)368)
#define VID_IMAGE_DATA              ((UINT32)369)
#define VID_IMAGE_PROTECTED         ((UINT32)370)
#define VID_NUM_IMAGES              ((UINT32)371)
#define VID_IMAGE_MIMETYPE          ((UINT32)372)
#define VID_PEER_NODE_ID            ((UINT32)373)
#define VID_PEER_INTERFACE_ID       ((UINT32)374)
#define VID_VRRP_VERSION            ((UINT32)375)
#define VID_VRRP_VR_COUNT           ((UINT32)376)
#define VID_DESTINATION_FILE_NAME   ((UINT32)377)
#define VID_NUM_TABLES              ((UINT32)378)
#define VID_IMAGE                   ((UINT32)379)
#define VID_DRIVER_NAME             ((UINT32)380)
#define VID_DRIVER_VERSION          ((UINT32)381)
#define VID_NUM_VLANS               ((UINT32)382)
#define VID_CREATE_JOB_ON_HOLD      ((UINT32)383)
#define VID_TILE_SERVER_URL         ((UINT32)384)
#define VID_BACKGROUND_LATITUDE     ((UINT32)385)
#define VID_BACKGROUND_LONGITUDE    ((UINT32)386)
#define VID_BACKGROUND_ZOOM         ((UINT32)387)
#define VID_BRIDGE_BASE_ADDRESS     ((UINT32)388)
#define VID_DRILL_DOWN_OBJECT_ID    ((UINT32)389)
#define VID_REPORT_DEFINITION       ((UINT32)390)
#define VID_SLMCHECK_TYPE           ((UINT32)391)
#define VID_REASON                  ((UINT32)392)
#define VID_NODE_ID                 ((UINT32)393)
#define VID_UPTIME_DAY              ((UINT32)394)
#define VID_UPTIME_WEEK             ((UINT32)395)
#define VID_UPTIME_MONTH            ((UINT32)396)
#define VID_PRIMARY_NAME            ((UINT32)397)
#define VID_NUM_RESULTS             ((UINT32)398)
#define VID_RESULT_ID_LIST          ((UINT32)399)
#define VID_RENDER_FORMAT           ((UINT32)400)
#define VID_FILE_OFFSET             ((UINT32)401)
#define VID_IS_TEMPLATE             ((UINT32)402)
#define VID_DOT1X_PAE_STATE         ((UINT32)403)
#define VID_DOT1X_BACKEND_STATE     ((UINT32)404)
#define VID_IS_COMPLETE             ((UINT32)405)
#define VID_MODIFICATION_TIME       ((UINT32)406)
#define VID_IS_PHYS_PORT            ((UINT32)407)
#define VID_CREATE_STATUS_DCI       ((UINT32)408)
#define VID_NUM_COMMENTS            ((UINT32)409)
#define VID_COMMENT_ID              ((UINT32)410)
#define VID_DCOBJECT_TYPE           ((UINT32)411)
#define VID_INSTANCE_COLUMN         ((UINT32)412)
#define VID_DATA_COLUMN             ((UINT32)413)
#define VID_ADMIN_STATE             ((UINT32)414)
#define VID_OPER_STATE              ((UINT32)415)
#define VID_EXPECTED_STATE          ((UINT32)416)
#define VID_LINK_COLOR              ((UINT32)417)
#define VID_CONNECTION_TYPE         ((UINT32)418)
#define VID_RESOLVED_BY_USER        ((UINT32)419)
#define VID_IS_STICKY               ((UINT32)420)
#define VID_DATE_FORMAT             ((UINT32)421)
#define VID_TIME_FORMAT             ((UINT32)422)
#define VID_LINK_ROUTING            ((UINT32)423)
#define VID_BACKGROUND_COLOR        ((UINT32)424)
#define VID_FORCE_RELOAD            ((UINT32)425)
#define VID_DISCOVERY_RADIUS        ((UINT32)426)
#define VID_BATTERY_LEVEL           ((UINT32)427)
#define VID_VENDOR                  ((UINT32)428)
#define VID_MODEL                   ((UINT32)429)
#define VID_OS_NAME                 ((UINT32)430)
#define VID_OS_VERSION              ((UINT32)431)
#define VID_SERIAL_NUMBER           ((UINT32)432)
#define VID_DEVICE_ID               ((UINT32)433)
#define VID_MAPPING_TABLE_ID        ((UINT32)434)
#define VID_INSTD_METHOD            ((UINT32)435)
#define VID_INSTD_DATA              ((UINT32)436)
#define VID_INSTD_FILTER            ((UINT32)437)
#define VID_ACCURACY                ((UINT32)438)
#define VID_GEOLOCATION_TIMESTAMP   ((UINT32)439)
#define VID_SAMPLE_COUNT            ((UINT32)440)
#define VID_HEIGHT                  ((UINT32)441)
#define VID_RADIO_COUNT             ((UINT32)442)
#define VID_OBJECT_TOOLTIP_ONLY     ((UINT32)443)
#define VID_SUMMARY_TABLE_ID        ((UINT32)444)
#define VID_MENU_PATH               ((UINT32)445)
#define VID_COLUMNS                 ((UINT32)446)
#define VID_TITLE                   ((UINT32)447)
#define VID_DAY_OF_WEEK             ((UINT32)448)
#define VID_DAY_OF_MONTH            ((UINT32)449)
#define VID_LOCALE                  ((UINT32)450)
#define VID_READ_ONLY               ((UINT32)451)
#define VID_CLIENT_ADDRESS          ((UINT32)452)
#define VID_SHORT_TIME_FORMAT       ((UINT32)453)
#define VID_BOOT_TIME               ((UINT32)454)
#define VID_REQUEST_ID              ((UINT32)455)
#define VID_ADDRESS_MAP             ((UINT32)456)
#define VID_XMPP_ID                 ((UINT32)457)
#define VID_FILE_SIZE_LIMIT         ((UINT32)458)
#define VID_FILE_FOLLOW             ((UINT32)459)
#define VID_FILE_DATA               ((UINT32)460)
#define VID_ALARM_STATUS_FLOW_STATE ((UINT32)461)
#define VID_GROUPS                  ((UINT32)462)
#define VID_EFFECTIVE_RIGHTS        ((UINT32)463)
#define VID_EXTENSION_COUNT         ((UINT32)464)
#define VID_TIMED_ALARM_ACK_ENABLED ((UINT32)465)
#define VID_TABLE_EXTENDED_FORMAT   ((UINT32)466)
#define VID_RS_JOB_ID               ((UINT32)467)
#define VID_RS_JOB_TYPE             ((UINT32)468)
#define VID_RS_REPORT_NAME          ((UINT32)469)
#define VID_HELPDESK_LINK_ACTIVE    ((UINT32)470)
#define VID_URL                     ((UINT32)471)
#define VID_PEER_PROTOCOL           ((UINT32)472)
#define VID_VIEW_REFRESH_INTERVAL   ((UINT32)473)
#define VID_COMMAND_NAME            ((UINT32)474)
#define VID_COMMAND_SHORT_NAME      ((UINT32)475)
#define VID_MODULE_DATA_COUNT       ((UINT32)476)
#define VID_NEW_FILE_NAME           ((UINT32)477)
#define VID_ALARM_LIST_DISP_LIMIT   ((UINT32)478)
#define VID_LANGUAGE                ((UINT32)479)
#define VID_ROOT                    ((UINT32)480)
#define VID_INCLUDE_NOVALUE_OBJECTS ((UINT32)481)
#define VID_RECEIVE_OUTPUT          ((UINT32)482)
#define VID_SESSION_STATE           ((UINT32)483)
#define VID_PAGE_SIZE               ((UINT32)484)
#define VID_EXECUTION_END_FLAG      ((UINT32)485)
#define VID_COUNTRY                 ((UINT32)486)
#define VID_CITY                    ((UINT32)487)
#define VID_STREET_ADDRESS          ((UINT32)488)
#define VID_POSTCODE                ((UINT32)489)
#define VID_FUNCTION                ((UINT32)490)
#define VID_RESPONSE_TIME           ((UINT32)491)
#define VID_QSIZE_DCI_CACHE_LOADER  ((UINT32)492)
#define VID_MTU                     ((UINT32)493)
#define VID_ALIAS                   ((UINT32)494)
#define VID_AP_INDEX                ((UINT32)495)
#define VID_PROTOCOL_VERSION_EX     ((UINT32)496)
#define VID_SCRIPT_LIST             ((UINT32)497)
#define VID_TOOL_LIST               ((UINT32)498)
#define VID_NUM_SUMMARY_TABLES      ((UINT32)499)
#define VID_SUMMARY_TABLE_LIST      ((UINT32)500)
#define VID_OVERVIEW_DCI_COUNT      ((UINT32)501)
#define VID_OVERVIEW_ONLY           ((UINT32)502)
#define VID_AGENT_CACHE_MODE        ((UINT32)503)
#define VID_DATE                    ((UINT32)504)
#define VID_RECONCILIATION          ((UINT32)505)
#define VID_DISPLAY_MODE            ((UINT32)506)
#define VID_NUM_FIELDS              ((UINT32)507)
#define VID_PASSWORD_IS_VALID       ((UINT32)508)
#define VID_SERIALIZE               ((UINT32)509)
#define VID_COMPILATION_STATUS      ((UINT32)510)
#define VID_ERROR_LINE              ((UINT32)511)
#define VID_SPEED                   ((UINT32)512)
#define VID_IFTABLE_SUFFIX          ((UINT32)513)
#define VID_SERVER_COMMAND_TIMEOUT  ((UINT32)514)
#define VID_SYS_CONTACT             ((UINT32)515)
#define VID_SYS_LOCATION            ((UINT32)516)
#define VID_RACK_ID                 ((UINT32)517)
#define VID_RACK_IMAGE_FRONT        ((UINT32)518)
#define VID_RACK_POSITION           ((UINT32)519)
#define VID_RACK_HEIGHT             ((UINT32)520)
#define VID_SCHEDULE_COUNT          ((UINT32)521)
#define VID_SCHEDULED_TASK_ID       ((UINT32)522)
#define VID_TASK_HANDLER            ((UINT32)523)
#define VID_SCHEDULE                ((UINT32)524)
#define VID_EXECUTION_TIME          ((UINT32)525)
#define VID_LAST_EXECUTION_TIME     ((UINT32)526)
#define VID_CALLBACK_COUNT          ((UINT32)527)
#define VID_DASHBOARDS              ((UINT32)528)
#define VID_OWNER                   ((UINT32)529)
#define VID_MAINTENANCE_MODE        ((UINT32)530)
#define VID_IS_MASTER               ((UINT32)531)
#define VID_AGENT_COMM_TIME         ((UINT32)532)
#define VID_GRAPH_TEMPALTE          ((UINT32)533)
#define VID_OVERWRITE               ((UINT32)534)
#define VID_IPV6_SUPPORT            ((UINT32)535)
#define VID_BULK_RECONCILIATION     ((UINT32)536)
#define VID_STATUS                  ((UINT32)537)
#define VID_FLAGS_MASK              ((UINT32)538)
#define VID_TOP_BOTTOM              ((UINT32)539)
#define VID_AUTH_TOKEN              ((UINT32)540)
#define VID_REPOSITORY_ID           ((UINT32)541)
#define VID_TOOLTIP_DCI_COUNT       ((UINT32)542)
#define VID_CONTROLLER_ID           ((UINT32)543)
#define VID_CHASSIS_ID              ((UINT32)544)
#define VID_NODE_SUBTYPE            ((UINT32)545)
#define VID_SSH_LOGIN               ((UINT32)546)
#define VID_SSH_PASSWORD            ((UINT32)547)
#define VID_SSH_PROXY               ((UINT32)548)
#define VID_ZONE_PROXY_COUNT        ((UINT32)549)
#define VID_MESSAGE_LENGTH          ((UINT32)550)
#define VID_LDAP_DN                 ((UINT32)551)
#define VID_LDAP_ID                 ((UINT32)552)
#define VID_FAIL_CODE_LIST          ((UINT32)553)
#define VID_FOLDER_SIZE             ((UINT32)554)
#define VID_ALARM_CATEGORY_ID       ((UINT32)555)
#define VID_FILE_COUNT              ((UINT32)556)
#define VID_ALARM_CATEGORY_ACL      ((UINT32)557)
#define VID_ALLOW_MULTIPART         ((UINT32)558)
#define VID_ALARM_ID_LIST           ((UINT32)559)
#define VID_NUM_COMPONENTS          ((UINT32)560)
#define VID_SERVER_NAME             ((UINT32)561)
#define VID_SERVER_COLOR            ((UINT32)562)
#define VID_MESSAGE_OF_THE_DAY      ((UINT32)563)
#define VID_PORT_ROW_COUNT          ((UINT32)564)
#define VID_PORT_NUMBERING_SCHEME   ((UINT32)565)
#define VID_NUM_VALUES              ((UINT32)566)
#define VID_NUM_PSTORAGE            ((UINT32)567)
#define VID_COMMAND_ID              ((UINT32)568)
#define VID_HOSTNAME                ((UINT32)569)
#define VID_ENABLE_COMPRESSION      ((UINT32)570)
#define VID_AGENT_COMPRESSION_MODE  ((UINT32)571)
#define VID_TRAP_TYPE               ((UINT32)572)
#define VID_IS_ACTIVE               ((UINT32)573)
#define VID_CHANNEL_ID              ((UINT32)574)
#define VID_NUM_URLS                ((UINT32)575)
#define VID_GRACE_LOGINS            ((UINT32)576)
#define VID_TUNNEL_GUID             ((UINT32)577)
#define VID_ORGANIZATION            ((UINT32)578)
#define VID_TUNNEL_ID               ((UINT32)579)
#define VID_PARENT_INTERFACE        ((UINT32)580)
#define VID_SENSOR_FLAGS            ((UINT32)581)
#define VID_DEVICE_CLASS            ((UINT32)582)
#define VID_COMM_PROTOCOL           ((UINT32)583)
#define VID_XML_CONFIG              ((UINT32)584)
#define VID_DEVICE_ADDRESS          ((UINT32)585)
#define VID_META_TYPE               ((UINT32)586)
#define VID_LAST_CONN_TIME          ((UINT32)587)
#define VID_FRAME_COUNT             ((UINT32)588)
#define VID_SIGNAL_STRENGHT         ((UINT32)589)
#define VID_SIGNAL_NOISE            ((UINT32)590)
#define VID_FREQUENCY               ((UINT32)591)
#define VID_SENSOR_PROXY            ((UINT32)592)
#define VID_XML_REG_CONFIG          ((UINT32)593)
#define VID_REG_TYPE                ((UINT32)594)
#define VID_DECODER                 ((UINT32)595)
#define VID_LORA_APP_EUI            ((UINT32)596)
#define VID_LORA_APP_KEY            ((UINT32)597)
#define VID_LORA_APP_S_KEY          ((UINT32)598)
#define VID_LORA_NWK_S_KWY          ((UINT32)599)
#define VID_DCI_NAME                ((UINT32)600)
#define VID_STATE_FLAGS             ((UINT32)601)
#define VID_CAPABILITIES            ((UINT32)602)
#define VID_INPUT_FIELD_COUNT       ((UINT32)603)
#define VID_STRING_COUNT            ((UINT32)604)
#define VID_EXPAND_STRING           ((UINT32)605)
#define VID_ACTION_LIST             ((UINT32)606)
#define VID_ZONE_SNMP_PORT_COUNT    ((UINT32)607)
#define VID_HISTORICAL_DATA_TYPE    ((UINT32)608)
#define VID_JOB_CANCELED            ((UINT32)609)
#define VID_INSTANCE_RETENTION      ((UINT32)610)
#define VID_RACK_ORIENTATION        ((UINT32)611)
#define VID_PASSIVE_ELEMENTS        ((UINT32)612)
#define VID_RACK_IMAGE_REAR         ((UINT32)613)
#define VID_INCLUDE_THRESHOLDS      ((UINT32)614)
#define VID_RESPONSIBLE_USERS       ((UINT32)615)
#define VID_AGENT_ID                ((UINT32)616)
#define VID_QUERY                   ((UINT32)617)
#define VID_CONFIG_HINT_COUNT       ((UINT32)618)
#define VID_DESTINATION_ADDRESS     ((UINT32)619)
#define VID_HYPERVISOR_TYPE         ((UINT32)620)
#define VID_HYPERVISOR_INFO         ((UINT32)621)
#define VID_VLAN_LIST               ((UINT32)622)
#define VID_TASK_KEY                ((UINT32)623)
#define VID_TIMER_COUNT             ((UINT32)624)
#define VID_AUTOBIND_FLAG           ((UINT32)625)
#define VID_AUTOUNBIND_FLAG         ((UINT32)626)
#define VID_TIMEOUT                 ((UINT32)627)
#define VID_PROGRESS                ((UINT32)628)
#define VID_POLICY_COUNT            ((UINT32)629)
#define VID_NEW_POLICY_TYPE         ((UINT32)630)
#define VID_USERAGENT               ((UINT32)631)
#define VID_ORDER_FIELDS            ((UINT32)632)
#define VID_RECORD_LIMIT            ((UINT32)633)
#define VID_LOCAL_CACHE             ((UINT32)634)
#define VID_RESTART                 ((UINT32)635)
#define VID_THIS_PROXY_ID           ((UINT32)636)
#define VID_ZONE_PROXY_LIST         ((UINT32)637)
#define VID_PRIMARY_ZONE_PROXY_ID   ((UINT32)638)
#define VID_BACKUP_ZONE_PROXY_ID    ((UINT32)639)

// Base variabe for single threshold in message
#define VID_THRESHOLD_BASE          ((UINT32)0x00800000)

// Map elements list base
#define VID_ELEMENT_LIST_BASE       ((UINT32)0x10000000)
#define VID_LINK_LIST_BASE          ((UINT32)0x40000000)

// Node info list base
#define VID_NODE_INFO_LIST_BASE     ((UINT32)0x60000000)

// Variable ranges for object's ACL
#define VID_ACL_USER_BASE           ((UINT32)0x00001000)
#define VID_ACL_USER_LAST           ((UINT32)0x00001FFF)
#define VID_ACL_RIGHTS_BASE         ((UINT32)0x00002000)
#define VID_ACL_RIGHTS_LAST         ((UINT32)0x00002FFF)

// Variable range for user group members
#define VID_GROUP_MEMBER_BASE       ((UINT32)0x00004000)
#define VID_GROUP_MEMBER_LAST       ((UINT32)0x00004FFF)

// Variable range for data collection object attributes
#define VID_DCI_COLUMN_BASE         ((UINT32)0x30000000)
#define VID_DCI_THRESHOLD_BASE      ((UINT32)0x20000000)
#define VID_DCI_SCHEDULE_BASE       ((UINT32)0x10000000)

// Variable range for event argument list
#define VID_EVENT_ARG_BASE          ((UINT32)0x00008000)
#define VID_EVENT_ARG_LAST          ((UINT32)0x00008FFF)

// Variable range for trap parameter list
#define VID_TRAP_PBASE              ((UINT32)0x00009000)

// Object information can contain variable number of parent and child objects' ids.
// Because each variable in message have to have unique identifier,
// we reserver a two range ids for this variables.
#define VID_PARENT_ID_BASE          ((UINT32)0x00003000)
#define VID_PARENT_ID_LAST          ((UINT32)0x00003FFF)

// Reservation of 0x7FFFFFFF ids for child object's list
#define VID_CHILD_ID_BASE           ((UINT32)0x80000000)
#define VID_CHILD_ID_LAST           ((UINT32)0xFFFFFFFE)

// Base value for custom attributes and module data
#define VID_CUSTOM_ATTRIBUTES_BASE  ((UINT32)0x70000000)
#define VID_MODULE_DATA_BASE        ((UINT32)0x71000000)

// Base value for overview DCI list
#define VID_OVERVIEW_DCI_LIST_BASE  ((UINT32)0x72000000)

// Base value for tooltip DCI list
#define VID_TOOLTIP_DCI_LIST_BASE   ((UINT32)0x73000000)

// Base value for URL list
#define VID_URL_LIST_BASE           ((UINT32)0x74000000)

// IP address list base
#define VID_IP_ADDRESS_LIST_BASE    ((UINT32)0x7F000000)

// Base value for cluster resource list
#define VID_RESOURCE_LIST_BASE      ((UINT32)0x20000000)

// Base value for cluster sync network list
#define VID_SYNC_SUBNETS_BASE       ((UINT32)0x28000000)

// Base value for agent's enum values
#define VID_ENUM_VALUE_BASE         ((UINT32)0x10000000)

// Base value for agent's action arguments
#define VID_ACTION_ARG_BASE         ((UINT32)0x10000000)

// Base value for agent's parameter list
#define VID_PARAM_LIST_BASE         ((UINT32)0x10000000)
#define VID_ENUM_LIST_BASE          ((UINT32)0x20000000)
#define VID_PUSHPARAM_LIST_BASE     ((UINT32)0x30000000)
#define VID_TABLE_LIST_BASE         ((UINT32)0x40000000)
#define VID_ACTION_LIST_BASE        ((UINT32)0x50000000)

// Base value for timer list
#define VID_TIMER_LIST_BASE         ((UINT32)0x60000000)

// Base value for DCI last values
#define VID_DCI_VALUES_BASE         ((UINT32)0x10000000)

// Base value for variable names
#define VID_VARLIST_BASE            ((UINT32)0x10000000)

// Base value for network list
#define VID_VPN_NETWORK_BASE        ((UINT32)0x10000000)

// Base value for network list
#define VID_OBJECT_TOOLS_BASE       ((UINT32)0x10000000)

// Base values for table data and object tools
#define VID_COLUMN_INFO_BASE        ((UINT32)0x10000000)
#define VID_COLUMN_NAME_BASE        ((UINT32)0x10000000)
#define VID_COLUMN_FMT_BASE         ((UINT32)0x20000000)
#define VID_ROW_DATA_BASE           ((UINT32)0x30000000)
#define VID_COLUMN_INFO_BASE        ((UINT32)0x10000000)
#define VID_FIELD_LIST_BASE         ((UINT32)0x70000000)
#define VID_ORDER_FIELD_LIST_BASE   ((UINT32)0x78000000)

// Base value for event log records
#define VID_EVENTLOG_MSG_BASE       ((UINT32)0x10000000)

// Base value for syslog records
#define VID_SYSLOG_MSG_BASE         ((UINT32)0x10000000)

// Base value for trap log records
#define VID_TRAP_LOG_MSG_BASE       ((UINT32)0x10000000)

// Base value for script list
#define VID_SCRIPT_LIST_BASE        ((UINT32)0x10000000)

// Base value for session data
#define VID_SESSION_DATA_BASE       ((UINT32)0x10000000)

// Base value for SNMP walker data
#define VID_SNMP_WALKER_DATA_BASE   ((UINT32)0x10000000)

// Base value for map list
#define VID_MAP_LIST_BASE           ((UINT32)0x10000000)

// Base value for module list
#define VID_MODULE_LIST_BASE        ((UINT32)0x10000000)

// Base value for agent configs list
#define VID_AGENT_CFG_LIST_BASE     ((UINT32)0x10000000)

// Base and last values for condition's DCI list
#define VID_DCI_LIST_BASE           ((UINT32)0x40000000)
#define VID_DCI_LIST_LAST           ((UINT32)0x4FFFFFFF)

// Base value for DCI push data
#define VID_PUSH_DCI_DATA_BASE      ((UINT32)0x10000000)

// Base value for address list
#define VID_ADDR_LIST_BASE          ((UINT32)0x10000000)

// Base value for trap configuration records
#define VID_TRAP_INFO_BASE          ((UINT32)0x10000000)

// Base value for graph list
#define VID_GRAPH_LIST_BASE         ((UINT32)0x10000000)
#define VID_GRAPH_ACL_BASE				((UINT32)0x20000000)

// Base value for system DCI list
#define VID_SYSDCI_LIST_BASE			((UINT32)0x10000000)

// Base value for certificate list
#define VID_CERT_LIST_BASE 			((UINT32)0x10000000)

// Base value for various string lists
#define VID_STRING_LIST_BASE 			((UINT32)0x10000000)

// Base values for persistent storage actions in epp
#define VID_PSTORAGE_SET_LIST_BASE     ((UINT32)0x10000000)
#define VID_PSTORAGE_DELETE_LIST_BASE  ((UINT32)0x20000000)

// Base values for persistent storage lists
#define VID_PSTORAGE_LIST_BASE     ((UINT32)0x10000000)

//Base values for file list
#define VID_INSTANCE_LIST_BASE      ((UINT32)0x20000000)

// Base value for object links list
#define VID_OBJECT_LINKS_BASE			((UINT32)0x10000000)
#define VID_SUBMAP_LINK_NAMES_BASE  ((UINT32)0x20000000)

#define VID_TABLE_COLUMN_INFO_BASE  ((UINT32)0x10000000)
#define VID_TABLE_DATA_BASE         ((UINT32)0x20000000)

#define VID_JOB_LIST_BASE           ((UINT32)0x10000000)

#define VID_COLUMN_FILTERS_BASE     ((UINT32)0x10000000)
#define VID_ORDERING_COLUMNS_BASE   ((UINT32)0x40000000)

#define VID_USM_CRED_LIST_BASE      ((UINT32)0x10000000)

#define VID_IMAGE_LIST_BASE         ((UINT32)0x10000000)

#define VID_VLAN_LIST_BASE          ((UINT32)0x10000000)

#define VID_NETWORK_PATH_BASE       ((UINT32)0x40000000)

#define VID_COMPONENT_LIST_BASE     ((UINT32)0x20000000)

#define VID_RADIO_LIST_BASE         ((UINT32)0x30000000)

#define VID_RULE_LIST_BASE          ((UINT32)0x10000000)

#define VID_EXTENSION_LIST_BASE     ((UINT32)0x10000000)

#define VID_DCI_VALUES_BASE         ((UINT32)0x10000000)

#define VID_FILE_LIST_BASE          ((UINT32)0x10000000)

#define VID_LOC_LIST_BASE           ((UINT32)0x10000000)

#define VID_SCHEDULE_LIST_BASE      ((UINT32)0x10000000)

#define VID_CALLBACK_BASE           ((UINT32)0x10000000)

#define VID_AGENT_POLICY_BASE       ((UINT32)0x10000000)

#define VID_EXPAND_STRING_BASE      ((UINT32)0x10000000)
#define VID_INPUT_FIELD_BASE        ((UINT32)0x20000000)

#define VID_ZMQ_SUBSCRIPTION_BASE   ((UINT32)0x10000000)

#define VID_CONFIG_HINT_LIST_BASE   ((UINT32)0x10000000)

#define VID_ZONE_PROXY_BASE         ((UINT32)0x70000000)

// base value for SNMP community strings
#define VID_COMMUNITY_STRING_LIST_BASE       ((UINT32)0x10000000)
#define VID_COMMUNITY_STRING_ZONE_LIST_BASE  ((UINT32)0x20000000)

#define VID_ZONE_SNMP_PORT_LIST_BASE         ((UINT32)0x10000000)

#ifdef __cplusplus

inline BOOL IsBinaryMsg(NXCP_MESSAGE *msg)
{
   return ntohs(msg->flags) & MF_BINARY;
}

#endif


#endif   /* _nms_cscp_h_ */
