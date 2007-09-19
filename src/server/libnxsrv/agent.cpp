/* $Id: agent.cpp,v 1.52 2007-09-19 16:57:42 victor Exp $ */
/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
** File: agent.cpp
**
**/

#include "libnxsrv.h"
#include <stdarg.h>


//
// Constants
//

#define RECEIVER_BUFFER_SIZE        262144


//
// Static data
//

#ifdef _WITH_ENCRYPTION
static int m_iDefaultEncryptionPolicy = ENCRYPTION_ALLOWED;
#else
static int m_iDefaultEncryptionPolicy = ENCRYPTION_DISABLED;
#endif


//
// Set default encryption policy for agent communication
//

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy)
{
#ifdef _WITH_ENCRYPTION
   m_iDefaultEncryptionPolicy = iPolicy;
#endif
}


//
// Receiver thread starter
//

THREAD_RESULT THREAD_CALL AgentConnection::ReceiverThreadStarter(void *pArg)
{
   ((AgentConnection *)pArg)->ReceiverThread();
   return THREAD_OK;
}


//
// Default constructor for AgentConnection - normally shouldn't be used
//

AgentConnection::AgentConnection()
{
   m_dwAddr = inet_addr("127.0.0.1");
   m_wPort = AGENT_LISTEN_PORT;
   m_iAuthMethod = AUTH_NONE;
   m_szSecret[0] = 0;
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_dwRequestId = 1;
   m_dwCommandTimeout = 10000;   // Default timeout 10 seconds
   m_bIsConnected = FALSE;
   m_mutexDataLock = MutexCreate();
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_pCtx = NULL;
   m_iEncryptionPolicy = m_iDefaultEncryptionPolicy;
   m_bUseProxy = FALSE;
   m_dwRecvTimeout = 420000;  // 7 minutes
   m_nProtocolVersion = NXCP_VERSION;
}


//
// Normal constructor for AgentConnection
//

AgentConnection::AgentConnection(DWORD dwAddr, WORD wPort,
                                 int iAuthMethod, const TCHAR *pszSecret)
{
   m_dwAddr = dwAddr;
   m_wPort = wPort;
   m_iAuthMethod = iAuthMethod;
   if (pszSecret != NULL)
   {
#ifdef UNICODE
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
                          pszSecret, -1, m_szSecret, MAX_SECRET_LENGTH, NULL, NULL);
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
   m_dwCommandTimeout = 10000;   // Default timeout 10 seconds
   m_bIsConnected = FALSE;
   m_mutexDataLock = MutexCreate();
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_pCtx = NULL;
   m_iEncryptionPolicy = m_iDefaultEncryptionPolicy;
   m_bUseProxy = FALSE;
   m_dwRecvTimeout = 420000;  // 7 minutes
   m_nProtocolVersion = NXCP_VERSION;
}


//
// Destructor
//

AgentConnection::~AgentConnection()
{
   // Disconnect from peer
   Disconnect();

   // Wait for receiver thread termination
   ThreadJoin(m_hReceiverThread);
   if (m_hSocket != -1)
      closesocket(m_hSocket);

   Lock();
   DestroyResultData();
   Unlock();

   delete m_pMsgWaitQueue;
   DestroyEncryptionContext(m_pCtx);

   MutexDestroy(m_mutexDataLock);
}


//
// Print message. This function is virtual and can be overrided in
// derived classes. Default implementation will print message to stdout.
//

void AgentConnection::PrintMsg(const TCHAR *pszFormat, ...)
{
   va_list args;

   va_start(args, pszFormat);
   _vtprintf(pszFormat, args);
   va_end(args);
   _tprintf(_T("\n"));
}


//
// Receiver thread
//

