/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Create NXSL VM from library script
 */
NXSL_VM NXCORE_EXPORTABLE *CreateServerScriptVM(const TCHAR *name)
{
   return s_scriptLibrary.createVM(name, new NXSL_ServerEnv());
}

/**
 * Load scripts from database
 */
void LoadScripts()
{
   DB_RESULT hResult;
   NXSL_LibraryScript *pScript;
   TCHAR buffer[MAX_DB_STRING];

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT script_id,guid,script_name,script_code FROM script_library"));
   if (hResult != NULL)
   {
      int numRows = DBGetNumRows(hResult);
      for(int i = 0; i < numRows; i++)
      {
         pScript = new NXSL_LibraryScript(DBGetFieldULong(hResult, i, 0), DBGetFieldGUID(hResult, i, 1),
                  DBGetField(hResult, i, 2, buffer, MAX_DB_STRING), DBGetField(hResult, i, 3, NULL, 0));
         if (pScript->isValid())
         {
            s_scriptLibrary.addScript(pScript);
            DbgPrintf(2, _T("Script %s added to library"), pScript->getName());
         }
         else
         {
            nxlog_write(MSG_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss",
                        pScript->getId(), pScript->getName(), pScript->getError());
         }
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Load script name and code from database
 */
NXSL_LibraryScript *LoadScriptFromDatabase(UINT32 id)
{
   NXSL_LibraryScript *script = NULL;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id,guid,script_name,script_code FROM script_library WHERE script_id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR buffer[MAX_DB_STRING];
            script = new NXSL_LibraryScript(DBGetFieldULong(hResult, 0, 0), DBGetFieldGUID(hResult, 0, 1),
                     DBGetField(hResult, 0, 2, buffer, MAX_DB_STRING), DBGetField(hResult, 0, 3, NULL, 0));
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
void ReloadScript(UINT32 id)
{
   NXSL_LibraryScript *script = LoadScriptFromDatabase(id);
   if (script == NULL)
   {
      DbgPrintf(3, _T("ReloadScript: failed to load script with ID %d from database"), (int)id);
      return;
   }

   s_scriptLibrary.lock();
   s_scriptLibrary.deleteScript(id);
   if (script->isValid())
   {
      s_scriptLibrary.addScript(script);
   }
   else
   {
      nxlog_write(MSG_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", id, script->getName(), script->getError());
      delete script;
   }
   s_scriptLibrary.unlock();
}

/**
 * Check if script ID is valid
 */
bool IsValidScriptId(UINT32 id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool valid = IsDatabaseRecordExist(hdb, _T("script_library"), _T("script_id"), id);
   DBConnectionPoolReleaseConnection(hdb);
   return valid;
}

/**
 * Resolve script name to ID
 */
UINT32 ResolveScriptName(const TCHAR *name)
{
   UINT32 id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id FROM script_library WHERE script_name=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
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
UINT32 ResolveScriptGuid(const uuid& guid)
{
   UINT32 id = 0;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_id FROM script_library WHERE guid=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
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
UINT32 UpdateScript(const NXCPMessage *request, UINT32 *scriptId)
{
   UINT32 rcc = RCC_DB_FAILURE;

   TCHAR scriptName[MAX_DB_STRING];
   request->getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);
   TCHAR *scriptSource = request->getFieldAsString(VID_SCRIPT_CODE);

   if (scriptSource == NULL)
      return RCC_INVALID_REQUEST;

   if (IsValidScriptName(scriptName))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt;

      UINT32 id = ResolveScriptName(scriptName);
      uuid guid;
      if (id == 0) // Create new script
      {
         guid = uuid::generate();
         id = CreateUniqueId(IDG_SCRIPT);
         hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id,guid) VALUES (?,?,?,?)"));
      }
      else // Update existing script
         hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, scriptName, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, scriptSource, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, id);
         if (!guid.isNull())
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, (const TCHAR *)guid.toString(), DB_BIND_STATIC);

         if (DBExecute(hStmt))
         {
            ReloadScript(id);
            *scriptId = id;
            rcc = RCC_SUCCESS;
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
      rcc = RCC_INVALID_SCRIPT_NAME;

   free(scriptSource);

   return rcc;
}

/*
 * Rename script
 */
UINT32 RenameScript(const NXCPMessage *request)
{
   UINT32 rcc = RCC_DB_FAILURE;
   UINT32 id = request->getFieldAsUInt32(VID_SCRIPT_ID);

   if (IsValidScriptId(id))
   {
      TCHAR scriptName[MAX_DB_STRING];
      request->getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);
      if (IsValidScriptName(scriptName))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=? WHERE script_id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, scriptName, DB_BIND_STATIC);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, id);
            if (DBExecute(hStmt))
            {
               ReloadScript(id);
               rcc = RCC_SUCCESS;
            }
            DBFreeStatement(hStmt);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
         rcc = RCC_INVALID_SCRIPT_NAME;
   }
   else
      rcc = RCC_INVALID_SCRIPT_ID;

   return rcc;
}

/**
 * Delete script
 */
UINT32 DeleteScript(const NXCPMessage *request)
{
   UINT32 rcc = RCC_DB_FAILURE;
   UINT32 id = request->getFieldAsUInt32(VID_SCRIPT_ID);

   if (IsValidScriptId(id))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM script_library WHERE script_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         if (DBExecute(hStmt))
         {
            s_scriptLibrary.lock();
            s_scriptLibrary.deleteScript(id);
            s_scriptLibrary.unlock();
            rcc = RCC_SUCCESS;
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
      rcc = RCC_INVALID_SCRIPT_ID;

   return rcc;
}

/**
 * Create export record for library script
 */
void CreateScriptExportRecord(String &xml, UINT32 id)
{
   NXSL_LibraryScript *script = LoadScriptFromDatabase(id);
   if (script == NULL)
   {
      DbgPrintf(3, _T("CreateScriptExportRecord: failed to load script with ID %d from database"), (int)id);
      return;
   }

   xml.append(_T("\t\t<script id=\""));
   xml.append(id);
   xml.append(_T("\">\n"));
   xml.append(_T("\t\t\t<guid>"));
   xml.append((const TCHAR *)script->getGuid().toString());
   xml.append(_T("</guid>\n"));
   xml.append(_T("\t\t\t<name>"));
   xml.append(EscapeStringForXML2(script->getName()));
   xml.append(_T("</name>\n"));
   xml.append(_T("\t\t\t<code>"));
   xml.append(EscapeStringForXML2(script->getCode()));
   xml.append(_T("</code>\n\t\t</script>\n"));
}

/**
 * Import script
 */
void ImportScript(ConfigEntry *config)
{
   const TCHAR *name = config->getSubEntryValue(_T("name"));
   if (name == NULL)
   {
      DbgPrintf(4, _T("ImportScript: name missing"));
      return;
   }

   uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (guid.isNull())
   {
      guid = uuid::generate();
      DbgPrintf(4, _T("ImportScript: GUID not found in config, generated GUID %s for script \"%s\""), (const TCHAR *)guid.toString(), name);
   }

   const TCHAR *code = config->getSubEntryValue(_T("code"));
   if (code == NULL)
   {
      DbgPrintf(4, _T("ImportScript: code missing"));
      return;
   }

   if (IsValidScriptName(name))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt;

      bool newScript = false;
      UINT32 id = ResolveScriptGuid(guid);
      if (id == 0) // Create new script
      {
         id = CreateUniqueId(IDG_SCRIPT);
         hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id,guid) VALUES (?,?,?,?)"));
         newScript = true;
      }
      else // Update existing script
         hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, id);
         if (newScript)
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, guid);

         if (DBExecute(hStmt))
            ReloadScript(id);
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
}

