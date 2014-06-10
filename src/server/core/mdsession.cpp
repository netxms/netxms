/*
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: mdsession.cpp
**
**/

#include "nxcore.h"

#ifdef _WIN32
#include <psapi.h>
#define write	_write
#define close	_close
#endif

// WARNING! this hack works only for d2i_X509(); be carefull when adding new code
#ifdef OPENSSL_CONST
# undef OPENSSL_CONST
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST
#endif

/**
 * Constants
 */
#define MAX_MSG_SIZE    65536

/**
 * Externals
 */
void UnregisterMobileDeviceSession(UINT32 dwIndex);

/**
 * Client communication read thread starter
 */
THREAD_RESULT THREAD_CALL MobileDeviceSession::readThreadStarter(void *pArg)
{
   ((MobileDeviceSession *)pArg)->readThread();

   // When MobileDeviceSession::readThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterMobileDeviceSession(((MobileDeviceSession *)pArg)->getIndex());
   delete (MobileDeviceSession *)pArg;
   return THREAD_OK;
}

/**
 * Client communication write thread starter
 */
THREAD_RESULT THREAD_CALL MobileDeviceSession::writeThreadStarter(void *pArg)
{
   ((MobileDeviceSession *)pArg)->writeThread();
   return THREAD_OK;
}

/**
 * Received message processing thread starter
 */
THREAD_RESULT THREAD_CALL MobileDeviceSession::processingThreadStarter(void *pArg)
{
   ((MobileDeviceSession *)pArg)->processingThread();
   return THREAD_OK;
}

/**
 * Mobile device session class constructor
 */