void AgentConnection::ReceiverThread(void)
{
   CSCPMessage *pMsg;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pMsgBuffer;
   BYTE *pDecryptionBuffer = NULL;
   int iErr;
   TCHAR szBuffer[128];

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pMsgBuffer, 0, NULL, NULL, 0);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)malloc(RECEIVER_BUFFER_SIZE);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(RECEIVER_BUFFER_SIZE);
#endif

   // Message receiving loop
   while(1)
   {
      // Receive raw message
      if ((iErr = RecvNXCPMessage(m_hSocket, pRawMsg, pMsgBuffer, RECEIVER_BUFFER_SIZE,
                                  &m_pCtx, pDecryptionBuffer, m_dwRecvTimeout)) <= 0)
         break;

      // Check if we get too large message
      if (iErr == 1)
      {
         PrintMsg(_T("Received too large message %s (%d bytes)"), 
                  NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                  ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check if we are unable to decrypt message
      if (iErr == 2)
      {
         PrintMsg(_T("Unable to decrypt received message"));
         continue;
      }

      // Check for timeout
      if (iErr == 3)
      {
         PrintMsg(_T("Timed out waiting for message"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         PrintMsg(_T("RecvMsg: Bad packet length [dwSize=%d ActualSize=%d]"), ntohl(pRawMsg->dwSize), iErr);
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      pMsg = new CSCPMessage(pRawMsg, m_nProtocolVersion);
      if (pMsg->GetCode() == CMD_TRAP)
      {
         OnTrap(pMsg);
         delete pMsg;
      }
      else
      {
         m_pMsgWaitQueue->Put(pMsg);
      }
   }

   // Close socket and mark connection as disconnected
   Lock();
   if (iErr == 0)
      shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   m_hSocket = -1;
   DestroyEncryptionContext(m_pCtx);
   m_pCtx = NULL;
   m_bIsConnected = FALSE;
   Unlock();

   free(pRawMsg);
   free(pMsgBuffer);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif
}


//
// Connect to agent
//

BOOL AgentConnection::Connect(RSA *pServerKey, BOOL bVerbose, DWORD *pdwError)
{
   struct sockaddr_in sa;
   TCHAR szBuffer[256];
   BOOL bSuccess = FALSE, bForceEncryption = FALSE, bSecondPass = FALSE;
   DWORD dwError = 0;

   if (pdwError != NULL)
      *pdwError = ERR_INTERNAL_ERROR;

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
      PrintMsg(_T("Call to socket() failed"));
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
   if (connect(m_hSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      if (bVerbose)
         PrintMsg(_T("Cannot establish connection with agent %s"),
                  IpToStr(ntohl(m_bUseProxy ? m_dwProxyAddr : m_dwAddr), szBuffer));
      dwError = ERR_CONNECT_FAILED;
      goto connect_cleanup;
   }

   if (!NXCPGetPeerProtocolVersion(m_hSocket, &m_nProtocolVersion))
   {
      dwError = ERR_INTERNAL_ERROR;
      goto connect_cleanup;
   }

   // Start receiver thread
   m_hReceiverThread = ThreadCreateEx(ReceiverThreadStarter, 0, this);

   // Setup encryption
setup_encryption:
   if ((m_iEncryptionPolicy == ENCRYPTION_PREFERRED) ||
       (m_iEncryptionPolicy == ENCRYPTION_REQUIRED) ||
       (bForceEncryption))    // Agent require encryption
   {
      if (pServerKey != NULL)
      {
         dwError = SetupEncryption(pServerKey);
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
   if ((dwError = Authenticate(m_bUseProxy && !bSecondPass)) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         bForceEncryption = TRUE;
         goto setup_encryption;
      }
      PrintMsg(_T("Authentication to agent %s failed (%s)"), IpToStr(ntohl(m_dwAddr), szBuffer),
               AgentErrorCodeToText(dwError));
      goto connect_cleanup;
   }

   // Test connectivity
   if ((dwError = Nop()) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         bForceEncryption = TRUE;
         goto setup_encryption;
      }
      PrintMsg(_T("Communication with agent %s failed (%s)"), IpToStr(ntohl(m_dwAddr), szBuffer),
               AgentErrorCodeToText(dwError));
      goto connect_cleanup;
   }

   if (m_bUseProxy && !bSecondPass)
   {
      dwError = SetupProxyConnection();
      if (dwError != ERR_SUCCESS)
         goto connect_cleanup;
      DestroyEncryptionContext(m_pCtx);
      m_pCtx = NULL;
      bSecondPass = TRUE;
      bForceEncryption = FALSE;
      goto setup_encryption;
   }

   bSuccess = TRUE;
   dwError = ERR_SUCCESS;

connect_cleanup:
   if (!bSuccess)
   {
      Lock();
      if (m_hSocket != -1)
         shutdown(m_hSocket, SHUT_RDWR);
      Unlock();
      ThreadJoin(m_hReceiverThread);
      m_hReceiverThread = INVALID_THREAD_HANDLE;

      Lock();
      if (m_hSocket != -1)
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
      }

      DestroyEncryptionContext(m_pCtx);
      m_pCtx = NULL;

      Unlock();
   }
   m_bIsConnected = bSuccess;
   if (pdwError != NULL)
      *pdwError = dwError;
   return bSuccess;
}


//
// Disconnect from agent
//

void AgentConnection::Disconnect(void)
{
   Lock();
   if (m_hSocket != -1)
   {
      shutdown(m_hSocket, SHUT_RDWR);
   }
   DestroyResultData();
   m_bIsConnected = FALSE;
   Unlock();
}


//
// Destroy command execuion results data
//

void AgentConnection::DestroyResultData(void)
{
   DWORD i;

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


//
// Get interface list from agent
//

INTERFACE_LIST *AgentConnection::GetInterfaceList(void)
{
   INTERFACE_LIST *pIfList = NULL;
   DWORD i, dwBits;
   TCHAR *pChar, *pBuf;

   if (GetList(_T("Net.InterfaceList")) == ERR_SUCCESS)
   {
      pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
      pIfList->iNumEntries = m_dwNumDataLines;
      pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * m_dwNumDataLines);
      memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * m_dwNumDataLines);

      // Parse result set. Each line should have the following format:
      // index ip_address/mask_bits iftype mac_address name
      for(i = 0; i < m_dwNumDataLines; i++)
      {
         pBuf = m_ppDataLines[i];

         // Index
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            pIfList->pInterfaces[i].dwIndex = _tcstoul(pBuf, NULL, 10);
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
            pIfList->pInterfaces[i].dwIpAddr = ntohl(_t_inet_addr(pBuf));
            dwBits = _tcstoul(pSlash, NULL, 10);
            pIfList->pInterfaces[i].dwIpNetMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
            pBuf = pChar + 1;
         }

         // Interface type
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            pIfList->pInterfaces[i].dwType = _tcstoul(pBuf, NULL, 10);
            pBuf = pChar + 1;
         }

         // MAC address
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            StrToBin(pBuf, pIfList->pInterfaces[i].bMacAddr, MAC_ADDR_LENGTH);
            pBuf = pChar + 1;
         }

         // Name
         nx_strncpy(pIfList->pInterfaces[i].szName, pBuf, MAX_OBJECT_NAME - 1);
      }

      Lock();
      DestroyResultData();
      Unlock();
   }

   return pIfList;
}


