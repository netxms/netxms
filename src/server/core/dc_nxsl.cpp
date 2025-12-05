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
** File: dc_nxsl.cpp
**
**/

#include "nxcore.h"

/**
 * NXSL function: Get DCI object
 * First argument is a node object (usually passed to script via $node variable),
 * and second is DCI ID
 */
static int F_GetDCIObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if (dci != NULL)
	{
		*result = dci->createNXSLObject(vm);
	}
	else
	{
		*result = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * Common handler for GetDCIValue and GetDCIRawValue
 */
static int GetDCIValueImpl(bool rawValue, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if (dci != nullptr)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *result = rawValue ? static_cast<DCItem*>(dci.get())->getRawValueForNXSL(vm) : static_cast<DCItem*>(dci.get())->getValueForNXSL(vm, F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         shared_ptr<Table> t = static_cast<DCTable*>(dci.get())->getLastValue();
         *result = (t != nullptr) ? vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(t))) : vm->createValue();
      }
      else
      {
   		*result = vm->createValue();	// Return NULL
      }
   }
	else
	{
		*result = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI ID
 */
static int F_GetDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return GetDCIValueImpl(false, argc, argv, ppResult, vm);
}

/**
 * NXSL function: Get raw DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI ID
 */
static int F_GetDCIRawValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return GetDCIValueImpl(true, argc, argv, ppResult, vm);
}

/**
 * Internal implementation of GetDCIValueByName and GetDCIValueByDescription
 */
static int GetDciValueExImpl(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, shared_ptr<DCObject> (DataCollectionOwner::*method)(const TCHAR*, uint32_t) const)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
   shared_ptr<DCObject> dci = ((*node).*method)(argv[1]->getValueAsCString(), 0);
	if (dci != NULL)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *result = static_cast<DCItem*>(dci.get())->getValueForNXSL(vm, F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         shared_ptr<Table> t = static_cast<DCTable*>(dci.get())->getLastValue();
         *result = (t != nullptr) ? vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(t))) : vm->createValue();
      }
      else
      {
   		*result = vm->createValue();	// Return NULL
      }
   }
	else
	{
		*result = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI name
 */
static int F_GetDCIValueByName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	return GetDciValueExImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByName);
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI description
 */
static int F_GetDCIValueByDescription(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	return GetDciValueExImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByDescription);
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI tag
 */
static int F_GetDCIValueByTag(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return GetDciValueExImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByTag);
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI tag pattern
 */
static int F_GetDCIValueByTagPattern(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return GetDciValueExImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByTagPattern);
}

/**
 * Common implementation for FindDCIBy... functions
 */
static int FindDCIImpl(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm, shared_ptr<DCObject> (DataCollectionOwner::*method)(const TCHAR*, uint32_t) const)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
   shared_ptr<DCObject> dci = ((*node).*method)(argv[1]->getValueAsCString(), 0);
   *result = (dci != nullptr) ? vm->createValue(dci->getId()) : vm->createValue(static_cast<uint32_t>(0));
   return NXSL_ERR_SUCCESS;
}

/**
 * NXSL function: Find DCI by name
 */
static int F_FindDCIByName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return FindDCIImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByName);
}

/**
 * NXSL function: Find DCI by description
 */
static int F_FindDCIByDescription(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return FindDCIImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByDescription);
}

/**
 * NXSL function: Find DCI by tag
 */
static int F_FindDCIByTag(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return FindDCIImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByTag);
}

/**
 * NXSL function: Find DCI by tag pattern
 */
static int F_FindDCIByTagPattern(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return FindDCIImpl(argc, argv, result, vm, &DataCollectionOwner::getDCObjectByTagPattern);
}

/**
 * NXSL function: Find all DCIs with matching name, description, tag, or related object
 */
static int F_FindAllDCIs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 5))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   const wchar_t *nameFilter = nullptr, *descriptionFilter = nullptr, *tagFilter = nullptr;
   uint32_t relatedObjectId = 0;
   if (argc > 1)
   {
      if (!argv[1]->isNull())
      {
         if (!argv[1]->isString())
            return NXSL_ERR_NOT_STRING;
         nameFilter = argv[1]->getValueAsCString();
      }

      if (argc > 2)
      {
         if (!argv[2]->isNull())
         {
            if (!argv[2]->isString())
               return NXSL_ERR_NOT_STRING;
            descriptionFilter = argv[2]->getValueAsCString();
         }

         if (argc > 3)
         {
            if (!argv[3]->isNull())
            {
               if (!argv[3]->isString())
                  return NXSL_ERR_NOT_STRING;
               tagFilter = argv[3]->getValueAsCString();
            }

            if (argc > 4)
            {
               if (!argv[4]->isNull())
               {
                  if (argv[4]->isObject(L"NetObj"))
                  {
                     NXSL_Object *o = argv[4]->getValueAsObject();
                     shared_ptr<NetObj> relatedObject = *static_cast<shared_ptr<NetObj>*>(o->getData());
                     relatedObjectId = relatedObject->getId();
                  }
                  else if (argv[4]->isInteger())
                  {
                     relatedObjectId = argv[4]->getValueAsUInt32();
                  }
                  else
                  {
                     return NXSL_ERR_NOT_INTEGER;
                  }
               }
            }
         }
      }
   }

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
	*result = node->getAllDCObjectsForNXSL(vm, nameFilter, descriptionFilter, tagFilter, relatedObjectId, 0);
	return 0;
}

