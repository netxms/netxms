/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
static int F_GetDCIObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if (dci != NULL)
	{
		*ppResult = dci->createNXSLObject(vm);
	}
	else
	{
		*ppResult = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * Common handler for GetDCIValue and GetDCIRawValue
 */
static int GetDCIValueImpl(bool rawValue, int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if (dci != NULL)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *ppResult = rawValue ? static_cast<DCItem*>(dci.get())->getRawValueForNXSL(vm) : static_cast<DCItem*>(dci.get())->getValueForNXSL(vm, F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         Table *t = static_cast<DCTable*>(dci.get())->getLastValue();
         *ppResult = (t != NULL) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, t)) : vm->createValue();
      }
      else
      {
   		*ppResult = vm->createValue();	// Return NULL
      }
   }
	else
	{
		*ppResult = vm->createValue();	// Return NULL if DCI not found
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
static int GetDciValueExImpl(bool byName, int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = byName ? node->getDCObjectByName(argv[1]->getValueAsCString(), 0) : node->getDCObjectByDescription(argv[1]->getValueAsCString(), 0);
	if (dci != NULL)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *ppResult = static_cast<DCItem*>(dci.get())->getValueForNXSL(vm, F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         Table *t = static_cast<DCTable*>(dci.get())->getLastValue();
         *ppResult = (t != NULL) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, t)) : vm->createValue();
      }
      else
      {
   		*ppResult = vm->createValue();	// Return NULL
      }
   }
	else
	{
		*ppResult = vm->createValue();	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI name
 */
static int F_GetDCIValueByName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return GetDciValueExImpl(true, argc, argv, ppResult, vm);
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI description
 */
static int F_GetDCIValueByDescription(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return GetDciValueExImpl(false, argc, argv, ppResult, vm);
}

/**
 * NXSL function: Find DCI by name
 */
static int F_FindDCIByName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectByName(argv[1]->getValueAsCString(), 0);
	*ppResult = (dci != NULL) ? vm->createValue(dci->getId()) : vm->createValue((UINT32)0);
	return 0;
}

/**
 * NXSL function: Find DCI by description
 */
static int F_FindDCIByDescription(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectByDescription(argv[1]->getValueAsCString(), 0);
	*ppResult = (dci != NULL) ? vm->createValue(dci->getId()) : vm->createValue((UINT32)0);
	return 0;
}

/**
 * NXSL function: Find all DCIs with matching name or description
 */
static int F_FindAllDCIs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 3))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

   const TCHAR *nameFilter = NULL, *descriptionFilter = NULL;
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
      }
   }

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	*ppResult = node->getAllDCObjectsForNXSL(vm, nameFilter, descriptionFilter, 0);
	return 0;
}

static int F_Instance(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if ((argc < 1) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   NXSL_Object *object = nullptr;
   const TCHAR *name = nullptr;
   const TCHAR *displayName = nullptr;

   for(int i = 0; i < argc; i++)
   {
      if(!stricmp(argv[i]->getName(), "name"))
      {
         if(!argv[i]->isString())
            return NXSL_ERR_NOT_STRING;
         name = argv[i]->getValueAsCString();
      }
      else if (!stricmp(argv[i]->getName(), "displayName"))
      {
         if(!argv[i]->isString())
            return NXSL_ERR_NOT_STRING;
         displayName = argv[i]->getValueAsCString();
      }
      else if (!stricmp(argv[i]->getName(), "object"))
      {
         if(!argv[i]->isObject(_T("NetObj")))
            return NXSL_ERR_NOT_OBJECT;
         object = argv[i]->getValueAsObject();
      }
   }

   NXSL_Array *list = new NXSL_Array(vm);
   list->set(list->size(), vm->createValue(true));
   list->set(list->size(), (name != nullptr) ? vm->createValue(name) : vm->createValue());
   list->set(list->size(), (displayName != nullptr) ? vm->createValue(displayName) : vm->createValue());
   list->set(list->size(), (object != nullptr) ? vm->createValue(new NXSL_Object(object)) : vm->createValue());
   *ppResult = vm->createValue(list);
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
      TCHAR *result = static_cast<DCItem*>(dci.get())->getAggregateValue(func, argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32());
      if (result != NULL)
      {
         *ppResult = vm->createValue(result);
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
 * Get average of DCI values for a period
 */
static int F_GetSumDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AGG_SUM);
}

/**
 * Get all DCI values for period
 * Format: GetDCIValues(node, dciId, startTime, endTime)
 * Returns NULL if DCI not found or array of DCI values (ordered from latest to earliest)
 */
static int F_GetDCIValues(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger() || !argv[2]->isInteger() || !argv[3]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      TCHAR query[1024];
      if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            _sntprintf(query, 256,
                     _T("SELECT idata_value FROM idata_sc_%s WHERE node_id=? AND item_id=? AND idata_timestamp BETWEEN ? AND ? ORDER BY idata_timestamp DESC"),
                     DCObject::getStorageClassName(dci->getStorageClass()));
         }
         else
         {
            _tcscpy(query, _T("SELECT idata_value FROM idata WHERE node_id=? AND item_id=? AND idata_timestamp BETWEEN ? AND ? ORDER BY idata_timestamp DESC"));
         }
      }
      else
      {
         _sntprintf(query, 1024, _T("SELECT idata_value FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? ORDER BY idata_timestamp DESC"), node->getId());
      }

      DB_STATEMENT hStmt = DBPrepare(hdb, query);
		if (hStmt != NULL)
		{
		   int index = 1;
	      if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	         DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, node->getId());
			DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, argv[1]->getValueAsUInt32());
			DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, argv[2]->getValueAsInt32());
			DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, argv[3]->getValueAsInt32());
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
            NXSL_Array *result = new NXSL_Array(vm);
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               TCHAR buffer[MAX_RESULT_LENGTH];
               DBGetField(hResult, i, 0, buffer, MAX_RESULT_LENGTH);
               result->set(i, vm->createValue(buffer));
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
 * NXSL function: create new DCI
 * Format: CreateDCI(node, origin, name, description, dataType, pollingInterval, retentionTime)
 * Possible origin values: "agent", "snmp", "internal", "push"
 * Possible dataType values: "int32", "uint32", "int64", "uint64", "float", "string"
 * Returns DCI object on success and NULL of failure
 */
