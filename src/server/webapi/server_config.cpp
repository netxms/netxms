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
** File: server_config.cpp
**
**/

#include "webapi.h"

/**
 * Maximum length of configuration variable name (defined by config.var_name)
 */
#define MAX_CONFIG_VAR_NAME_LEN     63

/**
 * Columns selected for building variable document, in the order expected by
 * ServerConfigVariableToJson()
 */
#define CONFIG_VARIABLE_COLUMNS     L"var_name,var_value,need_server_restart,data_type,description,default_value,units"

/**
 * Convert single character data type code stored in config.data_type into symbolic name.
 * Codes are the same as used by ServerVariableDataType on client side.
 */
static const char *DataTypeName(wchar_t code)
{
   switch(code)
   {
      case L'B':
         return "BOOLEAN";
      case L'C':
         return "CHOICE";
      case L'H':
         return "COLOR";
      case L'I':
         return "INTEGER";
      case L'P':
         return "PASSWORD";
      default:
         return "STRING";
   }
}

/**
 * Build JSON document for single configuration variable from result set row selected with
 * CONFIG_VARIABLE_COLUMNS. Possible values are attached separately by the caller.
 */
static json_t *ServerConfigVariableToJson(DB_RESULT hResult, int row)
{
   // DBGetField clears the buffer before reading and returns nullptr for NULL fields, so values
   // are always taken from the buffer - most columns of "config" are nullable, and a JSON null
   // for description or units would only complicate consumers.
   wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];

   json_t *variable = json_object();

   DBGetField(hResult, row, 0, buffer, MAX_CONFIG_VAR_NAME_LEN + 1);
   json_object_set_new(variable, "name", json_string_w(buffer));

   DBGetField(hResult, row, 1, buffer, MAX_CONFIG_VALUE_LENGTH);
   json_object_set_new(variable, "value", json_string_w(buffer));

   json_object_set_new(variable, "needServerRestart", json_boolean(DBGetFieldLong(hResult, row, 2) != 0));

   DBGetField(hResult, row, 3, buffer, MAX_CONFIG_VALUE_LENGTH);
   json_object_set_new(variable, "dataType", json_string(DataTypeName(buffer[0])));

   DBGetField(hResult, row, 4, buffer, MAX_CONFIG_VALUE_LENGTH);
   json_object_set_new(variable, "description", json_string_w(buffer));

   DBGetField(hResult, row, 5, buffer, MAX_CONFIG_VALUE_LENGTH);
   json_object_set_new(variable, "defaultValue", json_string_w(buffer));

   DBGetField(hResult, row, 6, buffer, MAX_CONFIG_VALUE_LENGTH);
   json_object_set_new(variable, "units", json_string_w(buffer));

   json_object_set_new(variable, "possibleValues", json_array());
   return variable;
}

/**
 * Load single visible configuration variable as JSON document. Returns nullptr if variable does not
 * exist, is not visible, or database query failed. Sets *dbFailure to true in the last case, so caller
 * can tell missing variable from failed query.
 */
static json_t *LoadServerConfigVariable(const wchar_t *name, bool *dbFailure)
{
   *dbFailure = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   json_t *variable = nullptr;
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT " CONFIG_VARIABLE_COLUMNS L" FROM config WHERE var_name=? AND is_visible=1");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            variable = ServerConfigVariableToJson(hResult, 0);
         DBFreeResult(hResult);
      }
      else
      {
         *dbFailure = true;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      *dbFailure = true;
   }

   if (variable != nullptr)
   {
      hStmt = DBPrepare(hdb, L"SELECT var_value,var_description FROM config_values WHERE var_name=?");
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            json_t *possibleValues = json_object_get(variable, "possibleValues");
            wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               json_t *element = json_object();
               DBGetField(hResult, i, 0, buffer, MAX_CONFIG_VALUE_LENGTH);
               json_object_set_new(element, "value", json_string_w(buffer));
               DBGetField(hResult, i, 1, buffer, MAX_CONFIG_VALUE_LENGTH);
               json_object_set_new(element, "description", json_string_w(buffer));
               json_array_append_new(possibleValues, element);
            }
            DBFreeResult(hResult);
         }
         else
         {
            *dbFailure = true;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         *dbFailure = true;
      }

      // Returning variable with empty possible values would understate constraints on its value
      if (*dbFailure)
      {
         json_decref(variable);
         variable = nullptr;
      }
   }

   DBConnectionPoolReleaseConnection(hdb);
   return variable;
}

/**
 * Send single configuration variable as response. Returns HTTP status code.
 */
static int SendServerConfigVariable(Context *context, const wchar_t *name)
{
   bool dbFailure;
   json_t *variable = LoadServerConfigVariable(name, &dbFailure);
   if (variable == nullptr)
   {
      if (dbFailure)
      {
         context->setErrorResponse("Database failure");
         return 500;
      }
      return 404;
   }

   context->setResponseData(variable);
   json_decref(variable);
   return 200;
}

/**
 * Validate name of configuration variable taken from URL
 */
static bool ValidateConfigVariableName(const wchar_t *name, Context *context)
{
   if ((name == nullptr) || (name[0] == 0))
   {
      context->setErrorResponse("Configuration variable name cannot be empty");
      return false;
   }
   if (wcslen(name) > MAX_CONFIG_VAR_NAME_LEN)
   {
      context->setErrorResponse("Configuration variable name is too long");
      return false;
   }
   return true;
}

/**
 * Handler for GET /v1/server-config
 * Returns all visible server configuration variables. Mirrors data set sent to desktop
 * console by ClientSession::getConfigurationVariables.
 */
