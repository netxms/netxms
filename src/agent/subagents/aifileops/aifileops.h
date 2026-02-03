/*
** NetXMS AI File Operations subagent
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: aifileops.h
**
**/

#ifndef _aifileops_h_
#define _aifileops_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <jansson/jansson.h>

#define DEBUG_TAG _T("aifileops")

// Default limits
#define DEFAULT_MAX_LINES        10000
#define DEFAULT_MAX_MATCHES      1000
#define DEFAULT_MAX_FILE_SIZE    (10 * 1024 * 1024)  // 10 MB
#define DEFAULT_CONTEXT_LINES    3

// Tool handlers
uint32_t H_LogGrep(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogRead(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogTail(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogHead(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogFind(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogTimeRange(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogStats(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileInfo(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileList(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileRead(json_t *params, json_t **result, AbstractCommSession *session);

#endif
