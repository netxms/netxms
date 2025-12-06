/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#define NXCP_VERSION                   5

#define SERVER_LISTEN_PORT_FOR_CLIENTS 4701
#define SERVER_LISTEN_PORT_FOR_MOBILES 4747
#define MAX_DCI_STRING_VALUE           256
#define CLIENT_CHALLENGE_SIZE				256
#define NXCP_HEADER_SIZE               16
#define NXCP_ENCRYPTION_HEADER_SIZE    16
#define NXCP_EH_UNENCRYPTED_BYTES      8
#define NXCP_EH_ENCRYPTED_BYTES        (NXCP_ENCRYPTION_HEADER_SIZE - NXCP_EH_UNENCRYPTED_BYTES)

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

#pragma pack(1)

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
   uint32_t fieldId;  // Field identifier
   uint8_t type;       // Data type
   uint8_t flags;      // flags (may by type-dependent)
   uint16_t int16;
   union
   {
      int32_t int32;
      int64_t int64;
      uint32_t uint32;
      uint64_t uint64;
      double real;
      struct
      {
         uint32_t length;
         uint16_t value[1]; // actual size depends on length value
      } string;
      struct
      {
         uint32_t length;
         char value[1]; // actual size depends on length value
      } utf8string;
      struct
      {
         uint32_t length;
         BYTE value[1]; // actual size depends on length value
      } binary;
      struct
      {
         union
         {
            uint32_t v4;
            BYTE v6[16];
         } addr;
         uint8_t family;
         uint8_t maskBits;
         BYTE padding[6];
      } inetaddr;
   } data;
} NXCP_MESSAGE_FIELD;

#define df_int16        int16
#define df_int32        data.int32
#define df_uint32       data.uint32
#define df_int64        data.int64
#define df_uint64       data.uint64
#define df_real         data.real
#define df_string       data.string
#define df_utf8string   data.utf8string
#define df_binary       data.binary
#define df_inetaddr     data.inetaddr

/**
 * Message structure
 */
typedef struct
{
   uint16_t code;      // Message (command) code
   uint16_t flags;     // Message flags
   uint32_t size;      // Message size (including header) in bytes
   uint32_t id;        // Unique message identifier
   uint32_t numFields; // Number of fields in message
   NXCP_MESSAGE_FIELD fields[1];    // Data fields - actual length depends on value in numFields
} NXCP_MESSAGE;

/**
 * Encrypted payload header
 */
typedef struct
{
   uint32_t dwChecksum;
   uint32_t dwReserved; // Align to 8-byte boundary
} NXCP_ENCRYPTED_PAYLOAD_HEADER;

/**
 * Encrypted message structure
 */
typedef struct
{
   uint16_t code;       // Should be CMD_ENCRYPTED_MESSAGE
   BYTE padding;    // Number of bytes added to the end of message
   BYTE reserved;
   uint32_t size;    // Size of encrypted message (including encryption header and padding)
   BYTE data[1];     // Encrypted payload
} NXCP_ENCRYPTED_MESSAGE;

#pragma pack()

/**
 * Data types
 */
#define NXCP_DT_INT32         0
#define NXCP_DT_STRING        1
#define NXCP_DT_INT64         2
#define NXCP_DT_INT16         3
#define NXCP_DT_BINARY        4
#define NXCP_DT_FLOAT         5
#define NXCP_DT_INETADDR      6
#define NXCP_DT_UTF8_STRING   7

/**
 * Message flags
 */
#define MF_BINARY             0x0001   /* binary message indicator */
#define MF_END_OF_FILE        0x0002   /* end of file indicator */
#define MF_DONT_ENCRYPT       0x0004   /* prevent message encryption */
#define MF_END_OF_SEQUENCE    0x0008   /* end of message sequence indicator */
#define MF_REVERSE_ORDER      0x0010   /* indicator of reversed order of messages in a sequence */
#define MF_CONTROL            0x0020   /* control message indicator */
#define MF_COMPRESSED         0x0040   /* compressed message indicator */
#define MF_STREAM             0x0080   /* indicates that this message is part of data stream */
#define MF_DONT_COMPRESS      0x0100   /* prevent message compression */
#define MF_NXCP_VERSION(v)    (((v) & 0x0F) << 12) /* protocol version encoded in highest 4 bits */

/**
 * Message (command) codes
 */