/**
 * Execute library script from scheduler
 */
void ExecuteScheduledScript(const ScheduledTaskParameters *param)
{
   TCHAR name[256];
   nx_strncpy(name, param->m_params, 256);
   Trim(name);

   ObjectArray<NXSL_Value> args(16, 16, false);

   Node *object = (Node *)FindObjectById(param->m_objectId, OBJECT_NODE);
   if (object != NULL)
   {
      if (!object->checkAccessRights(param->m_userId, OBJECT_ACCESS_CONTROL))
      {
			nxlog_debug(4, _T("ExecuteScheduledScript(%s): access denied for userId %d object \"%s\" [%d]"),
			            param->m_params, param->m_userId, object->getName(), object->getId());
         return;
      }
   }

   // Can be in form parameter(arg1, arg2, ... argN)
   TCHAR *p = _tcschr(name, _T('('));
   if (p != NULL)
   {
      if (name[_tcslen(name) - 1] != _T(')'))
         return;
      name[_tcslen(name) - 1] = 0;

      if (!ParseValueList(&p, args))
      {
         // argument parsing error
         if (object != NULL)
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): argument parsing error (object \"%s\" [%d])"),
                        param->m_params, object->getName(), object->getId());
         else
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): argument parsing error (not attached to object)"), param->m_params);
         args.setOwner(true);
         return;
      }
   }

   NXSL_VM *vm = s_scriptLibrary.createVM(name, new NXSL_ServerEnv());
   if (vm != NULL)
   {
      if(object != NULL)
      {
         vm->setGlobalVariable(_T("$object"), object->createNXSLObject());
         if (object->getObjectClass() == OBJECT_NODE)
            vm->setGlobalVariable(_T("$node"), object->createNXSLObject());
      }
      if (vm->run(&args))
      {
         if (object != NULL)
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): Script executed successfully on object \"%s\" [%d]"),
                        param->m_params, object->getName(), object->getId());
         else
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): Script executed successfully (not attached to object)"), param->m_params);
      }
      else
      {
         if (object != NULL)
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): Script execution error on object \"%s\" [%d]: %s"),
                        param->m_params, object->getName(), object->getId(), vm->getErrorText());
         else
            nxlog_debug(4, _T("ExecuteScheduledScript(%s): Script execution error (not attached to object): %s"),
                        param->m_params, vm->getErrorText());
      }
      delete vm;
   }
   else
   {
      args.setOwner(true);
   }
}

