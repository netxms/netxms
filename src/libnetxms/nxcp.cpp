/*
** NetXMS - Network Management System
** NetXMS Foundation Library
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
** File: nxcp.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>
#include <nxstat.h>
#include <zlib.h>

#ifdef _WIN32
#pragma warning( disable : 4267 )
#endif

/**
 * Additional message name resolvers
 */
static Array s_resolvers(4, 4, false);
static Mutex s_resolversLock;

/**
 * Get symbolic name for message code
 */
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(WORD code, TCHAR *pszBuffer)
{
   static const TCHAR *pszMsgNames[] =
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
      _T("CMD_EVENT_DB_RECORD"),
      _T("CMD_LOAD_EVENT_DB"),
      _T("CMD_REQUEST_COMPLETED"),
      _T("CMD_LOAD_USER_DB"),
      _T("CMD_USER_DATA"),
      _T("CMD_GROUP_DATA"),
      _T("CMD_USER_DB_EOF"),
      _T("CMD_UPDATE_USER"),
      _T("CMD_DELETE_USER"),
      _T("CMD_CREATE_USER"),
      _T("CMD_LOCK_USER_DB"),
      _T("CMD_UNLOCK_USER_DB"),
      _T("CMD_USER_DB_UPDATE"),
      _T("CMD_SET_PASSWORD"),
      _T("CMD_GET_NODE_DCI_LIST"),
      _T("CMD_NODE_DCI"),
      _T("CMD_GET_LOG_DATA"),
      _T("CMD_DELETE_NODE_DCI"),
      _T("CMD_MODIFY_NODE_DCI"),
      _T("CMD_UNLOCK_NODE_DCI_LIST"),
      _T("CMD_SET_OBJECT_MGMT_STATUS"),
      _T("CMD_CREATE_NEW_DCI"),
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
      _T("CMD_POLL_NODE"),
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
      _T("0x0070"),
      _T("0x0071"),
      _T("CMD_ABORT_FILE_TRANSFER"),
      _T("CMD_CHECK_NETWORK_SERVICE"),
      _T("CMD_GET_AGENT_CONFIG"),
      _T("CMD_UPDATE_AGENT_CONFIG"),
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
      _T("<unused>0x009F"),
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
      _T("CMD_GET_AGENT_CFG_LIST"),
      _T("CMD_OPEN_AGENT_CONFIG"),
      _T("CMD_SAVE_AGENT_CONFIG"),
      _T("CMD_DELETE_AGENT_CONFIG"),
      _T("CMD_SWAP_AGENT_CONFIGS"),
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
      _T("CMD_GET_DCI_EVENTS_LIST"),
      _T("CMD_EXPORT_CONFIGURATION"),
      _T("CMD_IMPORT_CONFIGURATION"),
      _T("CMD_GET_TRAP_CFG_RO"),
		_T("CMD_SNMP_REQUEST"),
		_T("CMD_GET_DCI_INFO"),
		_T("CMD_GET_GRAPH_LIST"),
		_T("CMD_SAVE_GRAPH"),
		_T("CMD_DELETE_GRAPH"),
		_T("CMD_GET_PERFTAB_DCI_LIST"),
		_T("CMD_ADD_CA_CERTIFICATE"),
		_T("CMD_DELETE_CERTIFICATE"),
		_T("CMD_GET_CERT_LIST"),
		_T("CMD_UPDATE_CERT_COMMENTS"),
		_T("CMD_QUERY_L2_TOPOLOGY"),
		_T("CMD_AUDIT_RECORD"),
		_T("CMD_GET_AUDIT_LOG"),
		_T("CMD_SEND_SMS"),
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
		_T("CMD_DELETE_REPORT_RESULTS"),
		_T("CMD_RENDER_REPORT"),
		_T("CMD_EXECUTE_REPORT"),
		_T("CMD_GET_REPORT_RESULTS"),
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
		_T("CMD_GET_TABLE_LAST_VALUES"),
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
      _T("CMD_ZMQ_SUBSCRIBE_EVENT"),
      _T("CMD_ZMQ_UNSUBSCRIBE_EVENT"),
      _T("CMD_ZMQ_SUBSCRIBE_DATA"),
      _T("CMD_ZMQ_UNSUBSCRIBE_DATA"),
      _T("CMD_ZMQ_GET_EVT_SUBSCRIPTIONS"),
      _T("CMD_ZMQ_GET_DATA_SUBSCRIPTIONS"),
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
      _T("CMD_HOST_BY_IP"),
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
      _T("CMD_GET_AGENT_POLICY"),
      _T("CMD_POLICY_EDITOR_CLOSED"),
      _T("CMD_POLICY_FORCE_APPLY")
   };

   if ((code >= CMD_LOGIN) && (code <= CMD_POLICY_FORCE_APPLY))
   {
      _tcscpy(pszBuffer, pszMsgNames[code - CMD_LOGIN]);
   }
   else
   {
      bool resolved = false;
      s_resolversLock.lock();
      for(int i = 0; i < s_resolvers.size(); i++)
         if (((NXCPMessageNameResolver)s_resolvers.get(i))(code, pszBuffer))
         {
            resolved = true;
            break;
         }
      s_resolversLock.unlock();
      if (!resolved)
         _sntprintf(pszBuffer, 64, _T("CMD_0x%04X"), code);
   }
   return pszBuffer;
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
 * Init NXCP receiver buffer
 */
