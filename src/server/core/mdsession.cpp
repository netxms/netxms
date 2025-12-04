/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <netxms-version.h>
#include <nms_users.h>

#ifdef _WIN32
#include <psapi.h>
#endif

/**
 * Max message size
 */
#define MAX_MSG_SIZE    65536

/**
 * Communication protocol name for mobile devices
 */
#define COMM_PROTO   _T("NXCP")

/**
 * Externals
 */
void UnregisterMobileDeviceSession(int id);

/**
 * Mobile device session class constructor
 */
MobileDeviceSession::MobileDeviceSession(SOCKET hSocket, const InetAddress& addr) : m_condEncryptionSetup(false)
{
   m_socket = hSocket;
   m_id = -1;
	m_clientAddr = addr;
	m_clientAddr.toString(m_hostName);
   _tcscpy(m_userName, _T("<not logged in>"));
	_tcscpy(m_clientInfo, _T("n/a"));
   m_userId = INVALID_INDEX;
	m_deviceObjectId = 0;
   m_encryptionRqId = 0;
   m_encryptionResult = 0;
   m_refCount = 0;
	m_authenticated = false;
}

/**
 * Destructor
 */
MobileDeviceSession::~MobileDeviceSession()
{
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
}

/**
 * Start all threads
 */
void MobileDeviceSession::run()
{
   ThreadCreate([this]() {
      this->readThread();
      // When MobileDeviceSession::readThread exits, all other session
      // threads are already stopped, so we can safely destroy
      // session object
      UnregisterMobileDeviceSession(this->getId());
      delete this;
   });
}

/**
 * Print debug information
 */
void MobileDeviceSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(_T("mobile.session"), m_id, level, format, args);
   va_end(args);
}

/**
 * Read thread
 */