int H_ServerConfigVariables(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, L"SELECT " CONFIG_VARIABLE_COLUMNS L" FROM config WHERE is_visible=1");
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = json_array();

   // Index into documents owned by output array, for attaching possible values below
   StringObjectMap<json_t> index(Ownership::False);

   wchar_t name[MAX_CONFIG_VAR_NAME_LEN + 1];
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      json_t *variable = ServerConfigVariableToJson(hResult, i);
      json_array_append_new(output, variable);
      DBGetField(hResult, i, 0, name, MAX_CONFIG_VAR_NAME_LEN + 1);
      index.set(name, variable);
   }
   DBFreeResult(hResult);

   hResult = DBSelect(hdb, L"SELECT var_name,var_value,var_description FROM config_values");
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      json_decref(output);
      context->setErrorResponse("Database failure");
      return 500;
   }

   wchar_t buffer[MAX_CONFIG_VALUE_LENGTH];
   count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      DBGetField(hResult, i, 0, name, MAX_CONFIG_VAR_NAME_LEN + 1);
      json_t *variable = index.get(name);
      if (variable == nullptr)
         continue;

      json_t *element = json_object();
      DBGetField(hResult, i, 1, buffer, MAX_CONFIG_VALUE_LENGTH);
      json_object_set_new(element, "value", json_string_w(buffer));
      DBGetField(hResult, i, 2, buffer, MAX_CONFIG_VALUE_LENGTH);
      json_object_set_new(element, "description", json_string_w(buffer));
      json_array_append_new(json_object_get(variable, "possibleValues"), element);
   }
   DBFreeResult(hResult);

   DBConnectionPoolReleaseConnection(hdb);

   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/server-config/:name
 */
int H_ServerConfigVariableDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
      return 403;

   const wchar_t *name = context->getPlaceholderValue(L"name");
   if (!ValidateConfigVariableName(name, context))
      return 400;

   return SendServerConfigVariable(context, name);
}

/**
 * Handler for PUT /v1/server-config/:name
 * Body: { "value": "..." }. Creates the variable if it does not exist, same as "create new
 * variable" action in desktop console.
 */
int H_ServerConfigVariableUpdate(Context *context)
{
   const wchar_t *name = context->getPlaceholderValue(L"name");

   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on setting server configuration variable \"%s\"", CHECK_NULL(name));
      return 403;
   }

   if (!ValidateConfigVariableName(name, context))
      return 400;

   json_t *request = context->getRequestDocument();
   if ((request == nullptr) || !json_is_object(request))
   {
      context->setErrorResponse("Request body must be a JSON object");
      return 400;
   }

   json_t *jsonValue = json_object_get(request, "value");
   if (!json_is_string(jsonValue))
   {
      context->setErrorResponse("Field \"value\" is missing or is not a string");
      return 400;
   }

   // utf8_wcharlen counts the terminating null, so the result may be equal to buffer capacity
   if (utf8_wcharlen(json_string_value(jsonValue), -1) > MAX_CONFIG_VALUE_LENGTH)
   {
      context->setErrorResponse("Configuration variable value is too long");
      return 400;
   }

   wchar_t newValue[MAX_CONFIG_VALUE_LENGTH];
   utf8_to_wchar(json_string_value(jsonValue), -1, newValue, MAX_CONFIG_VALUE_LENGTH);
   newValue[MAX_CONFIG_VALUE_LENGTH - 1] = 0;

   wchar_t oldValue[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStr(name, oldValue, MAX_CONFIG_VALUE_LENGTH, L"");

   if (!ConfigWriteStr(name, newValue, true))
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, 'T',
      L"Server configuration variable \"%s\" changed from \"%s\" to \"%s\"", name, oldValue, newValue);

   return SendServerConfigVariable(context, name);
}

/**
 * Handler for POST /v1/server-config/:name/reset
 * Resets variable to value from config.default_value.
 */
int H_ServerConfigVariableReset(Context *context)
{
   const wchar_t *name = context->getPlaceholderValue(L"name");

   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on resetting server configuration variable \"%s\"", CHECK_NULL(name));
      return 403;
   }

   if (!ValidateConfigVariableName(name, context))
      return 400;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   wchar_t defaultValue[MAX_CONFIG_VALUE_LENGTH];
   bool found = false;
   bool dbFailure = false;
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT default_value FROM config WHERE var_name=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, defaultValue, MAX_CONFIG_VALUE_LENGTH);
            found = true;
         }
         DBFreeResult(hResult);
      }
      else
      {
         dbFailure = true;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      dbFailure = true;
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (dbFailure)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   if (!found)
      return 404;

   wchar_t oldValue[MAX_CONFIG_VALUE_LENGTH];
   ConfigReadStr(name, oldValue, MAX_CONFIG_VALUE_LENGTH, L"");

   if (!ConfigWriteStr(name, defaultValue, false))
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, defaultValue, 'T',
      L"Server configuration variable \"%s\" reset to default value \"%s\"", name, defaultValue);

   return SendServerConfigVariable(context, name);
}

/**
 * Handler for DELETE /v1/server-config/:name
 */
int H_ServerConfigVariableDelete(Context *context)
{
   const wchar_t *name = context->getPlaceholderValue(L"name");

   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on deleting server configuration variable \"%s\"", CHECK_NULL(name));
      return 403;
   }

   if (!ValidateConfigVariableName(name, context))
      return 400;

   if (!ConfigDelete(name))
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"Server configuration variable \"%s\" deleted", name);
   return 204;
}
