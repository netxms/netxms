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
** File: snmp_mib.cpp
**
**/

#include "webapi.h"
#include <nxsnmp.h>

/**
 * Map MIB_TYPE_* constant to symbolic name
 */
static const char *MibTypeName(int type)
{
   switch (type)
   {
      case MIB_TYPE_OTHER:                 return "OTHER";
      case MIB_TYPE_IMPORT_ITEM:           return "IMPORT_ITEM";
      case MIB_TYPE_OBJID:                 return "OBJID";
      case MIB_TYPE_BITSTRING:             return "BITSTRING";
      case MIB_TYPE_INTEGER:               return "INTEGER";
      case MIB_TYPE_INTEGER32:             return "INTEGER32";
      case MIB_TYPE_INTEGER64:             return "INTEGER64";
      case MIB_TYPE_UNSIGNED32:            return "UNSIGNED32";
      case MIB_TYPE_COUNTER:               return "COUNTER";
      case MIB_TYPE_COUNTER32:             return "COUNTER32";
      case MIB_TYPE_COUNTER64:             return "COUNTER64";
      case MIB_TYPE_GAUGE:                 return "GAUGE";
      case MIB_TYPE_GAUGE32:               return "GAUGE32";
      case MIB_TYPE_TIMETICKS:             return "TIMETICKS";
      case MIB_TYPE_OCTETSTR:              return "OCTETSTR";
      case MIB_TYPE_OPAQUE:                return "OPAQUE";
      case MIB_TYPE_IPADDR:                return "IPADDR";
      case MIB_TYPE_PHYSADDR:              return "PHYSADDR";
      case MIB_TYPE_NETADDR:               return "NETADDR";
      case MIB_TYPE_NAMED_TYPE:            return "NAMED_TYPE";
      case MIB_TYPE_SEQID:                 return "SEQID";
      case MIB_TYPE_SEQUENCE:              return "SEQUENCE";
      case MIB_TYPE_CHOICE:                return "CHOICE";
      case MIB_TYPE_TEXTUAL_CONVENTION:    return "TEXTUAL_CONVENTION";
      case MIB_TYPE_MACRO_DEFINITION:      return "MACRO_DEFINITION";
      case MIB_TYPE_MODCOMP:               return "MODCOMP";
      case MIB_TYPE_TRAPTYPE:              return "TRAPTYPE";
      case MIB_TYPE_NOTIFTYPE:             return "NOTIFTYPE";
      case MIB_TYPE_MODID:                 return "MODID";
      case MIB_TYPE_NSAPADDRESS:           return "NSAPADDRESS";
      case MIB_TYPE_AGENTCAP:              return "AGENTCAP";
      case MIB_TYPE_UINTEGER:              return "UINTEGER";
      case MIB_TYPE_NULL:                  return "NULL";
      case MIB_TYPE_OBJGROUP:              return "OBJGROUP";
      case MIB_TYPE_NOTIFGROUP:            return "NOTIFGROUP";
      default:                             return nullptr;
   }
}

/**
 * Map MIB_STATUS_* constant to symbolic name
 */
static const char *MibStatusName(int status)
{
   switch (status)
   {
      case MIB_STATUS_MANDATORY:  return "MANDATORY";
      case MIB_STATUS_OPTIONAL:   return "OPTIONAL";
      case MIB_STATUS_OBSOLETE:   return "OBSOLETE";
      case MIB_STATUS_DEPRECATED: return "DEPRECATED";
      case MIB_STATUS_CURRENT:    return "CURRENT";
      default:                    return nullptr;
   }
}

/**
 * Map MIB_ACCESS_* constant to symbolic name
 */
static const char *MibAccessName(int access)
{
   switch (access)
   {
      case MIB_ACCESS_READONLY:  return "READONLY";
      case MIB_ACCESS_READWRITE: return "READWRITE";
      case MIB_ACCESS_WRITEONLY: return "WRITEONLY";
      case MIB_ACCESS_NOACCESS:  return "NOACCESS";
      case MIB_ACCESS_NOTIFY:    return "NOTIFY";
      case MIB_ACCESS_CREATE:    return "CREATE";
      default:                   return nullptr;
   }
}

/**
 * Build dotted OID for given MIB node by walking up to the root.
 * The synthetic root (m_pParent == nullptr) contributes no sub-id; the returned
 * string is empty for the root and "1.3.6.1..." for everything else (no leading dot).
 */
static StringBuffer BuildOidPath(SNMP_MIBObject *node)
{
   IntegerArray<uint32_t> ids(16, 16);
   for (SNMP_MIBObject *curr = node; (curr != nullptr) && (curr->getParent() != nullptr); curr = curr->getParent())
      ids.add(curr->getObjectId());

   StringBuffer result;
   for (int i = ids.size() - 1; i >= 0; i--)
   {
      if (!result.isEmpty())
         result.append(L'.');
      result.append(ids.get(i));
   }
   return result;
}

