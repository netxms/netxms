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
** File: script.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG_BASE        _T("scripts")
#define DEBUG_TAG_SCHEDULED   DEBUG_TAG_BASE _T(".scheduled")

void RegisterAIFunctionScriptHandler(const NXSL_LibraryScript *script, bool runtimeChange);
void UnregisterAIFunctionScriptHandler(const NXSL_LibraryScript *script);

/**
 * Script error counter
 */
static VolatileCounter s_scriptErrorCount = 0;

/**
 * Post script error event to system event queue.
 *
 * @param context script execution context
 * @param objectId NetObj ID
 * @param dciId DCI ID
 * @param errorText error text
 * @param nameFormat script name as formatted string
 */
static void ReportScriptErrorInternal(const TCHAR *context, const NetObj *object, uint32_t dciId, const TCHAR *errorText, const TCHAR *nameFormat, va_list args)
{
   uint32_t count = static_cast<uint32_t>(InterlockedIncrement(&s_scriptErrorCount));
   if (count >= 1000)
   {
      if (count == 1000)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_BASE, _T("Too many script errors - script error reporting temporarily disabled"));
         PostSystemEvent(EVENT_TOO_MANY_SCRIPT_ERRORS, g_dwMgmtNode);
      }
      return;
   }

   TCHAR name[1024];
   _vsntprintf(name, 1024, nameFormat, args);

   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_BASE, _T("Script error (object=%s objectId=%u dciId=%u context=%s name=%s): %s"),
         (object != nullptr) ? object->getName() : _T("NONE"), (object != nullptr) ? object->getId() : 0, dciId, context, name, errorText);

   EventBuilder(EVENT_SCRIPT_ERROR, g_dwMgmtNode)
      .dci(dciId)
      .param(_T("scriptName"), name)
      .param(_T("errorText"), errorText)
      .param(_T("dciId"), dciId)
      .param(_T("objectId"), (object != nullptr) ? object->getId() : 0)
      .param(_T("context"), context)
      .post();
}

/**
 * Post script error event to system event queue.
 *
 * @param context script execution context
 * @param objectId NetObj ID
 * @param dciId DCI ID
 * @param errorText error text
 * @param nameFormat script name as formatted string
 */
void NXCORE_EXPORTABLE ReportScriptError(const TCHAR *context, const NetObj *object, uint32_t dciId, const TCHAR *errorText, const TCHAR *nameFormat, ...)
{
   va_list args;
   va_start(args, nameFormat);
   ReportScriptErrorInternal(context, object, dciId, errorText, nameFormat, args);
   va_end(args);
}

/**
 * Reset script error count
 */
void ResetScriptErrorEventCounter()
{
   s_scriptErrorCount = 0;
   ThreadPoolScheduleRelative(g_mainThreadPool, 600000, ResetScriptErrorEventCounter);
}

/**
 * Script library
 */
static NXSL_Library s_scriptLibrary;

/**
 * Get server's script library
 */
NXSL_Library NXCORE_EXPORTABLE *GetServerScriptLibrary()
{
   return &s_scriptLibrary;
}

/**
 * Setup server script VM. Returns pointer to same VM for convenience.
 */
NXSL_VM NXCORE_EXPORTABLE *SetupServerScriptVM(NXSL_VM *vm, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo)
{
   if ((vm == nullptr) || (object == nullptr))
      return vm;

   vm->setGlobalVariable("$object", object->createNXSLObject(vm));
   if (object->getObjectClass() == OBJECT_NODE)
      vm->setGlobalVariable("$node", object->createNXSLObject(vm));
   else if (object->getObjectClass() == OBJECT_NETWORKMAP)
      vm->setGlobalVariable("$map", object->createNXSLObject(vm));
   vm->setGlobalVariable("$isCluster", vm->createValue(object->getObjectClass() == OBJECT_CLUSTER));

   if (dciInfo != nullptr)
      vm->setGlobalVariable("$dci", vm->createValue(vm->createObject(&g_nxslDciClass, new shared_ptr<DCObjectInfo>(dciInfo))));

   return vm;
}

