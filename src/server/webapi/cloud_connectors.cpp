/*
** NetXMS - Network Management System
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
** File: cloud_connectors.cpp
**
**/

#include "webapi.h"
#include <cloud-connector.h>

/**
 * Handler for GET /v1/cloud-connectors
 */
int H_CloudConnectors(Context *context)
{
   json_t *output = GetCloudConnectorNames().toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
