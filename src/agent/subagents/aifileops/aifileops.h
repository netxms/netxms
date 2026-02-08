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

// Write operation limits
#define MAX_WRITE_SIZE           (10 * 1024 * 1024)  // 10 MB max content size
#define MAX_FILE_SIZE_MODIFY     (50 * 1024 * 1024)  // 50 MB max file to modify

/**
 * Helper: Set JSON error response
 */
static inline void SetError(json_t **result, const char *code, const char *message)
{
   *result = json_object();
   json_t *error = json_object();
   json_object_set_new(error, "code", json_string(code));
   json_object_set_new(error, "message", json_string(message));
   json_object_set_new(*result, "error", error);
}

// Tool handlers - log operations
uint32_t H_LogGrep(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogRead(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogTail(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogHead(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogFind(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogTimeRange(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_LogStats(json_t *params, json_t **result, AbstractCommSession *session);

// Tool handlers - file read operations
uint32_t H_FileInfo(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileList(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileRead(json_t *params, json_t **result, AbstractCommSession *session);

// Tool handlers - file write/patch operations
uint32_t H_FileWrite(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileAppend(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileInsert(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileDeleteLines(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FileReplace(json_t *params, json_t **result, AbstractCommSession *session);
uint32_t H_FilePatch(json_t *params, json_t **result, AbstractCommSession *session);

#endif
