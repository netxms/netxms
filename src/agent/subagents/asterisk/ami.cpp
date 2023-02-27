/*
** NetXMS Asterisk subagent
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: ami.cpp
**
**/

#include "asterisk.h"

/**
 * Default AMI message object constructor
 */
AmiMessage::AmiMessage()
{
   m_type = AMI_UNKNOWN;
   m_subType[0] = 0;
   m_id = 0;
   m_tags = nullptr;
   m_data = nullptr;
}

/**
 * Constructor for request message
 */
AmiMessage::AmiMessage(const char *subType)
{
   m_type = AMI_ACTION;
   strlcpy(m_subType, subType, MAX_AMI_SUBTYPE_LEN);
   m_id = 0;
   m_tags = nullptr;
   m_data = nullptr;
}

/**
 * AMI message object destructor
 */
AmiMessage::~AmiMessage()
{
   AmiMessageTag *curr = m_tags;
   while(curr != nullptr)
   {
      AmiMessageTag *next = curr->next;
      delete curr;
      curr = next;
   }
   delete m_data;
}

/**
 * Find tag by name
 */
AmiMessageTag *AmiMessage::findTag(const char *name)
{
   AmiMessageTag *curr = m_tags;
   while(curr != nullptr)
   {
      if (!stricmp(curr->name, name))
         return curr;
      curr = curr->next;
   }
   return nullptr;
}

/**
 * Get tag value
 */
const char *AmiMessage::getTag(const char *name)
{
   AmiMessageTag *tag = findTag(name);
   return (tag != nullptr) ? tag->value : nullptr;
}

/**
 * Get tag value as 32 bit integer
 */
int32_t AmiMessage::getTagAsInt32(const char *name, int32_t defaultValue)
{
   const char *v = getTag(name);
   return (v != nullptr) ? strtol(v, nullptr, 0) : defaultValue;
}

/**
 * Get tag value as 32 bit unsigned integer
 */
uint32_t AmiMessage::getTagAsUInt32(const char *name, uint32_t defaultValue)
{
   const char *v = getTag(name);
   return (v != nullptr) ? strtoul(v, nullptr, 0) : defaultValue;
}

/**
 * Get tag value as double
 */
double AmiMessage::getTagAsDouble(const char *name, double defaultValue)
{
   const char *v = getTag(name);
   return (v != nullptr) ? strtod(v, nullptr) : defaultValue;
}

/**
 * Set tag value
 */
void AmiMessage::setTag(const char *name, const char *value)
{
   AmiMessageTag *tag = findTag(name);
   if (tag != nullptr)
   {
      MemFree(tag->value);
      tag->value = strdup(value);
   }
   else
   {
      m_tags = new AmiMessageTag(name, value, m_tags);
   }
}

/**
 * Serialize message
 */
ByteStream *AmiMessage::serialize()
{
   ByteStream *out = new ByteStream();

   switch(m_type)
   {
      case AMI_ACTION:
         out->write("Action: ", 8);
         break;
      case AMI_RESPONSE:
         out->write("Response: ", 10);
         break;
      case AMI_EVENT:
         out->write("Event: ", 7);
         break;
   }
   out->write(m_subType, strlen(m_subType));
   out->write("\r\n", 2);

   if (m_type != AMI_EVENT)
   {
      out->write("ActionID: ", 10);

      char buffer[64];
      sprintf(buffer, INT64_FMTA, m_id);
      out->write(buffer, strlen(buffer));
      out->write("\r\n", 2);
   }

   AmiMessageTag *tag = m_tags;
   while(tag != nullptr)
   {
      out->write(tag->name, strlen(tag->name));
      out->write(": ", 2);
      out->write(tag->value, strlen(tag->value));
      out->write("\r\n", 2);
      tag = tag->next;
   }

   out->write("\r\n", 2);
   return out;
}

/**
 * Check if tag is valid
 */
inline bool IsValidTag(char *line, char *separator)
{
   for(char *p = line; p < separator; p++)
      if (*p == ' ')
         return false;
   return true;
}

/**
 * Create AMI message object from network buffer
 */