#define CMD_LOGIN                         0x0001
#define CMD_LOGIN_RESPONSE                0x0002   /* Only used by mobile agent since 4.0 */
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
#define CMD_REQUEST_AI_ASSISTANT_COMMENT  0x001B
#define CMD_LOAD_EVENT_DB                 0x001C
#define CMD_REQUEST_COMPLETED             0x001D
#define CMD_LOAD_USER_DB                  0x001E
#define CMD_USER_DATA                     0x001F
#define CMD_GROUP_DATA                    0x0020
#define CMD_USER_DB_EOF                   0x0021
#define CMD_UPDATE_USER                   0x0022
#define CMD_DELETE_USER                   0x0023
#define CMD_CREATE_USER                   0x0024
#define CMD_2FA_GET_DRIVERS               0x0025
#define CMD_2FA_RENAME_METHOD             0x0026
#define CMD_USER_DB_UPDATE                0x0027
#define CMD_SET_PASSWORD                  0x0028
#define CMD_GET_NODE_DCI_LIST             0x0029
#define CMD_NODE_DCI                      0x002A
#define CMD_GET_LOG_DATA                  0x002B
#define CMD_DELETE_NODE_DCI               0x002C
#define CMD_MODIFY_NODE_DCI               0x002D
#define CMD_UNLOCK_NODE_DCI_LIST          0x002E
#define CMD_SET_OBJECT_MGMT_STATUS        0x002F
#define CMD_UPDATE_SYSTEM_ACCESS_RIGHTS   0x0030
#define CMD_GET_DCI_DATA                  0x0031
#define CMD_DCI_DATA                      0x0032
#define CMD_GET_MIB_TIMESTAMP             0x0033
#define CMD_GET_MIB                       0x0034
#define CMD_TEST_DCI_TRANSFORMATION       0x0035
#define CMD_QUERY_IP_TOPOLOGY             0x0036
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
#define CMD_POLL_OBJECT                   0x0059
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
#define CMD_THRESHOLD_UPDATE              0x0070
#define CMD_GET_SELECTED_USERS            0x0071
#define CMD_ABORT_FILE_TRANSFER           0x0072
#define CMD_CHECK_NETWORK_SERVICE         0x0073
#define CMD_READ_AGENT_CONFIG_FILE        0x0074
#define CMD_WRITE_AGENT_CONFIG_FILE       0x0075
#define CMD_GET_PARAMETER_LIST            0x0076
#define CMD_DEPLOY_PACKAGE                0x0077
#define CMD_PACKAGE_DEPLOYMENT_JOB_UPDATE 0x0078
#define CMD_GET_DATA_COLLECTION_SUMMARY   0x0079
#define CMD_REQUEST_AUTH_TOKEN            0x007A
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
#define CMD_START_CONFIG_BACKUP_JOB       0x0089
#define CMD_CHANGE_SUBSCRIPTION           0x008A
#define CMD_SET_CONFIG_TO_DEFAULT         0x008B
#define CMD_SYSLOG_RECORDS                0x008C
#define CMD_GET_LAST_CONFIG_BACKUP        0x008D
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
#define CMD_GET_ASSET_MANAGEMENT_SCHEMA   0x00A3
#define CMD_CREATE_ASSET_ATTRIBUTE        0x00A4
#define CMD_UPDATE_ASSET_ATTRIBUTE        0x00A5
#define CMD_DELETE_ASSET_ATTRIBUTE        0x00A6
#define CMD_SET_ASSET_PROPERTY            0x00A7
#define CMD_DELETE_ASSET_PROPERTY         0x00A8
#define CMD_LINK_ASSET                    0x00A9
#define CMD_UNLINK_ASSET                  0x00AA
#define CMD_GET_USER_SESSIONS             0x00AB
#define CMD_GET_ARP_CACHE                 0x00AC
#define CMD_COPY_USER_VARIABLE            0x00AD
#define CMD_RESOLVE_DCI_NAMES             0x00AE
#define CMD_GET_MY_CONFIG                 0x00AF
#define CMD_GET_AGENT_CONFIGURATION_LIST  0x00B0
#define CMD_GET_AGENT_CONFIGURATION       0x00B1
#define CMD_UPDATE_AGENT_CONFIGURATION    0x00B2
#define CMD_DELETE_AGENT_CONFIGURATION    0x00B3
#define CMD_SWAP_AGENT_CONFIGURATIONS     0x00B4
#define CMD_TERMINATE_ALARM               0x00B5
#define CMD_GET_NXCP_CAPS                 0x00B6
#define CMD_NXCP_CAPS                     0x00B7
#define CMD_STOP_SCRIPT                   0x00B8
#define CMD_UPDATE_OBJECT_COMMENTS        0x00B9
#define CMD_ENABLE_AGENT_TRAPS            0x00BA
#define CMD_PUSH_DCI_DATA                 0x00BB
#define CMD_GET_ADDR_LIST                 0x00BC
#define CMD_SET_ADDR_LIST                 0x00BD
#define CMD_RESET_COMPONENT               0x00BE
#define CMD_GET_RELATED_EVENTS_LIST       0x00BF
#define CMD_EXPORT_CONFIGURATION          0x00C0
#define CMD_IMPORT_CONFIGURATION          0x00C1
#define CMD_GET_TRAP_CFG_RO               0x00C2
#define CMD_SNMP_REQUEST                  0x00C3
#define CMD_GET_DCI_INFO                  0x00C4
#define CMD_GET_GRAPH_LIST                0x00C5
#define CMD_SAVE_GRAPH                    0x00C6
#define CMD_DELETE_GRAPH                  0x00C7
#define CMD_GET_PERFTAB_DCI_LIST          0x00C8
#define CMD_GET_OBJECT_CATEGORIES         0x00C9
#define CMD_MODIFY_OBJECT_CATEGORY        0x00CA
#define CMD_DELETE_OBJECT_CATEGORY        0x00CB
#define CMD_WINDOWS_EVENT                 0x00CC
#define CMD_QUERY_L2_TOPOLOGY             0x00CD
#define CMD_AUDIT_RECORD                  0x00CE
#define CMD_GET_AUDIT_LOG                 0x00CF
#define CMD_SEND_NOTIFICATION             0x00D0
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
#define CMD_RENAME_FILE                   0x00DD
#define CMD_CLONE_MAP                     0x00DE
#define CMD_AGENT_TUNNEL_UPDATE           0x00DF
#define CMD_FIND_VENDOR_BY_MAC            0x00E0
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
#define CMD_GET_SYSTEM_TIME               0x00FB
#define CMD_SET_SYSTEM_TIME               0x00FC
#define CMD_CHANGE_ZONE                   0x00FD
#define CMD_GET_AGENT_FILE                0x00FE
#define CMD_GET_FILE_DETAILS              0x00FF
#define CMD_IMAGE_LIBRARY_UPDATE          0x0100
#define CMD_GET_NODE_COMPONENTS           0x0101
#define CMD_UPDATE_ALARM_COMMENT          0x0102
#define CMD_GET_ALARM                     0x0103
#define CMD_GET_TABLE_LAST_VALUE          0x0104
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
#define CMD_GET_OBJECT_QUERIES            0x0149
#define CMD_MODIFY_OBJECT_QUERY           0x014A
#define CMD_DELETE_OBJECT_QUERY           0x014B
#define CMD_FILEMGR_CHMOD                 0x014C
#define CMD_FILEMGR_CHOWN                 0x014D
#define CMD_FILEMGR_GET_FILE_FINGERPRINT  0x014E
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
#define CMD_GET_HOSTNAME_BY_IPADDR        0x0167
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
#define CMD_GET_AGENT_POLICY_LIST         0x0178
#define CMD_POLICY_EDITOR_CLOSED          0x0179
#define CMD_POLICY_FORCE_APPLY            0x017A
#define CMD_GET_NODE_HARDWARE             0x017B
#define CMD_GET_MATCHING_DCI              0x017C
#define CMD_GET_UA_NOTIFICATIONS          0x017D
#define CMD_ADD_UA_NOTIFICATION           0x017E
#define CMD_RECALL_UA_NOTIFICATION        0x017F
#define CMD_UPDATE_UA_NOTIFICATIONS       0x0180
#define CMD_GET_NOTIFICATION_CHANNELS     0x0181
#define CMD_ADD_NOTIFICATION_CHANNEL      0x0182
#define CMD_UPDATE_NOTIFICATION_CHANNEL   0x0183
#define CMD_DELETE_NOTIFICATION_CHANNEL   0x0184
#define CMD_GET_NOTIFICATION_DRIVERS      0x0185
#define CMD_RENAME_NOTIFICATION_CHANNEL   0x0186
#define CMD_GET_AGENT_POLICY              0x0187
#define CMD_START_ACTIVE_DISCOVERY        0x0188
#define CMD_GET_PHYSICAL_LINKS            0x0189
#define CMD_UPDATE_PHYSICAL_LINK          0x018A
#define CMD_DELETE_PHYSICAL_LINK          0x018B
#define CMD_GET_FILE_SET_DETAILS          0x018C
#define CMD_IMPORT_CONFIGURATION_FILE     0x018D
#define CMD_GET_DEVICE_VIEW               0x018E
#define CMD_MAP_ELEMENT_UPDATE            0x018F        
#define CMD_GET_BACKGROUND_TASK_STATE     0x0190
#define CMD_QUERY_WEB_SERVICE             0x0191
#define CMD_GET_WEB_SERVICES              0x0192
#define CMD_MODIFY_WEB_SERVICE            0x0193
#define CMD_DELETE_WEB_SERVICE            0x0194
#define CMD_WEB_SERVICE_DEFINITION        0x0195
#define CMD_GET_SCREEN_INFO               0x0196
#define CMD_UPDATE_ENVIRONMENT            0x0197
#define CMD_GET_SHARED_SECRET_LIST        0x0198
#define CMD_UPDATE_SHARED_SECRET_LIST     0x0199
#define CMD_GET_WELL_KNOWN_PORT_LIST      0x019A
#define CMD_UPDATE_WELL_KNOWN_PORT_LIST   0x019B
#define CMD_GET_LOG_RECORD_DETAILS        0x019C
#define CMD_GET_DCI_LAST_VALUE            0x019D
#define CMD_OBJECT_CATEGORY_UPDATE        0x019E
#define CMD_GET_GEO_AREAS                 0x019F
#define CMD_MODIFY_GEO_AREA               0x01A0
#define CMD_DELETE_GEO_AREA               0x01A1
#define CMD_GEO_AREA_UPDATE               0x01A2
#define CMD_FIND_PROXY_FOR_NODE           0x01A3
#define CMD_BULK_DCI_UPDATE               0x01A4
#define CMD_GET_SCHEDULED_REPORTING_TASKS 0x01A5
#define CMD_CONFIGURE_REPORTING_SERVER    0x01A6
#define CMD_GET_SSH_KEYS_LIST             0x01A7
#define CMD_DELETE_SSH_KEY                0x01A8
#define CMD_UPDATE_SSH_KEYS               0x01A9
#define CMD_GENERATE_SSH_KEYS             0x01AA
#define CMD_GET_SSH_KEYS                  0x01AB
#define CMD_GET_TOOLTIP_LAST_VALUES       0x01AC
#define CMD_SYNC_AGENT_POLICIES           0x01AD
#define CMD_2FA_PREPARE_CHALLENGE         0x01AE
#define CMD_2FA_VALIDATE_RESPONSE         0x01AF
#define CMD_2FA_GET_METHODS               0x01B0
#define CMD_GET_OSPF_DATA                 0x01B1
#define CMD_2FA_MODIFY_METHOD             0x01B2
#define CMD_2FA_DELETE_METHOD             0x01B3
#define CMD_2FA_GET_USER_BINDINGS         0x01B4
#define CMD_SCRIPT_EXECUTION_RESULT       0x01B5
#define CMD_2FA_MODIFY_USER_BINDING       0x01B6
#define CMD_2FA_DELETE_USER_BINDING       0x01B7
#define CMD_WEB_SERVICE_CUSTOM_REQUEST    0x01B8
#define CMD_QUERY_OSPF_TOPOLOGY           0x01B9
#define CMD_FILEMGR_MERGE_FILES           0x01BA
#define CMD_GET_BIZSVC_CHECK_LIST         0x01BB
#define CMD_UPDATE_BIZSVC_CHECK           0x01BC
#define CMD_DELETE_BIZSVC_CHECK           0x01BD
#define CMD_GET_BUSINESS_SERVICE_UPTIME   0x01BE
#define CMD_GET_BUSINESS_SERVICE_TICKETS  0x01BF
#define CMD_SSH_COMMAND                   0x01C0
#define CMD_FIND_DCI                      0x01C1
#define CMD_UPDATE_PACKAGE_METADATA       0x01C2
#define CMD_GET_EVENT_REFERENCES          0x01C3
#define CMD_READ_MAINTENANCE_JOURNAL      0x01C4
#define CMD_WRITE_MAINTENANCE_JOURNAL     0x01C5
#define CMD_UPDATE_MAINTENANCE_JOURNAL    0x01C6
#define CMD_GET_SSH_CREDENTIALS           0x01C7
#define CMD_UPDATE_SSH_CREDENTIALS        0x01C8
#define CMD_UPDATE_RESPONSIBLE_USERS      0x01C9
#define CMD_REVOKE_AUTH_TOKEN             0x01CA
#define CMD_GET_AUTH_TOKENS               0x01CB
#define CMD_ENABLE_ANONYMOUS_ACCESS       0x01CC
#define CMD_ADD_WIRELESS_DOMAIN_CNTRL     0x01CD
#define CMD_PROGRESS_REPORT               0x01CE
#define CMD_COMPILE_MIB_FILES             0x01CF
#define CMD_EXECUTE_DASHBOARD_SCRIPT      0x01D0
#define CMD_UPDATE_PEER_INTERFACE         0x01D1
#define CMD_CLEAR_PEER_INTERFACE          0x01D2
#define CMD_GET_PACKAGE_DEPLOYMENT_JOBS   0x01D3
#define CMD_CANCEL_PACKAGE_DEPLOYMENT_JOB 0x01D4
#define CMD_QUERY_AI_ASSISTANT            0x01D5
#define CMD_GET_SMCLP_PROPERTIES          0x01D6
#define CMD_GET_INTERFACE_TRAFFIC_DCIS    0x01D7
#define CMD_SET_COMPONENT_TOKEN           0x01D8
#define CMD_CLEAR_AI_ASSISTANT_CHAT       0x01D9
#define CMD_LINK_NETWORK_MAP_NODES        0x01DA
#define CMD_GET_DC_OBJECT                 0x01DB
#define CMD_GET_AI_ASSISTANT_FUNCTIONS    0x01DC
#define CMD_CALL_AI_ASSISTANT_FUNCTION    0x01DD
#define CMD_GET_AI_AGENT_TASKS            0x01DE
#define CMD_DELETE_AI_AGENT_TASK          0x01DF
#define CMD_ADD_AI_AGENT_TASK             0x01E0
#define CMD_CREATE_AI_ASSISTANT_CHAT      0x01E1
#define CMD_DELETE_AI_ASSISTANT_CHAT      0x01E2

#define CMD_RS_LIST_REPORTS               0x1100
#define CMD_RS_GET_REPORT_DEFINITION      0x1101
#define CMD_RS_EXECUTE_REPORT             0x1102
#define CMD_RS_LIST_RESULTS               0x1103
#define CMD_RS_RENDER_RESULT              0x1104
#define CMD_RS_DELETE_RESULT              0x1105
#define CMD_RS_NOTIFY                     0x1106

/**
 * Variable identifiers
 */
