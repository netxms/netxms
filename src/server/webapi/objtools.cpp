/*
** NetXMS - Network Management System
** Copyright (C) 2023 Raden Solutions
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
** File: objects.cpp
**
**/

#include "webapi.h"

/**
 * Handler for /v1/object-tools
 */
int H_ObjectTools(Context *context)
{
   const char *typesFilter = context->getQueryParameter("types");
   json_t *tools = GetObjectToolsIntoJSON(context->getUserId(), context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS), typesFilter);
   if (tools == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }
   context->setResponseData(tools);
   json_decref(tools);
   return 200;
}

/**
 * Handler for /v1/object-tools/:tool-id
 */
int H_ObjectToolDetails(Context *context)
{
   uint32_t toolId = context->getPlaceholderValueAsUInt32(_T("tool-id"));
   if (toolId == 0)
      return 400;

   json_t *tool = GetObjectToolIntoJSON(toolId, context->getUserId(), context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS));
   if (tool == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   if (json_is_null(tool))
   {
      json_decref(tool);
      return 403;
   }

   context->setResponseData(tool);
   json_decref(tool);
   return 200;
}
