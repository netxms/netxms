/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: debug.cpp
**
**/

#include "nms_core.h"


//
// Test mutex state and print to stdout
//

void DbgTestMutex(MUTEX hMutex, char *szName)
{
   printf("  %s: ", szName);
   if (MutexLock(hMutex, 100))
   {
      printf("unlocked\n");
      MutexUnlock(hMutex);
   }
   else
   {
      printf("locked\n");
   }
}
