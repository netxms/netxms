/*
** NetXMS - Network Management System
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
** File: object_helpers.h
**
** Local helpers shared by object property handlers (object_properties.cpp,
** object_collections.cpp, object_node_config.cpp, object_dci.cpp). Not
** exposed via webapi.h to keep these out of unrelated handlers.
**
**/

#ifndef _object_helpers_h_
#define _object_helpers_h_

#include "webapi.h"

/**
 * Load object by URL placeholder "object-id" and check OBJECT_ACCESS_MODIFY.
 * Returns the object on success. On failure returns nullptr and writes the
 * corresponding HTTP status code to *httpCode (400 / 403 / 404).
 */
shared_ptr<NetObj> LoadObjectForModify(Context *context, int *httpCode);

#endif
