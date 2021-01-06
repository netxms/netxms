/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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

/**
 * Get ICMP statistic for node sub-object
 */
template <class O> static NXSL_Value *GetObjectIcmpStatistic(const O *object, IcmpStatFunction function, NXSL_VM *vm)
{
   NXSL_Value *value;
   auto parentNode = object->getParentNode();
   if (parentNode != nullptr)
   {
      TCHAR target[MAX_OBJECT_NAME + 5], buffer[MAX_RESULT_LENGTH];
      _sntprintf(target, MAX_OBJECT_NAME + 4, _T("X(N:%s)"), object->getName());
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
 * Safely get pointer from object data
 */
template<typename T> static T *SharedObjectFromData(NXSL_Object *nxslObject)
{
   void *data = nxslObject->getData();
   return (data != nullptr) ? static_cast<shared_ptr<T>*>(data)->get() : nullptr;
}

/**
 * NetObj::bind(object)
 */
NXSL_METHOD_DEFINITION(NetObj, bind)
{
   shared_ptr<NetObj> thisObject = *static_cast<shared_ptr<NetObj>*>(object->getData());
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
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

   thisObject->addChild(child);
   child->addParent(thisObject);
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
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!IsValidParentClass(thisObject->getObjectClass(), parent->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (thisObject->isChild(parent->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   parent->addChild(thisObject);
   thisObject->addParent(parent);
   parent->calculateCompoundStatus();

   *result = vm->createValue();
   return 0;
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
   *result = vm->createValue(n->expandText(argv[0]->getValueAsCString(), nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr));
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
 * NetObj::leaveMaintenance()
 */
NXSL_METHOD_DEFINITION(NetObj, leaveMaintenance)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->leaveMaintenanceMode(0);
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::manage()
 */
NXSL_METHOD_DEFINITION(NetObj, manage)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(true);
   *result = vm->createValue();
   return 0;
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
   return 0;
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
   return 0;
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
   return 0;
}

/**
 * setComments(text)
 */
NXSL_METHOD_DEFINITION(NetObj, setComments)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setComments(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
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

   if(argc == 3 && !argv[2]->isBoolean())
      return NXSL_ERR_NOT_BOOLEAN;

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
   return 0;
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
   return 0;
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
   return 0;
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
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *child = static_cast<shared_ptr<NetObj>*>(nxslChild->getData())->get();
   thisObject->deleteChild(*child);
   child->deleteParent(*thisObject);

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
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   parent->deleteChild(*thisObject);
   thisObject->deleteParent(*parent);

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::unmanage()
 */
NXSL_METHOD_DEFINITION(NetObj, unmanage)
{
   static_cast<shared_ptr<NetObj>*>(object->getData())->get()->setMgmtStatus(false);
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class NetObj: constructor
 */
NXSL_NetObjClass::NXSL_NetObjClass() : NXSL_Class()
{
   setName(_T("NetObj"));

   NXSL_REGISTER_METHOD(NetObj, bind, 1);
   NXSL_REGISTER_METHOD(NetObj, bindTo, 1);
   NXSL_REGISTER_METHOD(NetObj, clearGeoLocation, 0);
   NXSL_REGISTER_METHOD(NetObj, delete, 0);
   NXSL_REGISTER_METHOD(NetObj, deleteCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, enterMaintenance, -1);
   NXSL_REGISTER_METHOD(NetObj, expandString, 1);
   NXSL_REGISTER_METHOD(NetObj, getCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, leaveMaintenance, 0);
   NXSL_REGISTER_METHOD(NetObj, manage, 0);
   NXSL_REGISTER_METHOD(NetObj, rename, 1);
   NXSL_REGISTER_METHOD(NetObj, setAlias, 1);
   NXSL_REGISTER_METHOD(NetObj, setCategory, 1);
   NXSL_REGISTER_METHOD(NetObj, setComments, 1);
   NXSL_REGISTER_METHOD(NetObj, setCustomAttribute, -1);
   NXSL_REGISTER_METHOD(NetObj, setGeoLocation, 1);
   NXSL_REGISTER_METHOD(NetObj, setMapImage, 1);
   NXSL_REGISTER_METHOD(NetObj, setNameOnMap, 1);
   NXSL_REGISTER_METHOD(NetObj, setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(NetObj, setStatusPropagation, -1);
   NXSL_REGISTER_METHOD(NetObj, unbind, 1);
   NXSL_REGISTER_METHOD(NetObj, unbindFrom, 1);
   NXSL_REGISTER_METHOD(NetObj, unmanage, 0);
}

/**
 * Object destruction handler
 */
void NXSL_NetObjClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<NetObj>*>(object->getData());
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *_object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(_object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = _object->vm();
   auto object = SharedObjectFromData<NetObj>(_object);
   if (compareAttributeName(attr, "alarms"))
   {
      ObjectArray<Alarm> *alarms = GetAlarms(object->getId(), true);
      alarms->setOwner(Ownership::False);
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < alarms->size(); i++)
         array->append(vm->createValue(new NXSL_Object(vm, &g_nxslAlarmClass, alarms->get(i))));
      value = vm->createValue(array);
      delete alarms;
   }
   else if (compareAttributeName(attr, "alias"))
   {
      value = vm->createValue(object->getAlias());
   }
   else if (compareAttributeName(attr, "backupZoneProxy"))
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
   else if (compareAttributeName(attr, "backupZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(true));
   }
   else if (compareAttributeName(attr, "category"))
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
   else if (compareAttributeName(attr, "categoryId"))
   {
      value = vm->createValue(object->getCategoryId());
   }
   else if (compareAttributeName(attr, "children"))
   {
      value = vm->createValue(object->getChildrenForNXSL(vm));
   }
   else if (compareAttributeName(attr, "city"))
   {
      value = vm->createValue(object->getPostalAddress()->getCity());
   }
   else if (compareAttributeName(attr, "comments"))
   {
      value = vm->createValue(object->getComments());
   }
   else if (compareAttributeName(attr, "country"))
   {
      value = vm->createValue(object->getPostalAddress()->getCountry());
   }
   else if (compareAttributeName(attr, "creationTime"))
   {
      value = vm->createValue(static_cast<INT64>(object->getCreationTime()));
   }
   else if (compareAttributeName(attr, "customAttributes"))
   {
      value = object->getCustomAttributesForNXSL(vm);
   }
   else if (compareAttributeName(attr, "geolocation"))
   {
      value = NXSL_GeoLocationClass::createObject(vm, object->getGeoLocation());
   }
   else if (compareAttributeName(attr, "guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getGuid().toString(buffer));
   }
   else if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(object->getId());
   }
   else if (compareAttributeName(attr, "ipAddr"))
   {
      TCHAR buffer[64];
      object->getPrimaryIpAddress().toString(buffer);
      value = vm->createValue(buffer);
   }
   else if (compareAttributeName(attr, "isInMaintenanceMode"))
   {
      value = vm->createValue(object->isInMaintenanceMode());
   }
   else if (compareAttributeName(attr, "maintenanceInitiator"))
   {
      value = vm->createValue(object->getMaintenanceInitiator());
   }
   else if (compareAttributeName(attr, "mapImage"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getMapImage().toString(buffer));
   }
   else if (compareAttributeName(attr, "name"))
   {
      value = vm->createValue(object->getName());
   }
   else if (compareAttributeName(attr, "nameOnMap"))
   {
      value = vm->createValue(object->getNameOnMap());
   }
   else if (compareAttributeName(attr, "parents"))
   {
      value = vm->createValue(object->getParentsForNXSL(vm));
   }
   else if (compareAttributeName(attr, "postcode"))
   {
      value = vm->createValue(object->getPostalAddress()->getPostCode());
   }
   else if (compareAttributeName(attr, "primaryZoneProxy"))
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
   else if (compareAttributeName(attr, "primaryZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(false));
   }
   else if (compareAttributeName(attr, "responsibleUsers"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *responsibleUsers = object->getAllResponsibleUsers();
      ObjectArray<UserDatabaseObject> *userDB = FindUserDBObjects(responsibleUsers);
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         array->append(userDB->get(i)->createNXSLObject(vm));
      }
      value = vm->createValue(array);
      delete userDB;
      delete responsibleUsers;
   }
   else if (compareAttributeName(attr, "state"))
   {
      value = vm->createValue(object->getState());
   }
   else if (compareAttributeName(attr, "status"))
   {
      value = vm->createValue((LONG)object->getStatus());
   }
   else if (compareAttributeName(attr, "streetAddress"))
   {
      value = vm->createValue(object->getPostalAddress()->getStreetAddress());
   }
   else if (compareAttributeName(attr, "type"))
   {
      value = vm->createValue((LONG)object->getObjectClass());
   }
   else if (object != nullptr)   // Object can be null if attribute scan is running
   {
#ifdef UNICODE
      WCHAR wattr[MAX_IDENTIFIER_LENGTH];
      MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
      wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
      value = object->getCustomAttributeForNXSL(vm, wattr);
#else
      value = object->getCustomAttributeForNXSL(vm, attr);
#endif
   }
   return value;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_SubnetClass::NXSL_SubnetClass() : NXSL_NetObjClass()
{
   setName(_T("Subnet"));
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_SubnetClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto subnet = SharedObjectFromData<Subnet>(object);
   if (compareAttributeName(attr, "ipNetMask"))
   {
      value = vm->createValue(subnet->getIpAddress().getMaskBits());
   }
   else if (compareAttributeName(attr, "isSyntheticMask"))
   {
      value = vm->createValue(subnet->isSyntheticMask());
   }
   else if (compareAttributeName(attr, "zone"))
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
   else if (compareAttributeName(attr, "zoneUIN"))
   {
      value = vm->createValue(subnet->getZoneUIN());
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
   shared_ptr<DataCollectionTarget> thisObject = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslTemplate = argv[0]->getValueAsObject();
   if (!nxslTemplate->getClass()->instanceOf(g_nxslTemplateClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   auto tmpl = *static_cast<shared_ptr<Template>*>(nxslTemplate->getData());
   tmpl->deleteChild(*thisObject);
   thisObject->deleteParent(*tmpl);
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
   NXSL_REGISTER_METHOD(DataCollectionTarget, readInternalParameter, 1);
   NXSL_REGISTER_METHOD(DataCollectionTarget, removeTemplate, 1);
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_DCTargetClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto dcTarget = SharedObjectFromData<DataCollectionTarget>(object);
   if (compareAttributeName(attr, "templates"))
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
NXSL_Value *NXSL_ZoneClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto zone = SharedObjectFromData<Zone>(object);
   if (compareAttributeName(attr, "proxyNodes"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies->size(); i++)
      {
         shared_ptr<NetObj> node = FindObjectById(proxies->get(i), OBJECT_NODE);
         if (node != nullptr)
            array->append(node->createNXSLObject(vm));
      }
      value = vm->createValue(array);
      delete proxies;
   }
   else if (compareAttributeName(attr, "proxyNodeIds"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies->size(); i++)
         array->append(vm->createValue(proxies->get(i)));
      value = vm->createValue(array);
      delete proxies;
   }
   else if (compareAttributeName(attr, "uin"))
   {
      value = vm->createValue(zone->getUIN());
   }
   return value;
}

/**
 * Generic implementation for flag changing methods
 */
static int ChangeFlagMethod(NXSL_Object *object, NXSL_Value *arg, NXSL_Value **result, uint32_t flag, bool invert)
{
   if (!arg->isBoolean())
      return NXSL_ERR_NOT_BOOLEAN;

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
 * Node::createSNMPTransport(port, context) method
 */
NXSL_METHOD_DEFINITION(Node, createSNMPTransport)
{
   if (argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 1) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT16 port = (argc > 0) ? static_cast<UINT16>(argv[0]->getValueAsInt32()) : 0;
   const TCHAR *context = (argc > 1) ? argv[1]->getValueAsCString() : nullptr;
   SNMP_Transport *t = static_cast<shared_ptr<Node>*>(object->getData())->get()->createSnmpTransport(port, SNMP_VERSION_DEFAULT, context);
   *result = (t != nullptr) ? vm->createValue(new NXSL_Object(vm, &g_nxslSnmpTransportClass, t)) : vm->createValue();
   return 0;
}

/**
 * enableAgent(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableAgent)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_NXCP, true);
}

/**
 * enableConfigurationPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableConfigurationPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_CONF_POLL, true);
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
 * enableSnmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSnmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SNMP, true);
}

/**
 * enableStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_STATUS_POLL, true);
}

/**
 * enableTopologyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableTopologyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_TOPOLOGY_POLL, true);
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
         TCHAR command[MAX_PARAM_NAME], ipAddr[64];
         _sntprintf(command, MAX_PARAM_NAME, _T("SSH.Command(%s:%u,\"%s\",\"%s\",\"%s\",\"\",%d)"),
                    node->getIpAddress().toString(ipAddr), static_cast<uint32_t>(node->getSshPort()),
                    EscapeStringForAgent(node->getSshLogin()).cstr(),
                    EscapeStringForAgent(node->getSshPassword()).cstr(),
                    EscapeStringForAgent(argv[0]->getValueAsCString()).cstr(),
                    node->getSshKeyId());
         StringList *list;
         uint32_t rcc = proxyNode->getListFromAgent(command, &list);
         *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
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
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
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

   Table *table;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getTableFromAgent(argv[0]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, table)) : vm->createValue();
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

   Table *table;
   uint32_t rcc = static_cast<shared_ptr<Node>*>(object->getData())->get()->getInternalTable(argv[0]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, table)) : vm->createValue();
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
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
   delete list;
   return 0;
}

/**
 * Node::setIfXTableUsageMode(mode) method
 */
NXSL_METHOD_DEFINITION(Node, setIfXTableUsageMode)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_STRING;

   int mode = argv[0]->getValueAsInt32();
   if ((mode != IFXTABLE_DISABLED) && (mode != IFXTABLE_ENABLED))
      mode = IFXTABLE_DEFAULT;

   static_cast<shared_ptr<Node>*>(object->getData())->get()->setIfXtableUsageMode(mode);
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class Node: constructor
 */
NXSL_NodeClass::NXSL_NodeClass() : NXSL_DCTargetClass()
{
   setName(_T("Node"));

   NXSL_REGISTER_METHOD(Node, createSNMPTransport, -1);
   NXSL_REGISTER_METHOD(Node, enableAgent, 1);
   NXSL_REGISTER_METHOD(Node, enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableDiscoveryPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableEtherNetIP, 1);
   NXSL_REGISTER_METHOD(Node, enableIcmp, 1);
   NXSL_REGISTER_METHOD(Node, enablePrimaryIPPing, 1);
   NXSL_REGISTER_METHOD(Node, enableRoutingTablePolling, 1);
   NXSL_REGISTER_METHOD(Node, enableSnmp, 1);
   NXSL_REGISTER_METHOD(Node, enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableTopologyPolling, 1);
   NXSL_REGISTER_METHOD(Node, executeSSHCommand, 1);
   NXSL_REGISTER_METHOD(Node, getInterface, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByIndex, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByMACAddress, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceByName, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceName, 1);
   NXSL_REGISTER_METHOD(Node, readAgentList, 1);
   NXSL_REGISTER_METHOD(Node, readAgentParameter, 1);
   NXSL_REGISTER_METHOD(Node, readAgentTable, 1);
   NXSL_REGISTER_METHOD(Node, readDriverParameter, 1);
   NXSL_REGISTER_METHOD(Node, readInternalParameter, 1);
   NXSL_REGISTER_METHOD(Node, readInternalTable, 1);
   NXSL_REGISTER_METHOD(Node, readWebServiceList, 1);
   NXSL_REGISTER_METHOD(Node, readWebServiceParameter, 1);
   NXSL_REGISTER_METHOD(Node, setIfXTableUsageMode, 1);
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
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto node = SharedObjectFromData<Node>(object);
   if (compareAttributeName(attr, "agentCertificateMappingData"))
   {
      value = vm->createValue(node->getAgentCertificateMappingData());
   }
   else if (compareAttributeName(attr, "agentCertificateMappingMethod"))
   {
      value = vm->createValue(static_cast<int32_t>(node->getAgentCertificateMappingMethod()));
   }
   else if (compareAttributeName(attr, "agentCertificateSubject"))
   {
      value = vm->createValue(node->getAgentCertificateSubject());
   }
   else if (compareAttributeName(attr, "agentId"))
   {
      TCHAR buffer[64];
      value = vm->createValue(node->getAgentId().toString(buffer));
   }
   else if (compareAttributeName(attr, "agentVersion"))
   {
      value = vm->createValue(node->getAgentVersion());
   }
   else if (compareAttributeName(attr, "bootTime"))
   {
      value = vm->createValue(static_cast<INT64>(node->getBootTime()));
   }
   else if (compareAttributeName(attr, "bridgeBaseAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStr(node->getBridgeId(), MAC_ADDR_LENGTH, buffer));
   }
   else if (compareAttributeName(attr, "capabilities"))
   {
      value = vm->createValue(node->getCapabilities());
   }
   else if (compareAttributeName(attr, "cipDeviceType"))
   {
      value = vm->createValue(node->getCipDeviceType());
   }
   else if (compareAttributeName(attr, "cipDeviceTypeAsText"))
   {
      value = vm->createValue(CIP_DeviceTypeNameFromCode(node->getCipDeviceType()));
   }
   else if (compareAttributeName(attr, "cipExtendedStatus"))
   {
      value = vm->createValue((node->getCipStatus() & CIP_DEVICE_STATUS_EXTENDED_STATUS_MASK) >> 4);
   }
   else if (compareAttributeName(attr, "cipExtendedStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeExtendedDeviceStatus(node->getCipStatus()));
   }
   else if (compareAttributeName(attr, "cipStatus"))
   {
      value = vm->createValue(node->getCipStatus());
   }
   else if (compareAttributeName(attr, "cipStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeDeviceStatus(node->getCipStatus()));
   }
   else if (compareAttributeName(attr, "cipState"))
   {
      value = vm->createValue(node->getCipState());
   }
   else if (compareAttributeName(attr, "cipStateAsText"))
   {
      value = vm->createValue(CIP_DeviceStateTextFromCode(node->getCipState()));
   }
   else if (compareAttributeName(attr, "cipVendorCode"))
   {
      value = vm->createValue(node->getCipVendorCode());
   }
   else if (compareAttributeName(attr, "components"))
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
   else if (compareAttributeName(attr, "dependentNodes"))
   {
      StructArray<DependentNode> *dependencies = GetNodeDependencies(node->getId());
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < dependencies->size(); i++)
      {
         a->append(vm->createValue(new NXSL_Object(vm, &g_nxslNodeDependencyClass, new DependentNode(*dependencies->get(i)))));
      }
      value = vm->createValue(a);
   }
   else if (compareAttributeName(attr, "driver"))
   {
      value = vm->createValue(node->getDriverName());
   }
   else if (compareAttributeName(attr, "downSince"))
   {
      value = vm->createValue(static_cast<INT64>(node->getDownSince()));
   }
   else if (compareAttributeName(attr, "flags"))
   {
		value = vm->createValue(node->getFlags());
   }
   else if (compareAttributeName(attr, "hasAgentIfXCounters"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_AGENT_IFXCOUNTERS) ? 1 : 0);
   }
   else if (compareAttributeName(attr, "hasEntityMIB"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_ENTITY_MIB) ? 1 : 0);
   }
   else if (compareAttributeName(attr, "hasIfXTable"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_IFXTABLE) ? 1 : 0);
   }
   else if (compareAttributeName(attr, "hasUserAgent"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_HAS_USER_AGENT) ? 1 : 0));
   }
   else if (compareAttributeName(attr, "hasVLANs"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_VLANS) ? 1 : 0);
   }
   else if (compareAttributeName(attr, "hardwareId"))
   {
      TCHAR buffer[HARDWARE_ID_LENGTH * 2 + 1];
      value = vm->createValue(BinToStr(node->getHardwareId().value(), HARDWARE_ID_LENGTH, buffer));
   }
   else if (compareAttributeName(attr, "hasWinPDH"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_WINPDH) ? 1 : 0);
   }
   else if (compareAttributeName(attr, "hypervisorInfo"))
   {
      value = vm->createValue(node->getHypervisorInfo());
   }
   else if (compareAttributeName(attr, "hypervisorType"))
   {
      value = vm->createValue(node->getHypervisorType());
   }
   else if (compareAttributeName(attr, "icmpAverageRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::AVERAGE, vm);
   }
   else if (compareAttributeName(attr, "icmpLastRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LAST, vm);
   }
   else if (compareAttributeName(attr, "icmpMaxRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MAX, vm);
   }
   else if (compareAttributeName(attr, "icmpMinRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MIN, vm);
   }
   else if (compareAttributeName(attr, "icmpPacketLoss"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LOSS, vm);
   }
   else if (compareAttributeName(attr, "interfaces"))
   {
      value = vm->createValue(node->getInterfacesForNXSL(vm));
   }
   else if (compareAttributeName(attr, "isAgent"))
   {
      value = vm->createValue(node->isNativeAgent());
   }
   else if (compareAttributeName(attr, "isBridge"))
   {
      value = vm->createValue(node->isBridge());
   }
   else if (compareAttributeName(attr, "isCDP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_CDP) != 0);
   }
   else if (compareAttributeName(attr, "isEtherNetIP"))
   {
      value = vm->createValue(node->isEthernetIPSupported());
   }
   else if (compareAttributeName(attr, "isInMaintenanceMode"))
   {
      value = vm->createValue(node->isInMaintenanceMode());
   }
   else if (compareAttributeName(attr, "isLLDP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_LLDP) != 0);
   }
	else if (compareAttributeName(attr, "isLocalMgmt") || compareAttributeName(attr, "isLocalManagement"))
	{
		value = vm->createValue(node->isLocalManagement());
	}
   else if (compareAttributeName(attr, "isModbusTCP"))
   {
      value = vm->createValue(node->isModbusTCPSupported());
   }
   else if (compareAttributeName(attr, "isOSPF"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_OSPF) != 0);
   }
   else if (compareAttributeName(attr, "isPAE") || compareAttributeName(attr, "is802_1x"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_8021X) != 0);
   }
   else if (compareAttributeName(attr, "isPrinter"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_PRINTER) != 0);
   }
   else if (compareAttributeName(attr, "isProfiNet"))
   {
      value = vm->createValue(node->isProfiNetSupported());
   }
   else if (compareAttributeName(attr, "isRemotelyManaged") || compareAttributeName(attr, "isExternalGateway"))
   {
      value = vm->createValue((node->getFlags() & NF_EXTERNAL_GATEWAY) != 0);
   }
   else if (compareAttributeName(attr, "isRouter"))
   {
      value = vm->createValue(node->isRouter());
   }
   else if (compareAttributeName(attr, "isSMCLP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_SMCLP) != 0);
   }
   else if (compareAttributeName(attr, "isSNMP"))
   {
      value = vm->createValue(node->isSNMPSupported());
   }
   else if (compareAttributeName(attr, "isSONMP") || compareAttributeName(attr, "isNDP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_NDP) != 0);
   }
   else if (compareAttributeName(attr, "isSTP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_STP) != 0);
   }
   else if (compareAttributeName(attr, "isVirtual"))
   {
      value = vm->createValue(node->isVirtual());
   }
   else if (compareAttributeName(attr, "isVRRP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_VRRP) != 0);
   }
   else if (compareAttributeName(attr, "lastAgentCommTime"))
   {
      value = vm->createValue(static_cast<int64_t>(node->getLastAgentCommTime()));
   }
   else if (compareAttributeName(attr, "nodeSubType"))
   {
      value = vm->createValue(node->getSubType());
   }
   else if (compareAttributeName(attr, "nodeType"))
   {
      value = vm->createValue((INT32)node->getType());
   }
   else if (compareAttributeName(attr, "physicalContainer"))
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
   else if (compareAttributeName(attr, "physicalContainerId"))
   {
      value = vm->createValue(node->getPhysicalContainerId());
   }
   else if (compareAttributeName(attr, "platformName"))
   {
      value = vm->createValue(node->getPlatformName());
   }
   else if (compareAttributeName(attr, "primaryHostName"))
   {
      value = vm->createValue(node->getPrimaryHostName());
   }
   else if (compareAttributeName(attr, "productCode"))
   {
      value = vm->createValue(node->getProductCode());
   }
   else if (compareAttributeName(attr, "productName"))
   {
      value = vm->createValue(node->getProductName());
   }
   else if (compareAttributeName(attr, "productVersion"))
   {
      value = vm->createValue(node->getProductVersion());
   }
   else if (compareAttributeName(attr, "rack"))
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
   else if (compareAttributeName(attr, "rackId"))
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
   else if (compareAttributeName(attr, "rackHeight"))
   {
      value = vm->createValue(node->getRackHeight());
   }
   else if (compareAttributeName(attr, "rackPosition"))
   {
      value = vm->createValue(node->getRackPosition());
   }
   else if (compareAttributeName(attr, "runtimeFlags"))
   {
      value = vm->createValue(node->getRuntimeFlags());
   }
   else if (compareAttributeName(attr, "serialNumber"))
   {
      value = vm->createValue(node->getSerialNumber());
   }
   else if (compareAttributeName(attr, "snmpOID"))
   {
      value = vm->createValue(node->getSNMPObjectId());
   }
   else if (compareAttributeName(attr, "snmpSysContact"))
   {
      value = vm->createValue(node->getSysContact());
   }
   else if (compareAttributeName(attr, "snmpSysLocation"))
   {
      value = vm->createValue(node->getSysLocation());
   }
   else if (compareAttributeName(attr, "snmpSysName"))
   {
      value = vm->createValue(node->getSysName());
   }
   else if (compareAttributeName(attr, "snmpVersion"))
   {
      value = vm->createValue((LONG)node->getSNMPVersion());
   }
   else if (compareAttributeName(attr, "sysDescription"))
   {
      value = vm->createValue(node->getSysDescription());
   }
   else if (compareAttributeName(attr, "vendor"))
   {
      value = vm->createValue(node->getVendor());
   }
   else if (compareAttributeName(attr, "vlans"))
   {
      shared_ptr<VlanList> vlans = node->getVlans();
      if (vlans != nullptr)
      {
         NXSL_Array *a = new NXSL_Array(vm);
         for(int i = 0; i < vlans->size(); i++)
         {
            a->append(vm->createValue(new NXSL_Object(vm, &g_nxslVlanClass, new VlanInfo(vlans->get(i), node->getId()))));
         }
         value = vm->createValue(a);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (compareAttributeName(attr, "zone"))
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
   else if (compareAttributeName(attr, "zoneProxyAssignments"))
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
   else if (compareAttributeName(attr, "zoneProxyStatus"))
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
   else if (compareAttributeName(attr, "zoneUIN"))
	{
      value = vm->createValue(node->getZoneUIN());
   }
   return value;
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
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

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
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = static_cast<shared_ptr<Interface>*>(object->getData())->get();
   iface->setIncludeInIcmpPoll(argv[0]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class Interface: constructor
 */
NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_NetObjClass()
{
   setName(_T("Interface"));

   NXSL_REGISTER_METHOD(Interface, enableAgentStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, enableICMPStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, enableSNMPStatusPolling, 1);
   NXSL_REGISTER_METHOD(Interface, setExcludeFromTopology, 1);
   NXSL_REGISTER_METHOD(Interface, setExpectedState, 1);
   NXSL_REGISTER_METHOD(Interface, setIncludeInIcmpPoll, 1);
}

/**
 * NXSL class Interface: get attribute
 */
NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto iface = SharedObjectFromData<Interface>(object);
   if (compareAttributeName(attr, "adminState"))
   {
		value = vm->createValue((LONG)iface->getAdminState());
   }
   else if (compareAttributeName(attr, "bridgePortNumber"))
   {
		value = vm->createValue(iface->getBridgePortNumber());
   }
   else if (compareAttributeName(attr, "chassis"))
   {
      value = vm->createValue(iface->getChassis());
   }
   else if (compareAttributeName(attr, "description"))
   {
      value = vm->createValue(iface->getDescription());
   }
   else if (compareAttributeName(attr, "dot1xBackendAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xBackendAuthState());
   }
   else if (compareAttributeName(attr, "dot1xPaeAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xPaeAuthState());
   }
   else if (compareAttributeName(attr, "expectedState"))
   {
		value = vm->createValue((iface->getFlags() & IF_EXPECTED_STATE_MASK) >> 28);
   }
   else if (compareAttributeName(attr, "flags"))
   {
		value = vm->createValue(iface->getFlags());
   }
   else if (compareAttributeName(attr, "icmpAverageRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::AVERAGE, vm);
   }
   else if (compareAttributeName(attr, "icmpLastRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::LAST, vm);
   }
   else if (compareAttributeName(attr, "icmpMaxRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::MAX, vm);
   }
   else if (compareAttributeName(attr, "icmpMinRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::MIN, vm);
   }
   else if (compareAttributeName(attr, "icmpPacketLoss"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::LOSS, vm);
   }
   else if (compareAttributeName(attr, "ifIndex"))
   {
		value = vm->createValue(iface->getIfIndex());
   }
   else if (compareAttributeName(attr, "ifType"))
   {
		value = vm->createValue(iface->getIfType());
   }
   else if (compareAttributeName(attr, "ipAddressList"))
   {
      const InetAddressList *addrList = iface->getIpAddressList();
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < addrList->size(); i++)
      {
         a->append(NXSL_InetAddressClass::createObject(vm, addrList->get(i)));
      }
      value = vm->createValue(a);
   }
   else if (compareAttributeName(attr, "isExcludedFromTopology"))
   {
      value = vm->createValue((LONG)(iface->isExcludedFromTopology() ? 1 : 0));
   }
   else if (compareAttributeName(attr, "isIncludedInIcmpPoll"))
   {
      value = vm->createValue((LONG)(iface->isIncludedInIcmpPoll() ? 1 : 0));
   }
   else if (compareAttributeName(attr, "isLoopback"))
   {
		value = vm->createValue((LONG)(iface->isLoopback() ? 1 : 0));
   }
   else if (compareAttributeName(attr, "isManuallyCreated"))
   {
		value = vm->createValue((LONG)(iface->isManuallyCreated() ? 1 : 0));
   }
   else if (compareAttributeName(attr, "isPhysicalPort"))
   {
		value = vm->createValue((LONG)(iface->isPhysicalPort() ? 1 : 0));
   }
   else if (compareAttributeName(attr, "macAddr"))
   {
		TCHAR buffer[256];
		value = vm->createValue(iface->getMacAddr().toString(buffer));
   }
   else if (compareAttributeName(attr, "module"))
   {
      value = vm->createValue(iface->getModule());
   }
   else if (compareAttributeName(attr, "mtu"))
   {
      value = vm->createValue(iface->getMTU());
   }
   else if (compareAttributeName(attr, "node"))
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
   else if (compareAttributeName(attr, "operState"))
   {
		value = vm->createValue((LONG)iface->getOperState());
   }
   else if (compareAttributeName(attr, "peerInterface"))
   {
      shared_ptr<NetObj> peerIface = FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
		if (peerIface != nullptr)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
			   shared_ptr<Node> parentNode = iface->getParentNode();
				shared_ptr<Node> peerNode = static_cast<Interface*>(peerIface.get())->getParentNode();
				if ((parentNode != nullptr) && (peerNode != nullptr))
				{
					if (peerNode->isTrustedNode(parentNode->getId()))
					{
						value = peerIface->createNXSLObject(vm);
					}
					else
					{
						// No access, return null
						value = vm->createValue();
						DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): access denied for node %s [%d]"),
									 iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
					}
				}
				else
				{
					value = vm->createValue();
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=% [%u] peerNode=%s [%u]"),
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
   else if (compareAttributeName(attr, "peerNode"))
   {
      shared_ptr<NetObj> peerNode = FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if (peerNode != nullptr)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
			   shared_ptr<Node> parentNode = iface->getParentNode();
				if ((parentNode != nullptr) && (peerNode->isTrustedNode(parentNode->getId())))
				{
					value = peerNode->createNXSLObject(vm);
				}
				else
				{
					// No access, return null
					value = vm->createValue();
					DbgPrintf(4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
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
   else if (compareAttributeName(attr, "pic"))
   {
      value = vm->createValue(iface->getPIC());
   }
   else if (compareAttributeName(attr, "port"))
   {
      value = vm->createValue(iface->getPort());
   }
   else if (compareAttributeName(attr, "speed"))
   {
      value = vm->createValue(iface->getSpeed());
   }
   else if (compareAttributeName(attr, "vlans"))
   {
      value = iface->getVlanListForNXSL(vm);
   }
   else if (compareAttributeName(attr, "zone"))
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
   else if (compareAttributeName(attr, "zoneUIN"))
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
NXSL_Value *NXSL_AccessPointClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto ap = SharedObjectFromData<AccessPoint>(object);
   if (compareAttributeName(attr, "icmpAverageRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::AVERAGE, vm);
   }
   else if (compareAttributeName(attr, "icmpLastRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::LAST, vm);
   }
   else if (compareAttributeName(attr, "icmpMaxRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::MAX, vm);
   }
   else if (compareAttributeName(attr, "icmpMinRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::MIN, vm);
   }
   else if (compareAttributeName(attr, "icmpPacketLoss"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::LOSS, vm);
   }
   else if (compareAttributeName(attr, "index"))
   {
      value = vm->createValue(ap->getIndex());
   }
   else if (compareAttributeName(attr, "model"))
   {
      value = vm->createValue(ap->getModel());
   }
   else if (compareAttributeName(attr, "node"))
   {
      shared_ptr<Node> parentNode = ap->getParentNode();
      if (parentNode != nullptr)
      {
         value = parentNode->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (compareAttributeName(attr, "serialNumber"))
   {
      value = vm->createValue(ap->getSerialNumber());
   }
   else if (compareAttributeName(attr, "state"))
   {
      value = vm->createValue(ap->getApState());
   }
   else if (compareAttributeName(attr, "vendor"))
   {
      value = vm->createValue(ap->getVendor());
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
NXSL_Value *NXSL_MobileDeviceClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto mobileDevice = SharedObjectFromData<MobileDevice>(object);
   if (compareAttributeName(attr, "altitude"))
   {
		value = vm->createValue(mobileDevice->getAltitude());
   }
   else if (compareAttributeName(attr, "batteryLevel"))
   {
      value = vm->createValue(mobileDevice->getBatteryLevel());
   }
   else if (compareAttributeName(attr, "commProtocol"))
   {
      value = vm->createValue(mobileDevice->getCommProtocol());
   }
   else if (compareAttributeName(attr, "deviceId"))
   {
      value = vm->createValue(mobileDevice->getDeviceId());
   }
   else if (compareAttributeName(attr, "direction"))
   {
      value = vm->createValue(mobileDevice->getDirection());
   }
   else if (compareAttributeName(attr, "lastReportTime"))
   {
      value = vm->createValue(static_cast<int64_t>(mobileDevice->getLastReportTime()));
   }
   else if (compareAttributeName(attr, "model"))
   {
      value = vm->createValue(mobileDevice->getModel());
   }
   else if (compareAttributeName(attr, "osName"))
   {
      value = vm->createValue(mobileDevice->getOsName());
   }
   else if (compareAttributeName(attr, "osVersion"))
   {
      value = vm->createValue(mobileDevice->getOsVersion());
   }
   else if (compareAttributeName(attr, "serialNumber"))
   {
      value = vm->createValue(mobileDevice->getSerialNumber());
   }
   else if (compareAttributeName(attr, "speed"))
   {
      value = vm->createValue(mobileDevice->getSpeed());
   }
   else if (compareAttributeName(attr, "userId"))
   {
      value = vm->createValue(mobileDevice->getUserId());
   }
   else if (compareAttributeName(attr, "vendor"))
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
NXSL_Value *NXSL_ChassisClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto chassis = SharedObjectFromData<Chassis>(object);
   if (compareAttributeName(attr, "controller"))
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
   else if (compareAttributeName(attr, "controllerId"))
   {
      value = vm->createValue(chassis->getControllerId());
   }
   else if (compareAttributeName(attr, "flags"))
   {
      value = vm->createValue(chassis->getFlags());
   }
   else if (compareAttributeName(attr, "rack"))
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
   else if (compareAttributeName(attr, "rackId"))
   {
      value = vm->createValue(chassis->getRackId());
   }
   else if (compareAttributeName(attr, "rackHeight"))
   {
      value = vm->createValue(chassis->getRackHeight());
   }
   else if (compareAttributeName(attr, "rackPosition"))
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
   thisObject->addNode(static_pointer_cast<Node>(child));

   *result = vm->createValue();
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
   thisObject->deleteChild(*child);
   child->deleteParent(*thisObject);
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
NXSL_Value *NXSL_ClusterClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto cluster = SharedObjectFromData<Cluster>(object);
   if (compareAttributeName(attr, "nodes"))
   {
      value = vm->createValue(cluster->getNodesForNXSL(vm));
   }
   else if (compareAttributeName(attr, "zone"))
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
   else if (compareAttributeName(attr, "zoneUIN"))
   {
      value = vm->createValue(cluster->getZoneUIN());
   }
   return value;
}

/**
 * Container::setAutoBindMode() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindMode)
{
   if (!argv[0]->isInteger() || !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   static_cast<shared_ptr<Container>*>(object->getData())->get()->setAutoBindMode(argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
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

   static_cast<shared_ptr<Container>*>(object->getData())->get()->setAutoBindFilter(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Container" constructor
 */
NXSL_ContainerClass::NXSL_ContainerClass() : NXSL_NetObjClass()
{
   setName(_T("Container"));

   NXSL_REGISTER_METHOD(Container, setAutoBindMode, 2);
   NXSL_REGISTER_METHOD(Container, setAutoBindScript, 1);
}

/**
 * NXSL class "Container" attributes
 */
NXSL_Value *NXSL_ContainerClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto container = SharedObjectFromData<Container>(object);
   if (compareAttributeName(attr, "autoBindScript"))
   {
      const TCHAR *script = container->getAutoBindScriptSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (compareAttributeName(attr, "isAutoBindEnabled"))
   {
      value = vm->createValue(container->isAutoBindEnabled());
   }
   else if (compareAttributeName(attr, "isAutoUnbindEnabled"))
   {
      value = vm->createValue(container->isAutoUnbindEnabled());
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
   shared_ptr<Template> thisObject = *static_cast<shared_ptr<Template>*>(object->getData());

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *nxslTarget = argv[0]->getValueAsObject();
   if (!nxslTarget->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   auto target = *static_cast<shared_ptr<DataCollectionTarget>*>(nxslTarget->getData());
   thisObject->deleteChild(*target);
   target->deleteParent(*thisObject);
   thisObject->queueRemoveFromTarget(target->getId(), true);

   *result = vm->createValue();
   return 0;
}

/**
 * Template::setAutoApplyMode() method
 */
NXSL_METHOD_DEFINITION(Template, setAutoApplyMode)
{
   if (!argv[0]->isInteger() || !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   static_cast<shared_ptr<Template>*>(object->getData())->get()->setAutoBindMode(argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
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

   static_cast<shared_ptr<Template>*>(object->getData())->get()->setAutoBindFilter(argv[0]->getValueAsCString());
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
NXSL_Value *NXSL_TemplateClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto tmpl = SharedObjectFromData<Template>(object);
   if (compareAttributeName(attr, "autoApplyScript"))
   {
      const TCHAR *script = tmpl->getAutoBindScriptSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (compareAttributeName(attr, "isAutoApplyEnabled"))
   {
      value = vm->createValue(tmpl->isAutoBindEnabled());
   }
   else if (compareAttributeName(attr, "isAutoRemoveEnabled"))
   {
      value = vm->createValue(tmpl->isAutoUnbindEnabled());
   }
   else if (compareAttributeName(attr, "version"))
   {
      value = vm->createValue(tmpl->getVersion());
   }
   return value;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_TunnelClass::NXSL_TunnelClass() : NXSL_Class()
{
   setName(_T("Tunnel"));
}

/**
 * NXSL object creation handler
 */
void NXSL_TunnelClass::onObjectCreate(NXSL_Object *object)
{
   static_cast<AgentTunnel*>(object->getData())->incRefCount();
}

/**
 * NXSL object destructor
 */
void NXSL_TunnelClass::onObjectDelete(NXSL_Object *object)
{
   static_cast<AgentTunnel*>(object->getData())->decRefCount();
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_TunnelClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   AgentTunnel *tunnel = static_cast<AgentTunnel*>(object->getData());

   if (compareAttributeName(attr, "address"))
   {
      value = vm->createValue(NXSL_InetAddressClass::createObject(vm, tunnel->getAddress()));
   }
   else if (compareAttributeName(attr, "agentBuildTag"))
   {
      value = vm->createValue(tunnel->getAgentBuildTag());
   }
   else if (compareAttributeName(attr, "agentId"))
   {
      value = vm->createValue(tunnel->getAgentId().toString());
   }
   else if (compareAttributeName(attr, "agentVersion"))
   {
      value = vm->createValue(tunnel->getAgentVersion());
   }
   else if (compareAttributeName(attr, "certificateExpirationTime"))
   {
      value = vm->createValue(static_cast<int64_t>(tunnel->getCertificateExpirationTime()));
   }
   else if (compareAttributeName(attr, "guid"))
   {
      value = vm->createValue(tunnel->getGUID().toString());
   }
   else if (compareAttributeName(attr, "hostname"))
   {
      value = vm->createValue(tunnel->getHostname());
   }
   else if (compareAttributeName(attr, "hardwareId"))
   {
      value = vm->createValue(tunnel->getHostname());
   }
   else if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(tunnel->getId());
   }
   else if (compareAttributeName(attr, "isAgentProxy"))
   {
      value = vm->createValue(tunnel->isAgentProxy());
   }
   else if (compareAttributeName(attr, "isBound"))
   {
      value = vm->createValue(tunnel->isBound());
   }
   else if (compareAttributeName(attr, "isSnmpProxy"))
   {
      value = vm->createValue(tunnel->isSnmpProxy());
   }
   else if (compareAttributeName(attr, "isSnmpTrapProxy"))
   {
      value = vm->createValue(tunnel->isSnmpTrapProxy());
   }
   else if (compareAttributeName(attr, "isUserAgentInstalled"))
   {
      value = vm->createValue(tunnel->isUserAgentInstalled());
   }
   else if (compareAttributeName(attr, "platformName"))
   {
      value = vm->createValue(tunnel->getPlatformName());
   }
   else if (compareAttributeName(attr, "startTime"))
   {
      value = vm->createValue(static_cast<int64_t>(tunnel->getStartTime()));
   }
   else if (compareAttributeName(attr, "systemInfo"))
   {
      value = vm->createValue(tunnel->getSystemInfo());
   }
   else if (compareAttributeName(attr, "systemName"))
   {
      value = vm->createValue(tunnel->getSystemName());
   }
   else if (compareAttributeName(attr, "zoneUIN"))
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
      return NXSL_ERR_NOT_STRING;

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
 * Event::toJson() method
 */
NXSL_METHOD_DEFINITION(Event, toJson)
{
   Event *event = static_cast<Event*>(object->getData());
   json_t *json = event->toJson();
   char *text = json_dumps(json, JSON_INDENT(3) | JSON_EMBED);
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
   NXSL_REGISTER_METHOD(Event, hasTag, 1);
   NXSL_REGISTER_METHOD(Event, removeTag, 1);
   NXSL_REGISTER_METHOD(Event, setMessage, 1);
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
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   const Event *event = static_cast<Event*>(object->getData());
   if (compareAttributeName(attr, "code"))
   {
      value = vm->createValue(event->getCode());
   }
   else if (compareAttributeName(attr, "customMessage"))
   {
      value = vm->createValue(event->getCustomMessage());
   }
   else if (compareAttributeName(attr, "dci"))
   {
      UINT32 dciId = event->getDciId();
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
   else if (compareAttributeName(attr, "dciId"))
   {
      value = vm->createValue(event->getDciId());
   }
   else if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(event->getId());
   }
   else if (compareAttributeName(attr, "message"))
   {
      value = vm->createValue(event->getMessage());
   }
   else if (compareAttributeName(attr, "name"))
   {
		value = vm->createValue(event->getName());
   }
   else if (compareAttributeName(attr, "origin"))
   {
      value = vm->createValue(static_cast<INT32>(event->getOrigin()));
   }
   else if (compareAttributeName(attr, "originTimestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getOriginTimestamp()));
   }
   else if (compareAttributeName(attr, "parameters"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameter(i)));
      value = vm->createValue(array);
   }
   else if (compareAttributeName(attr, "parameterNames"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameterName(i)));
      value = vm->createValue(array);
   }
   else if (compareAttributeName(attr, "rootId"))
   {
      value = vm->createValue(event->getRootId());
   }
   else if (compareAttributeName(attr, "severity"))
   {
      value = vm->createValue(event->getSeverity());
   }
   else if (compareAttributeName(attr, "source"))
   {
      shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
      value = (object != nullptr) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (compareAttributeName(attr, "sourceId"))
   {
      value = vm->createValue(event->getSourceId());
   }
   else if (compareAttributeName(attr, "tagList"))
   {
      value = vm->createValue(event->getTagsAsList());
   }
   else if (compareAttributeName(attr, "tags"))
   {
      StringList *tags = String(event->getTagsAsList()).split(_T(","));
      value = vm->createValue(new NXSL_Array(vm, tags));
      delete tags;
   }
   else if (compareAttributeName(attr, "timestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getTimestamp()));
   }
   else if (event != nullptr)    // Event can be null if getAttr called by attribute scan
   {
      if (attr[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr[1], &eptr, 10);
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
#ifdef UNICODE
         WCHAR wattr[MAX_IDENTIFIER_LENGTH];
         MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         const TCHAR *s = event->getNamedParameter(wattr);
#else
         const TCHAR *s = event->getNamedParameter(attr);
#endif
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
bool NXSL_EventClass::setAttr(NXSL_Object *object, const char *attr, NXSL_Value *value)
{
   Event *event = static_cast<Event*>(object->getData());
   if (!strcmp(attr, "customMessage"))
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
   else if (!strcmp(attr, "message"))
   {
      if (value->isString())
      {
         event->setMessage(value->getValueAsCString());
      }
   }
   else if (!strcmp(attr, "severity"))
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
      if (attr[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *name = event->getParameterName(index - 1);
            event->setParameter(index - 1, CHECK_NULL_EX(name), value->getValueAsCString());
            success = true;
         }
      }
      if (!success)
      {
#ifdef UNICODE
         WCHAR wattr[MAX_IDENTIFIER_LENGTH];
         MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         event->setNamedParameter(wattr, value->getValueAsCString());
#else
         event->setNamedParameter(attr, value->getValueAsCString());
#endif
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
      if(!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;
      else
         syncWithHelpdesk = argv[1]->getValueAsBoolean();
   }

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   UINT32 id = 0;
   UINT32 rcc = UpdateAlarmComment(alarm->getAlarmId(), &id, argv[0]->getValueAsCString(), 0, syncWithHelpdesk);
   if(rcc != RCC_SUCCESS)
      return NXSL_ERR_INTERNAL;

   *result = vm->createValue(id);
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
      array->append(vm->createValue(new NXSL_Object(vm, &g_nxslAlarmCommentClass, alarmComments->get(i))));
   }
   delete alarmComments;
   *result = vm->createValue(array);
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
NXSL_Value *NXSL_AlarmClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   Alarm *alarm = static_cast<Alarm*>(object->getData());

   if (compareAttributeName(attr, "ackBy"))
   {
      value = vm->createValue(alarm->getAckByUser());
   }
   else if (compareAttributeName(attr, "creationTime"))
   {
      value = vm->createValue((INT64)alarm->getCreationTime());
   }
   else if (compareAttributeName(attr, "dciId"))
   {
      value = vm->createValue(alarm->getDciId());
   }
   else if (compareAttributeName(attr, "eventCode"))
   {
      value = vm->createValue(alarm->getSourceEventCode());
   }
   else if (compareAttributeName(attr, "eventId"))
   {
      value = vm->createValue(alarm->getSourceEventId());
   }
   else if (compareAttributeName(attr, "eventTagList"))
   {
      value = vm->createValue(alarm->getEventTags());
   }
   else if (compareAttributeName(attr, "helpdeskReference"))
   {
      value = vm->createValue(alarm->getHelpDeskRef());
   }
   else if (compareAttributeName(attr, "helpdeskState"))
   {
      value = vm->createValue(alarm->getHelpDeskState());
   }
   else if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(alarm->getAlarmId());
   }
   else if (compareAttributeName(attr, "impact"))
   {
      value = vm->createValue(alarm->getImpact());
   }
   else if (compareAttributeName(attr, "key"))
   {
      value = vm->createValue(alarm->getKey());
   }
   else if (compareAttributeName(attr, "lastChangeTime"))
   {
      value = vm->createValue((INT64)alarm->getLastChangeTime());
   }
   else if (compareAttributeName(attr, "message"))
   {
      value = vm->createValue(alarm->getMessage());
   }
   else if (compareAttributeName(attr, "originalSeverity"))
   {
      value = vm->createValue(alarm->getOriginalSeverity());
   }
   else if (compareAttributeName(attr, "parentId"))
   {
      value = vm->createValue(alarm->getParentAlarmId());
   }
   else if (compareAttributeName(attr, "repeatCount"))
   {
      value = vm->createValue(alarm->getRepeatCount());
   }
   else if (compareAttributeName(attr, "resolvedBy"))
   {
      value = vm->createValue(alarm->getResolvedByUser());
   }
   else if (compareAttributeName(attr, "rcaScriptName"))
   {
      value = vm->createValue(alarm->getRcaScriptName());
   }
   else if (compareAttributeName(attr, "ruleGuid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(alarm->getRule().toString(buffer));
   }
   else if (compareAttributeName(attr, "severity"))
   {
      value = vm->createValue(alarm->getCurrentSeverity());
   }
   else if (compareAttributeName(attr, "sourceObject"))
   {
      value = vm->createValue(alarm->getSourceObject());
   }
   else if (compareAttributeName(attr, "state"))
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
NXSL_Value *NXSL_AlarmCommentClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   AlarmComment *alarmComment = static_cast<AlarmComment*>(object->getData());

   if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(alarmComment->getId());
   }
   else if (compareAttributeName(attr, "changeTime"))
   {
      value = vm->createValue((INT64)alarmComment->getChangeTime());
   }
   else if (compareAttributeName(attr, "userId"))
   {
      value = vm->createValue(alarmComment->getUserId());
   }
   else if (compareAttributeName(attr, "text"))
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
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   setName(_T("DCI"));

   NXSL_REGISTER_METHOD(DCI, forcePoll, 0);
}

/**
 * Object destructor
 */
void NXSL_DciClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<shared_ptr<DCObjectInfo>*>(object->getData());
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   const DCObjectInfo *dci = SharedObjectFromData<DCObjectInfo>(object);
   if (compareAttributeName(attr, "activeThresholdSeverity"))
   {
      value = vm->createValue(dci->getThresholdSeverity());
   }
   else if (compareAttributeName(attr, "comments"))
   {
		value = vm->createValue(dci->getComments());
   }
   else if (compareAttributeName(attr, "dataType") && (dci->getType() == DCO_TYPE_ITEM))
   {
		value = vm->createValue(dci->getDataType());
   }
   else if (compareAttributeName(attr, "description"))
   {
		value = vm->createValue(dci->getDescription());
   }
   else if (compareAttributeName(attr, "errorCount"))
   {
		value = vm->createValue(dci->getErrorCount());
   }
   else if (compareAttributeName(attr, "hasActiveThreshold"))
   {
      value = vm->createValue(dci->hasActiveThreshold());
   }
   else if (compareAttributeName(attr, "id"))
   {
		value = vm->createValue(dci->getId());
   }
   else if (compareAttributeName(attr, "instance"))
   {
		value = vm->createValue(dci->getInstance());
   }
   else if (compareAttributeName(attr, "instanceData"))
   {
		value = vm->createValue(dci->getInstanceData());
   }
   else if (compareAttributeName(attr, "lastPollTime"))
   {
		value = vm->createValue((INT64)dci->getLastPollTime());
   }
   else if (compareAttributeName(attr, "name"))
   {
		value = vm->createValue(dci->getName());
   }
   else if (compareAttributeName(attr, "origin"))
   {
		value = vm->createValue((LONG)dci->getOrigin());
   }
   else if (compareAttributeName(attr, "pollingInterval"))
   {
      value = vm->createValue(dci->getPollingInterval());
   }
   else if (compareAttributeName(attr, "relatedObject"))
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
   else if (compareAttributeName(attr, "status"))
   {
		value = vm->createValue((LONG)dci->getStatus());
   }
   else if (compareAttributeName(attr, "systemTag"))
   {
		value = vm->createValue(dci->getSystemTag());
   }
   else if (compareAttributeName(attr, "template"))
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
   else if (compareAttributeName(attr, "templateId"))
   {
      value = vm->createValue(dci->getTemplateId());
   }
   else if (compareAttributeName(attr, "templateItemId"))
   {
      value = vm->createValue(dci->getTemplateItemId());
   }
   else if (compareAttributeName(attr, "type"))
   {
		value = vm->createValue((LONG)dci->getType());
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
NXSL_Value *NXSL_SensorClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   auto sensor = SharedObjectFromData<Sensor>(object);
   if (compareAttributeName(attr, "description"))
   {
      value = vm->createValue(sensor->getDescription());
   }
   else if (compareAttributeName(attr, "frameCount"))
   {
      value = vm->createValue(sensor->getFrameCount());
   }
   else if (compareAttributeName(attr, "lastContact"))
   {
      value = vm->createValue((UINT32)sensor->getLastContact());
   }
   else if (compareAttributeName(attr, "metaType"))
   {
      value = vm->createValue(sensor->getMetaType());
   }
   else if (compareAttributeName(attr, "protocol"))
   {
      value = vm->createValue(sensor->getCommProtocol());
   }
   else if (compareAttributeName(attr, "serial"))
   {
      value = vm->createValue(sensor->getSerialNumber());
   }
   else if (compareAttributeName(attr, "type"))
   {
      value = vm->createValue(sensor->getSensorClass());
   }
   else if (compareAttributeName(attr, "vendor"))
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
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   // Create PDU and send request
   UINT32 oid[MAX_OID_LEN];
   size_t olen = SNMPParseOID(argv[0]->getValueAsCString(), oid, MAX_OID_LEN);
   if (olen == 0)
      return NXSL_ERR_BAD_CONDITION;

   SNMP_PDU *pdu = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   pdu->bindVariable(new SNMP_Variable(oid, olen));

   SNMP_PDU *rspPDU;
   UINT32 rc = transport->doRequest(pdu, &rspPDU, SnmpGetDefaultTimeout(), 3);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if ((rspPDU->getNumVariables() > 0) && (rspPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         SNMP_Variable *pVar = rspPDU->getVariable(0);
         *result = vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, pVar));
         rspPDU->unlinkVariables();
      }
      else
      {
         *result = vm->createValue();
      }
      delete rspPDU;
   }
   else
   {
      *result = vm->createValue();
   }
   delete pdu;
   return 0;
}

/**
 * SNMPTransport::getValue() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, getValue)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   TCHAR buffer[4096];
   if (SnmpGetEx(transport, argv[0]->getValueAsCString(), nullptr, 0, buffer, sizeof(buffer), SG_STRING_RESULT, nullptr) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(buffer);
   }
   else
   {
      *result = vm->createValue();
   }

   return 0;
}

/**
 * SNMPTransport::set() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, set)
{
   if (argc < 2 || argc > 3)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isString() || !argv[1]->isString() || ((argc == 3) && !argv[2]->isString()))
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU *request = new SNMP_PDU(SNMP_SET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   SNMP_PDU *response = nullptr;
   bool success = false;

   if (SNMPIsCorrectOID(argv[0]->getValueAsCString()))
   {
      SNMP_Variable *var = new SNMP_Variable(argv[0]->getValueAsCString());
      if (argc == 2)
      {
         var->setValueFromString(ASN_OCTET_STRING, argv[1]->getValueAsCString());
      }
      else
      {
         UINT32 dataType = SNMPResolveDataType(argv[2]->getValueAsCString());
         if (dataType == ASN_NULL)
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: failed to resolve data type '%s', assume string"),
               argv[2]->getValueAsCString());
            dataType = ASN_OCTET_STRING;
         }
         var->setValueFromString(dataType, argv[1]->getValueAsCString());
      }
      request->bindVariable(var);
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: Invalid OID: %s"), argv[0]->getValueAsCString());
      goto finish;
   }

   // Send request and process response
   UINT32 snmpResult;
   if ((snmpResult = transport->doRequest(request, &response, SnmpGetDefaultTimeout(), 3)) == SNMP_ERR_SUCCESS)
   {
      if (response->getErrorCode() != 0)
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: operation failed (error code %d)"), response->getErrorCode());
         goto finish;
      }
      else
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: success"));
         success = true;
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: %s"), SNMPGetErrorText(snmpResult));
   }

finish:
   delete request;
   *result = vm->createValue(success);
   return 0;
}

/**
 * SNMP walk callback
 */
static UINT32 WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   NXSL_VM *vm = static_cast<NXSL_VM*>(static_cast<NXSL_Array*>(context)->vm());
   static_cast<NXSL_Array*>(context)->append(vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, new SNMP_Variable(var))));
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
   NXSL_REGISTER_METHOD(SNMPTransport, getValue, 1);
   NXSL_REGISTER_METHOD(SNMPTransport, set, -1);
   NXSL_REGISTER_METHOD(SNMPTransport, walk, 1);
}

/**
 * Implementation of "SNMP_Transport" class: get attribute
 */
NXSL_Value *NXSL_SNMPTransportClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

	SNMP_Transport *t = static_cast<SNMP_Transport*>(object->getData());

	if (compareAttributeName(attr, "snmpVersion"))
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
	delete (SNMP_Transport *)object->getData();
}

/**
 * NXSL class SNMP_VarBind: constructor
 */
NXSL_SNMPVarBindClass::NXSL_SNMPVarBindClass() : NXSL_Class()
{
	setName(_T("SNMP_VarBind"));
}

/**
 * NXSL class SNMP_VarBind: get attribute
 */
NXSL_Value *NXSL_SNMPVarBindClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
	SNMP_Variable *t = static_cast<SNMP_Variable*>(object->getData());
	if (compareAttributeName(attr, "type"))
	{
		value = vm->createValue((UINT32)t->getType());
	}
	else if (compareAttributeName(attr, "name"))
	{
		value = vm->createValue(t->getName().toString());
	}
	else if (compareAttributeName(attr, "value"))
	{
   	TCHAR strValue[1024];
		value = vm->createValue(t->getValueAsString(strValue, 1024));
	}
	else if (compareAttributeName(attr, "printableValue"))
	{
   	TCHAR strValue[1024];
		bool convToHex = true;
		t->getValueAsPrintableString(strValue, 1024, &convToHex);
		value = vm->createValue(strValue);
	}
	else if (compareAttributeName(attr, "valueAsIp"))
	{
   	TCHAR strValue[128];
		t->getValueAsIPAddr(strValue);
		value = vm->createValue(strValue);
	}
	else if (compareAttributeName(attr, "valueAsMac"))
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
 * NXSL class UserDBObjectClass: constructor
 */
NXSL_UserDBObjectClass::NXSL_UserDBObjectClass() : NXSL_Class()
{
   setName(_T("UserDBObject"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserDBObjectClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   UserDatabaseObject *dbObject = static_cast<UserDatabaseObject*>(object->getData());

   if (compareAttributeName(attr, "description"))
   {
      value = vm->createValue(dbObject->getDescription());
   }
   else if (compareAttributeName(attr, "flags"))
   {
      value = vm->createValue(dbObject->getFlags());
   }
   else if (compareAttributeName(attr, "guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(dbObject->getGuidAsText(buffer));
   }
   else if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(dbObject->getId());
   }
   else if (compareAttributeName(attr, "isDeleted"))
   {
      value = vm->createValue(dbObject->isDeleted());
   }
   else if (compareAttributeName(attr, "isDisabled"))
   {
      value = vm->createValue(dbObject->isDisabled());
   }
   else if (compareAttributeName(attr, "isGroup"))
   {
      value = vm->createValue(dbObject->isGroup());
   }
   else if (compareAttributeName(attr, "isModified"))
   {
      value = vm->createValue(dbObject->isModified());
   }
   else if (compareAttributeName(attr, "isLDAPUser"))
   {
      value = vm->createValue(dbObject->isLDAPUser());
   }
   else if (compareAttributeName(attr, "ldapDN"))
   {
      value = vm->createValue(dbObject->getDN());
   }
   else if (compareAttributeName(attr, "ldapId"))
   {
      value = vm->createValue(dbObject->getLdapId());
   }
   else if (compareAttributeName(attr, "name"))
   {
      value = vm->createValue(dbObject->getName());
   }
   else if (compareAttributeName(attr, "systemRights"))
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
NXSL_Value *NXSL_UserClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   User *user = static_cast<User*>(object->getData());
   if (compareAttributeName(attr, "authMethod"))
   {
      value = vm->createValue(static_cast<int32_t>(user->getAuthMethod()));
   }
   else if (compareAttributeName(attr, "certMappingData"))
   {
      value = vm->createValue(user->getCertMappingData());
   }
   else if (compareAttributeName(attr, "certMappingMethod"))
   {
      value = vm->createValue(user->getCertMappingMethod());
   }
   else if (compareAttributeName(attr, "disabledUntil"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getReEnableTime()));
   }
   else if (compareAttributeName(attr, "email"))
   {
      value = vm->createValue(user->getEmail());
   }
   else if (compareAttributeName(attr, "fullName"))
   {
      value = vm->createValue(user->getFullName());
   }
   else if (compareAttributeName(attr, "graceLogins"))
   {
      value = vm->createValue(user->getGraceLogins());
   }
   else if (compareAttributeName(attr, "lastLogin"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getLastLoginTime()));
   }
   else if (compareAttributeName(attr, "phoneNumber"))
   {
      value = vm->createValue(user->getPhoneNumber());
   }
   else if (compareAttributeName(attr, "xmppId"))
   {
      value = vm->createValue(user->getXmppId());
   }

   return value;
}

/**
 * NXSL class User: NXSL object destructor
 */
void NXSL_UserClass::onObjectDelete(NXSL_Object *object)
{
   delete (User *)object->getData();
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
NXSL_Value *NXSL_UserGroupClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   Group *group = static_cast<Group*>(object->getData());
   if (compareAttributeName(attr, "memberCount"))
   {
      value = vm->createValue(group->getMemberCount());
   }
   else if (compareAttributeName(attr, "members"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      ObjectArray<UserDatabaseObject> *userDB = FindUserDBObjects(group->getMembers());
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         if (userDB->get(i)->isGroup())
            array->append(vm->createValue(new NXSL_Object(vm, &g_nxslUserGroupClass, userDB->get(i))));
         else
            array->append(vm->createValue(new NXSL_Object(vm, &g_nxslUserClass, userDB->get(i))));
      }
      value = vm->createValue(array);
      delete userDB;
   }

   return value;
}

/**
 * NXSL class UserGroupClass: NXSL object destructor
 */
void NXSL_UserGroupClass::onObjectDelete(NXSL_Object *object)
{
   delete (Group *)object->getData();
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
NXSL_Value *NXSL_NodeDependencyClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   DependentNode *dn = static_cast<DependentNode*>(object->getData());

   if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(dn->nodeId);
   }
   else if (compareAttributeName(attr, "isAgentProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_AGENT_PROXY) != 0);
   }
   else if (compareAttributeName(attr, "isDataCollectionSource"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_DC_SOURCE) != 0);
   }
   else if (compareAttributeName(attr, "isICMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_ICMP_PROXY) != 0);
   }
   else if (compareAttributeName(attr, "isSNMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_SNMP_PROXY) != 0);
   }
   else if (compareAttributeName(attr, "type"))
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
 * NXSL "VLAN" class
 */
NXSL_VlanClass::NXSL_VlanClass()
{
   setName(_T("VLAN"));
}

/**
 * NXSL "VLAN" class: get attribute
 */
NXSL_Value *NXSL_VlanClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   VlanInfo *vlan = static_cast<VlanInfo*>(object->getData());

   if (compareAttributeName(attr, "id"))
   {
      value = vm->createValue(vlan->getVlanId());
   }
   else if (compareAttributeName(attr, "interfaces"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(vlan->getNodeId(), OBJECT_NODE));
      if (node != nullptr)
      {
         VlanPortInfo *ports = vlan->getPorts();
         for(int i = 0; i < vlan->getNumPorts(); i++)
         {
            shared_ptr<Interface> iface = node->findInterfaceByIndex(ports[i].ifIndex);
            if (iface != nullptr)
            {
               a->append(iface->createNXSLObject(vm));
            }
         }
      }
      value = vm->createValue(a);
   }
   else if (compareAttributeName(attr, "name"))
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
 * Class objects
 */
NXSL_AccessPointClass g_nxslAccessPointClass;
NXSL_AlarmClass g_nxslAlarmClass;
NXSL_AlarmCommentClass g_nxslAlarmCommentClass;
NXSL_ChassisClass g_nxslChassisClass;
NXSL_ClusterClass g_nxslClusterClass;
NXSL_ContainerClass g_nxslContainerClass;
NXSL_DciClass g_nxslDciClass;
NXSL_EventClass g_nxslEventClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_NodeDependencyClass g_nxslNodeDependencyClass;
NXSL_SensorClass g_nxslSensorClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_SubnetClass g_nxslSubnetClass;
NXSL_TemplateClass g_nxslTemplateClass;
NXSL_TunnelClass g_nxslTunnelClass;
NXSL_UserDBObjectClass g_nxslUserDBObjectClass;
NXSL_UserClass g_nxslUserClass;
NXSL_UserGroupClass g_nxslUserGroupClass;
NXSL_VlanClass g_nxslVlanClass;
NXSL_ZoneClass g_nxslZoneClass;
