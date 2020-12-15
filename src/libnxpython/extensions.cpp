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
** File: extensions.cpp
**
**/

#include "libnxpython.h"
#include <netxms-version.h>

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
 * Method netxms.version()
 */
static PyObject *M_version(PyObject *self, PyObject *args)
{
   return PyUnicode_FromString(NETXMS_VERSION_STRING_A);
}

/**
 * Method netxms.buildTag()
 */
static PyObject *M_buildTag(PyObject *self, PyObject *args)
{
   return PyUnicode_FromString(NETXMS_BUILD_TAG_A);
}

/**
 * Methods in module "netxms"
 */
static PyMethodDef s_methods[] =
{
   { "buildTag", M_buildTag, METH_NOARGS, "Return NetXMS build tag" },
   { "trace", M_trace, METH_VARARGS, "Write debug message to NetXMS log file" },
   { "version", M_version, METH_NOARGS, "Return NetXMS version" },
   { NULL, NULL, 0, NULL }
};

/**
 * Module definition
 */
static PyModuleDef s_module =
{
   PyModuleDef_HEAD_INIT,
   "netxms",   // name
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
PyObject *PyInit_netxms()
{
   PyObject *mod = PyModule_Create(&s_module);
   PyModule_AddObject(mod, "__path__", Py_BuildValue("()"));
   return mod;
}
