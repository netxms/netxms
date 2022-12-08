/*
** NetXMS Network Service check subagent
** Copyright (C) 2013-2022 Alex Kirhenshtein
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
**/

#ifndef __netsvc__h__
#define __netsvc__h__

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <curl/curl.h>
#include <netxms-regex.h>

#define DEBUG_TAG _T("netsvc")

enum
{
	PC_ERR_NONE,
	PC_ERR_BAD_PARAMS,
	PC_ERR_CONNECT,
    PC_ERR_NOMATCH,
	PC_ERR_INTERNAL,
    PC_ERR_HANDSHAKE
};

#define NETSVC_AF_VERIFYPEER 1
#define SCF_NEGATIVE_TIME_ON_ERROR 0x0001

LONG H_CheckServiceSMTP(CURL *curl, TCHAR *value, const OptionList& options);
LONG H_CheckServiceHTTP(CURL *curl, TCHAR *value, const OptionList& options, char *url, PCRE *compiledPattern);
LONG H_CheckServiceTELNET(char *szHost, const TCHAR *szPort, TCHAR *value, const OptionList& options);
LONG H_CheckServiceSSH(char *szHost, const TCHAR *szPort, TCHAR *value, const OptionList& options);
LONG H_CheckServiceDefault(CURL *curl, TCHAR *value);

LONG H_CheckTLS(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);
LONG H_TLSCertificateInfo(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session);

void CurlCommonSetup(CURL *curl, char *url, const OptionList& options);

extern uint32_t g_serviceCheckFlags;
extern uint32_t g_netsvcFlags;
extern uint32_t g_netsvcTimeout;
extern char g_certBundle[];

#endif // __netsvc__h__
