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
** File: bridge.cpp
**
**/

#include "python_subagent.h"

/**
 * Method netxms.trace()
 */
static PyObject *M_trace(PyObject *self, PyObject *args)
{
   int level;
   Py_UNICODE *text;
   if (!PyArg_ParseTuple(args, "iu", &level, &text))
      return NULL;

#ifdef UNICODE
   nxlog_debug(level, _T("%s"), text);
#else
   char *mbtext = MBStringFromWideString(text);
   nxlog_debug(level, _T("%s"), mbtext);
   free(mbtext);
#endif

   Py_INCREF(Py_None);
   return Py_None;
}

/**
 * Methods in module "netxms.agent"
 */
static PyMethodDef s_methods[] =
{
   { "trace", M_trace, METH_VARARGS, "Write debug message to NetXMS log file" },
   { NULL, NULL, 0, NULL }
};

/**
 * Module definition
 */
static PyModuleDef s_agentModule =
{
   PyModuleDef_HEAD_INIT,
   "netxms.agent",   // name
   NULL,       // documentation
   -1,         // state struct size
   s_methods,  // methods
   NULL,       // slots
   NULL,       // traverse
   NULL,       // clear
   NULL        // free
};

/**
 * Module initialization
 */
PyObject *PyInit_netxms_agent()
{
   PyObject *module = PyModule_Create(&s_agentModule);

   // DCI data types
   PyModule_AddIntConstant(module, "DCI_DT_COUNTER32", DCI_DT_COUNTER32);
   PyModule_AddIntConstant(module, "DCI_DT_COUNTER64", DCI_DT_COUNTER64);
   PyModule_AddIntConstant(module, "DCI_DT_FLOAT", DCI_DT_FLOAT);
   PyModule_AddIntConstant(module, "DCI_DT_INT", DCI_DT_INT);
   PyModule_AddIntConstant(module, "DCI_DT_INT64", DCI_DT_INT64);
   PyModule_AddIntConstant(module, "DCI_DT_STRING", DCI_DT_STRING);
   PyModule_AddIntConstant(module, "DCI_DT_UINT", DCI_DT_UINT);
   PyModule_AddIntConstant(module, "DCI_DT_UINT64", DCI_DT_UINT64);

   return module;
}

/**
 * Sub-agent module definition
 */
static PyModuleDef s_subAgentModule =
{
   PyModuleDef_HEAD_INIT,
   "netxms.subagent",   // name
   NULL,       // documentation
   -1,         // state struct size
   NULL,       // methods
   NULL,       // slots
   NULL,       // traverse
   NULL,       // clear
   NULL        // free
};

/**
 * Module initialization
 */
PyObject *PyInit_netxms_subagent()
{
   return PyModule_Create(&s_subAgentModule);
}