#define VID_LOGIN_NAME              ((uint32_t)1)
#define VID_PASSWORD                ((uint32_t)2)
#define VID_OBJECT_ID               ((uint32_t)3)
#define VID_OBJECT_NAME             ((uint32_t)4)
#define VID_OBJECT_CLASS            ((uint32_t)5)
#define VID_SNMP_VERSION            ((uint32_t)6)
#define VID_PARENT_CNT              ((uint32_t)7)
#define VID_IP_ADDRESS              ((uint32_t)8)
#define VID_IP_NETMASK              ((uint32_t)9)
#define VID_OBJECT_STATUS           ((uint32_t)10)
#define VID_IF_INDEX                ((uint32_t)11)
#define VID_IF_TYPE                 ((uint32_t)12)
#define VID_FLAGS                   ((uint32_t)13)
#define VID_CREATION_FLAGS          ((uint32_t)14)
#define VID_AGENT_PORT              ((uint32_t)15)
#define VID_AUTH_METHOD             ((uint32_t)16)
#define VID_SHARED_SECRET           ((uint32_t)17)
#define VID_SNMP_AUTH_OBJECT        ((uint32_t)18)
#define VID_SNMP_OID                ((uint32_t)19)
#define VID_NAME                    ((uint32_t)20)
#define VID_VALUE                   ((uint32_t)21)
#define VID_PEER_GATEWAY            ((uint32_t)22)
#define VID_NOTIFICATION_CODE       ((uint32_t)23)
#define VID_EVENT_CODE              ((uint32_t)24)
#define VID_SEVERITY                ((uint32_t)25)
#define VID_MESSAGE                 ((uint32_t)26)
#define VID_DESCRIPTION             ((uint32_t)27)
#define VID_RCC                     ((uint32_t)28)    /* RCC == Request Completion Code */
#define VID_LOCKED_BY               ((uint32_t)29)
#define VID_IS_DELETED              ((uint32_t)30)
#define VID_CHILD_CNT               ((uint32_t)31)
#define VID_ACL_SIZE                ((uint32_t)32)
#define VID_INHERIT_RIGHTS          ((uint32_t)33)
#define VID_USER_NAME               ((uint32_t)34)
#define VID_USER_ID                 ((uint32_t)35)
#define VID_USER_SYS_RIGHTS         ((uint32_t)36)
#define VID_USER_FLAGS              ((uint32_t)37)
#define VID_NUM_MEMBERS             ((uint32_t)38)    /* Number of members in users group */
#define VID_IS_GROUP                ((uint32_t)39)
#define VID_USER_FULL_NAME          ((uint32_t)40)
#define VID_USER_DESCRIPTION        ((uint32_t)41)
#define VID_UPDATE_TYPE             ((uint32_t)42)
#define VID_DCI_ID                  ((uint32_t)43)
#define VID_POLLING_INTERVAL        ((uint32_t)44)
#define VID_RETENTION_TIME          ((uint32_t)45)
#define VID_DCI_SOURCE_TYPE         ((uint32_t)46)
#define VID_DCI_DATA_TYPE           ((uint32_t)47)
#define VID_DCI_STATUS              ((uint32_t)48)
#define VID_MGMT_STATUS             ((uint32_t)49)
#define VID_MAX_ROWS                ((uint32_t)50)
#define VID_TIME_FROM               ((uint32_t)51)
#define VID_TIME_TO                 ((uint32_t)52)
#define VID_DCI_DATA                ((uint32_t)53)
#define VID_NUM_THRESHOLDS          ((uint32_t)54)
#define VID_DCI_NUM_MAPS            ((uint32_t)55)
#define VID_DCI_MAP_IDS             ((uint32_t)56)
#define VID_DCI_MAP_INDEXES         ((uint32_t)57)
#define VID_NUM_MIBS                ((uint32_t)58)
#define VID_MIB_NAME                ((uint32_t)59)
#define VID_MIB_FILE_SIZE           ((uint32_t)60)
#define VID_MIB_FILE                ((uint32_t)61)
#define VID_PROPERTIES              ((uint32_t)62)
#define VID_ALARM_SEVERITY          ((uint32_t)63)
#define VID_ALARM_KEY               ((uint32_t)64)
#define VID_ALARM_TIMEOUT           ((uint32_t)65)
#define VID_ALARM_MESSAGE           ((uint32_t)66)
#define VID_RULE_ID                 ((uint32_t)67)
#define VID_NUM_SOURCES             ((uint32_t)68)
#define VID_NUM_EVENTS              ((uint32_t)69)
#define VID_NUM_ACTIONS             ((uint32_t)70)
#define VID_RULE_SOURCES            ((uint32_t)71)
#define VID_RULE_EVENTS             ((uint32_t)72)
#define VID_RULE_ACTIONS            ((uint32_t)73)
#define VID_NUM_RULES               ((uint32_t)74)
#define VID_CATEGORY                ((uint32_t)75)
#define VID_UPDATED_CHILD_LIST      ((uint32_t)76)
#define VID_EVENT_NAME_TABLE        ((uint32_t)77)
#define VID_PARENT_ID               ((uint32_t)78)
#define VID_CHILD_ID                ((uint32_t)79)
#define VID_SNMP_PORT               ((uint32_t)80)
#define VID_CONFIG_FILE_DATA        ((uint32_t)81)
#define VID_COMMENTS                ((uint32_t)82)
#define VID_POLICY_ID               ((uint32_t)83)
#define VID_SNMP_USM_METHODS        ((uint32_t)84)
#define VID_PARAMETER               ((uint32_t)85)
#define VID_NUM_STRINGS             ((uint32_t)86)
#define VID_ACTION_NAME             ((uint32_t)87)
#define VID_NUM_ARGS                ((uint32_t)88)
#define VID_SNMP_AUTH_PASSWORD      ((uint32_t)89)
#define VID_CLASS_ID_LIST           ((uint32_t)90)
#define VID_SNMP_PRIV_PASSWORD      ((uint32_t)91)
#define VID_NOTIFICATION_DATA       ((uint32_t)92)
#define VID_ALARM_ID                ((uint32_t)93)
#define VID_TIMESTAMP               ((uint32_t)94)
#define VID_ACK_BY_USER             ((uint32_t)95)
#define VID_IS_ACK                  ((uint32_t)96)
#define VID_ACTION_ID               ((uint32_t)97)
#define VID_IS_DISABLED             ((uint32_t)98)
#define VID_ACTION_TYPE             ((uint32_t)99)
#define VID_ACTION_DATA             ((uint32_t)100)
#define VID_EMAIL_SUBJECT           ((uint32_t)101)
#define VID_RCPT_ADDR               ((uint32_t)102)
#define VID_NPE_NAME                ((uint32_t)103)
#define VID_CATEGORY_ID             ((uint32_t)104)
#define VID_DCI_DELTA_CALCULATION   ((uint32_t)105)
#define VID_TRANSFORMATION_SCRIPT   ((uint32_t)106)
#define VID_POLL_TYPE               ((uint32_t)107)
#define VID_POLLER_MESSAGE          ((uint32_t)108)
#define VID_SOURCE_OBJECT_ID        ((uint32_t)109)
#define VID_DESTINATION_OBJECT_ID   ((uint32_t)110)
#define VID_NUM_ITEMS               ((uint32_t)111)
#define VID_ITEM_LIST               ((uint32_t)112)
#define VID_MAC_ADDR                ((uint32_t)113)
#define VID_TEMPLATE_VERSION        ((uint32_t)114)
#define VID_NODE_TYPE               ((uint32_t)115)
#define VID_INSTANCE                ((uint32_t)116)
#define VID_TRAP_ID                 ((uint32_t)117)
#define VID_TRAP_OID                ((uint32_t)118)
#define VID_TRAP_OID_LEN            ((uint32_t)119)
#define VID_TRAP_NUM_MAPS           ((uint32_t)120)
#define VID_SERVER_VERSION          ((uint32_t)121)
#define VID_SUPPORTED_ENCRYPTION    ((uint32_t)122)
#define VID_EVENT_ID                ((uint32_t)123)
#define VID_AGENT_VERSION           ((uint32_t)124)
#define VID_FILE_NAME               ((uint32_t)125)
#define VID_PACKAGE_ID              ((uint32_t)126)
#define VID_PACKAGE_VERSION         ((uint32_t)127)
#define VID_PLATFORM_NAME           ((uint32_t)128)
#define VID_PACKAGE_NAME            ((uint32_t)129)
#define VID_SERVICE_TYPE            ((uint32_t)130)
#define VID_IP_PROTO                ((uint32_t)131)
#define VID_IP_PORT                 ((uint32_t)132)
#define VID_SERVICE_REQUEST         ((uint32_t)133)
#define VID_SERVICE_RESPONSE        ((uint32_t)134)
#define VID_POLLER_NODE_ID          ((uint32_t)135)
#define VID_SERVICE_STATUS          ((uint32_t)136)
#define VID_NUM_PARAMETERS          ((uint32_t)137)
#define VID_NUM_OBJECTS             ((uint32_t)138)
#define VID_OBJECT_LIST             ((uint32_t)139)
#define VID_DEPLOYMENT_STATUS       ((uint32_t)140)
#define VID_ERROR_MESSAGE           ((uint32_t)141)
#define VID_SERVER_ID               ((uint32_t)142)
#define VID_SEARCH_PATTERN          ((uint32_t)143)
#define VID_NUM_VARIABLES           ((uint32_t)144)
#define VID_COMMAND                 ((uint32_t)145)
#define VID_PROTOCOL_VERSION        ((uint32_t)146)
#define VID_ZONE_UIN                ((uint32_t)147)
#define VID_ZONING_ENABLED          ((uint32_t)148)
#define VID_ICMP_PROXY              ((uint32_t)149)
#define VID_IP_ADDRESS_COUNT        ((uint32_t)150)
#define VID_ENABLED                 ((uint32_t)151)
#define VID_REMOVE_DCI              ((uint32_t)152)
#define VID_TEMPLATE_ID             ((uint32_t)153)
#define VID_PUBLIC_KEY              ((uint32_t)154)
#define VID_SESSION_KEY             ((uint32_t)155)
#define VID_CIPHER                  ((uint32_t)156)
#define VID_KEY_LENGTH              ((uint32_t)157)
#define VID_SESSION_IV              ((uint32_t)158)
#define VID_CONFIG_FILE             ((uint32_t)159)
#define VID_STATUS_CALCULATION_ALG  ((uint32_t)160)
#define VID_NUM_LOCAL_NETS          ((uint32_t)161)
#define VID_NUM_REMOTE_NETS         ((uint32_t)162)
#define VID_APPLY_FLAG              ((uint32_t)163)
#define VID_NUM_TOOLS               ((uint32_t)164)
#define VID_TOOL_ID                 ((uint32_t)165)
#define VID_NUM_COLUMNS             ((uint32_t)166)
#define VID_NUM_ROWS                ((uint32_t)167)
#define VID_TABLE_TITLE             ((uint32_t)168)
#define VID_EVENT_NAME              ((uint32_t)169)
#define VID_CLIENT_TYPE             ((uint32_t)170)
#define VID_LOG_NAME                ((uint32_t)171)
#define VID_OPERATION               ((uint32_t)172)
#define VID_MAX_RECORDS             ((uint32_t)173)
#define VID_NUM_RECORDS             ((uint32_t)174)
#define VID_CLIENT_INFO             ((uint32_t)175)
#define VID_OS_INFO                 ((uint32_t)176)
#define VID_LIBNXCL_VERSION         ((uint32_t)177)
#define VID_VERSION                 ((uint32_t)178)
#define VID_NUM_NODES               ((uint32_t)179)
#define VID_LOG_FILE                ((uint32_t)180)
#define VID_HOP_COUNT               ((uint32_t)181)
#define VID_NUM_SCHEDULES           ((uint32_t)182)
#define VID_STATUS_PROPAGATION_ALG  ((uint32_t)183)
#define VID_FIXED_STATUS            ((uint32_t)184)
#define VID_STATUS_SHIFT            ((uint32_t)185)
#define VID_STATUS_TRANSLATION_1    ((uint32_t)186)
#define VID_STATUS_TRANSLATION_2    ((uint32_t)187)
#define VID_STATUS_TRANSLATION_3    ((uint32_t)188)
#define VID_STATUS_TRANSLATION_4    ((uint32_t)189)
#define VID_STATUS_SINGLE_THRESHOLD ((uint32_t)190)
#define VID_STATUS_THRESHOLD_1      ((uint32_t)191)
#define VID_STATUS_THRESHOLD_2      ((uint32_t)192)
#define VID_STATUS_THRESHOLD_3      ((uint32_t)193)
#define VID_STATUS_THRESHOLD_4      ((uint32_t)194)
#define VID_AGENT_PROXY             ((uint32_t)195)
#define VID_TOOL_TYPE               ((uint32_t)196)
#define VID_TOOL_DATA               ((uint32_t)197)
#define VID_ACL                     ((uint32_t)198)
#define VID_TOOL_FILTER             ((uint32_t)199)
#define VID_SERVER_UPTIME           ((uint32_t)200)
#define VID_NUM_ALARMS              ((uint32_t)201)
#define VID_ALARMS_BY_SEVERITY      ((uint32_t)202)
#define VID_NETXMSD_PROCESS_WKSET   ((uint32_t)203)
#define VID_NETXMSD_PROCESS_VMSIZE  ((uint32_t)204)
#define VID_NUM_SESSIONS            ((uint32_t)205)
#define VID_NUM_SCRIPTS             ((uint32_t)206)
#define VID_SCRIPT_ID               ((uint32_t)207)
#define VID_SCRIPT_CODE             ((uint32_t)208)
#define VID_SESSION_ID              ((uint32_t)209)
#define VID_RECORDS_ORDER           ((uint32_t)210)
#define VID_NUM_SUBMAPS             ((uint32_t)211)
#define VID_SUBMAP_LIST             ((uint32_t)212)
#define VID_SUBMAP_ATTR             ((uint32_t)213)
#define VID_NUM_LINKS               ((uint32_t)214)
#define VID_LINK_LIST               ((uint32_t)215)
#define VID_MAP_ID                  ((uint32_t)216)
#define VID_NUM_MAPS                ((uint32_t)217)
#define VID_NUM_MODULES             ((uint32_t)218)
#define VID_DST_USER_ID             ((uint32_t)219)
#define VID_MOVE_FLAG               ((uint32_t)220)
#define VID_CHANGE_PASSWD_FLAG      ((uint32_t)221)
#define VID_GUID                    ((uint32_t)222)
#define VID_ACTIVATION_EVENT        ((uint32_t)223)
#define VID_DEACTIVATION_EVENT      ((uint32_t)224)
#define VID_SOURCE_OBJECT           ((uint32_t)225)
#define VID_ACTIVE_STATUS           ((uint32_t)226)
#define VID_INACTIVE_STATUS         ((uint32_t)227)
#define VID_SCRIPT                  ((uint32_t)228)
#define VID_NODE_LIST               ((uint32_t)229)
#define VID_DCI_LIST                ((uint32_t)230)
#define VID_CONFIG_ID               ((uint32_t)231)
#define VID_FILTER                  ((uint32_t)232)
#define VID_SEQUENCE_NUMBER         ((uint32_t)233)
#define VID_VERSION_MAJOR           ((uint32_t)234)
#define VID_VERSION_MINOR           ((uint32_t)235)
#define VID_VERSION_RELEASE         ((uint32_t)236)
#define VID_CONFIG_ID_2             ((uint32_t)237)
#define VID_IV_LENGTH               ((uint32_t)238)
#define VID_DBCONN_STATUS           ((uint32_t)239)
#define VID_CREATION_TIME           ((uint32_t)240)
#define VID_LAST_CHANGE_TIME        ((uint32_t)241)
#define VID_TERMINATED_BY_USER      ((uint32_t)242)
#define VID_STATE                   ((uint32_t)243)
#define VID_CURRENT_SEVERITY        ((uint32_t)244)
#define VID_ORIGINAL_SEVERITY       ((uint32_t)245)
#define VID_HELPDESK_STATE          ((uint32_t)246)
#define VID_HELPDESK_REF            ((uint32_t)247)
#define VID_REPEAT_COUNT            ((uint32_t)248)
#define VID_SNMP_RAW_VALUE_TYPE     ((uint32_t)249)
#define VID_CONFIRMATION_TEXT       ((uint32_t)250)
#define VID_FAILED_DCI_INDEX        ((uint32_t)251)
#define VID_ADDR_LIST_TYPE          ((uint32_t)252)
#define VID_COMPONENT_ID            ((uint32_t)253)
#define VID_TCP_PROXY_CLIENT_CID    ((uint32_t)254)
#define VID_EVENT_LIST              ((uint32_t)255)
#define VID_NUM_TRAPS               ((uint32_t)256)
#define VID_TRAP_LIST               ((uint32_t)257)
#define VID_NXMP_CONTENT            ((uint32_t)258)
#define VID_ERROR_TEXT              ((uint32_t)259)
#define VID_COMPONENT               ((uint32_t)260)
#define VID_CONSOLE_UPGRADE_URL		((uint32_t)261)
#define VID_CLUSTER_TYPE				((uint32_t)262)
#define VID_NUM_SYNC_SUBNETS			((uint32_t)263)
#define VID_SYNC_SUBNETS				((uint32_t)264)
#define VID_NUM_RESOURCES				((uint32_t)265)
#define VID_RESOURCE_ID					((uint32_t)266)
#define VID_SNMP_PROXY					((uint32_t)267)
#define VID_PORT							((uint32_t)268)
#define VID_PDU							((uint32_t)269)
#define VID_PDU_SIZE						((uint32_t)270)
#define VID_AI_AGENT_INSTRUCTIONS   ((uint32_t)271)
#define VID_GRAPH_CONFIG				((uint32_t)272)
#define VID_NUM_GRAPHS					((uint32_t)273)
#define VID_GRAPH_ID						((uint32_t)274)
#define VID_AUTH_TYPE					((uint32_t)275)
#define VID_CERTIFICATE					((uint32_t)276)
#define VID_SIGNATURE					((uint32_t)277)
#define VID_CHALLENGE					((uint32_t)278)
#define VID_CERT_MAPPING_METHOD		((uint32_t)279)
#define VID_CERT_MAPPING_DATA       ((uint32_t)280)
#define VID_CERTIFICATE_ID				((uint32_t)281)
#define VID_NUM_CERTIFICATES        ((uint32_t)282)
#define VID_ALARM_TIMEOUT_EVENT     ((uint32_t)283)
#define VID_NUM_GROUPS              ((uint32_t)284)
#define VID_QSIZE_CONDITION_POLLER  ((uint32_t)285)
#define VID_QSIZE_CONF_POLLER       ((uint32_t)286)
#define VID_QSIZE_DCI_POLLER        ((uint32_t)287)
#define VID_QSIZE_DBWRITER          ((uint32_t)288)
#define VID_QSIZE_EVENT             ((uint32_t)289)
#define VID_QSIZE_DISCOVERY         ((uint32_t)290)
#define VID_QSIZE_NODE_POLLER       ((uint32_t)291)
#define VID_QSIZE_ROUTE_POLLER      ((uint32_t)292)
#define VID_QSIZE_STATUS_POLLER     ((uint32_t)293)
#define VID_SYNTHETIC_MASK          ((uint32_t)294)
#define VID_SUBSYSTEM               ((uint32_t)295)
#define VID_SUCCESS_AUDIT           ((uint32_t)296)
#define VID_WORKSTATION             ((uint32_t)297)
#define VID_USER_TAG                ((uint32_t)298)
#define VID_REQUIRED_POLLS          ((uint32_t)299)
#define VID_SYS_DESCRIPTION         ((uint32_t)300)
#define VID_PSTORAGE_KEY            ((uint32_t)301)
#define VID_PSTORAGE_VALUE          ((uint32_t)302)
#define VID_NUM_DELETE_PSTORAGE     ((uint32_t)303)
#define VID_INSTANCE_COUNT          ((uint32_t)304)
#define VID_NUM_SET_PSTORAGE        ((uint32_t)305)
#define VID_SERVER_BUILD            ((uint32_t)306)
#define VID_TRUSTED_OBJECTS         ((uint32_t)307)
#define VID_TIMEZONE                ((uint32_t)308)
#define VID_NUM_CUSTOM_ATTRIBUTES   ((uint32_t)309)
#define VID_MAP_DATA                ((uint32_t)310)
#define VID_PRODUCT_ID              ((uint32_t)311)
#define VID_CLIENT_ID               ((uint32_t)312)
#define VID_LICENSE_DATA            ((uint32_t)313)
#define VID_TOKEN                   ((uint32_t)314)
#define VID_SERVICE_ID              ((uint32_t)315)
#define VID_TOKEN_SOFTLIMIT         ((uint32_t)316)
#define VID_TOKEN_HARDLIMIT         ((uint32_t)317)
#define VID_USE_IFXTABLE            ((uint32_t)318)
#define VID_USE_X509_KEY_FORMAT     ((uint32_t)319)
#define VID_STICKY_FLAG             ((uint32_t)320)
#define VID_AUTOBIND_FILTER         ((uint32_t)321)
#define VID_COMMIT_ONLY             ((uint32_t)322)
#define VID_MULTIPLIER              ((uint32_t)323)
#define VID_UNITS_NAME              ((uint32_t)324)
#define VID_PERFTAB_SETTINGS        ((uint32_t)325)
#define VID_EXECUTION_STATUS        ((uint32_t)326)
#define VID_EXECUTION_RESULT        ((uint32_t)327)
#define VID_TABLE_NUM_ROWS          ((uint32_t)328)
#define VID_TABLE_NUM_COLS          ((uint32_t)329)
#define VID_DOWNTIME_TAG            ((uint32_t)330)
#define VID_TASK_ID                 ((uint32_t)331)
#define VID_VNC_PASSWORD            ((uint32_t)332)
#define VID_VNC_PORT                ((uint32_t)333)
#define VID_VNC_PROXY               ((uint32_t)334)
#define VID_FAILURE_MESSAGE         ((uint32_t)335)
#define VID_POLICY_TYPE             ((uint32_t)336)
#define VID_FIELDS                  ((uint32_t)337)
#define VID_LOG_HANDLE              ((uint32_t)338)
#define VID_START_ROW               ((uint32_t)339)
#define VID_TABLE_OFFSET            ((uint32_t)340)
#define VID_NUM_FILTERS             ((uint32_t)341)
#define VID_GEOLOCATION_TYPE        ((uint32_t)342)
#define VID_LATITUDE                ((uint32_t)343)
#define VID_LONGITUDE               ((uint32_t)344)
#define VID_NUM_ORDERING_COLUMNS    ((uint32_t)345)
#define VID_SYSTEM_TAG              ((uint32_t)346)
#define VID_NUM_ENUMS               ((uint32_t)347)
#define VID_NUM_PUSH_PARAMETERS     ((uint32_t)348)
#define VID_OLD_PASSWORD            ((uint32_t)349)
#define VID_MIN_PASSWORD_LENGTH     ((uint32_t)350)
#define VID_LAST_LOGIN              ((uint32_t)351)
#define VID_LAST_PASSWORD_CHANGE    ((uint32_t)352)
#define VID_DISABLED_UNTIL          ((uint32_t)353)
#define VID_AUTH_FAILURES           ((uint32_t)354)
#define VID_RUNTIME_FLAGS           ((uint32_t)355)
#define VID_FILE_SIZE               ((uint32_t)356)
#define VID_MAP_TYPE                ((uint32_t)357)
#define VID_LAYOUT                  ((uint32_t)358)
#define VID_SEED_OBJECTS            ((uint32_t)359)
#define VID_BACKGROUND              ((uint32_t)360)
#define VID_NUM_ELEMENTS            ((uint32_t)361)
#define VID_INTERFACE_ID            ((uint32_t)362)
#define VID_LOCAL_INTERFACE_ID      ((uint32_t)363)
#define VID_LOCAL_NODE_ID           ((uint32_t)364)
#define VID_SYS_NAME                ((uint32_t)365)
#define VID_LLDP_NODE_ID            ((uint32_t)366)
#define VID_PHY_MODULE              ((uint32_t)367)
#define VID_PHY_PORT                ((uint32_t)368)
#define VID_IMAGE_DATA              ((uint32_t)369)
#define VID_IMAGE_PROTECTED         ((uint32_t)370)
#define VID_NUM_IMAGES              ((uint32_t)371)
#define VID_IMAGE_MIMETYPE          ((uint32_t)372)
#define VID_PEER_NODE_ID            ((uint32_t)373)
#define VID_PEER_INTERFACE_ID       ((uint32_t)374)
#define VID_VRRP_VERSION            ((uint32_t)375)
#define VID_VRRP_VR_COUNT           ((uint32_t)376)
#define VID_DESTINATION_FILE_NAME   ((uint32_t)377)
#define VID_NUM_TABLES              ((uint32_t)378)
#define VID_IMAGE                   ((uint32_t)379)
#define VID_DRIVER_NAME             ((uint32_t)380)
#define VID_DRIVER_VERSION          ((uint32_t)381)
#define VID_NUM_VLANS               ((uint32_t)382)
#define VID_UPDATE_FAILED           ((uint32_t)383)
#define VID_TILE_SERVER_URL         ((uint32_t)384)
#define VID_BACKGROUND_LATITUDE     ((uint32_t)385)
#define VID_BACKGROUND_LONGITUDE    ((uint32_t)386)
#define VID_BACKGROUND_ZOOM         ((uint32_t)387)
#define VID_BRIDGE_BASE_ADDRESS     ((uint32_t)388)
#define VID_DRILL_DOWN_OBJECT_ID    ((uint32_t)389)
#define VID_REPORT_DEFINITION       ((uint32_t)390)
#define VID_BIZSVC_CHECK_TYPE       ((uint32_t)391)
#define VID_REASON                  ((uint32_t)392)
#define VID_NODE_ID                 ((uint32_t)393)
#define VID_UPTIME_DAY              ((uint32_t)394)
#define VID_UPTIME_WEEK             ((uint32_t)395)
#define VID_UPTIME_MONTH            ((uint32_t)396)
#define VID_PRIMARY_NAME            ((uint32_t)397)
#define VID_NUM_RESULTS             ((uint32_t)398)
#define VID_RESULT_ID_LIST          ((uint32_t)399)
#define VID_RENDER_FORMAT           ((uint32_t)400)
#define VID_FILE_OFFSET             ((uint32_t)401)
#define VID_IS_TEMPLATE             ((uint32_t)402)
#define VID_DOT1X_PAE_STATE         ((uint32_t)403)
#define VID_DOT1X_BACKEND_STATE     ((uint32_t)404)
#define VID_IS_COMPLETE             ((uint32_t)405)
#define VID_MODIFICATION_TIME       ((uint32_t)406)
#define VID_IS_PHYS_PORT            ((uint32_t)407)
#define VID_CREATE_STATUS_DCI       ((uint32_t)408)
#define VID_NUM_COMMENTS            ((uint32_t)409)
#define VID_COMMENT_ID              ((uint32_t)410)
#define VID_DCOBJECT_TYPE           ((uint32_t)411)
#define VID_INSTANCE_COLUMN         ((uint32_t)412)
#define VID_DATA_COLUMN             ((uint32_t)413)
#define VID_ADMIN_STATE             ((uint32_t)414)
#define VID_OPER_STATE              ((uint32_t)415)
#define VID_EXPECTED_STATE          ((uint32_t)416)
#define VID_LINK_COLOR              ((uint32_t)417)
#define VID_CONNECTION_TYPE         ((uint32_t)418)
#define VID_RESOLVED_BY_USER        ((uint32_t)419)
#define VID_IS_STICKY               ((uint32_t)420)
#define VID_DATE_FORMAT             ((uint32_t)421)
#define VID_TIME_FORMAT             ((uint32_t)422)
#define VID_LINK_ROUTING            ((uint32_t)423)
#define VID_BACKGROUND_COLOR        ((uint32_t)424)
#define VID_FORCE_RELOAD            ((uint32_t)425)
#define VID_DISCOVERY_RADIUS        ((uint32_t)426)
#define VID_BATTERY_LEVEL           ((uint32_t)427)
#define VID_VENDOR                  ((uint32_t)428)
#define VID_MODEL                   ((uint32_t)429)
#define VID_OS_NAME                 ((uint32_t)430)
#define VID_OS_VERSION              ((uint32_t)431)
#define VID_SERIAL_NUMBER           ((uint32_t)432)
#define VID_DEVICE_ID               ((uint32_t)433)
#define VID_MAPPING_TABLE_ID        ((uint32_t)434)
#define VID_INSTD_METHOD            ((uint32_t)435)
#define VID_INSTD_DATA              ((uint32_t)436)
#define VID_INSTD_FILTER            ((uint32_t)437)
#define VID_ACCURACY                ((uint32_t)438)
#define VID_GEOLOCATION_TIMESTAMP   ((uint32_t)439)
#define VID_SAMPLE_COUNT            ((uint32_t)440)
#define VID_HEIGHT                  ((uint32_t)441)
#define VID_RADIO_COUNT             ((uint32_t)442)
#define VID_OBJECT_TOOLTIP_ONLY     ((uint32_t)443)
#define VID_SUMMARY_TABLE_ID        ((uint32_t)444)
#define VID_MENU_PATH               ((uint32_t)445)
#define VID_COLUMNS                 ((uint32_t)446)
#define VID_TITLE                   ((uint32_t)447)
#define VID_DAY_OF_WEEK             ((uint32_t)448)
#define VID_DAY_OF_MONTH            ((uint32_t)449)
#define VID_LOCALE                  ((uint32_t)450)
#define VID_READ_ONLY               ((uint32_t)451)
#define VID_CLIENT_ADDRESS          ((uint32_t)452)
#define VID_SHORT_TIME_FORMAT       ((uint32_t)453)
#define VID_BOOT_TIME               ((uint32_t)454)
#define VID_REQUEST_ID              ((uint32_t)455)
#define VID_ADDRESS_MAP             ((uint32_t)456)
#define VID_XMPP_ID                 ((uint32_t)457)
#define VID_FILE_SIZE_LIMIT         ((uint32_t)458)
#define VID_FILE_FOLLOW             ((uint32_t)459)
#define VID_FILE_DATA               ((uint32_t)460)
#define VID_ALARM_STATUS_FLOW_STATE ((uint32_t)461)
#define VID_GROUPS                  ((uint32_t)462)
#define VID_EFFECTIVE_RIGHTS        ((uint32_t)463)
#define VID_EXTENSION_COUNT         ((uint32_t)464)
#define VID_TIMED_ALARM_ACK_ENABLED ((uint32_t)465)
#define VID_TABLE_EXTENDED_FORMAT   ((uint32_t)466)
#define VID_RS_JOB_ID               ((uint32_t)467)
#define VID_RS_JOB_TYPE             ((uint32_t)468)
#define VID_RS_REPORT_NAME          ((uint32_t)469)
#define VID_HELPDESK_LINK_ACTIVE    ((uint32_t)470)
#define VID_URL                     ((uint32_t)471)
#define VID_PEER_PROTOCOL           ((uint32_t)472)
#define VID_VIEW_REFRESH_INTERVAL   ((uint32_t)473)
#define VID_COMMAND_NAME            ((uint32_t)474)
#define VID_COMMAND_SHORT_NAME      ((uint32_t)475)
#define VID_MODULE_DATA_COUNT       ((uint32_t)476)
#define VID_NEW_FILE_NAME           ((uint32_t)477)
#define VID_ALARM_LIST_DISP_LIMIT   ((uint32_t)478)
#define VID_LANGUAGE                ((uint32_t)479)
#define VID_ROOT                    ((uint32_t)480)
#define VID_INCLUDE_NOVALUE_OBJECTS ((uint32_t)481)
#define VID_RECEIVE_OUTPUT          ((uint32_t)482)
#define VID_SESSION_STATE           ((uint32_t)483)
#define VID_PAGE_SIZE               ((uint32_t)484)
#define VID_EXECUTION_END_FLAG      ((uint32_t)485)
#define VID_COUNTRY                 ((uint32_t)486)
#define VID_CITY                    ((uint32_t)487)
#define VID_STREET_ADDRESS          ((uint32_t)488)
#define VID_POSTCODE                ((uint32_t)489)
#define VID_FUNCTION                ((uint32_t)490)
#define VID_RESPONSE_TIME           ((uint32_t)491)
#define VID_QSIZE_DCI_CACHE_LOADER  ((uint32_t)492)
#define VID_MTU                     ((uint32_t)493)
#define VID_ALIAS                   ((uint32_t)494)
#define VID_AP_INDEX                ((uint32_t)495)
#define VID_PROTOCOL_VERSION_EX     ((uint32_t)496)
#define VID_SCRIPT_LIST             ((uint32_t)497)
#define VID_TOOL_LIST               ((uint32_t)498)
#define VID_NUM_SUMMARY_TABLES      ((uint32_t)499)
#define VID_SUMMARY_TABLE_LIST      ((uint32_t)500)
#define VID_OVERVIEW_DCI_COUNT      ((uint32_t)501)
#define VID_OVERVIEW_ONLY           ((uint32_t)502)
#define VID_AGENT_CACHE_MODE        ((uint32_t)503)
#define VID_DATE                    ((uint32_t)504)
#define VID_RECONCILIATION          ((uint32_t)505)
#define VID_DISPLAY_MODE            ((uint32_t)506)
#define VID_NUM_FIELDS              ((uint32_t)507)
#define VID_PASSWORD_IS_VALID       ((uint32_t)508)
#define VID_SERIALIZE               ((uint32_t)509)
#define VID_COMPILATION_STATUS      ((uint32_t)510)
#define VID_ERROR_LINE              ((uint32_t)511)
#define VID_SPEED                   ((uint32_t)512)
#define VID_IFTABLE_SUFFIX          ((uint32_t)513)
#define VID_SERVER_COMMAND_TIMEOUT  ((uint32_t)514)
#define VID_SYS_CONTACT             ((uint32_t)515)
#define VID_SYS_LOCATION            ((uint32_t)516)
#define VID_PHYSICAL_CONTAINER_ID   ((uint32_t)517)
#define VID_RACK_IMAGE_FRONT        ((uint32_t)518)
#define VID_RACK_POSITION           ((uint32_t)519)
#define VID_RACK_HEIGHT             ((uint32_t)520)
#define VID_SCHEDULE_COUNT          ((uint32_t)521)
#define VID_SCHEDULED_TASK_ID       ((uint32_t)522)
#define VID_TASK_HANDLER            ((uint32_t)523)
#define VID_SCHEDULE                ((uint32_t)524)
#define VID_EXECUTION_TIME          ((uint32_t)525)
#define VID_LAST_EXECUTION_TIME     ((uint32_t)526)
#define VID_CALLBACK_COUNT          ((uint32_t)527)
#define VID_DASHBOARDS              ((uint32_t)528)
#define VID_OWNER                   ((uint32_t)529)
#define VID_MAINTENANCE_MODE        ((uint32_t)530)
#define VID_IS_MASTER               ((uint32_t)531)
#define VID_AGENT_COMM_TIME         ((uint32_t)532)
#define VID_GRAPH_TEMPALTE          ((uint32_t)533)
#define VID_OVERWRITE               ((uint32_t)534)
#define VID_IPV6_SUPPORT            ((uint32_t)535)
#define VID_BULK_RECONCILIATION     ((uint32_t)536)
#define VID_STATUS                  ((uint32_t)537)
#define VID_FLAGS_MASK              ((uint32_t)538)
#define VID_TOP_BOTTOM              ((uint32_t)539)
#define VID_AUTH_TOKEN              ((uint32_t)540)
#define VID_REPOSITORY_ID           ((uint32_t)541)
#define VID_TOOLTIP_DCI_COUNT       ((uint32_t)542)
#define VID_CONTROLLER_ID           ((uint32_t)543)
#define VID_CHASSIS_ID              ((uint32_t)544)
#define VID_NODE_SUBTYPE            ((uint32_t)545)
#define VID_SSH_LOGIN               ((uint32_t)546)
#define VID_SSH_PASSWORD            ((uint32_t)547)
#define VID_SSH_PROXY               ((uint32_t)548)
#define VID_ZONE_PROXY_COUNT        ((uint32_t)549)
#define VID_MESSAGE_LENGTH          ((uint32_t)550)
#define VID_LDAP_DN                 ((uint32_t)551)
#define VID_LDAP_ID                 ((uint32_t)552)
#define VID_FAIL_CODE_LIST          ((uint32_t)553)
#define VID_FOLDER_SIZE             ((uint32_t)554)
#define VID_ALARM_CATEGORY_ID       ((uint32_t)555)
#define VID_FILE_COUNT              ((uint32_t)556)
#define VID_ALARM_CATEGORY_ACL      ((uint32_t)557)
#define VID_ALLOW_MULTIPART         ((uint32_t)558)
#define VID_ALARM_ID_LIST           ((uint32_t)559)
#define VID_NUM_COMPONENTS          ((uint32_t)560)
#define VID_SERVER_NAME             ((uint32_t)561)
#define VID_SERVER_COLOR            ((uint32_t)562)
#define VID_MESSAGE_OF_THE_DAY      ((uint32_t)563)
#define VID_PORT_ROW_COUNT          ((uint32_t)564)
#define VID_PORT_NUMBERING_SCHEME   ((uint32_t)565)
#define VID_NUM_VALUES              ((uint32_t)566)
#define VID_NUM_PSTORAGE            ((uint32_t)567)
#define VID_COMMAND_ID              ((uint32_t)568)
#define VID_HOSTNAME                ((uint32_t)569)
#define VID_ENABLE_COMPRESSION      ((uint32_t)570)
#define VID_AGENT_COMPRESSION_MODE  ((uint32_t)571)
#define VID_TRAP_TYPE               ((uint32_t)572)
#define VID_IS_ACTIVE               ((uint32_t)573)
#define VID_CHANNEL_ID              ((uint32_t)574)
#define VID_NUM_URLS                ((uint32_t)575)
#define VID_GRACE_LOGINS            ((uint32_t)576)
#define VID_TUNNEL_GUID             ((uint32_t)577)
#define VID_ORGANIZATION            ((uint32_t)578)
#define VID_TUNNEL_ID               ((uint32_t)579)
#define VID_PARENT_INTERFACE        ((uint32_t)580)
#define VID_SENSOR_FLAGS            ((uint32_t)581)
#define VID_DEVICE_CLASS            ((uint32_t)582)
#define VID_COMM_PROTOCOL           ((uint32_t)583)
#define VID_XML_CONFIG              ((uint32_t)584)
#define VID_DEVICE_ADDRESS          ((uint32_t)585)
#define VID_LINK_WIDTH              ((uint32_t)586)
#define VID_LINK_STYLE              ((uint32_t)587)
#define VID_FRAME_COUNT             ((uint32_t)588)
#define VID_SIGNAL_STRENGHT         ((uint32_t)589)
#define VID_SIGNAL_NOISE            ((uint32_t)590)
#define VID_FREQUENCY               ((uint32_t)591)
#define VID_GATEWAY_NODE            ((uint32_t)592)
#define VID_INBOUND_UTILIZATION     ((uint32_t)593)
#define VID_OUTBOUND_UTILIZATION    ((uint32_t)594)
#define VID_PARTIAL_OBJECT          ((uint32_t)595)
#define VID_PERSISTENT              ((uint32_t)596)
#define VID_VALIDITY_TIME           ((uint32_t)597)
#define VID_TOKEN_ID                ((uint32_t)598)
#define VID_DOMAIN_ID               ((uint32_t)599)
#define VID_DCI_NAME                ((uint32_t)600)
#define VID_STATE_FLAGS             ((uint32_t)601)
#define VID_CAPABILITIES            ((uint32_t)602)
#define VID_INPUT_FIELD_COUNT       ((uint32_t)603)
#define VID_STRING_COUNT            ((uint32_t)604)
#define VID_EXPAND_STRING           ((uint32_t)605)
#define VID_ACTION_LIST             ((uint32_t)606)
#define VID_ZONE_PORT_COUNT         ((uint32_t)607)
#define VID_HISTORICAL_DATA_TYPE    ((uint32_t)608)
#define VID_JOB_CANCELLED           ((uint32_t)609)
#define VID_INSTANCE_RETENTION      ((uint32_t)610)
#define VID_RACK_ORIENTATION        ((uint32_t)611)
#define VID_PASSIVE_ELEMENTS        ((uint32_t)612)
#define VID_RACK_IMAGE_REAR         ((uint32_t)613)
#define VID_INCLUDE_THRESHOLDS      ((uint32_t)614)
#define VID_RESPONSIBLE_USERS_COUNT ((uint32_t)615)
#define VID_AGENT_ID                ((uint32_t)616)
#define VID_QUERY                   ((uint32_t)617)
#define VID_CONFIG_HINT_COUNT       ((uint32_t)618)
#define VID_DESTINATION_ADDRESS     ((uint32_t)619)
#define VID_HYPERVISOR_TYPE         ((uint32_t)620)
#define VID_HYPERVISOR_INFO         ((uint32_t)621)
#define VID_VLAN_LIST               ((uint32_t)622)
#define VID_TASK_KEY                ((uint32_t)623)
#define VID_TIMER_COUNT             ((uint32_t)624)
#define VID_AUTOBIND_FLAGS          ((uint32_t)625)
#define VID_AUTOBIND_FILTER_2       ((uint32_t)626)
#define VID_TIMEOUT                 ((uint32_t)627)
#define VID_PROGRESS                ((uint32_t)628)
#define VID_POLICY_COUNT            ((uint32_t)629)
#define VID_NEW_POLICY_TYPE         ((uint32_t)630)
#define VID_USERAGENT               ((uint32_t)631)
#define VID_ORDER_FIELDS            ((uint32_t)632)
#define VID_RECORD_LIMIT            ((uint32_t)633)
#define VID_LOCAL_CACHE             ((uint32_t)634)
#define VID_RESTART                 ((uint32_t)635)
#define VID_THIS_PROXY_ID           ((uint32_t)636)
#define VID_ZONE_PROXY_LIST         ((uint32_t)637)
#define VID_PRIMARY_ZONE_PROXY_ID   ((uint32_t)638)
#define VID_BACKUP_ZONE_PROXY_ID    ((uint32_t)639)
#define VID_USERAGENT_INSTALLED     ((uint32_t)640)
#define VID_CATEGORY_LIST           ((uint32_t)641)
#define VID_UA_NOTIFICATION_COUNT   ((uint32_t)642)
#define VID_SNMP_TRAP_PROXY         ((uint32_t)643)
#define VID_THRESHOLD_ID            ((uint32_t)644)
#define VID_TAGS                    ((uint32_t)645)
#define VID_ICMP_AVG_RESPONSE_TIME  ((uint32_t)646)
#define VID_ICMP_MIN_RESPONSE_TIME  ((uint32_t)647)
#define VID_ICMP_MAX_RESPONSE_TIME  ((uint32_t)648)
#define VID_ICMP_LAST_RESPONSE_TIME ((uint32_t)649)
#define VID_ICMP_PACKET_LOSS        ((uint32_t)650)
#define VID_ICMP_COLLECTION_MODE    ((uint32_t)651)
#define VID_ICMP_TARGET_COUNT       ((uint32_t)652)
#define VID_HAS_ICMP_DATA           ((uint32_t)653)
#define VID_CHANNEL_NAME            ((uint32_t)654)
#define VID_CHANNEL_COUNT           ((uint32_t)655)
#define VID_DRIVER_COUNT            ((uint32_t)656)
#define VID_NEW_NAME                ((uint32_t)657)
#define VID_RELATED_OBJECT          ((uint32_t)658)
#define VID_PHY_CHASSIS             ((uint32_t)659)
#define VID_PHY_PIC                 ((uint32_t)660)
#define VID_AGENT_BUILD_TAG         ((uint32_t)661)
#define VID_FILE_STORE              ((uint32_t)662)
#define VID_LINK_COUNT              ((uint32_t)663)
#define VID_PATCH_PANNEL_ID         ((uint32_t)664)
#define VID_PHYSICAL_LINK_ID        ((uint32_t)665)
#define VID_POLLING_SCHEDULE_TYPE   ((uint32_t)666)
#define VID_RETENTION_TYPE          ((uint32_t)667)
#define VID_IS_REFRESH              ((uint32_t)668)
#define VID_PARENT_ALARM_ID         ((uint32_t)669)
#define VID_RCA_SCRIPT_NAME         ((uint32_t)670)
#define VID_SUBORDINATE_ALARMS      ((uint32_t)671)
#define VID_IMPACT                  ((uint32_t)672)
#define VID_CHASSIS_PLACEMENT       ((uint32_t)673)
#define VID_ALLOW_PATH_EXPANSION    ((uint32_t)674)
#define VID_TAG                     ((uint32_t)675)
#define VID_NUM_HEADERS             ((uint32_t)676)
#define VID_REQUEST_TYPE            ((uint32_t)677)
#define VID_VERIFY_CERT             ((uint32_t)678)
#define VID_SYNC_NODE_COMPONENTS    ((uint32_t)679)
#define VID_WEBSVC_ID               ((uint32_t)680)
#define VID_PRODUCT_CODE            ((uint32_t)681)
#define VID_PRODUCT_NAME            ((uint32_t)682)
#define VID_PRODUCT_VERSION         ((uint32_t)683)
#define VID_CIP_DEVICE_TYPE         ((uint32_t)684)
#define VID_CIP_STATUS              ((uint32_t)685)
#define VID_CIP_STATE               ((uint32_t)686)
#define VID_ETHERNET_IP_PROXY       ((uint32_t)687)
#define VID_ETHERNET_IP_PORT        ((uint32_t)688)
#define VID_CIP_DEVICE_TYPE_NAME    ((uint32_t)689)
#define VID_CIP_STATUS_TEXT         ((uint32_t)690)
#define VID_CIP_EXT_STATUS_TEXT     ((uint32_t)691)
#define VID_CIP_STATE_TEXT          ((uint32_t)692)
#define VID_DUPLICATE               ((uint32_t)693)
#define VID_TASK_IS_DISABLED        ((uint32_t)694)
#define VID_PROCESS_ID              ((uint32_t)695)
#define VID_SCREEN_WIDTH            ((uint32_t)696)
#define VID_SCREEN_HEIGHT           ((uint32_t)697)
#define VID_SCREEN_BPP              ((uint32_t)698)
#define VID_EXTENDED_DCI_DATA       ((uint32_t)699)
#define VID_HARDWARE_ID             ((uint32_t)700)
#define VID_VERIFY_HOST             ((uint32_t)701)
#define VID_WEB_SERVICE_DEF_COUNT   ((uint32_t)702)
#define VID_MAINTENANCE_INITIATOR   ((uint32_t)703)
#define VID_RECORD_ID               ((uint32_t)704)
#define VID_RECORD_ID_COLUMN        ((uint32_t)705)
#define VID_OBJECT_ID_COLUMN        ((uint32_t)706)
#define VID_RAW_VALUE               ((uint32_t)707)
#define VID_LICENSE_PROBLEM_COUNT   ((uint32_t)708)
#define VID_FORCE_PLAIN_TEXT_PARSER ((uint32_t)709)
#define VID_SYSLOG_PROXY            ((uint32_t)710)
#define VID_CIP_VENDOR_CODE         ((uint32_t)711)
#define VID_NAME_ON_MAP             ((uint32_t)712)
#define VID_EXTPROV_CERTIFICATE     ((uint32_t)713)
#define VID_AGENT_CERT_SUBJECT      ((uint32_t)714)
#define VID_EVENT_SOURCE            ((uint32_t)715)
#define VID_RAW_DATA                ((uint32_t)716)
#define VID_NUM_MASKED_FIELDS       ((uint32_t)717)
#define VID_DIRECTION               ((uint32_t)718)
#define VID_ALTITUDE                ((uint32_t)719)
#define VID_FORCE_DELETE            ((uint32_t)720)
#define VID_ICON                    ((uint32_t)721)
#define VID_WEB_SERVICE_DEF_LIST    ((uint32_t)722)
#define VID_AREA_ID                 ((uint32_t)723)
#define VID_GEOLOCATION_CTRL_MODE   ((uint32_t)724)
#define VID_GEO_AREAS               ((uint32_t)725)
#define VID_ORIGIN_TIMESTAMP        ((uint32_t)726)
#define VID_SSH_PORT                ((uint32_t)727)
#define VID_EMAIL                   ((uint32_t)728)
#define VID_PHONE_NUMBER            ((uint32_t)729)
#define VID_EXECUTION_PARAMETERS    ((uint32_t)730)
#define VID_VIEW_NAME               ((uint32_t)731)
#define VID_DB_DRIVER               ((uint32_t)732)
#define VID_DB_SERVER               ((uint32_t)733)
#define VID_DB_NAME                 ((uint32_t)734)
#define VID_DB_LOGIN                ((uint32_t)735)
#define VID_DB_PASSWORD             ((uint32_t)736)
#define VID_SMTP_SERVER             ((uint32_t)737)
#define VID_SMTP_PORT               ((uint32_t)738)
#define VID_SMTP_FROM_NAME          ((uint32_t)739)
#define VID_SMTP_FROM_ADDRESS       ((uint32_t)740)
#define VID_SMTP_LOCAL_HOSTNAME     ((uint32_t)741)
#define VID_SMTP_LOGIN              ((uint32_t)742)
#define VID_SMTP_PASSWORD           ((uint32_t)743)
#define VID_PRIVATE_KEY             ((uint32_t)744)
#define VID_INCLUDE_PUBLIC_KEY      ((uint32_t)745)
#define VID_SSH_KEY_COUNT           ((uint32_t)746)
#define VID_SSH_KEY_ID              ((uint32_t)747)
#define VID_DATA_DIRECTORY          ((uint32_t)748)
#define VID_RULE_DESCRIPTION        ((uint32_t)749)
#define VID_READ_ALL_FIELDS         ((uint32_t)750)
#define VID_2FA_METHOD_COUNT        ((uint32_t)751)
#define VID_2FA_METHOD              ((uint32_t)752)
#define VID_2FA_RESPONSE            ((uint32_t)753)
#define VID_REQUIRES_DATA_VIEW      ((uint32_t)754)
#define VID_QUERY_ID                ((uint32_t)755)
#define VID_FILE_PERMISSIONS        ((uint32_t)756)
#define VID_GROUP_NAME              ((uint32_t)757)
#define VID_ERROR_INDICATOR         ((uint32_t)758)
#define VID_HASH_CRC32              ((uint32_t)759)
#define VID_HASH_MD5                ((uint32_t)760)
#define VID_HASH_SHA256             ((uint32_t)761)
#define VID_HTTP_REQUEST_METHOD     ((uint32_t)762)
#define VID_WEBSVC_RESPONSE_CODE    ((uint32_t)763)
#define VID_WEBSVC_RESPONSE         ((uint32_t)764)
#define VID_WEBSVC_ERROR_TEXT       ((uint32_t)765)
#define VID_REQUEST_DATA            ((uint32_t)766)
#define VID_OSPF_ROUTER_ID          ((uint32_t)767)
#define VID_CHECK_COUNT             ((uint32_t)768)
#define VID_CHECK_ID                ((uint32_t)769)
#define VID_PROTOTYPE_ID            ((uint32_t)770)
#define VID_RELATED_DCI             ((uint32_t)771)
#define VID_THRESHOLD               ((uint32_t)772)
#define VID_OBJECT_STATUS_THRESHOLD ((uint32_t)773)
#define VID_DCI_STATUS_THRESHOLD    ((uint32_t)774)
#define VID_BUSINESS_SERVICE_UPTIME ((uint32_t)775)
#define VID_TICKET_COUNT            ((uint32_t)776)
#define VID_COMMENTS_SOURCE         ((uint32_t)777)
#define VID_WEB_SERVICE_PROXY       ((uint32_t)778)
#define VID_MATCH                   ((uint32_t)779)
#define VID_SYSLOG_CODEPAGE         ((uint32_t)780)
#define VID_SNMP_CODEPAGE           ((uint32_t)781)
#define VID_REGION                  ((uint32_t)782)
#define VID_DISTRICT                ((uint32_t)783)
#define VID_PACKAGE_TYPE            ((uint32_t)784)
#define VID_REPORT_PROGRESS         ((uint32_t)785)
#define VID_ACCEPT_KEEPALIVE        ((uint32_t)786)
#define VID_QR_LABEL                ((uint32_t)787)
#define VID_RESULT_AS_MAP           ((uint32_t)788)
#define VID_HAS_DETAIL_FIELDS       ((uint32_t)789)
#define VID_IF_ALIAS                ((uint32_t)790)
#define VID_RESPONSIBLE_USER_TAGS   ((uint32_t)791)
#define VID_RESUME_MODE             ((uint32_t)792)
#define VID_MAC_ADDR_COUNT          ((uint32_t)793)
#define VID_DEVELOPMENT_MODE        ((uint32_t)794)
#define VID_DISPLAY_PRIORITY        ((uint32_t)795)
#define VID_OSPF_AREA               ((uint32_t)796)
#define VID_OSPF_INTERFACE_TYPE     ((uint32_t)797)
#define VID_OSPF_INTERFACE_STATE    ((uint32_t)798)
#define VID_OSPF_AREA_COUNT         ((uint32_t)799)
#define VID_OSPF_NEIGHBOR_COUNT     ((uint32_t)800)
#define VID_COMPRESSION_METHOD      ((uint32_t)801)
#define VID_MQTT_PROXY              ((uint32_t)802)
#define VID_TCP_PROXY               ((uint32_t)803)
#define VID_FOLLOW_LOCATION         ((uint32_t)804)
#define VID_ACTION_SCRIPT           ((uint32_t)805)
#define VID_CUSTOM_ATTR_SET_COUNT   ((uint32_t)806)
#define VID_CUSTOM_ATTR_DEL_COUNT   ((uint32_t)807)
#define VID_RULE_SOURCE_EXCLUSIONS  ((uint32_t)808)
#define VID_NETWORK_SERVICE_COUNT   ((uint32_t)809)
#define VID_VPN_CONNECTOR_COUNT     ((uint32_t)810)
#define VID_MONITOR_ID              ((uint32_t)811)
#define VID_NUM_TIME_FRAMES         ((uint32_t)812)
#define VID_NUM_ASSET_PROPERTIES    ((uint32_t)813)
#define VID_DATA_TYPE               ((uint32_t)814)
#define VID_IS_MANDATORY            ((uint32_t)815)
#define VID_IS_UNIQUE               ((uint32_t)816)
#define VID_RANGE_MIN               ((uint32_t)817)
#define VID_RANGE_MAX               ((uint32_t)818)
#define VID_SYSTEM_TYPE             ((uint32_t)819)
#define VID_ENUM_COUNT              ((uint32_t)820)
#define VID_DISPLAY_NAME            ((uint32_t)821)
#define VID_NUM_POLL_STATES         ((uint32_t)822)
#define VID_TRUSTED_DEVICE_TOKEN    ((uint32_t)823)
#define VID_TRUSTED_DEVICES_ALLOWED ((uint32_t)824)
#define VID_LINKED_OBJECT           ((uint32_t)825)
#define VID_NUM_ASSET_ATTRIBUTES    ((uint32_t)826)
#define VID_ASSET_ID                ((uint32_t)827)
#define VID_ASSET_ATTRIBUTE_NAMES   ((uint32_t)828)
#define VID_TIMER_LIST              ((uint32_t)829)
#define VID_UPDATE_IDENTIFICATION   ((uint32_t)830)
#define VID_TEMPLATE_ITEM_ID        ((uint32_t)831)
#define VID_IS_HIDDEN               ((uint32_t)832)
#define VID_LAST_UPDATE_TIMESTAMP   ((uint32_t)833)
#define VID_LAST_UPDATE_UID         ((uint32_t)834)
#define VID_MODBUS_PROXY            ((uint32_t)835)
#define VID_MODBUS_TCP_PORT         ((uint32_t)836)
#define VID_MODBUS_UNIT_ID          ((uint32_t)837)
#define VID_STP_PORT_STATE          ((uint32_t)838)
#define VID_NUM_WARNINGS            ((uint32_t)839)
#define VID_LINK_STYLING_SCRIPT     ((uint32_t)840)
#define VID_OBJ_MAINT_PREDEF_TIMES  ((uint32_t)841)
#define VID_NETMAP_DEFAULT_WIDTH    ((uint32_t)842)
#define VID_NETMAP_DEFAULT_HEIGHT   ((uint32_t)843)
#define VID_WIDTH                   ((uint32_t)844)
#define VID_ENABLE_TWO_PHASE_SETUP  ((uint32_t)845)
#define VID_CONTEXT_OBJECT_ID       ((uint32_t)846)
#define VID_UI_ACCESS_RULES         ((uint32_t)847)
#define VID_DELEGATE_OBJECT_ID      ((uint32_t)848)
#define VID_DASHBOARD_ID            ((uint32_t)849)
#define VID_ELEMENT_INDEX           ((uint32_t)850)
#define VID_USE_L1_TOPOLOGY         ((uint32_t)851)
#define VID_USE_CARBONE_RENDERER    ((uint32_t)852)
#define VID_TRANSFORMED_DATA_TYPE   ((uint32_t)853)
#define VID_PATH_CHECK_REASON       ((uint32_t)854)
#define VID_PATH_CHECK_NODE_ID      ((uint32_t)855)
#define VID_PATH_CHECK_INTERFACE_ID ((uint32_t)856)
#define VID_TIME_SYNC_ALLOWED       ((uint32_t)857)
#define VID_JDBC_OPTIONS            ((uint32_t)858)
#define VID_LAST_BACKUP_JOB_STATUS  ((uint32_t)859)
#define VID_JOB_ID                  ((uint32_t)860)
#define VID_EXPECTED_CAPABILITIES   ((uint32_t)861)
#define VID_AI_ASSISTANT_AVAILABLE  ((uint32_t)862)
#define VID_DCI_IDS                 ((uint32_t)863)
#define VID_SMTP_TLS_MODE           ((uint32_t)864)
#define VID_PEER_LAST_UPDATED       ((uint32_t)865)
#define VID_THRESHOLD_ENABLE_TIME   ((uint32_t)866)
#define VID_FORCED_CONTEXT_OBJECT   ((uint32_t)867)
#define VID_DASHBOARD_NAME_TEMPLATE ((uint32_t)868)
#define VID_USE_MULTIPLIER          ((uint32_t)869)
#define VID_ARGUMENTS               ((uint32_t)870)
#define VID_METADATA_SIZE           ((uint32_t)871)
#define VID_MAX_SPEED               ((uint32_t)872)
#define VID_PROMPT                  ((uint32_t)873)
#define VID_TIMESTAMP_MS            ((uint32_t)874)
#define VID_CHAT_ID                 ((uint32_t)875)