/**
 * Helper function for creating instance in instance discovery filter
 */
static int F_Instance(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   NXSL_Object *object = nullptr;
   const TCHAR *name = nullptr;
   const TCHAR *displayName = nullptr;

   for(int i = 0; i < argc; i++)
   {
      if (argv[i]->getName() == nullptr)
         continue;

      if (!strcmp(argv[i]->getName(), "name"))
      {
         if (!argv[i]->isString())
            return NXSL_ERR_NOT_STRING;
         name = argv[i]->getValueAsCString();
      }
      else if (!strcmp(argv[i]->getName(), "displayName"))
      {
         if (!argv[i]->isString())
            return NXSL_ERR_NOT_STRING;
         displayName = argv[i]->getValueAsCString();
      }
      else if (!strcmp(argv[i]->getName(), "object"))
      {
         if (!argv[i]->isObject(_T("NetObj")))
            return NXSL_ERR_NOT_OBJECT;
         object = argv[i]->getValueAsObject();
      }
   }

   NXSL_Array *list = new NXSL_Array(vm);
   list->set(list->size(), vm->createValue(true));
   list->set(list->size(), (name != nullptr) ? vm->createValue(name) : vm->createValue());
   list->set(list->size(), (displayName != nullptr) ? vm->createValue(displayName) : vm->createValue());
   list->set(list->size(), (object != nullptr) ? vm->createValue(vm->createObject(object)) : vm->createValue());
   *result = vm->createValue(list);
   return 0;
}

/**
 * Get min, max or average of DCI values for a period
 */
static int F_GetDCIValueStat(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm, AggregationFunction func)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger() || !argv[2]->isInteger() || !argv[3]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
	{
      TCHAR *result = static_cast<DCItem*>(dci.get())->getAggregateValue(func, argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32());
      if (result != nullptr)
      {
         // if there are no values driver will return empty string instead of actual NULL read from database
         *ppResult = ((*result != 0) ? vm->createValue(result) : vm->createValue());
         MemFree(result);
      }
      else
      {
		   *ppResult = vm->createValue();	// Return NULL if query failed
      }
	}
	else
	{
		*ppResult = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * Get min of DCI values for a period
 */
static int F_GetMinDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AGG_MIN);
}

/**
 * Get max of DCI values for a period
 */
static int F_GetMaxDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AGG_MAX);
}

/**
 * Get average of DCI values for a period
 */
static int F_GetAvgDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AGG_AVG);
}

/**
 * Get sum of DCI values for a period
 */
static int F_GetSumDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AGG_SUM);
}

/**
 * Get all DCI values for period (common implementation for GetDCIValues and GetDCIValuesEx).
 * If asDataPoints is true, return array of data points (timestamp + value).
 */
