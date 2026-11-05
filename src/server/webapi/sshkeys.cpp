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
** File: sshkeys.cpp
**
**/

#include "webapi.h"

/**
 * Map SSH key operation error to HTTP status and response
 */
static int SshKeyErrorResponse(Context *context, uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_INVALID_OBJECT_NAME:
         context->setErrorResponse("Invalid name");
         return 400;
      case RCC_INVALID_ARGUMENT:
         context->setErrorResponse("Invalid private key. Expected an unencrypted OpenSSH private key (convert with ssh-keygen -p -N \"\" -f <file>)");
         return 400;
      case RCC_INVALID_SSH_KEY_ID:
         return 404;
      default:
         context->setErrorResponse("Internal error");
         return 500;
   }
}

/**
 * Handler for GET /v1/ssh-keys
 */
int H_SshKeys(Context *context)
{
   bool includePublicKey = context->getQueryParameterAsBoolean("includePublicKey");
   json_t *output = GetSshKeysAsJson(includePublicKey);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/ssh-keys/:key-id
 */
int H_SshKeyDetails(Context *context)
{
   uint32_t keyId = context->getPlaceholderValueAsUInt32(L"key-id");
   if (keyId == 0)
      return 400;

   json_t *output = GetSshKeyByIdAsJson(keyId);
   if (output == nullptr)
      return 404;

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/ssh-keys
 */
int H_SshKeyCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SSH_KEY_CONFIGURATION))
      return 403;

   json_t *request = context->getRequestDocument();
   if (!json_is_object(request))
      return 400;

   uint32_t keyId = 0;
   uint32_t rcc;

   if (json_object_get(request, "privateKey") != nullptr)
   {
      // Import private key and derive public key
      rcc = CreateOrEditSshKey(0, request, &keyId);
   }
   else
   {
      // Generate new key pair
      String name = json_object_get_string(request, "name", L"");
      rcc = GenerateSshKey(name, &keyId);
   }

   if (rcc != RCC_SUCCESS)
      return SshKeyErrorResponse(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"SSH key [%u] created", keyId);

   json_t *output = GetSshKeyByIdAsJson(keyId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 201;
}

/**
 * Handler for PUT /v1/ssh-keys/:key-id
 */
int H_SshKeyUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SSH_KEY_CONFIGURATION))
      return 403;

   uint32_t keyId = context->getPlaceholderValueAsUInt32(L"key-id");
   if (keyId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (!json_is_object(request))
      return 400;

   uint32_t rcc = CreateOrEditSshKey(keyId, request);
   if (rcc != RCC_SUCCESS)
      return SshKeyErrorResponse(context, rcc);

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"SSH key [%u] updated", keyId);

   json_t *output = GetSshKeyByIdAsJson(keyId);
   if (output != nullptr)
   {
      context->setResponseData(output);
      json_decref(output);
   }
   return 200;
}

/**
 * Handler for DELETE /v1/ssh-keys/:key-id
 */
int H_SshKeyDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SSH_KEY_CONFIGURATION))
      return 403;

   uint32_t keyId = context->getPlaceholderValueAsUInt32(L"key-id");
   if (keyId == 0)
      return 400;

   bool forceDelete = context->getQueryParameterAsBoolean("force");
   SharedObjectArray<NetObj> *references = nullptr;
   uint32_t rcc = DeleteSshKey(keyId, forceDelete, &references);

   if (rcc == RCC_INVALID_SSH_KEY_ID)
   {
      return 404;
   }

   if (rcc == RCC_SSH_KEY_IN_USE)
   {
      json_t *output = json_object();
      json_object_set_new(output, "reason", json_string("SSH key is in use"));
      json_t *nodes = json_array();
      if (references != nullptr)
      {
         for (int i = 0; i < references->size(); i++)
            json_array_append_new(nodes, json_integer(references->get(i)->getId()));
         delete references;
      }
      json_object_set_new(output, "nodesInUse", nodes);
      context->setResponseData(output);
      json_decref(output);
      return 409;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"SSH key [%u] deleted", keyId);
   return 204;
}
