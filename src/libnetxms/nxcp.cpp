/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: nxcp.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>
#include <nxstat.h>
#include <zlib.h>
#include <fstream>

#ifdef _WIN32
#pragma warning( disable : 4267 )
#endif

/**
 * Additional message name resolvers
 */
static Array s_resolvers(4, 4, Ownership::False);
static Mutex s_resolversLock;

/**
 * Get symbolic name for message code
 */
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(uint16_t code, TCHAR *buffer)
{
   static const TCHAR *messageNames[] =
   {
      _T("CMD_LOGIN"),
      _T("CMD_LOGIN_RESP"),
      _T("CMD_KEEPALIVE"),
      _T("CMD_OPEN_HELPDESK_ISSUE"),
      _T("CMD_GET_OBJECTS"),
      _T("CMD_OBJECT"),
      _T("CMD_DELETE_OBJECT"),
      _T("CMD_MODIFY_OBJECT"),
      _T("CMD_OBJECT_LIST_END"),
      _T("CMD_OBJECT_UPDATE"),
      _T("CMD_RECALCULATE_DCI_VALUES"),
      _T("CMD_EVENTLOG_RECORDS"),
      _T("CMD_GET_CONFIG_VARLIST"),
      _T("CMD_SET_CONFIG_VARIABLE"),
      _T("CMD_GET_OBJECT_TOOLS"),
      _T("CMD_EXECUTE_ACTION"),
      _T("CMD_DELETE_CONFIG_VARIABLE"),
      _T("CMD_NOTIFY"),
      _T("CMD_TRAP"),
      _T("CMD_OPEN_EPP"),
      _T("CMD_CLOSE_EPP"),
      _T("CMD_SAVE_EPP"),
      _T("CMD_EPP_RECORD"),
      _T("CMD_EVENT_DB_UPDATE"),
      _T("CMD_TRAP_CFG_UPDATE"),
      _T("CMD_SET_EVENT_INFO"),
      _T("CMD_GET_DCI_MEASUREMENT_UNITS"),
      _T("CMD_LOAD_EVENT_DB"),
      _T("CMD_REQUEST_COMPLETED"),
      _T("CMD_LOAD_USER_DB"),
      _T("CMD_USER_DATA"),
      _T("CMD_GROUP_DATA"),
      _T("CMD_USER_DB_EOF"),
      _T("CMD_UPDATE_USER"),
      _T("CMD_DELETE_USER"),
      _T("CMD_CREATE_USER"),
      _T("CMD_2FA_GET_DRIVERS"),
      _T("CMD_2FA_RENAME_METHOD"),
      _T("CMD_USER_DB_UPDATE"),
      _T("CMD_SET_PASSWORD"),
      _T("CMD_GET_NODE_DCI_LIST"),
      _T("CMD_NODE_DCI"),
      _T("CMD_GET_LOG_DATA"),
      _T("CMD_DELETE_NODE_DCI"),
      _T("CMD_MODIFY_NODE_DCI"),
      _T("CMD_UNLOCK_NODE_DCI_LIST"),
      _T("CMD_SET_OBJECT_MGMT_STATUS"),
      _T("CMD_UPDATE_SYSTEM_ACCESS_RIGHTS"),
      _T("CMD_GET_DCI_DATA"),
      _T("CMD_DCI_DATA"),
      _T("CMD_GET_MIB_TIMESTAMP"),
      _T("CMD_GET_MIB"),
      _T("CMD_TEST_DCI_TRANSFORMATION"),
      _T("CMD_GET_JOB_LIST"),
      _T("CMD_CREATE_OBJECT"),
      _T("CMD_GET_EVENT_NAMES"),
      _T("CMD_EVENT_NAME_LIST"),
      _T("CMD_BIND_OBJECT"),
      _T("CMD_UNBIND_OBJECT"),
      _T("CMD_UNINSTALL_AGENT_POLICY"),
      _T("CMD_OPEN_SERVER_LOG"),
      _T("CMD_CLOSE_SERVER_LOG"),
      _T("CMD_QUERY_LOG"),
      _T("CMD_AUTHENTICATE"),
      _T("CMD_GET_PARAMETER"),
      _T("CMD_GET_LIST"),
      _T("CMD_ACTION"),
      _T("CMD_GET_CURRENT_USER_ATTR"),
      _T("CMD_SET_CURRENT_USER_ATTR"),
      _T("CMD_GET_ALL_ALARMS"),
      _T("CMD_GET_ALARM_COMMENTS"),
      _T("CMD_ACK_ALARM"),
      _T("CMD_ALARM_UPDATE"),
      _T("CMD_ALARM_DATA"),
      _T("CMD_DELETE_ALARM"),
      _T("CMD_ADD_CLUSTER_NODE"),
      _T("CMD_GET_POLICY_INVENTORY"),
      _T("CMD_LOAD_ACTIONS"),
      _T("CMD_ACTION_DB_UPDATE"),
      _T("CMD_MODIFY_ACTION"),
      _T("CMD_CREATE_ACTION"),
      _T("CMD_DELETE_ACTION"),
      _T("CMD_ACTION_DATA"),
      _T("CMD_SETUP_AGENT_TUNNEL"),
      _T("CMD_EXECUTE_LIBRARY_SCRIPT"),
      _T("CMD_GET_PREDICTION_ENGINES"),
      _T("CMD_GET_PREDICTED_DATA"),
      _T("CMD_STOP_SERVER_COMMAND"),
      _T("CMD_POLL_OBJECT"),
      _T("CMD_POLLING_INFO"),
      _T("CMD_COPY_DCI"),
      _T("CMD_WAKEUP_NODE"),
      _T("CMD_DELETE_EVENT_TEMPLATE"),
      _T("CMD_GENERATE_EVENT_CODE"),
      _T("CMD_FIND_NODE_CONNECTION"),
      _T("CMD_FIND_MAC_LOCATION"),
      _T("CMD_CREATE_TRAP"),
      _T("CMD_MODIFY_TRAP"),
      _T("CMD_DELETE_TRAP"),
      _T("CMD_LOAD_TRAP_CFG"),
      _T("CMD_TRAP_CFG_RECORD"),
      _T("CMD_QUERY_PARAMETER"),
      _T("CMD_GET_SERVER_INFO"),
      _T("CMD_SET_DCI_STATUS"),
      _T("CMD_FILE_DATA"),
      _T("CMD_TRANSFER_FILE"),
      _T("CMD_UPGRADE_AGENT"),
      _T("CMD_GET_PACKAGE_LIST"),
      _T("CMD_PACKAGE_INFO"),
      _T("CMD_REMOVE_PACKAGE"),
      _T("CMD_INSTALL_PACKAGE"),
      _T("CMD_THRESHOLD_UPDATE"),
      _T("CMD_GET_SELECTED_USERS"),
      _T("CMD_ABORT_FILE_TRANSFER"),
      _T("CMD_CHECK_NETWORK_SERVICE"),
      _T("CMD_READ_AGENT_CONFIG_FILE"),
      _T("CMD_WRITE_AGENT_CONFIG_FILE"),
      _T("CMD_GET_PARAMETER_LIST"),
      _T("CMD_DEPLOY_PACKAGE"),
      _T("CMD_INSTALLER_INFO"),
      _T("CMD_GET_LAST_VALUES"),
      _T("CMD_APPLY_TEMPLATE"),
      _T("CMD_SET_USER_VARIABLE"),
      _T("CMD_GET_USER_VARIABLE"),
      _T("CMD_ENUM_USER_VARIABLES"),
      _T("CMD_DELETE_USER_VARIABLE"),
      _T("CMD_ADM_MESSAGE"),
      _T("CMD_ADM_REQUEST"),
      _T("CMD_GET_NETWORK_PATH"),
      _T("CMD_REQUEST_SESSION_KEY"),
      _T("CMD_ENCRYPTED_MESSAGE"),
      _T("CMD_SESSION_KEY"),
      _T("CMD_REQUEST_ENCRYPTION"),
      _T("CMD_GET_ROUTING_TABLE"),
      _T("CMD_EXEC_TABLE_TOOL"),
      _T("CMD_TABLE_DATA"),
      _T("CMD_CANCEL_JOB"),
      _T("CMD_CHANGE_SUBSCRIPTION"),
      _T("CMD_SET_CONFIG_TO_DEFAULT"),
      _T("CMD_SYSLOG_RECORDS"),
      _T("CMD_JOB_CHANGE_NOTIFICATION"),
      _T("CMD_DEPLOY_AGENT_POLICY"),
      _T("CMD_LOG_DATA"),
      _T("CMD_GET_OBJECT_TOOL_DETAILS"),
      _T("CMD_EXECUTE_SERVER_COMMAND"),
      _T("CMD_UPLOAD_FILE_TO_AGENT"),
      _T("CMD_UPDATE_OBJECT_TOOL"),
      _T("CMD_DELETE_OBJECT_TOOL"),
      _T("CMD_SETUP_PROXY_CONNECTION"),
      _T("CMD_GENERATE_OBJECT_TOOL_ID"),
      _T("CMD_GET_SERVER_STATS"),
      _T("CMD_GET_SCRIPT_LIST"),
      _T("CMD_GET_SCRIPT"),
      _T("CMD_UPDATE_SCRIPT"),
      _T("CMD_DELETE_SCRIPT"),
      _T("CMD_RENAME_SCRIPT"),
      _T("CMD_GET_SESSION_LIST"),
      _T("CMD_KILL_SESSION"),
      _T("CMD_SET_DB_PASSWORD"),
      _T("CMD_TRAP_LOG_RECORDS"),
      _T("CMD_START_SNMP_WALK"),
      _T("CMD_SNMP_WALK_DATA"),
      _T("CMD_GET_MAP_LIST"),
      _T("CMD_LOAD_MAP"),
      _T("CMD_SAVE_MAP"),
      _T("CMD_DELETE_MAP"),
      _T("CMD_RESOLVE_MAP_NAME"),
      _T("CMD_SUBMAP_DATA"),
      _T("CMD_UPLOAD_SUBMAP_BK_IMAGE"),
      _T("CMD_GET_SUBMAP_BK_IMAGE"),
      _T("CMD_GET_MODULE_LIST"),
      _T("CMD_UPDATE_MODULE_INFO"),
      _T("CMD_COPY_USER_VARIABLE"),
      _T("CMD_RESOLVE_DCI_NAMES"),
      _T("CMD_GET_MY_CONFIG"),
      _T("CMD_GET_AGENT_CONFIGURATION_LIST"),
      _T("CMD_GET_AGENT_CONFIGURATION"),
      _T("CMD_UPDATE_AGENT_CONFIGURATION"),
      _T("CMD_DELETE_AGENT_CONFIGURATION"),
      _T("CMD_SWAP_AGENT_CONFIGURATIONS"),
      _T("CMD_TERMINATE_ALARM"),
      _T("CMD_GET_NXCP_CAPS"),
      _T("CMD_NXCP_CAPS"),
      _T("CMD_GET_OBJECT_COMMENTS"),
      _T("CMD_UPDATE_OBJECT_COMMENTS"),
      _T("CMD_ENABLE_AGENT_TRAPS"),
      _T("CMD_PUSH_DCI_DATA"),
      _T("CMD_GET_ADDR_LIST"),
      _T("CMD_SET_ADDR_LIST"),
      _T("CMD_RESET_COMPONENT"),
      _T("CMD_GET_RELATED_EVENTS_LIST"),
      _T("CMD_EXPORT_CONFIGURATION"),
      _T("CMD_IMPORT_CONFIGURATION"),
      _T("CMD_GET_TRAP_CFG_RO"),
		_T("CMD_SNMP_REQUEST"),
		_T("CMD_GET_DCI_INFO"),
		_T("CMD_GET_GRAPH_LIST"),
		_T("CMD_SAVE_GRAPH"),
		_T("CMD_DELETE_GRAPH"),
      _T("CMD_GET_PERFTAB_DCI_LIST"),
      _T("CMD_GET_OBJECT_CATEGORIES"),
      _T("CMD_MODIFY_OBJECT_CATEGORY"),
      _T("CMD_DELETE_OBJECT_CATEGORY"),
      _T("CMD_WINDOWS_EVENT"),
		_T("CMD_QUERY_L2_TOPOLOGY"),
		_T("CMD_AUDIT_RECORD"),
		_T("CMD_GET_AUDIT_LOG"),
		_T("CMD_SEND_NOTIFICATION"),
		_T("CMD_GET_COMMUNITY_LIST"),
		_T("CMD_UPDATE_COMMUNITY_LIST"),
		_T("CMD_GET_PERSISTENT_STORAGE"),
		_T("CMD_DELETE_PSTORAGE_VALUE"),
		_T("CMD_UPDATE_PSTORAGE_VALUE"),
		_T("CMD_GET_AGENT_TUNNELS"),
		_T("CMD_BIND_AGENT_TUNNEL"),
		_T("CMD_REQUEST_CERTIFICATE"),
		_T("CMD_NEW_CERTIFICATE"),
		_T("CMD_CREATE_MAP"),
		_T("CMD_UPLOAD_FILE"),
		_T("CMD_DELETE_FILE"),
		_T("CMD_RENAME_FILE"),
      _T("CMD_CLONE_MAP"),
		_T("CMD_AGENT_TUNNEL_UPDATE"),
		_T("CMD_FIND_VENDOR_BY_MAC"),
		_T("CMD_CONFIG_SET_CLOB"),
		_T("CMD_CONFIG_GET_CLOB"),
		_T("CMD_RENAME_MAP"),
		_T("CMD_CLEAR_DCI_DATA"),
		_T("CMD_GET_LICENSE"),
		_T("CMD_CHECK_LICENSE"),
		_T("CMD_RELEASE_LICENSE"),
		_T("CMD_ISC_CONNECT_TO_SERVICE"),
		_T("CMD_REGISTER_AGENT"),
		_T("CMD_GET_SERVER_FILE"),
		_T("CMD_FORWARD_EVENT"),
		_T("CMD_GET_USM_CREDENTIALS"),
		_T("CMD_UPDATE_USM_CREDENTIALS"),
		_T("CMD_GET_DCI_THRESHOLDS"),
		_T("CMD_GET_IMAGE"),
		_T("CMD_CREATE_IMAGE"),
		_T("CMD_DELETE_IMAGE"),
		_T("CMD_MODIFY_IMAGE"),
		_T("CMD_LIST_IMAGES"),
		_T("CMD_LIST_SERVER_FILES"),
		_T("CMD_GET_TABLE"),
		_T("CMD_QUERY_TABLE"),
		_T("CMD_OPEN_CONSOLE"),
		_T("CMD_CLOSE_CONSOLE"),
		_T("CMD_GET_SELECTED_OBJECTS"),
		_T("CMD_GET_VLANS"),
		_T("CMD_HOLD_JOB"),
		_T("CMD_UNHOLD_JOB"),
		_T("CMD_CHANGE_ZONE"),
		_T("CMD_GET_AGENT_FILE"),
		_T("CMD_GET_FILE_DETAILS"),
		_T("CMD_IMAGE_LIBRARY_UPDATE"),
		_T("CMD_GET_NODE_COMPONENTS"),
		_T("CMD_UPDATE_ALARM_COMMENT"),
		_T("CMD_GET_ALARM"),
		_T("CMD_GET_TABLE_LAST_VALUE"),
		_T("CMD_GET_TABLE_DCI_DATA"),
		_T("CMD_GET_THRESHOLD_SUMMARY"),
		_T("CMD_RESOLVE_ALARM"),
		_T("CMD_FIND_IP_LOCATION"),
		_T("CMD_REPORT_DEVICE_STATUS"),
		_T("CMD_REPORT_DEVICE_INFO"),
		_T("CMD_GET_ALARM_EVENTS"),
		_T("CMD_GET_ENUM_LIST"),
		_T("CMD_GET_TABLE_LIST"),
		_T("CMD_GET_MAPPING_TABLE"),
		_T("CMD_UPDATE_MAPPING_TABLE"),
		_T("CMD_DELETE_MAPPING_TABLE"),
		_T("CMD_LIST_MAPPING_TABLES"),
		_T("CMD_GET_NODE_SOFTWARE"),
		_T("CMD_GET_WINPERF_OBJECTS"),
		_T("CMD_GET_WIRELESS_STATIONS"),
		_T("CMD_GET_SUMMARY_TABLES"),
		_T("CMD_MODIFY_SUMMARY_TABLE"),
		_T("CMD_DELETE_SUMMARY_TABLE"),
		_T("CMD_GET_SUMMARY_TABLE_DETAILS"),
		_T("CMD_QUERY_SUMMARY_TABLE"),
      _T("CMD_SHUTDOWN"),
      _T("CMD_SNMP_TRAP"),
      _T("CMD_GET_SUBNET_ADDRESS_MAP"),
      _T("CMD_FILE_MONITORING"),
      _T("CMD_CANCEL_FILE_MONITORING"),
      _T("CMD_CHANGE_OBJECT_TOOL_STATUS"),
      _T("CMD_SET_ALARM_STATUS_FLOW"),
      _T("CMD_DELETE_ALARM_COMMENT"),
      _T("CMD_GET_EFFECTIVE_RIGHTS"),
      _T("CMD_GET_DCI_VALUES"),
      _T("CMD_GET_HELPDESK_URL"),
      _T("CMD_UNLINK_HELPDESK_ISSUE"),
      _T("CMD_GET_FOLDER_CONTENT"),
      _T("CMD_FILEMGR_DELETE_FILE"),
      _T("CMD_FILEMGR_RENAME_FILE"),
      _T("CMD_FILEMGR_MOVE_FILE"),
      _T("CMD_FILEMGR_UPLOAD"),
      _T("CMD_GET_SWITCH_FDB"),
      _T("CMD_COMMAND_OUTPUT"),
      _T("CMD_GET_LOC_HISTORY"),
      _T("CMD_TAKE_SCREENSHOT"),
      _T("CMD_EXECUTE_SCRIPT"),
      _T("CMD_EXECUTE_SCRIPT_UPDATE"),
      _T("CMD_FILEMGR_CREATE_FOLDER"),
      _T("CMD_QUERY_ADHOC_SUMMARY_TABLE"),
      _T("CMD_GRAPH_UPDATE"),
      _T("CMD_SET_SERVER_CAPABILITIES"),
      _T("CMD_FORCE_DCI_POLL"),
      _T("CMD_GET_DCI_SCRIPT_LIST"),
      _T("CMD_DATA_COLLECTION_CONFIG"),
      _T("CMD_SET_SERVER_ID"),
      _T("CMD_GET_PUBLIC_CONFIG_VAR"),
      _T("CMD_ENABLE_FILE_UPDATES"),
      _T("CMD_DETACH_LDAP_USER"),
      _T("CMD_VALIDATE_PASSWORD"),
      _T("CMD_COMPILE_SCRIPT"),
      _T("CMD_CLEAN_AGENT_DCI_CONF"),
      _T("CMD_RESYNC_AGENT_DCI_CONF"),
      _T("CMD_LIST_SCHEDULE_CALLBACKS"),
      _T("CMD_LIST_SCHEDULES"),
      _T("CMD_ADD_SCHEDULE"),
      _T("CMD_UPDATE_SCHEDULE"),
      _T("CMD_REMOVE_SCHEDULE"),
      _T("CMD_ENTER_MAINT_MODE"),
      _T("CMD_LEAVE_MAINT_MODE"),
      _T("CMD_JOIN_CLUSTER"),
      _T("CMD_CLUSTER_NOTIFY"),
      _T("CMD_GET_OBJECT_QUERIES"),
      _T("CMD_MODIFY_OBJECT_QUERY"),
      _T("CMD_DELETE_OBJECT_QUERY"),
      _T("CMD_FILEMGR_CHMOD"),
      _T("CMD_FILEMGR_CHOWN"),
      _T("CMD_FILEMGR_GET_FILE_FINGERPRINT"),
      _T("CMD_GET_REPOSITORIES"),
      _T("CMD_ADD_REPOSITORY"),
      _T("CMD_MODIFY_REPOSITORY"),
      _T("CMD_DELETE_REPOSITORY"),
      _T("CMD_GET_ALARM_CATEGORIES"),
      _T("CMD_MODIFY_ALARM_CATEGORY"),
      _T("CMD_DELETE_ALARM_CATEGORY"),
      _T("CMD_ALARM_CATEGORY_UPDATE"),
      _T("CMD_BULK_TERMINATE_ALARMS"),
      _T("CMD_BULK_RESOLVE_ALARMS"),
      _T("CMD_BULK_ALARM_STATE_CHANGE"),
      _T("CMD_GET_FOLDER_SIZE"),
      _T("CMD_FIND_HOSTNAME_LOCATION"),
      _T("CMD_RESET_TUNNEL"),
      _T("CMD_CREATE_CHANNEL"),
      _T("CMD_CHANNEL_DATA"),
      _T("CMD_CLOSE_CHANNEL"),
      _T("CMD_CREATE_OBJECT_ACCESS_SNAPSHOT"),
      _T("CMD_UNBIND_AGENT_TUNNEL"),
      _T("CMD_RESTART"),
      _T("CMD_REGISTER_LORAWAN_SENSOR"),
      _T("CMD_UNREGISTER_LORAWAN_SENSOR"),
      _T("CMD_EXPAND_MACROS"),
      _T("CMD_EXECUTE_ACTION_WITH_EXPANSION"),
      _T("CMD_GET_HOSTNAME_BY_IPADDR"),
      _T("CMD_CANCEL_FILE_DOWNLOAD"),
      _T("CMD_FILEMGR_COPY_FILE"),
      _T("CMD_QUERY_OBJECTS"),
      _T("CMD_QUERY_OBJECT_DETAILS"),
      _T("CMD_SETUP_TCP_PROXY"),
      _T("CMD_TCP_PROXY_DATA"),
      _T("CMD_CLOSE_TCP_PROXY"),
      _T("CMD_GET_DEPENDENT_NODES"),
      _T("CMD_DELETE_DCI_ENTRY"),
      _T("CMD_GET_ACTIVE_THRESHOLDS"),
      _T("CMD_QUERY_INTERNAL_TOPOLOGY"),
      _T("CMD_GET_ACTION_LIST"),
      _T("CMD_PROXY_MESSAGE"),
      _T("CMD_GET_GRAPH"),
      _T("CMD_UPDATE_AGENT_POLICY"),
      _T("CMD_DELETE_AGENT_POLICY"),
      _T("CMD_GET_AGENT_POLICY_LIST"),
      _T("CMD_POLICY_EDITOR_CLOSED"),
      _T("CMD_POLICY_FORCE_APPLY"),
      _T("CMD_GET_NODE_HARDWARE"),
      _T("CMD_GET_MATCHING_DCI"),
      _T("CMD_GET_UA_NOTIFICATIONS"),
      _T("CMD_ADD_UA_NOTIFICATION"),
      _T("CMD_RECALL_UA_NOTIFICATION"),
      _T("CMD_UPDATE_UA_NOTIFICATIONS"),
      _T("CMD_GET_NOTIFICATION_CHANNELS"),
      _T("CMD_ADD_NOTIFICATION_CHANNEL"),
      _T("CMD_UPDATE_NOTIFICATION_CHANNEL"),
      _T("CMD_DELETE_NOTIFICATION_CHANNEL"),
      _T("CMD_GET_NOTIFICATION_DRIVERS"),
      _T("CMD_RENAME_NOTIFICATION_CHANNEL"),
      _T("CMD_GET_AGENT_POLICY"),
      _T("CMD_START_ACTIVE_DISCOVERY"),
      _T("CMD_GET_PHYSICAL_LINKS"),
      _T("CMD_UPDATE_PHYSICAL_LINK"),
      _T("CMD_DELETE_PHYSICAL_LINK"),
      _T("CMD_GET_FILE_SET_DETAILS"),
      _T("CMD_IMPORT_CONFIGURATION_FILE"),
      _T("CMD_0x018E"), // was CMD_REMOVE_MQTT_BROKER
      _T("CMD_0x018F"), // was CMD_ADD_MQTT_TOPIC
      _T("CMD_0x0190"), // was CMD_REMOVE_MQTT_TOPIC
      _T("CMD_QUERY_WEB_SERVICE"),
      _T("CMD_GET_WEB_SERVICES"),
      _T("CMD_MODIFY_WEB_SERVICE"),
      _T("CMD_DELETE_WEB_SERVICE"),
      _T("CMD_WEB_SERVICE_DEFINITION"),
      _T("CMD_GET_SCREEN_INFO"),
      _T("CMD_UPDATE_ENVIRONMENT"),
      _T("CMD_GET_SHARED_SECRET_LIST"),
      _T("CMD_UPDATE_SHARED_SECRET_LIST"),
      _T("CMD_GET_WELL_KNOWN_PORT_LIST"),
      _T("CMD_UPDATE_WELL_KNOWN_PORT_LIST"),
      _T("CMD_GET_LOG_RECORD_DETAILS"),
      _T("CMD_GET_DCI_LAST_VALUE"),
      _T("CMD_OBJECT_CATEGORY_UPDATE"),
      _T("CMD_GET_GEO_AREAS"),
      _T("CMD_MODIFY_GEO_AREA"),
      _T("CMD_DELETE_GEO_AREA"),
      _T("CMD_GEO_AREA_UPDATE"),
      _T("CMD_FIND_PROXY_FOR_NODE"),
      _T("CMD_BULK_DCI_UPDATE"),
      _T("CMD_GET_SCHEDULED_REPORTING_TASKS"),
      _T("CMD_CONFIGURE_REPORTING_SERVER"),
      _T("CMD_GET_SSH_KEYS_LIST"),
      _T("CMD_DELETE_SSH_KEY"),
      _T("CMD_UPDATE_SSH_KEYS"),
      _T("CMD_GENERATE_SSH_KEYS"),
      _T("CMD_GET_SSH_KEYS"),
      _T("CMD_GET_TOOLTIP_LAST_VALUES"),
      _T("CMD_SYNC_AGENT_POLICIES"),
      _T("CMD_2FA_PREPARE_CHALLENGE"),
      _T("CMD_2FA_VALIDATE_RESPONSE"),
      _T("CMD_2FA_GET_METHODS"),
      _T("CMD_GET_OSPF_DATA"),
      _T("CMD_2FA_MODIFY_METHOD"),
      _T("CMD_2FA_DELETE_METHOD"),
      _T("CMD_2FA_GET_USER_BINDINGS"),
      _T("CMD_SCRIPT_EXECUTION_RESULT"),
      _T("CMD_2FA_MODIFY_USER_BINDING"),
      _T("CMD_2FA_DELETE_USER_BINDING"),
      _T("CMD_WEB_SERVICE_CUSTOM_REQUEST"),
      _T("CMD_QUERY_OSPF_TOPOLOGY"),
      _T("CMD_FILEMGR_MERGE_FILES"),
      _T("CMD_GET_BIZSVC_CHECK_LIST"),
      _T("CMD_UPDATE_BIZSVC_CHECK"),
      _T("CMD_DELETE_BIZSVC_CHECK"),
      _T("CMD_GET_BUSINESS_SERVICE_UPTIME"),
      _T("CMD_GET_BUSINESS_SERVICE_TICKETS"),
      _T("CMD_SSH_COMMAND"),
      _T("CMD_FIND_DCI"),
      _T("CMD_UPDATE_PACKAGE_METADATA"),
      _T("CMD_GET_EVENT_REFERENCES"),
      _T("CMD_READ_MAINTENANCE_JOURNAL"),
      _T("CMD_WRITE_MAINTENANCE_JOURNAL"),
      _T("CMD_UPDATE_MAINTENANCE_JOURNAL"),
      _T("CMD_GET_SSH_CREDENTIALS"),
      _T("CMD_UPDATE_SSH_CREDENTIALS"),
      _T("CMD_GET_ASSET_MGMT_ATTRIBUTE"),
      _T("CMD_CREATE_ASSET_MGMT_ATTRIBUTE"),
      _T("CMD_UPDATE_ASSET_MGMT_ATTRIBUTE"),
      _T("CMD_DELETE_ASSET_MGMT_ATTRIBUTE"),
      _T("CMD_UPDATE_ASSET_INSTANCE"),
      _T("CMD_DELETE_ASSET_INSTANCE")
   };
   static const TCHAR *reportingMessageNames[] =
   {
      _T("CMD_RS_LIST_REPORTS"),
      _T("CMD_RS_GET_REPORT_DEFINITION"),
      _T("CMD_RS_EXECUTE_REPORT"),
      _T("CMD_RS_LIST_RESULTS"),
      _T("CMD_RS_RENDER_RESULT"),
      _T("CMD_RS_DELETE_RESULT"),
      _T("CMD_RS_NOTIFY")
   };

   if ((code >= CMD_LOGIN) && (code <= CMD_DELETE_ASSET_INSTANCE))
   {
      _tcscpy(buffer, messageNames[code - CMD_LOGIN]);
   }
   else if ((code >= CMD_RS_LIST_REPORTS) && (code <= CMD_RS_NOTIFY))
   {
      _tcscpy(buffer, reportingMessageNames[code - CMD_RS_LIST_REPORTS]);
   }
   else
   {
      bool resolved = false;
      s_resolversLock.lock();
      for(int i = 0; i < s_resolvers.size(); i++)
         if (((NXCPMessageNameResolver)s_resolvers.get(i))(code, buffer))
         {
            resolved = true;
            break;
         }
      s_resolversLock.unlock();
      if (!resolved)
         _sntprintf(buffer, 64, _T("CMD_0x%04X"), code);
   }
   return buffer;
}

