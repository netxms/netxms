/*
** NetXMS Session Agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxsagent.h
**
**/

#ifndef _nxsagent_h_
#define _nxsagent_h_

#define _WIN32_WINNT 0x0501

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nms_agent.h>
#include <nms_cscp.h>
#include <nxcpapi.h>
#include <wtsapi32.h>

void TakeScreenshot(NXCPMessage *response);
bool SaveBitmapToPng(HBITMAP hBitmap, const TCHAR *fileName);

#endif
