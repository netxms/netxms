/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: nc.cpp
**
**/

#include "aitools.h"

/**
 * Read SNMP OID from given node
 */
std::string F_SNMPRead(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectByNameOrId(objectName, OBJECT_NODE));
   if ((node == nullptr) || !node->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!node->isSNMPSupported())
      return std::string("SNMP is not supported on the specified node");

   const char *oidStr = json_object_get_string_utf8(arguments, "oid", nullptr);
   if ((oidStr == nullptr) || (oidStr[0] == 0))
      return std::string("SNMP OID must be provided");

   SNMP_ObjectId oid = SNMP_ObjectId::parseA(oidStr);
   if (oid.length() == 0)
      return std::string("Invalid SNMP OID format");

   SNMP_Transport *transport = node->createSnmpTransport();
   if (transport == nullptr)
      return std::string("Failed to create SNMP transport for the specified node");

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(oid));
   SNMP_PDU *response = nullptr;
   uint32_t rc = transport->doRequest(&request, &response);
   delete transport;
   if (rc != SNMP_ERR_SUCCESS)
   {
      StringBuffer error(L"SNMP request failed: ");
      error.append(SnmpGetErrorText(rc));
      char buffer[256];
      wchar_to_utf8(error.cstr(), -1, buffer, 256);
      return std::string(buffer);
   }

   SNMP_Variable *v = response->getVariable(0);
   if ((v == nullptr) || (v->getType() == ASN_NULL))
   {
      delete response;
      return std::string("No such object");
   }

   json_t *output = json_object();
   char buffer[256];
   json_object_set_new(output, "oid", json_string(oid.toStringA(buffer, 256)));
   json_object_set_new(output, "type", json_string(SnmpDataTypeNameA(v->getType(), buffer, 256)));
   wchar_t value[256];
   json_object_set_new(output, "value", json_string_w(FormatSNMPValue(v, value, 256)));
   delete response;
   return JsonToString(output);
}

/**
 * Walk SNMP MIB from given node
 */
std::string F_SNMPWalk(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectByNameOrId(objectName, OBJECT_NODE));
   if ((node == nullptr) || !node->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!node->isSNMPSupported())
      return std::string("SNMP is not supported on the specified node");

   const char *oidStr = json_object_get_string_utf8(arguments, "oid", nullptr);
   if ((oidStr == nullptr) || (oidStr[0] == 0))
      return std::string("SNMP OID must be provided");

   SNMP_ObjectId oid = SNMP_ObjectId::parseA(oidStr);
   if (oid.length() == 0)
      return std::string("Invalid SNMP OID format");

   SNMP_Transport *transport = node->createSnmpTransport();
   if (transport == nullptr)
      return std::string("Failed to create SNMP transport for the specified node");

   int32_t limit = json_object_get_int32(arguments, "limit", 1000);
   int32_t count = 0;
   json_t *output = json_array();
   uint32_t rc = SnmpWalk(transport, oid,
      [output, limit, &count] (SNMP_Variable *v) -> uint32_t
      {
         if (count++ >= limit)
            return SNMP_ERR_SNAPSHOT_TOO_BIG;

         char buffer[256];
         json_t *entry = json_object();
         SNMP_ObjectId oid = v->getName();
         json_object_set_new(entry, "oid", json_string(oid.toStringA(buffer, 256)));
         json_object_set_new(entry, "type", json_string(SnmpDataTypeNameA(v->getType(), buffer, 256)));
         wchar_t value[256];
         json_object_set_new(entry, "value", json_string_w(FormatSNMPValue(v, value, 256)));
         json_array_append_new(output, entry);
         return SNMP_ERR_SUCCESS;
      });
   delete transport;

   if (rc != SNMP_ERR_SUCCESS)
   {
      json_decref(output);
      StringBuffer error(L"SNMP request failed: ");
      error.append(SnmpGetErrorText(rc));
      char buffer[256];
      wchar_to_utf8(error.cstr(), -1, buffer, 256);
      return std::string(buffer);
   }

   return JsonToString(output);
}