shared_ptr<AmiMessage> AmiMessage::createFromNetwork(RingBuffer& buffer)
{
   char line[4096];
   size_t linePos = 0;
   bool dataMode = false;

   AmiMessage *m = new AmiMessage();

   buffer.savePos();
   while(!buffer.isEmpty())
   {
      line[linePos++] = static_cast<char>(buffer.readByte());
      if ((linePos > 0) && (line[linePos - 1] == '\n'))
      {
         if ((linePos > 1) && (line[linePos - 2] == '\r'))
            line[linePos - 2] = 0;
         else
            line[linePos - 1] = 0;
         if (!dataMode && (strlen(line) == 0))
         {
            // Empty CR/LF
            if ((m->m_type != AMI_RESPONSE) || stricmp(m->m_subType, "Follows") || (m->m_data != NULL))
               return shared_ptr<AmiMessage>(m);
            dataMode = true;   // CLI data will follow
            m->m_data = new StringList();
         }

         if (dataMode)
         {
            if (!stricmp(line, "--END COMMAND--"))
            {
               dataMode = false;
            }
            else
            {
               m->m_data->addMBString(line);
            }
         }
         else
         {
            char *s = strchr(line, ':');
            if ((s != nullptr) && IsValidTag(line, s))
            {
               *s = 0;
               s++;
               TrimA(line);
               TrimA(s);
               if (!stricmp(line, "Action"))
               {
                  m->m_type = AMI_ACTION;
                  strlcpy(m->m_subType, s, MAX_AMI_SUBTYPE_LEN);
               }
               else if (!stricmp(line, "Response"))
               {
                  m->m_type = AMI_RESPONSE;
                  strlcpy(m->m_subType, s, MAX_AMI_SUBTYPE_LEN);
               }
               else if (!stricmp(line, "Event"))
               {
                  m->m_type = AMI_EVENT;
                  strlcpy(m->m_subType, s, MAX_AMI_SUBTYPE_LEN);
               }
               else if (!stricmp(line, "ActionID"))
               {
                  m->m_id = strtoll(s, nullptr, 0);
               }
               else
               {
                  m->m_tags = new AmiMessageTag(line, s, m->m_tags);
               }
            }
            else if ((m->m_type == AMI_RESPONSE) && !stricmp(m->m_subType, "Follows") && (m->m_data == nullptr))
            {
               m->m_data = new StringList();
               if (stricmp(line, "--END COMMAND--"))
               {
                  m->m_data->addMBString(line);
                  dataMode = true;   // CLI data will follow
               }
            }
         }
         linePos = 0;
      }
   }
   buffer.restorePos();
   delete m;
   return nullptr;
}

/**
 * AMI connector thread
 */
void AsteriskSystem::connectorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Connector thread started for Asterisk system %s at %s:%d"),
            m_name, (const TCHAR *)m_ipAddress.toString(), m_port);
   do
   {
      m_resetSession = false;
      m_networkBuffer.clear();
      m_socket = ConnectToHost(m_ipAddress, m_port, 5000);
      if (m_socket != INVALID_SOCKET)
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to Asterisk system %s at %s:%d"), m_name, (const TCHAR *)m_ipAddress.toString(), m_port);
         if (sendLoginRequest())
         {
            ThreadPoolExecute(g_asteriskThreadPool, this, &AsteriskSystem::setupEventFilters);
            while(!m_resetSession)
            {
               BYTE data[4096];
               int bytes = RecvEx(m_socket, data, 4096, 0, 1000);
               if (bytes == -2)
                  continue;
               if (bytes <= 0)
                  break;
               m_networkBuffer.write(data, bytes);

               while(true)
               {
                  shared_ptr<AmiMessage> msg = readMessage();
                  if (msg == nullptr)
                     break;
                  if (!processMessage(msg))
                     break;
               }
            }
            m_amiSessionReady = false;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot send login request to Asterisk system %s at %s:%d"),
                     m_name, (const TCHAR *)m_ipAddress.toString(), m_port);
         }
         closesocket(m_socket);
         m_socket = INVALID_SOCKET;
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Disconnected from Asterisk system %s at %s:%d"), m_name, (const TCHAR *)m_ipAddress.toString(), m_port);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot connect to Asterisk system %s at %s:%d"),
                  m_name, (const TCHAR *)m_ipAddress.toString(), m_port);
      }
   } while(!AgentSleepAndCheckForShutdown(m_resetSession ? 1000 : 60000));
}

/**
 * Send login request
 */
bool AsteriskSystem::sendLoginRequest()
{
   char msg[1024];
   snprintf(msg, 1024, "Action: Login\r\nActionID: -1\r\nUsername: %s\r\nSecret: %s\r\n\r\n", m_login, m_password);
   size_t len = strlen(msg);
   return SendEx(m_socket, msg, len, 0, nullptr) == len;
}

/**
 * Callback for starting registration tests
 */
static EnumerationCallbackResult StartRegistrationTest(const TCHAR *name, const SIPRegistrationTest *test)
{
   ((SIPRegistrationTest*)test)->start();
   return _CONTINUE;
}

/**
 * Start connector
 */
void AsteriskSystem::start()
{
   m_connectorThread = ThreadCreateEx(this, &AsteriskSystem::connectorThread);
   m_registrationTests.forEach(StartRegistrationTest);
}

/**
 * Callback for stopping registration tests
 */
static EnumerationCallbackResult StopRegistrationTest(const TCHAR *name, const SIPRegistrationTest *test)
{
   ((SIPRegistrationTest*)test)->stop();
   return _CONTINUE;
}

/**
 * Stop connector
 */
void AsteriskSystem::stop()
{
   m_registrationTests.forEach(StopRegistrationTest);

   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   ThreadJoin(m_connectorThread);
   m_connectorThread = INVALID_THREAD_HANDLE;
}

