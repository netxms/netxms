/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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
** File: plugin.cpp
**
**/

#include "python_subagent.h"

/**
 * Handler data
 */
struct HandlerData
{
   PythonPlugin *plugin;
   TCHAR *id;

   HandlerData(PythonPlugin *_plugin, const TCHAR *_id) { plugin = _plugin; id = _tcsdup(_id); }
};

/**
 * Constructor
 */
PythonPlugin::PythonPlugin(const TCHAR *name, PythonInterpreter *interpreter, PyObject *subagentModule)
{
   m_name = MemCopyString(name);
   m_interpreter = interpreter;
   m_subagentModule = subagentModule;
}

/**
 * Destructor
 */
PythonPlugin::~PythonPlugin()
{
   m_interpreter->decref(m_subagentModule);
   delete m_interpreter;
   MemFree(m_name);
}

/**
 * Get supported parameters
 */
bool PythonPlugin::getParameters(StructArray<NETXMS_SUBAGENT_PARAM> *parameters)
{
   PyObject *result = m_interpreter->call(m_subagentModule, "SubAgent_GetParameters", NULL);
   if (result == NULL)
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot execute SubAgent_GetParameters in plugin \"%s\""), m_name);
      return false;
   }
   if (PyList_Check(result))
   {
      Py_ssize_t size = PyList_Size(result);
      for(Py_ssize_t i = 0; i < size; i++)
      {
         PyObject *element = PyList_GetItem(result, i);
         if ((element != NULL) && PyTuple_Check(element) && (PyTuple_Size(element) == 4))
         {
            Py_UNICODE *id, *name, *description;
            int type;
            if (PyArg_ParseTuple(element, "uuiu", &id, &name, &type, &description))
            {
               NETXMS_SUBAGENT_PARAM p;
               _tcslcpy(p.name, name, MAX_PARAM_NAME);
               p.dataType = type;
               _tcslcpy(p.description, description, MAX_DB_STRING);
               p.handler = &PythonPlugin::parameterHandlerEntryPoint;
               p.arg = reinterpret_cast<TCHAR*>(new HandlerData(this, id));
               parameters->add(&p);
               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 6, _T("Parameter added (id=%s name=\"%s\" type=%d)"), id, name, type);
            }
            else
            {
               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Malformed element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, m_name);
            }
         }
         else
         {
            nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Invalid element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, m_name);
         }
      }
   }
   else
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Result of SubAgent_GetParameters in plugin \"%s\" is not a list"), m_name);
   }
   m_interpreter->decref(result);
   return true;
}

/**
 * Get supported lists
 */
bool PythonPlugin::getLists(StructArray<NETXMS_SUBAGENT_LIST> *lists)
{
   PyObject *result = m_interpreter->call(m_subagentModule, "SubAgent_GetLists", NULL);
   if (result == NULL)
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot execute SubAgent_GetLists in plugin \"%s\""), m_name);
      return false;
   }
   if (PyList_Check(result))
   {
      Py_ssize_t size = PyList_Size(result);
      for(Py_ssize_t i = 0; i < size; i++)
      {
         PyObject *element = PyList_GetItem(result, i);
         if ((element != NULL) && PyTuple_Check(element) && (PyTuple_Size(element) == 3))
         {
            Py_UNICODE *id, *name, *description;
            if (PyArg_ParseTuple(element, "uuu", &id, &name, &description))
            {
               NETXMS_SUBAGENT_LIST l;
               _tcslcpy(l.name, name, MAX_PARAM_NAME);
               _tcslcpy(l.description, description, MAX_DB_STRING);
               l.handler = &PythonPlugin::listHandlerEntryPoint;
               l.arg = reinterpret_cast<TCHAR*>(new HandlerData(this, id));
               lists->add(&l);
               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 6, _T("List added (id=%s name=\"%s\")"), id, name);
            }
            else
            {
               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Malformed element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, m_name);
            }
         }
         else
         {
            nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Invalid element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, m_name);
         }
      }
   }
   else
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Result of SubAgent_GetParameters in plugin \"%s\" is not a list"), m_name);
   }
   m_interpreter->decref(result);
   return true;
}

