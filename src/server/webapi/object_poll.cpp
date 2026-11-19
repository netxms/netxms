/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: object_poll.cpp
**
**/

#include "webapi.h"

#define DEBUG_TAG L"webapi.poll"

/**
 * Poll output writer that sends each poll output message to WebSocket client
 */
class WebSocketPollOutputWriter : public PollOutputWriter
{
private:
   WebSocketOutputSession *m_session;

public:
   WebSocketPollOutputWriter(WebSocketOutputSession *session) : m_session(session) {}

   virtual void write(PollOutputLevel level, const wchar_t *text) override;
};

/**
 * Send poll output message to client. Level is omitted for normal messages.
 */
void WebSocketPollOutputWriter::write(PollOutputLevel level, const wchar_t *text)
{
   json_t *msg = json_object();
   json_object_set_new(msg, "type", json_string("output"));
   switch(level)
   {
      case PollOutputLevel::INFO:
         json_object_set_new(msg, "level", json_string("info"));
         break;
      case PollOutputLevel::WARNING:
         json_object_set_new(msg, "level", json_string("warning"));
         break;
      case PollOutputLevel::FAILURE:
         json_object_set_new(msg, "level", json_string("error"));
         break;
      default:
         break;
   }
   json_object_set_new(msg, "data", json_string_w(text));
   m_session->sendMessage(msg);
   json_decref(msg);
}

/**
 * Forced poll request
 */
struct ForcedPollRequest
{
   shared_ptr<NetObj> object;
   int pollType;
   shared_ptr<WebSocketOutputSession> session;

   ForcedPollRequest(const shared_ptr<NetObj>& _object, int _pollType, const shared_ptr<WebSocketOutputSession>& _session) :
      object(_object), session(_session)
   {
      pollType = _pollType;
   }
};

/**
 * Execute forced poll (poller thread pool callback). Poll continues to completion even if client disconnects;
 * output produced after disconnect is discarded.
 */
static void ExecutePoll(ForcedPollRequest *request)
{
   if (request->session != nullptr)
   {
      WebSocketPollOutputWriter output(request->session.get());
      ExecuteForcedPoll(request->object, request->pollType, &output);
      request->session->sendCompleted();
   }
   else
   {
      ExecuteForcedPoll(request->object, request->pollType, nullptr);
   }
   nxlog_debug_tag(DEBUG_TAG, 5, L"Forced %hs poll of object %s [%u] completed", PollTypeToName(request->pollType), request->object->getName(), request->object->getId());
   delete request;
}

/**
 * Wait for WebSocket client connection and start poll (main thread pool callback). Poll is started only after
 * client connects, so all poll output is delivered.
 */
static void StartStreamingPoll(ForcedPollRequest *request)
{
   if (!request->session->waitForConnection())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Forced %hs poll of object %s [%u] cancelled: client did not connect", PollTypeToName(request->pollType), request->object->getName(), request->object->getId());
      delete request;
      return;
   }

   WebSocketPollOutputWriter(request->session.get()).write(PollOutputLevel::NORMAL, L"Poll request accepted, waiting for outstanding polling requests to complete...");
   ThreadPoolExecute(g_pollerThreadPool, ExecutePoll, request);
}

/**
 * Handler for POST /v1/objects/:object-id/poll/:poll-type
 */
int H_ObjectPoll(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   if (objectId == 0)
      return 400;

   const wchar_t *pollTypeName = context->getPlaceholderValue(L"poll-type");
   char pollTypeNameUtf8[64] = "";
   if (pollTypeName != nullptr)
   {
      size_t bytes = wchar_to_utf8(pollTypeName, -1, pollTypeNameUtf8, sizeof(pollTypeNameUtf8) - 1);
      pollTypeNameUtf8[bytes] = 0;
   }
   int pollType = PollTypeFromName(pollTypeNameUtf8);
   if (pollType == -1)
   {
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Unknown poll type"));
      json_object_set_new(response, "validTypes", GetPollTypeNames());
      context->setResponseData(response);
      json_decref(response);
      return 400;
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   uint32_t rcc = ValidateForcedPoll(*object, pollType, context->getUserId());
   if (rcc == RCC_ACCESS_DENIED)
      return 403;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Poll type is not applicable to this object");
      return 400;
   }

   json_t *request = context->getRequestDocument();
   bool stream = json_object_get_boolean(request, "stream", true);

   nxlog_debug_tag(DEBUG_TAG, 4, L"Forced %hs poll of object %s [%u] requested by user %s [%u] (stream=%s)", pollTypeNameUtf8,
      object->getName(), object->getId(), context->getLoginName(), context->getUserId(), stream ? L"true" : L"false");

   json_t *response;
   if (stream)
   {
      auto session = make_shared<WebSocketOutputSession>();
      uuid token = RegisterWebSocketOutputSession(session);
      ThreadPoolExecute(g_mainThreadPool, StartStreamingPoll, new ForcedPollRequest(object, pollType, session));
      response = CreateWebSocketOutputResponse(token, "/v1/polls/output");
   }
   else
   {
      ThreadPoolExecute(g_pollerThreadPool, ExecutePoll, new ForcedPollRequest(object, pollType, shared_ptr<WebSocketOutputSession>()));
      response = json_object();
   }
   json_object_set_new(response, "pollType", json_string(pollTypeNameUtf8));
   context->setResponseData(response);
   json_decref(response);
   return 202;
}

/**
 * WebSocket upgrade handler for poll output streaming
 */
void WS_PollOutputConnect(void *cls, MHD_Connection *connection, void *con_cls,
                          const char *extra_in, size_t extra_in_size, MHD_socket sock,
                          MHD_UpgradeResponseHandle *responseHandle)
{
   ConnectWebSocketOutputSession(static_cast<Context*>(cls), sock, responseHandle);
}
