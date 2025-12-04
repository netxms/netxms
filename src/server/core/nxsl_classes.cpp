/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxsl_classes.cpp
**
**/

#include "nxcore.h"
#include <entity_mib.h>
#include <ethernet_ip.h>
#include <agent_tunnel.h>
#include <nxcore_websvc.h>
#include <netxms_maps.h>
#include <asset_management.h>
#include <nms_users.h>

/**
 * Maintenance journal access
 */
bool AddMaintenanceJournalRecord(uint32_t objectId, uint32_t userId, const TCHAR *description);
NXSL_Value *ReadMaintenanceJournal(const shared_ptr<NetObj>& object, NXSL_VM *vm, time_t startTime, time_t endTime);

/**
 * Safely get pointer from object data
 */
template<typename T> static T *SharedObjectFromData(NXSL_Object *nxslObject)
{
   void *data = nxslObject->getData();
   return (data != nullptr) ? static_cast<shared_ptr<T>*>(data)->get() : nullptr;
}

/**
 * Generic implementation for flag changing methods
 */
static int ChangeFlagMethod(NXSL_Object *object, NXSL_Value *arg, NXSL_Value **result, uint32_t flag, bool invert)
{
   NetObj *nobject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   if (arg->isTrue())
   {
      if (invert)
         nobject->clearFlag(flag);
      else
         nobject->setFlag(flag);
   }
   else
   {
      if (invert)
         nobject->setFlag(flag);
      else
         nobject->clearFlag(flag);
   }

   *result = object->vm()->createValue();
   return 0;
}

/**
 * NetObj::bind(object)
 */