/**
 * Load plugin
 */
PythonPlugin *PythonPlugin::load(const TCHAR *file, StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists)
{
   PythonInterpreter *pi = PythonInterpreter::create();
   if (pi == NULL)
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot create interpreter for plugin \"%s\""), file);
      return NULL;
   }

   TCHAR subagentPyCode[MAX_PATH];
   GetNetXMSDirectory(nxDirLib, subagentPyCode);
   _tcslcat(subagentPyCode, FS_PATH_SEPARATOR _T("subagent.py"), MAX_PATH);
   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 6, _T("Loading subagent Python code from \"%s\""), subagentPyCode);
   PyObject *subagentModule = pi->loadModuleFromFile(subagentPyCode, "netxms.subagent");
   if (subagentModule == NULL)
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot load subagent Python code from \"%s\""), subagentPyCode);
      delete pi;
      return NULL;
   }

   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 6, _T("Loading plugin from \"%s\""), file);
   if (!pi->loadMainModuleFromFile(file))
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot load plugin from \"%s\""), file);
      delete pi;
      return NULL;
   }

   PythonPlugin *plugin = new PythonPlugin(file, pi, subagentModule);
   plugin->getParameters(parameters);
   plugin->getLists(lists);

   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Plugin \"%s\" loaded"), file);
   return plugin;
}

/**
 * Parameter handler entry point
 */
LONG PythonPlugin::parameterHandlerEntryPoint(const TCHAR *parameter, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   const HandlerData *hd = reinterpret_cast<const HandlerData*>(arg);
   return hd->plugin->parameterHandler(hd->id, parameter, value);
}

/**
 * Parameter handler
 */
INT32 PythonPlugin::parameterHandler(const TCHAR *id, const TCHAR *name, TCHAR *value)
{
   PyObject *result = m_interpreter->call(m_subagentModule, "SubAgent_CallParameterHandler", "uu", id, name);
   if (result == NULL)
      return SYSINFO_RC_ERROR;

   INT32 rc;
   if (PyUnicode_Check(result))
   {
      ret_utf8string(value, PyUnicode_AsUTF8(result));
      rc = SYSINFO_RC_SUCCESS;
   }
   else if (PyLong_Check(result))
   {
      ret_int64(value, PyLong_AsLong(result));
      rc = SYSINFO_RC_SUCCESS;
   }
   else if (PyFloat_Check(result))
   {
      ret_double(value, PyFloat_AsDouble(result));
      rc = SYSINFO_RC_SUCCESS;
   }
   else if (result == Py_None)
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}

/**
 * List handler entry point
 */
LONG PythonPlugin::listHandlerEntryPoint(const TCHAR *parameter, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   const HandlerData *hd = reinterpret_cast<const HandlerData*>(arg);
   return hd->plugin->listHandler(hd->id, parameter, value);
}

/**
 * Add element to StringList
 */
static void AddListElement(StringList *list, PyObject *element)
{
   if (PyUnicode_Check(element))
   {
      list->addUTF8String(PyUnicode_AsUTF8(element));
   }
   else if (PyLong_Check(element))
   {
      list->add(static_cast<INT64>(PyLong_AsLong(element)));
   }
   else if (PyFloat_Check(element))
   {
      list->add(PyFloat_AsDouble(element));
   }
}

/**
 * List handler
 */
INT32 PythonPlugin::listHandler(const TCHAR *id, const TCHAR *name, StringList *value)
{
   PyObject *result = m_interpreter->call(m_subagentModule, "SubAgent_CallListHandler", "uu", id, name);
   if (result == NULL)
      return SYSINFO_RC_ERROR;

   INT32 rc;
   if (PyList_Check(result))
   {
      Py_ssize_t size = PyList_Size(result);
      for(Py_ssize_t i = 0; i < size; i++)
      {
         AddListElement(value, PyList_GetItem(result, i));
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else if (PyTuple_Check(result))
   {
      Py_ssize_t size = PyTuple_Size(result);
      for(Py_ssize_t i = 0; i < size; i++)
      {
         AddListElement(value, PyTuple_GetItem(result, i));
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else if (result == Py_None)
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}
