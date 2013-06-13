/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: agent.cpp
**
**/

#include "libnxsrv.h"
#include <stdarg.h>

#ifdef _WIN32
#define open	_open
#define close	_close
#define write	_write
#else
#define _tell(f) lseek(f,0,SEEK_CUR)
#endif

/**
 * Constants
 */
#define MAX_MSG_SIZE    8388608

/**
 * Static data
 */
#ifdef _WITH_ENCRYPTION
static int m_iDefaultEncryptionPolicy = ENCRYPTION_ALLOWED;
#else
static int m_iDefaultEncryptionPolicy = ENCRYPTION_DISABLED;
#endif

/**
 * Set default encryption policy for agent communication
 */
void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy)
{
#ifdef _WITH_ENCRYPTION
   m_iDefaultEncryptionPolicy = iPolicy;
#endif
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL AgentConnection::receiverThreadStarter(void *pArg)
{
   ((AgentConnection *)pArg)->receiverThread();
   return THREAD_OK;
}

/**
 * Constructor for AgentConnection
 */
AgentConnection::AgentConnection(UINT32 dwAddr, WORD wPort, int iAuthMethod, const TCHAR *pszSecret)
{
   m_dwAddr = dwAddr;
   m_wPort = wPort;
   m_iAuthMethod = iAuthMethod;
   if (pszSecret != NULL)
   {
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pszSecret, -1, m_szSecret, MAX_SECRET_LENGTH, NULL, NULL);
		m_szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
      nx_strncpy(m_szSecret, pszSecret, MAX_SECRET_LENGTH);
#endif
   }
   else
   {
      m_szSecret[0] = 0;
   }
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_dwRequestId = 1;
	m_connectionTimeout = 30000;	// 30 seconds
   m_dwCommandTimeout = 10000;   // Default timeout 10 seconds
   m_bIsConnected = FALSE;
   m_mutexDataLock = MutexCreate();
	m_mutexSocketWrite = MutexCreate();
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_pCtx = NULL;
   m_iEncryptionPolicy = m_iDefaultEncryptionPolicy;
   m_bUseProxy = FALSE;
   m_dwRecvTimeout = 420000;  // 7 minutes
   m_nProtocolVersion = NXCP_VERSION;
	m_hCurrFile = -1;
	m_deleteFileOnDownloadFailure = true;
	m_condFileDownload = ConditionCreate(TRUE);
	m_fileUploadInProgress = false;
}

/**
 * Destructor
 */
AgentConnection::~AgentConnection()
{
   // Disconnect from peer
   disconnect();

   // Wait for receiver thread termination
   ThreadJoin(m_hReceiverThread);
   
	// Close socket if active
	lock();
   if (m_hSocket != -1)
	{
      closesocket(m_hSocket);
		m_hSocket = -1;
	}
	unlock();

   lock();
   destroyResultData();
   unlock();

   delete m_pMsgWaitQueue;
	if (m_pCtx != NULL)
		m_pCtx->decRefCount();

	if (m_hCurrFile != -1)
	{
		close(m_hCurrFile);
		onFileDownload(FALSE);
	}

   MutexDestroy(m_mutexDataLock);
	MutexDestroy(m_mutexSocketWrite);
	ConditionDestroy(m_condFileDownload);
}

/**
 * Print message. This method is virtual and can be overrided in
 * derived classes. Default implementation will print message to stdout.
 */
void AgentConnection::printMsg(const TCHAR *pszFormat, ...)
{
   va_list args;

   va_start(args, pszFormat);
   _vtprintf(pszFormat, args);
   va_end(args);
   _tprintf(_T("\n"));
}

/**
 * Receiver thread
 */