// Base variabe for single threshold in message
#define VID_THRESHOLD_BASE          ((uint32_t)0x00800000)

// Map elements list base
#define VID_ELEMENT_LIST_BASE       ((uint32_t)0x10000000)
#define VID_LINK_LIST_BASE          ((uint32_t)0x40000000)

// Node info list base
#define VID_NODE_INFO_LIST_BASE     ((uint32_t)0x60000000)

// Node info list base
#define VID_EXTRA_DCI_INFO_BASE     ((uint32_t)0x30000000)

// Variable ranges for object's ACL
#define VID_ACL_USER_BASE           ((uint32_t)0x00001000)
#define VID_ACL_USER_LAST           ((uint32_t)0x00001FFF)
#define VID_ACL_RIGHTS_BASE         ((uint32_t)0x00002000)
#define VID_ACL_RIGHTS_LAST         ((uint32_t)0x00002FFF)

// Variable range for user group members
#define VID_GROUP_MEMBER_BASE       ((uint32_t)0x00004000)
#define VID_GROUP_MEMBER_LAST       ((uint32_t)0x00004FFF)

// Variable range for data collection object attributes
#define VID_DCI_COLUMN_BASE         ((uint32_t)0x30000000)
#define VID_DCI_THRESHOLD_BASE      ((uint32_t)0x20000000)
#define VID_DCI_SCHEDULE_BASE       ((uint32_t)0x10000000)