static int GetDCIValuesImpl(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm, bool asDataPoints)
{
   if ((argc < 3) || (argc > 5))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger() || !argv[2]->isInteger() || ((argc > 3) && !argv[3]->isInteger()))
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
	{
      StringBuffer query(asDataPoints ? L"SELECT idata_timestamp," : L"SELECT ");
      query.append(((argc > 4) && argv[4]->isTrue()) ? _T("raw_value") : _T("idata_value"));
      if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            query.append(_T(" FROM idata_sc_"));
            query.append(DCObject::getStorageClassName(dci->getStorageClass()));
            query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN to_timestamp(?) AND to_timestamp(?)"));
         }
         else
         {
            query.append(_T(" FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
         }
      }
      else
      {
         query.append(_T(" FROM idata_"));
         query.append(node->getId());
         query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
      }
      query.append(_T(" ORDER BY idata_timestamp DESC"));

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      DB_STATEMENT hStmt = DBPrepare(hdb, query);
		if (hStmt != nullptr)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, argv[1]->getValueAsUInt32());
			DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, argv[2]->getValueAsInt32());
			DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (argc > 3) ? argv[3]->getValueAsInt32() : static_cast<int32_t>(time(nullptr)));
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != nullptr)
			{
            wchar_t buffer[MAX_RESULT_LENGTH];
            NXSL_Array *result = new NXSL_Array(vm);
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               if (asDataPoints)
               {
                  time_t t = DBGetFieldInt64(hResult, i, 0);
                  auto data = new std::pair<time_t, String>(t, DBGetFieldAsString(hResult, i, 1));
                  result->set(i, vm->createValue(vm->createObject(&g_nxslDataPointClass, data)));
               }
               else
               {
                  DBGetField(hResult, i, 0, buffer, MAX_RESULT_LENGTH);
                  result->set(i, vm->createValue(buffer));
               }
            }
				*ppResult = vm->createValue(result);
				DBFreeResult(hResult);
			}
			else
			{
				*ppResult = vm->createValue();	// Return NULL if prepared select failed
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			*ppResult = vm->createValue();	// Return NULL if prepare failed
		}

		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		*ppResult = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * Get all DCI values for period.
 * Format: GetDCIValues(node, dciId, startTime, endTime, rawValue)
 * Raw value indicator is optional (default is false)
 * End time is optional (default is current time)
 * Returns NULL if DCI not found or array of DCI values (ordered from latest to earliest)
 */
static int F_GetDCIValues(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   return GetDCIValuesImpl(argc, argv, ppResult, vm, false);
}

/**
 * Get all DCI values for period as data points (timestamp + values).
 * Format: GetDCIValues(node, dciId, startTime, endTime, rawValue)
 * Raw value indicator is optional (default is false)
 * End time is optional (default is current time)
 * Returns NULL if DCI not found or array of DCI values (ordered from latest to earliest)
 */
static int F_GetDCIValuesEx(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   return GetDCIValuesImpl(argc, argv, ppResult, vm, true);
}

/**
 * NXSL function: create new DCI
 * Format: CreateDCI(node, origin, name, description, dataType, pollingInterval, retentionTime)
 * Possible origin values: "agent", "snmp", "internal", "push", "websvc", "winperf", "script", "ssh", "mqtt", "driver", "modbus", "ethernetip"
 * Possible dataType values: "int32", "uint32", "int64", "uint64", "counter32", "counter64", "float", "string"
 * Returns DCI object on success and NULL of failure
 */
static int F_CreateDCI(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString() || !argv[2]->isString() || !argv[3]->isString() ||
	    !argv[4]->isString() || !(argv[5]->isString() || argv[5]->isNull()) || !(argv[6]->isString() || argv[6]->isNull()))
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
		return NXSL_ERR_BAD_CLASS;
	shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());

	// Origin
	int origin = -1;
	if (argv[1]->isInteger())
	{
	   origin = argv[1]->getValueAsInt32();
	   if ((origin < 0) || (origin > 12))
	      origin = -1;
	}
	else
	{
	   static const WCHAR *originNames[] = {
            L"internal", L"agent", L"snmp", L"websvc", L"push", L"winperf",
            L"smclp", L"script", L"ssh", L"mqtt", L"driver", L"modbus", L"ethernetip", nullptr };
      const WCHAR *name = argv[1]->getValueAsCString();
      for(int i = 0; originNames[i] != nullptr; i++)
         if (!wcsicmp(originNames[i], name))
         {
            origin = i;
            break;
         }
	}

	// Data types
	int dataType = -1;
	if (argv[4]->isInteger())
	{
	   dataType = argv[4]->getValueAsInt32();
      if ((dataType < 0) || (dataType > 8))
         dataType = -1;
	}
	else
	{
      dataType = TextToDataType(argv[4]->getValueAsCString());
	}

	StringBuffer type(argv[5]->isString() ? argv[5]->getValueAsCString() : _T(""));
	type.trim();
	BYTE scheduleType = (type.isEmpty() || type.equals(_T("0"))) ? DC_POLLING_SCHEDULE_DEFAULT : DC_POLLING_SCHEDULE_CUSTOM;

   type = argv[6]->isString() ? argv[6]->getValueAsCString() : _T("");
   type.trim();
   BYTE retentionType = DC_RETENTION_CUSTOM;
   if (type.isEmpty())
   {
      retentionType = DC_RETENTION_DEFAULT;
   }
   else if (type.equals(_T("0")))
   {
      retentionType = DC_RETENTION_NONE;
   }

	if ((origin != -1) && (dataType != -1))
	{
		DCItem *dci = new DCItem(CreateUniqueId(IDG_ITEM), argv[2]->getValueAsCString(),
		         origin, dataType, scheduleType, argv[5]->isString() ? argv[5]->getValueAsCString() : nullptr,
		         retentionType, argv[6]->isString() ? argv[6]->getValueAsCString() : nullptr, node, argv[3]->getValueAsCString());
		node->addDCObject(dci);
		*result = dci->createNXSLObject(vm);
	}
	else
	{
		*result = vm->createValue();
	}
   return NXSL_ERR_SUCCESS;;
}