void AgentConnection::receiverThread()
{
	UINT32 msgBufferSize = 1024;
   CSCPMessage *pMsg;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pMsgBuffer;
   BYTE *pDecryptionBuffer = NULL;
   int error;
   TCHAR szBuffer[128], szIpAddr[16];
	SOCKET nSocket;

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pMsgBuffer, 0, NULL, NULL, 0);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)malloc(msgBufferSize);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(msgBufferSize);
#endif

   // Message receiving loop
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

      // Receive raw message
      lock();
		nSocket = m_hSocket;
		unlock();
      if ((error = RecvNXCPMessageEx(nSocket, &pRawMsg, pMsgBuffer, &msgBufferSize,
                                     &m_pCtx, (pDecryptionBuffer != NULL) ? &pDecryptionBuffer : NULL,
											    m_dwRecvTimeout, MAX_MSG_SIZE)) <= 0)
		{
			if (WSAGetLastError() != WSAESHUTDOWN)
				DbgPrintf(6, _T("AgentConnection::ReceiverThread(): RecvNXCPMessage() failed: error=%d, socket_error=%d"), error, WSAGetLastError());
         break;
		}

      // Check if we get too large message
      if (error == 1)
      {
         printMsg(_T("Received too large message %s (%d bytes)"), 
                  NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                  ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check if we are unable to decrypt message
      if (error == 2)
      {
         printMsg(_T("Unable to decrypt received message"));
         continue;
      }

      // Check for timeout
      if (error == 3)
      {
			if (m_fileUploadInProgress)
				continue;	// Receive timeout may occur when uploading large files via slow links
         printMsg(_T("Timed out waiting for message"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != error)
      {
         printMsg(_T("RecvMsg: Bad packet length [dwSize=%d ActualSize=%d]"), ntohl(pRawMsg->dwSize), error);
         continue;   // Bad packet, wait for next
      }

		if (ntohs(pRawMsg->wFlags) & MF_BINARY)
		{
         // Convert message header to host format
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);
         DbgPrintf(6, _T("Received raw message %s from agent at %s"),
			          NXCPMessageCodeName(pRawMsg->wCode, szBuffer), IpToStr(getIpAddr(), szIpAddr));

			if ((pRawMsg->wCode == CMD_FILE_DATA) &&
				 (m_hCurrFile != -1) && (pRawMsg->dwId == m_dwDownloadRequestId))
			{
            if (write(m_hCurrFile, pRawMsg->df, pRawMsg->dwNumVars) == (int)pRawMsg->dwNumVars)
            {
               if (ntohs(pRawMsg->wFlags) & MF_END_OF_FILE)
               {
                  close(m_hCurrFile);
                  m_hCurrFile = -1;
            
                  onFileDownload(TRUE);
               }
					else
					{
						if (m_downloadProgressCallback != NULL)
						{
							m_downloadProgressCallback(_tell(m_hCurrFile), m_downloadProgressCallbackArg);
						}
					}
            }
            else
            {
               // I/O error
               close(m_hCurrFile);
               m_hCurrFile = -1;
         
               onFileDownload(FALSE);
            }
			}
		}
		else
		{
			// Create message object from raw message
			pMsg = new CSCPMessage(pRawMsg, m_nProtocolVersion);
			switch(pMsg->GetCode())
			{
				case CMD_TRAP:
					onTrap(pMsg);
					delete pMsg;
					break;
				case CMD_PUSH_DCI_DATA:
					onDataPush(pMsg);
					delete pMsg;
					break;
				case CMD_REQUEST_COMPLETED:
					m_pMsgWaitQueue->put(pMsg);
					break;
				default:
					if (processCustomMessage(pMsg))
						delete pMsg;
					else
						m_pMsgWaitQueue->put(pMsg);
					break;
			}
		}
   }

   // Close socket and mark connection as disconnected
   lock();
	if (m_hCurrFile != -1)
	{
		close(m_hCurrFile);
		m_hCurrFile = -1;
		onFileDownload(FALSE);
	}
	
	if (error == 0)
      shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   m_hSocket = -1;
	if (m_pCtx != NULL)
	{
		m_pCtx->decRefCount();
		m_pCtx = NULL;
	}
   m_bIsConnected = FALSE;
   unlock();

   free(pRawMsg);
   free(pMsgBuffer);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif
}

/**
 * Connect to agent
 */
BOOL AgentConnection::connect(RSA *pServerKey, BOOL bVerbose, UINT32 *pdwError, UINT32 *pdwSocketError)
{
   struct sockaddr_in sa;
   TCHAR szBuffer[256];
   BOOL bSuccess = FALSE, bForceEncryption = FALSE, bSecondPass = FALSE;
   UINT32 dwError = 0;

   if (pdwError != NULL)
      *pdwError = ERR_INTERNAL_ERROR;

   if (pdwSocketError != NULL)
      *pdwSocketError = 0;

   // Check if already connected
   if (m_bIsConnected)
      return FALSE;

   // Wait for receiver thread from previous connection, if any
   ThreadJoin(m_hReceiverThread);
   m_hReceiverThread = INVALID_THREAD_HANDLE;

   // Check if we need to close existing socket
   if (m_hSocket != -1)
      closesocket(m_hSocket);

   // Create socket
   m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (m_hSocket == -1)
   {
      printMsg(_T("Call to socket() failed"));
      goto connect_cleanup;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   if (m_bUseProxy)
   {
      sa.sin_addr.s_addr = m_dwProxyAddr;
      sa.sin_port = htons(m_wProxyPort);
   }
   else
   {
      sa.sin_addr.s_addr = m_dwAddr;
      sa.sin_port = htons(m_wPort);
   }

   // Connect to server
	if (ConnectEx(m_hSocket, (struct sockaddr *)&sa, sizeof(sa), m_connectionTimeout) == -1)
   {
      if (bVerbose)
         printMsg(_T("Cannot establish connection with agent %s"),
                  IpToStr(ntohl(m_bUseProxy ? m_dwProxyAddr : m_dwAddr), szBuffer));
      dwError = ERR_CONNECT_FAILED;
      goto connect_cleanup;
   }

   if (!NXCPGetPeerProtocolVersion(m_hSocket, &m_nProtocolVersion, m_mutexSocketWrite))
   {
      dwError = ERR_INTERNAL_ERROR;
      goto connect_cleanup;
   }

   // Start receiver thread
   m_hReceiverThread = ThreadCreateEx(receiverThreadStarter, 0, this);

   // Setup encryption
setup_encryption:
   if ((m_iEncryptionPolicy == ENCRYPTION_PREFERRED) ||
       (m_iEncryptionPolicy == ENCRYPTION_REQUIRED) ||
       (bForceEncryption))    // Agent require encryption
   {
      if (pServerKey != NULL)
      {
         dwError = setupEncryption(pServerKey);
         if ((dwError != ERR_SUCCESS) &&
             ((m_iEncryptionPolicy == ENCRYPTION_REQUIRED) || bForceEncryption))
            goto connect_cleanup;
      }
      else
      {
         if ((m_iEncryptionPolicy == ENCRYPTION_REQUIRED) || bForceEncryption)
         {
            dwError = ERR_ENCRYPTION_REQUIRED;
            goto connect_cleanup;
         }
      }
   }

   // Authenticate itself to agent
   if ((dwError = authenticate(m_bUseProxy && !bSecondPass)) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         bForceEncryption = TRUE;
         goto setup_encryption;
      }
      printMsg(_T("Authentication to agent %s failed (%s)"), IpToStr(ntohl(m_dwAddr), szBuffer),
               AgentErrorCodeToText(dwError));
      goto connect_cleanup;
   }

   // Test connectivity
   if ((dwError = nop()) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         bForceEncryption = TRUE;
         goto setup_encryption;
      }
      printMsg(_T("Communication with agent %s failed (%s)"), IpToStr(ntohl(m_dwAddr), szBuffer),
               AgentErrorCodeToText(dwError));
      goto connect_cleanup;
   }

   if (m_bUseProxy && !bSecondPass)
   {
      dwError = setupProxyConnection();
      if (dwError != ERR_SUCCESS)
         goto connect_cleanup;
		lock();
		if (m_pCtx != NULL)
		{
			m_pCtx->decRefCount();
	      m_pCtx = NULL;
		}
		unlock();
      bSecondPass = TRUE;
      bForceEncryption = FALSE;
      goto setup_encryption;
   }

   bSuccess = TRUE;
   dwError = ERR_SUCCESS;

connect_cleanup:
   if (!bSuccess)
   {
		if (pdwSocketError != NULL)
			*pdwSocketError = (UINT32)WSAGetLastError();

      lock();
      if (m_hSocket != -1)
         shutdown(m_hSocket, SHUT_RDWR);
      unlock();
      ThreadJoin(m_hReceiverThread);
      m_hReceiverThread = INVALID_THREAD_HANDLE;

      lock();
      if (m_hSocket != -1)
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
      }

		if (m_pCtx != NULL)
		{
			m_pCtx->decRefCount();
	      m_pCtx = NULL;
		}

      unlock();
   }
   m_bIsConnected = bSuccess;
   if (pdwError != NULL)
      *pdwError = dwError;
   return bSuccess;
}


