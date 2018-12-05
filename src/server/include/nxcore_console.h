/*
** NetXMS - Network Management System
** Server Core
** Copyright (C) 2018 RadenSolutions
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
** File: nxcore_console.h
**
**/



#ifndef _nxcore_console_h_
#define _nxcore_console_h_

#include <nms_common.h>
#include <nxcpapi.h>

class ClientSession;

/**
 * Console context
 */
struct __console_ctx
{
   SOCKET hSocket;
   MUTEX socketMutex;
   NXCPMessage *pMsg;
   ClientSession *session;
   String *output;
};

typedef __console_ctx * CONSOLE_CTX;

void NXCORE_EXPORTABLE ConsolePrintf(CONSOLE_CTX pCtx, const TCHAR *pszFormat, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void NXCORE_EXPORTABLE ConsoleWrite(CONSOLE_CTX pCtx, const TCHAR *text);

#endif   /* _nxcore_console_h_ */
