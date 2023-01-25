/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: nxweblauncher.h
**
**/

#ifndef _nxweblauncher_h_
#define _nxweblauncher_h_

#include <nms_util.h>

#define WEBUI_SERVICE_NAME L"nxWebUI"

#define DEBUG_TAG_JAVA_RUNTIME   L"java"
#define DEBUG_TAG_STARTUP        L"startup"
#define DEBUG_TAG_SHUTDOWN       L"shutdown"

void InitService();
void StartWebUIService();
void StopWebUIService();
void InstallService();
void RemoveService();

int RunServer();

bool RunJetty(WCHAR *commandLine);
void StopJetty();

extern WCHAR g_installDir[];
extern WCHAR g_javaExecutable[];

#endif