//
// Disconnect from agent
//

void AgentConnection::disconnect()
{
   lock();
	if (m_hCurrFile != -1)
	{
		close(m_hCurrFile);
		m_hCurrFile = -1;
		onFileDownload(FALSE);
	}
	
   if (m_hSocket != -1)
   {
      shutdown(m_hSocket, SHUT_RDWR);
   }
   destroyResultData();
   m_bIsConnected = FALSE;
   unlock();
}


//
// Destroy command execuion results data
//

void AgentConnection::destroyResultData()
{
   UINT32 i;

   if (m_ppDataLines != NULL)
   {
      for(i = 0; i < m_dwNumDataLines; i++)
         if (m_ppDataLines[i] != NULL)
            free(m_ppDataLines[i]);
      free(m_ppDataLines);
      m_ppDataLines = NULL;
   }
   m_dwNumDataLines = 0;
}

/**
 * Get interface list from agent
 */
InterfaceList *AgentConnection::getInterfaceList()
{
   InterfaceList *pIfList = NULL;
	NX_INTERFACE_INFO iface;
   UINT32 i, dwBits;
   TCHAR *pChar, *pBuf;

   if (getList(_T("Net.InterfaceList")) == ERR_SUCCESS)
   {
      pIfList = new InterfaceList(m_dwNumDataLines);

      // Parse result set. Each line should have the following format:
      // index ip_address/mask_bits iftype mac_address name
      for(i = 0; i < m_dwNumDataLines; i++)
      {
         pBuf = m_ppDataLines[i];
			memset(&iface, 0, sizeof(NX_INTERFACE_INFO));

         // Index
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            iface.dwIndex = _tcstoul(pBuf, NULL, 10);
            pBuf = pChar + 1;
         }

         // Address and mask
         pChar = _tcschr(pBuf, _T(' '));
         if (pChar != NULL)
         {
            TCHAR *pSlash;
            static TCHAR defaultMask[] = _T("24");

            *pChar = 0;
            pSlash = _tcschr(pBuf, _T('/'));
            if (pSlash != NULL)
            {
               *pSlash = 0;
               pSlash++;
            }
            else     // Just a paranoia protection, should'n happen if agent working correctly
            {
               pSlash = defaultMask;
            }
            iface.dwIpAddr = ntohl(_t_inet_addr(pBuf));
            dwBits = _tcstoul(pSlash, NULL, 10);
            iface.dwIpNetMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
            pBuf = pChar + 1;
         }

         // Interface type
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            iface.dwType = _tcstoul(pBuf, NULL, 10);
            pBuf = pChar + 1;
         }

         // MAC address
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            StrToBin(pBuf, iface.bMacAddr, MAC_ADDR_LENGTH);
            pBuf = pChar + 1;
         }

         // Name (set description to name)
         nx_strncpy(iface.szName, pBuf, MAX_DB_STRING);
			nx_strncpy(iface.szDescription, pBuf, MAX_DB_STRING);

			pIfList->add(&iface);
      }

      lock();
      destroyResultData();
      unlock();
   }

   return pIfList;
}


//
// Get parameter value
//

UINT32 AgentConnection::getParameter(const TCHAR *pszParam, UINT32 dwBufSize, TCHAR *pszBuffer)
{
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   UINT32 dwRqId, dwRetCode;

   if (m_bIsConnected)
   {
      dwRqId = m_dwRequestId++;
      msg.SetCode(CMD_GET_PARAMETER);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_PARAMETER, pszParam);
      if (sendMessage(&msg))
      {
         pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
         if (pResponse != NULL)
         {
            dwRetCode = pResponse->GetVariableLong(VID_RCC);
            if (dwRetCode == ERR_SUCCESS)
               pResponse->GetVariableStr(VID_VALUE, pszBuffer, dwBufSize);
            delete pResponse;
         }
         else
         {
            dwRetCode = ERR_REQUEST_TIMEOUT;
         }
      }
      else
      {
         dwRetCode = ERR_CONNECTION_BROKEN;
      }
   }
   else
   {
      dwRetCode = ERR_NOT_CONNECTED;
   }

   return dwRetCode;
}


