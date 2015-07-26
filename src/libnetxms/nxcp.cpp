/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
#include <nxstat.h>

#ifdef _WIN32
#define read	_read
#define close	_close
#endif

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
      _T("CMD_GET_EVENTS"),
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
      _T("CMD_GET_CONTAINER_CAT_LIST"),
      _T("CMD_CONTAINER_CAT_DATA"),
      _T("CMD_DELETE_CONTAINER_CAT"),
      _T("CMD_CREATE_CONTAINER_CAT"),
      _T("CMD_MODIFY_CONTAINER_CAT"),
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
      _T("CMD_LOCK_PACKAGE_DB"),
      _T("CMD_UNLOCK_PACKAGE_DB"),
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
      _T("CMD_GET_SYSLOG"),
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
      _T("CMD_GET_TRAP_LOG"),
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
		_T("CMD_GET_SITUATION_LIST"),
		_T("CMD_DELETE_SITUATION"),
		_T("CMD_CREATE_SITUATION"),
		_T("CMD_DEL_SITUATION_INSTANCE"),
		_T("CMD_UPDATE_SITUATION"),
		_T("CMD_SITUATION_DATA"),
		_T("CMD_SITUATION_CHANGE"),
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
      _T("CMD_ENABLE_IPV6"),
      _T("CMD_FORCE_DCI_POLL"),
      _T("CMD_GET_DCI_SCRIPT_LIST"),
      _T("CMD_DATA_COLLECTION_CONFIG"),
      _T("CMD_SET_SERVER_ID"),
      _T("CMD_GET_PUBLIC_CONFIG_VAR"),
      _T("CMD_ENABLE_FILE_UPDATES"),
      _T("CMD_DETACH_LDAP_USER"),
      _T("CMD_VALIDATE_PASSWORD")
   };

   if ((code >= CMD_LOGIN) && (code <= CMD_VALIDATE_PASSWORD))
      _tcscpy(pszBuffer, pszMsgNames[code - CMD_LOGIN]);
   else
      _sntprintf(pszBuffer, 64, _T("CMD_0x%04X"), code);
   return pszBuffer;
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
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, NXCP_MESSAGE **msgBuffer,
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
			iErr = RecvEx(hSocket, &nxcpBuffer->buffer[nxcpBuffer->bufferSize],
                       NXCP_TEMP_BUF_SIZE - nxcpBuffer->bufferSize, 0, dwTimeout);
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
      dwBytesRead = min(dwMsgSize, nxcpBuffer->bufferSize);
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
		iErr = RecvEx(hSocket, &nxcpBuffer->buffer[nxcpBuffer->bufferSize],
		              NXCP_TEMP_BUF_SIZE - nxcpBuffer->bufferSize, 0, dwTimeout);
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
      dwBytesToCopy = min((UINT32)iErr, dwMsgSize - dwBytesRead);
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

/**
 * Create NXCP message with raw data (MF_BINARY flag)
 * If pBuffer is NULL, new buffer is allocated with malloc()
 * Buffer should be of dwDataSize + NXCP_HEADER_SIZE + 8 bytes.
 */
NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(WORD code, UINT32 id, WORD flags,
                                                        UINT32 dwDataSize, void *pData,
                                                        NXCP_MESSAGE *pBuffer)
{
   NXCP_MESSAGE *pMsg;
   UINT32 dwPadding;

   if (pBuffer == NULL)
      pMsg = (NXCP_MESSAGE *)malloc(dwDataSize + NXCP_HEADER_SIZE + 8);
   else
      pMsg = pBuffer;

   // Message should be aligned to 8 bytes boundary
   dwPadding = (8 - ((dwDataSize + NXCP_HEADER_SIZE) % 8)) & 7;

   pMsg->code = htons(code);
   pMsg->flags = htons(MF_BINARY | flags);
   pMsg->id = htonl(id);
   pMsg->size = htonl(dwDataSize + NXCP_HEADER_SIZE + dwPadding);
   pMsg->numFields = htonl(dwDataSize);   // numFields contains actual data size for binary message
   memcpy(pMsg->fields, pData, dwDataSize);

   return pMsg;
}

/**
 * Send file over CSCP
 */
