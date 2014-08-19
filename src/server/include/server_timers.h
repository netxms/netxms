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
** File: server_timers.h
**
**/

#ifndef _server_timers_h_
#define _server_timers_h_

/**
 * Timer ID type
 */
#if defined(_WIN32)
#define TIMER_ID  HANDLE
#elif HAVE_TIMER_T
#define TIMER_ID  timer_t
#else
#define TIMER_ID  int
#endif

/**
 * Timer actions
 */
enum TimerAction
{
   TIMER_ACTION_SEND_EVENT = 0,
   TIMER_ACTION_EXECUTE_SCRIPT = 1,
   TIMER_ACTION_EXECUTE_SERVER_ACTION = 2
};

/**
 * Timer
 */
class NXCORE_EXPORTABLE Timer : public RefCountObject
{
private:
   TIMER_ID m_id;
   TCHAR *m_name;
   time_t m_startTime;
   TimerAction m_action;
   UINT32 m_node;
   StringMap m_parameters;

   void execute();

#ifdef _WIN32
   static HANDLE m_queue;
#endif

public:
   Timer(const TCHAR *name, time_t startTime, TimerAction action, UINT32 node, StringMap *parameters);
   virtual ~Timer();

   time_t getStartTime() { return m_startTime; }

   static void schedule(Timer *t);
   static void cancel(Timer *t);
};

#endif