void MobileDeviceSession::readThread()
{
   SocketMessageReceiver receiver(m_socket, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      // Check for decryption error
      if (result == MSGRECV_DECRYPTION_FAILURE)
      {
         debugPrintf(4, _T("Unable to decrypt received message"));
         continue;
      }

      // Receive error
      if (msg == nullptr)
      {
         debugPrintf(5, _T("readThread: message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (nxlog_get_debug_level() >= 8)
      {
         String msgDump = NXCPMessage::dump(receiver.getRawMessageBuffer(), NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }

      TCHAR szBuffer[256];
      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), szBuffer));
      if ((msg->getCode() == CMD_SESSION_KEY) && (msg->getId() == m_encryptionRqId))
      {
         NXCPEncryptionContext *encryptionContext = nullptr;
         m_encryptionResult = SetupEncryptionContext(msg, &encryptionContext, nullptr, g_serverKey, NXCP_VERSION);
         m_encryptionContext = shared_ptr<NXCPEncryptionContext>(encryptionContext);
         receiver.setEncryptionContext(m_encryptionContext);
         m_condEncryptionSetup.set();
         m_encryptionRqId = 0;
         delete msg;
      }
      else if (msg->getCode() == CMD_KEEPALIVE)
		{
			respondToKeepalive(msg->getId());
			delete msg;
		}
		else
      {
		   ThreadPoolExecute(g_mobileThreadPool, this, &MobileDeviceSession::processRequest, msg);
      }
   }

   // Waiting while reference count becomes 0
   if (m_refCount > 0)
   {
      debugPrintf(3, _T("Waiting for pending requests..."));
      do
      {
         ThreadSleep(1);
      } while(m_refCount > 0);
   }

	WriteAuditLog(AUDIT_SECURITY, TRUE, m_userId, m_hostName, m_id, 0, _T("Mobile device logged out (client: %s)"), m_clientInfo);
   debugPrintf(3, _T("Session closed"));
}

/**
 * Message processing thread
 */
void MobileDeviceSession::processRequest(NXCPMessage *request)
{
   uint16_t command = request->getCode();
   if (!m_authenticated &&
       (command != CMD_LOGIN) &&
       (command != CMD_GET_SERVER_INFO) &&
       (command != CMD_REQUEST_ENCRYPTION))
   {
      delete request;
      return;
   }

   switch(command)
   {
      case CMD_GET_SERVER_INFO:
         sendServerInfo(request->getId());
         break;
      case CMD_LOGIN:
         login(*request);
         break;
      case CMD_REQUEST_ENCRYPTION:
         setupEncryption(request);
         break;
      case CMD_REPORT_DEVICE_INFO:
         updateDeviceInfo(request);
         break;
      case CMD_REPORT_DEVICE_STATUS:
         updateDeviceStatus(request);
         break;
      case CMD_PUSH_DCI_DATA:
         pushData(request);
         break;
      default:
         // Pass message to loaded modules
         int status = NXMOD_COMMAND_IGNORED;
         ENUMERATE_MODULES(pfMobileDeviceCommandHandler)
         {
            status = CURRENT_MODULE.pfMobileDeviceCommandHandler(command, *request, this);
            if (status != NXMOD_COMMAND_IGNORED)
            {
               if (status == NXMOD_COMMAND_ACCEPTED_ASYNC)
               {
                  request = nullptr;	// Prevent deletion
               }
               break;   // Message was processed by the module
            }
         }
         if (status == NXMOD_COMMAND_IGNORED)
         {
            NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
            response.setField(VID_RCC, RCC_NOT_IMPLEMENTED);
            sendMessage(response);
         }
         break;
   }
   delete request;
}

/**
 * Respond to client's keepalive message
 */
void MobileDeviceSession::respondToKeepalive(uint32_t requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(msg);
}

/**
 * Send message to client
 */
void MobileDeviceSession::sendMessage(const NXCPMessage& msg)
{
   TCHAR szBuffer[128];
	debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg.getCode(), szBuffer));
	NXCP_MESSAGE *rawMsg = msg.serialize();
   if (nxlog_get_debug_level() >= 8)
   {
      String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }

   bool success;
   if (m_encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *encryptedMsg = m_encryptionContext->encryptMessage(rawMsg);
      if (encryptedMsg != nullptr)
      {
         success = (SendEx(m_socket, (char *)encryptedMsg, ntohl(encryptedMsg->size), 0, &m_mutexSocketWrite) == (int)ntohl(encryptedMsg->size));
         MemFree(encryptedMsg);
      }
      else
      {
         success = false;
      }
   }
   else
   {
      success = (SendEx(m_socket, (const char *)rawMsg, ntohl(rawMsg->size), 0, &m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   MemFree(rawMsg);

   if (!success)
   {
      closesocket(m_socket);
      m_socket = INVALID_SOCKET;
   }
}

/**
 * Send server information to client
 */
void MobileDeviceSession::sendServerInfo(uint32_t requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);

	// Generate challenge for certificate authentication
	RAND_bytes(m_challenge, CLIENT_CHALLENGE_SIZE);

   // Fill message with server info
   msg.setField(VID_RCC, RCC_SUCCESS);
   msg.setField(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_PROTOCOL_VERSION, (UINT32)MOBILE_DEVICE_PROTOCOL_VERSION);
	msg.setField(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);

   // Send response
   sendMessage(msg);
}

/**
 * Authenticate client
 */
void MobileDeviceSession::login(const NXCPMessage& request)
{
   TCHAR szLogin[MAX_USER_NAME], szPassword[1024];
	int nAuthType;
   bool changePasswd = false, intruderLockout = false, closeOtherSessions = false;
   UINT32 dwResult;
	X509 *pCert;

   NXCPMessage msg(CMD_LOGIN_RESPONSE, request.getId());

   // Get client info string
   if (request.isFieldExist(VID_CLIENT_INFO))
   {
      TCHAR clientInfo[32], osInfo[32], libVersion[16];
      request.getFieldAsString(VID_CLIENT_INFO, clientInfo, 32);
      request.getFieldAsString(VID_OS_INFO, osInfo, 32);
      request.getFieldAsString(VID_LIBNXCL_VERSION, libVersion, 16);
      _sntprintf(m_clientInfo, 96, _T("%s (%s; libnxcl %s)"), clientInfo, osInfo, libVersion);
   }

   if (!m_authenticated)
   {
      request.getFieldAsString(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
		nAuthType = (int)request.getFieldAsUInt16(VID_AUTH_TYPE);
		uint64_t userRights;
		uint32_t graceLogins;
		switch(nAuthType)
		{
			case NETXMS_AUTH_TYPE_PASSWORD:
				request.getFieldAsString(VID_PASSWORD, szPassword, 256);
				dwResult = AuthenticateUser(szLogin, szPassword, 0, nullptr, nullptr, &m_userId,
													 &userRights, &changePasswd, &intruderLockout,
													 &closeOtherSessions, false, &graceLogins);
				break;
			case NETXMS_AUTH_TYPE_CERTIFICATE:
				pCert = CertificateFromLoginMessage(request);
				if (pCert != nullptr)
				{
               size_t sigLen;
					const BYTE *signature = request.getBinaryFieldPtr(VID_SIGNATURE, &sigLen);
               if (signature != nullptr)
               {
                  dwResult = AuthenticateUser(szLogin, reinterpret_cast<const TCHAR *>(signature), sigLen,
                     pCert, m_challenge, &m_userId, &userRights,
                     &changePasswd, &intruderLockout,
                     &closeOtherSessions, false, &graceLogins);
               }
               else
               {
                  dwResult = RCC_INVALID_REQUEST;
               }
					X509_free(pCert);
				}
				else
				{
					dwResult = RCC_BAD_CERTIFICATE;
				}
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
				request.getFieldAsString(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
				shared_ptr<MobileDevice> md = FindMobileDeviceByDeviceID(deviceId);
				if (md != nullptr)
				{
					m_deviceObjectId = md->getId();
					m_authenticated = true;
					_sntprintf(m_userName, MAX_SESSION_NAME, _T("%s@%s"), szLogin, m_hostName);
					msg.setField(VID_RCC, RCC_SUCCESS);
					msg.setField(VID_USER_SYS_RIGHTS, userRights);
					msg.setField(VID_USER_ID, m_userId);
					msg.setField(VID_CHANGE_PASSWD_FLAG, (WORD)changePasswd);
					msg.setField(VID_DBCONN_STATUS, (WORD)((g_flags & AF_DB_CONNECTION_LOST) ? 0 : 1));
					msg.setField(VID_ZONING_ENABLED, (WORD)((g_flags & AF_ENABLE_ZONING) ? 1 : 0));
					debugPrintf(3, _T("User %s authenticated as mobile device"), m_userName);
					WriteAuditLog(AUDIT_SECURITY, true, m_userId, m_hostName, m_id, 0,
									  _T("Mobile device logged in as user \"%s\" (client info: %s)"), szLogin, m_clientInfo);
				}
				else
				{
					debugPrintf(3, _T("Mobile device object with device ID \"%s\" not found"), deviceId);
					msg.setField(VID_RCC, RCC_ACCESS_DENIED);
					WriteAuditLog(AUDIT_SECURITY, false, m_userId, m_hostName, m_id, 0,
									  _T("Mobile device login as user \"%s\" failed - mobile device object not found (client info: %s)"),
									  szLogin, m_clientInfo);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_ACCESS_DENIED);
				WriteAuditLog(AUDIT_SECURITY, false, m_userId, m_hostName, m_id, 0,
								  _T("Mobile device login as user \"%s\" failed - user does not have mobile device login rights (client info: %s)"),
								  szLogin, m_clientInfo);
			}
      }
      else
      {
         msg.setField(VID_RCC, dwResult);
			WriteAuditLog(AUDIT_SECURITY, FALSE, m_userId, m_hostName, m_id, 0,
			              _T("Mobile device login as user \"%s\" failed with error code %d (client info: %s)"),
							  szLogin, dwResult, m_clientInfo);
			if (intruderLockout)
			{
				WriteAuditLog(AUDIT_SECURITY, FALSE, m_userId, m_hostName, m_id, 0,
								  _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), szLogin);
			}
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   // Send response
   sendMessage(msg);
}

/**
 * Setup encryption with client
 */
void MobileDeviceSession::setupEncryption(NXCPMessage *request)
{
   NXCPMessage msg;

	m_encryptionRqId = request->getId();
   m_encryptionResult = RCC_TIMEOUT;

   // Send request for session key
	PrepareKeyRequestMsg(&msg, g_serverKey, request->getFieldAsUInt16(VID_USE_X509_KEY_FORMAT) != 0);
	msg.setId(request->getId());
   sendMessage(msg);
   msg.deleteAllFields();

   // Wait for encryption setup
   m_condEncryptionSetup.wait(30000);

   // Send response
   msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());
   msg.setField(VID_RCC, m_encryptionResult);
   sendMessage(msg);
}

/**
 * Update device system information
 */
void MobileDeviceSession::updateDeviceInfo(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	shared_ptr<MobileDevice> device = static_pointer_cast<MobileDevice>(FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE));
	if (device != nullptr)
	{
	   MobileDeviceInfo info;
	   info.commProtocol = COMM_PROTO;
	   info.vendor = request->getFieldAsSharedString(VID_VENDOR);
	   info.model = request->getFieldAsSharedString(VID_MODEL);
	   info.serialNumber = request->getFieldAsSharedString(VID_SERIAL_NUMBER);
	   info.osName = request->getFieldAsSharedString(VID_OS_NAME);
	   info.osVersion = request->getFieldAsSharedString(VID_OS_VERSION);
	   info.userId = request->getFieldAsSharedString(VID_USER_NAME);
		device->updateSystemInfo(info);
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(msg);
}

/**
 * Update device status
 */
void MobileDeviceSession::updateDeviceStatus(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<MobileDevice> device = static_pointer_cast<MobileDevice>(FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE));
	if (device != nullptr)
	{
	   MobileDeviceStatus status;
      status.commProtocol = COMM_PROTO;

      int type = request->getFieldType(VID_BATTERY_LEVEL);
      if (type == NXCP_DT_INT32)
         status.batteryLevel = static_cast<int8_t>(request->getFieldAsInt32(VID_BATTERY_LEVEL));
      else if (type == NXCP_DT_INT16)
         status.batteryLevel = static_cast<int8_t>(request->getFieldAsInt16(VID_BATTERY_LEVEL));

      if (request->isFieldExist(VID_GEOLOCATION_TYPE))
         status.geoLocation = GeoLocation(*request);

      if (request->isFieldExist(VID_IP_ADDRESS))
         status.ipAddress = request->getFieldAsInetAddress(VID_IP_ADDRESS);

      if (request->isFieldExist(VID_SPEED))
         status.speed = static_cast<float>(request->getFieldAsDouble(VID_SPEED));

      if (request->isFieldExist(VID_ALTITUDE))
         status.altitude = request->getFieldAsInt32(VID_ALTITUDE);

      if (request->isFieldExist(VID_DIRECTION))
         status.direction = request->getFieldAsInt16(VID_DIRECTION);

		device->updateStatus(status);
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(msg);
}

/**
 * Data push element
 */
struct MobileDataPushElement
{
   shared_ptr<DCObject> dci;
   TCHAR *value;

   MobileDataPushElement(const shared_ptr<DCObject> &_dci, TCHAR *_value)
   {
      dci = _dci;
      value = _value;
   }

   ~MobileDataPushElement()
   {
      MemFree(value);
   }
};

/**
 * Push DCI data
 */
void MobileDeviceSession::pushData(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<MobileDevice> device = static_pointer_cast<MobileDevice>(FindObjectById(m_deviceObjectId, OBJECT_MOBILEDEVICE));
	if (device != nullptr)
	{
      int count = (int)request->getFieldAsUInt32(VID_NUM_ITEMS);
      if (count > 0)
      {
         ObjectArray<MobileDataPushElement> values(count, 16, Ownership::True);

         int i;
         uint32_t varId = VID_PUSH_DCI_DATA_BASE;
         bool ok = true;
         for(i = 0; (i < count) && ok; i++)
         {
            ok = false;

            // find DCI by ID or name (if ID==0)
            uint32_t dciId = request->getFieldAsUInt32(varId++);
            shared_ptr<DCObject> pItem;
            if (dciId != 0)
            {
               pItem = device->getDCObjectById(dciId, 0);
            }
            else
            {
               TCHAR name[MAX_PARAM_NAME];
               request->getFieldAsString(varId++, name, MAX_PARAM_NAME);
               pItem = device->getDCObjectByName(name, 0);
            }

            if ((pItem != nullptr) && (pItem->getType() == DCO_TYPE_ITEM))
            {
               if (pItem->getDataSource() == DS_PUSH_AGENT)
               {
                  values.add(new MobileDataPushElement(pItem, request->getFieldAsString(varId++)));
                  ok = true;
               }
               else
               {
                  msg.setField(VID_RCC, RCC_NOT_PUSH_DCI);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
            }
         }

         // If all items was checked OK, push data
         if (ok)
         {
            Timestamp t = Timestamp::fromMilliseconds(0);
            int ft = request->getFieldType(VID_TIMESTAMP);
            if (ft == NXCP_DT_INT32)
            {
               t = Timestamp::fromTime(request->getFieldAsTime(VID_TIMESTAMP));
            }
            else if ((ft == NXCP_DT_STRING) || (ft == NXCP_DT_UTF8_STRING))
            {
               char ts[256];
               request->getFieldAsMBString(VID_TIMESTAMP, ts, 256);

               struct tm timeBuff;
               if (strptime(ts, "%Y/%m/%d %H:%M:%S", &timeBuff) != nullptr)
               {
                  timeBuff.tm_isdst = -1;
                  t = Timestamp::fromTime(timegm(&timeBuff));
               }
            }
            if (t.isNull())
            {
               t = Timestamp::now();
            }

            for(i = 0; i < values.size(); i++)
            {
               MobileDataPushElement *v = values.get(i);
			      if (_tcslen(v->value) >= MAX_DCI_STRING_VALUE)
				      v->value[MAX_DCI_STRING_VALUE - 1] = 0;
			      device->processNewDCValue(v->dci, t, v->value, shared_ptr<Table>(), false);
			      v->dci->setLastPollTime(t);
            }
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_FAILED_DCI_INDEX, i - 1);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
      }
   }
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(msg);
}
