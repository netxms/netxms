#ifndef _python_subagent_h_
#define _python_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxpython.h>

#define PY_SUBAGENT_DEBUG_TAG _T("python")

/**
 * Plugin information
 */
class PythonPlugin
{
private:
   TCHAR *m_name;
   PythonInterpreter *m_interpreter;
   PyObject *m_subagentModule;

   PythonPlugin(const TCHAR *name, PythonInterpreter *interpreter, PyObject *subagentModule);

   bool getParameters(StructArray<NETXMS_SUBAGENT_PARAM> *parameters);
   bool getLists(StructArray<NETXMS_SUBAGENT_LIST> *lists);

   INT32 parameterHandler(const TCHAR *id, const TCHAR *name, TCHAR *value);
   INT32 listHandler(const TCHAR *id, const TCHAR *name, StringList *value);

public:
   ~PythonPlugin();

   static PythonPlugin *load(const TCHAR *file, StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists);

   static LONG parameterHandlerEntryPoint(const TCHAR *parameter, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
   static LONG listHandlerEntryPoint(const TCHAR *parameter, const TCHAR *arg, StringList *value, AbstractCommSession *session);
};

#endif
