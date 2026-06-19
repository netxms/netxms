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
** File: object_dci.cpp
**
** Data collection configuration endpoints (create / read / modify / delete DCIs on objects).
**
**/

#include "object_helpers.h"

/**
 * Load object addressed by URL placeholder "object-id", verify that it supports data collection
 * (data collection target or template) and that the caller has the given access rights. On failure
 * returns nullptr and writes the matching HTTP status code to *httpCode.
 */
static shared_ptr<DataCollectionOwner> LoadDataCollectionOwner(Context *context, uint32_t requiredRights, int *httpCode)
{
   shared_ptr<NetObj> object = LoadObjectForModify(context, requiredRights, httpCode);
   if (object == nullptr)
      return shared_ptr<DataCollectionOwner>();

   if (!object->isDataCollectionTarget() && (object->getObjectClass() != OBJECT_TEMPLATE))
   {
      context->setErrorResponse("Object does not support data collection");
      *httpCode = 400;
      return shared_ptr<DataCollectionOwner>();
   }

   return static_pointer_cast<DataCollectionOwner>(object);
}

/**
 * Handler for GET /v1/objects/:object-id/data-collection/configuration
 * Returns the definitions of all data collection items configured on the object.
 */
int H_DataCollectionConfigList(Context *context)
{
   int httpCode = 0;
   shared_ptr<DataCollectionOwner> owner = LoadDataCollectionOwner(context, OBJECT_ACCESS_READ, &httpCode);
   if (owner == nullptr)
      return httpCode;

   unique_ptr<SharedObjectArray<DCObject>> dcObjects = owner->getAllDCObjects(context->getUserId());
   json_t *output = json_array();
   for(int i = 0; i < dcObjects->size(); i++)
      json_array_append_new(output, dcObjects->get(i)->toJson());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/data-collection/configuration/:dci-id
 * Returns the definition of a single data collection item.
 */
int H_DataCollectionConfigGet(Context *context)
{
   int httpCode = 0;
   shared_ptr<DataCollectionOwner> owner = LoadDataCollectionOwner(context, OBJECT_ACCESS_READ, &httpCode);
   if (owner == nullptr)
      return httpCode;

   uint32_t dciId = context->getPlaceholderValueAsUInt32(L"dci-id");
   shared_ptr<DCObject> dcObject = owner->getDCObjectById(dciId, context->getUserId());
   if (dcObject == nullptr)
      return 404;

   json_t *output = dcObject->toJson();
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/data-collection/configuration
 * Creates a new data collection item. The DCI type is selected by the "type" property in the body
 * ("item" or "table", default "item"). The server assigns the DCI id.
 */
int H_DataCollectionConfigCreate(Context *context)
{
   int httpCode = 0;
   shared_ptr<DataCollectionOwner> owner = LoadDataCollectionOwner(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (owner == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   String type = json_object_get_string(request, "type", L"item");
   DCObject *dcObject;
   if (!_tcsicmp(type, L"item"))
   {
      dcObject = new DCItem(CreateUniqueId(IDG_ITEM), L"unnamed", DS_INTERNAL, DCI_DT_INT,
            DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr, owner);
   }
   else if (!_tcsicmp(type, L"table"))
   {
      dcObject = new DCTable(CreateUniqueId(IDG_ITEM), L"unnamed", DS_INTERNAL,
            DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr, owner);
   }
   else
   {
      context->setErrorResponse("Invalid DCI type (must be \"item\" or \"table\")");
      return 400;
   }

   uint32_t rcc = dcObject->updateFromJSON(request, true);
   if (rcc != RCC_SUCCESS)
   {
      delete dcObject;
      context->setErrorResponse("Invalid data collection item definition");
      return 400;
   }

   uint32_t dciId = dcObject->getId();
   if (!owner->addDCObject(dcObject))   // takes ownership and notifies clients
   {
      delete dcObject;
      return 500;
   }
   owner->applyDCIChanges(true);

   shared_ptr<DCObject> added = owner->getDCObjectById(dciId, 0, true);
   json_t *newValue = (added != nullptr) ? added->toJson() : json_object();
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, owner->getId(), nullptr, newValue,
         L"Data collection configuration item [%u] created on object %s [%u]", dciId, owner->getName(), owner->getId());

   wchar_t location[256];
   _sntprintf(location, 256, L"/v1/objects/%u/data-collection/configuration/%u", owner->getId(), dciId);
   context->setResponseHeader(L"Location", location);

   context->setResponseData(newValue);
   json_decref(newValue);
   return 201;
}

/**
 * Handler for PATCH /v1/objects/:object-id/data-collection/configuration/:dci-id
 * Applies a JSON merge-patch to an existing data collection item.
 */
int H_DataCollectionConfigUpdate(Context *context)
{
   int httpCode = 0;
   shared_ptr<DataCollectionOwner> owner = LoadDataCollectionOwner(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (owner == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   uint32_t dciId = context->getPlaceholderValueAsUInt32(L"dci-id");
   shared_ptr<DCObject> dcObject = owner->getDCObjectById(dciId, context->getUserId());
   if (dcObject == nullptr)
      return 404;

   // "type", if supplied, must match the existing DCI type - it cannot be changed
   json_t *typeField = json_object_get(request, "type");
   if (json_is_string(typeField))
   {
      String type(json_string_value(typeField), "utf8");
      bool typeMatches = (dcObject->getType() == DCO_TYPE_TABLE) ? !_tcsicmp(type, L"table") : !_tcsicmp(type, L"item");
      if (!typeMatches)
      {
         context->setErrorResponse("DCI type cannot be changed");
         return 400;
      }
   }

   json_t *oldValue = dcObject->toJson();
   uint32_t rcc = dcObject->updateFromJSON(request, false);
   if (rcc != RCC_SUCCESS)
   {
      json_decref(oldValue);
      context->setErrorResponse("Invalid data collection item definition");
      return (rcc == RCC_ACCESS_DENIED) ? 403 : 400;
   }

   NotifyClientsOnDCIUpdate(*owner, dcObject.get());
   owner->applyDCIChanges(true);

   json_t *newValue = dcObject->toJson();
   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, owner->getId(), oldValue, newValue,
         L"Data collection configuration item [%u] on object %s [%u] updated", dciId, owner->getName(), owner->getId());
   json_decref(oldValue);

   context->setResponseData(newValue);
   json_decref(newValue);
   return 200;
}

/**
 * Handler for DELETE /v1/objects/:object-id/data-collection/configuration/:dci-id
 * Removes a data collection item.
 */
int H_DataCollectionConfigDelete(Context *context)
{
   int httpCode = 0;
   shared_ptr<DataCollectionOwner> owner = LoadDataCollectionOwner(context, OBJECT_ACCESS_MODIFY, &httpCode);
   if (owner == nullptr)
      return httpCode;

   uint32_t dciId = context->getPlaceholderValueAsUInt32(L"dci-id");
   uint32_t rcc = RCC_SUCCESS;
   json_t *oldValue = nullptr;
   if (!owner->deleteDCObject(dciId, true, context->getUserId(), &rcc, &oldValue))   // notifies clients
   {
      if (rcc == RCC_ACCESS_DENIED)
      {
         context->writeAuditLog(AUDIT_OBJECTS, false, owner->getId(),
               L"Access denied on delete of data collection configuration item [%u] for object %s [%u]", dciId, owner->getName(), owner->getId());
         return 403;
      }
      return 404;
   }
   owner->applyDCIChanges(true);

   context->writeAuditLogWithValues(AUDIT_OBJECTS, true, owner->getId(), oldValue, nullptr,
         L"Data collection configuration item [%u] on object %s [%u] deleted", dciId, owner->getName(), owner->getId());
   json_decref(oldValue);
   return 204;
}