BOOL LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, UINT32 id, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex, NXCPCompressionMethod compressionMethod)
{
   int hFile, iBytes;
	INT64 bytesTransferred = 0;
   UINT32 dwPadding;
   BOOL bResult = FALSE;
   NXCP_MESSAGE *pMsg;
   NXCP_ENCRYPTED_MESSAGE *pEnMsg;

   StreamCompressor *compressor = (compressionMethod != NXCP_COMPRESSION_NONE) ? StreamCompressor::create(compressionMethod, true, FILE_BUFFER_SIZE) : NULL;
   BYTE *compBuffer = (compressor != NULL) ? (BYTE *)malloc(FILE_BUFFER_SIZE) : NULL;

   hFile = _topen(pszFile, O_RDONLY | O_BINARY);
   if (hFile != -1)
   {
      NX_STAT_STRUCT st;
      NX_FSTAT(hFile, &st);
      long fileSize = (long)st.st_size;
      if (labs(offset) > fileSize)
         offset = 0;
      long bytesToRead = (offset < 0) ? (0 - offset) : (fileSize - offset);

		if (lseek(hFile, offset, (offset < 0) ? SEEK_END : SEEK_SET) != -1)
		{
			// Allocate message and prepare it's header
         pMsg = (NXCP_MESSAGE *)malloc(NXCP_HEADER_SIZE + 8 + ((compressor != NULL) ? compressor->compressBufferSize(FILE_BUFFER_SIZE) + 4 : FILE_BUFFER_SIZE));
			pMsg->id = htonl(id);
			pMsg->code = htons(CMD_FILE_DATA);
         pMsg->flags = htons(MF_BINARY | ((compressionMethod != NXCP_COMPRESSION_NONE) ? MF_COMPRESSED : 0));

			while(1)
			{
            if (compressor != NULL)
            {
				   iBytes = read(hFile, compBuffer, min(FILE_BUFFER_SIZE, bytesToRead));
				   if (iBytes < 0)
					   break;
               bytesToRead -= iBytes;
               // Each compressed data block prepended with 4 bytes header
               // First byte contains compression method, second is always 0,
               // third and fourth contains uncompressed block size in network byte order
               *((BYTE *)pMsg->fields) = (BYTE)compressionMethod;
               *((BYTE *)pMsg->fields + 1) = 0;
               *((UINT16 *)((BYTE *)pMsg->fields + 2)) = htons((UINT16)iBytes);
               iBytes = (int)compressor->compress(compBuffer, iBytes, (BYTE *)pMsg->fields + 4, compressor->compressBufferSize(FILE_BUFFER_SIZE)) + 4;
            }
            else
            {
				   iBytes = read(hFile, pMsg->fields, min(FILE_BUFFER_SIZE, bytesToRead));
				   if (iBytes < 0)
					   break;
               bytesToRead -= iBytes;
            }

				// Message should be aligned to 8 bytes boundary
				dwPadding = (8 - (((UINT32)iBytes + NXCP_HEADER_SIZE) % 8)) & 7;
				pMsg->size = htonl((UINT32)iBytes + NXCP_HEADER_SIZE + dwPadding);
				pMsg->numFields = htonl((UINT32)iBytes);   // numFields contains actual data size for binary message
				if (bytesToRead <= 0)
					pMsg->flags |= htons(MF_END_OF_FILE);

				if (pCtx != NULL)
				{
               pEnMsg = pCtx->encryptMessage(pMsg);
					if (pEnMsg != NULL)
					{
						SendEx(hSocket, (char *)pEnMsg, ntohl(pEnMsg->size), 0, mutex);
						free(pEnMsg);
					}
				}
				else
				{
					if (SendEx(hSocket, (char *)pMsg, (UINT32)iBytes + NXCP_HEADER_SIZE + dwPadding, 0, mutex) <= 0)
						break;	// Send error
				}
				if (progressCallback != NULL)
				{
					bytesTransferred += iBytes;
					progressCallback(bytesTransferred, cbArg);
				}

				if (bytesToRead <= 0)
				{
					// End of file
					bResult = TRUE;
					break;
				}
			}

			free(pMsg);
		}
		close(hFile);
	}

   safe_free(compBuffer);
   delete compressor;

   // If file upload failed, send CMD_ABORT_FILE_TRANSFER
   if (!bResult)
   {
      NXCP_MESSAGE msg;

      msg.id = htonl(id);
      msg.code = htons(CMD_ABORT_FILE_TRANSFER);
      msg.flags = htons(MF_BINARY);
      msg.numFields = 0;
      msg.size = htonl(NXCP_HEADER_SIZE);
      if (pCtx != NULL)
      {
         pEnMsg = pCtx->encryptMessage(&msg);
         if (pEnMsg != NULL)
         {
            SendEx(hSocket, (char *)pEnMsg, ntohl(pEnMsg->size), 0, mutex);
            free(pEnMsg);
         }
      }
      else
      {
         SendEx(hSocket, (char *)&msg, NXCP_HEADER_SIZE, 0, mutex);
      }
   }

   return bResult;
}

/**
 * Get version of NXCP used by peer
 */
BOOL LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET hSocket, int *pnVersion, MUTEX mutex)
{
   NXCP_MESSAGE msg;
   NXCPEncryptionContext *pDummyCtx = NULL;
   NXCP_BUFFER *pBuffer;
   BOOL bRet = FALSE;
   int nSize;

   msg.id = 0;
   msg.numFields = 0;
   msg.size = htonl(NXCP_HEADER_SIZE);
   msg.code = htons(CMD_GET_NXCP_CAPS);
   msg.flags = htons(MF_CONTROL);
   if (SendEx(hSocket, &msg, NXCP_HEADER_SIZE, 0, mutex) == NXCP_HEADER_SIZE)
   {
      pBuffer = (NXCP_BUFFER *)malloc(sizeof(NXCP_BUFFER));
      RecvNXCPMessage(0, NULL, pBuffer, 0, NULL, NULL, 0);
      nSize = RecvNXCPMessage(hSocket, &msg, pBuffer, NXCP_HEADER_SIZE, &pDummyCtx, NULL, 30000);
      if ((nSize == NXCP_HEADER_SIZE) &&
          (ntohs(msg.code) == CMD_NXCP_CAPS) &&
          (ntohs(msg.flags) & MF_CONTROL))
      {
         bRet = TRUE;
         *pnVersion = ntohl(msg.numFields) >> 24;
      }
      else if ((nSize == 1) || (nSize == 3) || (nSize >= NXCP_HEADER_SIZE))
      {
         // We don't receive any answer or receive invalid answer -
         // assume that peer doesn't understand CMD_GET_NXCP_CAPS message
         // and set version number to 1
         bRet = TRUE;
         *pnVersion = 1;
      }
      free(pBuffer);
   }
   return bRet;
}
