/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if (dci != NULL)
	{
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslDciClass, dci));
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if (dci != NULL)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *ppResult = rawValue ? ((DCItem *)dci)->getRawValueForNXSL() : ((DCItem *)dci)->getValueForNXSL(F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         Table *t = ((DCTable *)dci)->getLastValue();
         *ppResult = (t != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslTableClass, t)) : new NXSL_Value;
      }
      else
      {
   		*ppResult = new NXSL_Value;	// Return NULL
      }
   }
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
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
static int GetDciValueExImpl(bool byName, int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = byName ? node->getDCObjectByName(argv[1]->getValueAsCString()) : node->getDCObjectByDescription(argv[1]->getValueAsCString());
	if (dci != NULL)
   {
      if (dci->getType() == DCO_TYPE_ITEM)
	   {
         *ppResult = ((DCItem *)dci)->getValueForNXSL(F_LAST, 1);
	   }
      else if (dci->getType() == DCO_TYPE_TABLE)
      {
         Table *t = ((DCTable *)dci)->getLastValue();
         *ppResult = (t != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslTableClass, t)) : new NXSL_Value;
      }
      else
      {
   		*ppResult = new NXSL_Value;	// Return NULL
      }
   }
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
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
	return GetDciValueExImpl(true, argc, argv, ppResult);
}

/**
 * NXSL function: Get DCI value from within transformation script
 * First argument is a node object (passed to script via $node variable),
 * and second is DCI description
 */
static int F_GetDCIValueByDescription(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return GetDciValueExImpl(false, argc, argv, ppResult);
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectByName(argv[1]->getValueAsCString());
	*ppResult = (dci != NULL) ? new NXSL_Value(dci->getId()) : new NXSL_Value((UINT32)0);
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectByDescription(argv[1]->getValueAsCString());
	*ppResult = (dci != NULL) ? new NXSL_Value(dci->getId()) : new NXSL_Value((UINT32)0);
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
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

	Node *node = (Node *)object->getData();
	*ppResult = node->getAllDCObjectsForNXSL(nameFilter, descriptionFilter);
	return 0;
}

/**
 * Get min, max or average of DCI values for a period
 */
typedef enum { DCI_MIN = 0, DCI_MAX = 1, DCI_AVG = 2, DCI_SUM = 3 } DciSqlFunc_t;

static int F_GetDCIValueStat(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm, DciSqlFunc_t sqlFunc)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger() || !argv[2]->isInteger() || !argv[3]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;
	
	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		double result = 0;
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		TCHAR query[1024];
      static const TCHAR *functions[] = { _T("min"), _T("max"), _T("avg"), _T("sum") };

		if (g_dbSyntax == DB_SYNTAX_ORACLE)
		{
			_sntprintf(query, 1024, _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata_%u ")
				_T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"), 
				functions[sqlFunc], node->Id());
		}
		else if (g_dbSyntax == DB_SYNTAX_MSSQL)
		{
			_sntprintf(query, 1024, _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata_%u ")
				_T("WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"), 
				functions[sqlFunc], node->Id());
		}
		else if (g_dbSyntax == DB_SYNTAX_PGSQL)
		{
			_sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value::double precision,0)) FROM idata_%u ")
				_T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"), 
				functions[sqlFunc],	node->Id());
		}
		else
		{
			_sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value,0)) FROM idata_%u ")
				_T("WHERE item_id=? and idata_timestamp between ? and ?"), 
				functions[sqlFunc],	node->Id());
		}

		DB_STATEMENT hStmt = DBPrepare(hdb, query);
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, argv[1]->getValueAsUInt32());
			DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, argv[2]->getValueAsInt32());
			DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, argv[3]->getValueAsInt32());
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				if (DBGetNumRows(hResult) == 1)
				{
					result = DBGetFieldDouble(hResult, 0, 0);
				}
				*ppResult = new NXSL_Value(result);
				DBFreeResult(hResult);
			}
			else
			{
				*ppResult = new NXSL_Value;	// Return NULL if prepared select failed
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			*ppResult = new NXSL_Value;	// Return NULL if prepare failed
		}

		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
	}

	return 0;
}