// Variable range for event argument list
#define VID_EVENT_ARG_BASE          ((uint32_t)0x00008000)
#define VID_EVENT_ARG_LAST          ((uint32_t)0x00008FFF)
#define VID_EVENT_ARG_NAMES_BASE    ((uint32_t)0x10000000)

// Variable range for trap parameter list
#define VID_TRAP_PBASE              ((uint32_t)0x00009000)

// Base value for poll state list
#define VID_POLL_STATE_LIST_BASE    ((uint32_t)0x0000A000)

// Object information can contain variable number of parent and child objects' ids.
// Because each variable in message have to have unique identifier,
// we reserver a two range ids for this variables.
#define VID_PARENT_ID_BASE          ((uint32_t)0x00003000)
#define VID_PARENT_ID_LAST          ((uint32_t)0x00003FFF)

// Reservation of 0x7FFFFFFF ids for child object's list
#define VID_CHILD_ID_BASE           ((uint32_t)0x80000000)
#define VID_CHILD_ID_LAST           ((uint32_t)0xFFFFFFFE)

// Base value for responsible users
#define VID_RESPONSIBLE_USERS_BASE  ((uint32_t)0x69000000)

// Base value for custom attributes and module data
#define VID_CUSTOM_ATTRIBUTES_BASE  ((uint32_t)0x70000000)
#define VID_MODULE_DATA_BASE        ((uint32_t)0x71000000)

