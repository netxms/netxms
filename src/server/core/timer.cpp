/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: timer.cpp
**
**/

#include "nxcore.h"

/**
 * Timer queue
 */
#ifdef _WIN32
HANDLE Timer::m_queue = NULL;
#endif

/**
 * Timer class global init
 */
void Timer::globalInit()
{
#ifdef _WIN32
   m_queue = CreateTimerQueue();
#endif
}

/**
 * Timer constructor
 */
Timer::Timer(const TCHAR *name, time_t startTime, UINT32 node, TimerAction action, UINT32 dataInt, const TCHAR *dataStr, StringMap *parameters)
{
   m_name = _tcsdup_ex(name);
   m_startTime = startTime;
   m_node = node;
   m_action = action;
   m_dataInt = dataInt;
   m_dataStr = _tcsdup_ex(dataStr);
   m_parameters.addAll(parameters);
}

/**
 * Destructor
 */
Timer::~Timer()
{
   free(m_name);
   free(m_dataStr);
#ifdef _WIN32
   DeleteTimerQueueTimer(m_queue, m_id, NULL);
#endif
}

/**
 * Execute timer
 */
void Timer::execute()
{
   switch(m_action)
   {
      case TIMER_ACTION_SEND_EVENT:
         PostEventWithNames(m_dataInt, m_node, &m_parameters);
         break;
      default:
         DbgPrintf(4, _T("Unknown timer action %d"), m_action);
         break;
   }
}

/**
 * Schedule timer
 */
void Timer::schedule(Timer *t)
{
}

/**
 * Cancel timer
 */
void Timer::cancel(Timer *t)
{
}