/**
 * Parse value list
 */
bool ParseValueList(TCHAR **start, ObjectArray<NXSL_Value> &args)
{
   TCHAR *p = *start;

   *p = 0;
   p++;

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
               args.add(new NXSL_Value(s));
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(new NXSL_Value(s));
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
               args.add(new NXSL_Value(s));
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
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(new NXSL_Value(s));
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
               ObjectArray<NXSL_Value> elements(16, 16, false);
               if (ParseValueList(&p, elements))
               {
                  NXSL_Array *array = new NXSL_Array();
                  for(int i = 0; i < elements.size(); i++)
                  {
                     array->set(i, elements.get(i));
                  }
                  args.add(new NXSL_Value(array));
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
 * Execute server startup scripts
 */
void ExecuteStartupScripts()
{
   TCHAR path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   _tcscat(path, SDIR_SCRIPTS);

   int count = 0;
   nxlog_debug(1, _T("Running startup scripts from %s"), path);
   _TDIR *dir = _topendir(path);
   if (dir != NULL)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != NULL)
      {
         if (MatchString(_T("*.nxsl"), f->d_name, FALSE))
         {
            count++;
            _tcscpy(&path[insPos], f->d_name);

            UINT32 size;
            char *source = (char *)LoadFile(path, &size);
            if (source != NULL)
            {
               TCHAR errorText[1024];
#ifdef UNICODE
               WCHAR *wsource = WideStringFromUTF8String(source);
               NXSL_VM *vm = NXSLCompileAndCreateVM(wsource, errorText, 1024, new NXSL_ServerEnv());
               free(wsource);
#else
               NXSL_VM *vm = NXSLCompileAndCreateVM(source, errorText, 1024, new NXSL_ServerEnv());
#endif
               free(source);
               if (vm != NULL)
               {
                  if (vm->run())
                  {
                     nxlog_debug(1, _T("Startup script %s completed successfully"), f->d_name);
                  }
                  else
                  {
                     nxlog_debug(1, _T("Runtime error in startup script %s: %s"), f->d_name, vm->getErrorText());
                  }
                  delete vm;
               }
               else
               {
                  nxlog_debug(1, _T("Cannot compile startup script %s (%s)"), f->d_name, errorText);
               }
            }
            else
            {
               nxlog_debug(1, _T("Cannot load startup script %s"), f->d_name);
            }
         }
      }
      _tclosedir(dir);
   }
   nxlog_debug(1, _T("%d startup scripts processed"), count);
}