void LIBNETXMS_EXPORTABLE NXCPInitBuffer(NXCP_BUFFER *nxcpBuffer)
{
   nxcpBuffer->bufferSize = 0;
   nxcpBuffer->bufferPos = 0;
}

/**
 * Receive raw CSCP message from network
 * If pMsg is NULL, temporary buffer will be re-initialized
 * Returns message size on success or:
 *   0 if connection is closed
 *  <0 on socket errors
 *   1 if message is too large to fit in buffer (normal messages is at least 16
 *     bytes long, so we never get length of 1 for valid message)
 *     In this case, only message header will be copied into buffer
 *   2 Message decryption failed
 *   3 Receive timeout
 */
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(AbstractCommChannel *channel, NXCP_MESSAGE **msgBuffer,
                                           NXCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx,
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
														 UINT32 maxMsgSize)
{
   UINT32 dwMsgSize = 0, dwBytesRead = 0, dwBytesToCopy;
   int iErr;
   BOOL bSkipMsg = FALSE;

   // Initialize buffer if requested
   if (msgBuffer == NULL)
   {
      nxcpBuffer->bufferSize = 0;
      nxcpBuffer->bufferPos = 0;
      return 0;
   }

   // Check if we have something in buffer
   if (nxcpBuffer->bufferSize > 0)
   {
      // Handle the case when entire message header have not been read into the buffer
      if (nxcpBuffer->bufferSize < NXCP_HEADER_SIZE)
      {
         // Most likely we are at the buffer end, so move content
         // to the beginning
         memmove(nxcpBuffer->buffer, &nxcpBuffer->buffer[nxcpBuffer->bufferPos], nxcpBuffer->bufferSize);
         nxcpBuffer->bufferPos = 0;

         // Receive new portion of data from the network
         // and append it to existing data in buffer
			iErr = channel->recv(&nxcpBuffer->buffer[nxcpBuffer->bufferSize],
                       NXCP_TEMP_BUF_SIZE - nxcpBuffer->bufferSize, dwTimeout);
         if (iErr <= 0)
            return (iErr == -2) ? 3 : iErr;
         nxcpBuffer->bufferSize += (UINT32)iErr;
      }

      // Get message size from message header and copy available
      // message bytes from buffer
      dwMsgSize = ntohl(((NXCP_MESSAGE *)(&nxcpBuffer->buffer[nxcpBuffer->bufferPos]))->size);
      if (dwMsgSize > *bufferSize)
      {
			if ((*bufferSize >= maxMsgSize) || (dwMsgSize > maxMsgSize))
			{
				bSkipMsg = TRUE;  // Message is too large, will skip it
				memcpy(*msgBuffer, &nxcpBuffer->buffer[nxcpBuffer->bufferPos], NXCP_HEADER_SIZE);
			}
			else
			{
				// Increase buffer
				*bufferSize = dwMsgSize;
				*msgBuffer = (NXCP_MESSAGE *)realloc(*msgBuffer, *bufferSize);
				if (decryptionBuffer != NULL)
					*decryptionBuffer = (BYTE *)realloc(*decryptionBuffer, *bufferSize);
			}
      }
      dwBytesRead = std::min(dwMsgSize, nxcpBuffer->bufferSize);
      if (!bSkipMsg)
         memcpy(*msgBuffer, &nxcpBuffer->buffer[nxcpBuffer->bufferPos], dwBytesRead);
      nxcpBuffer->bufferSize -= dwBytesRead;
      nxcpBuffer->bufferPos = (nxcpBuffer->bufferSize > 0) ? (nxcpBuffer->bufferPos + dwBytesRead) : 0;
      if (dwBytesRead == dwMsgSize)
         goto decrypt_message;
   }

   // Receive rest of message from the network
	// Buffer is empty now
	nxcpBuffer->bufferSize = 0;
	nxcpBuffer->bufferPos = 0;
   do
   {
		iErr = channel->recv(&nxcpBuffer->buffer[nxcpBuffer->bufferSize],
		                     NXCP_TEMP_BUF_SIZE - nxcpBuffer->bufferSize, dwTimeout);
      if (iErr <= 0)
         return (iErr == -2) ? 3 : iErr;

		if (dwBytesRead == 0) // New message?
      {
			if ((iErr + nxcpBuffer->bufferSize) < NXCP_HEADER_SIZE)
			{
				// Header not received completely
				nxcpBuffer->bufferSize += iErr;
				continue;
			}
			iErr += nxcpBuffer->bufferSize;
			nxcpBuffer->bufferSize = 0;

         dwMsgSize = ntohl(((NXCP_MESSAGE *)(nxcpBuffer->buffer))->size);
         if (dwMsgSize > *bufferSize)
         {
				if ((*bufferSize >= maxMsgSize) || (dwMsgSize > maxMsgSize))
				{
					bSkipMsg = TRUE;  // Message is too large, will skip it
	            memcpy(*msgBuffer, nxcpBuffer->buffer, NXCP_HEADER_SIZE);
				}
				else
				{
					// Increase buffer
					*bufferSize = dwMsgSize;
					*msgBuffer = (NXCP_MESSAGE *)realloc(*msgBuffer, *bufferSize);
					if (decryptionBuffer != NULL)
						*decryptionBuffer = (BYTE *)realloc(*decryptionBuffer, *bufferSize);
				}
         }
      }
      dwBytesToCopy = std::min((UINT32)iErr, dwMsgSize - dwBytesRead);
      if (!bSkipMsg)
         memcpy(((char *)(*msgBuffer)) + dwBytesRead, nxcpBuffer->buffer, dwBytesToCopy);
      dwBytesRead += dwBytesToCopy;
   }
	while((dwBytesRead < dwMsgSize) || (dwBytesRead < NXCP_HEADER_SIZE));

   // Check if we have something left in buffer
   if (dwBytesToCopy < (UINT32)iErr)
   {
      nxcpBuffer->bufferPos = dwBytesToCopy;
      nxcpBuffer->bufferSize = (UINT32)iErr - dwBytesToCopy;
   }

   // Check for encrypted message
decrypt_message:
   if ((!bSkipMsg) && (ntohs((*msgBuffer)->code) == CMD_ENCRYPTED_MESSAGE))
   {
      if ((*ppCtx != NULL) && (*ppCtx != PROXY_ENCRYPTION_CTX))
      {
         if ((*ppCtx)->decryptMessage((NXCP_ENCRYPTED_MESSAGE *)(*msgBuffer), *decryptionBuffer))
         {
            dwMsgSize = ntohl((*msgBuffer)->size);
         }
         else
         {
            dwMsgSize = 2;    // Decryption failed
         }
      }
      else
      {
         if (*ppCtx != PROXY_ENCRYPTION_CTX)
            dwMsgSize = 2;
      }
   }

   return bSkipMsg ? 1 : (int)dwMsgSize;
}