// Base value for overview DCI list
#define VID_OVERVIEW_DCI_LIST_BASE  ((uint32_t)0x72000000)

// Base value for tooltip DCI list
#define VID_TOOLTIP_DCI_LIST_BASE   ((uint32_t)0x73000000)

// Base value for URL list
#define VID_URL_LIST_BASE           ((uint32_t)0x74000000)

// Base value for ICMP target list
#define VID_ICMP_TARGET_LIST_BASE   ((uint32_t)0x75000000)

// Base value for asset properties
#define VID_ASSET_PROPERTIES_BASE   ((uint32_t)0x76000000)

// IP address list base
#define VID_IP_ADDRESS_LIST_BASE    ((uint32_t)0x7F000000)

// Base value for cluster resource list
#define VID_RESOURCE_LIST_BASE      ((uint32_t)0x20000000)

// Base value for cluster sync network list
#define VID_SYNC_SUBNETS_BASE       ((uint32_t)0x28000000)

// Base value for agent's enum values
#define VID_ENUM_VALUE_BASE         ((uint32_t)0x10000000)

// Base value for agent's action arguments
#define VID_ACTION_ARG_BASE         ((uint32_t)0x10000000)

// Base value for agent's parameter list
#define VID_PARAM_LIST_BASE         ((uint32_t)0x10000000)
#define VID_ENUM_LIST_BASE          ((uint32_t)0x20000000)
#define VID_PUSHPARAM_LIST_BASE     ((uint32_t)0x30000000)
#define VID_TABLE_LIST_BASE         ((uint32_t)0x40000000)
#define VID_ACTION_LIST_BASE        ((uint32_t)0x50000000)