NXSL_METHOD_DEFINITION(NetObj, bind)
{
   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_COLLECTOR) &&
            (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> child = *static_cast<shared_ptr<NetObj>*>(nxslChild->getData());
   if (!IsValidParentClass(child->getObjectClass(), thisObject->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (child->isChild(thisObject->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   NetObj::linkObjects(thisObject, child);
   thisObject->calculateCompoundStatus();

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::bindTo(object)
 */
NXSL_METHOD_DEFINITION(NetObj, bindTo)
{
   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> parent = *static_cast<shared_ptr<NetObj>*>(nxslParent->getData());
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_COLLECTOR) &&
            (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!IsValidParentClass(thisObject->getObjectClass(), parent->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (thisObject->isChild(parent->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   NetObj::linkObjects(parent, thisObject);
   parent->calculateCompoundStatus();

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::calculateDowntime(tag, periodStart, periodEnd)
 */
NXSL_METHOD_DEFINITION(NetObj, calculateDowntime)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if (!argv[1]->isInteger() || !argv[2]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   uint32_t objectId = static_cast<shared_ptr<NetObj>*>(object->getData())->get()->getId();
   StructArray<DowntimeInfo> downtimes = CalculateDowntime(objectId, static_cast<time_t>(argv[1]->getValueAsInt64()), static_cast<time_t>(argv[2]->getValueAsInt64()), argv[0]->getValueAsCString());
   if (!downtimes.isEmpty())
   {
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < downtimes.size(); i++)
         a->append(vm->createValue(vm->createObject(&g_nxslDowntimeInfoClass, MemCopyBlock(downtimes.get(i), sizeof(DowntimeInfo)))));
      *result = vm->createValue(a);
   }
   else
   {
      *result = vm->createValue();
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::clearGeoLocation()
 */
NXSL_METHOD_DEFINITION(NetObj, clearGeoLocation)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setGeoLocation(GeoLocation());
   *result = vm->createValue();
   return 0;
}

/**
 * Create user agent notification
 * Syntax:
 *    createUserAgentNotification(message, startTime, endTime, showOnStartup)
 * where:
 *     message       - message to be sent
 *     startTime     - start time of message delivery
 *     endTime       - end time of message delivery
 *     showOnStartup - true to show message on startup (optional, defaults to false)
 * Return value:
 *     message id
 */
NXSL_METHOD_DEFINITION(NetObj, createUserAgentNotification)
{
   if ((argc < 3) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if (!argv[1]->isInteger() || !argv[2]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   uint32_t len = MAX_USER_AGENT_MESSAGE_SIZE;
   const TCHAR *message = argv[0]->getValueAsString(&len);
   IntegerArray<uint32_t> idList(16,16);
   idList.add(static_cast<shared_ptr<NetObj>*>(object->getData())->get()->getId());
   time_t startTime = static_cast<time_t>(argv[1]->getValueAsInt64());
   time_t endTime = static_cast<time_t>(argv[2]->getValueAsInt64());

   UserAgentNotificationItem *n = CreateNewUserAgentNotification(message, idList, startTime, endTime, (argc > 3) ? argv[3]->getValueAsBoolean() : false, 0);
   *result = vm->createValue(n->getId());
   n->decRefCount();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::delete()
 */
NXSL_METHOD_DEFINITION(NetObj, delete)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->deleteObject();
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::deleteCustomAttribute(name)
 * Returns last attribute value
 */
NXSL_METHOD_DEFINITION(NetObj, deleteCustomAttribute)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *netobj = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   const TCHAR *name = argv[0]->getValueAsCString();
   NXSL_Value *value = netobj->getCustomAttributeForNXSL(vm, name);
   *result = (value != nullptr) ? value : vm->createValue();
   netobj->deleteCustomAttribute(name);
   return 0;
}

/**
 * NetObj::enterMaintenance(comment)
 */
NXSL_METHOD_DEFINITION(NetObj, enterMaintenance)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc != 0) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->enterMaintenanceMode(0, (argc != 0) ? argv[0]->getValueAsCString() : nullptr);
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::expandString() method
 */
NXSL_METHOD_DEFINITION(NetObj, expandString)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *n = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   *result = vm->createValue(n->expandText(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * NetObj::getCustomAttribute(name)
 */
NXSL_METHOD_DEFINITION(NetObj, getCustomAttribute)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Value *value = static_cast<shared_ptr<NetObj>*>(object->getData())->get()->getCustomAttributeForNXSL(vm, argv[0]->getValueAsCString());
   *result = (value != nullptr) ? value : vm->createValue();
   return 0;
}

/**
 * NetObj::getResponsibleUsers(tag)
 */
NXSL_METHOD_DEFINITION(NetObj, getResponsibleUsers)
{
   if (!argv[0]->isString() && !argv[0]->isNull())
      return NXSL_ERR_NOT_STRING;

   NXSL_Array *array = new NXSL_Array(vm);
   unique_ptr<StructArray<ResponsibleUser>> responsibleUsers = static_cast<shared_ptr<NetObj>*>(object->getData())->get()->getAllResponsibleUsers(argv[0]->isNull() ? nullptr : argv[0]->getValueAsCString());
   unique_ptr<ObjectArray<UserDatabaseObject>> userDB = FindUserDBObjects(*responsibleUsers);
   userDB->setOwner(Ownership::False);
   for(int i = 0; i < userDB->size(); i++)
   {
      array->append(userDB->get(i)->createNXSLObject(vm));
   }
   *result = vm->createValue(array);
   return 0;
}

/**
 * Generic implementation for NetObj methods isParent, isChild, isDirectParent, isDirectChild
 */
template<bool (NObject::*method)(uint32_t) const> int TestObjectRelation(NXSL_VM *vm, NXSL_Object *object, NXSL_Value *arg, NXSL_Value **result)
{
   if (!arg->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslObject = arg->getValueAsObject();
   if (!nxslObject->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *testingObject = static_cast<shared_ptr<NetObj>*>(nxslObject->getData())->get();
   NetObj *thisObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   *result = vm->createValue((thisObject->*method)(testingObject->getId()));
   return 0;
}

/**
 * NetObj::isChild(object)
 */
NXSL_METHOD_DEFINITION(NetObj, isChild)
{
   return TestObjectRelation<&NObject::isChild>(vm, object, argv[0], result);
}

/**
 * NetObj::isDirectChild(object)
 */
NXSL_METHOD_DEFINITION(NetObj, isDirectChild)
{
   return TestObjectRelation<&NObject::isDirectChild>(vm, object, argv[0], result);
}

/**
 * NetObj::isDirectParent(object)
 */
NXSL_METHOD_DEFINITION(NetObj, isDirectParent)
{
   return TestObjectRelation<&NObject::isDirectParent>(vm, object, argv[0], result);
}

/**
 * NetObj::isParent(object)
 */
NXSL_METHOD_DEFINITION(NetObj, isParent)
{
   return TestObjectRelation<&NObject::isParent>(vm, object, argv[0], result);
}

/**
 * NetObj::leaveMaintenance()
 */
NXSL_METHOD_DEFINITION(NetObj, leaveMaintenance)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->leaveMaintenanceMode(0);
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::manage()
 */
NXSL_METHOD_DEFINITION(NetObj, manage)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(true);
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::readMaintenanceJournal(startTime, endTime)
 */
NXSL_METHOD_DEFINITION(NetObj, readMaintenanceJournal)
{
   if (argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (((argc > 0) && !argv[0]->isInteger()) || ((argc > 1) && !argv[1]->isInteger()))
      return NXSL_ERR_NOT_INTEGER;

   time_t startTime = (argc > 0) ? static_cast<time_t>(argv[0]->getValueAsInt64()) : 0;
   time_t endTime = (argc > 1) ? static_cast<time_t>(argv[1]->getValueAsInt64()) : 0;

   *result = ReadMaintenanceJournal(*static_cast<shared_ptr<NetObj>*>(object->getData()), vm, startTime, endTime);
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::rename(name)
 */
NXSL_METHOD_DEFINITION(NetObj, rename)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setName(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setAlias(text)
 */
NXSL_METHOD_DEFINITION(NetObj, setAlias)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setAlias(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setCategory(idOrName)
 */
NXSL_METHOD_DEFINITION(NetObj, setCategory)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<ObjectCategory> category =
            argv[0]->isInteger() ?
                     GetObjectCategory(argv[0]->getValueAsUInt32()) :
                     FindObjectCategoryByName(argv[0]->getValueAsCString());
   if (category != nullptr)
   {
      static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setCategoryId(category->getId());
      *result = vm->createValue(true);
   }
   else
   {
      *result = vm->createValue(false);
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * setComments(text, [isMarkdown])
 */
NXSL_METHOD_DEFINITION(NetObj, setComments)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() && !argv[0]->isNull())
      return NXSL_ERR_NOT_STRING;

   if ((argc == 1) || argv[1]->isNull())
   {
      static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setComments(argv[0]->getValueAsCString());
   }
   else
   {
      const TCHAR *s = argv[0]->getValueAsCString();
      if ((argv[1]->isTrue() && !_tcsncmp(s, _T("{\x7f}"), 3)) || (argv[1]->isFalse() && _tcsncmp(s, _T("{\x7f}"), 3)))
      {
         // Correct mode already
         static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setComments(s);
      }
      else
      {
         if (!_tcsncmp(s, _T("{\x7f}"), 3))
         {
            // Remove Markdown indicator
            static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setComments(&s[3]);
         }
         else
         {
            // Add Markdown indicator
            Buffer<TCHAR, 256> ms(_tcslen(s) + 4);
            memcpy(ms, _T("{\x7f}"), 3 * sizeof(TCHAR));
            _tcscpy(ms.buffer() + 3, s);
            static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setComments(ms);
         }
      }
   }

   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::setCustomAttribute(name, value, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setCustomAttribute)
{
   if (argc > 3 || argc < 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *netxmsObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   const TCHAR *name = argv[0]->getValueAsCString();
   NXSL_Value *value = netxmsObject->getCustomAttributeForNXSL(vm, name);
   *result = (value != nullptr) ? value : vm->createValue(); // Return nullptr if attribute not found
   StateChange inherit = (argc == 3) ? (argv[2]->getValueAsBoolean() ? StateChange::SET : StateChange::CLEAR) : StateChange::IGNORE;
   netxmsObject->setCustomAttribute(name, argv[1]->getValueAsCString(), inherit);
   return 0;
}

/**
 * setGeoLocation(loc)
 */
NXSL_METHOD_DEFINITION(NetObj, setGeoLocation)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *o = argv[0]->getValueAsObject();
   if (_tcscmp(o->getClass()->getName(), g_nxslGeoLocationClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   GeoLocation *gl = (GeoLocation *)o->getData();
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setGeoLocation(*gl);
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * setMapImage(image)
 */
NXSL_METHOD_DEFINITION(NetObj, setMapImage)
{
   if (!argv[0]->isString() && !argv[0]->isNull())
      return NXSL_ERR_NOT_STRING;

   bool success = false;
   if (argv[0]->isNull() || !_tcscmp(argv[0]->getValueAsCString(), _T("00000000-0000-0000-0000-000000000000")))
   {
      static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMapImage(uuid::NULL_UUID);
      success = true;
   }
   else
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid FROM images WHERE upper(guid)=upper(?) OR upper(name)=upper(?)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               uuid guid = DBGetFieldGUID(hResult, 0, 0);
               static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMapImage(guid);
               success = true;
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   *result = vm->createValue(success);
   return NXSL_ERR_SUCCESS;
}

/**
 * setNameOnMap(text)
 */
NXSL_METHOD_DEFINITION(NetObj, setNameOnMap)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setNameOnMap(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::setStatusCalculation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusCalculation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   switch(method)
   {
      case SA_CALCULATE_DEFAULT:
      case SA_CALCULATE_MOST_CRITICAL:
         netobj->setStatusCalculation(method);
         break;
      case SA_CALCULATE_SINGLE_THRESHOLD:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32());
         break;
      case SA_CALCULATE_MULTIPLE_THRESHOLDS:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = vm->createValue(success);
   return 0;
}

/**
 * NetObj::setStatusPropagation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusPropagation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   switch(method)
   {
      case SA_PROPAGATE_DEFAULT:
      case SA_PROPAGATE_UNCHANGED:
         netobj->setStatusPropagation(method);
         break;
      case SA_PROPAGATE_FIXED:
      case SA_PROPAGATE_RELATIVE:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32());
         break;
      case SA_PROPAGATE_TRANSLATED:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = vm->createValue(success);
   return 0;
}

/**
 * NetObj::unbind(object)
 */
NXSL_METHOD_DEFINITION(NetObj, unbind)
{
   NetObj *thisObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_COLLECTOR) &&
            (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *child = static_cast<shared_ptr<NetObj>*>(nxslChild->getData())->get();
   NetObj::unlinkObjects(thisObject, child);

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::unbindFrom(object)
 */
NXSL_METHOD_DEFINITION(NetObj, unbindFrom)
{
   NetObj *thisObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *parent = static_cast<shared_ptr<NetObj>*>(nxslParent->getData())->get();
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_COLLECTOR) &&
            (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   NetObj::unlinkObjects(parent, thisObject);

   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::unmanage()
 */
NXSL_METHOD_DEFINITION(NetObj, unmanage)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(false);
   *result = vm->createValue();
   return NXSL_ERR_SUCCESS;
}

/**
 * NetObj::writeMaintenanceJournal(description)
 */
NXSL_METHOD_DEFINITION(NetObj, writeMaintenanceJournal)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *thisObject = static_cast<shared_ptr<NetObj>*>(object->getData())->get();
   *result = vm->createValue(AddMaintenanceJournalRecord(thisObject->getId(), 0, argv[0]->getValueAsCString()));
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL class NetObj: constructor
 */
NXSL_NetObjClass::NXSL_NetObjClass() : NXSL_Class()
{
   setName(_T("NetObj"));

   NXSL_REGISTER_METHOD(NetObj, bind, 1);
   NXSL_REGISTER_METHOD(NetObj, bindTo, 1);
   NXSL_REGISTER_METHOD(NetObj, calculateDowntime, 3);
   NXSL_REGISTER_METHOD(NetObj, clearGeoLocation, 0);
   NXSL_REGISTER_METHOD(NetObj, createUserAgentNotification, -1);
   NXSL_REGISTER_METHOD(NetObj, delete, 0);
   NXSL_REGISTER_METHOD(NetObj, deleteCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, isDirectChild, 1);
   NXSL_REGISTER_METHOD(NetObj, isDirectParent, 1);
   NXSL_REGISTER_METHOD(NetObj, enterMaintenance, -1);
   NXSL_REGISTER_METHOD(NetObj, expandString, 1);
   NXSL_REGISTER_METHOD(NetObj, getCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, getResponsibleUsers, 1);
   NXSL_REGISTER_METHOD(NetObj, isChild, 1);
   NXSL_REGISTER_METHOD(NetObj, isParent, 1);
   NXSL_REGISTER_METHOD(NetObj, leaveMaintenance, 0);
   NXSL_REGISTER_METHOD(NetObj, manage, 0);
   NXSL_REGISTER_METHOD(NetObj, readMaintenanceJournal, -1);
   NXSL_REGISTER_METHOD(NetObj, rename, 1);
   NXSL_REGISTER_METHOD(NetObj, setAlias, 1);
   NXSL_REGISTER_METHOD(NetObj, setCategory, 1);
   NXSL_REGISTER_METHOD(NetObj, setComments, -1);
   NXSL_REGISTER_METHOD(NetObj, setCustomAttribute, -1);
   NXSL_REGISTER_METHOD(NetObj, setGeoLocation, 1);
   NXSL_REGISTER_METHOD(NetObj, setMapImage, 1);
   NXSL_REGISTER_METHOD(NetObj, setNameOnMap, 1);
   NXSL_REGISTER_METHOD(NetObj, setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(NetObj, setStatusPropagation, -1);
   NXSL_REGISTER_METHOD(NetObj, unbind, 1);
   NXSL_REGISTER_METHOD(NetObj, unbindFrom, 1);
   NXSL_REGISTER_METHOD(NetObj, unmanage, 0);
   NXSL_REGISTER_METHOD(NetObj, writeMaintenanceJournal, 1);
}

/**
 * Object destruction handler
 */
void NXSL_NetObjClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<NetObj>*>(object->getData());
}

/**
 * Convert object to JSON
 */
json_t *NXSL_NetObjClass::toJson(NXSL_Object *object, int depth)
{
   return SharedObjectFromData<NetObj>(object)->toJson();
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *_object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(_object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = _object->vm();
   auto object = SharedObjectFromData<NetObj>(_object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("alarms"))
   {
      ObjectArray<Alarm> *alarms = GetAlarms(object->getId(), true);
      alarms->setOwner(Ownership::False);
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < alarms->size(); i++)
         array->append(vm->createValue(vm->createObject(&g_nxslAlarmClass, alarms->get(i))));
      value = vm->createValue(array);
      delete alarms;
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("alarmStats"))
   {
      std::array<int, 5> stats = GetAlarmStatsForObject(object->getId());

      unique_ptr<SharedObjectArray<NetObj>> children = object->getAllChildren(true);
      for(int n = 0; n < children->size(); n++)
      {
         std::array<int, 5> cs = GetAlarmStatsForObject(children->get(n)->getId());
         for(int i = 0; i < 5; i++)
            stats[i] += cs[i];
      }

      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < 5; i++)
         array->append(vm->createValue(stats[i]));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("alias"))
   {
      value = vm->createValue(object->getAlias());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("asset"))
   {
      uint32_t assetId = object->getAssetId();
      if (assetId != 0)
      {
         shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(assetId, OBJECT_ASSET));
         value = (asset != nullptr) ? asset->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("assetId"))
   {
      value = vm->createValue(object->getAssetId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("assetProperties"))
   {
      uint32_t assetId = object->getAssetId();
      if (assetId != 0)
      {
         shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(assetId, OBJECT_ASSET));
         value = (asset != nullptr) ? vm->createValue(vm->createObject(&g_nxslAssetPropertiesClass, new shared_ptr<Asset>(asset))) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backupZoneProxy"))
   {
      uint32_t id = object->getAssignedZoneProxyId(true);
      if (id != 0)
      {
         shared_ptr<NetObj> proxy = FindObjectById(id, OBJECT_NODE);
         value = (proxy != nullptr) ? proxy->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backupZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(true));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("category"))
   {
      if (object->getCategoryId() != 0)
      {
         shared_ptr<ObjectCategory> category = GetObjectCategory(object->getCategoryId());
         value = (category != nullptr) ? vm->createValue(category->getName()) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("categoryId"))
   {
      value = vm->createValue(object->getCategoryId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("children"))
   {
      value = object->getChildrenForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("city"))
   {
      value = vm->createValue(object->getPostalAddress().getCity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("comments"))
   {
      value = vm->createValue(object->getComments());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("country"))
   {
      value = vm->createValue(object->getPostalAddress().getCountry());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("creationTime"))
   {
      value = vm->createValue(static_cast<INT64>(object->getCreationTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("customAttributes"))
   {
      value = object->getCustomAttributesForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("district"))
   {
      value = vm->createValue(object->getPostalAddress().getDistrict());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("geolocation"))
   {
      value = NXSL_GeoLocationClass::createObject(vm, object->getGeoLocation());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getGuid().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(object->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddr"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getPrimaryIpAddress().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddress"))
   {
      value = NXSL_InetAddressClass::createObject(vm, object->getPrimaryIpAddress());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isInMaintenanceMode"))
   {
      value = vm->createValue(object->isInMaintenanceMode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("maintenanceInitiator"))
   {
      value = vm->createValue(object->getMaintenanceInitiator());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("mapImage"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getMapImage().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(object->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("nameOnMap"))
   {
      value = vm->createValue(object->getNameOnMap());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("parents"))
   {
      value = object->getParentsForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("postcode"))
   {
      value = vm->createValue(object->getPostalAddress().getPostCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("primaryZoneProxy"))
   {
      UINT32 id = object->getAssignedZoneProxyId(false);
      if (id != 0)
      {
         shared_ptr<NetObj> proxy = FindObjectById(id, OBJECT_NODE);
         value = (proxy != nullptr) ? proxy->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("primaryZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(false));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("region"))
   {
      value = vm->createValue(object->getPostalAddress().getRegion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("responsibleUsers"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      unique_ptr<StructArray<ResponsibleUser>> responsibleUsers = object->getAllResponsibleUsers();
      unique_ptr<ObjectArray<UserDatabaseObject>> userDB = FindUserDBObjects(*responsibleUsers);
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         array->append(userDB->get(i)->createNXSLObject(vm));
      }
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("state"))
   {
      value = vm->createValue(object->getState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("status"))
   {
      value = vm->createValue((LONG)object->getStatus());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("streetAddress"))
   {
      value = vm->createValue(object->getPostalAddress().getStreetAddress());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue((LONG)object->getObjectClass());
   }
   else if (object != nullptr)   // Object can be null if attribute scan is running
   {
      wchar_t wattr[MAX_IDENTIFIER_LENGTH];
      utf8_to_wchar(attr.value, -1, wattr, MAX_IDENTIFIER_LENGTH);
      wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
      value = object->getCustomAttributeForNXSL(vm, wattr);
   }
   return value;
}

/**
 * NXSL class Subnet: constructor
 */
NXSL_SubnetClass::NXSL_SubnetClass() : NXSL_NetObjClass()
{
   setName(L"Subnet");
}

/**
 * NXSL class Subnet: get attribute
 */
NXSL_Value *NXSL_SubnetClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto subnet = SharedObjectFromData<Subnet>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("ipNetMask"))
   {
      value = vm->createValue(subnet->getIpAddress().getMaskBits());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSyntheticMask"))
   {
      value = vm->createValue(subnet->isSyntheticMask());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zone"))
   {
      if (g_flags & AF_ENABLE_ZONING)
      {
         shared_ptr<Zone> zone = FindZoneByUIN(subnet->getZoneUIN());
         if (zone != nullptr)
         {
            value = zone->createNXSLObject(vm);
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
   {
      value = vm->createValue(subnet->getZoneUIN());
   }
   return value;
}

/**
 * NXSL class NetworkMap: constructor
 */
NXSL_NetworkMapClass::NXSL_NetworkMapClass() : NXSL_NetObjClass()
{
   setName(_T("NetworkMap"));
}

/**
 * NXSL class NetworkMap: get attribute
 */
NXSL_Value *NXSL_NetworkMapClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto netmap = SharedObjectFromData<NetworkMap>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("backgroundColor"))
   {
      value = vm->createValue(netmap->getBackgroundColor());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backgroundImageId"))
   {
      value = vm->createValue(netmap->getBackgroundImageId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backgroundLatitude"))
   {
      value = vm->createValue(netmap->getBackgroundLatitude());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backgroundLongitude"))
   {
      value = vm->createValue(netmap->getBackgroundLatitude());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("backgroundZoom"))
   {
      value = vm->createValue(netmap->getBackgroundZoom());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("defaultLinkColor"))
   {
      value = vm->createValue(netmap->getDefaultLinkColor());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("defaultLinkRouting"))
   {
      value = vm->createValue(netmap->getDefaultLinkRouting());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("defaultLinkStyle"))
   {
      value = vm->createValue(netmap->getDefaultLinkStyle());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("defaultLinkWidth"))
   {
      value = vm->createValue(netmap->getDefaultLinkWidth());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("discoveryRadius"))
   {
      value = vm->createValue(netmap->getDiscoveryRadius());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("height"))
   {
      value = vm->createValue(netmap->getHeight());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isUpdateFailed"))
   {
      value = vm->createValue(netmap->isUpdateFailed());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("layout"))
   {
      value = vm->createValue(netmap->getLayout());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("mapType"))
   {
      value = vm->createValue(netmap->getMapType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectDisplayMode"))
   {
      value = vm->createValue(netmap->getObjectDisplayMode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("seedObjects"))
   {
      value = vm->createValue(netmap->getSeedObjectsForNXSL(vm));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("width"))
   {
      value = vm->createValue(netmap->getWidth());
   }
   return value;
}

/**
 * NXSL class "Asset" constructor
 */
NXSL_AssetClass::NXSL_AssetClass() : NXSL_NetObjClass()
{
   setName(_T("Asset"));
}

/**
 * NXSL class "Asset" attribute getter
 */
NXSL_Value *NXSL_AssetClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto asset = SharedObjectFromData<Asset>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("lastUpdateTimestamp"))
   {
      value = vm->createValue(static_cast<int64_t>(asset->getLastUpdateTimestamp()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastUpdateUserId"))
   {
      value = vm->createValue(asset->getLastUpdateUserId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("linkedObject"))
   {
      uint32_t id = asset->getLinkedObjectId();
      if (id != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(id);
         value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("linkedObjectId"))
   {
      value = vm->createValue(asset->getLinkedObjectId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("properties"))
   {
      value = vm->createValue(vm->createObject(&g_nxslAssetPropertiesClass, new shared_ptr<Asset>(asset->self())));
   }
   return value;
}

/**
 * NXSL class "AssetProperties" constructor
 */
NXSL_AssetPropertiesClass::NXSL_AssetPropertiesClass() : NXSL_Class()
{
   setName(_T("AssetProperties"));
}

/**
 * NXSL class "AssetProperties" destructor
 */
void NXSL_AssetPropertiesClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<Asset>*>(object->getData());
}

/**
 * NXSL class "AssetProperties" attribute getter
 */
NXSL_Value *NXSL_AssetPropertiesClass::getAttr(NXSL_Object *_object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(_object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = _object->vm();
   auto asset = SharedObjectFromData<Asset>(_object);
   if (asset != nullptr)   // Object can be null if attribute scan is running
   {
      wchar_t wattr[MAX_IDENTIFIER_LENGTH];
      utf8_to_wchar(attr.value, -1, wattr, MAX_IDENTIFIER_LENGTH);
      wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
      value = asset->getPropertyValueForNXSL(vm, wattr);
   }
   else if (attr.value[0] == '?')
   {
      registerAttributes(*GetAssetAttributeNames());
   }

   return value;
}

/**
 * DataCollectionTarget::applyTemplate(object)
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, applyTemplate)
{
   shared_ptr<DataCollectionTarget> thisObject = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslTemplate = argv[0]->getValueAsObject();
   if (!nxslTemplate->getClass()->instanceOf(g_nxslTemplateClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   static_cast<shared_ptr<Template>*>(nxslTemplate->getData())->get()->applyToTarget(thisObject);

   *result = vm->createValue();
   return 0;
}

/**
 * enableConfigurationPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, enableConfigurationPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_CONF_POLL, true);
}

/**
 * enableDataCollection(enabled) method
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, enableDataCollection)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_DATA_COLLECT, true);
}

/**
 * enableStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, enableStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_STATUS_POLL, true);
}

/**
 * readInternalParameter(name) method
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, readInternalParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   DataCollectionTarget *dct = static_cast<shared_ptr<DataCollectionTarget>*>(object->getData())->get();

   TCHAR value[MAX_RESULT_LENGTH];
   DataCollectionError rc = dct->getInternalMetric(argv[0]->getValueAsCString(), value, MAX_RESULT_LENGTH);
   *result = (rc == DCE_SUCCESS) ? object->vm()->createValue(value) : object->vm()->createValue();
   return 0;
}

/**
 * DataCollectionTarget::removeTemplate(object)
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, removeTemplate)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   DataCollectionTarget *thisObject = static_cast<shared_ptr<DataCollectionTarget>*>(object->getData())->get();

   NXSL_Object *nxslTemplate = argv[0]->getValueAsObject();
   if (!nxslTemplate->getClass()->instanceOf(g_nxslTemplateClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   Template *tmpl = static_cast<shared_ptr<Template>*>(nxslTemplate->getData())->get();
   NetObj::unlinkObjects(tmpl, thisObject);
   tmpl->queueRemoveFromTarget(thisObject->getId(), true);

   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class DataCollectionTarget: constructor
 */
NXSL_DCTargetClass::NXSL_DCTargetClass() : NXSL_NetObjClass()
{
   setName(_T("DataCollectionTarget"));

   NXSL_REGISTER_METHOD(DataCollectionTarget, applyTemplate, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, enableDataCollection, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, readInternalParameter, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, removeTemplate, 1);
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_DCTargetClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto dcTarget = SharedObjectFromData<DataCollectionTarget>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("templates"))
   {
      value = vm->createValue(dcTarget->getTemplatesForNXSL(vm));
   }
   return value;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_ZoneClass::NXSL_ZoneClass() : NXSL_NetObjClass()
{
   setName(_T("Zone"));
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_ZoneClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto zone = SharedObjectFromData<Zone>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("proxyNodes"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<uint32_t> proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies.size(); i++)
      {
         shared_ptr<NetObj> node = FindObjectById(proxies.get(i), OBJECT_NODE);
         if (node != nullptr)
            array->append(node->createNXSLObject(vm));
      }
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("proxyNodeIds"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<uint32_t> proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies.size(); i++)
         array->append(vm->createValue(proxies.get(i)));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("uin"))
   {
      value = vm->createValue(zone->getUIN());
   }
   return value;
}

/**
 * Node::createSNMPTransport(port, community, context, failIfUnreachable) method
 */
NXSL_METHOD_DEFINITION(Node, createSNMPTransport)
{
   if (argc > 4)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isNull() && !argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 1) && !argv[1]->isNull() && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc > 2) && !argv[2]->isNull() && !argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

   // If 4th argument is provided and is true, check that SNMP is reachable on the node before creating SNMP transport
   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   if ((argc <= 3) || argv[3]->isFalse() || (node->isSNMPSupported() && ((node->getState() & (NSF_SNMP_UNREACHABLE | DCSF_UNREACHABLE)) == 0)))
   {
      uint16_t port = ((argc > 0) && argv[0]->isInteger()) ? static_cast<uint16_t>(argv[0]->getValueAsInt32()) : 0;
      const char *community = ((argc > 1) && argv[1]->isString()) ? argv[1]->getValueAsMBString() : nullptr;
      const char *context = ((argc > 2) && argv[2]->isString()) ? argv[2]->getValueAsMBString() : nullptr;
      SNMP_Transport *t = node->createSnmpTransport(port, SNMP_VERSION_DEFAULT, context, community);
      *result = (t != nullptr) ? vm->createValue(vm->createObject(&g_nxslSnmpTransportClass, t)) : vm->createValue();
   }
   else
   {
      *result = vm->createValue();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Web service handle (combination of web service definition and node used for executing request)
 */
typedef std::pair<shared_ptr<WebServiceDefinition>, shared_ptr<Node>> WebServiceHandle;

/**
 * Web service custom request with data
 */
static int BaseWebServiceRequestWithData(WebServiceHandle *websvc, int argc, NXSL_Value **argv,
      NXSL_Value **result, NXSL_VM *vm, const HttpRequestMethod requestMethod)
{
   // Mandatory first positional parameter:
   TCHAR *data = nullptr;
   // Optional second positional parameter:
   const TCHAR *contentType = _T("application/json");
   // Optional named parameter. Can go before, after or between the above positional parameters:
   bool acceptCached = false;
   // Optional variadic parameters.
   StringList parameters;

   // Before processing the first positional parameter, let's check if it's the named parameter.
   if (argc > 0 && argv[0]->getName() && !strcmp(argv[0]->getName(), "acceptCached"))
   {
      acceptCached = argv[0]->getValueAsBoolean();
      argc--;
      argv++;
   }

   // Process first positional parameter.
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (argv[0]->isObject(_T("JsonObject")) || argv[0]->isObject(_T("JsonArray")))
   {
      json_t *json = static_cast<json_t*>(argv[0]->getValueAsObject()->getData());
      char *tmp = json_dumps(json, JSON_INDENT(3));
      data = WideStringFromUTF8String(tmp);
      MemFree(tmp);
   }
   else if (argv[0]->isString())
   {
      data = MemCopyString(argv[0]->getValueAsCString());
   }
   else
   {
      return NXSL_ERR_NOT_STRING;
   }
   argc--;
   argv++;

   // What are the possibilities after the first parameter?
   // - No more params.
   // - contentType: optional positional parameter.
   // - cacheAccepted: optional named parameter.
   // We are not considering variadic parameters yet, they can only follow
   // contentType, otherwise we can't distinguish contentType from the first
   // variadic parameter.
   if (argc > 0 && argv[0]->getName() && !strcmp(argv[0]->getName(), "acceptCached"))
   {
      acceptCached = argv[0]->getValueAsBoolean();
      argc--;
      argv++;
   }

   if (argc > 0)
   {
      if (!argv[0]->isString())
      {
         return NXSL_ERR_NOT_STRING;
      }
      else
      {
         contentType = argv[0]->getValueAsCString();
      }
      argc--;
      argv++;

      // Handle optional variadic parameters
      for (int i = 0; i < argc; i++)
         parameters.add(argv[i]->getValueAsCString());
   }

   // GET, HEAD, OPTIONS, TRACE are cacheable per 4.2.1 of RFC 7231.
   // Of those, netxms knows only GET.
   if (requestMethod != HttpRequestMethod::_GET)
   {
      acceptCached = false;
   }
   WebServiceCallResult *response = websvc->first->makeCustomRequest(websvc->second, requestMethod, parameters, data, contentType, acceptCached);
   *result = vm->createValue(vm->createObject(&g_nxslWebServiceResponseClass, response));
   MemFree(data);

   return 0;
}

/**
 * Web service custom request with data
 */
static int BaseWebServiceRequestWithoutData(WebServiceHandle *websvc, int argc, NXSL_Value **argv,
      NXSL_Value **result, NXSL_VM *vm, const HttpRequestMethod requestMethod)
{
   bool acceptCached = false;
   if (argc > 0 && argv[0]->getName() && !strcmp(argv[0]->getName(), "acceptCached"))
   {
      acceptCached = argv[0]->getValueAsBoolean();
      argc -= 1;
      argv += 1;
   }

   StringList parameters;
   for (int i = 0 ; i < argc; i++)
      parameters.add(argv[i]->getValueAsCString());

   // GET, HEAD, OPTIONS, TRACE are cacheable per 4.2.1 of RFC 7231.
   // Of those, netxms knows only GET.
   if (requestMethod != HttpRequestMethod::_GET)
   {
      acceptCached = false;
   }

   WebServiceCallResult *response = websvc->first->makeCustomRequest(websvc->second, requestMethod, parameters, nullptr, nullptr, acceptCached);
   *result = vm->createValue(vm->createObject(&g_nxslWebServiceResponseClass, response));

   return 0;
}

/**
 * Node::callWebService(webSwcName, requestMethod, [postData], parameters...) method
 */
NXSL_METHOD_DEFINITION(Node, callWebService)
{
   if (argc < 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc > 1) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<WebServiceDefinition> d = FindWebServiceDefinition(argv[0]->getValueAsCString());
   shared_ptr<Node> *node = static_cast<shared_ptr<Node>*>(object->getData());

   if (d == nullptr)
   {
      WebServiceCallResult *webSwcResult = new WebServiceCallResult();
      _tcsncpy(webSwcResult->errorMessage, _T("Web service definition not found"), WEBSVC_ERROR_TEXT_MAX_SIZE);
      *result = vm->createValue(vm->createObject(&g_nxslWebServiceResponseClass, webSwcResult));
      return 0;
   }

   WebServiceHandle websvc = WebServiceHandle(d, *node);
   const TCHAR *requestMethod = argv[1]->getValueAsCString();
   if (!_tcsicmp(_T("GET"), requestMethod))
   {
      return BaseWebServiceRequestWithoutData(&websvc, argc - 2, argv  + 2, result, vm, HttpRequestMethod::_GET);
   }
   else if (!_tcsicmp(_T("DELETE"), requestMethod))
   {
      return BaseWebServiceRequestWithoutData(&websvc, argc - 2, argv  + 2, result, vm, HttpRequestMethod::_DELETE);
   }
   else if (!_tcsicmp(_T("POST"), requestMethod))
   {
      return BaseWebServiceRequestWithData(&websvc, argc - 2, argv  + 2, result, vm, HttpRequestMethod::_POST);
   }
   else if (!_tcsicmp(_T("PUT"), requestMethod))
   {
      return BaseWebServiceRequestWithData(&websvc, argc - 2, argv  + 2, result, vm, HttpRequestMethod::_PUT);
   }
   else if (!_tcsicmp(_T("PATCH"), requestMethod))
   {
      return BaseWebServiceRequestWithData(&websvc, argc - 2, argv  + 2, result, vm, HttpRequestMethod::_PATCH);
   }

   WebServiceCallResult *webSwcResult = new WebServiceCallResult();
   _tcslcpy(webSwcResult->errorMessage, _T("Invalid web service request method"), WEBSVC_ERROR_TEXT_MAX_SIZE);
   *result = vm->createValue(vm->createObject(&g_nxslWebServiceResponseClass, webSwcResult));
   return 0;
}

/**
 * enable8021x(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enable8021xStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_8021X_STATUS_POLL, true);
}

/**
 * enableAgent(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableAgent)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_NXCP, true);
}

/**
 * enableDiscoveryPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableDiscoveryPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_DISCOVERY_POLL, true);
}

/**
 * enableEtherNetIP(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableEtherNetIP)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ETHERNET_IP, true);
}

/**
 * enableIcmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableIcmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ICMP, true);
}

/**
 * enableModbusTcp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableModbusTcp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_MODBUS_TCP, true);
}

/**
 * enablePrimaryIPPing(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enablePrimaryIPPing)
{
   return ChangeFlagMethod(object, argv[0], result, NF_PING_PRIMARY_IP, false);
}

/**
 * enableRoutingTablePolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableRoutingTablePolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ROUTE_POLL, true);
}

/**
 * enableSmclpPropertyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSmclpPropertyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SMCLP_PROPERTIES, true);
}

/**
 * enableSnmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSnmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SNMP, true);
}

/**
 * enableSsh(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSsh)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SSH, true);
}

/**
 * enableTopologyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableTopologyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_TOPOLOGY_POLL, true);
}

/**
 * enableVnc(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableVnc)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_VNC, true);
}

/**
 * enableWinPerfCountersCache(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableWinPerfCountersCache)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_PERF_COUNT, true);
}

/**
 * Node::executeAgentCommand(command, ...) method
 */
NXSL_METHOD_DEFINITION(Node, executeAgentCommand)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   for(int i = 0; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
   if (conn != nullptr)
   {
      StringList list;
      for(int i = 1; (i < argc) && (i < 128); i++)
         list.add(argv[i]->getValueAsCString());
      uint32_t rcc = conn->executeCommand(argv[0]->getValueAsCString(), list);
      *result = vm->createValue(rcc == ERR_SUCCESS);
      nxlog_debug_tag(_T("nxsl.agent"), 5, _T("NXSL: Node::executeAgentCommand: command \"%s\" on node %s [%u]: RCC=%u"), argv[0]->getValueAsCString(), node->getName(), node->getId(), rcc);
   }
   else
   {
      *result = vm->createValue(false);
   }
   return 0;
}

/**
 * Node::executeAgentCommandWithOutput(command, ...) method
 */
NXSL_METHOD_DEFINITION(Node, executeAgentCommandWithOutput)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   for(int i = 0; i < argc; i++)
      if (!argv[i]->isString())
         return NXSL_ERR_NOT_STRING;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
   if (conn != nullptr)
   {
      StringList list;
      for(int i = 1; (i < argc) && (i < 128); i++)
         list.add(argv[i]->getValueAsCString());
      StringBuffer output;
      uint32_t rcc = conn->executeCommand(argv[0]->getValueAsCString(), list, true, [](ActionCallbackEvent event, const TCHAR *text, void *context) {
         if (event == ACE_DATA)
            static_cast<StringBuffer*>(context)->append(text);
      }, &output);
      *result = (rcc == ERR_SUCCESS) ? vm->createValue(output) : vm->createValue();
      nxlog_debug_tag(_T("nxsl.agent"), 5, _T("NXSL: Node::executeAgentCommandWithOutput: command \"%s\" on node %s [%u]: RCC=%u"), argv[0]->getValueAsCString(), node->getName(), node->getId(), rcc);
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Node::executeSSHCommand(command) method
 */
NXSL_METHOD_DEFINITION(Node, executeSSHCommand)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Node *node = static_cast<shared_ptr<Node>*>(object->getData())->get();
   uint32_t proxyId = node->getEffectiveSshProxy();
   if (proxyId != 0)
   {
      shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxyNode != nullptr)
      {
         TCHAR ipAddr[64];
         StringBuffer request(_T("SSH.Command("));
         request.append(node->getIpAddress().toString(ipAddr));
         request.append(_T(':'));
         request.append(node->getSshPort());
         request.append(_T(",\""));
         request.append(EscapeStringForAgent(node->getSshLogin()).cstr());
         request.append(_T("\",\""));
         request.append(EscapeStringForAgent(node->getSshPassword()).cstr());
         request.append(_T("\",\""));
         request.append(EscapeStringForAgent(argv[0]->getValueAsCString()).cstr());
         request.append(_T("\",,"));
         request.append(node->getSshKeyId());
         request.append(_T(')'));

         StringList *list;
         uint32_t rcc = proxyNode->getListFromAgent(request, &list);
         *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, *list)) : vm->createValue();
         delete list;
      }
      else
      {
         *result = vm->createValue();
      }
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Node::getInterface(interfaceId) method
 * Interface ID could be ifIndex, name, or MAC address
 */
NXSL_METHOD_DEFINITION(Node, getInterface)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<Interface> iface;
   if (argv[0]->isInteger())  // Assume interface index
   {
      iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByIndex(argv[0]->getValueAsUInt32());
   }
   else
   {
      MacAddress macAddr = MacAddress::parse(argv[0]->getValueAsCString());
      if (macAddr.isValid() && macAddr.length() >= 6)
         iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByMAC(macAddr);
      else
         iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByName(argv[0]->getValueAsCString());
   }
   *result = (iface != nullptr) ? iface->createNXSLObject(vm) : vm->createValue();
   return 0;
}

/**
 * Node::getInterfaceByIndex(ifIndex) method
 */
NXSL_METHOD_DEFINITION(Node, getInterfaceByIndex)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   shared_ptr<Interface> iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByIndex(argv[0]->getValueAsUInt32());
   *result = (iface != nullptr) ? iface->createNXSLObject(vm) : vm->createValue();
   return 0;
}

/**
 * Node::getInterfaceByMACAddress(macAddress) method
 */
NXSL_METHOD_DEFINITION(Node, getInterfaceByMACAddress)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   MacAddress macAddr = MacAddress::parse(argv[0]->getValueAsCString());
   shared_ptr<Interface> iface = macAddr.isValid() ? static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByMAC(macAddr) : shared_ptr<Interface>();
   *result = (iface != nullptr) ? iface->createNXSLObject(vm) : vm->createValue();
   return 0;
}

/**
 * Node::getInterfaceByName(name) method
 */
NXSL_METHOD_DEFINITION(Node, getInterfaceByName)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<Interface> iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByName(argv[0]->getValueAsCString());
   *result = (iface != nullptr) ? iface->createNXSLObject(vm) : vm->createValue();
   return 0;
}

/**
 * Node::getInterfaceName(ifIndex) method
 */
NXSL_METHOD_DEFINITION(Node, getInterfaceName)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   shared_ptr<Interface> iface = static_cast<shared_ptr<Node>*>(object->getData())->get()->findInterfaceByIndex(argv[0]->getValueAsUInt32());
   *result = (iface != nullptr) ? vm->createValue(iface->getName()) : vm->createValue();
   return 0;
}

/**
 * Node::getWebService(name) method
 */
NXSL_METHOD_DEFINITION(Node, getWebService)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_INTEGER;

   shared_ptr<Node> *node = static_cast<shared_ptr<Node>*>(object->getData());

   shared_ptr<WebServiceDefinition> d = FindWebServiceDefinition(argv[0]->getValueAsCString());
   if (d == nullptr)
   {
      *result = vm->createValue();
   }
   else
   {
      *result = vm->createValue(vm->createObject(&g_nxslWebServiceClass, new WebServiceHandle(d, *node)));
   }
   return 0;
}

/**
 * Node::readAgentParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getMetricFromAgent(argv[0]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readAgentList(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentList)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   StringList *list;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getListFromAgent(argv[0]->getValueAsCString(), &list);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, *list)) : vm->createValue();
   delete list;
   return 0;
}

/**
 * Node::readAgentTable(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentTable)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<Table> table;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getTableFromAgent(argv[0]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(table))) : vm->createValue();
   return 0;
}

/**
 * Node::readDriverParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readDriverParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getMetricFromDeviceDriver(argv[0]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readInternalParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readInternalParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getInternalMetric(argv[0]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readInternalTable(name) method
 */
NXSL_METHOD_DEFINITION(Node, readInternalTable)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<Table> table;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getInternalTable(argv[0]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(table))) : vm->createValue();
   return 0;
}

/**
 * Node::readWebServiceParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readWebServiceParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getMetricFromWebService(argv[0]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readWebServiceList(name) method
 */
NXSL_METHOD_DEFINITION(Node, readWebServiceList)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   StringList *list;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getListFromWebService(argv[0]->getValueAsCString(), &list);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, *list)) : vm->createValue();
   delete list;
   return 0;
}

/**
 * Node::setExpectedCapabilities(capabilities) method
 */
NXSL_METHOD_DEFINITION(Node, setExpectedCapabilities)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   uint64_t capabilities = argv[0]->getValueAsUInt64();
   capabilities &= NC_IS_NATIVE_AGENT | NC_IS_SNMP | NC_IS_ETHERNET_IP | NC_IS_MODBUS_TCP | NC_IS_SSH;
   static_cast<shared_ptr<Node>*>(object->getData())->get()->setExpectedCapabilities(capabilities);
   *result = vm->createValue();
   return 0;
}

/**
 * Node::setIfXTableUsageMode(mode) method
 */
NXSL_METHOD_DEFINITION(Node, setIfXTableUsageMode)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   int mode = argv[0]->getValueAsInt32();
   if ((mode != IFXTABLE_DISABLED) && (mode != IFXTABLE_ENABLED))
      mode = IFXTABLE_DEFAULT;

   static_cast<shared_ptr<Node>*>(object->getData())->get()->setIfXtableUsageMode(mode);
   *result = vm->createValue();
   return 0;
}

/**
 * Node::setPollCountForStatusChange(count) method
 */
NXSL_METHOD_DEFINITION(Node, setPollCountForStatusChange)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   int count = argv[0]->getValueAsInt32();
   if (count >= 0)
      static_cast<shared_ptr<Node>*>(object->getData())->get()->setRequiredPollCount(count);
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class Node: constructor
 */
NXSL_NodeClass::NXSL_NodeClass() : NXSL_DCTargetClass()
{
   setName(_T("Node"));

   NXSL_REGISTER_METHOD(Node, callWebService, -1);
   NXSL_REGISTER_METHOD(Node, createSNMPTransport, -1);
   NXSL_REGISTER_METHOD(Node, enable8021xStatusPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableAgent, 1);
   NXSL_REGISTER_METHOD(Node, enableDiscoveryPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableEtherNetIP, 1);
   NXSL_REGISTER_METHOD(Node, enableIcmp, 1);
   NXSL_REGISTER_METHOD(Node, enableModbusTcp, 1);
   NXSL_REGISTER_METHOD(Node, enablePrimaryIPPing, 1);
   NXSL_REGISTER_METHOD(Node, enableRoutingTablePolling, 1);
   NXSL_REGISTER_METHOD(Node, enableSmclpPropertyPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableSnmp, 1);
   NXSL_REGISTER_METHOD(Node, enableSsh, 1);
   NXSL_REGISTER_METHOD(Node, enableTopologyPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableVnc, 1);
   NXSL_REGISTER_METHOD(Node, enableWinPerfCountersCache, 1);
   NXSL_REGISTER_METHOD(Node, executeAgentCommand, -1);
   NXSL_REGISTER_METHOD(Node, executeAgentCommandWithOutput, -1);
   NXSL_REGISTER_METHOD(Node, executeSSHCommand, 1);
   NXSL_REGISTER_METHOD(Node, getInterface, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByIndex, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByMACAddress, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByName, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceName, 1);
   NXSL_REGISTER_METHOD(Node, getWebService, 1);
   NXSL_REGISTER_METHOD(Node, readAgentList, 1);
   NXSL_REGISTER_METHOD(Node, readAgentParameter, 1);
   NXSL_REGISTER_METHOD(Node, readAgentTable, 1);
   NXSL_REGISTER_METHOD(Node, readDriverParameter, 1);
   NXSL_REGISTER_METHOD(Node, readInternalParameter, 1);
   NXSL_REGISTER_METHOD(Node, readInternalTable, 1);
   NXSL_REGISTER_METHOD(Node, readWebServiceList, 1);
   NXSL_REGISTER_METHOD(Node, readWebServiceParameter, 1);
   NXSL_REGISTER_METHOD(Node, setExpectedCapabilities, 1);
   NXSL_REGISTER_METHOD(Node, setIfXTableUsageMode, 1);
   NXSL_REGISTER_METHOD(Node, setPollCountForStatusChange, 1);
}

/**
 * Get ICMP statistic for object
 */
static NXSL_Value *GetNodeIcmpStatistic(Node *node, IcmpStatFunction function, NXSL_VM *vm)
{
   NXSL_Value *value;
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (node->getIcmpStatistic(nullptr, function, buffer) == DCE_SUCCESS)
   {
      value = vm->createValue(buffer);
   }
   else
   {
      value = vm->createValue();
   }
   return value;
}

/**
 * NXSL class Node: get attribute
 */
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto node = SharedObjectFromData<Node>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("agentCertificateMappingData"))
   {
      value = vm->createValue(node->getAgentCertificateMappingData());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentCertificateMappingMethod"))
   {
      value = vm->createValue(static_cast<int32_t>(node->getAgentCertificateMappingMethod()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentCertificateSubject"))
   {
      value = vm->createValue(node->getAgentCertificateSubject());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentId"))
   {
      TCHAR buffer[64];
      value = vm->createValue(node->getAgentId().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getAgentProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentVersion"))
   {
      value = vm->createValue(node->getAgentVersion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bootTime"))
   {
      value = vm->createValue(static_cast<INT64>(node->getBootTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bridgeBaseAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStr(node->getBridgeId(), MAC_ADDR_LENGTH, buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("capabilities"))
   {
      value = vm->createValue(node->getCapabilities());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipDeviceType"))
   {
      value = vm->createValue(node->getCipDeviceType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipDeviceTypeAsText"))
   {
      value = vm->createValue(CIP_DeviceTypeNameFromCode(node->getCipDeviceType()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipExtendedStatus"))
   {
      value = vm->createValue((node->getCipStatus() & CIP_DEVICE_STATUS_EXTENDED_STATUS_MASK) >> 4);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipExtendedStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeExtendedDeviceStatus(node->getCipStatus()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipStatus"))
   {
      value = vm->createValue(node->getCipStatus());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeDeviceStatus(node->getCipStatus()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipState"))
   {
      value = vm->createValue(node->getCipState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipStateAsText"))
   {
      value = vm->createValue(CIP_DeviceStateTextFromCode(node->getCipState()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cipVendorCode"))
   {
      value = vm->createValue(node->getCipVendorCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("cluster"))
   {
      shared_ptr<Cluster> cluster = node->getCluster();
      value = (cluster != nullptr) ? cluster->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("components"))
   {
      shared_ptr<ComponentTree> components = node->getComponents();
      if (components != nullptr)
      {
         value = ComponentTree::getRootForNXSL(vm, components);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dependentNodes"))
   {
      unique_ptr<StructArray<DependentNode>> dependencies = GetNodeDependencies(node->getId());
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < dependencies->size(); i++)
      {
         a->append(vm->createValue(vm->createObject(&g_nxslNodeDependencyClass, new DependentNode(*dependencies->get(i)))));
      }
      value = vm->createValue(a);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("driver"))
   {
      value = vm->createValue(node->getDriverName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("downSince"))
   {
      value = vm->createValue(static_cast<INT64>(node->getDownSince()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("effectiveAgentProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getEffectiveAgentProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("effectiveIcmpProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getEffectiveIcmpProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("effectiveSnmpProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getEffectiveSnmpProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("expectedCapabilities"))
   {
      value = vm->createValue(node->getExpectedCapabilities());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
		value = vm->createValue(node->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasAgentIfXCounters"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_AGENT_IFXCOUNTERS));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasEntityMIB"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_ENTITY_MIB));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasIfXTable"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_IFXTABLE));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasUserAgent"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_USER_AGENT));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasVLANs"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_VLANS));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hardwareId"))
   {
      TCHAR buffer[HARDWARE_ID_LENGTH * 2 + 1];
      value = vm->createValue(BinToStr(node->getHardwareId().value(), HARDWARE_ID_LENGTH, buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hardwareComponents"))
   {
      value = node->getHardwareComponentsForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasServiceManager"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_SERVICE_MANAGER));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasWinPDH"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_HAS_WINPDH));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hypervisorInfo"))
   {
      value = vm->createValue(node->getHypervisorInfo());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hypervisorType"))
   {
      value = vm->createValue(node->getHypervisorType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpAverageRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::AVERAGE, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpLastRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LAST, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpMaxRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MAX, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpMinRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MIN, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpPacketLoss"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LOSS, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getIcmpProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interfaces"))
   {
      value = node->getInterfacesForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAgent"))
   {
      value = vm->createValue(node->isNativeAgent());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isBridge"))
   {
      value = vm->createValue(node->isBridge());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isCDP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_CDP));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isEtherNetIP"))
   {
      value = vm->createValue(node->isEthernetIPSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isInMaintenanceMode"))
   {
      value = vm->createValue(node->isInMaintenanceMode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLLDP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_LLDP));
   }
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLocalMgmt") || NXSL_COMPARE_ATTRIBUTE_NAME("isLocalManagement"))
	{
		value = vm->createValue(node->isLocalManagement());
	}
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isModbusTCP"))
   {
      value = vm->createValue(node->isModbusTCPSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isOSPF"))
   {
      value = vm->createValue(node->isOSPFSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isPAE") || NXSL_COMPARE_ATTRIBUTE_NAME("is802_1x"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_8021X));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isPrinter"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_PRINTER));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isProfiNet"))
   {
      value = vm->createValue(node->isProfiNetSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isRemotelyManaged") || NXSL_COMPARE_ATTRIBUTE_NAME("isExternalGateway"))
   {
      value = vm->createValue(is_bit_set(node->getFlags(), NF_EXTERNAL_GATEWAY));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isRouter"))
   {
      value = vm->createValue(node->isRouter());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSMCLP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_SMCLP));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSNMP"))
   {
      value = vm->createValue(node->isSNMPSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSSH"))
   {
      value = vm->createValue(node->isSSHSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSONMP") || NXSL_COMPARE_ATTRIBUTE_NAME("isNDP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_NDP));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSTP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_STP));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isVirtual"))
   {
      value = vm->createValue(node->isVirtual());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isVRRP"))
   {
      value = vm->createValue(is_bit_set(node->getCapabilities(), NC_IS_VRRP));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isWirelessAP"))
   {
      value = vm->createValue(node->isWirelessAccessPoint());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isWirelessController"))
   {
      value = vm->createValue(node->isWirelessController());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastAgentCommTime"))
   {
      value = vm->createValue(static_cast<int64_t>(node->getLastAgentCommTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modbusProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getModbusProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modbusProxyId"))
   {
      value = vm->createValue(node->getModbusProxy());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modbusTCPPort"))
   {
      value = vm->createValue(node->getModbusTcpPort());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modbusUnitId"))
   {
      value = vm->createValue(node->getModbusUnitId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("networkPathCheckResult"))
   {
      value = vm->createValue(vm->createObject(&g_nxslNetworkPathCheckResultClass, new NetworkPathCheckResult(node->getNetworkPathCheckResult())));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("nodeSubType"))
   {
      value = vm->createValue(node->getSubType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("nodeType"))
   {
      value = vm->createValue(static_cast<int32_t>(node->getType()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfAreas"))
   {
      value = node->isOSPFSupported() ? node->getOSPFAreasForNXSL(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfNeighbors"))
   {
      value = node->isOSPFSupported() ? node->getOSPFNeighborsForNXSL(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfRouterId"))
   {
      TCHAR buffer[16];
      value = node->isOSPFSupported() ? vm->createValue(IpToStr(node->getOSPFRouterId(), buffer)) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("physicalContainer"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getPhysicalContainerId());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("physicalContainerId"))
   {
      value = vm->createValue(node->getPhysicalContainerId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("platformName"))
   {
      value = vm->createValue(node->getPlatformName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pollCountForStatusChange"))
   {
      value = vm->createValue(node->getRequiredPollCount());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("primaryHostName"))
   {
      value = vm->createValue(node->getPrimaryHostName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("productCode"))
   {
      value = vm->createValue(node->getProductCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("productName"))
   {
      value = vm->createValue(node->getProductName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("productVersion"))
   {
      value = vm->createValue(node->getProductVersion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rack"))
   {
      shared_ptr<NetObj> rack = FindObjectById(node->getPhysicalContainerId(), OBJECT_RACK);
      if (rack != nullptr)
      {
         value = rack->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackId"))
   {
      if (FindObjectById(node->getPhysicalContainerId(), OBJECT_RACK) != nullptr)
      {
         value = vm->createValue(node->getPhysicalContainerId());
      }
      else
      {
         value = vm->createValue(0);
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackHeight"))
   {
      value = vm->createValue(node->getRackHeight());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackPosition"))
   {
      value = vm->createValue(node->getRackPosition());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("runtimeFlags"))
   {
      value = vm->createValue(node->getRuntimeFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(node->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpOID"))
   {
      value = vm->createValue(node->getSNMPObjectId().toString());   // FIXME: use object representation
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpProxy"))
   {
      shared_ptr<NetObj> object = FindObjectById(node->getSNMPProxy());
      if (object != nullptr)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpProxyId"))
   {
      value = vm->createValue(node->getSNMPProxy());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpSysContact"))
   {
      value = vm->createValue(node->getSysContact());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpSysLocation"))
   {
      value = vm->createValue(node->getSysLocation());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpSysName"))
   {
      value = vm->createValue(node->getSysName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpVersion"))
   {
      value = vm->createValue((LONG)node->getSNMPVersion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("softwarePackages"))
   {
      value = node->getSoftwarePackagesForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("sysDescription"))
   {
      value = vm->createValue(node->getSysDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("tunnel"))
   {
      shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(node->getId());
      if (tunnel != nullptr)
         value = vm->createValue(vm->createObject(&g_nxslTunnelClass, new shared_ptr<AgentTunnel>(tunnel)));
      else
         value = vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(node->getVendor());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vlans"))
   {
      shared_ptr<VlanList> vlans = node->getVlans();
      if (vlans != nullptr)
      {
         NXSL_Array *a = new NXSL_Array(vm);
         for(int i = 0; i < vlans->size(); i++)
         {
            a->append(vm->createValue(vm->createObject(&g_nxslVlanClass, new VlanInfo(vlans->get(i), node->getId()))));
         }
         value = vm->createValue(a);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessDomain"))
   {
      shared_ptr<WirelessDomain> wirelessDomain = node->getWirelessDomain();
      value = (wirelessDomain != nullptr) ? wirelessDomain->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessDomainId"))
   {
      shared_ptr<WirelessDomain> wirelessDomain = node->getWirelessDomain();
      value = (wirelessDomain != nullptr) ? vm->createValue(wirelessDomain->getId()) : vm->createValue(static_cast<uint32_t>(0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessStations"))
   {
      if (node->getCapabilities() & (NC_IS_WIFI_AP | NC_IS_WIFI_CONTROLLER))
      {
         value = node->getWirelessStationsForNXSL(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zone"))
	{
      if (IsZoningEnabled())
      {
         shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
		   if (zone != nullptr)
		   {
			   value = zone->createNXSLObject(vm);
		   }
		   else
		   {
			   value = vm->createValue();
		   }
	   }
	   else
	   {
		   value = vm->createValue();
	   }
	}
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneProxyAssignments"))
   {
      if (IsZoningEnabled())
      {
         shared_ptr<Zone> zone = FindZoneByProxyId(node->getId());
         if (zone != nullptr)
         {
            value = vm->createValue(zone->getProxyNodeAssignments(node->getId()));
         }
         else
         {
            value = vm->createValue(0);
         }
      }
      else
      {
         value = vm->createValue(0);
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneProxyStatus"))
   {
      if (IsZoningEnabled())
      {
         shared_ptr<Zone> zone = FindZoneByProxyId(node->getId());
         if (zone != nullptr)
         {
            value = vm->createValue(zone->isProxyNodeAvailable(node->getId()));
         }
         else
         {
            value = vm->createValue(0);
         }
      }
      else
      {
         value = vm->createValue(0);
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
	{
      value = vm->createValue(node->getZoneUIN());
   }
   return value;
}

/**
 * Interface::clearPeer(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, clearPeer)
{
   Interface *iface = static_cast<shared_ptr<Interface>*>(object->getData())->get();
   if (iface->getPeerInterfaceId() != 0)
   {
      ClearPeer(iface->getPeerInterfaceId());
      ClearPeer(iface->getId());
   }
   return 0;
}

/**
 * Interface::enableAgentStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, enableAgentStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, IF_DISABLE_AGENT_STATUS_POLL, true);
}

/**
 * Interface::enableICMPStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, enableICMPStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, IF_DISABLE_ICMP_STATUS_POLL, true);
}

/**
 * Interface::enableSNMPStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, enableSNMPStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, IF_DISABLE_SNMP_STATUS_POLL, true);
}

/**
 * Interface::setExcludeFromTopology(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, setExcludeFromTopology)
{
   Interface *iface = static_cast<shared_ptr<Interface>*>(object->getData())->get();
   iface->setExcludeFromTopology(argv[0]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Interface::setExpectedState(state) method
 */
NXSL_METHOD_DEFINITION(Interface, setExpectedState)
{
   int state;
   if (argv[0]->isInteger())
   {
      state = argv[0]->getValueAsInt32();
   }
   else if (argv[0]->isString())
   {
      static const TCHAR *stateNames[] = { _T("UP"), _T("DOWN"), _T("IGNORE"), nullptr };
      const TCHAR *name = argv[0]->getValueAsCString();
      for(state = 0; stateNames[state] != nullptr; state++)
         if (!_tcsicmp(stateNames[state], name))
            break;
   }
   else
   {
      return NXSL_ERR_NOT_STRING;
   }

   if ((state >= 0) && (state <= 2))
      static_cast<shared_ptr<Interface>*>(object->getData())->get()->setExpectedState(state);

   *result = vm->createValue();
   return 0;
}

/**
 * Interface::setIncludeInIcmpPoll(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, setIncludeInIcmpPoll)
{
   Interface *iface = static_cast<shared_ptr<Interface>*>(object->getData())->get();
   iface->setIncludeInIcmpPoll(argv[0]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Interface::setPollCountForStatusChange(count) method
 */
NXSL_METHOD_DEFINITION(Interface, setPollCountForStatusChange)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   int count = argv[0]->getValueAsInt32();
   if (count >= 0)
      static_cast<shared_ptr<Interface>*>(object->getData())->get()->setRequiredPollCount(count);
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class Interface: constructor
 */
NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_NetObjClass()
{
   setName(_T("Interface"));

   NXSL_REGISTER_METHOD(Interface, clearPeer, 0);
   NXSL_REGISTER_METHOD(Interface, enableAgentStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, enableICMPStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, enableSNMPStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, setExcludeFromTopology, 1);
   NXSL_REGISTER_METHOD(Interface, setExpectedState, 1);
   NXSL_REGISTER_METHOD(Interface, setIncludeInIcmpPoll, 1);
   NXSL_REGISTER_METHOD(Interface, setPollCountForStatusChange, 1);
}

/**
 * Get ICMP statistic for interface
 */
static NXSL_Value *GetInterfaceIcmpStatistic(const Interface *iface, IcmpStatFunction function, NXSL_VM *vm)
{
   NXSL_Value *value;
   auto parentNode = iface->getParentNode();
   if (parentNode != nullptr)
   {
      TCHAR target[MAX_OBJECT_NAME + 5], buffer[MAX_RESULT_LENGTH];
      _sntprintf(target, MAX_OBJECT_NAME + 4, _T("X(N:%s)"), iface->getName());
      if (parentNode->getIcmpStatistic(target, function, buffer) == DCE_SUCCESS)
      {
         value = vm->createValue(buffer);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else
   {
      value = vm->createValue();
   }
   return value;
}

/**
 * NXSL class Interface: get attribute
 */
NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto iface = SharedObjectFromData<Interface>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("adminState"))
   {
		value = vm->createValue((LONG)iface->getAdminState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bridgePortNumber"))
   {
		value = vm->createValue(iface->getBridgePortNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("chassis"))
   {
      value = vm->createValue(iface->getChassis());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(iface->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dot1xBackendAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xBackendAuthState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dot1xPaeAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xPaeAuthState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("expectedState"))
   {
		value = vm->createValue((iface->getFlags() & IF_EXPECTED_STATE_MASK) >> 28);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
		value = vm->createValue(iface->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpAverageRTT"))
   {
      value = GetInterfaceIcmpStatistic(iface, IcmpStatFunction::AVERAGE, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpLastRTT"))
   {
      value = GetInterfaceIcmpStatistic(iface, IcmpStatFunction::LAST, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpMaxRTT"))
   {
      value = GetInterfaceIcmpStatistic(iface, IcmpStatFunction::MAX, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpMinRTT"))
   {
      value = GetInterfaceIcmpStatistic(iface, IcmpStatFunction::MIN, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("icmpPacketLoss"))
   {
      value = GetInterfaceIcmpStatistic(iface, IcmpStatFunction::LOSS, vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifAlias"))
   {
      value = vm->createValue(iface->getIfAlias());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifIndex"))
   {
		value = vm->createValue(iface->getIfIndex());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifName"))
   {
      value = vm->createValue(iface->getIfName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifType"))
   {
		value = vm->createValue(iface->getIfType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("inboundUtilization"))
   {
      uint32_t util = iface->getInboundUtilization();
      TCHAR buffer[32];
      _sntprintf(buffer, 32, _T("%d.%d"), util / 10,util % 10);
      value = vm->createValue(buffer);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddressList"))
   {
      const InetAddressList *addrList = iface->getIpAddressList();
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < addrList->size(); i++)
      {
         a->append(NXSL_InetAddressClass::createObject(vm, addrList->get(i)));
      }
      value = vm->createValue(a);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isExcludedFromTopology"))
   {
      value = vm->createValue(iface->isExcludedFromTopology());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isIncludedInIcmpPoll"))
   {
      value = vm->createValue(iface->isIncludedInIcmpPoll());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLoopback"))
   {
		value = vm->createValue(iface->isLoopback());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isManuallyCreated"))
   {
		value = vm->createValue(iface->isManuallyCreated());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isOSPF"))
   {
      value = vm->createValue(iface->isOSPF());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isPhysicalPort"))
   {
		value = vm->createValue(iface->isPhysicalPort());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddress") || NXSL_COMPARE_ATTRIBUTE_NAME("macAddr"))
   {
		TCHAR buffer[256];
		value = vm->createValue(iface->getMacAddress().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("module"))
   {
      value = vm->createValue(iface->getModule());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("mtu"))
   {
      value = vm->createValue(iface->getMTU());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("node"))
	{
      shared_ptr<Node> parentNode = iface->getParentNode();
		if (parentNode != nullptr)
		{
         value = parentNode->createNXSLObject(vm);
		}
		else
		{
			value = vm->createValue();
		}
	}
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("operState"))
   {
		value = vm->createValue((LONG)iface->getOperState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfAreaId"))
   {
      TCHAR buffer[16];
      value = vm->createValue(IpToStr(iface->getOSPFArea(), buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfState"))
   {
      value = vm->createValue(static_cast<int32_t>(iface->getOSPFState()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfStateText"))
   {
      value = vm->createValue(OSPFInterfaceStateToText(iface->getOSPFState()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfType"))
   {
      value = vm->createValue(static_cast<int32_t>(iface->getOSPFType()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ospfTypeText"))
   {
      value = vm->createValue(OSPFInterfaceTypeToText(iface->getOSPFType()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("outboundUtilization"))
   {
      uint32_t util = iface->getOutboundUtilization();
      TCHAR buffer[32];
      _sntprintf(buffer, 32, _T("%d.%d"), util / 10,util % 10);
      value = vm->createValue(buffer);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("peerInterface"))
   {
      shared_ptr<NetObj> peerIface = FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
		if (peerIface != nullptr)
		{
			if (g_flags & AF_CHECK_TRUSTED_OBJECTS)
			{
			   shared_ptr<Node> parentNode = iface->getParentNode();
				shared_ptr<Node> peerNode = static_cast<Interface*>(peerIface.get())->getParentNode();
				if ((parentNode != nullptr) && (peerNode != nullptr))
				{
					if (peerNode->isTrustedObject(parentNode->getId()))
					{
						value = peerIface->createNXSLObject(vm);
					}
					else
					{
						// No access, return null
						value = vm->createValue();
						nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::Interface::peerInterface(%s [%u]): access denied for node %s [%u]"),
									 iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
					}
				}
				else
				{
					value = vm->createValue();
					nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::Interface::peerInterface(%s [%u]): parentNode=% [%u] peerNode=%s [%u]"),
                     iface->getName(), iface->getId(), parentNode->getName(), parentNode->getId(), peerNode->getName(), peerNode->getId());
				}
			}
			else
			{
				value = peerIface->createNXSLObject(vm);
			}
		}
		else
		{
			value = vm->createValue();
		}
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("peerLastUpdateTime"))
   {
      value = vm->createValue(static_cast<int64_t>(iface->getPeerLastUpdateTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("peerNode"))
   {
      shared_ptr<NetObj> peerNode = FindObjectById(iface->getPeerNodeId());
		if (peerNode != nullptr)
		{
			if (g_flags & AF_CHECK_TRUSTED_OBJECTS)
			{
			   shared_ptr<Node> parentNode = iface->getParentNode();
				if ((parentNode != nullptr) && (peerNode->isTrustedObject(parentNode->getId())))
				{
					value = peerNode->createNXSLObject(vm);
				}
				else
				{
					// No access, return null
					value = vm->createValue();
					nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
					          iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
				}
			}
			else
			{
				value = peerNode->createNXSLObject(vm);
			}
		}
		else
		{
			value = vm->createValue();
		}
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pic"))
   {
      value = vm->createValue(iface->getPIC());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pollCountForStatusChange"))
   {
      value = vm->createValue(iface->getRequiredPollCount());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("port"))
   {
      value = vm->createValue(iface->getPort());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("speed"))
   {
      value = vm->createValue(iface->getSpeed());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("stpState"))
   {
      value = vm->createValue(static_cast<int32_t>(iface->getSTPPortState()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("stpStateText"))
   {
      value = vm->createValue(STPPortStateToText(iface->getSTPPortState()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vlans"))
   {
      value = iface->getVlanListForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zone"))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         shared_ptr<Zone> zone = FindZoneByUIN(iface->getZoneUIN());
		   if (zone != nullptr)
		   {
			   value = zone->createNXSLObject(vm);
		   }
		   else
		   {
			   value = vm->createValue();
		   }
	   }
	   else
	   {
		   value = vm->createValue();
	   }
	}
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
	{
      value = vm->createValue(iface->getZoneUIN());
   }
   return value;
}

/**
 * NXSL class AccessPoint: constructor
 */
NXSL_AccessPointClass::NXSL_AccessPointClass() : NXSL_DCTargetClass()
{
   setName(_T("AccessPoint"));
}

/**
 * NXSL class AccessPoint: get attribute
 */
NXSL_Value* NXSL_AccessPointClass::getAttr(NXSL_Object* object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto ap = SharedObjectFromData<AccessPoint>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("apState"))
   {
      value = vm->createValue(ap->getApState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("controller"))
   {
      shared_ptr<Node> controller = ap->getController();
      value = (controller != nullptr) ? controller->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("controllerId"))
   {
      value = vm->createValue(ap->getControllerId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("downSince"))
   {
      value = vm->createValue(static_cast<INT64>(ap->getDownSince()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("index"))
   {
      value = vm->createValue(ap->getIndex());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(ap->getMacAddress().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("model"))
   {
      value = vm->createValue(ap->getModel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("peerInterface"))
   {
      shared_ptr<NetObj> peerIface = FindObjectById(ap->getPeerInterfaceId(), OBJECT_INTERFACE);
      if (peerIface != nullptr)
      {
         if (g_flags & AF_CHECK_TRUSTED_OBJECTS)
         {
            shared_ptr<Node> parentNode = ap->getController();
            shared_ptr<Node> peerNode = static_cast<Interface*>(peerIface.get())->getParentNode();
            if ((parentNode != nullptr) && (peerNode != nullptr))
            {
               if (peerNode->isTrustedObject(parentNode->getId()))
               {
                  value = peerIface->createNXSLObject(vm);
               }
               else
               {
                  // No access, return null
                  value = vm->createValue();
                  nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::AccessPoint::peerInterface(%s [%u]): access denied for node %s [%u]"),
                            ap->getName(), ap->getId(), peerNode->getName(), peerNode->getId());
               }
            }
            else
            {
               value = vm->createValue();
               nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::AccessPoint::peerInterface(%s [%u]): parentNode=% [%u] peerNode=%s [%u]"),
                     ap->getName(), ap->getId(), parentNode->getName(), parentNode->getId(), peerNode->getName(), peerNode->getId());
            }
         }
         else
         {
            value = peerIface->createNXSLObject(vm);
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("peerNode"))
   {
      shared_ptr<NetObj> peerNode = FindObjectById(ap->getPeerNodeId(), OBJECT_NODE);
      if (peerNode != nullptr)
      {
         if (g_flags & AF_CHECK_TRUSTED_OBJECTS)
         {
            shared_ptr<Node> parentNode = ap->getController();
            if ((parentNode != nullptr) && (peerNode->isTrustedObject(parentNode->getId())))
            {
               value = peerNode->createNXSLObject(vm);
            }
            else
            {
               // No access, return null
               value = vm->createValue();
               nxlog_debug_tag(_T("nxsl.objects"), 4, _T("NXSL::AccessPoint::peerNode(%s [%u]): access denied for node %s [%u]"),
                         ap->getName(), ap->getId(), peerNode->getName(), peerNode->getId());
            }
         }
         else
         {
            value = peerNode->createNXSLObject(vm);
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("radioInterfaces"))
   {
      value = ap->getRadioInterfacesForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(ap->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(ap->getVendor());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessDomain"))
   {
      shared_ptr<WirelessDomain> wirelessDomain = ap->getWirelessDomain();
      value = (wirelessDomain != nullptr) ? wirelessDomain->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessDomainId"))
   {
      value = vm->createValue(ap->getWirelessDomainId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("wirelessStations"))
   {
      value = ap->getWirelessStationsForNXSL(vm);
   }
   return value;
}

/**
 * NXSL class RadioInterface: constructor
 */
NXSL_RadioInterfaceClass::NXSL_RadioInterfaceClass()
{
   setName(_T("RadioInterface"));
}

/**
 * NXSL class RadioInterface: get attribute
 */
NXSL_Value *NXSL_RadioInterfaceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto rif = static_cast<RadioInterfaceInfo*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("band"))
   {
      value = vm->createValue(rif->band);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bandName"))
   {
      value = vm->createValue(RadioBandDisplayName(rif->band));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bssid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStrEx(rif->bssid, MAC_ADDR_LENGTH, buffer, ':', 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("channel"))
   {
      value = vm->createValue(rif->channel);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("frequency"))
   {
      value = vm->createValue(rif->frequency);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifIndex"))
   {
      value = vm->createValue(rif->ifIndex);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("index"))
   {
      value = vm->createValue(rif->index);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(rif->name);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("powerDBm"))
   {
      value = vm->createValue(rif->powerDBm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("powerMW"))
   {
      value = vm->createValue(rif->powerMW);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ssid"))
   {
      value = vm->createValue(rif->ssid);
   }
   return value;
}

/**
 * NXSL class RadioInterface: destroy object
 */
void NXSL_RadioInterfaceClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<RadioInterfaceInfo*>(object->getData());
}

/**
 * NXSL class WirelessStation: constructor
 */
NXSL_WirelessStationClass::NXSL_WirelessStationClass()
{
   setName(_T("WirelessStation"));
}

/**
 * NXSL class WirelessStation: get attribute
 */
NXSL_Value *NXSL_WirelessStationClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto ws = static_cast<WirelessStationInfo*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("accessPointId"))
   {
      value = vm->createValue(ws->apObjectId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("bssid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStrEx(ws->bssid, MAC_ADDR_LENGTH, buffer, ':', 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddress"))
   {
      value = NXSL_InetAddressClass::createObject(vm, ws->ipAddr);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStrEx(ws->macAddr, MAC_ADDR_LENGTH, buffer, ':', 0));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rfIndex"))
   {
      value = vm->createValue(ws->rfIndex);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rfName"))
   {
      value = vm->createValue(ws->rfName);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rssi"))
   {
      value = vm->createValue(ws->rssi);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rxRate"))
   {
      value = vm->createValue(ws->rxRate);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ssid"))
   {
      value = vm->createValue(ws->ssid);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("txRate"))
   {
      value = vm->createValue(ws->txRate);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vlan"))
   {
      value = vm->createValue(ws->vlan);
   }
   return value;
}

/**
 * NXSL class WirelessStation: destroy object
 */
void NXSL_WirelessStationClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<WirelessStationInfo*>(object->getData());
}

/**
 * NXSL class "WirelessDomain" constructor
 */
NXSL_WirelessDomainClass::NXSL_WirelessDomainClass() : NXSL_NetObjClass()
{
   setName(_T("WirelessDomain"));
}

/**
 * NXSL class "Container" attributes
 */
NXSL_Value *NXSL_WirelessDomainClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto wirelessDomain = SharedObjectFromData<WirelessDomain>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("apCountDown"))
   {
      value = vm->createValue(wirelessDomain->getApCountDown());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("apCountUnknown"))
   {
      value = vm->createValue(wirelessDomain->getApCountUnknown());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("apCountUnprovisioned"))
   {
      value = vm->createValue(wirelessDomain->getApCountUnprovisioned());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("apCountUp"))
   {
      value = vm->createValue(wirelessDomain->getApCountUp());
   }
   return value;
}

/**
 * NXSL class Mobile Device: constructor
 */
NXSL_MobileDeviceClass::NXSL_MobileDeviceClass() : NXSL_DCTargetClass()
{
   setName(_T("MobileDevice"));
}

/**
 * NXSL class Mobile Device: get attribute
 */
NXSL_Value *NXSL_MobileDeviceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto mobileDevice = SharedObjectFromData<MobileDevice>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("altitude"))
   {
		value = vm->createValue(mobileDevice->getAltitude());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("batteryLevel"))
   {
      value = vm->createValue(mobileDevice->getBatteryLevel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("commProtocol"))
   {
      value = vm->createValue(mobileDevice->getCommProtocol());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("deviceId"))
   {
      value = vm->createValue(mobileDevice->getDeviceId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("direction"))
   {
      value = vm->createValue(mobileDevice->getDirection());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastReportTime"))
   {
      value = vm->createValue(static_cast<int64_t>(mobileDevice->getLastReportTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("model"))
   {
      value = vm->createValue(mobileDevice->getModel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("osName"))
   {
      value = vm->createValue(mobileDevice->getOsName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("osVersion"))
   {
      value = vm->createValue(mobileDevice->getOsVersion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(mobileDevice->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("speed"))
   {
      value = vm->createValue(mobileDevice->getSpeed());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userId"))
   {
      value = vm->createValue(mobileDevice->getUserId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(mobileDevice->getVendor());
   }

   return value;
}

/**
 * NXSL class "Chassis" constructor
 */
NXSL_ChassisClass::NXSL_ChassisClass() : NXSL_DCTargetClass()
{
   setName(_T("Chassis"));
}

/**
 * NXSL class "Chassis" attributes
 */
NXSL_Value *NXSL_ChassisClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto chassis = SharedObjectFromData<Chassis>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("controller"))
   {
      shared_ptr<NetObj> node = FindObjectById(chassis->getControllerId(), OBJECT_NODE);
      if (node != nullptr)
      {
         value = node->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("controllerId"))
   {
      value = vm->createValue(chassis->getControllerId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
      value = vm->createValue(chassis->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rack"))
   {
      shared_ptr<NetObj> rack = FindObjectById(chassis->getRackId(), OBJECT_RACK);
      if (rack != nullptr)
      {
         value = rack->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackId"))
   {
      value = vm->createValue(chassis->getRackId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackHeight"))
   {
      value = vm->createValue(chassis->getRackHeight());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rackPosition"))
   {
      value = vm->createValue(chassis->getRackPosition());
   }
   return value;
}

/**
 * Cluster::getResourceOwner() method
 */
NXSL_METHOD_DEFINITION(Cluster, getResourceOwner)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT32 ownerId = static_cast<shared_ptr<Cluster>*>(object->getData())->get()->getResourceOwner(argv[0]->getValueAsCString());
   if (ownerId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(ownerId);
      *result = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Cluster::add(object)
 */
NXSL_METHOD_DEFINITION(Cluster, add)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> child = *static_cast<shared_ptr<NetObj>*>(nxslChild->getData());
   if (child->getObjectClass() != OBJECT_NODE)
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<Cluster> thisObject = *static_cast<shared_ptr<Cluster>*>(object->getData());

   *result = vm->createValue(thisObject->addNode(static_pointer_cast<Node>(child)));
   return 0;
}

/**
 * Cluster::removeFrom(object)
 */
NXSL_METHOD_DEFINITION(Cluster, remove)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<NetObj> child = *static_cast<shared_ptr<NetObj>*>(nxslChild->getData());
   if (child->getObjectClass() != OBJECT_NODE)
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<Cluster> thisObject = *static_cast<shared_ptr<Cluster>*>(object->getData());
   NetObj::unlinkObjects(thisObject.get(), child.get());
   thisObject->removeNode(static_pointer_cast<Node>(child));

   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Cluster" constructor
 */
NXSL_ClusterClass::NXSL_ClusterClass() : NXSL_DCTargetClass()
{
   setName(_T("Cluster"));

   NXSL_REGISTER_METHOD(Cluster, getResourceOwner, 1);
   NXSL_REGISTER_METHOD(Cluster, add, 1);
   NXSL_REGISTER_METHOD(Cluster, remove, 1);
}

/**
 * NXSL class "Cluster" attributes
 */
NXSL_Value *NXSL_ClusterClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto cluster = SharedObjectFromData<Cluster>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("nodes"))
   {
      value = cluster->getNodesForNXSL(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zone"))
   {
      if (g_flags & AF_ENABLE_ZONING)
      {
         shared_ptr<Zone> zone = FindZoneByUIN(cluster->getZoneUIN());
         if (zone != nullptr)
         {
            value = zone->createNXSLObject(vm);
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
   {
      value = vm->createValue(cluster->getZoneUIN());
   }
   return value;
}

/**
 * Create container object - common method implementation
 */
static int CreateContainerImpl(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());
   shared_ptr<Container> container = make_shared<Container>(argv[0]->getValueAsCString());
   NetObjInsert(container, true, false);
   NetObj::linkObjects(thisObject, container);
   container->unhide();

   *result = container->createNXSLObject(vm);
   return NXSL_ERR_SUCCESS;
}

/**
 * Create collector object - common method implementation
 */
static int CreateCollectorImpl(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());
   shared_ptr<Collector> collector = make_shared<Collector>(argv[0]->getValueAsCString());
   NetObjInsert(collector, true, false);
   NetObj::linkObjects(thisObject, collector);
   collector->unhide();

   *result = collector->createNXSLObject(vm);
   return NXSL_ERR_SUCCESS;
}

/**
 * Create node object - common method implementation
 */
static int CreateNodeImpl(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());

   if (!argv[0]->isString() || ((argc > 1) && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   if ((argc > 2) && !argv[2]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   const TCHAR *pname;
   if (argc > 1)
   {
      pname = argv[1]->getValueAsCString();
      if (*pname == 0)
         pname = argv[0]->getValueAsCString();
   }
   else
   {
      pname = argv[0]->getValueAsCString();
   }
   NewNodeData newNodeData(InetAddress::resolveHostName(pname));
   _tcslcpy(newNodeData.name, argv[0]->getValueAsCString(), MAX_OBJECT_NAME);
   newNodeData.zoneUIN = (argc > 2) ? argv[2]->getValueAsUInt32() : 0;
   newNodeData.doConfPoll = true;

   shared_ptr<Node> node = PollNewNode(&newNodeData);
   if (node != nullptr)
   {
      node->setPrimaryHostName(pname);
      NetObj::linkObjects(thisObject, node);
      node->unhide();
      *result = node->createNXSLObject(vm);
   }
   else
   {
      *result = vm->createValue();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * Create sensor object - common method implementation
 * Arguments: name, [deviceClass], [gateway], [modbusUnitId]
 */
static int CreateSensorImpl(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc > 1) && !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 2) && !argv[2]->isNull() && !argv[2]->isObject(L"Node"))
      return NXSL_ERR_NOT_OBJECT;

   if ((argc > 3) && !argv[3]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   uint32_t gatewayId = ((argc > 2) && !argv[2]->isNull()) ? static_cast<shared_ptr<NetObj>*>(argv[2]->getValueAsObject()->getData())->get()->getId() : 0;
   SensorDeviceClass deviceClass = (argc > 1) ? SensorDeviceClassFromInt(argv[1]->getValueAsInt32()) : SENSOR_OTHER;

   shared_ptr<Sensor> sensor = make_shared<Sensor>(argv[0]->getValueAsCString(), deviceClass, gatewayId, static_cast<uint16_t>((argc > 3) ? argv[3]->getValueAsUInt32() : 255));
   NetObjInsert(sensor, true, false);
   NetObj::linkObjects(thisObject, sensor);
   sensor->unhide();
   *result = sensor->createNXSLObject(vm);
   return NXSL_ERR_SUCCESS;
}

/**
 * Container::createCollector() method
 */
NXSL_METHOD_DEFINITION(Container, createCollector)
{
   return CreateCollectorImpl(object, argc, argv, result, vm);
}

/**
 * Container::createContainer() method
 */
NXSL_METHOD_DEFINITION(Container, createContainer)
{
   return CreateContainerImpl(object, argc, argv, result, vm);
}

/**
 * Container::createNode() method
 */
NXSL_METHOD_DEFINITION(Container, createNode)
{
   return CreateNodeImpl(object, argc, argv, result, vm);
}

/**
 * Container::createSensor() method
 */
NXSL_METHOD_DEFINITION(Container, createSensor)
{
   return CreateSensorImpl(object, argc, argv, result, vm);
}

/**
 * Container::setAutoBindMode() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindMode)
{
   static_cast<shared_ptr<Container>*>(object->getData())->get()->setAutoBindMode(0, argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Container::setAutoBindScript() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindScript)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<Container>*>(object->getData())->get()->setAutoBindFilter(0, argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Container" constructor
 */
NXSL_ContainerClass::NXSL_ContainerClass() : NXSL_NetObjClass()
{
   setName(_T("Container"));

   NXSL_REGISTER_METHOD(Container, createCollector, 1);
   NXSL_REGISTER_METHOD(Container, createContainer, 1);
   NXSL_REGISTER_METHOD(Container, createNode, -1);
   NXSL_REGISTER_METHOD(Container, createSensor, -1);
   NXSL_REGISTER_METHOD(Container, setAutoBindMode, 2);
   NXSL_REGISTER_METHOD(Container, setAutoBindScript, 1);
}

/**
 * NXSL class "Container" attributes
 */
NXSL_Value *NXSL_ContainerClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto container = SharedObjectFromData<Container>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("autoBindScript"))
   {
      const TCHAR *script = container->getAutoBindFilterSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoBindEnabled"))
   {
      value = vm->createValue(container->isAutoBindEnabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoUnbindEnabled"))
   {
      value = vm->createValue(container->isAutoUnbindEnabled());
   }
   return value;
}

/**
 * Collector::createCollector() method
 */
NXSL_METHOD_DEFINITION(Collector, createCollector)
{
   return CreateCollectorImpl(object, argc, argv, result, vm);
}

/**
 * Collector::createContainer() method
 */
NXSL_METHOD_DEFINITION(Collector, createContainer)
{
   return CreateContainerImpl(object, argc, argv, result, vm);
}

/**
 * Collector::createNode() method
 */
NXSL_METHOD_DEFINITION(Collector, createNode)
{
   return CreateNodeImpl(object, argc, argv, result, vm);
}

/**
 * Collector::createSensor() method
 */
NXSL_METHOD_DEFINITION(Collector, createSensor)
{
   return CreateSensorImpl(object, argc, argv, result, vm);
}

/**
 * Collector::setAutoBindMode() method
 */
NXSL_METHOD_DEFINITION(Collector, setAutoBindMode)
{
   static_cast<shared_ptr<Collector>*>(object->getData())->get()->setAutoBindMode(0, argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Collector::setAutoBindScript() method
 */
NXSL_METHOD_DEFINITION(Collector, setAutoBindScript)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<Collector>*>(object->getData())->get()->setAutoBindFilter(0, argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Collector" constructor
 */
NXSL_CollectorClass::NXSL_CollectorClass() : NXSL_DCTargetClass()
{
   setName(_T("Collector"));

   NXSL_REGISTER_METHOD(Collector, createCollector, 1);
   NXSL_REGISTER_METHOD(Collector, createContainer, 1);
   NXSL_REGISTER_METHOD(Collector, createNode, -1);
   NXSL_REGISTER_METHOD(Collector, createSensor, -1);
   NXSL_REGISTER_METHOD(Collector, setAutoBindMode, 2);
   NXSL_REGISTER_METHOD(Collector, setAutoBindScript, 1);
}

/**
 * NXSL class "Collector" attributes
 */
NXSL_Value *NXSL_CollectorClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto collector = SharedObjectFromData<Collector>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("autoBindScript"))
   {
      const TCHAR *script = collector->getAutoBindFilterSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoBindEnabled"))
   {
      value = vm->createValue(collector->isAutoBindEnabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoUnbindEnabled"))
   {
      value = vm->createValue(collector->isAutoUnbindEnabled());
   }
   return value;
}

/**
 * NXSL class "Circuit" constructor
 */
NXSL_CircuitClass::NXSL_CircuitClass() : NXSL_DCTargetClass()
{
   setName(_T("Circuit"));
}

/**
 * NXSL class "Circuit" attributes
 */
NXSL_Value *NXSL_CircuitClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto circuit = SharedObjectFromData<Circuit>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("autoBindScript"))
   {
      const TCHAR *script = circuit->getAutoBindFilterSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interfaces"))
   {
      value = vm->createValue(circuit->getInterfacesForNXSL(vm));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoBindEnabled"))
   {
      value = vm->createValue(circuit->isAutoBindEnabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoUnbindEnabled"))
   {
      value = vm->createValue(circuit->isAutoUnbindEnabled());
   }
   return value;
}

/**
 * ServiceRoot::createCollector() method
 */
NXSL_METHOD_DEFINITION(ServiceRoot, createCollector)
{
   return CreateCollectorImpl(object, argc, argv, result, vm);
}

/**
 * ServiceRoot::createContainer() method
 */
NXSL_METHOD_DEFINITION(ServiceRoot, createContainer)
{
   return CreateContainerImpl(object, argc, argv, result, vm);
}

/**
 * ServiceRoot::createNode() method
 */
NXSL_METHOD_DEFINITION(ServiceRoot, createNode)
{
   return CreateNodeImpl(object, argc, argv, result, vm);
}

/**
 * ServiceRoot::createSensor() method
 */
NXSL_METHOD_DEFINITION(ServiceRoot, createSensor)
{
   return CreateSensorImpl(object, argc, argv, result, vm);
}

/**
 * NXSL class "ServiceRoot" constructor
 */
NXSL_ServiceRootClass::NXSL_ServiceRootClass() : NXSL_NetObjClass()
{
   setName(_T("ServiceRoot"));

   NXSL_REGISTER_METHOD(ServiceRoot, createCollector, 1);
   NXSL_REGISTER_METHOD(ServiceRoot, createContainer, 1);
   NXSL_REGISTER_METHOD(ServiceRoot, createNode, -1);
   NXSL_REGISTER_METHOD(ServiceRoot, createSensor, -1);
}

/**
 * NXSL class "BusinessService" constructor
 */
NXSL_BusinessServiceClass::NXSL_BusinessServiceClass() : NXSL_NetObjClass()
{
   setName(_T("BusinessService"));
}

/**
 * NXSL class "BusinessService" attributes
 */
NXSL_Value *NXSL_BusinessServiceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto businessService = SharedObjectFromData<BusinessService>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("checks"))
   {
      unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = businessService->getChecks();
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < checks->size(); i++)
         array->append(vm->createValue(vm->createObject(&g_nxslBusinessServiceCheckClass, new shared_ptr<BusinessServiceCheck>(checks->getShared(i)))));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("instance"))
   {
      value = vm->createValue(businessService->getInstance());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("prototypeId"))
   {
      value = vm->createValue(businessService->getPrototypeId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serviceState"))
   {
      value = vm->createValue(businessService->getServiceState());
   }
   return value;
}

/**
 * NXSL class "BusinessServiceCheck" constructor
 */
NXSL_BusinessServiceCheckClass::NXSL_BusinessServiceCheckClass() : NXSL_Class()
{
   setName(_T("BusinessServiceCheck"));
}

/**
 * NXSL object destructor for class "BusinessServiceCheck"
 */
void NXSL_BusinessServiceCheckClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<BusinessServiceCheck>*>(object->getData());
}

/**
 * NXSL class "BusinessService" attributes
 */
NXSL_Value *NXSL_BusinessServiceCheckClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   BusinessServiceCheck *check = (object->getData() != nullptr) ? static_cast<shared_ptr<BusinessServiceCheck>*>(object->getData())->get() : nullptr;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("currentTicketId"))
   {
      value = vm->createValue(check->getCurrentTicket());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(check->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("failureReason"))
   {
      value = vm->createValue(check->getFailureReason());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(check->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedDCI"))
   {
      uint32_t dciId = check->getRelatedDCI();
      uint32_t objectId = check->getRelatedObject();
      if ((dciId != 0) && (objectId != 0))
      {
         shared_ptr<NetObj> object = FindObjectById(objectId);
         if ((object != nullptr) && object->isDataCollectionTarget())
         {
            shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, 0);
            value = (dci != nullptr) ? dci->createNXSLObject(vm) : vm->createValue();
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedDCIId"))
   {
      value = vm->createValue(check->getRelatedDCI());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedObject"))
   {
      uint32_t id = check->getRelatedObject();
      if (id != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(id);
         value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedObjectId"))
   {
      value = vm->createValue(check->getRelatedObject());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("state"))
   {
      value = vm->createValue(check->getStatus());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(static_cast<int32_t>(check->getType()));
   }
   return value;
}

/**
 * Template::applyTo(object)
 */
NXSL_METHOD_DEFINITION(Template, applyTo)
{
   shared_ptr<Template> thisObject = *static_cast<shared_ptr<Template>*>(object->getData());

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslTarget = argv[0]->getValueAsObject();
   if (!nxslTarget->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   thisObject->applyToTarget(*static_cast<shared_ptr<DataCollectionTarget>*>(nxslTarget->getData()));

   *result = vm->createValue();
   return 0;
}

/**
 * Template::removeFrom(object)
 */
NXSL_METHOD_DEFINITION(Template, removeFrom)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   Template *thisObject = static_cast<shared_ptr<Template>*>(object->getData())->get();

   NXSL_Object *nxslTarget = argv[0]->getValueAsObject();
   if (!nxslTarget->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   DataCollectionTarget *target = static_cast<shared_ptr<DataCollectionTarget>*>(nxslTarget->getData())->get();
   NetObj::unlinkObjects(thisObject, target);
   thisObject->queueRemoveFromTarget(target->getId(), true);

   *result = vm->createValue();
   return 0;
}

/**
 * Template::setAutoApplyMode() method
 */
NXSL_METHOD_DEFINITION(Template, setAutoApplyMode)
{
   static_cast<shared_ptr<Template>*>(object->getData())->get()->setAutoBindMode(0, argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Template::setAutoApplyScript() method
 */
NXSL_METHOD_DEFINITION(Template, setAutoApplyScript)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<Template>*>(object->getData())->get()->setAutoBindFilter(0, argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Template" constructor
 */
NXSL_TemplateClass::NXSL_TemplateClass() : NXSL_NetObjClass()
{
   setName(_T("Template"));

   NXSL_REGISTER_METHOD(Template, applyTo, 1);
   NXSL_REGISTER_METHOD(Template, removeFrom, 1);
   NXSL_REGISTER_METHOD(Template, setAutoApplyMode, 2);
   NXSL_REGISTER_METHOD(Template, setAutoApplyScript, 1);
}

/**
 * NXSL class "Template" attributes
 */
NXSL_Value *NXSL_TemplateClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto tmpl = SharedObjectFromData<Template>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("autoApplyScript"))
   {
      const TCHAR *script = tmpl->getAutoBindFilterSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoApplyEnabled"))
   {
      value = vm->createValue(tmpl->isAutoBindEnabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAutoRemoveEnabled"))
   {
      value = vm->createValue(tmpl->isAutoUnbindEnabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("version"))
   {
      value = vm->createValue(tmpl->getVersion());
   }
   return value;
}

/**
 * Tunnel::bind(node)
 */
NXSL_METHOD_DEFINITION(Tunnel, bind)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *node = argv[0]->getValueAsObject();
   if (!node->getClass()->instanceOf(L"Node"))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<AgentTunnel> tunnel = *static_cast<shared_ptr<AgentTunnel>*>(object->getData());
   uint32_t nodeId = (*static_cast<shared_ptr<Node>*>(node->getData()))->getId();
   uint32_t rcc = tunnel->bind(nodeId, 0);
   *result = vm->createValue(rcc);
   return 0;
}

/**
 * NXSL class Tunnel: constructor
 */
NXSL_TunnelClass::NXSL_TunnelClass() : NXSL_Class()
{
   setName(_T("Tunnel"));

   NXSL_REGISTER_METHOD(Tunnel, bind, 1);
}

/**
 * NXSL object destructor
 */
void NXSL_TunnelClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<AgentTunnel>*>(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_TunnelClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   AgentTunnel *tunnel = (object->getData() != nullptr) ? static_cast<shared_ptr<AgentTunnel>*>(object->getData())->get() : nullptr;

   if (NXSL_COMPARE_ATTRIBUTE_NAME("address"))
   {
      value = NXSL_InetAddressClass::createObject(vm, tunnel->getAddress());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentBuildTag"))
   {
      value = vm->createValue(tunnel->getAgentBuildTag());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentId"))
   {
      value = vm->createValue(tunnel->getAgentId().toString());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("agentVersion"))
   {
      value = vm->createValue(tunnel->getAgentVersion());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("certificateExpirationTime"))
   {
      value = vm->createValue(static_cast<int64_t>(tunnel->getCertificateExpirationTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("guid"))
   {
      value = vm->createValue(tunnel->getGUID().toString());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hostname"))
   {
      value = vm->createValue(tunnel->getHostname());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hardwareId"))
   {
      value = vm->createValue(tunnel->getHostname());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(tunnel->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAgentProxy"))
   {
      value = vm->createValue(tunnel->isAgentProxy());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isBound"))
   {
      value = vm->createValue(tunnel->isBound());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSnmpProxy"))
   {
      value = vm->createValue(tunnel->isSnmpProxy());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSnmpTrapProxy"))
   {
      value = vm->createValue(tunnel->isSnmpTrapProxy());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isUserAgentInstalled"))
   {
      value = vm->createValue(tunnel->isUserAgentInstalled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddresses"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(const MacAddress *m : tunnel->getMacAddressList())
         array->append(vm->createValue(m->toString()));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("platformName"))
   {
      value = vm->createValue(tunnel->getPlatformName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("startTime"))
   {
      value = vm->createValue(static_cast<int64_t>(tunnel->getStartTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(tunnel->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("systemInfo"))
   {
      value = vm->createValue(tunnel->getSystemInfo());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("systemName"))
   {
      value = vm->createValue(tunnel->getSystemName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("zoneUIN"))
   {
      value = vm->createValue(tunnel->getZoneUIN());
   }
   return value;
}

/**
 * Event::setMessage() method
 */
NXSL_METHOD_DEFINITION(Event, setMessage)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->setMessage(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::setSeverity() method
 */
NXSL_METHOD_DEFINITION(Event, setSeverity)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   int s = argv[0]->getValueAsInt32();
   if ((s >= SEVERITY_NORMAL) && (s <= SEVERITY_CRITICAL))
   {
      Event *event = static_cast<Event*>(object->getData());
      event->setSeverity(s);
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Event::addParameter() method
 */
NXSL_METHOD_DEFINITION(Event, addParameter)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || ((argc == 2) && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   if (argc == 1)
      event->addParameter(_T(""), argv[0]->getValueAsCString());
   else
      event->addParameter(argv[0]->getValueAsCString(), argv[1]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::addTag() method
 */
NXSL_METHOD_DEFINITION(Event, addTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->addTag(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::correlateTo() method
 */
NXSL_METHOD_DEFINITION(Event, correlateTo)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Event *event = static_cast<Event*>(object->getData());
   event->setRootId(argv[0]->getValueAsUInt64());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::expandString() method
 */
NXSL_METHOD_DEFINITION(Event, expandString)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   *result = vm->createValue(event->expandText(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Event::getParameter() method
 */
NXSL_METHOD_DEFINITION(Event, getParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   const TCHAR *value = argv[0]->isInteger() ? event->getParameter(argv[0]->getValueAsInt32()) : event->getNamedParameter(argv[0]->getValueAsCString());
   *result = (value != nullptr) ? vm->createValue(value) : vm->createValue();
   return 0;
}

/**
 * Event::hasTag() method
 */
NXSL_METHOD_DEFINITION(Event, hasTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   *result = vm->createValue(event->hasTag(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Event::removeTag() method
 */
NXSL_METHOD_DEFINITION(Event, removeTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->removeTag(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::setNamedParameter() method
 */
NXSL_METHOD_DEFINITION(Event, setNamedParameter)
{
   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->setNamedParameter(argv[0]->getValueAsCString(), argv[1]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::setParameter() method
 */
NXSL_METHOD_DEFINITION(Event, setParameter)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (!argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->setParameter(argv[0]->getValueAsInt32() - 1, nullptr, argv[1]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::toJson() method
 */
NXSL_METHOD_DEFINITION(Event, toJson)
{
   Event *event = static_cast<Event*>(object->getData());
   json_t *json = event->toJson();
   char *text = json_dumps(json, JSON_INDENT(3));
   *result = vm->createValue(text);
   MemFree(text);
   json_decref(json);
   return 0;
}

/**
 * NXSL class Event: constructor
 */
NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   setName(_T("Event"));

   NXSL_REGISTER_METHOD(Event, addParameter, -1);
   NXSL_REGISTER_METHOD(Event, addTag, 1);
   NXSL_REGISTER_METHOD(Event, correlateTo, 1);
   NXSL_REGISTER_METHOD(Event, expandString, 1);
   NXSL_REGISTER_METHOD(Event, getParameter, 1);
   NXSL_REGISTER_METHOD(Event, hasTag, 1);
   NXSL_REGISTER_METHOD(Event, removeTag, 1);
   NXSL_REGISTER_METHOD(Event, setMessage, 1);
   NXSL_REGISTER_METHOD(Event, setNamedParameter, 2);
   NXSL_REGISTER_METHOD(Event, setParameter, 2);
   NXSL_REGISTER_METHOD(Event, setSeverity, 1);
   NXSL_REGISTER_METHOD(Event, toJson, 0);
}

/**
 * Destructor
 */
void NXSL_EventClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<Event*>(object->getData());
}

/**
 * NXSL class Event: get attribute
 */
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   const Event *event = static_cast<Event*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("code"))
   {
      value = vm->createValue(event->getCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("customMessage"))
   {
      value = vm->createValue(event->getCustomMessage());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dci"))
   {
      uint32_t dciId = event->getDciId();
      if (dciId != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
         if ((object != nullptr) && object->isDataCollectionTarget())
         {
            shared_ptr<DCObject> dci = static_cast<DataCollectionTarget*>(object.get())->getDCObjectById(dciId, 0, true);
            if (dci != nullptr)
            {
               value = dci->createNXSLObject(vm);
            }
            else
            {
               value = vm->createValue();
            }
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dciId"))
   {
      value = vm->createValue(event->getDciId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(event->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastAlarmKey"))
   {
      value = vm->createValue(event->getLastAlarmKey());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("message"))
   {
      value = vm->createValue(event->getMessage());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
		value = vm->createValue(event->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("origin"))
   {
      value = vm->createValue(static_cast<int32_t>(event->getOrigin()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("originTimestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getOriginTimestamp()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("parameters"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameter(i)));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("parameterNames"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameterName(i)));
      value = vm->createValue(array);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rootId"))
   {
      value = vm->createValue(event->getRootId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("severity"))
   {
      value = vm->createValue(event->getSeverity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("source"))
   {
      shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("sourceId"))
   {
      value = vm->createValue(event->getSourceId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("tagList"))
   {
      value = vm->createValue(event->getTagsAsList());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("tags"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      String::split(event->getTagsAsList(), _T(","), false,
         [a, vm] (const String& s)
         {
            a->append(vm->createValue(s));
         });
      value = vm->createValue(a);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("timestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getTimestamp()));
   }
   else if (event != nullptr)    // Event can be null if getAttr called by attribute scan
   {
      if (attr.value[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr.value[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *s = event->getParameter(index - 1);
            if (s != nullptr)
            {
               value = vm->createValue(s);
            }
         }
      }

      // Try to find named parameter with given name
      if (value == nullptr)
      {
         wchar_t wattr[MAX_IDENTIFIER_LENGTH];
         utf8_to_wchar(attr.value, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         const TCHAR *s = event->getNamedParameter(wattr);
         if (s != nullptr)
         {
            value = vm->createValue(s);
         }
      }
   }
   return value;
}

/**
 * NXSL class Event: set attribute
 */
bool NXSL_EventClass::setAttr(NXSL_Object *object, const NXSL_Identifier& attr, NXSL_Value *value)
{
   Event *event = static_cast<Event*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("customMessage"))
   {
      if (value->isString())
      {
         event->setCustomMessage(value->getValueAsCString());
      }
      else
      {
         event->setCustomMessage(nullptr);
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("message"))
   {
      if (value->isString())
      {
         event->setMessage(value->getValueAsCString());
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("severity"))
   {
      int s = value->getValueAsInt32();
      if ((s >= SEVERITY_NORMAL) && (s <= SEVERITY_CRITICAL))
      {
         event->setSeverity(s);
      }
   }
   else
   {
      bool success = false;
      if (attr.value[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr.value[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *name = event->getParameterName(index - 1);
            event->setParameter(index - 1, CHECK_NULL_EX(name), value->getValueAsCString());
            success = true;
         }
      }
      if (!success)
      {
         wchar_t wattr[MAX_IDENTIFIER_LENGTH];
         utf8_to_wchar(attr.value, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         event->setNamedParameter(wattr, value->getValueAsCString());
      }
   }
   return true;
}

/**
 * Alarm::acknowledge() method
 */
NXSL_METHOD_DEFINITION(Alarm, acknowledge)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(AckAlarmById(alarm->getAlarmId(), nullptr, false, 0, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::resolve() method
 */
NXSL_METHOD_DEFINITION(Alarm, resolve)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(ResolveAlarmById(alarm->getAlarmId(), nullptr, false, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::terminate() method
 */
NXSL_METHOD_DEFINITION(Alarm, terminate)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(ResolveAlarmById(alarm->getAlarmId(), nullptr, true, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::addComment() method
 */
NXSL_METHOD_DEFINITION(Alarm, addComment)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   bool syncWithHelpdesk = true;
   if (argc > 1)
   {
      if (!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;
      syncWithHelpdesk = argv[1]->getValueAsBoolean();
   }

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   uint32_t id = 0;
   uint32_t rcc = UpdateAlarmComment(alarm->getAlarmId(), &id, argv[0]->getValueAsCString(), 0, syncWithHelpdesk);
   *result = (rcc == RCC_SUCCESS) ? vm->createValue(id) : vm->createValue();
   return 0;
}

/**
 * Alarm::getComments() method
 */
NXSL_METHOD_DEFINITION(Alarm, getComments)
{
   NXSL_Array *array = new NXSL_Array(vm);
   ObjectArray<AlarmComment> *alarmComments = GetAlarmComments(((Alarm *)object->getData())->getAlarmId());
   for(int i = 0; i < alarmComments->size(); i++)
   {
      array->append(vm->createValue(vm->createObject(&g_nxslAlarmCommentClass, alarmComments->get(i))));
   }
   delete alarmComments;
   *result = vm->createValue(array);
   return 0;
}

/**
 * Alarm::getComments() method
 */
NXSL_METHOD_DEFINITION(Alarm, requestAiAssistantComment)
{
   Alarm *alarm = static_cast<Alarm*>(object->getData());
   String response = alarm->requestAIAssistantComment();
   *result = !response.isEmpty() ? vm->createValue(response) : vm->createValue();
   return 0;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmClass::NXSL_AlarmClass() : NXSL_Class()
{
   setName(_T("Alarm"));

   NXSL_REGISTER_METHOD(Alarm, acknowledge, -1);
   NXSL_REGISTER_METHOD(Alarm, resolve, -1);
   NXSL_REGISTER_METHOD(Alarm, terminate, -1);
   NXSL_REGISTER_METHOD(Alarm, addComment, -1);
   NXSL_REGISTER_METHOD(Alarm, getComments, 0);
   NXSL_REGISTER_METHOD(Alarm, requestAiAssistantComment, 0);
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<Alarm*>(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   Alarm *alarm = static_cast<Alarm*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("ackBy"))
   {
      // Cast UID to signed to represent invalid UID as -1
      value = vm->createValue(static_cast<int32_t>(alarm->getAckByUser()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ackByUser"))
   {
      value = GetUserDBObjectForNXSL(alarm->getAckByUser(), vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("categories"))
   {
      value = alarm->categoryListToNXSLArray(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("creationTime"))
   {
      value = vm->createValue((INT64)alarm->getCreationTime());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dciId"))
   {
      value = vm->createValue(alarm->getDciId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("eventCode"))
   {
      value = vm->createValue(alarm->getSourceEventCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("eventId"))
   {
      value = vm->createValue(alarm->getSourceEventId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("eventName"))
   {
      shared_ptr<EventTemplate> e = FindEventTemplateByCode(alarm->getSourceEventCode());
      if (e != nullptr)
         value = vm->createValue(e->getName());
      else
         value = vm->createValue(alarm->getSourceEventCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("eventTagList"))
   {
      value = vm->createValue(alarm->getEventTags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("helpdeskReference"))
   {
      value = vm->createValue(alarm->getHelpDeskRef());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("helpdeskState"))
   {
      value = vm->createValue(alarm->getHelpDeskState());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(alarm->getAlarmId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("impact"))
   {
      value = vm->createValue(alarm->getImpact());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("key"))
   {
      value = vm->createValue(alarm->getKey());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastChangeTime"))
   {
      value = vm->createValue((INT64)alarm->getLastChangeTime());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("message"))
   {
      value = vm->createValue(alarm->getMessage());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("originalSeverity"))
   {
      value = vm->createValue(alarm->getOriginalSeverity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("parentId"))
   {
      value = vm->createValue(alarm->getParentAlarmId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedEvents"))
   {
      value = alarm->relatedEventsToNXSLArray(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("repeatCount"))
   {
      value = vm->createValue(alarm->getRepeatCount());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("resolvedBy"))
   {
      // Cast UID to signed to represent invalid UID as -1
      value = vm->createValue(static_cast<int32_t>(alarm->getResolvedByUser()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("resolvedByUser"))
   {
      value = GetUserDBObjectForNXSL(alarm->getResolvedByUser(), vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rcaScriptName"))
   {
      value = vm->createValue(alarm->getRcaScriptName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ruleGuid"))
   {
      wchar_t buffer[64];
      value = vm->createValue(alarm->getRuleGuid().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ruleDescription"))
   {
      value = vm->createValue(alarm->getRuleDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("severity"))
   {
      value = vm->createValue(alarm->getCurrentSeverity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("sourceObject"))
   {
      value = vm->createValue(alarm->getSourceObject());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("state"))
   {
      value = vm->createValue(alarm->getState());
   }
   return value;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmCommentClass::NXSL_AlarmCommentClass() : NXSL_Class()
{
   setName(_T("AlarmComment"));
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmCommentClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<AlarmComment*>(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmCommentClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   AlarmComment *alarmComment = static_cast<AlarmComment*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(alarmComment->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("changeTime"))
   {
      value = vm->createValue((INT64)alarmComment->getChangeTime());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("user"))
   {
      value = GetUserDBObjectForNXSL(alarmComment->getUserId(), vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userId"))
   {
      value = vm->createValue(alarmComment->getUserId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("text"))
   {
      value = vm->createValue(alarmComment->getText());
   }
   return value;
}

/**
 * DCI::forcePoll() method
 */
NXSL_METHOD_DEFINITION(DCI, forcePoll)
{
   const DCObjectInfo *dci = static_cast<shared_ptr<DCObjectInfo>*>(object->getData())->get();
   shared_ptr<NetObj> dcTarget = FindObjectById(dci->getOwnerId());
   if ((dcTarget != nullptr) && dcTarget->isDataCollectionTarget())
   {
      shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget*>(dcTarget.get())->getDCObjectById(dci->getId(), 0, true);
      if (dcObject != nullptr)
      {
         dcObject->requestForcePoll(nullptr);
      }
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Implementation of "DataPoint" class: constructor
 */
NXSL_DataPointClass::NXSL_DataPointClass() : NXSL_Class()
{
   setName(_T("DataPoint"));
}

/**
 * Implementation of "DataPoint" class: object destructor
 */
void NXSL_DataPointClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<std::pair<time_t, String>*>(object->getData());
}

/**
 * Implementation of "DataPoint" class: get attribute
 */
NXSL_Value *NXSL_DataPointClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto dp = static_cast<std::pair<time_t, String>*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("timestamp"))
   {
      value = vm->createValue(static_cast<int64_t>(dp->first));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("value"))
   {
      value = vm->createValue(dp->second);
   }
   return value;
}

/**
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   setName(_T("DCI"));

   NXSL_REGISTER_METHOD(DCI, forcePoll, 0);
}

/**
 * Implementation of "DCI" class: object destructor
 */
void NXSL_DciClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<DCObjectInfo>*>(object->getData());
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   const DCObjectInfo *dci = SharedObjectFromData<DCObjectInfo>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("activeThresholdSeverity"))
   {
      value = vm->createValue(dci->getThresholdSeverity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("comments"))
   {
		value = vm->createValue(dci->getComments());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dataType") && (dci->getType() == DCO_TYPE_ITEM))
   {
		value = vm->createValue(dci->getDataType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
		value = vm->createValue(dci->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("errorCount"))
   {
		value = vm->createValue(dci->getErrorCount());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
      value = vm->createValue(dci->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("hasActiveThreshold"))
   {
      value = vm->createValue(dci->hasActiveThreshold());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
		value = vm->createValue(dci->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("instance"))
   {
      value = vm->createValue(dci->getInstanceData());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("instanceName"))
   {
		value = vm->createValue(dci->getInstanceName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastCollectionTime"))
   {
      value = vm->createValue(dci->getLastCollectionTime());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastPollTime"))
   {
		value = vm->createValue(dci->getLastPollTime());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
		value = vm->createValue(dci->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("origin"))
   {
		value = vm->createValue(dci->getOrigin());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pollingInterval"))
   {
      value = vm->createValue(dci->getPollingInterval());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pollingSchedules"))
   {
      value = vm->createValue(new NXSL_Array(vm, dci->getPollingSchedules()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pollingScheduleType"))
   {
      value = vm->createValue(dci->getPollingScheduleType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("relatedObject"))
   {
      if (dci->getRelatedObject() != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(dci->getRelatedObject());
         value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("status"))
   {
		value = vm->createValue((LONG)dci->getStatus());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("systemTag"))
   {
		value = vm->createValue(dci->getSystemTag());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("template"))
   {
      if (dci->getTemplateId() != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(dci->getTemplateId());
         value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("templateId"))
   {
      value = vm->createValue(dci->getTemplateId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("templateItemId"))
   {
      value = vm->createValue(dci->getTemplateItemId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("thresholdDisableEndTime"))
   {
      value = vm->createValue(static_cast<int64_t>(dci->getThresholdDisableEndTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("transformedDataType") && (dci->getType() == DCO_TYPE_ITEM))
   {
      value = vm->createValue(dci->getTransformedDataType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
		value = vm->createValue((LONG)dci->getType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userTag"))
   {
      value = vm->createValue(dci->getUserTag());
   }
   return value;
}

/**
 * Implementation of "ScoredDciValue" class: constructor
 */
NXSL_ScoredDciValueClass::NXSL_ScoredDciValueClass() : NXSL_Class()
{
   setName(_T("ScoredDciValue"));
}

/**
 * Object destructor for ScoredDciValue
 */
void NXSL_ScoredDciValueClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<ScoredDciValue*>(object->getData());
}

/**
 * Implementation of "ScoredDciValue" class: get attribute
 */
NXSL_Value *NXSL_ScoredDciValueClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   const ScoredDciValue *v = static_cast<ScoredDciValue*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("score"))
   {
      value = vm->createValue(v->score);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("timestamp"))
   {
      value = vm->createValue(v->timestamp);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("value"))
   {
      value = vm->createValue(v->value);
   }
   return value;
}

/**
 * Implementation of "Sensor" class: constructor
 */
NXSL_SensorClass::NXSL_SensorClass() : NXSL_DCTargetClass()
{
   setName(_T("Sensor"));
}

/**
 * Implementation of "Sensor" class: get attribute
 */
NXSL_Value *NXSL_SensorClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto sensor = SharedObjectFromData<Sensor>(object);
   if (NXSL_COMPARE_ATTRIBUTE_NAME("capabilities"))
   {
      value = vm->createValue(sensor->getCapabilities());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("deviceAddress"))
   {
      value = vm->createValue(sensor->getDeviceAddress());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("deviceClass"))
   {
      value = vm->createValue(sensor->getDeviceClass());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("gatewayNode"))
   {
      shared_ptr<NetObj> gatewayNode = FindObjectById(sensor->getGatewayNodeId(), OBJECT_NODE);
      value = (gatewayNode != nullptr) ? gatewayNode->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("gatewayNodeId"))
   {
      value = vm->createValue(sensor->getGatewayNodeId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isModbus"))
   {
      value = vm->createValue(sensor->isModbusSupported());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("macAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(sensor->getMacAddress().toString(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modbusUnitId"))
   {
      value = vm->createValue(sensor->getModbusUnitId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("model"))
   {
      value = vm->createValue(sensor->getModel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(sensor->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(sensor->getVendor());
   }

   return value;
}

/**
 * SNMPTransport::get() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, get)
{
   if (!argv[0]->isString() && !argv[0]->isArray())
      return NXSL_ERR_NOT_STRING;

   SNMP_ObjectId oids[64];
   int varbindCount = 0;
   if (argv[0]->isString())
   {
      oids[0] = SNMP_ObjectId::parse(argv[0]->getValueAsCString());
      if (!oids[0].isValid())
      {
         *result = vm->createValue();
         return NXSL_ERR_SUCCESS;
      }
      varbindCount++;
   }
   else
   {
      NXSL_Array *a = argv[0]->getValueAsArray();
      for(int i = 0; (i < a->size()) && (i < 64); i++)
      {
         oids[i] = SNMP_ObjectId::parse(a->getByPosition(i)->getValueAsCString());
         if (!oids[i].isValid())
         {
            *result = vm->createValue();
            return 0;
         }
         varbindCount++;
      }
   }
   if (varbindCount == 0)
   {
      *result = vm->createValue();
      return NXSL_ERR_SUCCESS;
   }

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   for(int i = 0; i < varbindCount; i++)
      request.bindVariable(new SNMP_Variable(oids[i]));

   SNMP_PDU *response;
   uint32_t rc = transport->doRequest(&request, &response);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if ((response->getNumVariables() >= varbindCount) && (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         if (argv[0]->isString())
         {
            SNMP_Variable *v = response->getVariable(0);
            *result = vm->createValue(vm->createObject(&g_nxslSnmpVarBindClass, v));
         }
         else
         {
            NXSL_Array *a = new NXSL_Array(vm);
            for(int i = 0; i < varbindCount; i++)
            {
               SNMP_Variable *v = response->getVariable(i);
               a->append(vm->createValue(vm->createObject(&g_nxslSnmpVarBindClass, v)));
            }
            *result = vm->createValue(a);
         }
         response->unlinkVariables();
      }
      else
      {
         *result = vm->createValue();
      }
      delete response;
   }
   else
   {
      *result = vm->createValue();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * SNMPTransport::getValue() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, getValue)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const char *codepage;
   if (argc > 1)
   {
      if (!argv[1]->isString() && !argv[1]->isNull())
         return NXSL_ERR_NOT_STRING;
      codepage = argv[1]->isString() ? argv[1]->getValueAsMBString() : nullptr;
   }
   else
   {
      codepage = nullptr;
   }

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   TCHAR buffer[4096];
   if (SnmpGetEx(transport, argv[0]->getValueAsCString(), nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr, codepage) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(buffer);
   }
   else
   {
      *result = vm->createValue();
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * SNMPTransport::getValues() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, getValues)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isArray())
      return NXSL_ERR_NOT_ARRAY;

   if ((argc > 1) && !argv[1]->isString() && !argv[1]->isNull())
      return NXSL_ERR_NOT_STRING;

   const char *codepage;
   if (argc > 1)
   {
      if (!argv[1]->isString() && !argv[1]->isNull())
         return NXSL_ERR_NOT_STRING;
      codepage = argv[1]->isString() ? argv[1]->getValueAsMBString() : nullptr;
   }
   else
   {
      codepage = nullptr;
   }

   SNMP_ObjectId oids[64];
   int varbindCount = 0;
   NXSL_Array *a = argv[0]->getValueAsArray();
   if (a->size() == 0)
   {
      *result = vm->createValue();
      return NXSL_ERR_SUCCESS;
   }
   for(int i = 0; (i < a->size()) && (i < 64); i++)
   {
      oids[i] = SNMP_ObjectId::parse(a->getByPosition(i)->getValueAsCString());
      if (!oids[i].isValid())
      {
         *result = vm->createValue();
         return 0;
      }
      varbindCount++;
   }

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   for(int i = 0; i < varbindCount; i++)
      request.bindVariable(new SNMP_Variable(oids[i]));

   SNMP_PDU *response;
   uint32_t rc = transport->doRequest(&request, &response);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if ((response->getNumVariables() >= varbindCount) && (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         TCHAR buffer[4096];
         NXSL_Array *a = new NXSL_Array(vm);
         for(int i = 0; i < varbindCount; i++)
         {
            SNMP_Variable *v = response->getVariable(i);
            a->append(vm->createValue(v->getValueAsString(buffer, 4096, codepage)));
         }
         *result = vm->createValue(a);
      }
      else
      {
         *result = vm->createValue();
      }
      delete response;
   }
   else
   {
      *result = vm->createValue();
   }

   return NXSL_ERR_SUCCESS;
}

/**
 * SNMPTransport::set(oid, value, [dataType]) method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, set)
{
   if (argc < 2 || argc > 3)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isString() || (!argv[1]->isString() && !argv[1]->isObject(_T("ByteStream"))) || ((argc == 3) && !argv[2]->isString()))
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU request(SNMP_SET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   bool success;
   if (SnmpIsCorrectOID(argv[0]->getValueAsCString()))
   {
      SNMP_Variable *var = new SNMP_Variable(argv[0]->getValueAsCString());
      if (argc == 2)
      {
         if (argv[1]->isObject())
            var->setValueFromByteStream(ASN_OCTET_STRING, *static_cast<ByteStream*>(argv[1]->getValueAsObject()->getData()));
         else
            var->setValueFromString(ASN_OCTET_STRING, argv[1]->getValueAsCString());
      }
      else
      {
         uint32_t dataType = SnmpResolveDataType(argv[2]->getValueAsCString());
         if (dataType == ASN_NULL)
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: failed to resolve data type '%s', assume string"),
               argv[2]->getValueAsCString());
            dataType = ASN_OCTET_STRING;
         }
         if (argv[1]->isObject())
            var->setValueFromByteStream(dataType, *static_cast<ByteStream*>(argv[1]->getValueAsObject()->getData()));
         else
            var->setValueFromString(dataType, argv[1]->getValueAsCString());
      }
      request.bindVariable(var);
      success = true;
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: Invalid OID: %s"), argv[0]->getValueAsCString());
      success = false;
   }

   // Send request and process response
   if (success)
   {
      success = false;
      SNMP_PDU *response = nullptr;
      uint32_t snmpResult = transport->doRequest(&request, &response);
      if (snmpResult == SNMP_ERR_SUCCESS)
      {
         if (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS)
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: success"));
            success = true;
         }
         else
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: operation failed (error code %d)"), response->getErrorCode());
         }
         delete response;
      }
      else
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: %s"), SnmpGetErrorText(snmpResult));
      }
   }

   *result = vm->createValue(success);
   return 0;
}

/**
 * SNMP walk callback
 */
static uint32_t WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, NXSL_Array *varbinds)
{
   NXSL_VM *vm = static_cast<NXSL_VM*>(varbinds->vm());
   varbinds->append(vm->createValue(vm->createObject(&g_nxslSnmpVarBindClass, new SNMP_Variable(std::move(*var)))));
   return SNMP_ERR_SUCCESS;
}

/**
 * SNMPTransport::walk() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, walk)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   NXSL_Array *varbinds = new NXSL_Array(vm);
   if (SnmpWalk(transport, argv[0]->getValueAsCString(), WalkCallback, varbinds) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(varbinds);
   }
   else
   {
      *result = vm->createValue();
      delete varbinds;
   }
   return 0;
}

/**
 * Implementation of "SNMP_Transport" class: constructor
 */
NXSL_SNMPTransportClass::NXSL_SNMPTransportClass() : NXSL_Class()
{
	setName(_T("SNMPTransport"));

   NXSL_REGISTER_METHOD(SNMPTransport, get, 1);
   NXSL_REGISTER_METHOD(SNMPTransport, getValue, -1);
   NXSL_REGISTER_METHOD(SNMPTransport, getValues, -1);
   NXSL_REGISTER_METHOD(SNMPTransport, set, -1);
   NXSL_REGISTER_METHOD(SNMPTransport, walk, 1);
}

/**
 * Implementation of "SNMP_Transport" class: get attribute
 */
NXSL_Value *NXSL_SNMPTransportClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

	SNMP_Transport *t = static_cast<SNMP_Transport*>(object->getData());

	if (NXSL_COMPARE_ATTRIBUTE_NAME("snmpVersion"))
	{
	   const TCHAR *version = NULL;
		switch (t->getSnmpVersion())
		{
		   case SNMP_VERSION_1:
		      version = _T("1");
		      break;
		   case SNMP_VERSION_2C:
		      version = _T("2c");
		      break;
		   case SNMP_VERSION_3:
		      version = _T("3");
		      break;
		   case SNMP_VERSION_DEFAULT:
		      version = _T("Default");
		      break;
		   default:
		      version = _T("Unknown");
		}

		value = object->vm()->createValue(version);
	}

	return value;
}

/**
 * Implementation of "SNMP_Transport" class: NXSL object destructor
 */
void NXSL_SNMPTransportClass::onObjectDelete(NXSL_Object *object)
{
	delete static_cast<SNMP_Transport*>(object->getData());
}

/**
 * SNMPVarBind::getValueAsString() method
 */
NXSL_METHOD_DEFINITION(SNMPVarBind, getValueAsString)
{
   if (!argv[0]->isString() && !argv[0]->isNull())
      return NXSL_ERR_NOT_STRING;

   SNMP_Variable *var = static_cast<SNMP_Variable*>(object->getData());
   TCHAR buffer[4096];
   *result = vm->createValue(var->getValueAsString(buffer, sizeof(buffer) / sizeof(TCHAR), argv[0]->isNull() ? nullptr : argv[0]->getValueAsMBString()));
   return 0;
}

/**
 * SNMPVarBind::getValueAsByteStream() method
 */
NXSL_METHOD_DEFINITION(SNMPVarBind, getValueAsByteStream)
{
   SNMP_Variable* var = static_cast<SNMP_Variable*>(object->getData());
   *result = vm->createValue(vm->createObject(&g_nxslByteStreamClass, var->getValueAsByteStream()));
   return 0;
}

/**
 * NXSL class SNMP_VarBind: constructor
 */
NXSL_SNMPVarBindClass::NXSL_SNMPVarBindClass() : NXSL_Class()
{
	setName(_T("SNMPVarBind"));

   NXSL_REGISTER_METHOD(SNMPVarBind, getValueAsByteStream, 0);
   NXSL_REGISTER_METHOD(SNMPVarBind, getValueAsString, 1);
}

/**
 * NXSL class SNMP_VarBind: get attribute
 */
NXSL_Value *NXSL_SNMPVarBindClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
	SNMP_Variable *t = static_cast<SNMP_Variable*>(object->getData());
	if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
	{
		value = vm->createValue((UINT32)t->getType());
	}
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
	{
		value = vm->createValue(t->getName().toString());
	}
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("value"))
	{
   	TCHAR strValue[1024];
		value = vm->createValue(t->getValueAsString(strValue, 1024));
	}
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("printableValue"))
	{
   	TCHAR strValue[1024];
		bool convToHex = true;
		t->getValueAsPrintableString(strValue, 1024, &convToHex);
		value = vm->createValue(strValue);
	}
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("valueAsIp"))
	{
   	TCHAR strValue[128];
		t->getValueAsIPAddr(strValue);
		value = vm->createValue(strValue);
	}
	else if (NXSL_COMPARE_ATTRIBUTE_NAME("valueAsMac"))
	{
   	TCHAR strValue[128];
		value = vm->createValue(t->getValueAsMACAddr().toString(strValue));
	}

	return value;
}

/**
 * NXSL class SNMP_VarBind: NXSL object desctructor
 */
void NXSL_SNMPVarBindClass::onObjectDelete(NXSL_Object *object)
{
	delete static_cast<SNMP_Variable*>(object->getData());
}

/**
 * NXSL class ClientSession: constructor
 */
NXSL_ClientSessionClass::NXSL_ClientSessionClass() : NXSL_Class()
{
   setName(_T("ClientSession"));
}

/**
 * NXSL class ClientSession: get attribute
 */
NXSL_Value *NXSL_ClientSessionClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   ClientSession *session = static_cast<ClientSession*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("clientInfo"))
   {
      value = vm->createValue(session->getClientInfo());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("clientType"))
   {
      value = vm->createValue(session->getClientType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
      value = vm->createValue(session->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(session->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("loginName"))
   {
      value = vm->createValue(session->getLoginName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("loginTime"))
   {
      value = vm->createValue(static_cast<int64_t>(session->getLoginTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(session->getSessionName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("systemAccessRights"))
   {
      value = vm->createValue(session->getSystemAccessRights());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("user"))
   {
      value = GetUserDBObjectForNXSL(session->getUserId(), vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("userId"))
   {
      value = vm->createValue(session->getUserId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("webServerAddress"))
   {
      value = vm->createValue(session->getWebServerAddress());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("workstation"))
   {
      value = vm->createValue(session->getWorkstation());
   }

   return value;
}

/**
 * NXSL class UserDBObjectClass: constructor
 */
NXSL_UserDBObjectClass::NXSL_UserDBObjectClass() : NXSL_Class()
{
   setName(_T("UserDBObject"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserDBObjectClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   UserDatabaseObject *dbObject = static_cast<UserDatabaseObject*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(dbObject->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("flags"))
   {
      value = vm->createValue(dbObject->getFlags());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(dbObject->getGuidAsText(buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(dbObject->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isDeleted"))
   {
      value = vm->createValue(dbObject->isDeleted());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isDisabled"))
   {
      value = vm->createValue(dbObject->isDisabled());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isGroup"))
   {
      value = vm->createValue(dbObject->isGroup());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isModified"))
   {
      value = vm->createValue(dbObject->isModified());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isLDAPUser"))
   {
      value = vm->createValue(dbObject->isLDAPUser());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ldapDN"))
   {
      value = vm->createValue(dbObject->getDN());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ldapId"))
   {
      value = vm->createValue(dbObject->getLdapId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(dbObject->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("systemRights"))
   {
      value = vm->createValue(dbObject->getSystemRights());
   }

   return value;
}

/**
 * NXSL class UserClass: constructor
 */
NXSL_UserClass::NXSL_UserClass() : NXSL_UserDBObjectClass()
{
   setName(_T("User"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   User *user = static_cast<User*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("authMethod"))
   {
      value = vm->createValue(static_cast<int32_t>(user->getAuthMethod()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("certMappingData"))
   {
      value = vm->createValue(user->getCertMappingData());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("certMappingMethod"))
   {
      value = vm->createValue(user->getCertMappingMethod());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("disabledUntil"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getReEnableTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("email"))
   {
      value = vm->createValue(user->getEmail());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("fullName"))
   {
      value = vm->createValue(user->getFullName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("graceLogins"))
   {
      value = vm->createValue(user->getGraceLogins());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lastLogin"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getLastLoginTime()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("phoneNumber"))
   {
      value = vm->createValue(user->getPhoneNumber());
   }

   return value;
}

/**
 * NXSL class User: NXSL object destructor
 */
void NXSL_UserClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<User*>(object->getData());
}

/**
 * NXSL class UserGroupClass: constructor
 */
NXSL_UserGroupClass::NXSL_UserGroupClass() : NXSL_UserDBObjectClass()
{
   setName(_T("UserGroup"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserGroupClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   Group *group = static_cast<Group*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("memberCount"))
   {
      value = vm->createValue(group->getMemberCount());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("members"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      unique_ptr<ObjectArray<UserDatabaseObject>> userDB = FindUserDBObjects(group->getMembers());
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         if (userDB->get(i)->isGroup())
            array->append(vm->createValue(vm->createObject(&g_nxslUserGroupClass, userDB->get(i))));
         else
            array->append(vm->createValue(vm->createObject(&g_nxslUserClass, userDB->get(i))));
      }
      value = vm->createValue(array);
   }

   return value;
}

/**
 * NXSL class UserGroupClass: NXSL object destructor
 */
void NXSL_UserGroupClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<Group*>(object->getData());
}

/**
 * NXSL class NodeDependency: constructor
 */
NXSL_NodeDependencyClass::NXSL_NodeDependencyClass() : NXSL_Class()
{
   setName(_T("NodeDependency"));
}

/**
 * NXSL class NodeDependency: get attribute
 */
NXSL_Value *NXSL_NodeDependencyClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   DependentNode *dn = static_cast<DependentNode*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(dn->nodeId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isAgentProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_AGENT_PROXY) != 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isDataCollectionSource"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_DC_SOURCE) != 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isICMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_ICMP_PROXY) != 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isSNMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_SNMP_PROXY) != 0);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(dn->dependencyType);
   }
   return value;
}

/**
 * NXSL class NodeDependency: NXSL object desctructor
 */
void NXSL_NodeDependencyClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<DependentNode*>(object->getData());
}

/**
 * NXSL class HardwareComponent: constructor
 */
NXSL_HardwareComponent::NXSL_HardwareComponent()
{
   setName(_T("HardwareComponent"));
}

/**
 * NXSL class HardwareComponent: get attribute
 */
NXSL_Value* NXSL_HardwareComponent::getAttr(NXSL_Object* object, const NXSL_Identifier& attr)
{
   NXSL_Value* value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM* vm = object->vm();

   auto component = static_cast<HardwareComponent*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("capacity"))
   {
      value = vm->createValue(component->getCapacity());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("category"))
   {
      value = vm->createValue(component->getCategory());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("categoryName"))
   {
      value = vm->createValue(component->getCategoryName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("changeCode"))
   {
      value = vm->createValue(component->getChangeCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(component->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("index"))
   {
      value = vm->createValue(component->getIndex());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("location"))
   {
      value = vm->createValue(component->getLocation());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("model"))
   {
      value = vm->createValue(component->getModel());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("partNumber"))
   {
      value = vm->createValue(component->getPartNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("serialNumber"))
   {
      value = vm->createValue(component->getSerialNumber());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(component->getType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(component->getVendor());
   }
   return value;
}

/**
 * NXSL class HardwareComponent: object destructor
 */
void NXSL_HardwareComponent::onObjectDelete(NXSL_Object* object)
{
   delete static_cast<HardwareComponent*>(object->getData());
}

/**
 * NXSL class SoftwarePackage: constructor
 */
NXSL_SoftwarePackage::NXSL_SoftwarePackage()
{
   setName(_T("SoftwarePackage"));
}

/**
 * NXSL class SoftwarePackage: get attribute
 */
NXSL_Value* NXSL_SoftwarePackage::getAttr(NXSL_Object* object, const NXSL_Identifier& attr)
{
   NXSL_Value* value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM* vm = object->vm();

   auto package = static_cast<SoftwarePackage*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("changeCode"))
   {
      value = vm->createValue(package->getChangeCode());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("date"))
   {
      time_t t = package->getDate();
#if HAVE_LOCALTIME_R
      struct tm tmbuffer;
      struct tm *ltm = localtime_r(&t, &tmbuffer);
#else
      struct tm *ltm = localtime(&t);
#endif
      TCHAR buffer[32];
      _tcsftime(buffer, 32, _T("%Y%m%d"), ltm);
      value = vm->createValue(buffer);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(package->getDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(package->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("timestamp"))
   {
      value = vm->createValue(static_cast<int64_t>(package->getDate()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("uninstallKey"))
   {
      value = vm->createValue(package->getUninstallKey());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("url"))
   {
      value = vm->createValue(package->getUrl());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("vendor"))
   {
      value = vm->createValue(package->getVendor());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("version"))
   {
      value = vm->createValue(package->getVersion());
   }
   return value;
}

/**
 * NXSL class SoftwarePackage: object destructor
 */
void NXSL_SoftwarePackage::onObjectDelete(NXSL_Object* object)
{
   delete static_cast<SoftwarePackage*>(object->getData());
}

/**
 * NXSL "VLAN" class
 */
NXSL_VlanClass::NXSL_VlanClass()
{
   setName(_T("VLAN"));
}

/**
 * NXSL "VLAN" class: get attribute
 */
NXSL_Value *NXSL_VlanClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   VlanInfo *vlan = static_cast<VlanInfo*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(vlan->getVlanId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interfaces"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(vlan->getNodeId(), OBJECT_NODE));
      if (node != nullptr)
      {
         for(int i = 0; i < vlan->getNumPorts(); i++)
         {
            shared_ptr<Interface> iface = node->findInterfaceByIndex(vlan->getPort(i)->ifIndex);
            if (iface != nullptr)
            {
               a->append(iface->createNXSLObject(vm));
            }
         }
      }
      value = vm->createValue(a);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(vlan->getName());
   }
   return value;
}

/**
 * NXSL "VLAN" class: on object delete
 */
void NXSL_VlanClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<VlanInfo*>(object->getData());
}

/**
 * WebService::get() method
 */
NXSL_METHOD_DEFINITION(WebService, get)
{
   WebServiceHandle *websvc = static_cast<WebServiceHandle*>(object->getData());
   return BaseWebServiceRequestWithoutData(websvc, argc, argv, result, vm, HttpRequestMethod::_GET);
}

/**
 * WebService::post() method
 */
NXSL_METHOD_DEFINITION(WebService, post)
{
   WebServiceHandle *websvc = static_cast<WebServiceHandle*>(object->getData());
   return BaseWebServiceRequestWithData(websvc, argc, argv, result, vm, HttpRequestMethod::_POST);
}

/**
 * WebService::put() method
 */
NXSL_METHOD_DEFINITION(WebService, put)
{
   WebServiceHandle *websvc = static_cast<WebServiceHandle*>(object->getData());
   return BaseWebServiceRequestWithData(websvc, argc, argv, result, vm, HttpRequestMethod::_PUT);
}

/**
 * WebService::delete() method
 */
NXSL_METHOD_DEFINITION(WebService, delete)
{
   WebServiceHandle *websvc = static_cast<WebServiceHandle*>(object->getData());
   return BaseWebServiceRequestWithoutData(websvc, argc, argv, result, vm, HttpRequestMethod::_DELETE);
}

/**
 * WEB_SERVICE::patch() method
 */
NXSL_METHOD_DEFINITION(WebService, patch)
{
   WebServiceHandle *websvc = static_cast<WebServiceHandle*>(object->getData());
   return BaseWebServiceRequestWithData(websvc, argc, argv, result, vm, HttpRequestMethod::_PATCH);
}

/**
 * NXSL class "WebService"
 */
NXSL_WebServiceClass::NXSL_WebServiceClass()
{
   setName(_T("WebService"));

   NXSL_REGISTER_METHOD(WebService, get, -1);
   NXSL_REGISTER_METHOD(WebService, post, -1);
   NXSL_REGISTER_METHOD(WebService, put, -1);
   NXSL_REGISTER_METHOD(WebService, delete, -1);
   NXSL_REGISTER_METHOD(WebService, patch, -1);
}

/**
 * NXSL class WEB_SERVICE: get attribute
 */
NXSL_Value *NXSL_WebServiceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto websvc = static_cast<WebServiceHandle*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      value = vm->createValue(websvc->first->getId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(websvc->first->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(websvc->first->getDescription());
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_WebServiceClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<WebServiceHandle*>(object->getData());
}

/**
 * NXSL class "WebServiceResponse"
 */
NXSL_WebServiceResponseClass::NXSL_WebServiceResponseClass()
{
   setName(_T("WebServiceResponse"));
}

/**
 * NXSL class WebServiceCallResult: get attribute
 */
NXSL_Value *NXSL_WebServiceResponseClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto result = static_cast<WebServiceCallResult*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("agentErrorCode"))
   {
      value = vm->createValue(result->agentErrorCode);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("document"))
   {
      value = vm->createValue(result->document);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("errorMessage"))
   {
      value = vm->createValue(result->errorMessage);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("httpResponseCode"))
   {
      value = vm->createValue(result->httpResponseCode);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("success"))
   {
      value = vm->createValue(result->success);
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_WebServiceResponseClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<WebServiceCallResult*>(object->getData());
}

/**
 * NXSL "OSPFArea" class
 */
NXSL_OSPFAreaClass::NXSL_OSPFAreaClass()
{
   setName(_T("OSPFArea"));
}

/**
 * NXSL class OSPFArea: get attribute
 */
NXSL_Value *NXSL_OSPFAreaClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto area = static_cast<OSPFArea*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("areaBorderRouterCount"))
   {
      value = vm->createValue(area->areaBorderRouterCount);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("asBorderRouterCount"))
   {
      value = vm->createValue(area->asBorderRouterCount);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("id"))
   {
      TCHAR buffer[16];
      value = vm->createValue(IpToStr(area->id, buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("lsaCount"))
   {
      value = vm->createValue(area->lsaCount);
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_OSPFAreaClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<OSPFArea*>(object->getData());
}

/**
 * NXSL "OSPFNeighbor" class
 */
NXSL_OSPFNeighborClass::NXSL_OSPFNeighborClass()
{
   setName(_T("OSPFNeighbor"));
}

/**
 * NXSL class OSPFNeighbor: get attribute
 */
NXSL_Value *NXSL_OSPFNeighborClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto neighbor = static_cast<OSPFNeighbor*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("areaId"))
   {
      TCHAR buffer[16];
      value = neighbor->isVirtual ? vm->createValue(IpToStr(neighbor->areaId, buffer)) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ifIndex"))
   {
      value = vm->createValue(neighbor->ifIndex);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("ipAddress"))
   {
      value = NXSL_InetAddressClass::createObject(vm, neighbor->ipAddress);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("isVirtual"))
   {
      value = vm->createValue(neighbor->isVirtual);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("node"))
   {
      shared_ptr<NetObj> node = FindObjectById(neighbor->nodeId, OBJECT_NODE);
      value = (node != nullptr) ? node->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("nodeId"))
   {
      value = vm->createValue(neighbor->nodeId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("routerId"))
   {
      TCHAR buffer[16];
      value = vm->createValue(IpToStr(neighbor->routerId, buffer));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("state"))
   {
      value = vm->createValue(static_cast<int32_t>(neighbor->state));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("stateText"))
   {
      value = vm->createValue(OSPFNeighborStateToText(neighbor->state));
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_OSPFNeighborClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<OSPFNeighbor*>(object->getData());
}

/**
 * NetworkMapLink::setName(name) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setName)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   if (_tcscmp(link->get()->getName(), argv[0]->getValueAsCString()))
   {
      link->get()->setName(argv[0]->getValueAsCString());
      link->setModified();
   }
   return 0;
}

/**
 * NetworkMapLink::setConnectorName1(name) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setConnectorName1)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   if (_tcscmp(link->get()->getConnector1Name(), argv[0]->getValueAsCString()))
   {
      link->get()->setConnector1Name(argv[0]->getValueAsCString());
      link->setModified();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * NetworkMapLink::setConnectorName1(name) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setConnectorName2)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   if (_tcscmp(link->get()->getConnector2Name(), argv[0]->getValueAsCString()))
   {
      link->get()->setConnector2Name(argv[0]->getValueAsCString());
      link->setModified();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * NetworkMapLink::setInterface1(name) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setInterface1)
{
   if (!argv[0]->isInteger() && !argv[0]->isObject(_T("Interface")))
      return NXSL_ERR_NOT_INTEGER;

   uint32_t ifaceId = argv[0]->isInteger() ?
      argv[0]->getValueAsUInt32() :
      static_cast<shared_ptr<NetObj>*>(argv[0]->getValueAsObject()->getData())->get()->getId();

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   if (link->get()->getInterface1() != ifaceId)
   {
      link->get()->setInterface1(ifaceId);
      link->setModified();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * NetworkMapLink::setInterface1(name) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setInterface2)
{
   if (!argv[0]->isInteger() && !argv[0]->isObject(_T("Interface")))
      return NXSL_ERR_NOT_INTEGER;

   uint32_t ifaceId = argv[0]->isInteger() ?
      argv[0]->getValueAsUInt32() :
      static_cast<shared_ptr<NetObj>*>(argv[0]->getValueAsObject()->getData())->get()->getId();

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   if (link->get()->getInterface2() != ifaceId)
   {
      link->get()->setInterface2(ifaceId);
      link->setModified();
   }
   return NXSL_ERR_SUCCESS;
}

/**
 * NetworkMapLink::setColorConfig() method
 * 1 - color selection mode
 * 2 - array of objects or object's id for "object status" mode / script name for "script" mode / color definition for "custom color" mode
 * 3 - (optional) indicates if thresholds should be included into calculation for "object status" mode, default false
 * 4 - (optional) indicates if link utilization should be included into calculation for "object status" mode, default false
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setColorConfig)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());

   MapLinkColorSource source = static_cast<MapLinkColorSource>(argv[0]->getValueAsUInt32());
   if (source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
   {
      if (argc < 2)
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;

      IntegerArray<uint32_t> objects;
      if (!argv[1]->isArray())
      {
         return NXSL_ERR_NOT_ARRAY;
      }
      NXSL_Array *array = argv[1]->getValueAsArray();
      int size = array->size();
      for (int i = 0; i < size; i++)
      {
         NXSL_Value *entry = array->get(i);
         if (entry->isObject(_T("NetObj")))
         {
            objects.add(static_cast<shared_ptr<NetObj>*>(entry->getValueAsObject()->getData())->get()->getId());
         }
         else if (entry->isInteger())
         {
            objects.add(entry->getValueAsUInt32());
         }
      }

      bool useThresholds = (argc > 2) ? argv[2]->getValueAsBoolean() : false;
      bool useLinkUtilization = (argc > 3) ? argv[3]->getValueAsBoolean() : false;

      link->setColorSourceToObjectStatus(objects, useThresholds, useLinkUtilization);
   }
   else if (source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_SCRIPT)
   {
      const TCHAR *scriptName = nullptr;
      if (argc > 1)
      {
         if (!argv[1]->isString())
         {
            return NXSL_ERR_NOT_STRING;
         }
         scriptName = argv[1]->getValueAsCString();
      }
      else
      {
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
      }
      link->setColorSourceToScript(scriptName);
   }
   else if (source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_CUSTOM_COLOR)
   {
      if (argc > 1)
      {
         if (argv[1]->isInteger())
         {
            Color color(argv[1]->getValueAsUInt32());
            color.swap();  // Assume color returned as 0xRRGGBB, but Java UI expects 0xBBGGRR
            link->setColorSourceToCustomColor(color.toInteger());
         }
         else if (argv[1]->isString())
         {
            link->setColorSourceToCustomColor(Color::parseCSS(argv[1]->getValueAsCString()).toInteger());
         }
         else
         {
            return NXSL_ERR_NOT_STRING;
         }
      }
      else
      {
         return NXSL_ERR_INVALID_ARGUMENT_COUNT;
      }
   }
   else if ((source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_DEFAULT) ||
            (source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_INTERFACE_STATUS) ||
            (source == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_LINK_UTILIZATION))
   {
      link->setColorSource(source);
   }
   return 0;
}

/**
 * NetworkMapLink::setRoutingAlgorithm(algorithm) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setRoutingAlgorithm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   link->setRoutingAlgorithm(argv[0]->getValueAsUInt32());
   return 0;
}

/**
 * NetworkMapLink::setWidth(width) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setWidth)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   link->setWidth(argv[0]->getValueAsUInt32());
   return 0;
}

/**
 * NetworkMapLink::setStyle(style) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, setStyle)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   link->setStyle(argv[0]->getValueAsUInt32());
   return 0;
}

/**
 * NetworkMapLink::updateDataSource(dci, format, location) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, updateDataSource)
{
   if (argc < 1 || argc > 3)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isObject(g_nxslDciClass.getName()))
      return NXSL_ERR_NOT_OBJECT;

   if ((argc > 1) && !argv[1]->isString() && !argv[1]->isNull())
      return NXSL_ERR_NOT_STRING;

   if ((argc > 2) && !argv[2]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());

   const shared_ptr<DCObjectInfo> dci = *static_cast<shared_ptr<DCObjectInfo>*>(argv[0]->getValueAsObject()->getData());
   LinkDataLocation location = LinkLocationFromInt((argc > 2) ? argv[2]->getValueAsUInt32() : 0);
   link->updateDataSource(dci, (argc > 1) ? argv[1]->getValueAsCString() : _T(""), location);

   return 0;
}

/**
 * NetworkMapLink::clearDataSource() method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, clearDataSource)
{
   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   link->clearDataSource();
   return 0;
}

/**
 * NetworkMapLink::removeDataSource(index) method
 */
NXSL_METHOD_DEFINITION(NetworkMapLink, removeDataSource)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());
   link->removeDataSource(argv[0]->getValueAsUInt32());
   return 0;
}

/**
 * NXSL "NetworkMapLink" class
 */
NXSL_NetworkMapLinkClass::NXSL_NetworkMapLinkClass()
{
   setName(_T("NetworkMapLink"));

   NXSL_REGISTER_METHOD(NetworkMapLink, setName, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setConnectorName1, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setConnectorName2, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setColorConfig, -1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setInterface1, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setInterface2, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setRoutingAlgorithm, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setStyle, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, setWidth, 1);
   NXSL_REGISTER_METHOD(NetworkMapLink, updateDataSource, -1);
   NXSL_REGISTER_METHOD(NetworkMapLink, clearDataSource, 0);
   NXSL_REGISTER_METHOD(NetworkMapLink, removeDataSource, 1);
}

/**
 * NXSL class NetworkMapLink: get attribute
 */
NXSL_Value *NXSL_NetworkMapLinkClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto link = static_cast<NetworkMapLinkContainer*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("connectorName1"))
   {
      value = vm->createValue(link->get()->getConnector1Name());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("connectorName2"))
   {
      value = vm->createValue(link->get()->getConnector2Name());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("color"))
   {
      if (link->get()->getColorSource() == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_CUSTOM_COLOR)
      {
         Color c(link->get()->getColor());
         value = vm->createValue(c.toCSS());
      }
      else
         value = vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("colorScriptName"))
   {
      if (link->get()->getColorSource() == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_SCRIPT)
      {
         value = vm->createValue(link->get()->getColorProvider());
      }
      else
         value = vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("colorObjects"))
   {
      if (link->get()->getColorSource() == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
      {
         value = link->getColorObjects(vm);
      }
      else
         value = vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("colorSource"))
   {
      value = vm->createValue(link->get()->getColorSource());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dataSource"))
   {
      value = link->getDataSource(vm);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interface1"))
   {
      shared_ptr<NetObj> object = FindObjectById(link->get()->getInterface1());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("interface2"))
   {
      shared_ptr<NetObj> object = FindObjectById(link->get()->getInterface2());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("name"))
   {
      value = vm->createValue(link->get()->getName());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("object1"))
   {
      shared_ptr<NetObj> object = FindObjectById(link->get()->getElement1());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("object2"))
   {
      shared_ptr<NetObj> object = FindObjectById(link->get()->getElement2());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectId1"))
   {
      value = vm->createValue(link->get()->getElement1());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectId2"))
   {
      value = vm->createValue(link->get()->getElement2());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("routing"))
   {
      value = vm->createValue(link->getRouting());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("style"))
   {
      value = vm->createValue(link->getStyle());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("type"))
   {
      value = vm->createValue(link->get()->getType());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("useActiveThresholds"))
   {
      if (link->get()->getColorSource() == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
      {
         value = vm->createValue(link->isUsingActiveThresholds());
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("useInterfaceUtilization"))
   {
      if (link->get()->getColorSource() == MapLinkColorSource::MAP_LINK_COLOR_SOURCE_OBJECT_STATUS)
      {
         value = vm->createValue(link->useInterfaceUtilization());
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("width"))
   {
      value = vm->createValue(link->getWidth());
   }
   return value;
}

/**
 * NXSL "LinkDataSource" class
 */
NXSL_LinkDataSourceClass::NXSL_LinkDataSourceClass()
{
   setName(_T("LinkDataSource"));
}

/**
 * NXSL class LinkDataSource: get attribute
 */
NXSL_Value *NXSL_LinkDataSourceClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto linkDataSource = static_cast<LinkDataSouce*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("dci"))
   {
      shared_ptr<NetObj> object = FindObjectById(linkDataSource->getNodeId());
      shared_ptr<DCObject> dci = nullptr;
      if ((object != nullptr) && object->isDataCollectionTarget())
      {
         dci = static_cast<DataCollectionTarget *>(object.get())->getDCObjectById(linkDataSource->getDciId(), 0);
      }
      value = (dci != nullptr) ? dci->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("dciId"))
   {
      value = vm->createValue(linkDataSource->getDciId());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("format"))
   {
      value = vm->createValue(linkDataSource->getFormat());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("location"))
   {
      value = vm->createValue(IntFromLinkLocation(linkDataSource->getLocation()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("object"))
   {
      shared_ptr<NetObj> object = FindObjectById(linkDataSource->getNodeId());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectId"))
   {
      value = vm->createValue(linkDataSource->getNodeId());
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_LinkDataSourceClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<LinkDataSouce*>(object->getData());
}

/**
 * NXSL "DowntimeInfo" class
 */
NXSL_DowntimeInfoClass::NXSL_DowntimeInfoClass()
{
   setName(_T("DowntimeInfo"));
}

/**
 * NXSL class DowntimeInfo: get attribute
 */
NXSL_Value *NXSL_DowntimeInfoClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto downtimeInfo = static_cast<DowntimeInfo*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("object"))
   {
      shared_ptr<NetObj> object = FindObjectById(downtimeInfo->objectId);
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectId"))
   {
      value = vm->createValue(downtimeInfo->objectId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("totalDowntime"))
   {
      value = vm->createValue(downtimeInfo->totalDowntime);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("uptimePercentage"))
   {
      value = vm->createValue(downtimeInfo->uptimePct);
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_DowntimeInfoClass::onObjectDelete(NXSL_Object *object)
{
   MemFree(object->getData());
}

/**
 * NXSL "NetworkPathCheckResult" class
 */
NXSL_NetworkPathCheckResultClass::NXSL_NetworkPathCheckResultClass()
{
   setName(_T("NetworkPathCheckResult"));
}

/**
 * NXSL class NetworkPathCheckResult: get attribute
 */
NXSL_Value *NXSL_NetworkPathCheckResultClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto pathCheckResult = static_cast<NetworkPathCheckResult*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(pathCheckResult->buildDescription());
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("reason"))
   {
      value = vm->createValue(static_cast<int32_t>(pathCheckResult->reason));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rootCauseInterface"))
   {
      shared_ptr<NetObj> object = FindObjectById(pathCheckResult->rootCauseInterfaceId, OBJECT_INTERFACE);
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rootCauseInterfaceId"))
   {
      value = vm->createValue(pathCheckResult->rootCauseInterfaceId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rootCauseNode"))
   {
      shared_ptr<NetObj> object = FindObjectById(pathCheckResult->rootCauseNodeId);
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("rootCauseNodeId"))
   {
      value = vm->createValue(pathCheckResult->rootCauseNodeId);
   }
   return value;
}

/**
 * Object destruction handler
 */
void NXSL_NetworkPathCheckResultClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<NetworkPathCheckResult*>(object->getData());
}

/**
 * Class objects
 */
NXSL_AccessPointClass g_nxslAccessPointClass;
NXSL_AlarmClass g_nxslAlarmClass;
NXSL_AlarmCommentClass g_nxslAlarmCommentClass;
NXSL_AssetClass g_nxslAssetClass;
NXSL_AssetPropertiesClass g_nxslAssetPropertiesClass;
NXSL_BusinessServiceClass g_nxslBusinessServiceClass;
NXSL_BusinessServiceCheckClass g_nxslBusinessServiceCheckClass;
NXSL_ChassisClass g_nxslChassisClass;
NXSL_CircuitClass g_nxslCircuitClass;
NXSL_ClientSessionClass g_nxslClientSessionClass;
NXSL_ClusterClass g_nxslClusterClass;
NXSL_CollectorClass g_nxslCollectorClass;
NXSL_ContainerClass g_nxslContainerClass;
NXSL_DataPointClass g_nxslDataPointClass;
NXSL_DciClass g_nxslDciClass;
NXSL_DCTargetClass g_nxslDCTargetClass;
NXSL_DowntimeInfoClass g_nxslDowntimeInfoClass;
NXSL_EventClass g_nxslEventClass;
NXSL_HardwareComponent g_nxslHardwareComponent;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_LinkDataSourceClass g_nxslLinkDataSourceClass;
NXSL_MaintenanceJournalRecordClass g_nxslMaintenanceJournalRecordClass;
NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NetworkMapClass g_nxslNetworkMapClass;
NXSL_NetworkMapLinkClass g_nxslNetworkMapLinkClass;
NXSL_NetworkPathCheckResultClass g_nxslNetworkPathCheckResultClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_NodeDependencyClass g_nxslNodeDependencyClass;
NXSL_OSPFAreaClass g_nxslOSPFAreaClass;
NXSL_OSPFNeighborClass g_nxslOSPFNeighborClass;
NXSL_RadioInterfaceClass g_nxslRadioInterfaceClass;
NXSL_ScoredDciValueClass g_nxslScoredDciValueClass;
NXSL_SensorClass g_nxslSensorClass;
NXSL_ServiceRootClass g_nxslServiceRootClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_SoftwarePackage g_nxslSoftwarePackage;
NXSL_SubnetClass g_nxslSubnetClass;
NXSL_TemplateClass g_nxslTemplateClass;
NXSL_TunnelClass g_nxslTunnelClass;
NXSL_UserDBObjectClass g_nxslUserDBObjectClass;
NXSL_UserClass g_nxslUserClass;
NXSL_UserGroupClass g_nxslUserGroupClass;
NXSL_VlanClass g_nxslVlanClass;
NXSL_WebServiceClass g_nxslWebServiceClass;
NXSL_WebServiceResponseClass g_nxslWebServiceResponseClass;
NXSL_WirelessDomainClass g_nxslWirelessDomainClass;
NXSL_WirelessStationClass g_nxslWirelessStationClass;
NXSL_ZoneClass g_nxslZoneClass;
