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
 * Constructor
 */
PythonPlugin::PythonPlugin(PythonInterpreter *interpreter)
{
   m_interpreter = interpreter;
}

/**
 * Destructor
 */
PythonPlugin::~PythonPlugin()
{
   delete m_interpreter;
}

/**
 * Load plugin
 */
PythonPlugin *PythonPlugin::load(const TCHAR *file, StructArray<NETXMS_SUBAGENT_PARAM> *parameters)
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

   // Read supported parameters
   PyObject *result = pi->call(subagentModule, "SubAgent_GetParameters", NULL);
   if (result == NULL)
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Cannot execute netxms.agent.plugin.get_parameters in plugin \"%s\""), file);
      delete pi;
      return NULL;
   }
   if (PyList_Check(result))
   {
      Py_ssize_t size = PyList_Size(result);
      for(Py_ssize_t i = 0; i < size; i++)
      {
         PyObject *element = PyList_GetItem(result, i);
         if ((element != NULL) && PyTuple_Check(element) && (PyTuple_Size(element) == 3))
         {
            Py_UNICODE *name, *description;
            int type;
            if (PyArg_ParseTuple(element, "uiu", &name, &type, &description))
            {

               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 6, _T("Added parameter \"%s\""), name);
            }
            else
            {
               nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Malformed element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, file);
            }
         }
         else
         {
            nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Invalid element %d in data returned by SubAgent_GetParameters in plugin \"%s\""), (int)i, file);
         }
      }
   }
   else
   {
      nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Result of SubAgent_GetParameters in plugin \"%s\" is not a list"), file);
   }
   pi->decref(result);

   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Plugin \"%s\" loaded"), file);
   return new PythonPlugin(pi);
}