// Base value for timer list
#define VID_TIMER_LIST_BASE         ((uint32_t)0x60000000)

// Base value for DCI last values
#define VID_DCI_VALUES_BASE         ((uint32_t)0x10000000)

// Base value for variable names
#define VID_VARLIST_BASE            ((uint32_t)0x10000000)

// Base value for network list
#define VID_VPN_NETWORK_BASE        ((uint32_t)0x10000000)

// Base value for network list
#define VID_OBJECT_TOOLS_BASE       ((uint32_t)0x10000000)

// Base values for table data and object tools
#define VID_COLUMN_INFO_BASE        ((uint32_t)0x10000000)
#define VID_COLUMN_NAME_BASE        ((uint32_t)0x10000000)
#define VID_COLUMN_FMT_BASE         ((uint32_t)0x20000000)
#define VID_ROW_DATA_BASE           ((uint32_t)0x30000000)
#define VID_COLUMN_INFO_BASE        ((uint32_t)0x10000000)
#define VID_FIELD_LIST_BASE         ((uint32_t)0x70000000)
#define VID_ORDER_FIELD_LIST_BASE   ((uint32_t)0x78000000)

// Base value for event log records
#define VID_EVENTLOG_MSG_BASE       ((uint32_t)0x10000000)

// Base value for syslog records
#define VID_SYSLOG_MSG_BASE         ((uint32_t)0x10000000)