MobileDeviceSession::MobileDeviceSession(SOCKET hSocket, struct sockaddr *addr)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_hSocket = hSocket;
   m_dwIndex = INVALID_INDEX;
   m_iState = SESSION_STATE_INIT;
   m_pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   m_pCtx = NULL;
   m_hWriteThread = INVALID_THREAD_HANDLE;
   m_hProcessingThread = INVALID_THREAD_HANDLE;
	m_mutexSocketWrite = MutexCreate();
	m_clientAddr = (struct sockaddr *)nx_memdup(addr, (addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
	if (addr->sa_family == AF_INET)
		IpToStr(ntohl(((struct sockaddr_in *)m_clientAddr)->sin_addr.s_addr), m_szHostName);
#ifdef WITH_IPV6
	else
		Ip6ToStr(((struct sockaddr_in6 *)m_clientAddr)->sin6_addr.s6_addr, m_szHostName);
#endif
   _tcscpy(m_szUserName, _T("<not logged in>"));
	_tcscpy(m_szClientInfo, _T("n/a"));
   m_dwUserId = INVALID_INDEX;
	m_deviceObjectId = 0;
   m_dwEncryptionRqId = 0;
   m_condEncryptionSetup = INVALID_CONDITION_HANDLE;
   m_dwRefCount = 0;
	m_isAuthenticated = false;
}

/**
 * Destructor
 */
MobileDeviceSession::~MobileDeviceSession()
{
   if (m_hSocket != -1)
      closesocket(m_hSocket);
   delete m_pSendQueue;
   delete m_pMessageQueue;
   safe_free(m_pMsgBuffer);
	safe_free(m_clientAddr);
	MutexDestroy(m_mutexSocketWrite);
	if (m_pCtx != NULL)
		m_pCtx->decRefCount();
   if (m_condEncryptionSetup != INVALID_CONDITION_HANDLE)
      ConditionDestroy(m_condEncryptionSetup);
}

/**
 * Start all threads
 */
void MobileDeviceSession::run()
{
   m_hWriteThread = ThreadCreateEx(writeThreadStarter, 0, this);
   m_hProcessingThread = ThreadCreateEx(processingThreadStarter, 0, this);
   ThreadCreate(readThreadStarter, 0, this);
}

/**
 * Print debug information
 */
void MobileDeviceSession::debugPrintf(int level, const TCHAR *format, ...)
{
   if (level <= (int)g_debugLevel)
   {
      va_list args;
		TCHAR buffer[4096];

      va_start(args, format);
      _vsntprintf(buffer, 4096, format, args);
      va_end(args);
		DbgPrintf(level, _T("[MDSN-%d] %s"), m_dwIndex, buffer);
   }
}

/**
 * Read thread
 */
void MobileDeviceSession::readThread()
{
	UINT32 msgBufferSize = 1024;
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   BYTE *pDecryptionBuffer = NULL;
   TCHAR szBuffer[256];
   int iErr;

   // Initialize raw message receiving function
   RecvNXCPMessage(0, NULL, m_pMsgBuffer, 0, NULL, NULL, 0);

   pRawMsg = (CSCP_MESSAGE *)malloc(msgBufferSize);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(msgBufferSize);
#endif
   while(1)
   {
		// Shrink buffer after receiving large message
		if (msgBufferSize > 131072)
		{
			msgBufferSize = 131072;
		   pRawMsg = (CSCP_MESSAGE *)realloc(pRawMsg, msgBufferSize);
			if (pDecryptionBuffer != NULL)
			   pDecryptionBuffer = (BYTE *)realloc(pDecryptionBuffer, msgBufferSize);
		}

      if ((iErr = RecvNXCPMessageEx(m_hSocket, &pRawMsg, m_pMsgBuffer, &msgBufferSize,
		                              &m_pCtx, (pDecryptionBuffer != NULL) ? &pDecryptionBuffer : NULL,
												900000, MAX_MSG_SIZE)) <= 0)  // timeout 15 minutes
		{
         debugPrintf(5, _T("RecvNXCPMessageEx failed (%d)"), iErr);
         break;
      }

      // Receive timeout
      if (iErr == 3)
      {
         debugPrintf(5, _T("RecvNXCPMessageEx: receive timeout"));
         break;
      }

      // Check if message is too large
      if (iErr == 1)
      {
         debugPrintf(4, _T("Received message %s is too large (%d bytes)"),
                     NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                     ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check for decryption error
      if (iErr == 2)
      {
         debugPrintf(4, _T("Unable to decrypt received message"));
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         debugPrintf(4, _T("Actual message size doesn't match wSize value (%d,%d)"), iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      if (g_debugLevel >= 8)
      {
         String msgDump = CSCPMessage::dump(pRawMsg, NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }

      WORD wFlags = ntohs(pRawMsg->wFlags);
      if (!(wFlags & MF_BINARY))
      {
         // Create message object from raw message
         pMsg = new CSCPMessage(pRawMsg);
         if ((pMsg->GetCode() == CMD_SESSION_KEY) && (pMsg->GetId() == m_dwEncryptionRqId))
         {
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(pMsg->GetCode(), szBuffer));
            m_dwEncryptionResult = SetupEncryptionContext(pMsg, &m_pCtx, NULL, g_pServerKey, NXCP_VERSION);
            ConditionSet(m_condEncryptionSetup);
            m_dwEncryptionRqId = 0;
            delete pMsg;
         }
         else if (pMsg->GetCode() == CMD_KEEPALIVE)
			{
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(pMsg->GetCode(), szBuffer));
				respondToKeepalive(pMsg->GetId());
				delete pMsg;
			}
			else
         {
            m_pMessageQueue->Put(pMsg);
         }
      }
   }
   if (iErr < 0)
      nxlog_write(MSG_MD_SESSION_CLOSED, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
   free(pRawMsg);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif

   // Notify other threads to exit
	while((pRawMsg = (CSCP_MESSAGE *)m_pSendQueue->Get()) != NULL)
		free(pRawMsg);
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
	while((pMsg = (CSCPMessage *)m_pMessageQueue->Get()) != NULL)
		delete pMsg;
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   ThreadJoin(m_hWriteThread);
   ThreadJoin(m_hProcessingThread);

   // Waiting while reference count becomes 0
   if (m_dwRefCount > 0)
   {
      debugPrintf(3, _T("Waiting for pending requests..."));
      do
      {
         ThreadSleep(1);
      } while(m_dwRefCount > 0);
   }

	WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_szHostName, 0, _T("Mobile device logged out (client: %s)"), m_szClientInfo);
   debugPrintf(3, _T("Session closed"));
}

/**
 * Network write thread
 */
void MobileDeviceSession::writeThread()
{
   CSCP_MESSAGE *pRawMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   TCHAR szBuffer[128];
   BOOL bResult;

   while(1)
   {
      pRawMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pRawMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

		if (ntohs(pRawMsg->wCode) != CMD_ADM_MESSAGE)
			debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer));

      if (m_pCtx != NULL)
      {
         pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
         if (pEnMsg != NULL)
         {
            bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
            free(pEnMsg);
         }
         else
         {
            bResult = FALSE;
         }
      }
      else
      {
         bResult = (SendEx(m_hSocket, (const char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pRawMsg->dwSize));
      }
      free(pRawMsg);

      if (!bResult)
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
         break;
      }
   }
}

/**
 * Message processing thread
 */
void MobileDeviceSession::processingThread()
{
   CSCPMessage *pMsg;
   TCHAR szBuffer[128];
   UINT32 i;
	int status;

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      m_wCurrentCmd = pMsg->GetCode();
      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(m_wCurrentCmd, szBuffer));
      if (!m_isAuthenticated &&
          (m_wCurrentCmd != CMD_LOGIN) &&
			 (m_wCurrentCmd != CMD_GET_SERVER_INFO) &&
          (m_wCurrentCmd != CMD_REQUEST_ENCRYPTION))
      {
         delete pMsg;
         continue;
      }

      m_iState = SESSION_STATE_PROCESSING;
      switch(m_wCurrentCmd)
      {
         case CMD_GET_SERVER_INFO:
            sendServerInfo(pMsg->GetId());
            break;
         case CMD_LOGIN:
            login(pMsg);
            break;
         case CMD_REQUEST_ENCRYPTION:
            setupEncryption(pMsg);
            break;
			case CMD_REPORT_DEVICE_INFO:
				updateDeviceInfo(pMsg);
				break;
			case CMD_REPORT_DEVICE_STATUS:
				updateDeviceStatus(pMsg);
				break;
         case CMD_PUSH_DCI_DATA:
            pushData(pMsg);
            break;
         default:
            // Pass message to loaded modules
            for(i = 0; i < g_dwNumModules; i++)
				{
					if (g_pModuleList[i].pfMobileDeviceCommandHandler != NULL)
					{
						status = g_pModuleList[i].pfMobileDeviceCommandHandler(m_wCurrentCmd, pMsg, this);
						if (status != NXMOD_COMMAND_IGNORED)
						{
							if (status == NXMOD_COMMAND_ACCEPTED_ASYNC)
							{
								pMsg = NULL;	// Prevent deletion
								m_dwRefCount++;
							}
							break;   // Message was processed by the module
						}
					}
				}
            if (i == g_dwNumModules)
            {
               CSCPMessage response;

               response.SetId(pMsg->GetId());
               response.SetCode(CMD_REQUEST_COMPLETED);
               response.SetVariable(VID_RCC, RCC_NOT_IMPLEMENTED);
               sendMessage(&response);
            }
            break;
      }
      delete pMsg;
      m_iState = m_isAuthenticated ? SESSION_STATE_IDLE : SESSION_STATE_INIT;
   }
}