/**
 * Server script validator
 */
static ScriptVMFailureReason ScriptValidator(NXSL_LibraryScript *script)
{
   if (!script->isValid())
      return ScriptVMFailureReason::SCRIPT_VALIDATION_ERROR;
   if (script->isEmpty())
      return ScriptVMFailureReason::SCRIPT_IS_EMPTY;
   return ScriptVMFailureReason::SUCCESS;
}

/**
 * Create NXSL VM from library script. Created VM will take ownership of DCI descriptor.
 */
ScriptVMHandle NXCORE_EXPORTABLE CreateServerScriptVM(const TCHAR *name, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo)
{
   ScriptVMHandle vm = s_scriptLibrary.createVM(name, [] { return new NXSL_ServerEnv(); }, ScriptValidator);
   if (vm.isValid())
      SetupServerScriptVM(vm, object, dciInfo);
   return vm;
}

/**
 * Create NXSL VM from compiled script. Created VM will take ownership of DCI descriptor.
 */
ScriptVMHandle NXCORE_EXPORTABLE CreateServerScriptVM(const NXSL_Program *script, const shared_ptr<NetObj>& object, const shared_ptr<DCObjectInfo>& dciInfo)
{
   if (script->isEmpty())
      return ScriptVMHandle(ScriptVMFailureReason::SCRIPT_IS_EMPTY);

   NXSL_VM *vm = new NXSL_VM(new NXSL_ServerEnv());
   if (!vm->load(script))
   {
      delete vm;
      return ScriptVMHandle(ScriptVMFailureReason::SCRIPT_LOAD_ERROR);
   }

   return ScriptVMHandle(SetupServerScriptVM(vm, object, dciInfo));
}

/**
 * Compile server script and report error on failure
 */
NXSL_Program NXCORE_EXPORTABLE *CompileServerScript(const TCHAR *source, const TCHAR *context, const NetObj *object, uint32_t dciId, const TCHAR *nameFormat, ...)
{
   NXSL_ServerEnv env;
   NXSL_CompilationDiagnostic diag;
   NXSL_Program *output = NXSLCompile(source, &env, &diag);
   if (output == nullptr)
   {
      va_list args;
      va_start(args, nameFormat);
      ReportScriptErrorInternal(context, object, dciId, diag.errorText, nameFormat, args);
      va_end(args);
   }
   else if (!diag.warnings.isEmpty())
   {
      TCHAR name[1024];
      va_list args;
      va_start(args, nameFormat);
      _vsntprintf(name, 1024, nameFormat, args);
      va_end(args);
      for(NXSL_CompilationWarning *w : diag.warnings)
      {
         nxlog_debug_tag(DEBUG_TAG_BASE, 5, _T("Compilation warning in script %s line %d: %s"), name, w->lineNumber, w->message.cstr());
      }
   }
   return output;
}

/**
 * Load scripts from database
 */
void LoadScripts()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT script_id,guid,script_name,script_code FROM script_library"));
   if (hResult != nullptr)
   {
      NXSL_ServerEnv env;
      int numRows = DBGetNumRows(hResult);
      for(int i = 0; i < numRows; i++)
      {
         TCHAR buffer[MAX_DB_STRING];
         auto script = new NXSL_LibraryScript(DBGetFieldULong(hResult, i, 0), DBGetFieldGUID(hResult, i, 1),
                  DBGetField(hResult, i, 2, buffer, MAX_DB_STRING), DBGetField(hResult, i, 3, nullptr, 0), &env);
         if (!script->isValid())
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_BASE, _T("Error compiling library script %s [%u] (%s)"),
                     script->getName(), script->getId(), script->getError());
         }
         s_scriptLibrary.addScript(script);
         nxlog_debug_tag(DEBUG_TAG_BASE,  2, _T("Script %s added to library"), script->getName());
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Validate all scripts in script library
 */
