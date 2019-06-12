#include "nxuseragent.h"

/**
 * Connection state
 */
bool g_connectedToAgent = false;

/**
 * Connection port
 */
static UINT16 s_port = 28180;

/**
 * Socket
 */
static SOCKET s_socket = INVALID_SOCKET;

/**
 * Socket lock
 */
static MUTEX s_socketLock = MutexCreate();

/**
 * Protocol buffer
 */
static NXCP_BUFFER s_msgBuffer;

/**
 * Connect to master agent
 */
static bool ConnectToMasterAgent()
{
   nxlog_debug(7, _T("Connecting to master agent"));
   s_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (s_socket == INVALID_SOCKET)
   {
      nxlog_debug(5, _T("Call to socket() failed\n"));
      return false;
   }

   // Fill in address structure
   struct sockaddr_in sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = inet_addr("127.0.0.1");
   sa.sin_port = htons(s_port);

   // Connect to server
   if (ConnectEx(s_socket, (struct sockaddr *)&sa, sizeof(sa), 5000) == -1)
   {
      nxlog_debug(5, _T("Cannot establish connection with master agent"));
      closesocket(s_socket);
      s_socket = INVALID_SOCKET;
      return false;
   }

   return true;
}

/**
 * Send message to master agent
 */
static bool SendMessageToAgent(NXCPMessage *msg)
{
   if (s_socket == INVALID_SOCKET)
      return false;

   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = (SendEx(s_socket, rawMsg, ntohl(rawMsg->size), 0, s_socketLock) == ntohl(rawMsg->size));
   MemFree(rawMsg);
   return success;
}

/**
 * Send login message
 */
void SendLoginMessage()
{
   UserSession session;
   GetSessionInformation(&session);

   NXCPMessage msg(CMD_LOGIN, 0);
   msg.setField(VID_SESSION_ID, (UINT32)session.sid);
   msg.setField(VID_SESSION_STATE, (INT16)session.state);
   msg.setField(VID_NAME, session.name);
   msg.setField(VID_USER_NAME, session.user);
   msg.setField(VID_CLIENT_INFO, session.client);
   msg.setField(VID_USERAGENT, true);
   SendMessageToAgent(&msg);
}

/**
 * Shutdown user agent
 */
static void ShutdownAgent(bool restart)
{
   nxlog_debug(1, _T("Shutdown request with restart option %s"), restart ? _T("ON") : _T("OFF"));

   if (restart)
   {
      TCHAR path[MAX_PATH];
      GetNetXMSDirectory(nxDirBin, path);

      TCHAR exe[MAX_PATH];
      _tcscpy(exe, path);
      _tcslcat(exe, _T("\\nxreload.exe"), MAX_PATH);
      if (VerifyFileSignature(exe))
      {
         String command;
         command.append(_T('"'));
         command.append(exe);
         command.append(_T("\" -- \""));
         GetModuleFileName(NULL, path, MAX_PATH);
         command.append(path);
         command.append(_T('"'));

         PROCESS_INFORMATION pi;
         STARTUPINFO si;
         memset(&si, 0, sizeof(STARTUPINFO));
         si.cb = sizeof(STARTUPINFO);

         nxlog_debug(3, _T("Starting reload helper:"));
         nxlog_debug(3, _T("%s"), command.getBuffer());
         if (CreateProcess(NULL, command.getBuffer(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
         {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
         }
      }
      else
      {
         nxlog_debug(1, _T("Cannot verify signature of reload helper %s"), exe);
      }
   }

   PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
}

/** 
 * Process request from master agent
 */
static void ProcessRequest(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   switch (request->getCode())
   {
      case CMD_KEEPALIVE:
         msg.setField(VID_RCC, ERR_SUCCESS);
         break;
      case CMD_TAKE_SCREENSHOT:
         TakeScreenshot(&msg);
         break;
      case CMD_UPDATE_AGENT_CONFIG:
         UpdateConfig(request);
         break;
      case CMD_UPDATE_USER_AGENT_MESSAGES:
         UpdateNotifications(request);
         break;
      case CMD_SHUTDOWN:
         ShutdownAgent(request->getFieldAsBoolean(VID_RESTART));
         break;
      default:
         msg.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
         break;
   }

   SendMessageToAgent(&msg);
}

/**
 * Message processing loop
 */
static void ProcessMessages()
{
   SocketMessageReceiver receiver(s_socket, 4096, 256 * 1024);
   while (true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      // Check for decryption error
      if (result == MSGRECV_DECRYPTION_FAILURE)
      {
         nxlog_debug(4, _T("Unable to decrypt received message"));
         continue;
      }

      // Receive error
      if (msg == NULL)
      {
         if (result == MSGRECV_CLOSED)
            nxlog_debug(5, _T("Connection with master agent closed"));
         else
            nxlog_debug(5, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (!msg->isBinary())
      {
         TCHAR msgCodeName[256];
         nxlog_debug(5, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), msgCodeName));
         ProcessRequest(msg);
      }
      delete msg;
   }
}

/**
 * Stop flag
 */
static bool s_stop = false;

/**
 * Communication thread
 */
static THREAD_RESULT THREAD_CALL CommThread(void *arg)
{
   nxlog_debug(1, _T("Communication thread started"));
   while(!s_stop)
   {
      if (!ConnectToMasterAgent())
      {
         ThreadSleep(30);
         continue;
      }

      nxlog_debug(1, _T("Connected to master agent"));
      g_connectedToAgent = true;
      SendLoginMessage();
      ProcessMessages();
      g_connectedToAgent = false;
   }
   nxlog_debug(1, _T("Communication thread stopped"));
   return THREAD_OK;
}

/**
 * Start agent connector
 */
void StartAgentConnector()
{
   ThreadCreate(CommThread, 0, NULL);
}

/**
 * Stop agent connector
 */
void StopAgentConnector()
{
   s_stop = true;
   closesocket(s_socket);
}
