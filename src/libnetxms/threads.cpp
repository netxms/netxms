/* 
** libnetxms - Common NetXMS utility library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: threads.cpp
**
**/

#if !defined(_WIN32) && !defined(_NETWARE)

#include "libnetxms.h"
#include <signal.h>
#include <sys/wait.h>

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

/**
 * Start main loop and signal handler for daemon
 */
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain)
{
   THREAD hThread;
   struct utsname un;
   int nModel = 0;
   sigset_t signals;

   // Block signals
   sigemptyset(&signals);
   sigaddset(&signals, SIGTERM);
   sigaddset(&signals, SIGINT);
   sigaddset(&signals, SIGPIPE);
   sigaddset(&signals, SIGSEGV);
   sigaddset(&signals, SIGCHLD);
   sigaddset(&signals, SIGHUP);
   sigaddset(&signals, SIGUSR1);
   sigaddset(&signals, SIGUSR2);
   sigprocmask(SIG_BLOCK, &signals, NULL);

   if (uname(&un) != -1)
   {
      char *ptr;

      ptr = strchr(un.release, '.');
      if (ptr != NULL)
         *ptr = 0;
      if (!stricmp(un.sysname, "FreeBSD") && (atoi(un.release) >= 5))
         nModel = 1;
   }

   if (pfMain != NULL)
   {
      if (nModel == 0)
      {
         hThread = ThreadCreateEx(pfMain, 0, NULL);
         pfSignalHandler(NULL);
         ThreadJoin(hThread);
      }
      else
      {
         hThread = ThreadCreateEx(pfSignalHandler, 0, NULL);
         pfMain(NULL);
         ThreadJoin(hThread);
      }
   }
   else
   {
      if (nModel == 0)
      {
         pfSignalHandler(NULL);
      }
      else
      {
         hThread = ThreadCreateEx(pfSignalHandler, 0, NULL);
         ThreadJoin(hThread);
      }
   }
}


#endif   /* _WIN32 && _NETWARE*/