int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, NXCP_MESSAGE **msgBuffer,
                                           NXCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx,
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
                                           UINT32 maxMsgSize)
{
   SocketCommChannel *channel = new SocketCommChannel(hSocket, false);
   int result = RecvNXCPMessageEx(channel, msgBuffer, nxcpBuffer, bufferSize, ppCtx, decryptionBuffer, dwTimeout, maxMsgSize);
   channel->decRefCount();
   return result;
}

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, NXCP_MESSAGE *msgBuffer,
                                         NXCP_BUFFER *nxcpBuffer, UINT32 bufferSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *decryptionBuffer, UINT32 dwTimeout)
{
	NXCP_MESSAGE *mb = msgBuffer;
	UINT32 bs = bufferSize;
	BYTE *db = decryptionBuffer;
	return RecvNXCPMessageEx(hSocket, (msgBuffer != NULL) ? &mb : NULL, nxcpBuffer, &bs, ppCtx,
	                         (decryptionBuffer != NULL) ? &db : NULL, dwTimeout, bufferSize);
}

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(AbstractCommChannel *channel, NXCP_MESSAGE *msgBuffer,
                                         NXCP_BUFFER *nxcpBuffer, UINT32 bufferSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *decryptionBuffer, UINT32 dwTimeout)
{
   NXCP_MESSAGE *mb = msgBuffer;
   UINT32 bs = bufferSize;
   BYTE *db = decryptionBuffer;
   return RecvNXCPMessageEx(channel, (msgBuffer != NULL) ? &mb : NULL, nxcpBuffer, &bs, ppCtx,
                            (decryptionBuffer != NULL) ? &db : NULL, dwTimeout, bufferSize);
}