void ValidateScripts()
{
   s_scriptLibrary.lock();
   s_scriptLibrary.forEach([] (const NXSL_LibraryScript *script) {
      if (!script->isValid())
         ReportScriptError(_T("ValidateScripts"), nullptr, 0, script->getError(), _T("%s"), script->getName());
   });
   s_scriptLibrary.unlock();
}

/**
 * Load script name and code from database
 */
static NXSL_LibraryScript *LoadScriptFromDatabase(uint32_t id)
{
   NXSL_LibraryScript *script = nullptr;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id,guid,script_name,script_code FROM script_library WHERE script_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR name[MAX_DB_STRING];
            NXSL_ServerEnv env;
            script = new NXSL_LibraryScript(DBGetFieldULong(hResult, 0, 0), DBGetFieldGUID(hResult, 0, 1),
                     DBGetField(hResult, 0, 2, name, MAX_DB_STRING), DBGetField(hResult, 0, 3, nullptr, 0), &env);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return script;
}

/**
 * Reload script from database
 */
void ReloadScript(uint32_t id)
{
   NXSL_LibraryScript *script = LoadScriptFromDatabase(id);
   if (script == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BASE,  3, _T("ReloadScript: failed to load script with ID %d from database"), (int)id);
      return;
   }

   if (!script->isValid())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_BASE, _T("Error compiling library script %s [%u] (%s)"), script->getName(), id, script->getError());
   }

   s_scriptLibrary.lock();
   NXSL_LibraryScript *s = s_scriptLibrary.findScript(id);
   if ((s != nullptr) && s->getMetadataEntryAsBoolean(L"ai_tool"))
   {
      UnregisterAIFunctionScriptHandler(s);
   }
   s_scriptLibrary.deleteScript(id);
   s_scriptLibrary.addScript(script);
   if (script->getMetadataEntryAsBoolean(L"ai_tool"))
   {
      RegisterAIFunctionScriptHandler(script, true);
   }
   s_scriptLibrary.unlock();
}

/**
 * Check if script ID is valid
 */
bool IsValidScriptId(uint32_t id)
{
   s_scriptLibrary.lock();
   bool valid = (s_scriptLibrary.findScript(id) != nullptr);
   s_scriptLibrary.unlock();
   return valid;
}

/**
 * Get script name by ID
 */
bool GetScriptName(uint32_t scriptId, TCHAR *buffer, size_t size)
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_name FROM script_library WHERE script_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, scriptId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, buffer, size);
            success = true;
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Resolve script name to ID
 */
uint32_t ResolveScriptName(const TCHAR *name)
{
   uint32_t id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id FROM script_library WHERE script_name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Resolve script GUID to ID
 */
static uint32_t ResolveScriptGuid(const uuid& guid)
{
   uint32_t id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id FROM script_library WHERE guid=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Update or create new script
 */
uint32_t UpdateScript(const NXCPMessage& request, uint32_t *scriptId, ClientSession *session)
{
   TCHAR *scriptSource = request.getFieldAsString(VID_SCRIPT_CODE);
   if (scriptSource == nullptr)
      return RCC_INVALID_REQUEST;

   TCHAR scriptName[MAX_DB_STRING];
   request.getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);

   uint32_t rcc = RCC_DB_FAILURE;
   if (IsValidScriptName(scriptName))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      uint32_t id = ResolveScriptName(scriptName);

      bool newScript;
      TCHAR *oldSource = nullptr;
      DB_STATEMENT hStmt;
      if (id == 0) // Create new script
      {
         newScript = true;
         id = CreateUniqueId(IDG_SCRIPT);
         hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id,guid) VALUES (?,?,?,?)"));
      }
      else // Update existing script
      {
         newScript = false;

         hStmt = DBPrepare(hdb, _T("SELECT script_code FROM script_library WHERE script_id=?"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != nullptr)
            {
               oldSource = DBGetField(hResult, 0, 0, nullptr, 0);
               DBFreeResult(hResult);
            }
            DBFreeStatement(hStmt);
         }

         hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));
      }

      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, scriptName, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, scriptSource, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, id);
         if (newScript)
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, uuid::generate());

         if (DBExecute(hStmt))
         {
            ReloadScript(id);
            *scriptId = id;
            rcc = RCC_SUCCESS;
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);

      if (rcc == RCC_SUCCESS)
      {
         session->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldSource, scriptSource, 'T',
                  _T("Library script %s [%u] %s"), scriptName, id, newScript ? _T("created") : _T("modified"));
      }
      MemFree(oldSource);
   }
   else
   {
      rcc = RCC_INVALID_SCRIPT_NAME;
   }

   MemFree(scriptSource);
   return rcc;
}

