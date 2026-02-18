/*
** NetXMS Oxidized integration module
** Copyright (C) 2026 Raden Solutions
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
** File: oxidized.h
**/

#ifndef _oxidized_h_
#define _oxidized_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <device-backup.h>
#include <netxms-webapi.h>

#define DEBUG_TAG L"oxidized"

/**
 * Do GET request to Oxidized REST API, returns parsed JSON response.
 * Caller must call json_decref() on returned object.
 */
json_t *OxidizedApiRequest(const char *endpoint);

/**
 * Do GET request to Oxidized REST API, returns raw text response.
 * Caller must call MemFree() on returned string.
 */
char *OxidizedApiRequestText(const char *endpoint, size_t *responseSize = nullptr);

/**
 * Get cached node list from Oxidized (GET /nodes.json).
 * Caller must call json_decref() on returned object.
 * Returns nullptr on failure.
 */
json_t *GetCachedNodeList();

/**
 * Invalidate the node list cache.
 */
void InvalidateNodeCache();

/**
 * Find node in Oxidized node list by name (NetXMS node ID).
 * Returns borrowed reference to json object within nodeList array, or nullptr if not found.
 */
json_t *FindOxidizedNodeByName(json_t *nodeList, const char *nodeName);

/**
 * Resolve Oxidized model name from node vendor string.
 * Returns empty string if no mapping found and no default model configured.
 */
std::string ResolveOxidizedModel(const TCHAR *vendor);

/**
 * Global configuration variables
 */
extern char g_oxidizedBaseURL[256];
extern char g_oxidizedLogin[128];
extern char g_oxidizedPassword[128];
extern char g_oxidizedDefaultModel[64];

#endif
