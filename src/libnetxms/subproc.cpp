/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: subproc.cpp
**
**/

#include "libnetxms.h"
#include <nxproc.h>
#include <nxcpapi.h>

#define DEBUG_TAG _T("proc.spexec")

/**
 * Sub-process stop condition
 */
static Condition s_stopCondition(true);

/**
 * Sub-process pipe connector
 */
static void PipeConnector(NamedPipe *pipe, void *userArg)
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Connected to master process"));
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *request = receiver.readMessage(INFINITE, &result);
      if (result != MSGRECV_SUCCESS)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Pipe receiver failure (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }
      nxlog_debug(6, _T("Received message 0x%04x"), request->getCode());
      NXCPMessage *response = NULL;
      switch(request->getCode())
      {
         case SPC_EXIT:
            response = new NXCPMessage(SPC_REQUEST_COMPLETED, request->getId());
            response->setField(VID_RCC, 0);
            goto stop;
         default:
            if (request->getCode() >= SPC_USER)
            {
               const void *data;
               size_t size;
               if (request->isBinary())
               {
                  data = request->getBinaryData();
                  size = request->getBinaryDataSize();
               }
               else
               {
                  data = nullptr;
                  size = 0;
               }
               response = reinterpret_cast<SubProcessRequestHandler>(userArg)(request->getCode(), data, size);
               if (response != nullptr)
                  response->setId(request->getId());
            }
            break;
      }
      delete request;

      if (response != nullptr)
      {
         NXCP_MESSAGE *data = response->serialize(false);
         pipe->write(data, ntohl(data->size));
         MemFree(data);
         delete response;
      }
   }

stop:
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Connection with master process closed"));
   s_stopCondition.set();
}

/**
 * Initialize and run sub-process (should be called from sub-process's main())
 */
int LIBNETXMS_EXPORTABLE SubProcessMain(int argc, char *argv[], SubProcessRequestHandler requestHandler)
{
   TCHAR pipeName[256];
   _sntprintf(pipeName, 256, _T("netxms.subprocess.%u"), GetCurrentProcessId());
   NamedPipeListener *pipeListener = NamedPipeListener::create(pipeName, PipeConnector, reinterpret_cast<void*>(requestHandler), nullptr);
   if (pipeListener == nullptr)
      return 1;
   pipeListener->start();
   s_stopCondition.wait(INFINITE);
   pipeListener->stop();
   delete pipeListener;
   return 0;
}

/**
 * Sub-process registry
 */
ObjectArray<SubProcessExecutor> LIBNETXMS_EXPORTABLE *SubProcessExecutor::m_registry = nullptr;
Mutex LIBNETXMS_EXPORTABLE SubProcessExecutor::m_registryLock(MutexType::FAST);

/**
 * Sub-process manager thread handle
 */
THREAD LIBNETXMS_EXPORTABLE SubProcessExecutor::m_monitorThread = INVALID_THREAD_HANDLE;

/**
 * Sub-process manager thread stop condition
 */
Condition LIBNETXMS_EXPORTABLE SubProcessExecutor::m_stopCondition(true);

/**
 * Sub-process monitor thread
 */
THREAD_RESULT THREAD_CALL SubProcessExecutor::monitorThread(void *arg)
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Sub-process monitor started"));
   while(!m_stopCondition.wait(5000))
   {
      m_registryLock.lock();
      for(int i = 0; i < m_registry->size(); i++)
      {
         SubProcessExecutor *p = m_registry->get(i);
         if (p->isStarted() && !p->isRunning())
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Sub-process %s is not running, attempting restart"), p->getName());
            p->stop();
            p->execute();
         }
      }
      m_registryLock.unlock();
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Sub-process monitor stopped"));
   return THREAD_OK;
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL SubProcessExecutor::receiverThreadStarter(void *arg)
{
   static_cast<SubProcessExecutor*>(arg)->receiverThread();
   return THREAD_OK;
}

/**
 * Shutdown monitor thread and all registered sub-processes
 */
void SubProcessExecutor::shutdown()
{
   m_stopCondition.set();
   ThreadJoin(m_monitorThread);
   m_monitorThread = INVALID_THREAD_HANDLE;

   m_registryLock.lock();
   if (m_registry != nullptr)
   {
      for(int i = 0; i < m_registry->size(); i++)
      {
         SubProcessExecutor *p = m_registry->get(i);
         if (p->isStarted() && p->isRunning())
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Stopping sub-process %s"), p->getName());
            p->stop();
         }
      }
      delete_and_null(m_registry);
   }
   m_registryLock.unlock();
}

/**
 * Sub-process executor constructor
 */
