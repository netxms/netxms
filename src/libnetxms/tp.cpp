/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: tp.cpp
**
**/

#include "libnetxms.h"
#include <nxqueue.h>

/**
 * Thread pool
 */
struct ThreadPool
{
   int minThreads;
   int maxThreads;
   int curThreads;
   VolatileCounter activeRequests;
   MUTEX mutex;
   THREAD *threads;
   Queue *queue;
   TCHAR *name;
};

/**
 * Thread work request
 */
struct WorkRequest
{
   ThreadPoolWorkerFunction func;
   void *arg;
};

/**
 * Debug callback
 */
static void (*s_debugCallback)(int, const TCHAR *, va_list) = NULL;

/**
 * Debug print
 */
inline void dprint(int level, const TCHAR *format, ...)
{
   if (s_debugCallback == NULL)
      return;

   va_list args;
   va_start(args, format);
   s_debugCallback(level, format, args);
   va_end(args);
}

/**
 * Set debug callback
 */
void LIBNETXMS_EXPORTABLE ThreadPoolSetDebugCallback(void (*cb)(int, const TCHAR *, va_list))
{
   s_debugCallback = cb;
}

/**
 * Worker thread function
 */
static THREAD_RESULT THREAD_CALL WorkerThread(void *arg)
{
   Queue *q = ((ThreadPool *)arg)->queue;
   while(true)
   {
      WorkRequest *rq = (WorkRequest *)q->GetOrBlock();
      if (rq->func == NULL)
         break;   // stop indicator
      rq->func(rq->arg);
      free(rq);
      InterlockedDecrement(&((ThreadPool *)arg)->activeRequests);
   }
   return THREAD_OK;
}

/**
 * Create thread pool
 */
ThreadPool LIBNETXMS_EXPORTABLE *ThreadPoolCreate(int minThreads, int maxThreads, const TCHAR *name)
{
   ThreadPool *p = (ThreadPool *)malloc(sizeof(ThreadPool));
   p->minThreads = minThreads;
   p->maxThreads = maxThreads;
   p->curThreads = minThreads;
   p->activeRequests = 0;
   p->threads = (THREAD *)calloc(maxThreads, sizeof(THREAD));
   p->queue = new Queue(64, 64);
   p->mutex = MutexCreate();
   p->name = (name != NULL) ? _tcsdup(name) : _tcsdup(_T("NONAME"));

   for(int i = 0; i < p->curThreads; i++)
      p->threads[i] = ThreadCreateEx(WorkerThread, 0, p);

   dprint(1, _T("Thread pool %s initialized (min=%d, max=%d)"), p->name, p->minThreads, p->maxThreads);
   return p;
}

/**
 * Destroy thread pool
 */
void LIBNETXMS_EXPORTABLE ThreadPoolDestroy(ThreadPool *p)
{
   dprint(3, _T("Stopping threads in thread pool %s"), p->name);

   WorkRequest rq;
   rq.func = NULL;
   for(int i = 0; i < p->curThreads; i++)
      p->queue->Put(&rq);

   for(int i = 0; i < p->curThreads; i++)
      ThreadJoin(p->threads[i]);

   dprint(1, _T("Thread pool %s destroyed"), p->name);
   free(p->threads);
   delete p->queue;
   MutexDestroy(p->mutex);
   free(p->name);
   free(p);
}

/**
 * Execute task as soon as possible
 */
void LIBNETXMS_EXPORTABLE ThreadPoolExecute(ThreadPool *p, ThreadPoolWorkerFunction f, void *arg)
{
   if (InterlockedIncrement(&p->activeRequests) > p->curThreads)
   {
      bool started = false;
      MutexLock(p->mutex);
      if (p->curThreads < p->maxThreads)
      {
         p->threads[p->curThreads++] = ThreadCreateEx(WorkerThread, 0, p);
      }
      MutexUnlock(p->mutex);
      if (started)
         dprint(4, _T("New thread started in thread pool %s"), p->name);
   }

   WorkRequest *rq = (WorkRequest *)malloc(sizeof(WorkRequest));
   rq->func = f;
   rq->arg = arg;
   p->queue->Put(rq);
}

/**
 * Schedule task for execution using absolute time
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleAbsolute(ThreadPool *p, time_t runTime, ThreadPoolWorkerFunction *f, void *arg)
{
}

/**
 * Schedule task for execution using relative time
 */
void LIBNETXMS_EXPORTABLE ThreadPoolScheduleRelative(ThreadPool *p, UINT32 delay, ThreadPoolWorkerFunction *f, void *arg)
{
}

/**
 * Get pool information
 */
void LIBNETXMS_EXPORTABLE ThreadPoolGetInfo(ThreadPool *p, ThreadPoolInfo *info)
{
   info->name = p->name;
   info->minThreads = p->minThreads;
   info->maxThreads = p->maxThreads;
   info->curThreads = p->curThreads;
   info->activeRequests = p->activeRequests;
   info->load = info->activeRequests * 100 / info->curThreads;
   info->usage = info->curThreads * 100 / info->maxThreads;
}