//
// Get ARP cache
//

ARP_CACHE *AgentConnection::getArpCache()
{
   ARP_CACHE *pArpCache = NULL;
   TCHAR szByte[4], *pBuf, *pChar;
   UINT32 i, j;

   if (getList(_T("Net.ArpCache")) == ERR_SUCCESS)
   {
      // Create empty structure
      pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
      pArpCache->dwNumEntries = m_dwNumDataLines;
      pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * m_dwNumDataLines);
      memset(pArpCache->pEntries, 0, sizeof(ARP_ENTRY) * m_dwNumDataLines);

      szByte[2] = 0;

      // Parse data lines
      // Each line has form of XXXXXXXXXXXX a.b.c.d n
      // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
      // a.b.c.d is an IP address in decimal dotted notation
      // n is an interface index
      for(i = 0; i < m_dwNumDataLines; i++)
      {
         pBuf = m_ppDataLines[i];
         if (_tcslen(pBuf) < 20)     // Invalid line
            continue;

         // MAC address
         for(j = 0; j < 6; j++)
         {
            memcpy(szByte, pBuf, sizeof(TCHAR) * 2);
            pArpCache->pEntries[i].bMacAddr[j] = (BYTE)_tcstol(szByte, NULL, 16);
            pBuf+=2;
         }

         // IP address
         while(*pBuf == ' ')
            pBuf++;
         pChar = _tcschr(pBuf, _T(' '));
         if (pChar != NULL)
            *pChar = 0;
         pArpCache->pEntries[i].dwIpAddr = ntohl(_t_inet_addr(pBuf));

         // Interface index
         if (pChar != NULL)
            pArpCache->pEntries[i].dwIndex = _tcstoul(pChar + 1, NULL, 10);
      }

      lock();
      destroyResultData();
      unlock();
   }
   return pArpCache;
}


//
// Send dummy command to agent (can be used for keepalive)
//

UINT32 AgentConnection::nop()
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_KEEPALIVE);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Wait for request completion code
//

UINT32 AgentConnection::waitForRCC(UINT32 dwRqId, UINT32 dwTimeOut)
{
   CSCPMessage *pMsg;
   UINT32 dwRetCode;

   pMsg = m_pMsgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
   if (pMsg != NULL)
   {
      dwRetCode = pMsg->GetVariableLong(VID_RCC);
      delete pMsg;
   }
   else
   {
      dwRetCode = ERR_REQUEST_TIMEOUT;
   }
   return dwRetCode;
}


//
// Send message to agent
//

BOOL AgentConnection::sendMessage(CSCPMessage *pMsg)
{
   BOOL bResult;

   CSCP_MESSAGE *pRawMsg = pMsg->CreateMessage();
	NXCPEncryptionContext *pCtx = acquireEncryptionContext();
   if (pCtx != NULL)
   {
      CSCP_ENCRYPTED_MESSAGE *pEnMsg = CSCPEncryptMessage(pCtx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
		pCtx->decRefCount();
   }
   else
   {
      bResult = (SendEx(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pRawMsg->dwSize));
   }
   free(pRawMsg);
   return bResult;
}

/**
 * Trap handler. Should be overriden in derived classes to implement
 * actual trap processing. Default implementation do nothing.
 */
void AgentConnection::onTrap(CSCPMessage *pMsg)
{
}

/**
 * Data push handler. Should be overriden in derived classes to implement
 * actual data push processing. Default implementation do nothing.
 */
void AgentConnection::onDataPush(CSCPMessage *pMsg)
{
}

/**
 * Custom message handler
 * If returns true, message considered as processed and will not be placed in wait queue
 */
bool AgentConnection::processCustomMessage(CSCPMessage *pMsg)
{
	return false;
}

/**
 * Get list of values
 */
UINT32 AgentConnection::getList(const TCHAR *pszParam)
{
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   UINT32 i, dwRqId, dwRetCode;

   if (m_bIsConnected)
   {
      destroyResultData();
      dwRqId = m_dwRequestId++;
      msg.SetCode(CMD_GET_LIST);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_PARAMETER, pszParam);
      if (sendMessage(&msg))
      {
         pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
         if (pResponse != NULL)
         {
            dwRetCode = pResponse->GetVariableLong(VID_RCC);
            if (dwRetCode == ERR_SUCCESS)
            {
               m_dwNumDataLines = pResponse->GetVariableLong(VID_NUM_STRINGS);
               m_ppDataLines = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumDataLines);
               for(i = 0; i < m_dwNumDataLines; i++)
                  m_ppDataLines[i] = pResponse->GetVariableStr(VID_ENUM_VALUE_BASE + i);
            }
            delete pResponse;
         }
         else
         {
            dwRetCode = ERR_REQUEST_TIMEOUT;
         }
      }
      else
      {
         dwRetCode = ERR_CONNECTION_BROKEN;
      }
   }
   else
   {
      dwRetCode = ERR_NOT_CONNECTED;
   }

   return dwRetCode;
}

/**
 * Get table
 */
UINT32 AgentConnection::getTable(const TCHAR *pszParam, Table **table)
{
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   UINT32 dwRqId, dwRetCode;

	*table = NULL;
   if (m_bIsConnected)
   {
      dwRqId = m_dwRequestId++;
      msg.SetCode(CMD_GET_TABLE);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_PARAMETER, pszParam);
      if (sendMessage(&msg))
      {
         pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
         if (pResponse != NULL)
         {
            dwRetCode = pResponse->GetVariableLong(VID_RCC);
            if (dwRetCode == ERR_SUCCESS)
            {
					*table = new Table(pResponse);
            }
            delete pResponse;
         }
         else
         {
            dwRetCode = ERR_REQUEST_TIMEOUT;
         }
      }
      else
      {
         dwRetCode = ERR_CONNECTION_BROKEN;
      }
   }
   else
   {
      dwRetCode = ERR_NOT_CONNECTED;
   }

   return dwRetCode;
}

