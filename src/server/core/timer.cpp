/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
HANDLE Timer::m_queue = CreateTimerQueue();
#endif

/**
 * Timer constructor
 */
Timer::Timer(const TCHAR *name, time_t startTime, TimerAction action, UINT32 node, StringMap *parameters)
{
   m_name = _tcsdup_ex(name);
   m_startTime = startTime;
   m_action = action;
   m_node = node;
   m_parameters.addAll(parameters);
}

/**
 * Destructor
 */
Timer::~Timer()
{
   free(m_name);
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
         PostEventWithNames();
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
