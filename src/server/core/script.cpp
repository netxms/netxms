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
** File: script.cpp
**
**/

#include "nxcore.h"

/**
 * Script library
 */
NXSL_Library *g_pScriptLibrary = NULL;

/**
 * Load scripts from database
 */
void LoadScripts()
{
   DB_RESULT hResult;
   NXSL_Program *pScript;
   TCHAR *pszCode, szError[1024], szBuffer[MAX_DB_STRING];
   int i, nRows;

   g_pScriptLibrary = new NXSL_Library;
   hResult = DBSelect(g_hCoreDB, _T("SELECT script_id,script_name,script_code FROM script_library"));
   if (hResult != NULL)
   {
      nRows = DBGetNumRows(hResult);
      for(i = 0; i < nRows; i++)
      {
         pszCode = DBGetField(hResult, i, 2, NULL, 0);
         pScript = (NXSL_Program *)NXSLCompile(pszCode, szError, 1024, NULL);
         free(pszCode);
         if (pScript != NULL)
         {
            g_pScriptLibrary->addScript(DBGetFieldULong(hResult, i, 0),
                                        DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING), pScript);
				DbgPrintf(2, _T("Script %s added to library"), szBuffer);
         }
         else
         {
            nxlog_write(MSG_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss",
                        DBGetFieldULong(hResult, i, 0),
                        DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING), szError);
         }
      }
      DBFreeResult(hResult);
   }
}

/**
 * Load script name and code from database
 */
static bool LoadScriptFromDatabase(UINT32 id, TCHAR **name, TCHAR **code)
{
   bool success = false;

   *name = NULL;
   *code = NULL;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT script_name,script_code FROM script_library WHERE script_id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            *name = DBGetField(hResult, 0, 0, NULL, 0);
            *code = DBGetField(hResult, 0, 1, NULL, 0);
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
 * Reload script from database
 */
void ReloadScript(UINT32 id)
{
   TCHAR *name, *code;
   if (!LoadScriptFromDatabase(id, &name, &code))
   {
      DbgPrintf(3, _T("ReloadScript: failed to load script with ID %d from database"), (int)id);
      return;
   }

   TCHAR error[1024];
   NXSL_Program *script = NXSLCompile(code, error, 1024, NULL);
   free(code);

   g_pScriptLibrary->lock();
   g_pScriptLibrary->deleteScript(id);
   if (script != NULL)
   {
      g_pScriptLibrary->addScript(id, name, script);
   }
   else
   {
      nxlog_write(MSG_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", id, name, error);
   }
   g_pScriptLibrary->unlock();
   free(name);
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
         id = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return id;
}

/**
 * Create export record for library script
 */
void CreateScriptExportRecord(String &xml, UINT32 id)
{
   TCHAR *name, *code;
   if (!LoadScriptFromDatabase(id, &name, &code))
   {
      DbgPrintf(3, _T("CreateScriptExportRecord: failed to load script with ID %d from database"), (int)id);
      return;
   }

   xml.append(_T("\t\t<script id=\""));
   xml.append(id);
   xml.append(_T("\">\n"));
   xml.append(_T("\t\t\t<name>"));
   xml.append(EscapeStringForXML2(name));
   xml.append(_T("</name>\n"));
   xml.append(_T("\t\t\t<code>"));
   xml.append(EscapeStringForXML2(code));
   xml.append(_T("</code>\n\t\t</script>\n"));

   free(name);
   free(code);
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

   const TCHAR *code = config->getSubEntryValue(_T("code"));
   if (code == NULL)
   {
      DbgPrintf(4, _T("ImportScript: code missing"));
      return;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt;
   UINT32 id = ResolveScriptName(name);
   if (id == 0)
   {
      id = CreateUniqueId(IDG_SCRIPT);
      hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id) VALUES (?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));
   }

   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);

   ReloadScript(id);
}

/**
 * Execute library script from scheduler
 */
void ExecuteScript(const ScheduleParameters *param)
{
   size_t bufSize = 512;
   TCHAR name[256];
   nx_strncpy(name, param->m_params, 256);
   Trim(name);

   ObjectArray<NXSL_Value> args(16, 16, false);

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
         args.clear();
         return;
      }
   }

   NXSL_VM *vm = g_pScriptLibrary->createVM(name, new NXSL_ServerEnv);
   if (vm != NULL)
   {
      if (vm->run(&args))
      {
			DbgPrintf(4, _T("ExecuteScript(%s): Script executed sucesfully."), param, vm->getErrorText());
      }
      else
      {
			DbgPrintf(4, _T("ExecuteScript(%s): Script execution error: %s"), param, vm->getErrorText());
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