SubProcessExecutor::SubProcessExecutor(const TCHAR *name, const TCHAR *command) : ProcessExecutor(command, false)
{
   _tcslcpy(m_name, name, MAX_SUBPROCESS_NAME_LEN);
   m_state = SP_INIT;
   m_requestId = 0;
   m_pipe = NULL;
   m_messageQueue = new MsgWaitQueue();
   m_receiverThread = INVALID_THREAD_HANDLE;

   m_registryLock.lock();
   if (m_registry == nullptr)
      m_registry = new ObjectArray<SubProcessExecutor>(16, 16, Ownership::False);
   if (m_monitorThread == INVALID_THREAD_HANDLE)
      m_monitorThread = ThreadCreateEx(SubProcessExecutor::monitorThread, 0, nullptr);
   m_registry->add(this);
   m_registryLock.unlock();
}

/**
 * Sub-process executor destructor
 */
SubProcessExecutor::~SubProcessExecutor()
{
   m_registryLock.lock();
   m_registry->remove(this);
   m_registryLock.unlock();

   delete m_messageQueue;
   delete m_pipe;
}

/**
 * Execute sub-process
 */
bool SubProcessExecutor::execute()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("About to start sub-process %s with command %s"), m_name, m_cmd);
   if (!ProcessExecutor::execute())
      return false;

   TCHAR pipeName[256];
   _sntprintf(pipeName, 256, _T("netxms.subprocess.%u"), getProcessId());
   int retryCount = 5;
   do
   {
      m_pipe = NamedPipe::connect(pipeName, 5000);
      if (m_pipe == nullptr)
      {
         if (--retryCount > 0)
         {
            ThreadSleep(1);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Sub-process %s started but did not respond to connect"), m_name);
            stop();
            return false;
         }
      }
   } while(m_pipe == nullptr);

   m_state = SP_RUNNING;
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Sub-process %s (%u) started and connected"), m_name, getProcessId());

   m_receiverThread = ThreadCreateEx(SubProcessExecutor::receiverThreadStarter, 0, this);
   return true;
}

/**
 * Stop sub-process
 */
void SubProcessExecutor::stop()
{
   m_state = SP_STOPPED;
   if (isRunning() && sendCommand(SPC_EXIT))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Shutdown request sent to sub-process %s"), m_name);
      ThreadSleep(1);
   }
   ThreadJoin(m_receiverThread);
   m_receiverThread = INVALID_THREAD_HANDLE;
   delete_and_null(m_pipe);
   ProcessExecutor::stop();
}

/**
 * Receiver thread
 */
void SubProcessExecutor::receiverThread()
{
   PipeMessageReceiver receiver(m_pipe->handle(), 8192, 4194304);  // 8K initial, 4M max
   while(m_state == SP_RUNNING)
   {
      MessageReceiverResult result;
      NXCPMessage *response = receiver.readMessage(5000, &result);
      if (response != NULL)
      {
         m_messageQueue->put(response);
      }
      else if (result == MSGRECV_CLOSED)
      {
         m_state = SP_COMM_FAILURE;
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Communication channel with sub-process %s (%u) closed"), m_name, getProcessId());
         break;
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Sub-process %s receiver thread stopped"), m_name);
}

/**
 * Send command to sub-process
 */
bool SubProcessExecutor::sendCommand(uint16_t command, const void *data, size_t dataSize, uint32_t *requestId)
{
   if (m_pipe == nullptr)
      return false;

   uint32_t rid = InterlockedIncrement(&m_requestId);

   NXCP_MESSAGE msg;
   msg.code = htons(command);
   msg.id = htonl(rid);
   msg.flags = htons((data != nullptr) ? MF_BINARY : MF_CONTROL);
   msg.numFields = htonl((uint32_t)dataSize);
   uint32_t padding = (8 - (dataSize % 8)) & 7;
   msg.size = htonl(NXCP_HEADER_SIZE + static_cast<uint32_t>(dataSize) + padding);
   if (!m_pipe->write(&msg, NXCP_HEADER_SIZE))
      return false;
   if (data != nullptr)
   {
      if (!m_pipe->write(data, dataSize))
         return false;
      if (padding > 0)
      {
         char paddingBytes[8] = "ABCDEFG";
         if (!m_pipe->write(paddingBytes, padding))
            return false;
      }
   }

   if (requestId != nullptr)
      *requestId = rid;

   return true;
}

/**
 * Send request to sub-process
 */
bool SubProcessExecutor::sendRequest(uint16_t command, const void *data, size_t dataSize, void **response, size_t *rspSize, uint32_t timeout)
{
   uint32_t requestId;
   if (!sendCommand(command, data, dataSize, &requestId))
      return false;

   NXCPMessage *rmsg = m_messageQueue->waitForMessage(SPC_REQUEST_COMPLETED, requestId, timeout);
   if (rmsg == nullptr)
      return false;

   if ((response != nullptr) && (rspSize != nullptr))
   {
      if (rmsg->isBinary())
      {
         *response = MemCopyBlock(rmsg->getBinaryData(), rmsg->getBinaryDataSize());
         *rspSize = rmsg->getBinaryDataSize();
      }
      else
      {
         *response = nullptr;
         *rspSize = 0;
      }
   }

   delete rmsg;
   return true;
}