/**
 * Authenticate to agent
 */
UINT32 AgentConnection::authenticate(BOOL bProxyData)
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;
   BYTE hash[32];
   int iAuthMethod = bProxyData ? m_iProxyAuth : m_iAuthMethod;
   const char *pszSecret = bProxyData ? m_szProxySecret : m_szSecret;
#ifdef UNICODE
   WCHAR szBuffer[MAX_SECRET_LENGTH];
#endif

   if (iAuthMethod == AUTH_NONE)
      return ERR_SUCCESS;  // No authentication required

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_AUTHENTICATE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_AUTH_METHOD, (WORD)iAuthMethod);
   switch(iAuthMethod)
   {
      case AUTH_PLAINTEXT:
#ifdef UNICODE
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszSecret, -1, szBuffer, MAX_SECRET_LENGTH);
         msg.SetVariable(VID_SHARED_SECRET, szBuffer);
#else
         msg.SetVariable(VID_SHARED_SECRET, pszSecret);
#endif
         break;
      case AUTH_MD5_HASH:
         CalculateMD5Hash((BYTE *)pszSecret, (int)strlen(pszSecret), hash);
         msg.SetVariable(VID_SHARED_SECRET, hash, MD5_DIGEST_SIZE);
         break;
      case AUTH_SHA1_HASH:
         CalculateSHA1Hash((BYTE *)pszSecret, (int)strlen(pszSecret), hash);
         msg.SetVariable(VID_SHARED_SECRET, hash, SHA1_DIGEST_SIZE);
         break;
      default:
         break;
   }
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Execute action on agent
//

UINT32 AgentConnection::execAction(const TCHAR *pszAction, int argc, TCHAR **argv)
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;
   int i;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_ACTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ACTION_NAME, pszAction);
   msg.SetVariable(VID_NUM_ARGS, (UINT32)argc);
   for(i = 0; i < argc; i++)
      msg.SetVariable(VID_ACTION_ARG_BASE + i, argv[i]);

   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Upload file to agent
//

UINT32 AgentConnection::uploadFile(const TCHAR *localFile, const TCHAR *destinationFile, void (* progressCallback)(INT64, void *), void *cbArg)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion);
   int i;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_TRANSFER_FILE);
   msg.SetId(dwRqId);
   for(i = (int)_tcslen(localFile) - 1; 
       (i >= 0) && (localFile[i] != '\\') && (localFile[i] != '/'); i--);
   msg.SetVariable(VID_FILE_NAME, &localFile[i + 1]);
   if (destinationFile != NULL)
   {
		msg.SetVariable(VID_DESTINATION_FILE_NAME, destinationFile);
   }

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   if (dwResult == ERR_SUCCESS)
   {
		m_fileUploadInProgress = true;
		NXCPEncryptionContext *ctx = acquireEncryptionContext();
      if (SendFileOverNXCP(m_hSocket, dwRqId, localFile, ctx, 0, progressCallback, cbArg, m_mutexSocketWrite))
         dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
      else
         dwResult = ERR_IO_FAILURE;
		m_fileUploadInProgress = false;
   }

   return dwResult;
}


//
// Send upgrade command
//

UINT32 AgentConnection::startUpgrade(const TCHAR *pszPkgName)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion);
   int i;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_UPGRADE_AGENT);
   msg.SetId(dwRqId);
   for(i = (int)_tcslen(pszPkgName) - 1; 
       (i >= 0) && (pszPkgName[i] != '\\') && (pszPkgName[i] != '/'); i--);
   msg.SetVariable(VID_FILE_NAME, &pszPkgName[i + 1]);

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}


//
// Check status of network service via agent
//

UINT32 AgentConnection::checkNetworkService(UINT32 *pdwStatus, UINT32 dwIpAddr, int iServiceType, 
                                           WORD wPort, WORD wProto, 
                                           const TCHAR *pszRequest, const TCHAR *pszResponse)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   static WORD m_wDefaultPort[] = { 7, 22, 110, 25, 21, 80 };

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_CHECK_NETWORK_SERVICE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IP_ADDRESS, dwIpAddr);
   msg.SetVariable(VID_SERVICE_TYPE, (WORD)iServiceType);
   msg.SetVariable(VID_IP_PORT, 
      (wPort != 0) ? wPort : 
         m_wDefaultPort[((iServiceType >= NETSRV_CUSTOM) && 
                         (iServiceType <= NETSRV_HTTP)) ? iServiceType : 0]);
   msg.SetVariable(VID_IP_PROTO, (wProto != 0) ? wProto : (WORD)IPPROTO_TCP);
   msg.SetVariable(VID_SERVICE_REQUEST, pszRequest);
   msg.SetVariable(VID_SERVICE_RESPONSE, pszResponse);

   if (sendMessage(&msg))
   {
      // Wait up to 90 seconds for results
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 90000);
      if (pResponse != NULL)
      {
         dwResult = pResponse->GetVariableLong(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            *pdwStatus = pResponse->GetVariableLong(VID_SERVICE_STATUS);
         }
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Get list of supported parameters from agent
 */
UINT32 AgentConnection::getSupportedParameters(ObjectArray<AgentParameterDefinition> **paramList, ObjectArray<AgentTableDefinition> **tableList)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion), *pResponse;

   *paramList = NULL;
	*tableList = NULL;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_GET_PARAMETER_LIST);
   msg.SetId(dwRqId);

   if (sendMessage(&msg))
   {
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponse != NULL)
      {
         dwResult = pResponse->GetVariableLong(VID_RCC);
			DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): RCC=%d"), dwResult);
         if (dwResult == ERR_SUCCESS)
         {
            UINT32 count = pResponse->GetVariableLong(VID_NUM_PARAMETERS);
            ObjectArray<AgentParameterDefinition> *plist = new ObjectArray<AgentParameterDefinition>(count, 16, true);
            for(UINT32 i = 0, dwId = VID_PARAM_LIST_BASE; i < count; i++)
            {
               plist->add(new AgentParameterDefinition(pResponse, dwId));
               dwId += 3;
            }
				*paramList = plist;
				DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): %d parameters received from agent"), count);

            count = pResponse->GetVariableLong(VID_NUM_TABLES);
            ObjectArray<AgentTableDefinition> *tlist = new ObjectArray<AgentTableDefinition>(count, 16, true);
            for(UINT32 i = 0, dwId = VID_TABLE_LIST_BASE; i < count; i++)
            {
               tlist->add(new AgentTableDefinition(pResponse, dwId));
               dwId += 3;
            }
				*tableList = tlist;
				DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): %d tables received from agent"), count);
			}
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Setup encryption
 */