//
// Get parameter value
//

DWORD AgentConnection::GetParameter(const TCHAR *pszParam, DWORD dwBufSize, TCHAR *pszBuffer)
{
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   DWORD dwRqId, dwRetCode;

   if (m_bIsConnected)
   {
      dwRqId = m_dwRequestId++;
      msg.SetCode(CMD_GET_PARAMETER);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_PARAMETER, pszParam);
      if (SendMessage(&msg))
      {
         pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
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

ARP_CACHE *AgentConnection::GetArpCache(void)
{
   ARP_CACHE *pArpCache = NULL;
   TCHAR szByte[4], *pBuf, *pChar;
   DWORD i, j;

   if (GetList(_T("Net.ArpCache")) == ERR_SUCCESS)
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

      Lock();
      DestroyResultData();
      Unlock();
   }
   return pArpCache;
}


//
// Send dummy command to agent (can be used for keepalive)
//

DWORD AgentConnection::Nop(void)
{
   CSCPMessage msg(m_nProtocolVersion);
   DWORD dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_KEEPALIVE);
   msg.SetId(dwRqId);
   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Wait for request completion code
//

DWORD AgentConnection::WaitForRCC(DWORD dwRqId, DWORD dwTimeOut)
{
   CSCPMessage *pMsg;
   DWORD dwRetCode;

   pMsg = m_pMsgWaitQueue->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
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

BOOL AgentConnection::SendMessage(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   BOOL bResult;

   pRawMsg = pMsg->CreateMessage();
   if (m_pCtx != NULL)
   {
      pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0) == (int)ntohl(pRawMsg->dwSize));
   }
   free(pRawMsg);
   return bResult;
}