/**
 * Create NXCP message with raw data (MF_BINARY flag)
 * If buffer is NULL, new buffer is allocated with malloc()
 * Buffer should be at least dataSize + NXCP_HEADER_SIZE + 8 bytes.
 */
NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(UINT16 code, UINT32 id, UINT16 flags,
                                                        const void *data, size_t dataSize,
                                                        NXCP_MESSAGE *buffer, bool allowCompression)
{
   NXCP_MESSAGE *msg = (buffer == NULL) ? (NXCP_MESSAGE *)malloc(dataSize + NXCP_HEADER_SIZE + 8) : buffer;

   // Message should be aligned to 8 bytes boundary
   size_t padding = (8 - ((dataSize + NXCP_HEADER_SIZE) % 8)) & 7;

   msg->code = htons(code);
   msg->flags = htons(MF_BINARY | flags);
   msg->id = htonl(id);
   size_t msgSize = dataSize + NXCP_HEADER_SIZE + padding;
   msg->size = htonl((UINT32)msgSize);
   msg->numFields = htonl((UINT32)dataSize);   // numFields contains actual data size for binary message

   if (allowCompression)
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
   else
   {
      memcpy(msg->fields, data, dataSize);
   }
   return msg;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, UINT32 id, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
                                           void (* progressCallback)(INT64, void *), void *cbArg,
                                           MUTEX mutex, NXCPStreamCompressionMethod compressionMethod,
                                           VolatileCounter *cancellationFlag)
{
   SocketCommChannel *ch = new SocketCommChannel(hSocket, false);
   bool result = SendFileOverNXCP(ch, id, pszFile, pCtx, offset, progressCallback, cbArg, mutex, compressionMethod, cancellationFlag);
   ch->decRefCount();
   return result;
}