UINT32 AgentConnection::setupEncryption(RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   CSCPMessage msg(m_nProtocolVersion), *pResp;
   UINT32 dwRqId, dwError, dwResult;

   dwRqId = m_dwRequestId++;

   PrepareKeyRequestMsg(&msg, pServerKey, false);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
   {
      pResp = waitForMessage(CMD_SESSION_KEY, dwRqId, m_dwCommandTimeout);
      if (pResp != NULL)
      {
         dwResult = SetupEncryptionContext(pResp, &m_pCtx, NULL, pServerKey, m_nProtocolVersion);
         switch(dwResult)
         {
            case RCC_SUCCESS:
               dwError = ERR_SUCCESS;
               break;
            case RCC_NO_CIPHERS:
               dwError = ERR_NO_CIPHERS;
               break;
            case RCC_INVALID_PUBLIC_KEY:
               dwError = ERR_INVALID_PUBLIC_KEY;
               break;
            case RCC_INVALID_SESSION_KEY:
               dwError = ERR_INVALID_SESSION_KEY;
               break;
            default:
               dwError = ERR_INTERNAL_ERROR;
               break;
         }
			delete pResp;
      }
      else
      {
         dwError = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwError = ERR_CONNECTION_BROKEN;
   }

   return dwError;
#else
   return ERR_NOT_IMPLEMENTED;
#endif
}


//
// Get configuration file from agent
//

UINT32 AgentConnection::getConfigFile(TCHAR **ppszConfig, UINT32 *pdwSize)
{
   UINT32 i, dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
#ifdef UNICODE
   BYTE *pBuffer;
#endif

   *ppszConfig = NULL;
   *pdwSize = 0;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_GET_AGENT_CONFIG);
   msg.SetId(dwRqId);

   if (sendMessage(&msg))
   {
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponse != NULL)
      {
         dwResult = pResponse->GetVariableLong(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            *pdwSize = pResponse->GetVariableBinary(VID_CONFIG_FILE, NULL, 0);
            *ppszConfig = (TCHAR *)malloc((*pdwSize + 1) * sizeof(TCHAR));
#ifdef UNICODE
            pBuffer = (BYTE *)malloc(*pdwSize + 1);
            pResponse->GetVariableBinary(VID_CONFIG_FILE, pBuffer, *pdwSize);
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)pBuffer, *pdwSize, *ppszConfig, *pdwSize);
            free(pBuffer);
#else
            pResponse->GetVariableBinary(VID_CONFIG_FILE, (BYTE *)(*ppszConfig), *pdwSize);
#endif
            (*ppszConfig)[*pdwSize] = 0;

            // We expect text file, so replace all non-printable characters with spaces
            for(i = 0; i < *pdwSize; i++)
               if (((*ppszConfig)[i] < _T(' ')) && 
                   ((*ppszConfig)[i] != _T('\t')) &&
                   ((*ppszConfig)[i] != _T('\r')) &&
                   ((*ppszConfig)[i] != _T('\n')))
                  (*ppszConfig)[i] = _T(' ');
         }
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}


//
// Get configuration file from agent
//

UINT32 AgentConnection::updateConfigFile(const TCHAR *pszConfig)
{
   UINT32 dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion);
#ifdef UNICODE
   int nChars;
   BYTE *pBuffer;
#endif

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_UPDATE_AGENT_CONFIG);
   msg.SetId(dwRqId);
#ifdef UNICODE
   nChars = (int)_tcslen(pszConfig);
   pBuffer = (BYTE *)malloc(nChars + 1);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszConfig, nChars, (char *)pBuffer, nChars + 1, NULL, NULL);
   msg.SetVariable(VID_CONFIG_FILE, pBuffer, nChars);
   free(pBuffer);
#else
   msg.SetVariable(VID_CONFIG_FILE, (BYTE *)pszConfig, (UINT32)strlen(pszConfig));
#endif

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}


//
// Get routing table from agent
//

