/*
** NetXMS - Network Management System
** Copyright (C) 2024 Raden Solutions
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
** File: find.cpp
**
**/

#include "webapi.h"
#include <nms_topo.h>

/**
 * Handler for /v1/find/mac-address
 */
int H_FindMacAddress(Context *context)
{
   json_t *output = json_array();

   const char *macAddress = context->getQueryParameter("macAddress");
   int searchLimit = context->getQueryParameterAsInt32("searchLimit", 100);
   bool includeObject = context->getQueryParameterAsBoolean("includeObjects", false);
   MacAddress mac = MacAddress::parse(macAddress, true);

   ObjectArray<MacAddressInfo> icpl(0, 16, Ownership::True);
   FindMacAddresses(mac.value(), mac.length(), &icpl, searchLimit);

   for(int i = 0; i < icpl.size() && i < searchLimit; i++)
   {
      json_t *json = json_object();
      icpl.get(i)->fillJson(json, context->getUserId(), includeObject, CreateObjectSummary);
      json_array_append_new(output, json);
   }

   context->setResponseData(output);
   json_decref(output);
   return 200;
}