/*
 * Rename script
 */
uint32_t RenameScript(uint32_t scriptId, const TCHAR *newName)
{
   uint32_t rcc;
   if (IsValidScriptId(scriptId))
   {
      if (IsValidScriptName(newName))
      {
         rcc = RCC_DB_FAILURE;
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=? WHERE script_id=?"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, newName, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, scriptId);
            if (DBExecute(hStmt))
            {
               ReloadScript(scriptId);
               rcc = RCC_SUCCESS;
            }
            DBFreeStatement(hStmt);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         rcc = RCC_INVALID_SCRIPT_NAME;
      }
   }
   else
   {
      rcc = RCC_INVALID_SCRIPT_ID;
   }
   return rcc;
}

/**
 * Delete script
 */
uint32_t DeleteScript(uint32_t scriptId)
{
   uint32_t rcc;
   if (IsValidScriptId(scriptId))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (ExecuteQueryOnObject(hdb, scriptId, _T("DELETE FROM script_library WHERE script_id=?")))
      {
         s_scriptLibrary.lock();
         NXSL_LibraryScript *s = s_scriptLibrary.findScript(scriptId);
         if ((s != nullptr) && s->getMetadataEntryAsBoolean(L"ai_tool"))
         {
            UnregisterAIFunctionScriptHandler(s);
         }
         s_scriptLibrary.deleteScript(scriptId);
         s_scriptLibrary.unlock();
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      rcc = RCC_INVALID_SCRIPT_ID;
   }
   return rcc;
}

/**
 * Create export record for library script
 */
void CreateScriptExportRecord(TextFileWriter& xml, uint32_t id)
{
   NXSL_LibraryScript *script = LoadScriptFromDatabase(id);
   if (script == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_BASE, 3, _T("CreateScriptExportRecord: failed to load script with ID %u from database"), id);
      return;
   }

   xml.appendUtf8String("\t\t<script id=\"");
   xml.append(id);
   xml.appendUtf8String("\">\n\t\t\t<guid>");
   xml.append(script->getGuid());
   xml.appendUtf8String("</guid>\n\t\t\t<name>");
   xml.append(EscapeStringForXML2(script->getName()));
   xml.appendUtf8String("</name>\n\t\t\t<code>");
   xml.append(EscapeStringForXML2(script->getSourceCode()));
   xml.appendUtf8String("</code>\n\t\t</script>\n");

   delete script;
}

/**
 * Import script
 */
void ImportScript(ConfigEntry *config, bool overwrite, ImportContext *context, bool nxslV5)
{
   const TCHAR *name = config->getSubEntryValue(_T("name"));
   if (name == nullptr)
   {
      context->log(NXLOG_ERROR, _T("ImportScript()"), _T("Script name missing"));
      return;
   }

   uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (guid.isNull())
   {
      guid = uuid::generate();
      context->log(NXLOG_INFO, _T("ImportScript()"), _T("GUID script \"%s\" not found in configuration file, generated new GUID %s"), name, guid.toString().cstr());
   }

   if (config->getSubEntryValue(_T("code")) == nullptr)
   {
      context->log(NXLOG_ERROR, _T("ImportScript()"), _T("Missing source code for script \"%s\""), name);
      return;
   }

   StringBuffer code;
   if (nxslV5)
      code = config->getSubEntryValue(_T("code"));
   else
   {
      code = NXSLConvertToV5(config->getSubEntryValue(_T("code")));
   }

   if (IsValidScriptName(name))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt;

      bool newScript = false;
      uint32_t id = ResolveScriptGuid(guid);
      if (id == 0) // Create new script
      {
         id = CreateUniqueId(IDG_SCRIPT);
         hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id,guid) VALUES (?,?,?,?)"));
         newScript = true;
      }
      else // Update existing script
      {
         if (!overwrite)
         {
            DBConnectionPoolReleaseConnection(hdb);
            return;
         }
         hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));
      }

      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, id);
         if (newScript)
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, guid);

         if (DBExecute(hStmt))
         {
            context->log(NXLOG_INFO, _T("ImportScript()"), _T("Script \"%s\" successfully imported"), name);
            ReloadScript(id);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      context->log(NXLOG_ERROR, _T("ImportScript()"), _T("Script name \"%s\" is invalid"), name);
   }
}