ROUTING_TABLE *AgentConnection::getRoutingTable()
{
   ROUTING_TABLE *pRT = NULL;
   UINT32 i, dwBits;
   TCHAR *pChar, *pBuf;

   if (getList(_T("Net.IP.RoutingTable")) == ERR_SUCCESS)
   {
      pRT = (ROUTING_TABLE *)malloc(sizeof(ROUTING_TABLE));
      pRT->iNumEntries = m_dwNumDataLines;
      pRT->pRoutes = (ROUTE *)malloc(sizeof(ROUTE) * m_dwNumDataLines);
      memset(pRT->pRoutes, 0, sizeof(ROUTE) * m_dwNumDataLines);
      for(i = 0; i < m_dwNumDataLines; i++)
      {
         pBuf = m_ppDataLines[i];

         // Destination address and mask
         pChar = _tcschr(pBuf, _T(' '));
         if (pChar != NULL)
         {
            TCHAR *pSlash;
            static TCHAR defaultMask[] = _T("24");

            *pChar = 0;
            pSlash = _tcschr(pBuf, _T('/'));
            if (pSlash != NULL)
            {
               *pSlash = 0;
               pSlash++;
            }
            else     // Just a paranoia protection, should'n happen if agent working correctly
            {
               pSlash = defaultMask;
            }
            pRT->pRoutes[i].dwDestAddr = ntohl(_t_inet_addr(pBuf));
            dwBits = _tcstoul(pSlash, NULL, 10);
            pRT->pRoutes[i].dwDestMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
            pBuf = pChar + 1;
         }

         // Next hop address
         pChar = _tcschr(pBuf, _T(' '));
         if (pChar != NULL)
         {
            *pChar = 0;
            pRT->pRoutes[i].dwNextHop = ntohl(_t_inet_addr(pBuf));
            pBuf = pChar + 1;
         }

         // Interface index
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            pRT->pRoutes[i].dwIfIndex = _tcstoul(pBuf, NULL, 10);
            pBuf = pChar + 1;
         }

         // Route type
         pRT->pRoutes[i].dwRouteType = _tcstoul(pBuf, NULL, 10);
      }

      lock();
      destroyResultData();
      unlock();
   }

   return pRT;
}


//
// Set proxy information
//

void AgentConnection::setProxy(UINT32 dwAddr, WORD wPort, int iAuthMethod, const TCHAR *pszSecret)
{
   m_dwProxyAddr = dwAddr;
   m_wProxyPort = wPort;
   m_iProxyAuth = iAuthMethod;
   if (pszSecret != NULL)
   {
#ifdef UNICODE
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                          pszSecret, -1, m_szProxySecret, MAX_SECRET_LENGTH, NULL, NULL);
#else
      nx_strncpy(m_szProxySecret, pszSecret, MAX_SECRET_LENGTH);
#endif
   }
   else
   {
      m_szProxySecret[0] = 0;
   }
   m_bUseProxy = TRUE;
}


//
// Setup proxy connection
//

UINT32 AgentConnection::setupProxyConnection()
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_SETUP_PROXY_CONNECTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IP_ADDRESS, (UINT32)ntohl(m_dwAddr));
   msg.SetVariable(VID_AGENT_PORT, m_wPort);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, 60000);   // Wait 60 seconds for remote connect
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Enable trap receiving on connection
//

UINT32 AgentConnection::enableTraps()
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_ENABLE_AGENT_TRAPS);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Send custom request to agent
//

CSCPMessage *AgentConnection::customRequest(CSCPMessage *pRequest, const TCHAR *recvFile, bool appendFile,
														  void (*downloadProgressCallback)(size_t, void *), void *cbArg)
{
   UINT32 dwRqId, rcc;
	CSCPMessage *msg = NULL;

   dwRqId = m_dwRequestId++;
   pRequest->SetId(dwRqId);
	if (recvFile != NULL)
	{
		rcc = prepareFileDownload(recvFile, dwRqId, appendFile, downloadProgressCallback, cbArg);
		if (rcc != ERR_SUCCESS)
		{
			// Create fake response message
			msg = new CSCPMessage;
			msg->SetCode(CMD_REQUEST_COMPLETED);
			msg->SetId(dwRqId);
			msg->SetVariable(VID_RCC, rcc);
		}
	}

	if (msg == NULL)
	{
		if (sendMessage(pRequest))
		{
			msg = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
			if ((msg != NULL) && (recvFile != NULL))
			{
				if (msg->GetVariableLong(VID_RCC) == ERR_SUCCESS)
				{
					if (ConditionWait(m_condFileDownload, 1800000))	 // 30 min timeout
					{
						if (!m_fileDownloadSucceeded)
						{
							msg->SetVariable(VID_RCC, ERR_IO_FAILURE);
							if (m_deleteFileOnDownloadFailure)
								_tremove(recvFile);
						}
					}
					else
					{
						msg->SetVariable(VID_RCC, ERR_REQUEST_TIMEOUT);
					}
				}
				else
				{
					close(m_hCurrFile);
					m_hCurrFile = -1;
					_tremove(recvFile);
				}
			}
		}
	}

	return msg;
}


//
// Prepare for file upload
//