/**
 * Strict OID lookup - returns nullptr unless every sub-id resolves to a real child.
 * (SnmpFindMIBObjectByOID is lenient: it returns the deepest partial match.)
 */
static SNMP_MIBObject *FindMibNodeExact(SNMP_MIBObject *root, const SNMP_ObjectId& oid)
{
   SNMP_MIBObject *curr = root;
   for (size_t i = 0; i < oid.length(); i++)
   {
      curr = curr->findChildByID(oid.value()[i]);
      if (curr == nullptr)
         return nullptr;
   }
   return curr;
}

/**
 * Build JSON object describing one child (subId, full OID, name, hasChildren).
 */
static json_t *ChildToJson(SNMP_MIBObject *parent, const StringBuffer& parentOid, SNMP_MIBObject *child)
{
   json_t *json = json_object();
   json_object_set_new(json, "subId", json_integer(child->getObjectId()));

   StringBuffer childOid(parentOid);
   if (!childOid.isEmpty())
      childOid.append(L'.');
   childOid.append(child->getObjectId());
   json_object_set_new(json, "oid", json_string_w(childOid.cstr()));

   json_object_set_new(json, "name", (child->getName() != nullptr) ? json_string_t(child->getName()) : json_null());
   json_object_set_new(json, "hasChildren", json_boolean(child->getFirstChild() != nullptr));
   return json;
}

/**
 * Build full JSON response for a MIB node.
 */
static json_t *NodeToJson(SNMP_MIBObject *node)
{
   StringBuffer oidPath = BuildOidPath(node);

   json_t *response = json_object();
   json_object_set_new(response, "oid", json_string_w(oidPath.cstr()));
   json_object_set_new(response, "name", (node->getName() != nullptr) ? json_string_t(node->getName()) : json_null());

   const char *typeName = MibTypeName(node->getType());
   json_object_set_new(response, "type", (typeName != nullptr) ? json_string(typeName) : json_null());
   json_object_set_new(response, "typeCode", json_integer(node->getType()));

   const char *statusName = MibStatusName(node->getStatus());
   json_object_set_new(response, "status", (statusName != nullptr) ? json_string(statusName) : json_null());
   json_object_set_new(response, "statusCode", json_integer(node->getStatus()));

   const char *accessName = MibAccessName(node->getAccess());
   json_object_set_new(response, "access", (accessName != nullptr) ? json_string(accessName) : json_null());
   json_object_set_new(response, "accessCode", json_integer(node->getAccess()));

   json_object_set_new(response, "description",
      (node->getDescription() != nullptr) ? json_string_t(node->getDescription()) : json_null());
   json_object_set_new(response, "textualConvention",
      (node->getTextualConvention() != nullptr) ? json_string_t(node->getTextualConvention()) : json_null());
   json_object_set_new(response, "displayHint",
      (node->getDisplayHint() != nullptr) ? json_string_t(node->getDisplayHint()) : json_null());
   json_object_set_new(response, "enumValues",
      (node->getEnumValues() != nullptr) ? json_string_t(node->getEnumValues()) : json_null());
   json_object_set_new(response, "index",
      (node->getIndex() != nullptr) ? json_string_t(node->getIndex()) : json_null());

   if (node->getParent() != nullptr)
   {
      StringBuffer parentOid = BuildOidPath(node->getParent());
      json_object_set_new(response, "parent", json_string_w(parentOid.cstr()));
   }
   else
   {
      json_object_set_new(response, "parent", json_null());
   }

   json_t *children = json_array();
   for (SNMP_MIBObject *child = node->getFirstChild(); child != nullptr; child = child->getNext())
      json_array_append_new(children, ChildToJson(node, oidPath, child));
   json_object_set_new(response, "children", children);

   return response;
}

/**
 * Handler for GET /v1/snmp-mib and GET /v1/snmp-mib/:oid
 */
int H_GetMibNode(Context *context)
{
   const wchar_t *oidParam = context->getPlaceholderValue(L"oid");

   SNMP_ObjectId parsedOid;
   if (oidParam != nullptr)
   {
      parsedOid = SNMP_ObjectId::parse(oidParam);
      if (parsedOid.length() == 0)
      {
         context->setErrorResponse("Invalid OID");
         return 400;
      }
   }

   SNMP_MIBObject *root = AcquireMIBTreeReadLock();
   if (root == nullptr)
   {
      context->setErrorResponse("MIB tree not loaded");
      return 503;
   }

   SNMP_MIBObject *node = (oidParam != nullptr) ? FindMibNodeExact(root, parsedOid) : root;
   if (node == nullptr)
   {
      ReleaseMIBTreeReadLock();
      context->setErrorResponse("MIB object not found");
      return 404;
   }

   json_t *response = NodeToJson(node);
   ReleaseMIBTreeReadLock();

   context->setResponseData(response);
   json_decref(response);
   return 200;
}