/**
 * Execute library script from scheduler
 */
void ExecuteScheduledScript(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   TCHAR name[256];
   TCHAR *argListStart = _tcschr(parameters->m_persistentData, _T('('));
   if (argListStart != nullptr)
   {
      size_t l = argListStart - parameters->m_persistentData;
      if (l > 255)
         l = 255;
      memcpy(name, parameters->m_persistentData, l * sizeof(TCHAR));
      name[l] = 0;
   }
   else
   {
      _tcslcpy(name, parameters->m_persistentData, 256);
   }
   Trim(name);

   shared_ptr<NetObj> object = FindObjectById(parameters->m_objectId);
   if ((object != nullptr) && !object->checkAccessRights(parameters->m_userId, OBJECT_ACCESS_CONTROL))
   {
      nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): access denied for userId %d object \"%s\" [%d]"),
               name, parameters->m_userId, object->getName(), object->getId());
      return;
   }

   NXSL_VM *vm = CreateServerScriptVM(name, object);
   if (vm == nullptr)
   {
      if (object != nullptr)
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): cannot create VM (object \"%s\" [%d])"),
                  name, object->getName(), object->getId());
      else
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): cannot create VM (not attached to object)"), name);
      return;
   }

   ObjectRefArray<NXSL_Value> args(16, 16);

   // Can be in form script(arg1, arg2, ... argN)
   if (argListStart != nullptr)
   {
      TCHAR parameters[4096];
      _tcslcpy(parameters, argListStart, 4096);

      TCHAR *p = parameters;
      if (!ParseValueList(vm, &p, args, true))
      {
         // argument parsing error
         if (object != nullptr)
            nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): argument parsing error (object \"%s\" [%d])"),
                     name, object->getName(), object->getId());
         else
            nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): argument parsing error (not attached to object)"), name);
         delete vm;
         return;
      }
   }

   if (vm->run(args))
   {
      if (object != nullptr)
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): Script executed successfully on object \"%s\" [%d]"),
                  name, object->getName(), object->getId());
      else
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): Script executed successfully (not attached to object)"), name);
   }
   else
   {
      if (object != nullptr)
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): Script execution error on object \"%s\" [%d]: %s"),
                  name, object->getName(), object->getId(), vm->getErrorText());
      else
         nxlog_debug_tag(DEBUG_TAG_SCHEDULED, 4, _T("ExecuteScheduledScript(%s): Script execution error (not attached to object): %s"),
                  name, vm->getErrorText());
   }
   delete vm;
}

/**
 * Parse value list. This function may modify data pointed by "start".
 */