/**
 * Get min of DCI values for a period
 */
static int F_GetMinDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_MIN);
}

/**
 * Get max of DCI values for a period
 */
static int F_GetMaxDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_MAX);
}

/**
 * Get average of DCI values for a period
 */
static int F_GetAvgDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_AVG);
}

/**
 * Get average of DCI values for a period
 */
static int F_GetSumDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	return F_GetDCIValueStat(argc, argv, ppResult, vm, DCI_SUM);
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
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;
	Node *node = (Node *)object->getData();

	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT idata_value FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? ORDER BY idata_timestamp DESC"), node->Id());

      DB_STATEMENT hStmt = DBPrepare(hdb, query);
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, argv[1]->getValueAsUInt32());
			DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, argv[2]->getValueAsInt32());
			DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, argv[3]->getValueAsInt32());
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
            NXSL_Array *result = new NXSL_Array();
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               TCHAR buffer[MAX_RESULT_LENGTH];
               DBGetField(hResult, i, 0, buffer, MAX_RESULT_LENGTH);
               result->set(i, new NXSL_Value(buffer));
            }
				*ppResult = new NXSL_Value(result);
				DBFreeResult(hResult);
			}
			else
			{
				*ppResult = new NXSL_Value;	// Return NULL if prepared select failed
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			*ppResult = new NXSL_Value;	// Return NULL if prepare failed
		}

		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
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

	if (!argv[1]->isString() || !argv[2]->isString() || !argv[3]->isString() || !argv[4]->isString())
		return NXSL_ERR_NOT_STRING;

	if (!argv[5]->isInteger() || !argv[6]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;
	Node *node = (Node *)object->getData();

	// Origin
	static const TCHAR *originNames[] = { _T("internal"), _T("agent"), _T("snmp"), _T("cpsnmp"), _T("push"), NULL };
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

	int pollingInterval = argv[5]->getValueAsInt32();
	int retentionTime = argv[6]->getValueAsInt32();

	if ((origin != -1) && (dataType != -1) && (pollingInterval > 0) && (retentionTime > 0))
	{
		DCItem *dci = new DCItem(CreateUniqueId(IDG_ITEM), argv[2]->getValueAsCString(), 
			origin, dataType, pollingInterval, retentionTime, node, argv[3]->getValueAsCString());
		node->addDCObject(dci);
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslDciClass, dci));
	}
	else
	{
		*ppResult = new NXSL_Value;
	}
	return 0;
}

/**
 * Additional NXSL functions for DCI manipulation
 */
static NXSL_ExtFunction m_nxslDCIFunctions[] =
{
   { _T("CreateDCI"), F_CreateDCI, 7 },
   { _T("FindAllDCIs"), F_FindAllDCIs, -1 },
   { _T("FindDCIByName"), F_FindDCIByName, 2 },
   { _T("FindDCIByDescription"), F_FindDCIByDescription, 2 },
	{ _T("GetAvgDCIValue"), F_GetAvgDCIValue, 4 },
   { _T("GetDCIObject"), F_GetDCIObject, 2 },
   { _T("GetDCIRawValue"), F_GetDCIRawValue, 2 },
   { _T("GetDCIValue"), F_GetDCIValue, 2 },
   { _T("GetDCIValues"), F_GetDCIValues, 4 },
   { _T("GetDCIValueByDescription"), F_GetDCIValueByDescription, 2 },
   { _T("GetDCIValueByName"), F_GetDCIValueByName, 2 },
	{ _T("GetMaxDCIValue"), F_GetMaxDCIValue, 4 },
	{ _T("GetMinDCIValue"), F_GetMinDCIValue, 4 },
	{ _T("GetSumDCIValue"), F_GetSumDCIValue, 4 }
};

/**
 * Register DCI-related functions in NXSL environment
 */
void RegisterDCIFunctions(NXSL_Environment *pEnv)
{
	pEnv->registerFunctionSet(sizeof(m_nxslDCIFunctions) / sizeof(NXSL_ExtFunction), m_nxslDCIFunctions);
}