/**
 * Register NXCP message name resolver
 */
void LIBNETXMS_EXPORTABLE NXCPRegisterMessageNameResolver(NXCPMessageNameResolver r)
{
   s_resolversLock.lock();
   if (s_resolvers.indexOf((void *)r) == -1)
      s_resolvers.add((void *)r);
   s_resolversLock.unlock();
}

/**
 * Unregister NXCP message name resolver
 */
void LIBNETXMS_EXPORTABLE NXCPUnregisterMessageNameResolver(NXCPMessageNameResolver r)
{
   s_resolversLock.lock();
   s_resolvers.remove((void *)r);
   s_resolversLock.unlock();
}

/**
 * Create NXCP message with raw data (MF_BINARY flag)
 * If buffer is NULL, new buffer is allocated with malloc()
 * Buffer should be at least dataSize + NXCP_HEADER_SIZE + 8 bytes.
 */
NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(uint16_t code, uint32_t id, uint16_t flags,
         const void *data, size_t dataSize, NXCP_MESSAGE *buffer, bool allowCompression)
{
   NXCP_MESSAGE *msg = (buffer == nullptr) ? static_cast<NXCP_MESSAGE*>(MemAlloc(dataSize + NXCP_HEADER_SIZE + 8)) : buffer;

   // Message should be aligned to 8 bytes boundary
   size_t padding = (8 - ((dataSize + NXCP_HEADER_SIZE) % 8)) & 7;

   msg->code = htons(code);
   msg->flags = htons(MF_BINARY | flags);
   msg->id = htonl(id);
   size_t msgSize = dataSize + NXCP_HEADER_SIZE + padding;
   msg->size = htonl(static_cast<uint32_t>(msgSize));
   msg->numFields = htonl(static_cast<uint32_t>(dataSize));   // numFields contains actual data size for binary message

   if (allowCompression && (dataSize > 128))
   {
      z_stream stream;
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;
      stream.avail_in = 0;
      stream.next_in = Z_NULL;
      if (deflateInit(&stream, 9) == Z_OK)
      {
         stream.next_in = (BYTE *)data;
         stream.avail_in = (UINT32)dataSize;
         stream.next_out = (BYTE *)msg->fields + 4;
         stream.avail_out = (UINT32)(dataSize + padding - 4);
         if (deflate(&stream, Z_FINISH) == Z_STREAM_END)
         {
            size_t compMsgSize = dataSize - stream.avail_out + NXCP_HEADER_SIZE + 4;
            // Message should be aligned to 8 bytes boundary
            compMsgSize += (8 - (compMsgSize % 8)) & 7;
            if (compMsgSize < msgSize - 4)
            {
               msg->flags |= htons(MF_COMPRESSED);
               memcpy((BYTE *)msg + NXCP_HEADER_SIZE, &msg->size, 4); // Save size of uncompressed message
               msg->size = htonl((UINT32)compMsgSize);
            }
            else
            {
               // compression produce message of same size
               memcpy(msg->fields, data, dataSize);
            }
         }
         else
         {
            // compression failed, send uncompressed message
            memcpy(msg->fields, data, dataSize);
         }
         deflateEnd(&stream);
      }
   }
   else if (dataSize > 0)
   {
      memcpy(msg->fields, data, dataSize);
   }
   return msg;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, uint32_t requestId, const TCHAR *fileName, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag)
{
   SocketCommChannel ch(hSocket, nullptr, Ownership::False);
   bool result = SendFileOverNXCP(&ch, requestId, fileName, ectx, offset, progressCallback, cbArg, mutex, compressionMethod, cancellationFlag);
   return result;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, uint32_t requestId, const TCHAR *fileName, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag)
{
   std::ifstream s;
#ifdef UNICODE
   char fn[MAX_PATH];
   WideCharToMultiByteSysLocale(fileName, fn, MAX_PATH);
   s.open(fn, std::ios::binary);
#else
   s.open(fileName, std::ios::binary);
#endif
   if (s.fail())
      return false;

   bool result = SendFileOverNXCP(channel, requestId, &s, ectx, offset, progressCallback, cbArg, mutex, compressionMethod, cancellationFlag);

   s.close();
   return result;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, uint32_t requestId, std::istream *stream, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag)
{
   SocketCommChannel ch(hSocket, nullptr, Ownership::False);
   bool result = SendFileOverNXCP(&ch, requestId, stream, ectx, offset, progressCallback, cbArg, mutex, compressionMethod, cancellationFlag);
   return result;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, uint32_t requestId, std::istream *stream, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag)
{
   bool success = false;
   size_t bytesTransferred = 0;

   stream->seekg(offset, (offset < 0) ? std::ios_base::end : std::ios_base::beg);
   if (!stream->fail())
   {
      StreamCompressor *compressor = (compressionMethod != NXCP_STREAM_COMPRESSION_NONE) ? StreamCompressor::create(compressionMethod, true, FILE_BUFFER_SIZE) : nullptr;
      BYTE *compBuffer = (compressor != nullptr) ? MemAllocArrayNoInit<BYTE>(FILE_BUFFER_SIZE) : nullptr;

      // Allocate message and prepare it's header
      NXCP_MESSAGE *msg = static_cast<NXCP_MESSAGE*>(MemAlloc(NXCP_HEADER_SIZE + 8 + ((compressor != nullptr) ? compressor->compressBufferSize(FILE_BUFFER_SIZE) + 4 : FILE_BUFFER_SIZE)));
      msg->id = htonl(requestId);
      msg->code = htons(CMD_FILE_DATA);
      msg->flags = htons(MF_BINARY | MF_STREAM | ((compressionMethod != NXCP_STREAM_COMPRESSION_NONE) ? MF_COMPRESSED : 0));

      while(true)
      {
         if ((cancellationFlag != nullptr) && (*cancellationFlag > 0))
            break;

         size_t bytes;
         if (compressor != nullptr)
         {
            stream->read(reinterpret_cast<char*>(compBuffer), FILE_BUFFER_SIZE);
            if (stream->bad())
               break;
            bytes = static_cast<size_t>(stream->gcount());
            if (bytes > 0)
            {
               // Each compressed data block prepended with 4 bytes header
               // First byte contains compression method, second is always 0,
               // third and fourth contains uncompressed block size in network byte order
               *((BYTE *)msg->fields) = (BYTE)compressionMethod;
               *((BYTE *)msg->fields + 1) = 0;
               *((uint16_t *)((BYTE *)msg->fields + 2)) = htons((uint16_t)bytes);
               bytes = compressor->compress(compBuffer, bytes, (BYTE *)msg->fields + 4, compressor->compressBufferSize(FILE_BUFFER_SIZE)) + 4;
            }
         }
         else
         {
            stream->read(reinterpret_cast<char*>(msg->fields), FILE_BUFFER_SIZE);
            if (stream->bad())
               break;
            bytes = static_cast<size_t>(stream->gcount());
         }

         // Message should be aligned to 8 bytes boundary
         uint32_t padding = (8 - ((static_cast<uint32_t>(bytes) + NXCP_HEADER_SIZE) % 8)) & 7;
         msg->size = htonl(static_cast<uint32_t>(bytes) + NXCP_HEADER_SIZE + padding);
         msg->numFields = htonl(static_cast<uint32_t>(bytes));   // numFields contains actual data size for binary message
         if (stream->eof())
            msg->flags |= htons(MF_END_OF_FILE);

         if (ectx != nullptr)
         {
            NXCP_ENCRYPTED_MESSAGE *emsg = ectx->encryptMessage(msg);
            if (emsg != nullptr)
            {
               channel->send(emsg, ntohl(emsg->size), mutex);
               MemFree(emsg);
            }
         }
         else
         {
            if (channel->send(msg, bytes + NXCP_HEADER_SIZE + padding, mutex) <= 0)
               break;	// Send error
         }

         if (progressCallback != nullptr)
         {
            bytesTransferred += bytes;
            progressCallback(bytesTransferred, cbArg);
         }

         if (stream->eof())
         {
            // End of file
            success = true;
            break;
         }
      }

      MemFree(compBuffer);
      delete compressor;

      MemFree(msg);
   }

   // If file upload failed, send CMD_ABORT_FILE_TRANSFER
   if (!success)
   {
      NXCP_MESSAGE msg;
      msg.id = htonl(requestId);
      msg.code = htons(CMD_ABORT_FILE_TRANSFER);
      msg.flags = htons(MF_BINARY);
      msg.numFields = 0;
      msg.size = htonl(NXCP_HEADER_SIZE);
      if (ectx != nullptr)
      {
         NXCP_ENCRYPTED_MESSAGE *emsg = ectx->encryptMessage(&msg);
         if (emsg != nullptr)
         {
            channel->send(emsg, ntohl(emsg->size), mutex);
            MemFree(emsg);
         }
      }
      else
      {
         channel->send(&msg, NXCP_HEADER_SIZE, mutex);
      }
   }

   return success;
}

