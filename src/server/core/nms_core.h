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
** $module: nms_core.h
**
**/

#ifndef _nms_core_h_
#define _nms_core_h_

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else    /* _WIN32 */
#include <unistd.h>
#endif   /* _WIN32 */

#include <nms_common.h>
#include <nms_threads.h>


//
// Common constants
//

#define VERSION_MAJOR      1
#define VERSION_MINOR      0
#define VERSION_RELEASE    0
#define VERSION_STRING     "1.0.0"

#ifdef _WIN32
#define DEFAULT_CONFIG_FILE   "C:\\nms.conf"
#define IsStandalone() (g_flags & CF_STANDALONE)
#else    /* _WIN32 */
#define DEFAULT_CONFIG_FILE   "/etc/nms.conf"
#define IsStandalone() (1)
#endif   /* _WIN32 */


//
// Configuration flags
//

#define CF_STANDALONE   0x0001


//
// Win32 service constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME  "NMSCore"
#define CORE_EVENT_SOURCE  "NMSCore"

#endif   /* _WIN32 */


//
// Functions
//

int ParseCommandLine(int argc, char *argv[]);
int LoadConfig(void);

void Shutdown(void);
BOOL Initialize(void);
void Main(void);

#ifdef _WIN32

void InitService(void);
void InstallService(char *execName);
void RemoveService(void);
void StartCoreService(void);
void StopCoreService(void);
void InstallEventSource(char *path);
void RemoveEventSource(void);

char *GetSystemErrorText(DWORD error);

#endif   /* _WIN32 */


//
// Global variables
//

extern DWORD g_flags;
extern char g_configFile[];

#endif   /* _nms_core_h_ */
