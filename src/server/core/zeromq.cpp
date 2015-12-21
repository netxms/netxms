/*
** NetXMS - Network Management System
** Copyright (C) 2015 Raden Solutions
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
** File: zeromq.cpp
**
**/

#include "nxcore.h"

#ifdef WITH_ZMQ
#include <zmq.h>
#include <jansson.h>

static void *m_context = NULL;
static void *m_socket = NULL;

void StartZMQConnector()
{
   char endpoint[MAX_CONFIG_VALUE];
   ConfigReadStrUTF8(_T("ZeroMQEndpoint"), endpoint, MAX_CONFIG_VALUE, "");
   if (endpoint[0] == 0)
   {
      DbgPrintf(1, _T("ZeroMQ: endpoint not configured, integration disabled (expected parameter: ZeroMQEndpoint)"));
      return;
   }

   m_context = zmq_ctx_new();
   if (m_context != NULL)
   {
      m_socket = zmq_socket(m_context, ZMQ_PUSH);
      if (m_socket != NULL)
      {
         if (zmq_bind(m_socket, endpoint) == 0)
         {
            DbgPrintf(1, _T("ZeroMQ: initialised successfully"));
         }
         else
         {
            DbgPrintf(1, _T("ZeroMQ: bind failed, integration disabled"));
            zmq_ctx_term(m_context);
            m_context = NULL;
            zmq_close(m_socket);
            m_socket = NULL;
         }
      }
      else
      {
         DbgPrintf(1, _T("ZeroMQ: cannot create socket, integration disabled"));
         zmq_ctx_term(m_context);
         m_context = NULL;
      }
   }
   else
   {
         DbgPrintf(1, _T("ZeroMQ: cannot initalise context, integration disabled"));
   }
}

void StopZMQConnector()
{
   DbgPrintf(6, _T("ZeroMQ: shutdown initiated"));
   if (m_socket != NULL)
   {
      zmq_close(m_socket);
      m_socket = NULL;
   }
   DbgPrintf(6, _T("ZeroMQ: socket closed"));
   if (m_context != NULL)
   {
      zmq_ctx_term(m_context);
      m_context = NULL;
   }
   DbgPrintf(6, _T("ZeroMQ: context destroyed"));
}

static char *EventToJson(Event *event, NetObj *object)
{
   json_t *root = json_object();
   json_t *args = json_object();
   json_t *source = json_object();

   json_object_set_new(root, "source", source);
   json_object_set_new(root, "arguments", args);

   json_object_set_new(root, "id", json_integer(event->getId()));
   json_object_set_new(root, "time", json_integer(event->getTimeStamp()));
   json_object_set_new(root, "code", json_integer(event->getCode()));
   json_object_set_new(root, "severity", json_integer(event->getSeverity()));
   json_object_set_new(root, "message", json_string(UTF8StringFromTchar(event->getMessage())));

   json_object_set_new(source, "id", json_integer(event->getSourceId()));
   int objectType = object->getObjectClass();
   json_object_set_new(source, "type", json_integer(objectType));
   json_object_set_new(source, "name", json_string(UTF8StringFromTchar(object->getName())));
   if (objectType == OBJECT_NODE)
   {
      InetAddress ip = ((Node *)object)->getIpAddress();
      json_object_set_new(source, "primary-ip", json_string(ip.toStringA(NULL)));
   }
   
   int count = event->getParametersCount();
   json_object_set_new(args, "count", json_integer(count));
   for (int i = 0; i < count; i++)
   {
      char name[16];
      sprintf(name, "p%d", i+1);
      json_object_set_new(args, name, json_string(UTF8StringFromTchar(event->getParameter(i))));
   }

   char *message = json_dumps(root, 0);
   json_decref(root);
   return message;
}

void ZmqPublishEvent(Event *event)
{
   if (m_socket == NULL)
   {
      return;
   }

   NetObj *object = FindObjectById(event->getSourceId());
   if (object == NULL)
   {
      return;
   }
   DbgPrintf(6, _T("ZeroMQ: publish event: %s(%d) from %s(%d)"), event->getName(), event->getCode(), object->getName(), event->getSourceId());

   char *message = EventToJson(event, object);
   if (message != NULL)
   {
      (void)zmq_send(m_socket, message, strlen(message), ZMQ_DONTWAIT);
      free(message);
   }
}

#endif // WITH_ZMQ