/**
 * Get version of NXCP used by peer
 */
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(const shared_ptr<AbstractCommChannel>& channel, int *version, Mutex *mutex)
{
   bool success = false;

   NXCP_MESSAGE msg;
   msg.id = 0;
   msg.numFields = 0;
   msg.size = htonl(NXCP_HEADER_SIZE);
   msg.code = htons(CMD_GET_NXCP_CAPS);
   msg.flags = htons(MF_CONTROL | MF_NXCP_VERSION(NXCP_VERSION));
   if (channel->send(&msg, NXCP_HEADER_SIZE, mutex) == NXCP_HEADER_SIZE)
   {
      CommChannelMessageReceiver receiver(channel, 1024, 32768);
      MessageReceiverResult result;
      NXCPMessage *response = receiver.readMessage(10000, &result);
      if ((response != nullptr) && (response->getCode() == CMD_NXCP_CAPS) && response->isControl())
      {
         success = true;
         *version = response->getControlData() >> 24;
      }
      else if ((result == MSGRECV_TIMEOUT) || (result == MSGRECV_PROTOCOL_ERROR))
      {
         // We don't receive any answer or receive invalid answer -
         // assume that peer doesn't understand CMD_GET_NXCP_CAPS message
         // and set version number to 1
         success = true;
         *version = 1;
      }
      delete response;
   }
   return success;
}

/**
 * Get version of NXCP used by peer
 */
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET s, int *version, Mutex *mutex)
{
   auto channel = make_shared<SocketCommChannel>(s, nullptr, Ownership::False);
   bool success = NXCPGetPeerProtocolVersion(static_pointer_cast<AbstractCommChannel>(channel), version, mutex);
   return success;
}