/**
 * Respond to client's keepalive message
 */
void MobileDeviceSession::respondToKeepalive(UINT32 dwRqId)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Send message to client
 */
void MobileDeviceSession::sendMessage(CSCPMessage *msg)
{
   TCHAR szBuffer[128];
   BOOL bResult;

	debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->GetCode(), szBuffer));
	CSCP_MESSAGE *pRawMsg = msg->createMessage();
   if (g_debugLevel >= 8)
   {
      String msgDump = CSCPMessage::dump(pRawMsg, NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }
   if (m_pCtx != NULL)
   {
      CSCP_ENCRYPTED_MESSAGE *pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_hSocket, (const char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pRawMsg->dwSize));
   }
   free(pRawMsg);

   if (!bResult)
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
}

/**
 * Send server information to client
 */
void MobileDeviceSession::sendServerInfo(UINT32 dwRqId)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

	// Generate challenge for certificate authentication
#ifdef _WITH_ENCRYPTION
	RAND_bytes(m_challenge, CLIENT_CHALLENGE_SIZE);
#else
	memset(m_challenge, 0, CLIENT_CHALLENGE_SIZE);
#endif

   // Fill message with server info
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   msg.SetVariable(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.SetVariable(VID_SERVER_ID, (BYTE *)&g_qwServerId, sizeof(QWORD));
   msg.SetVariable(VID_PROTOCOL_VERSION, (UINT32)MOBILE_DEVICE_PROTOCOL_VERSION);
	msg.SetVariable(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);

   // Send response
   sendMessage(&msg);
}

/**
 * Authenticate client
 */