//
// Trap handler. Should be overriden in derived classes to implement
// actual trap processing. Default implementation do nothing.
//

void AgentConnection::OnTrap(CSCPMessage *pMsg)
{
}


//
// Get list of values
//

DWORD AgentConnection::GetList(const TCHAR *pszParam)
{
   CSCPMessage msg(m_nProtocolVersion), *pResponse;
   DWORD i, dwRqId, dwRetCode;

   if (m_bIsConnected)
   {
      DestroyResultData();
      dwRqId = m_dwRequestId++;
      msg.SetCode(CMD_GET_LIST);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_PARAMETER, pszParam);
      if (SendMessage(&msg))
      {
         pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
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


//
// Authenticate to agent
//

DWORD AgentConnection::Authenticate(BOOL bProxyData)
{
   CSCPMessage msg(m_nProtocolVersion);
   DWORD dwRqId;
   BYTE hash[32];
   int iAuthMethod = bProxyData ? m_iProxyAuth : m_iAuthMethod;
   char *pszSecret = bProxyData ? m_szProxySecret : m_szSecret;
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
   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Execute action on agent
//

DWORD AgentConnection::ExecAction(TCHAR *pszAction, int argc, TCHAR **argv)
{
   CSCPMessage msg(m_nProtocolVersion);
   DWORD dwRqId;
   int i;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_ACTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_ACTION_NAME, pszAction);
   msg.SetVariable(VID_NUM_ARGS, (DWORD)argc);
   for(i = 0; i < argc; i++)
      msg.SetVariable(VID_ACTION_ARG_BASE + i, argv[i]);

   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Upload file to agent
//

DWORD AgentConnection::UploadFile(TCHAR *pszFile)
{
   DWORD dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion);
   int i;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_TRANSFER_FILE);
   msg.SetId(dwRqId);
   for(i = (int)_tcslen(pszFile) - 1; 
       (i >= 0) && (pszFile[i] != '\\') && (pszFile[i] != '/'); i--);
   msg.SetVariable(VID_FILE_NAME, &pszFile[i + 1]);

   if (SendMessage(&msg))
   {
      dwResult = WaitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   if (dwResult == ERR_SUCCESS)
   {
      if (SendFileOverNXCP(m_hSocket, dwRqId, pszFile, m_pCtx))
         dwResult = WaitForRCC(dwRqId, m_dwCommandTimeout);
      else
         dwResult = ERR_IO_FAILURE;
   }

   return dwResult;
}


//
// Send upgrade command
//

DWORD AgentConnection::StartUpgrade(TCHAR *pszPkgName)
{
   DWORD dwRqId, dwResult;
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

   if (SendMessage(&msg))
   {
      dwResult = WaitForRCC(dwRqId, m_dwCommandTimeout);
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

DWORD AgentConnection::CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, 
                                           WORD wPort, WORD wProto, 
                                           TCHAR *pszRequest, TCHAR *pszResponse)
{
   DWORD dwRqId, dwResult;
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

   if (SendMessage(&msg))
   {
      // Wait up to 90 seconds for results
      pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 90000);
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


//
// Get list of supported parameters from subagent
//

DWORD AgentConnection::GetSupportedParameters(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList)
{
   DWORD i, dwId, dwRqId, dwResult;
   CSCPMessage msg(m_nProtocolVersion), *pResponse;

   *pdwNumParams = 0;
   *ppParamList = NULL;

   if (!m_bIsConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = m_dwRequestId++;

   msg.SetCode(CMD_GET_PARAMETER_LIST);
   msg.SetId(dwRqId);

   if (SendMessage(&msg))
   {
      pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponse != NULL)
      {
         dwResult = pResponse->GetVariableLong(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            *pdwNumParams = pResponse->GetVariableLong(VID_NUM_PARAMETERS);
            *ppParamList = (NXC_AGENT_PARAM *)malloc(sizeof(NXC_AGENT_PARAM) * *pdwNumParams);
            for(i = 0, dwId = VID_PARAM_LIST_BASE; i < *pdwNumParams; i++)
            {
               pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szName, MAX_PARAM_NAME);
               pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szDescription, MAX_DB_STRING);
               (*ppParamList)[i].iDataType = (int)pResponse->GetVariableShort(dwId++);
            }
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
// Setup encryption
//

DWORD AgentConnection::SetupEncryption(RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   CSCPMessage msg(m_nProtocolVersion), *pResp;
   DWORD dwRqId, dwError, dwResult;

   dwRqId = m_dwRequestId++;

   PrepareKeyRequestMsg(&msg, pServerKey);
   msg.SetId(dwRqId);
   if (SendMessage(&msg))
   {
      pResp = WaitForMessage(CMD_SESSION_KEY, dwRqId, m_dwCommandTimeout);
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

DWORD AgentConnection::GetConfigFile(TCHAR **ppszConfig, DWORD *pdwSize)
{
   DWORD i, dwRqId, dwResult;
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

   if (SendMessage(&msg))
   {
      pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
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
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pBuffer, *pdwSize, *ppszConfig, *pdwSize);
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

DWORD AgentConnection::UpdateConfigFile(TCHAR *pszConfig)
{
   DWORD dwRqId, dwResult;
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
   nChars = _tcslen(pszConfig);
   pBuffer = (BYTE *)malloc(nChars + 1);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszConfig, nChars, pBuffer, nChars + 1, NULL, NULL);
   msg.SetVariable(VID_CONFIG_FILE, pBuffer, nChars);
   free(pBuffer);
#else
   msg.SetVariable(VID_CONFIG_FILE, (BYTE *)pszConfig, (DWORD)strlen(pszConfig));
#endif

   if (SendMessage(&msg))
   {
      dwResult = WaitForRCC(dwRqId, m_dwCommandTimeout);
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

ROUTING_TABLE *AgentConnection::GetRoutingTable(void)
{
   ROUTING_TABLE *pRT = NULL;
   DWORD i, dwBits;
   TCHAR *pChar, *pBuf;

   if (GetList(_T("Net.IP.RoutingTable")) == ERR_SUCCESS)
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

      Lock();
      DestroyResultData();
      Unlock();
   }

   return pRT;
}


//
// Set proxy information
//

void AgentConnection::SetProxy(DWORD dwAddr, WORD wPort, int iAuthMethod, TCHAR *pszSecret)
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

DWORD AgentConnection::SetupProxyConnection(void)
{
   CSCPMessage msg(m_nProtocolVersion);
   DWORD dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_SETUP_PROXY_CONNECTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_IP_ADDRESS, (DWORD)ntohl(m_dwAddr));
   msg.SetVariable(VID_AGENT_PORT, m_wPort);
   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, 60000);   // Wait 60 seconds for remote connect
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Enable trap receiving on connection
//

DWORD AgentConnection::EnableTraps(void)
{
   CSCPMessage msg(m_nProtocolVersion);
   DWORD dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_ENABLE_AGENT_TRAPS);
   msg.SetId(dwRqId);
   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Send custom request to agent
//

CSCPMessage *AgentConnection::CustomRequest(CSCPMessage *pRequest)
{
   DWORD dwRqId;

   dwRqId = m_dwRequestId++;
   pRequest->SetId(dwRqId);
   if (SendMessage(pRequest))
      return WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
   else
      return NULL;
}
