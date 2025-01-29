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
** File: manager.cpp
**
**/

#include "libnxpython.h"
#include <nxqueue.h>

#define DEBUG_TAG _T("python")

/**
 * "netxms" module initialization function
 */
PyObject *PyInit_netxms();

/**
 * Extra modules
 */
static NXPYTHON_MODULE_INFO *s_extraModules = NULL;

/**
 * Manager request queue
 */
static Queue s_managerRequestQueue;

/**
 * Manager request
 */
struct PythonManagerRequest
{
   int command;
   PyThreadState *interpreter;
   Condition condition;

   PythonManagerRequest() : condition(true)
   {
      command = 0;   // create interpreter
      interpreter = NULL;
   }

   PythonManagerRequest(PyThreadState *pi) : condition(true)
   {
      command = 1;   // destroy interpreter
      interpreter = pi;
   }

   void wait()
   {
      condition.wait(INFINITE);
   }

   void notify()
   {
      condition.set();
   }
};

/**
 * Manager thread
 */
static void PythonManager(Condition *initCompleteCondition)
{
   PyImport_AppendInittab("netxms", PyInit_netxms);
   if (s_extraModules != NULL)
   {
      for(int i = 0; s_extraModules[i].name != NULL; i++)
      {
         PyImport_AppendInittab(s_extraModules[i].name, s_extraModules[i].initFunction);
      }
   }
   Py_InitializeEx(0);
   char version[128];
   strlcpy(version, Py_GetVersion(), 128);
   char *p = strchr(version, ' ');
   if (p != NULL)
      *p = 0;

   PyEval_InitThreads();
   PyThreadState *mainPyThread = PyEval_SaveThread();
   initCompleteCondition->set();
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Embedded Python interpreter (version %hs) initialized"), version);

   while(true)
   {
      PythonManagerRequest *request = static_cast<PythonManagerRequest*>(s_managerRequestQueue.getOrBlock());
      if (request == INVALID_POINTER_VALUE)
         break;

      PyEval_AcquireThread(mainPyThread);
      switch(request->command)
      {
         case 0:  // create interpreter
            request->interpreter = Py_NewInterpreter();
            if (request->interpreter != NULL)
            {
               PyRun_SimpleString(
                       "import importlib.abc\n"
                       "import importlib.machinery\n"
                       "import sys\n"
                       "\n"
                       "class Finder(importlib.abc.MetaPathFinder):\n"
                       "    def find_spec(self, fullname, path, target=None):\n"
                       "        if fullname in sys.builtin_module_names:\n"
                       "            return importlib.machinery.ModuleSpec(\n"
                       "                fullname,\n"
                       "                importlib.machinery.BuiltinImporter,\n"
                       "            )\n"
                       "\n"
                       "sys.meta_path.append(Finder())\n"
                   );

               PyThreadState_Swap(mainPyThread);
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Created Python interpreter instance %p"), request->interpreter);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot create Python interpreter instance"));
            }
            break;
         case 1:  // destroy interpreter
            PyThreadState_Swap(request->interpreter);
            Py_EndInterpreter(request->interpreter);
            PyThreadState_Swap(mainPyThread);
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Destroyed Python interpreter instance %p"), request->interpreter);
            break;
      }
      PyEval_ReleaseThread(mainPyThread);
      request->notify();
   }

   PyEval_RestoreThread(mainPyThread);
   Py_Finalize();
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Embedded Python interpreter terminated"));
}

/**
 * Create Python sub-interpreter
 */
PythonInterpreter *PythonInterpreter::create()
{
   PythonManagerRequest r;
   s_managerRequestQueue.put(&r);
   r.wait();
   return (r.interpreter != NULL) ? new PythonInterpreter(r.interpreter) : NULL;
}

/**
 * Internal constructor
 */
PythonInterpreter::PythonInterpreter(PyThreadState *s)
{
   m_threadState = s;
   m_mainModule = NULL;
}

/**
 * Destructor
 */
PythonInterpreter::~PythonInterpreter()
{
   if (m_mainModule != NULL)
   {
      PyEval_AcquireThread(m_threadState);
      Py_DECREF(m_mainModule);
      m_mainModule = NULL;
      PyEval_ReleaseThread(m_threadState);
   }
   PythonManagerRequest r(m_threadState);
   s_managerRequestQueue.put(&r);
   r.wait();
}

/**
 * Log execution error
 */
void PythonInterpreter::logExecutionError(const TCHAR *format)
{
   PyObject *type, *value, *traceback;
   PyErr_Fetch(&type, &value, &traceback);
   PyObject *text = (value != NULL) ? PyObject_Str(value) : NULL;
#ifdef UNICODE
   if (text != nullptr)
   {
      wchar_t buffer[1024];
      Py_ssize_t s = PyUnicode_AsWideChar(text, buffer, 1023);
      buffer[s] = 0;
      nxlog_debug_tag(DEBUG_TAG, 6, format, buffer);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, format, L"error text not provided");
   }
#else
   nxlog_debug_tag(DEBUG_TAG, 6, format, (text != NULL) ? PyUnicode_AsUTF8(text) : "error text not provided");
#endif
   if (text != NULL)
      Py_DECREF(text);
   if (type != NULL)
      Py_DECREF(type);
   if (value != NULL)
      Py_DECREF(value);
   if (traceback != NULL)
      Py_DECREF(traceback);
}