void MobileDeviceSession::login(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szLogin[MAX_USER_NAME], szPassword[1024];
	int nAuthType;
   bool changePasswd = false, intruderLockout = false;
   UINT32 dwResult;
#ifdef _WITH_ENCRYPTION
	X509 *pCert;
#endif

   // Prepare response message
   msg.SetCode(CMD_LOGIN_RESP);
   msg.SetId(pRequest->GetId());

   // Get client info string
   if (pRequest->isFieldExist(VID_CLIENT_INFO))
   {
      TCHAR szClientInfo[32], szOSInfo[32], szLibVersion[16];

      pRequest->GetVariableStr(VID_CLIENT_INFO, szClientInfo, 32);
      pRequest->GetVariableStr(VID_OS_INFO, szOSInfo, 32);
      pRequest->GetVariableStr(VID_LIBNXCL_VERSION, szLibVersion, 16);
      _sntprintf(m_szClientInfo, 96, _T("%s (%s; libnxcl %s)"),
                 szClientInfo, szOSInfo, szLibVersion);
   }

   if (!m_isAuthenticated)
   {
      pRequest->GetVariableStr(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
		nAuthType = (int)pRequest->GetVariableShort(VID_AUTH_TYPE);
		UINT64 userRights;
		switch(nAuthType)
		{
			case NETXMS_AUTH_TYPE_PASSWORD:
#ifdef UNICODE
				pRequest->GetVariableStr(VID_PASSWORD, szPassword, 256);
#else
				pRequest->GetVariableStrUTF8(VID_PASSWORD, szPassword, 1024);
#endif
				dwResult = AuthenticateUser(szLogin, szPassword, 0, NULL, NULL, &m_dwUserId,
													 &userRights, &changePasswd, &intruderLockout, false);
				break;
			case NETXMS_AUTH_TYPE_CERTIFICATE:
#ifdef _WITH_ENCRYPTION
				pCert = CertificateFromLoginMessage(pRequest);
				if (pCert != NULL)
				{
					BYTE signature[256];
					UINT32 dwSigLen;

					dwSigLen = pRequest->GetVariableBinary(VID_SIGNATURE, signature, 256);
					dwResult = AuthenticateUser(szLogin, (TCHAR *)signature, dwSigLen, pCert,
														 m_challenge, &m_dwUserId, &userRights,
														 &changePasswd, &intruderLockout, false);
					X509_free(pCert);
				}
				else
				{
					dwResult = RCC_BAD_CERTIFICATE;
				}
#else
				dwResult = RCC_NOT_IMPLEMENTED;
#endif
				break;
			default:
				dwResult = RCC_UNSUPPORTED_AUTH_TYPE;
				break;
		}

      if (dwResult == RCC_SUCCESS)
      {
			if (userRights & SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN)
			{
				TCHAR deviceId[MAX_OBJECT_NAME] = _T("");
				pRequest->GetVariableStr(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
				MobileDevice *md = FindMobileDeviceByDeviceID(deviceId);
				if (md != NULL)
				{
					m_deviceObjectId = md->Id();
					m_isAuthenticated = true;
					_sntprintf(m_szUserName, MAX_SESSION_NAME, _T("%s@%s"), szLogin, m_szHostName);
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					msg.SetVariable(VID_USER_SYS_RIGHTS, userRights);
					msg.SetVariable(VID_USER_ID, m_dwUserId);
					msg.SetVariable(VID_CHANGE_PASSWD_FLAG, (WORD)changePasswd);
					msg.SetVariable(VID_DBCONN_STATUS, (WORD)((g_dwFlags & AF_DB_CONNECTION_LOST) ? 0 : 1));
					msg.SetVariable(VID_ZONING_ENABLED, (WORD)((g_dwFlags & AF_ENABLE_ZONING) ? 1 : 0));
					debugPrintf(3, _T("User %s authenticated as mobile device"), m_szUserName);
					WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_szHostName, 0,
									  _T("Mobile device logged in as user \"%s\" (client info: %s)"), szLogin, m_szClientInfo);
				}
				else
				{
					debugPrintf(3, _T("Mobile device object with device ID \"%s\" not found"), deviceId);
					msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
					WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_szHostName, 0,
									  _T("Mobile device login as user \"%s\" failed - mobile device object not found (client info: %s)"),
									  szLogin, m_szClientInfo);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
				WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_szHostName, 0,
								  _T("Mobile device login as user \"%s\" failed - user does not have mobile device login rights (client info: %s)"),
								  szLogin, m_szClientInfo);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, dwResult);
			WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_szHostName, 0,
			              _T("Mobile device login as user \"%s\" failed with error code %d (client info: %s)"),
							  szLogin, dwResult, m_szClientInfo);
			if (intruderLockout)
			{
				WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_szHostName, 0,
								  _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), szLogin);
			}
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Setup encryption with client
 */