/**
 * Push DCI value
 * Format: PushDCIData(node, dciId, value)
 * No return value
 */
static int F_PushDCIData(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (!argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
	if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
		return NXSL_ERR_BAD_CLASS;
   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());

   bool success = false;
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
   if ((dci != nullptr) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getType() == DCO_TYPE_ITEM))
   {
      Timestamp t = Timestamp::now();
      if (dci->getLastValueTimestamp() == t)
      {
         // Ensure 1 ms difference between two consecutive values
         ThreadSleepMs(1);
         t = Timestamp::now();
      }
      success = node->processNewDCValue(dci, t, argv[2]->getValueAsCString(), shared_ptr<Table>(), false);
      if (success)
         dci->setLastPollTime(t);
   }

   *result = vm->createValue(success ? 1 : 0);
   return NXSL_ERR_SUCCESS;;
}

/**
 * Detect anomalies for DCI within given period
 * Format: DetectAnomalies(node, dciId, startTime, endTime, threshold)
 * End time is optional (default is current time)
 * Threshold is optional (default is 0.75)
 * Returns null on error and detected anomalies on success
 */
static int F_DetectAnomalies(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 3) || (argc > 5))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isInteger() || !argv[2]->isInteger() || ((argc > 3) && !argv[3]->isInteger()))
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 4) && !argv[4]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   NXSL_Object *object = argv[0]->getValueAsObject();
   if (!object->getClass()->instanceOf(_T("DataCollectionTarget")))
      return NXSL_ERR_BAD_CLASS;

   shared_ptr<DataCollectionTarget> node = *static_cast<shared_ptr<DataCollectionTarget>*>(object->getData());
   unique_ptr<StructArray<ScoredDciValue>> anomalies = DetectAnomalies(*node, argv[1]->getValueAsUInt32(), static_cast<time_t>(argv[2]->getValueAsInt64()),
      (argc > 3) ? static_cast<time_t>(argv[3]->getValueAsInt64()) : time(nullptr), (argc > 4) ? argv[4]->getValueAsReal() : 0.75);
   if (anomalies != nullptr)
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < anomalies->size(); i++)
         array->append(vm->createValue(vm->createObject(&g_nxslScoredDciValueClass, new ScoredDciValue(*anomalies->get(i)))));
      *result = vm->createValue(array);
   }
   else
   {
      *result = vm->createValue();
   }
   return NXSL_ERR_SUCCESS;;
}

/**
 * Additional NXSL functions for DCI manipulation
 */
static NXSL_ExtFunction m_nxslDCIFunctions[] =
{
   { "CreateDCI", F_CreateDCI, 7 },
   { "DetectAnomalies", F_DetectAnomalies, -1 },
   { "FindAllDCIs", F_FindAllDCIs, -1 },
   { "FindDCIByName", F_FindDCIByName, 2 },
   { "FindDCIByDescription", F_FindDCIByDescription, 2 },
   { "FindDCIByTag", F_FindDCIByTag, 2 },
   { "FindDCIByTagPattern", F_FindDCIByTagPattern, 2 },
	{ "GetAvgDCIValue", F_GetAvgDCIValue, 4 },
   { "GetDCIObject", F_GetDCIObject, 2 },
   { "GetDCIRawValue", F_GetDCIRawValue, 2 },
   { "GetDCIValue", F_GetDCIValue, 2 },
   { "GetDCIValues", F_GetDCIValues, -1 },
   { "GetDCIValuesEx", F_GetDCIValuesEx, -1 },
   { "GetDCIValueByDescription", F_GetDCIValueByDescription, 2 },
   { "GetDCIValueByName", F_GetDCIValueByName, 2 },
   { "GetDCIValueByTag", F_GetDCIValueByTag, 2 },
   { "GetDCIValueByTagPattern", F_GetDCIValueByTagPattern, 2 },
	{ "GetMaxDCIValue", F_GetMaxDCIValue, 4 },
	{ "GetMinDCIValue", F_GetMinDCIValue, 4 },
	{ "GetSumDCIValue", F_GetSumDCIValue, 4 },
   { "Instance", F_Instance, -1 , true },
   { "PushDCIData", F_PushDCIData, 3 }
};

/**
 * Register DCI-related functions in NXSL environment
 */
void RegisterDCIFunctions(NXSL_Environment *pEnv)
{
	pEnv->registerFunctionSet(sizeof(m_nxslDCIFunctions) / sizeof(NXSL_ExtFunction), m_nxslDCIFunctions);
}