// Base value for trap log records
#define VID_TRAP_LOG_MSG_BASE       ((uint32_t)0x10000000)

// Base value for script list
#define VID_SCRIPT_LIST_BASE        ((uint32_t)0x10000000)

// Base value for session data
#define VID_SESSION_DATA_BASE       ((uint32_t)0x10000000)

// Base value for SNMP walker data
#define VID_SNMP_WALKER_DATA_BASE   ((uint32_t)0x10000000)

// Base value for map list
#define VID_MAP_LIST_BASE           ((uint32_t)0x10000000)

// Base value for agent configs list
#define VID_AGENT_CFG_LIST_BASE     ((uint32_t)0x10000000)

// Base and last values for condition's DCI list
#define VID_DCI_LIST_BASE           ((uint32_t)0x40000000)
#define VID_DCI_LIST_LAST           ((uint32_t)0x4FFFFFFF)

// Base value for DCI push data
#define VID_PUSH_DCI_DATA_BASE      ((uint32_t)0x10000000)

// Base value for address list
#define VID_ADDR_LIST_BASE          ((uint32_t)0x10000000)

// Base value for trap configuration records
#define VID_TRAP_INFO_BASE          ((uint32_t)0x10000000)

// Base value for graph list
#define VID_GRAPH_LIST_BASE         ((uint32_t)0x10000000)
#define VID_GRAPH_ACL_BASE				((uint32_t)0x20000000)

// Base value for system DCI list
#define VID_SYSDCI_LIST_BASE			((uint32_t)0x10000000)

// Base value for certificate list
#define VID_CERT_LIST_BASE 			((uint32_t)0x10000000)

// Base value for various string lists
#define VID_STRING_LIST_BASE 			((uint32_t)0x10000000)

// Base values for persistent storage actions in EPP
#define VID_PSTORAGE_SET_LIST_BASE    ((uint32_t)0x10000000)
#define VID_PSTORAGE_DELETE_LIST_BASE ((uint32_t)0x20000000)
// Base values for persistent storage actions in EPP
#define VID_CUSTOM_ATTR_SET_LIST_BASE ((uint32_t)0x30000000)
#define VID_CUSTOM_ATTR_DEL_LIST_BASE ((uint32_t)0x40000000)
// Base value for time based filtr frames in EPP
#define VID_TIME_FRAME_LIST_BASE      ((uint32_t)0x70000000)

// Base values for persistent storage lists
#define VID_PSTORAGE_LIST_BASE      ((uint32_t)0x10000000)

//Base values for file list
#define VID_INSTANCE_LIST_BASE      ((uint32_t)0x20000000)

// Base value for object links list
#define VID_OBJECT_LINKS_BASE			((uint32_t)0x10000000)
#define VID_SUBMAP_LINK_NAMES_BASE  ((uint32_t)0x20000000)

#define VID_TABLE_COLUMN_INFO_BASE  ((uint32_t)0x10000000)
#define VID_TABLE_DATA_BASE         ((uint32_t)0x20000000)

#define VID_COLUMN_FILTERS_BASE     ((uint32_t)0x10000000)
#define VID_ORDERING_COLUMNS_BASE   ((uint32_t)0x40000000)

#define VID_USM_CRED_LIST_BASE      ((uint32_t)0x10000000)

#define VID_IMAGE_LIST_BASE         ((uint32_t)0x20000000)

#define VID_VLAN_LIST_BASE          ((uint32_t)0x10000000)

#define VID_NETWORK_PATH_BASE       ((uint32_t)0x40000000)

#define VID_COMPONENT_LIST_BASE     ((uint32_t)0x20000000)

#define VID_LICENSE_PROBLEM_BASE    ((uint32_t)0x28000000)

#define VID_RADIO_LIST_BASE         ((uint32_t)0x34000000)

#define VID_RULE_LIST_BASE          ((uint32_t)0x10000000)

#define VID_EXTENSION_LIST_BASE     ((uint32_t)0x10000000)

#define VID_DCI_VALUES_BASE         ((uint32_t)0x10000000)

#define VID_FILE_LIST_BASE          ((uint32_t)0x10000000)

#define VID_LOC_LIST_BASE           ((uint32_t)0x10000000)

#define VID_SCHEDULE_LIST_BASE      ((uint32_t)0x10000000)

#define VID_CALLBACK_BASE           ((uint32_t)0x10000000)

#define VID_AGENT_POLICY_BASE       ((uint32_t)0x10000000)

#define VID_EXPAND_STRING_BASE      ((uint32_t)0x10000000)
#define VID_INPUT_FIELD_BASE        ((uint32_t)0x20000000)

#define VID_ZMQ_SUBSCRIPTION_BASE   ((uint32_t)0x10000000)

#define VID_CONFIG_HINT_LIST_BASE   ((uint32_t)0x10000000)

#define VID_ZONE_PROXY_BASE         ((uint32_t)0x70000000)

#define VID_UA_NOTIFICATION_BASE    ((uint32_t)0x10000000)

#define VID_HEADERS_BASE            ((uint32_t)0x20000000)

#define VID_SHARED_SECRET_LIST_BASE ((uint32_t)0x10000000)

#define VID_WEB_SERVICE_DEF_LIST_BASE ((uint32_t)0x10000000)

#define VID_MASKED_FIELD_LIST_BASE  ((uint32_t)0x10000000)

#define VID_SSH_KEY_LIST_BASE       ((uint32_t)0x10000000)

#define VID_MAC_ADDR_LIST_BASE      ((uint32_t)0x30000000)

#define VID_OSPF_AREA_LIST_BASE     ((uint32_t)0x20000000)
#define VID_OSPF_NEIGHBOR_LIST_BASE ((uint32_t)0x30000000)

// base value for SNMP community strings
#define VID_COMMUNITY_STRING_LIST_BASE       ((uint32_t)0x10000000)
#define VID_COMMUNITY_STRING_ZONE_LIST_BASE  ((uint32_t)0x20000000)

#define VID_ZONE_PORT_LIST_BASE     ((uint32_t)0x10000000)

#define VID_2FA_METHOD_LIST_BASE    ((uint32_t)0x10000000)

#define VID_CHECK_LIST_BASE         ((uint32_t)0x10000000)

#define VID_TICKET_LIST_BASE        ((uint32_t)0x10000000)

#define VID_AM_ATTRIBUTES_BASE      ((uint32_t)0x10000000)
#define VID_AM_ENUM_MAP_BASE        ((uint32_t)0x20000000)

// Base field for script warnings
#define VID_WARNING_LIST_BASE       ((uint32_t)0x38000000)

#define VID_UNIT_NAMES_BASE         ((uint32_t)0x10000000)

#define VID_METADATA_BASE           ((uint32_t)0x1F000000)

#endif   /* _nms_cscp_h_ */
