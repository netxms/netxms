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
** File: taskproc.cpp
**/

#include "asterisk.h"

/**
 * Task processor information
 */
struct TaskProcessor
{
   TCHAR name[128];
   UINT64 processed;
   UINT32 queued;
   UINT32 maxDepth;
   UINT32 lowWatermark;
   UINT32 highWatermark;
};

/**
 * Get list of task processors
 */
static ObjectArray<TaskProcessor> *ReadTaskProcessorList(AsteriskSystem *sys)
{
   StringList *rawData = sys->executeCommand("core show taskprocessors");
   if (rawData == nullptr)
      return nullptr;

   ObjectArray<TaskProcessor> *list = new ObjectArray<TaskProcessor>(128, 128, Ownership::True);
   for(int i = 0; i < rawData->size(); i++)
   {
      TCHAR name[128];
      unsigned long long processed;
      unsigned int queued, maxDepth, lw, hw;
      if (_stscanf(rawData->get(i), _T("%127s %Lu %u %u %u %u"), name, &processed, &queued, &maxDepth, &lw, &hw) == 6)
      {
         TaskProcessor *p = new TaskProcessor;
         _tcscpy(p->name, name);
         p->processed = processed;
         p->queued = queued;
         p->maxDepth = maxDepth;
         p->lowWatermark = lw;
         p->highWatermark = hw;
         list->add(p);
      }
   }

   delete rawData;
   return list;
}

/**
 * Handler for Asterisk.TaskProcessors list
 */
LONG H_TaskProcessorList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   ObjectArray<TaskProcessor> *list = ReadTaskProcessorList(sys);
   if (list == nullptr)
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < list->size(); i++)
      value->add(list->get(i)->name);
   delete list;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Asterisk.TaskProcessors table
 */
LONG H_TaskProcessorTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(0);
   ObjectArray<TaskProcessor> *list = ReadTaskProcessorList(sys);
   if (list == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("PROCESSED"), DCI_DT_COUNTER64, _T("Processed"));
   value->addColumn(_T("QUEUED"), DCI_DT_UINT, _T("Queued"));
   value->addColumn(_T("MAX_DEPTH"), DCI_DT_UINT, _T("Max Depth"));
   value->addColumn(_T("LOW_WATERMARK"), DCI_DT_UINT, _T("Low Watermark"));
   value->addColumn(_T("HIGH_WATERMARK"), DCI_DT_UINT, _T("High Watermark"));

   for(int i = 0; i < list->size(); i++)
   {
      value->addRow();
      value->set(0, list->get(i)->name);
      value->set(1, list->get(i)->processed);
      value->set(2, list->get(i)->queued);
      value->set(3, list->get(i)->maxDepth);
      value->set(4, list->get(i)->lowWatermark);
      value->set(5, list->get(i)->highWatermark);
   }
   delete list;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Task processor cache
 */
struct TaskProcessorCacheEntry
{
   ObjectArray<TaskProcessor> *data;
   time_t timestamp;

   TaskProcessorCacheEntry()
   {
      data = nullptr;
      timestamp = 0;
   }

   ~TaskProcessorCacheEntry()
   {
      delete data;
   }
};
static StringObjectMap<TaskProcessorCacheEntry> s_cache(Ownership::True);
static Mutex s_cacheLock;

/**
 * Handler for task processor Asterisk.TaskProcessor.* parameters
 */
LONG H_TaskProcessorDetails(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   GET_ASTERISK_SYSTEM(1);

   s_cacheLock.lock();
   TaskProcessorCacheEntry *cache = s_cache.get(sysName);
   if ((cache == nullptr) || (time(nullptr) - cache->timestamp > 5))
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("H_TaskProcessorDetails: re-read task processors list for %s"), sysName);

      if (cache == nullptr)
      {
         cache = new TaskProcessorCacheEntry();
         s_cache.set(sysName, cache);
      }

      ObjectArray<TaskProcessor> *list = ReadTaskProcessorList(sys);
      if (list == nullptr)
      {
         cache->timestamp = 0;
         delete_and_null(cache->data);
         s_cacheLock.unlock();
         return SYSINFO_RC_ERROR;
      }

      delete cache->data;
      cache->data = list;
      cache->timestamp = time(nullptr);
   }

   TCHAR name[128];
   GET_ARGUMENT(1, name, 128);

   TaskProcessor *p = nullptr;
   for(int i = 0; i < cache->data->size(); i++)
   {
      TaskProcessor *curr = cache->data->get(i);
      if (!_tcsicmp(name, curr->name))
      {
         p = curr;
         break;
      }
   }

   LONG rc;
   if (p != nullptr)
   {
      switch(*arg)
      {
         case 'H':
            ret_uint(value, p->highWatermark);
            break;
         case 'L':
            ret_uint(value, p->lowWatermark);
            break;
         case 'M':
            ret_uint(value, p->maxDepth);
            break;
         case 'P':
            ret_uint64(value, p->processed);
            break;
         case 'Q':
            ret_uint(value, p->queued);
            break;
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   s_cacheLock.unlock();
   return rc;
}
