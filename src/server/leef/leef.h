/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: leef.h
**
**/

#ifndef _leef_h_
#define _leef_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <nxconfig.h>

#define DEBUG_TAG_LEEF  _T("leef")

void StartLeefSender();
void StopLeefSender();
void EnqueueLeefAuditRecord(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
         session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *text);
void EnqueueLeefAuditRecord(const NXCPMessage& msg, uint32_t nodeId);

extern InetAddress g_leefServer;
extern uint16_t g_leefPort;
extern char g_leefVendor[];
extern char g_leefProduct[];
extern char g_leefProductVersion[];
extern char g_leefHostname[];
extern uint32_t g_leefEventCode;
extern BYTE g_leefSeparatorChar;
extern bool g_leefRFC5424Timestamp;
extern int g_leefSyslogFacility;
extern int g_leefSyslogSeverity;
extern char *g_leefExtraData;

#endif
