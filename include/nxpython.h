/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: nxpython.h
**
**/

#ifndef _nxpython_h_
#define _nxpython_h_

#include <nms_common.h>

#ifdef LIBNXPYTHON_EXPORTS
#define LIBNXPYTHON_EXPORTABLE __EXPORT
#else
#define LIBNXPYTHON_EXPORTABLE __IMPORT
#endif

#if WITH_PYTHON

#include <Python.h>

/**
 * Wrapper for Python sub-interpreter
 */
class LIBNXPYTHON_EXPORTABLE PythonInterpreter
{
private:
   PyThreadState *m_threadState;
   PyObject *m_mainModule;

   PythonInterpreter(PyThreadState *s);

   void logExecutionError(const TCHAR *format);

public:
   ~PythonInterpreter();
   
   bool execute(const char *source);
   PyObject *call(const char *name, PyObject *args = NULL);
   
   static PythonInterpreter *create();
};

/**
 * Additional module info
 */
struct NXPYTHON_MODULE_INFO
{
   const char *name;
   PyObject *(*initFunction)();
};

void LIBNXPYTHON_EXPORTABLE InitializeEmbeddedPython(NXPYTHON_MODULE_INFO *modules = NULL);
void LIBNXPYTHON_EXPORTABLE ShutdownEmbeddedPython();
PythonInterpreter LIBNXPYTHON_EXPORTABLE *CreatePythonInterpreter();

#endif   /* WITH_PYTHON */

#endif