static int F_CreateDCI(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString() || !argv[2]->isString() || !argv[3]->isString() ||
	    !argv[4]->isString() || !(argv[5]->isString() || argv[5]->isNull()) || !(argv[6]->isString() || argv[6]->isNull()))
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;
	DataCollectionTarget *node = static_cast<DataCollectionTarget*>(object->getData());

	// Origin
	static const TCHAR *originNames[] = { _T("internal"), _T("agent"), _T("snmp"), _T("cpsnmp"),
	         _T("push"), _T("winperf"), _T("smclp"), _T("script"), _T("ssh"), _T("mqtt"), _T("driver"), NULL };
	int origin = -1;
	const TCHAR *name = argv[1]->getValueAsCString();
	for(int i = 0; originNames[i] != NULL; i++)
		if (!_tcsicmp(originNames[i], name))
		{
			origin = i;
			break;
		}

	// Data types
	static const TCHAR *dtNames[] = { _T("int32"), _T("uint32"), _T("int64"), _T("uint64"), _T("string"), _T("float"), NULL };
	int dataType = -1;
	name = argv[4]->getValueAsCString();
	for(int i = 0; dtNames[i] != NULL; i++)
		if (!_tcsicmp(dtNames[i], name))
		{
			dataType = i;
			break;
		}

	if ((origin != -1) && (dataType != -1))
	{
		DCItem *dci = new DCItem(CreateUniqueId(IDG_ITEM), argv[2]->getValueAsCString(),
		         origin, dataType, argv[5]->isString() ? argv[5]->getValueAsCString() : NULL,
			      argv[6]->isString() ? argv[6]->getValueAsCString() : NULL, node, argv[3]->getValueAsCString());
		node->addDCObject(dci);
		*ppResult = dci->createNXSLObject(vm);
	}
	else
	{
		*ppResult = vm->createValue();
	}
	return 0;
}

/**
 * Push DCI value
 * Format: PushDCIData(node, dciId, value)
 * No return value
 */
static int F_PushDCIData(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   if (!argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (!argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslClusterClass.getName())
	    && _tcscmp(object->getClass()->getName(), g_nxslMobileDeviceClass.getName()) && _tcscmp(object->getClass()->getName(), g_nxslSensorClass.getName()))
		return NXSL_ERR_BAD_CLASS;
	DataCollectionTarget *node = (DataCollectionTarget *)object->getData();

   bool success = false;
	shared_ptr<DCObject> dci = node->getDCObjectById(argv[1]->getValueAsUInt32(), 0);
   if ((dci != NULL) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getType() == DCO_TYPE_ITEM))
   {
      time_t t = time(NULL);
      success = node->processNewDCValue(dci, t, const_cast<TCHAR*>(argv[2]->getValueAsCString())); // value will not be modified by processNewDCValue
      if (success)
         dci->setLastPollTime(t);
   }

   *ppResult = vm->createValue(success ? 1 : 0);
   return 0;
}

/**
 * Additional NXSL functions for DCI manipulation
 */
static NXSL_ExtFunction m_nxslDCIFunctions[] =
{
   { "CreateDCI", F_CreateDCI, 7 },
   { "FindAllDCIs", F_FindAllDCIs, -1 },
   { "FindDCIByName", F_FindDCIByName, 2 },
   { "FindDCIByDescription", F_FindDCIByDescription, 2 },
	{ "GetAvgDCIValue", F_GetAvgDCIValue, 4 },
   { "GetDCIObject", F_GetDCIObject, 2 },
   { "GetDCIRawValue", F_GetDCIRawValue, 2 },
   { "GetDCIValue", F_GetDCIValue, 2 },
   { "GetDCIValues", F_GetDCIValues, 4 },
   { "GetDCIValueByDescription", F_GetDCIValueByDescription, 2 },
   { "GetDCIValueByName", F_GetDCIValueByName, 2 },
	{ "GetMaxDCIValue", F_GetMaxDCIValue, 4 },
	{ "GetMinDCIValue", F_GetMinDCIValue, 4 },
	{ "GetSumDCIValue", F_GetSumDCIValue, 4 },
   { "Instance", F_Instance, -1 },
   { "PushDCIData", F_PushDCIData, 3 }
};

/**
 * Register DCI-related functions in NXSL environment
 */
void RegisterDCIFunctions(NXSL_Environment *pEnv)
{
	pEnv->registerFunctionSet(sizeof(m_nxslDCIFunctions) / sizeof(NXSL_ExtFunction), m_nxslDCIFunctions);
}
