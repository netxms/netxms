/* 
** NetXMS - Network Management System
** Server startup module
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: netxmsd.h
**
**/

#ifndef _netxmsd_h_
#define _netxmsd_h_

#include <nms_core.h>

#ifdef _WIN32

void InitService(void);
void InstallService(char *execName, char *dllName);
void RemoveService(void);
void StartCoreService(void);
void StopCoreService(void);
void InstallEventSource(char *path);
void RemoveEventSource(void);

#endif   /* _WIN32 */

#endif   /* _netxmsd_h_ */