/**
 * Send file over NXCP
 */
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, UINT32 id, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex, NXCPStreamCompressionMethod compressionMethod,
														 VolatileCounter *cancellationFlag)
{
   bool success = false;

   int hFile = _topen(pszFile, O_RDONLY | O_BINARY);
   if (hFile != -1)
   {
      NX_STAT_STRUCT st;
      NX_FSTAT(hFile, &st);
      size_t fileSize = st.st_size;
      if (labs(offset) > fileSize)
         offset = 0;
      size_t bytesToRead = (offset < 0) ? (0 - offset) : (fileSize - offset);
      INT64 bytesTransferred = 0;

		if (lseek(hFile, offset, (offset < 0) ? SEEK_END : SEEK_SET) != -1)
		{
         StreamCompressor *compressor = (compressionMethod != NXCP_STREAM_COMPRESSION_NONE) ? StreamCompressor::create(compressionMethod, true, FILE_BUFFER_SIZE) : NULL;
         BYTE *compBuffer = (compressor != NULL) ? (BYTE *)malloc(FILE_BUFFER_SIZE) : NULL;

			// Allocate message and prepare it's header
         NXCP_MESSAGE *msg = (NXCP_MESSAGE *)malloc(NXCP_HEADER_SIZE + 8 + ((compressor != NULL) ? compressor->compressBufferSize(FILE_BUFFER_SIZE) + 4 : FILE_BUFFER_SIZE));
			msg->id = htonl(id);
			msg->code = htons(CMD_FILE_DATA);
         msg->flags = htons(MF_BINARY | MF_STREAM | ((compressionMethod != NXCP_STREAM_COMPRESSION_NONE) ? MF_COMPRESSED : 0));

         size_t bufferSize = FILE_BUFFER_SIZE;
			UINT32 delay = 0;
         int successCount = 0;

			while(true)
			{
			   if ((cancellationFlag != NULL) && (*cancellationFlag > 0))
			      break;

			   int bytes;
            if (compressor != NULL)
            {
               bytes = _read(hFile, compBuffer, std::min(bufferSize, bytesToRead));
				   if (bytes < 0)
					   break;
               bytesToRead -= bytes;
               // Each compressed data block prepended with 4 bytes header
               // First byte contains compression method, second is always 0,
               // third and fourth contains uncompressed block size in network byte order
               *((BYTE *)msg->fields) = (BYTE)compressionMethod;
               *((BYTE *)msg->fields + 1) = 0;
               *((UINT16 *)((BYTE *)msg->fields + 2)) = htons((UINT16)bytes);
               bytes = (int)compressor->compress(compBuffer, bytes, (BYTE *)msg->fields + 4, compressor->compressBufferSize(FILE_BUFFER_SIZE)) + 4;
            }
            else
            {
               bytes = _read(hFile, msg->fields, std::min(bufferSize, bytesToRead));
				   if (bytes < 0)
					   break;
               bytesToRead -= bytes;
            }

				// Message should be aligned to 8 bytes boundary
				UINT32 padding = (8 - (((UINT32)bytes + NXCP_HEADER_SIZE) % 8)) & 7;
				msg->size = htonl(static_cast<UINT32>(bytes) + NXCP_HEADER_SIZE + padding);
				msg->numFields = htonl((UINT32)bytes);   // numFields contains actual data size for binary message
				if (bytesToRead <= 0)
					msg->flags |= htons(MF_END_OF_FILE);

            INT64 startTime = GetCurrentTimeMs();
				if (pCtx != NULL)
				{
               NXCP_ENCRYPTED_MESSAGE *emsg = pCtx->encryptMessage(msg);
					if (emsg != NULL)
					{
					   channel->send(emsg, ntohl(emsg->size), mutex);
						free(emsg);
					}
				}
				else
				{
					if (channel->send(msg, static_cast<UINT32>(bytes) + NXCP_HEADER_SIZE + padding, mutex) <= 0)
						break;	// Send error
				}

            // Throttling
            UINT32 elapsedTime = static_cast<UINT32>(GetCurrentTimeMs() - startTime);
            if ((elapsedTime > 200) && ((bufferSize > 1024) || (delay < 1000)))
            {
               bufferSize = MAX(bufferSize / (elapsedTime / 200), 1024);
               delay = MIN(delay + 50 * (elapsedTime / 200), 1000);
            }
            else if ((elapsedTime < 50) && ((bufferSize < FILE_BUFFER_SIZE) || (delay > 0)))
            {
               successCount++;
               if (successCount > 10)
               {
                  successCount = 0;
                  bufferSize = std::min(bufferSize + bufferSize / 16, FILE_BUFFER_SIZE);
                  if (delay >= 5)
                     delay -= 5;
                  else
                     delay = 0;
               }
            }

				if (progressCallback != NULL)
				{
					bytesTransferred += bytes;
					progressCallback(bytesTransferred, cbArg);
				}

				if (bytesToRead <= 0)
				{
					// End of file
				   success = true;
					break;
				}

            if (delay > 0)
               ThreadSleepMs(delay);
			}

		   free(compBuffer);
		   delete compressor;

			free(msg);
		}
		_close(hFile);
	}

   // If file upload failed, send CMD_ABORT_FILE_TRANSFER
   if (!success)
   {
      NXCP_MESSAGE msg;
      msg.id = htonl(id);
      msg.code = htons(CMD_ABORT_FILE_TRANSFER);
      msg.flags = htons(MF_BINARY);
      msg.numFields = 0;
      msg.size = htonl(NXCP_HEADER_SIZE);
      if (pCtx != NULL)
      {
         NXCP_ENCRYPTED_MESSAGE *emsg = pCtx->encryptMessage(&msg);
         if (emsg != NULL)
         {
            channel->send(emsg, ntohl(emsg->size), mutex);
            free(emsg);
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
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(AbstractCommChannel *channel, int *pnVersion, MUTEX mutex)
{
   NXCP_MESSAGE msg;
   NXCPEncryptionContext *pDummyCtx = NULL;
   NXCP_BUFFER *pBuffer;
   bool success = false;
   int nSize;

   msg.id = 0;
   msg.numFields = 0;
   msg.size = htonl(NXCP_HEADER_SIZE);
   msg.code = htons(CMD_GET_NXCP_CAPS);
   msg.flags = htons(MF_CONTROL);
   if (channel->send(&msg, NXCP_HEADER_SIZE, mutex) == NXCP_HEADER_SIZE)
   {
      pBuffer = (NXCP_BUFFER *)malloc(sizeof(NXCP_BUFFER));
      NXCPInitBuffer(pBuffer);
      nSize = RecvNXCPMessage(channel, &msg, pBuffer, NXCP_HEADER_SIZE, &pDummyCtx, NULL, 30000);
      if ((nSize == NXCP_HEADER_SIZE) &&
          (ntohs(msg.code) == CMD_NXCP_CAPS) &&
          (ntohs(msg.flags) & MF_CONTROL))
      {
         success = true;
         *pnVersion = ntohl(msg.numFields) >> 24;
      }
      else if ((nSize == 1) || (nSize == 3) || (nSize >= NXCP_HEADER_SIZE))
      {
         // We don't receive any answer or receive invalid answer -
         // assume that peer doesn't understand CMD_GET_NXCP_CAPS message
         // and set version number to 1
         success = true;
         *pnVersion = 1;
      }
      free(pBuffer);
   }
   return success;
}

/**
 * Get version of NXCP used by peer
 */
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET s, int *pnVersion, MUTEX mutex)
{
   SocketCommChannel *channel = new SocketCommChannel(s, false);
   bool success = NXCPGetPeerProtocolVersion(channel, pnVersion, mutex);
   channel->decRefCount();
   return success;
}
