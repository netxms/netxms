/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#ifdef _WIN32
#define read	_read
#define close	_close
#endif

/**
 * Constants
 */
#define FILE_BUFFER_SIZE      32768

/**
 * Get symbolic name for message code
 */
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(WORD wCode, TCHAR *pszBuffer)
{
   static const TCHAR *pszMsgNames[] =
   {
      _T("CMD_LOGIN"),
      _T("CMD_LOGIN_RESP"),
      _T("CMD_KEEPALIVE"),
      _T("CMD_SET_ALARM_HD_STATE"),
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
      _T("CMD_GET_ALARM_NOTES"),
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
		_T("CMD_DEFINE_GRAPH"),
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
		_T("CMD_UPLOAD_FILE_TO_AGENT"),
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
		_T("CMD_UPDATE_ALARM_NOTE"),
		_T("CMD_GET_ALARM"),
		_T("CMD_GET_TABLE_LAST_VALUES"),
		_T("CMD_GET_TABLE_DCI_DATA"),
		_T("CMD_GET_THRESHOLD_SUMMARY"),
		_T("CMD_RESOLVE_ALARM"),
		_T("CMD_FIND_IP_LOCATION"),
		_T("CMD_REPORT_DEVICE_STATUS"),
		_T("CMD_REPORT_DEVICE_INFO"),
		_T("CMD_GET_ALARM_EVENTS")
   };

   if ((wCode >= CMD_LOGIN) && (wCode <= CMD_GET_ALARM_EVENTS))
      _tcscpy(pszBuffer, pszMsgNames[wCode - CMD_LOGIN]);
   else
      _sntprintf(pszBuffer, 64, _T("CMD_0x%04X"), wCode);
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
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, CSCP_MESSAGE **msgBuffer,
                                           CSCP_BUFFER *nxcpBuffer, DWORD *bufferSize,
                                           NXCPEncryptionContext **ppCtx, 
                                           BYTE **decryptionBuffer, DWORD dwTimeout,
														 DWORD maxMsgSize)
{
   DWORD dwMsgSize = 0, dwBytesRead = 0, dwBytesToCopy;
   int iErr;
   BOOL bSkipMsg = FALSE;

   // Initialize buffer if requested
   if (msgBuffer == NULL)
   {
      nxcpBuffer->dwBufSize = 0;
      nxcpBuffer->dwBufPos = 0;
      return 0;
   }

   // Check if we have something in buffer
   if (nxcpBuffer->dwBufSize > 0)
   {
      // Handle the case when entire message header have not been read into the buffer
      if (nxcpBuffer->dwBufSize < CSCP_HEADER_SIZE)
      {
         // Most likely we are at the buffer end, so move content
         // to the beginning
         memmove(nxcpBuffer->szBuffer, &nxcpBuffer->szBuffer[nxcpBuffer->dwBufPos], nxcpBuffer->dwBufSize);
         nxcpBuffer->dwBufPos = 0;

         // Receive new portion of data from the network 
         // and append it to existing data in buffer
			iErr = RecvEx(hSocket, &nxcpBuffer->szBuffer[nxcpBuffer->dwBufSize],
                       CSCP_TEMP_BUF_SIZE - nxcpBuffer->dwBufSize, 0, dwTimeout);
         if (iErr <= 0)
            return (iErr == -2) ? 3 : iErr;
         nxcpBuffer->dwBufSize += (DWORD)iErr;
      }

      // Get message size from message header and copy available 
      // message bytes from buffer
      dwMsgSize = ntohl(((CSCP_MESSAGE *)(&nxcpBuffer->szBuffer[nxcpBuffer->dwBufPos]))->dwSize);
      if (dwMsgSize > *bufferSize)
      {
			if ((*bufferSize >= maxMsgSize) || (dwMsgSize > maxMsgSize))
			{
				bSkipMsg = TRUE;  // Message is too large, will skip it
				memcpy(*msgBuffer, &nxcpBuffer->szBuffer[nxcpBuffer->dwBufPos], CSCP_HEADER_SIZE);
			}
			else
			{
				// Increase buffer
				*bufferSize = dwMsgSize;
				*msgBuffer = (CSCP_MESSAGE *)realloc(*msgBuffer, *bufferSize);
				if (decryptionBuffer != NULL)
					*decryptionBuffer = (BYTE *)realloc(*decryptionBuffer, *bufferSize);
			}
      }
      dwBytesRead = min(dwMsgSize, nxcpBuffer->dwBufSize);
      if (!bSkipMsg)
         memcpy(*msgBuffer, &nxcpBuffer->szBuffer[nxcpBuffer->dwBufPos], dwBytesRead);
      nxcpBuffer->dwBufSize -= dwBytesRead;
      nxcpBuffer->dwBufPos = (nxcpBuffer->dwBufSize > 0) ? (nxcpBuffer->dwBufPos + dwBytesRead) : 0;
      if (dwBytesRead == dwMsgSize)
         goto decrypt_message;
   }

   // Receive rest of message from the network
	// Buffer is empty now
	nxcpBuffer->dwBufSize = 0;
	nxcpBuffer->dwBufPos = 0;
   do
   {
		iErr = RecvEx(hSocket, &nxcpBuffer->szBuffer[nxcpBuffer->dwBufSize], 
		              CSCP_TEMP_BUF_SIZE - nxcpBuffer->dwBufSize, 0, dwTimeout);
      if (iErr <= 0)
         return (iErr == -2) ? 3 : iErr;

		if (dwBytesRead == 0) // New message?
      {
			if ((iErr + nxcpBuffer->dwBufSize) < CSCP_HEADER_SIZE)
			{
				// Header not received completely
				nxcpBuffer->dwBufSize += iErr;
				continue;
			}
			iErr += nxcpBuffer->dwBufSize;
			nxcpBuffer->dwBufSize = 0;

         dwMsgSize = ntohl(((CSCP_MESSAGE *)(nxcpBuffer->szBuffer))->dwSize);
         if (dwMsgSize > *bufferSize)
         {
				if ((*bufferSize >= maxMsgSize) || (dwMsgSize > maxMsgSize))
				{
					bSkipMsg = TRUE;  // Message is too large, will skip it
	            memcpy(*msgBuffer, nxcpBuffer->szBuffer, CSCP_HEADER_SIZE);
				}
				else
				{
					// Increase buffer
					*bufferSize = dwMsgSize;
					*msgBuffer = (CSCP_MESSAGE *)realloc(*msgBuffer, *bufferSize);
					if (decryptionBuffer != NULL)
						*decryptionBuffer = (BYTE *)realloc(*decryptionBuffer, *bufferSize);
				}
         }
      }
      dwBytesToCopy = min((DWORD)iErr, dwMsgSize - dwBytesRead);
      if (!bSkipMsg)
         memcpy(((char *)(*msgBuffer)) + dwBytesRead, nxcpBuffer->szBuffer, dwBytesToCopy);
      dwBytesRead += dwBytesToCopy;
   }
	while((dwBytesRead < dwMsgSize) || (dwBytesRead < CSCP_HEADER_SIZE));
   
   // Check if we have something left in buffer
   if (dwBytesToCopy < (DWORD)iErr)
   {
      nxcpBuffer->dwBufPos = dwBytesToCopy;
      nxcpBuffer->dwBufSize = (DWORD)iErr - dwBytesToCopy;
   }

   // Check for encrypted message
decrypt_message:
   if ((!bSkipMsg) && (ntohs((*msgBuffer)->wCode) == CMD_ENCRYPTED_MESSAGE))
   {
      if ((*ppCtx != NULL) && (*ppCtx != PROXY_ENCRYPTION_CTX))
      {
         if (CSCPDecryptMessage(*ppCtx, (CSCP_ENCRYPTED_MESSAGE *)(*msgBuffer), *decryptionBuffer))
         {
            dwMsgSize = ntohl((*msgBuffer)->dwSize);
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

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, CSCP_MESSAGE *msgBuffer,
                                         CSCP_BUFFER *nxcpBuffer, DWORD bufferSize,
                                         NXCPEncryptionContext **ppCtx, 
                                         BYTE *decryptionBuffer, DWORD dwTimeout)
{
	CSCP_MESSAGE *mb = msgBuffer;
	DWORD bs = bufferSize;
	BYTE *db = decryptionBuffer;
	return RecvNXCPMessageEx(hSocket, (msgBuffer != NULL) ? &mb : NULL, nxcpBuffer, &bs, ppCtx,
	                         (decryptionBuffer != NULL) ? &db : NULL, dwTimeout, bufferSize);
}


//
// Create CSCP message with raw data (MF_BINARY flag)
// If pBuffer is NULL, new buffer is allocated with malloc()
// Buffer should be of dwDataSize + CSCP_HEADER_SIZE + 8 bytes.
//

CSCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(WORD wCode, DWORD dwId, WORD wFlags,
                                                        DWORD dwDataSize, void *pData, 
                                                        CSCP_MESSAGE *pBuffer)
{
   CSCP_MESSAGE *pMsg;
   DWORD dwPadding;

   if (pBuffer == NULL)
      pMsg = (CSCP_MESSAGE *)malloc(dwDataSize + CSCP_HEADER_SIZE + 8);
   else
      pMsg = pBuffer;

   // Message should be aligned to 8 bytes boundary
   dwPadding = (8 - ((dwDataSize + CSCP_HEADER_SIZE) % 8)) & 7;

   pMsg->wCode = htons(wCode);
   pMsg->wFlags = htons(MF_BINARY | wFlags);
   pMsg->dwId = htonl(dwId);
   pMsg->dwSize = htonl(dwDataSize + CSCP_HEADER_SIZE + dwPadding);
   pMsg->dwNumVars = htonl(dwDataSize);   // dwNumVars contains actual data size for binary message
   memcpy(pMsg->df, pData, dwDataSize);

   return pMsg;
}


//
// Send file over CSCP
//

BOOL LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, DWORD dwId, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex)
{
#ifndef UNDER_CE
   int hFile, iBytes;
#else
   FILE *hFile;
   int iBytes;
#endif
	INT64 bytesTransferred = 0;
   DWORD dwPadding;
   BOOL bResult = FALSE;
   CSCP_MESSAGE *pMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;

#ifndef UNDER_CE
   hFile = _topen(pszFile, O_RDONLY | O_BINARY);
   if (hFile != -1)
   {
		if (lseek(hFile, offset, offset < 0 ? SEEK_END : SEEK_SET) != -1)
		{
#else
   hFile = _tfopen(pszFile, _T("rb"));
   if (hFile != NULL)
   {
	   if (fseek(hFile, offset, offset < 0 ? SEEK_END : SEEK_SET) == 0)
	   {
#endif
			// Allocate message and prepare it's header
			pMsg = (CSCP_MESSAGE *)malloc(FILE_BUFFER_SIZE + CSCP_HEADER_SIZE + 8);
			pMsg->dwId = htonl(dwId);
			pMsg->wCode = htons(CMD_FILE_DATA);
			pMsg->wFlags = htons(MF_BINARY);

			while(1)
			{
#ifndef UNDER_CE
				iBytes = read(hFile, pMsg->df, FILE_BUFFER_SIZE);
				if (iBytes < 0)
					break;
#else
				iBytes = fread(pMsg->df, 1, FILE_BUFFER_SIZE, hFile);
				if (ferror(hFile))
					break;	// Read error
#endif

				// Message should be aligned to 8 bytes boundary
				dwPadding = (8 - (((DWORD)iBytes + CSCP_HEADER_SIZE) % 8)) & 7;
				pMsg->dwSize = htonl((DWORD)iBytes + CSCP_HEADER_SIZE + dwPadding);
				pMsg->dwNumVars = htonl((DWORD)iBytes);   // dwNumVars contains actual data size for binary message
				if (iBytes < FILE_BUFFER_SIZE)
					pMsg->wFlags |= htons(MF_END_OF_FILE);

				if (pCtx != NULL)
				{
					pEnMsg = CSCPEncryptMessage(pCtx, pMsg);
					if (pEnMsg != NULL)
					{
						SendEx(hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, mutex);
						free(pEnMsg);
					}
				}
				else
				{
					if (SendEx(hSocket, (char *)pMsg, (DWORD)iBytes + CSCP_HEADER_SIZE + dwPadding, 0, mutex) <= 0)
						break;	// Send error
				}

				if (progressCallback != NULL)
				{
					bytesTransferred += iBytes;
					progressCallback(bytesTransferred, cbArg);
				}

				if (iBytes < FILE_BUFFER_SIZE)
				{
					// End of file
					bResult = TRUE;
					break;
				}
			}

			free(pMsg);
		}
#ifndef UNDER_CE
		close(hFile);
#else
		fclose(hFile);
#endif
	}

   // If file upload failed, send CMD_ABORT_FILE_TRANSFER
   if (!bResult)
   {
      CSCP_MESSAGE msg;

      msg.dwId = htonl(dwId);
      msg.wCode = htons(CMD_ABORT_FILE_TRANSFER);
      msg.wFlags = htons(MF_BINARY);
      msg.dwNumVars = 0;
      msg.dwSize = htonl(CSCP_HEADER_SIZE);
      if (pCtx != NULL)
      {
         pEnMsg = CSCPEncryptMessage(pCtx, &msg);
         if (pEnMsg != NULL)
         {
            SendEx(hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, mutex);
            free(pEnMsg);
         }
      }
      else
      {
         SendEx(hSocket, (char *)&msg, CSCP_HEADER_SIZE, 0, mutex);
      }
   }

   return bResult;
}


//
// Get version of NXCP used by peer
//

BOOL LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET hSocket, int *pnVersion, MUTEX mutex)
{
   CSCP_MESSAGE msg;
   NXCPEncryptionContext *pDummyCtx = NULL;
   CSCP_BUFFER *pBuffer;
   BOOL bRet = FALSE;
   int nSize;

   msg.dwId = 0;
   msg.dwNumVars = 0;
   msg.dwSize = htonl(CSCP_HEADER_SIZE);
   msg.wCode = htons(CMD_GET_NXCP_CAPS);
   msg.wFlags = htons(MF_CONTROL);
   if (SendEx(hSocket, &msg, CSCP_HEADER_SIZE, 0, mutex) == CSCP_HEADER_SIZE)
   {
      pBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
      RecvNXCPMessage(0, NULL, pBuffer, 0, NULL, NULL, 0);
      nSize = RecvNXCPMessage(hSocket, &msg, pBuffer, CSCP_HEADER_SIZE, &pDummyCtx, NULL, 30000);
      if ((nSize == CSCP_HEADER_SIZE) &&
          (ntohs(msg.wCode) == CMD_NXCP_CAPS) &&
          (ntohs(msg.wFlags) & MF_CONTROL))
      {
         bRet = TRUE;
         *pnVersion = ntohl(msg.dwNumVars) >> 24;
      }
      else if ((nSize == 1) || (nSize == 3) || (nSize >= CSCP_HEADER_SIZE))
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
