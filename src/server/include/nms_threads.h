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
** $module: nms_threads.h
**
**/

#ifndef _nms_threads_h_
#define _nms_threads_h_

#ifdef _WIN32

#include <process.h>

inline HANDLE nms_create_thread(void (__cdecl *start_address )(void *), int stack_size, void *args)
{
   return (HANDLE)_beginthread(start_address, stack_size, args);
}

inline void nms_exit_thread(void)
{
   _endthread();
}

#else    /* _WIN32 */

#endif   /* _WIN32 */

#endif   /* _nms_threads_h_ */