bool NXCORE_EXPORTABLE ParseValueList(NXSL_VM *vm, TCHAR **start, ObjectRefArray<NXSL_Value> &args, bool hasBrackets)
{
   TCHAR *p = *start;

   if (hasBrackets)
   {
      *p = 0;
      p++;
   }

   // Check for empty argument list
   while(_istspace(*p))
      p++;
   if ((*p == 0) || (hasBrackets && (*p == ')')))
   {
      *start = p - 1;
      return true;
   }

   TCHAR *s = p;
   int state = 1; // normal text

   for(; state > 0; p++)
   {
      switch(*p)
      {
         case '"':
            if (state == 1)
            {
               state = 2;
               s = p + 1;
            }
            else
            {
               state = 3;
               *p = 0;
               args.add(vm->createValue(s));
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(vm->createValue(s));
               s = p + 1;
            }
            else if (state == 3)
            {
               state = 1;
               s = p + 1;
            }
            break;
         case 0:
            if (state == 1)
            {
               Trim(s);
               args.add(vm->createValue(s));
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            else
            {
               state = -1; // error
            }
            break;
         case ' ':
         case '\t':
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(vm->createValue(s));
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            break;
         case '\\':
            if (state == 2)
            {
               memmove(p, p + 1, _tcslen(p) * sizeof(TCHAR));
               switch(*p)
               {
                  case 'r':
                     *p = '\r';
                     break;
                  case 'n':
                     *p = '\n';
                     break;
                  case 't':
                     *p = '\t';
                     break;
                  default:
                     break;
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         case '%':
            if ((state == 1) && (*(p + 1) == '('))
            {
               p++;
               ObjectRefArray<NXSL_Value> elements(16, 16);
               if (ParseValueList(vm, &p, elements, true))
               {
                  NXSL_Array *array = new NXSL_Array(vm);
                  for(int i = 0; i < elements.size(); i++)
                  {
                     array->set(i, elements.get(i));
                  }
                  args.add(vm->createValue(array));
                  state = 3;
               }
               else
               {
                  state = -1;
                  elements.clear();
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         default:
            if (state == 3)
               state = -1;
            break;
      }
   }

   *start = p - 1;
   return (state != -1);
}

/**
 * Extract entry point separated by dot or slash from script name.
 * Will update "name" argument to have only script name and place entry point
 * into "entryPoint". Will set "entryPoint" to empty string if entry point
 * is not part of script name.
 */
void NXCORE_EXPORTABLE ExtractScriptEntryPoint(wchar_t *name, char *entryPoint)
{
   wchar_t *s = name;
   while(*s != 0)
   {
      if ((*s == L'.') || (*s == L'/'))
         break;
      s++;
   }
   if (*s != 0)
   {
      *s = 0;
      s++;
      TrimW(s);
      wchar_to_utf8(s, -1, entryPoint, MAX_IDENTIFIER_LENGTH);
      entryPoint[MAX_IDENTIFIER_LENGTH - 1] = 0;
   }
   else
   {
      entryPoint[0] = 0;
   }
   TrimW(name);
}

/**
 * Execute server startup scripts
 */
void ExecuteStartupScripts()
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   _tcscat(path, SDIR_SCRIPTS);

   int count = 0;
   nxlog_debug_tag(DEBUG_TAG_BASE,  1, _T("Running startup scripts from %s"), path);
   _TDIR *dir = _topendir(path);
   if (dir != nullptr)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != nullptr)
      {
         if (MatchString(_T("*.nxsl"), f->d_name, false))
         {
            count++;
            _tcscpy(&path[insPos], f->d_name);

            TCHAR *source = NXSLLoadFile(path);
            if (source != nullptr)
            {
               NXSL_CompilationDiagnostic diag;
               NXSL_VM *vm = NXSLCompileAndCreateVM(source, new NXSL_ServerEnv(), &diag);
               MemFree(source);
               if (vm != nullptr)
               {
                  if (vm->run())
                  {
                     nxlog_debug_tag(DEBUG_TAG_BASE,  1, _T("Startup script %s completed successfully"), f->d_name);
                  }
                  else
                  {
                     nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_BASE, _T("Runtime error in startup script %s (%s)"), f->d_name, vm->getErrorText());
                  }
                  delete vm;
               }
               else
               {
                  nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_BASE, _T("Cannot compile startup script %s (%s)"), f->d_name, diag.errorText.cstr());
               }
            }
            else
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_BASE, _T("Cannot load startup script %s"), f->d_name);
            }
         }
      }
      _tclosedir(dir);
   }
   nxlog_debug_tag(DEBUG_TAG_BASE,  1, _T("%d startup scripts processed"), count);
}
