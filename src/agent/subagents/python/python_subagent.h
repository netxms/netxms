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
   PythonInterpreter *m_interpreter;

   PythonPlugin(PythonInterpreter *interpreter);

public:
   ~PythonPlugin();

   static PythonPlugin *load(const TCHAR *file, StructArray<NETXMS_SUBAGENT_PARAM> *parameters);
};

#endif