/**
 * Load and execute module from given source
 */
PyObject *PythonInterpreter::loadModule(const char *source, const char *moduleName, const char *fileName)
{
   PyObject *module = NULL;
   PyEval_AcquireThread(m_threadState);

   PyObject *code = Py_CompileString(source, (fileName != NULL) ? fileName : ":memory:", Py_file_input);
   if (code != NULL)
   {
      module = PyImport_ExecCodeModuleEx(moduleName, code, fileName);
      if (module == NULL)
      {
         TCHAR format[256];
         _sntprintf(format, 256, _T("Cannot import module %hs (%%s)"), moduleName);
         logExecutionError(format);
      }
      Py_DECREF(code);
   }
   else
   {
      logExecutionError(_T("Source compilation failed (%s)"));
   }

   PyEval_ReleaseThread(m_threadState);
   return module;
}

/**
 * Load source code from file and execute
 */
PyObject *PythonInterpreter::loadModuleFromFile(const TCHAR *fileName, const char *moduleName)
{
   char *source = LoadFileAsUTF8String(fileName);
   if (source == NULL)
      return NULL;
#ifdef UNICODE
   char *utfFileName = UTF8StringFromWideString(fileName);
   PyObject *module = loadModule(source, moduleName, utfFileName);
   free(utfFileName);
#else
   bool success = loadModule(source, moduleName, fileName);
#endif
   free(source);
   return module;
}

/**
 * Load main module
 */
bool PythonInterpreter::loadMainModule(const char *source, const char *fileName)
{
   bool success = false;
   PyEval_AcquireThread(m_threadState);

   if (m_mainModule != NULL)
   {
      Py_DECREF(m_mainModule);
      m_mainModule = NULL;
   }

   PyObject *code = Py_CompileString(source, (fileName != NULL) ? fileName : ":memory:", Py_file_input);
   if (code != NULL)
   {
      m_mainModule = PyImport_ExecCodeModuleEx("__main__", code, fileName);
      if (m_mainModule != NULL)
      {
         success = true;
      }
      else
      {
         logExecutionError(_T("Cannot create __main__ module (%s)"));
      }
      Py_DECREF(code);
   }
   else
   {
      logExecutionError(_T("Source compilation failed (%s)"));
   }

   PyEval_ReleaseThread(m_threadState);
   return success;
}

/**
 * Load main module from file
 */
bool PythonInterpreter::loadMainModuleFromFile(const TCHAR *fileName)
{
   char *source = LoadFileAsUTF8String(fileName);
   if (source == NULL)
      return false;
#ifdef UNICODE
   char *utfFileName = UTF8StringFromWideString(fileName);
   bool success = loadMainModule(source, utfFileName);
   free(utfFileName);
#else
   bool success = loadMainModule(source, fileName);
#endif
   free(source);
   return success;
}

/**
 * Call function in main module with C arguments
 */
PyObject *PythonInterpreter::call(const char *name, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   PyObject *result = callv(m_mainModule, name, format, args);
   va_end(args);
   return result;
}

/**
 * Call function with C arguments
 */
PyObject *PythonInterpreter::call(PyObject *module, const char *name, const char *format, ...)
{
   if (module == NULL)
      return NULL;

   va_list args;
   va_start(args, format);
   PyObject *result = callv(module, name, format, args);
   va_end(args);
   return result;
}

/**
 * Call function with C arguments
 */
PyObject *PythonInterpreter::callv(PyObject *module, const char *name, const char *format, va_list args)
{
   if (module == NULL)
      return NULL;

   PyObject *result = NULL;
   PyEval_AcquireThread(m_threadState);

   PyObject *func = PyObject_GetAttrString(module, name);
   if ((func != NULL) && PyCallable_Check(func))
   {
      PyObject *pargs = (format != NULL) ? Py_VaBuildValue(format, args) : NULL;
      result = PyObject_CallObject(func, pargs);
      if (result == NULL)
      {
         logExecutionError(_T("Function call failed (%s)"));
      }
      if (pargs != NULL)
         Py_DECREF(pargs);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Function %hs does not exist"), name);
   }

   PyEval_ReleaseThread(m_threadState);
   return result;
}

/**
 * Decrement object reference
 */
void PythonInterpreter::decref(PyObject *object)
{
   PyEval_AcquireThread(m_threadState);
   Py_DECREF(object);
   PyEval_ReleaseThread(m_threadState);
}

/**
 * Manager thread handle
 */
static THREAD s_managerThread = INVALID_THREAD_HANDLE;

/**
 * Initialize embedded Python
 */
void LIBNXPYTHON_EXPORTABLE InitializeEmbeddedPython(NXPYTHON_MODULE_INFO *modules)
{
   s_extraModules = modules;
   Condition initCompleted(true);
   s_managerThread = ThreadCreateEx(PythonManager, &initCompleted);
   initCompleted.wait(INFINITE);
}

/**
 * Shutdown embedded Python
 */
void LIBNXPYTHON_EXPORTABLE ShutdownEmbeddedPython()
{
   s_managerRequestQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_managerThread);
}

/**
 * Create Python interpreter
 */
PythonInterpreter LIBNXPYTHON_EXPORTABLE *CreatePythonInterpreter()
{
   return PythonInterpreter::create();
}
