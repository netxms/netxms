/*
** NetXMS NETCONF subagent
** Copyright (C) 2026 Raden Solutions
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
** File: handlers.cpp
**
**/

#include "netconf_subagent.h"

/**
 * Parse common metric arguments: host[:port], login, password, [keyId]
 */
static bool ParseTargetArguments(const TCHAR *param, InetAddress *addr, uint16_t *port, TCHAR *login, TCHAR *password, uint32_t *keyId)
{
   TCHAR hostName[256];
   if (!AgentGetParameterArg(param, 1, hostName, 256) ||
       !AgentGetParameterArg(param, 2, login, 64) ||
       !AgentGetParameterArg(param, 3, password, 64))
   {
      return false;
   }

   *port = NETCONF_DEFAULT_PORT;
   TCHAR *p = _tcschr(hostName, _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;
      *port = static_cast<uint16_t>(_tcstoul(p, nullptr, 10));
   }

   *addr = InetAddress::resolveHostName(hostName);
   *keyId = 0;
   AgentGetMetricArgAsUInt32(param, 4, keyId);
   return true;
}

/**
 * NETCONF connectivity check handler
 * Parameters: host[:port], login, password, [keyId]
 * Returns 1 if NETCONF session can be established (including hello exchange), 0 otherwise
 */
LONG H_NETCONFCheckConnection(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   InetAddress addr;
   uint16_t port;
   TCHAR login[64], password[64];
   uint32_t keyId;
   if (!ParseTargetArguments(param, &addr, &port, login, password, &keyId))
      return SYSINFO_RC_UNSUPPORTED;

   if (!addr.isValidUnicast())
   {
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }

   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   NETCONFSession *netconfSession = AcquireSession(addr, port, login, password, keys);
   if (netconfSession == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONF.CheckConnection: failed to establish NETCONF session with %s:%u"), addr.toString().cstr(), port);
      ret_int(value, 0);
      return SYSINFO_RC_SUCCESS;
   }
   ReleaseSession(netconfSession);

   nxlog_debug_tag(DEBUG_TAG, 8, _T("NETCONF.CheckConnection: NETCONF session with %s:%u established successfully"), addr.toString().cstr(), port);
   ret_int(value, 1);
   return SYSINFO_RC_SUCCESS;
}

/**
 * NETCONF capability list handler
 * Parameters: host[:port], login, password, [keyId]
 * Returns list of capability URIs announced by the device in hello message
 */
LONG H_NETCONFCapabilities(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   InetAddress addr;
   uint16_t port;
   TCHAR login[64], password[64];
   uint32_t keyId;
   if (!ParseTargetArguments(param, &addr, &port, login, password, &keyId))
      return SYSINFO_RC_UNSUPPORTED;

   if (!addr.isValidUnicast())
      return SYSINFO_RC_ERROR;

   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   NETCONFSession *netconfSession = AcquireSession(addr, port, login, password, keys);
   if (netconfSession == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONF.Capabilities: failed to establish NETCONF session with %s:%u"), addr.toString().cstr(), port);
      return SYSINFO_RC_ERROR;
   }

   value->addAll(netconfSession->getPeerCapabilities().getAll());
   ReleaseSession(netconfSession);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Process CMD_NETCONF_EXECUTE request. Request contains list of RPCs executed in order
 * on a single NETCONF session (so locks span the sequence). Execution stops at first RPC
 * that fails on transport level or returns rpc-error with severity "error"; replies
 * received so far are returned in any case.
 */
void ExecuteNETCONFRequest(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session)
{
   InetAddress addr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
   uint16_t port = request.getFieldAsUInt16(VID_PORT);
   if (port == 0)
      port = NETCONF_DEFAULT_PORT;

   TCHAR user[256], password[256];
   request.getFieldAsString(VID_USER_NAME, user, 256);
   request.getFieldAsString(VID_PASSWORD, password, 256);

   uint32_t keyId = request.getFieldAsUInt32(VID_SSH_KEY_ID);
   shared_ptr<KeyPair> keys;
   if (keyId != 0)
      keys = GetSshKey(session, keyId);

   uint32_t timeout = request.getFieldAsUInt32(VID_TIMEOUT);
   if (timeout == 0)
      timeout = g_netconfRequestTimeout;

   int count = request.getFieldAsInt32(VID_NUM_ELEMENTS);
   if (count <= 0)
   {
      response->setField(VID_RCC, ERR_MALFORMED_COMMAND);
      return;
   }

   TCHAR ipAddrText[64];
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteNETCONFRequest: executing %d RPC(s) on %s:%u"), count, addr.toString(ipAddrText), port);

   for(int attempt = 0; attempt < 2; attempt++)
   {
      NETCONFSession *netconfSession = AcquireSession(addr, port, user, password, keys);
      if (netconfSession == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteNETCONFRequest: cannot establish NETCONF session with %s:%u"), addr.toString(ipAddrText), port);
         response->setField(VID_RCC, ERR_REMOTE_CONNECT_FAILED);
         return;
      }

      int executed = 0;
      bool transportFailure = false;
      for(int i = 0; i < count; i++)
      {
         char *content = request.getFieldAsUtf8String(VID_ELEMENT_LIST_BASE + i);
         if ((content == nullptr) || (*content == 0))
         {
            MemFree(content);
            ReleaseSession(netconfSession);
            response->setField(VID_RCC, ERR_MALFORMED_COMMAND);
            return;
         }

         size_t replySize;
         char *reply = netconfSession->executeRpc(content, timeout, &replySize);
         MemFree(content);
         if (reply == nullptr)
         {
            transportFailure = true;
            break;
         }

         response->setFieldFromUtf8String(VID_ELEMENT_LIST_BASE + executed, reply);
         executed++;

         NETCONF_Response parsedReply;
         bool rpcError = parsedReply.parse(reply, replySize) && !parsedReply.isSuccess();
         MemFree(reply);
         if (rpcError)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteNETCONFRequest: RPC %d of %d on %s:%u returned error, aborting sequence"), i + 1, count, addr.toString(ipAddrText), port);
            break;
         }
      }

      if (transportFailure && (executed == 0) && (attempt == 0))
      {
         // First RPC failed without any reply - pooled session may be stale, retry with new session
         nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteNETCONFRequest: RPC execution on %s:%u failed, retrying with new session"), addr.toString(ipAddrText), port);
         ReleaseSession(netconfSession, true);
         continue;
      }

      ReleaseSession(netconfSession, transportFailure);
      response->setField(VID_NUM_ELEMENTS, static_cast<int32_t>(executed));
      response->setField(VID_RCC, transportFailure ? ERR_EXEC_FAILED : ERR_SUCCESS);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExecuteNETCONFRequest: %d of %d RPC(s) executed on %s:%u"), executed, count, addr.toString(ipAddrText), port);
      return;
   }

   response->setField(VID_RCC, ERR_EXEC_FAILED);
}
