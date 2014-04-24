/*
 ** Device Emulation subagent
 ** Copyright (C) 2014 Raden Solutions
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

#include <nms_common.h>
#include <nms_agent.h>
#include <nxstat.h>

#ifdef _WIN32
#define DEVEMU_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define DEVEMU_EXPORTABLE
#endif

/**
 * Configuration data
 */
static TCHAR s_ipAddress[32] = _T("10.0.0.1");
static TCHAR s_ipNetMask[32] = _T("255.0.0.0");
static TCHAR s_ifName[64] = _T("eth0");
static TCHAR s_macAddress[16] = _T("000000000000");
static TCHAR s_paramConfigFile[MAX_PATH] = _T("");


/**
 * Variables
 */
static NX_STAT_STRUCT fileStats;
static time_t fileLastModifyTime = 0;
static StringMap *s_values = new StringMap();
static MUTEX s_valuesMutex = MutexCreate();
static bool s_shutdown = false;

/**
 * Interface list
 */
static LONG H_InterfaceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value)
{
   TCHAR buffer[256];
   _sntprintf(buffer, 256, _T("1 %s/%d 6 %s %s"), s_ipAddress,
              BitsInMask(ntohl(_t_inet_addr(s_ipNetMask))), s_macAddress, s_ifName);
   value->add(buffer);
   value->add(_T("255 127.0.0.1/8 24 000000000000 lo0"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for constant values
 */
static LONG H_Constant(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   ret_string(pValue, pArg);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for configured values
 */
static LONG H_Value(const TCHAR *pszParam, const TCHAR *arg, TCHAR *pValue)
{
   MutexLock(s_valuesMutex);
   const TCHAR *value = s_values->get(arg);
   if (value == NULL)
   {
      ret_string(pValue, _T(""));
   }
   else
   {
      ret_string(pValue, value);
   }
   MutexUnlock(s_valuesMutex);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   s_shutdown = true;
}

/**
 * Provided parameters (default set)
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Net.Interface.AdminStatus(*)"), H_Constant, _T("1"), DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.Link(*)"), H_Constant, _T("1"), DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.MTU(*)"), H_Constant, _T("1500"), DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OperStatus(*)"), H_Constant, _T("1"), DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.IP.Forwarding"), H_Constant, _T("0"), DCI_DT_INT, DCIDESC_NET_IP_FORWARDING }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Net.InterfaceList"), H_InterfaceList, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("DEVEMU"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, NULL,
   0, NULL, // parameters
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   0, NULL, // tables
   0, NULL, // actions
   0, NULL  // push parameters
};

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("InterfaceName"), CT_STRING, 0, 0, 64, 0, s_ifName },
   { _T("IpAddress"), CT_STRING, 0, 0, 32, 0, s_ipAddress },
   { _T("IpNetMask"), CT_STRING, 0, 0, 32, 0, s_ipNetMask },
   { _T("MacAddress"), CT_STRING, 0, 0, 16, 0, s_macAddress },
   { _T("ParametersConfig"), CT_STRING, 0, 0, MAX_PATH, 0, s_paramConfigFile },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

static void LoadConfiguration(bool initial)
{
   StructArray<NETXMS_SUBAGENT_PARAM> *parameters = NULL;
   if (initial)
   {
      parameters = new StructArray<NETXMS_SUBAGENT_PARAM>(m_info.parameters, m_info.numParameters);
   }

   // do load
   FILE *file = _tfopen(s_paramConfigFile, _T("r"));
   if (file == NULL)
   {
      AgentWriteDebugLog(3, _T("Cannot open DEVEMU configuration file (%s)"), s_paramConfigFile);
      return;
   }

   MutexLock(s_valuesMutex);
   s_values->clear();

   TCHAR line[10240];
   while (_fgetts(line, 10240, file) != NULL)
   {
      TCHAR *ptr = line;
      while (*ptr != 0)
      {
         if (*ptr == _T('\n') || *ptr == _T('\r'))
         {
            *ptr = 0;
            break;
         }
         ptr++;
      }
      if (line[0] == 0 || line[0] == _T('#'))
      {
         continue;
      }

      // Format: name:type:description=value
      //    name - string
      //    type - string, possible values: INT, UINT, INT64, UINT64, STRING, FLOAT
      //    description - string, can't contain "="
      //    value - string

      TCHAR *value = _tcschr(line, _T('='));
      if (value == NULL) continue;
      *value = 0;
      value++;

      TCHAR *name = line;

      TCHAR *description = NULL;
      TCHAR *typeStr = _tcschr(name, _T(':'));
      if (typeStr != NULL)
      {
         *typeStr = 0;
         typeStr++;

         description = _tcschr(typeStr, _T(':'));
         if (description != NULL)
         {
            *description = 0;
            description++;
         }
      }

      s_values->set(name, value);

      if (initial)
      {
         NETXMS_SUBAGENT_PARAM *param = new NETXMS_SUBAGENT_PARAM();
         _tcscpy(param->name, name);
         param->handler = H_Value;
         param->arg = _tcsdup(name);
         param->dataType = NxDCIDataTypeFromText(typeStr == NULL ? _T("STRING") : typeStr);
         _tcscpy(param->description, description == NULL ? _T("") : description);

         parameters->add(param);

         delete param;
      }
   }

   MutexUnlock(s_valuesMutex);

   if (initial)
   {
      m_info.numParameters = parameters->size();
      m_info.parameters = (NETXMS_SUBAGENT_PARAM *)nx_memdup(parameters->getBuffer(),
                          parameters->size() * sizeof(NETXMS_SUBAGENT_PARAM));
      delete parameters;
   }
}

/**
 * Check file for changes and if required - re-load configuration
 */
THREAD_RESULT THREAD_CALL MonitorChanges(void *args)
{
   int threadSleepTime = 1;

   while (!s_shutdown)
   {
      int ret = CALL_STAT(s_paramConfigFile, &fileStats);
      if (ret != 0)
      {
         AgentWriteDebugLog(3, _T("Cannot stat DEVEMU configuration file (%s)"), s_paramConfigFile);
      }
      else
      {
         if (fileLastModifyTime != fileStats.st_mtime)
         {
            AgentWriteDebugLog(6, _T("DEVEMU configuration file changed (was: %ld, now: %ld)"), fileLastModifyTime,
                               fileStats.st_mtime);
            fileLastModifyTime = fileStats.st_mtime;
            LoadConfiguration(false);
         }
      }
      ThreadSleep(threadSleepTime);
   }

   return THREAD_OK;
}

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DEVEMU)
{
   if (m_info.parameters != NULL)
      return FALSE;  // Most likely another instance of DEVEMU subagent already loaded

   if (!config->parseTemplate(_T("DEVEMU"), m_cfgTemplate))
      return FALSE;

   LoadConfiguration(true);

   ThreadCreateEx(MonitorChanges, 0, NULL);

   *ppInfo = &m_info;
   return TRUE;
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