UINT32 AgentConnection::prepareFileDownload(const TCHAR *fileName, UINT32 rqId, bool append,
														 void (*downloadProgressCallback)(size_t, void *), void *cbArg)
{
	if (m_hCurrFile != -1)
		return ERR_RESOURCE_BUSY;

	nx_strncpy(m_currentFileName, fileName, MAX_PATH);
	ConditionReset(m_condFileDownload);
	m_hCurrFile = _topen(fileName, (append ? 0 : (O_CREAT | O_TRUNC)) | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
	if (m_hCurrFile == -1)
	{
		DbgPrintf(4, _T("AgentConnection::PrepareFileDownload(): cannot open file %s (%s); append=%d rqId=%d"),
		          fileName, _tcserror(errno), append, rqId);
	}
	else 
	{
		if (append)
			lseek(m_hCurrFile, 0, SEEK_END);
	}
	m_dwDownloadRequestId = rqId;
	m_downloadProgressCallback = downloadProgressCallback;
	m_downloadProgressCallbackArg = cbArg;
	return (m_hCurrFile != -1) ? ERR_SUCCESS : ERR_FILE_OPEN_ERROR;
}


//
// File upload completion handler
//

void AgentConnection::onFileDownload(BOOL success)
{
	if (!success && m_deleteFileOnDownloadFailure)
		_tremove(m_currentFileName);
	m_fileDownloadSucceeded = success;
	ConditionSet(m_condFileDownload);
}


//
// Enable trap receiving on connection
//

UINT32 AgentConnection::getPolicyInventory(AgentPolicyInfo **info)
{
   CSCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId, rcc;

	*info = NULL;
   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_GET_POLICY_INVENTORY);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
		if (response != NULL)
		{
			rcc = response->GetVariableLong(VID_RCC);
			if (rcc == ERR_SUCCESS)
				*info = new AgentPolicyInfo(response);
			delete response;
		}
		else
		{
	      rcc = ERR_REQUEST_TIMEOUT;
		}
	}
   else
	{
      rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}


//
// Uninstall policy by GUID
//

UINT32 AgentConnection::uninstallPolicy(uuid_t guid)
{
	UINT32 rqId, rcc;
	CSCPMessage msg(m_nProtocolVersion);

   rqId = generateRequestId();
   msg.SetId(rqId);
	msg.SetCode(CMD_UNINSTALL_AGENT_POLICY);
	msg.SetVariable(VID_GUID, guid, UUID_LENGTH);
	if (sendMessage(&msg))
	{
		rcc = waitForRCC(rqId, m_dwCommandTimeout);
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
   return rcc;
}

/**
 * Acquire encryption context
 */
NXCPEncryptionContext *AgentConnection::acquireEncryptionContext()
{
	lock();
	NXCPEncryptionContext *ctx = m_pCtx;
	if (ctx != NULL)
		ctx->incRefCount();
	unlock();
	return ctx;
}

/**
 * Create new agent parameter definition from NXCP message
 */
AgentParameterDefinition::AgentParameterDefinition(CSCPMessage *msg, UINT32 baseId)
{
   m_name = msg->GetVariableStr(baseId);
   m_description = msg->GetVariableStr(baseId + 1);
   m_dataType = (int)msg->GetVariableShort(baseId + 2);
}

/**
 * Create new agent parameter definition from another definition object
 */
AgentParameterDefinition::AgentParameterDefinition(AgentParameterDefinition *src)
{
   m_name = (src->m_name != NULL) ? _tcsdup(src->m_name) : NULL;
   m_description = (src->m_description != NULL) ? _tcsdup(src->m_description) : NULL;
   m_dataType = src->m_dataType;
}

/**
 * Destructor for agent parameter definition
 */
AgentParameterDefinition::~AgentParameterDefinition()
{
   safe_free(m_name);
   safe_free(m_description);
}

/**
 * Fill NXCP message
 */
UINT32 AgentParameterDefinition::fillMessage(CSCPMessage *msg, UINT32 baseId)
{
   msg->SetVariable(baseId, m_name);
   msg->SetVariable(baseId + 1, m_description);
   msg->SetVariable(baseId + 2, (WORD)m_dataType);
   return 3;
}

/**
 * Create new agent table definition from NXCP message
 */
AgentTableDefinition::AgentTableDefinition(CSCPMessage *msg, UINT32 baseId)
{
   m_name = msg->GetVariableStr(baseId);
   m_description = msg->GetVariableStr(baseId + 2);

   TCHAR *instanceColumns = msg->GetVariableStr(baseId + 1);
   if (instanceColumns != NULL)
   {
      m_instanceColumns = new StringList(instanceColumns, _T("|"));
      free(instanceColumns);
   }
   else
   {
      m_instanceColumns = new StringList;
   }

   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, true);
}

/**
 * Create new agent table definition from another definition object
 */
AgentTableDefinition::AgentTableDefinition(AgentTableDefinition *src)
{
   m_name = (src->m_name != NULL) ? _tcsdup(src->m_name) : NULL;
   m_description = (src->m_description != NULL) ? _tcsdup(src->m_description) : NULL;
   m_instanceColumns = new StringList(src->m_instanceColumns);
   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, true);
   for(int i = 0; i < src->m_columns->size(); i++)
   {
      m_columns->add(new AgentTableColumnDefinition(src->m_columns->get(i)));
   }
}
/**
 * Destructor for agent table definition
 */
AgentTableDefinition::~AgentTableDefinition()
{
   safe_free(m_name);
   safe_free(m_description);
   delete m_instanceColumns;
   delete m_columns;
}

/**
 * Fill NXCP message
 */
UINT32 AgentTableDefinition::fillMessage(CSCPMessage *msg, UINT32 baseId)
{
   msg->SetVariable(baseId + 1, m_name);
   msg->SetVariable(baseId + 2, m_description);

   TCHAR *instanceColumns = m_instanceColumns->join(_T("|"));
   msg->SetVariable(baseId + 3, instanceColumns);
   free(instanceColumns);

   UINT32 varId = baseId + 4;
   for(int i = 0; i < m_columns->size(); i++)
   {
      msg->SetVariable(varId++, m_columns->get(i)->m_name);
      msg->SetVariable(varId++, (WORD)m_columns->get(i)->m_dataType);
   }

   msg->SetVariable(baseId, varId - baseId);
   return varId - baseId;
}