void MobileDeviceSession::setupEncryption(CSCPMessage *request)
{
   CSCPMessage msg;

#ifdef _WITH_ENCRYPTION
	m_dwEncryptionRqId = request->GetId();
   m_dwEncryptionResult = RCC_TIMEOUT;
   if (m_condEncryptionSetup == INVALID_CONDITION_HANDLE)
      m_condEncryptionSetup = ConditionCreate(FALSE);

   // Send request for session key
	PrepareKeyRequestMsg(&msg, g_pServerKey, request->GetVariableShort(VID_USE_X509_KEY_FORMAT) != 0);
	msg.SetId(request->GetId());
   sendMessage(&msg);
   msg.deleteAllVariables();

   // Wait for encryption setup
   ConditionWait(m_condEncryptionSetup, 30000);

   // Send response
   msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());
   msg.SetVariable(VID_RCC, m_dwEncryptionResult);
#else    /* _WITH_ENCRYPTION not defined */
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());
   msg.SetVariable(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif

   sendMessage(&msg);
}

/**
 * Update device system information
 */
void MobileDeviceSession::updateDeviceInfo(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	MobileDevice *device = (MobileDevice *)FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE);
	if (device != NULL)
	{
		device->updateSystemInfo(request);
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}

/**
 * Update device status
 */
void MobileDeviceSession::updateDeviceStatus(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	MobileDevice *device = (MobileDevice *)FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE);
	if (device != NULL)
	{
		device->updateStatus(request);
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}

/**
 * Push DCI data
 */
void MobileDeviceSession::pushData(CSCPMessage *request)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	MobileDevice *device = (MobileDevice *)FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE);
	if (device != NULL)
	{
      int count = (int)request->GetVariableLong(VID_NUM_ITEMS);
      if (count > 0)
      {
         DCItem **dciList = (DCItem **)malloc(sizeof(DCItem *) * count);
         TCHAR **valueList = (TCHAR **)malloc(sizeof(TCHAR *) * count);
         memset(valueList, 0, sizeof(TCHAR *) * count);

         int i;
         UINT32 varId = VID_PUSH_DCI_DATA_BASE;
         bool ok = true;
         for(i = 0; (i < count) && ok; i++)
         {
            ok = false;

            // find DCI by ID or name (if ID==0)
            UINT32 dciId = request->GetVariableLong(varId++);
		      DCObject *pItem;
            if (dciId != 0)
            {
               pItem = device->getDCObjectById(dciId);
            }
            else
            {
               TCHAR name[MAX_PARAM_NAME];
               request->GetVariableStr(varId++, name, MAX_PARAM_NAME);
               pItem = device->getDCObjectByName(name);
            }

            if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
            {
               if (pItem->getDataSource() == DS_PUSH_AGENT)
               {
                  dciList[i] = (DCItem *)pItem;
                  valueList[i] = request->GetVariableStr(varId++);
                  ok = true;
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_NOT_PUSH_DCI);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
            }
         }

         // If all items was checked OK, push data
         if (ok)
         {
            time_t t = 0;
            int ft = request->getFieldType(VID_TIMESTAMP);
            if (ft == CSCP_DT_INTEGER)
            {
               t = (time_t)request->GetVariableLong(VID_TIMESTAMP);
            }
            else if (ft == CSCP_DT_STRING)
            {
               char ts[256];
               request->GetVariableStrA(VID_TIMESTAMP, ts, 256);

               struct tm timeBuff;
               if (strptime(ts, "%Y/%m/%d %H:%M:%S", &timeBuff) != NULL)
               {
                  timeBuff.tm_isdst = -1;
                  t = timegm(&timeBuff);
               }
            }
            if (t == 0)
            {
               time(&t);
            }

            for(i = 0; i < count; i++)
            {
			      if (_tcslen(valueList[i]) >= MAX_DCI_STRING_VALUE)
				      valueList[i][MAX_DCI_STRING_VALUE - 1] = 0;
			      device->processNewDCValue(dciList[i], t, valueList[i]);
			      dciList[i]->setLastPollTime(t);
            }
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_FAILED_DCI_INDEX, i - 1);
         }

         // Cleanup
         for(i = 0; i < count; i++)
            safe_free(valueList[i]);
         safe_free(valueList);
         free(dciList);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
      }
   }
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}
