/*
 ** Python integration subagent
 ** Copyright (C) 2006-2020 Raden Solutions
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
 **/

#include "python_subagent.h"
#include <netxms-version.h>

/**
 * Plugin registry
 */
static ObjectArray<PythonPlugin> s_plugins(16, 16, Ownership::False);

/**
 * Register plugins
 */
static void RegisterPlugins(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_LIST> *lists, Config *config)
{
   ConfigEntry *plugins = config->getEntry(_T("/Python/Plugin"));
   if (plugins != NULL)
   {
      for(int i = 0; i < plugins->getValueCount(); i++)
      {
         PythonPlugin *p = PythonPlugin::load(plugins->getValue(i), parameters, lists);
         if (p != NULL)
         {
            s_plugins.add(p);
         }
         else
         {
            AgentWriteLog(NXLOG_WARNING, _T("Python: cannot load plugin %s"), plugins->getValue(i));
         }
      }
   }
   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 3, _T("%d parameters added from plugins"), parameters->size());
}

/**
 * Agent module initialization
 */
PyObject *PyInit_netxms_agent();

/**
 * Sub-agent module initialization
 */
PyObject *PyInit_netxms_subagent();

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("PYTHON"), NETXMS_VERSION_STRING,
   NULL, NULL, NULL, NULL, NULL,
   0, NULL,    // parameters
   0, NULL,		// lists
   0, NULL,		// tables
   0, NULL,		// actions
   0, NULL		// push parameters
};

/**
 * Python modules
 */
static struct NXPYTHON_MODULE_INFO s_pythonModules[] =
{
   { "netxms.agent", PyInit_netxms_agent },
   { "netxms.subagent", PyInit_netxms_subagent },
   { NULL, NULL }
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PYTHON)
{
   nxlog_debug_tag(PY_SUBAGENT_DEBUG_TAG, 1, _T("Initializing Python subagent"));

   InitializeEmbeddedPython(s_pythonModules);

   StructArray<NETXMS_SUBAGENT_PARAM> *parameters = new StructArray<NETXMS_SUBAGENT_PARAM>();
   StructArray<NETXMS_SUBAGENT_LIST> *lists = new StructArray<NETXMS_SUBAGENT_LIST>();

   RegisterPlugins(parameters, lists, config);

   m_info.numParameters = parameters->size();
   m_info.parameters = MemCopyArray(parameters->getBuffer(), parameters->size());
   m_info.numLists = lists->size();
   m_info.lists = MemCopyArray(lists->getBuffer(), lists->size());

   delete parameters;
   delete lists;

   *ppInfo = &m_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