/**
 * Reset connector
 */
void AsteriskSystem::reset()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Connection reset for Asterisk system %s"), m_name);
   m_resetSession = true;
}

/**
 * Process AMi message
 */
bool AsteriskSystem::processMessage(const shared_ptr<AmiMessage>& msg)
{
   bool success = true;
   nxlog_debug_tag(DEBUG_TAG, 6, _T("AMI message received from %s: type=%d subType=%hs id=") INT64_FMT, m_name, msg->getType(), msg->getSubType(), msg->getId());

   if ((msg->getType() == AMI_RESPONSE) && (msg->getId() == -1))
   {
      if (!strcmp(msg->getSubType(), "Success"))
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Successfully logged in to Asterisk system %s"), m_name);
         m_amiSessionReady = true;
      }
      else
      {
         const char *reason = msg->getTag("Message");
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot login to Asterisk system %s (%hs)"), m_name, (reason != NULL) ? reason : "Unknown reason");
         success = false;
      }
   }
   else if ((msg->getType() == AMI_RESPONSE) && (msg->getId() == m_activeRequestId))
   {
      m_response = msg;
      m_requestCompletion.set();
   }
   else if (msg->getType() == AMI_EVENT)
   {
      m_eventListenersLock.lock();
      for(int i = 0; i < m_eventListeners.size(); i++)
      {
         m_eventListeners.get(i)->processEvent(msg);
      }
      m_eventListenersLock.unlock();

      if (!stricmp(msg->getSubType(), "Hangup"))
         processHangup(msg);
      else if (!stricmp(msg->getSubType(), "RTCPReceived"))
         processRTCP(msg);
   }

   return success;
}

/**
 * List request data
 */
class ListCollector : public AmiEventListener
{
private:
   SharedObjectArray<AmiMessage> *m_messages;
   int64_t m_requestId;
   Condition m_completed;

public:
   ListCollector(SharedObjectArray<AmiMessage> *messages, int64_t requestId) : m_completed(true)
   {
      m_messages = messages;
      m_requestId = requestId;
   }

   virtual void processEvent(const shared_ptr<AmiMessage>& event) override;

   bool waitForCompletion(uint32_t timeout)
   {
      return m_completed.wait(timeout);
   }
};

/**
 * Process event
 */
void ListCollector::processEvent(const shared_ptr<AmiMessage>& event)
{
   if (m_requestId != event->getId())
      return;

   const char *v = event->getTag("EventList");
   if ((v != nullptr) && !stricmp(v, "Complete"))
   {
      m_completed.set();
      return;
   }

   m_messages->add(event);
}

/**
 * Send AMI request and wait for response.
 * Will decrease reference count of request
 */
shared_ptr<AmiMessage> AsteriskSystem::sendRequest(const shared_ptr<AmiMessage>& request, SharedObjectArray<AmiMessage> *list, uint32_t timeout)
{
   if (!m_amiSessionReady)
      return nullptr;

   if (timeout == 0)
      timeout = m_amiTimeout;

   shared_ptr<AmiMessage> response;
   m_requestLock.lock();
   m_requestCompletion.reset();

   m_activeRequestId = m_requestId++;
   request->setId(m_activeRequestId);
   ByteStream *serializedMessage = request->serialize();

   ListCollector *collector = nullptr;
   if (list != nullptr)
   {
      collector = new ListCollector(list, m_activeRequestId);
      addEventListener(collector);
   }

   size_t size;
   const BYTE *bytes = serializedMessage->buffer(&size);
   if (SendEx(m_socket, bytes, size, 0, nullptr) == size)
   {
      if (m_requestCompletion.wait(timeout))
      {
         response = m_response;
         m_response.reset();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Request timeout waiting for AMI request \"%hs\" ID ") INT64_FMT, request->getSubType(), m_activeRequestId);
         reset();
      }
   }
   delete serializedMessage;

   if (collector != nullptr)
   {
      bool success;
      if ((response != nullptr) && response->isSuccess())
      {
         success = collector->waitForCompletion(timeout);
      }
      else
      {
         success = false;
      }
      removeEventListener(collector);
      delete collector;

      // Clear partially collected messages in case of error
      if (!success)
      {
         list->clear();
      }
   }

   m_activeRequestId = 0;
   m_requestLock.unlock();

   return response;
}

/**
 * Send simple AMI request - response message is analyzed and dropped
 */
bool AsteriskSystem::sendSimpleRequest(const shared_ptr<AmiMessage>& request)
{
   shared_ptr<AmiMessage> response = sendRequest(request);
   if (response == nullptr)
      return false;

   bool success = response->isSuccess();
   if (!success)
   {
      const char *reason = response->getTag("Message");
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Request \"%hs\" to %s failed (%hs)"), request->getSubType(), m_name, (reason != NULL) ? reason : "Unknown reason");
   }
   return success;
}
