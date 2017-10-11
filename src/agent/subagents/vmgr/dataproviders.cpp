/*
 ** File management subagent
 ** Copyright (C) 2016 Raden Solutions
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

#include "vmgr.h"


/**
 * --------------------------------
 * Parameter, list, table providers
 * --------------------------------
 */

/**
 * Get list of configured VMs
 */
LONG H_GetHostList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   StringList *keyList = g_connectionList.keys();
	for(int i = 0; i < keyList->size(); i++)
		value->add(keyList->get(i));
	return SYSINFO_RC_SUCCESS;
}

/**
 * Get parameter form capabilities
 */
LONG H_GetFromCapabilities(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(pszParam, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const char *xml = conn->getCapabilitiesAndLock();
   if(xml == NULL)
   {
      conn->unlockCapabilities();
      return SYSINFO_RC_ERROR;
   }

   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "capabilities", false))
   {
      conn->unlockCapabilities();
      return SYSINFO_RC_ERROR;
   }

   ret_string(pValue, conf.getValue(pArg, _T("")));
   conn->unlockCapabilities();
	return SYSINFO_RC_SUCCESS;
}

/**
 * Get UINT32 parameters
 */
LONG H_GetUIntParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(pszParam, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   UINT32 resultValue = 0;
   LONG result = SYSINFO_RC_SUCCESS;

   switch(*pArg)
   {
      case 'C': //CPU.MaxCount
         resultValue = conn->getMaxVCpuCount();
         if(resultValue == -1)
            result = SYSINFO_RC_UNSUPPORTED;
         break;
      case 'F': //CPU.Frequency
      {
         const virNodeInfo *nodeInfo = conn->getNodeInfoAndLock();
         if(nodeInfo == NULL)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         else
         {
            resultValue = nodeInfo->mhz;
         }
         break;
         conn->unlockNodeInfo();
      }
      default:
         result = SYSINFO_RC_ERROR;
   }

   ret_uint(pValue, resultValue);

	return result;
}

/**
 * Get UINT64 parameters
 */
LONG H_GetUInt64Param(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(pszParam, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   UINT64 resultValue = 0;
   LONG result = SYSINFO_RC_SUCCESS;

   switch(*pArg)
   {
      case 'F': //FreeMemory
         resultValue = conn->getHostFreeMemory();
         if(resultValue == 0)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         break;
      case 'M': //MemorySize
      {
         const virNodeInfo *nodeInfo = conn->getNodeInfoAndLock();
         if(nodeInfo == NULL)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         else
         {
            resultValue = nodeInfo->memory*1024;
         }
         break;
         conn->unlockNodeInfo();
      }
      case 'V': //ConnectionVersion
         resultValue = conn->getConnectionVersion();
         if(resultValue == -1)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         break;
      case 'L': //LibraryVersion
         resultValue = conn->getLibraryVersion();
         if(resultValue == -1)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         break;
      default:
         result = SYSINFO_RC_ERROR;
   }

   ret_uint64(pValue, resultValue);

	return result;
}

/**
 * Get string parameters
 */
LONG H_GetStringParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(pszParam, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   char *resultValue = NULL;
   LONG result = SYSINFO_RC_SUCCESS;

   switch(*pArg)
   {
      case 'M': //Model
      {
         const virNodeInfo *nodeInfo = conn->getNodeInfoAndLock();
         if(nodeInfo == NULL)
         {
            result = SYSINFO_RC_UNSUPPORTED;
         }
         else
         {
            resultValue = (char *)nodeInfo->model;
         }
         break;
         conn->unlockNodeInfo();
      }
      case 'C': //ConnectionType
         resultValue = (char *)conn->getConnectionType();
         if(resultValue == NULL)
            result = SYSINFO_RC_UNSUPPORTED;
         break;
      default:
         result = SYSINFO_RC_ERROR;
   }

   ret_mbstring(pValue, resultValue);

	return result;
}

/**
 * Get VM info specific parameters
 */
LONG H_GetVMInfoAsParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR name[256], vm[256];

	if (!AgentGetParameterArg(pszParam, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(pszParam, 2, vm, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   char *resultValue = NULL;
   LONG result = SYSINFO_RC_SUCCESS;
   const virDomainInfo *info = conn->getDomainInfoAndLock(vm);
   if(info != NULL)
   {
      switch(*pArg)
      {
         case 'U': //Memory.Used
            ret_uint64(pValue, (UINT64)info->memory * 1024);
            break;
         case 'P': //Memory.UsedPrec
            ret_uint(pValue, (UINT32)(info->memory/(float)info->maxMem)*100);
            break;
         case 'M': //Memory.Max
            ret_uint64(pValue, (UINT64)info->maxMem * 1024);
            break;
         case 'C': //CPU.Time
            ret_uint64(pValue, info->nrVirtCpu);
            break;
         default:
            result = SYSINFO_RC_ERROR;
      }
   }
   else
   {
      result = SYSINFO_RC_ERROR;
   }
   conn->unlockDomainInfo();

   return result;
}

/**
 * Get domain(VM) list
 */
LONG H_GetVMList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   StringList *keyList = conn->getDomainListAndLock()->keys();
	for(int i = 0; i < keyList->size(); i++)
		value->add(keyList->get(i));
   conn->unlockDomainList();
	return SYSINFO_RC_SUCCESS;
}

static const TCHAR *vmStateMapping[] =
{
   _T("no state"),
   _T("Running"),
   _T("blocked on resource"),
   _T("paused by user"),
   _T("shut down"),
   _T("shut off"),
   _T("crashed"),
   _T("suspended by guest power management")
};

static const TCHAR *vmOpStateMapping[] =
{
   _T("Ready to accept commands"),
   _T("Background job is running"),
   _T("Occupied by a running command"),
   _T("Unusable")
};

static const TCHAR *vmOpStateDescMapping[] =
{
   _T("No reason"),
   _T("Unknown"),
   _T("Monitor connection is broken"),
   _T("Error caused due to internal failure in libvirt")
};

struct VMDataStr
{
   Table *value;
   HostConnections *conn;
};

/**
 * Callback that fills table with VM information
 */
EnumerationCallbackResult FillVMData(const TCHAR *key, const void *obj, void *userData)
{
   NXvirDomain *vm = (NXvirDomain *)obj;
   Table *value = ((VMDataStr *)userData)->value;
   HostConnections *conn = ((VMDataStr *)userData)->conn;

   const char *xml = conn->getDomainDefenitionAndLock(key, vm);
   if(xml == NULL)
   {
      conn->unlockDomainDefenition();
      return _CONTINUE;
   }
   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "domain", false))
      AgentWriteLog(2, _T("VMGR.FillVMData(): Not possible to parse VM XML definition"));
   conn->unlockDomainDefenition();

   value->addRow();
   value->set(0, virDomainGetID(*vm));
   value->set(1, key);
   value->set(2, conf.getValue(_T("/uuid"), _T("")));
   String os(conf.getValue(_T("/os/type"), _T("Unkonown")));
   ConfigEntry *entry = conf.getEntry(_T("/os/type"));
   if(entry != NULL)
   {
      os.append(_T("/"));
      os.append(entry->getAttribute(_T("arch")));
   }
   value->set(4, os.getBuffer());

   const virDomainInfo *info = conn->getDomainInfoAndLock(key, vm);
   if(info != NULL)
   {
      value->set(3, info->state < 8 ? vmStateMapping[info->state] :  _T("Unkonown"));
      value->set(6, (UINT64)info->memory * 1024);
      value->set(7, (UINT64)info->maxMem * 1024);
      value->set(8, (UINT32)(info->memory/(float)info->maxMem)*100);
      value->set(9, info->nrVirtCpu);
      value->set(10, info->cpuTime);
   }
   else
   {
      value->set(3, _T("Unkonown"));
      value->set(6, 0);
      value->set(7, 0);
      value->set(8, 0);
      value->set(9, 0);
      value->set(10, 0);
   }
   conn->unlockDomainInfo();

   int autostart;
   if(virDomainGetAutostart(*vm, &autostart) == 0)
   {
      value->set(5, autostart == 0 ? _T("No") : _T("Yes"));
   }
   else
      value->set(5, _T("Unkonown"));

   virDomainControlInfo cInfo;
   if(virDomainGetControlInfo(*vm, &cInfo,0) == 0)
   {
      value->set(11, cInfo.state > 4 ? _T("Unknown") : vmOpStateMapping[cInfo.state]);
      value->set(12, cInfo.details > 4 ? _T("Unknown") : vmOpStateDescMapping[cInfo.details]);
      value->set(13, cInfo.stateTime);
   }
   else
   {
      value->set(11, _T("Unkonown"));
      value->set(12, _T(""));
      value->set(13, 0);
   }

#if HAVE_VIRDOMAINGETTIME
   INT64 seconds;
   UINT32 nsec;
   if(virDomainGetTime(*vm, &seconds, &nsec, 0) == 0)
      value->set(14, seconds);
   else
#endif
      value->set(14, 0);

   int pers = virDomainIsPersistent(*vm);
   value->set(15, pers == -1 ? _T("Unknown") : pers == 1 ? _T("Yes") : _T("No"));

   return _CONTINUE;
}

/**
 * Get domain(VM) table
 */
LONG H_GetVMTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const StringObjectMap<NXvirDomain> *domains = conn->getDomainListAndLock();

   value->addColumn(_T("ID"), DCI_DT_UINT, _T("ID"));
	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
	value->addColumn(_T("UUID"), DCI_DT_STRING, _T("UUID"));
	value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
	value->addColumn(_T("OS"), DCI_DT_STRING, _T("OS"));
	value->addColumn(_T("AUTOSTART"), DCI_DT_STRING, _T("Autostart scheduled"));
	value->addColumn(_T("MEMORY"), DCI_DT_UINT64, _T("Memory currently used by VM"));
	value->addColumn(_T("MAX_MEMORY"), DCI_DT_UINT64, _T("Maximum VM available memory"));
	value->addColumn(_T("MEMORY_PREC"), DCI_DT_UINT64, _T("Percentage of memory currently used"));
	value->addColumn(_T("VIRTUAL_CPU_COUNT"), DCI_DT_UINT, _T("CPU count available for VM"));
	value->addColumn(_T("VIRTUAL_CPU_TIME"), DCI_DT_UINT64, _T("CPU time used for VM in nanoseconds"));
	value->addColumn(_T("CONTROL_STATE"), DCI_DT_STRING, _T("VM control state"));
	value->addColumn(_T("STATE_DETAILS"), DCI_DT_STRING, _T("VM control state details"));
	value->addColumn(_T("CONTROL_STATE_DURATION"), DCI_DT_UINT, _T("Time since VM entered the control state"));
	value->addColumn(_T("TIME"), DCI_DT_UINT, _T("Current VM time")); // +DISPLAY NAME AND TYPE
	value->addColumn(_T("IS_PERSISTENT"), DCI_DT_STRING, _T("VM is persistent"));

   VMDataStr data;
   data.value = value;
   data.conn = conn;

   domains->forEach(FillVMData, &data);
   conn->unlockDomainList();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get interface table
 */
LONG H_GetIfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetParameterArg(cmd, 1, name, 256))
   return SYSINFO_RC_UNSUPPORTED;

   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const StringList *ifaceList = conn->getIfaceListAndLock();

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   for(int i=0; i < ifaceList->size(); i++)
   {
      value->addRow();
      value->set(0, ifaceList->get(i));
   }

   conn->unlockIfaceList();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get table of disks for VM
 */
LONG H_GetVMDiskTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   TCHAR vmName[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(cmd, 2, vmName, 256))
		return SYSINFO_RC_UNSUPPORTED;

   _tcsupr(vmName);
   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const char *xml = conn->getDomainDefenitionAndLock(vmName);
   if(xml == NULL)
   {
      conn->unlockDomainDefenition();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "domain", false))
		return SYSINFO_RC_UNSUPPORTED;
   conn->unlockDomainDefenition();

   value->addColumn(_T("SOURCE"), DCI_DT_STRING, _T("Source file"));
	value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
	value->addColumn(_T("DTYPE"), DCI_DT_STRING, _T("VM device type"));
	value->addColumn(_T("DNAME"), DCI_DT_STRING, _T("VM device name"), true);
	value->addColumn(_T("CTYPE"), DCI_DT_STRING, _T("VM controller type"));
	value->addColumn(_T("ADDRESS"), DCI_DT_STRING, _T("Address"));

   ObjectArray<ConfigEntry> *deviceList = conf.getSubEntries(_T("/devices"), _T("disk"));
   for(int i = 0; i < deviceList->size(); i++)
   {
      value->addRow();
      ConfigEntry *item = deviceList->get(i);
      value->set(1, item->getAttribute(_T("type")));
      value->set(2, item->getAttribute(_T("device")));
      ConfigEntry *tmp = item->findEntry(_T("source"));
      if(tmp != NULL)
         value->set(0, tmp->getAttribute(_T("file")));
      tmp = item->findEntry(_T("target"));
      if(tmp != NULL)
      {
         value->set(3, tmp->getAttribute(_T("dev")));
         value->set(4, tmp->getAttribute(_T("bus")));
      }
      //get data form address
      tmp = item->findEntry(_T("address"));
      if(tmp != NULL)
      {
         TCHAR address[16];
         _sntprintf(address, 16, _T("%d/%d/%d/%d"), tmp->getAttributeAsInt(_T("controller")),
                     tmp->getAttributeAsInt(_T("bus")), tmp->getAttributeAsInt(_T("target")), tmp->getAttributeAsInt(_T("unit")));
         value->set(5, address);
      }
   }
   delete deviceList;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Get table of controller for VM
 */
LONG H_GetVMControllerTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   TCHAR vmName[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(cmd, 2, vmName, 256))
		return SYSINFO_RC_UNSUPPORTED;

   _tcsupr(vmName);
   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const char *xml = conn->getDomainDefenitionAndLock(vmName);
   if(xml == NULL)
   {
      conn->unlockDomainDefenition();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "domain", false))
		return SYSINFO_RC_UNSUPPORTED;
   conn->unlockDomainDefenition();

	value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"), true);
	value->addColumn(_T("INDEX"), DCI_DT_INT, _T("Index"));
	value->addColumn(_T("MODEL"), DCI_DT_STRING, _T("Model"));

   ObjectArray<ConfigEntry> *deviceList = conf.getSubEntries(_T("/devices"), _T("controller"));
   for(int i = 0; i < deviceList->size(); i++)
   {
      value->addRow();
      ConfigEntry *item = deviceList->get(i);
      value->set(0, item->getAttribute(_T("type")));
      value->set(1, item->getAttributeAsInt(_T("index"), 0));
      value->set(2, item->getAttribute(_T("model")));
   }
   delete deviceList;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Get table of interface for VM
 */
LONG H_GetVMInterfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   TCHAR vmName[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(cmd, 2, vmName, 256))
		return SYSINFO_RC_UNSUPPORTED;

   _tcsupr(vmName);
   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const char *xml = conn->getDomainDefenitionAndLock(vmName);
   if(xml == NULL)
   {
      conn->unlockDomainDefenition();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "domain", false))
		return SYSINFO_RC_UNSUPPORTED;
   conn->unlockDomainDefenition();

   value->addColumn(_T("MAC"), DCI_DT_STRING, _T("MAC address"), true);
	value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
	value->addColumn(_T("SOURCE"), DCI_DT_STRING, _T("Source"));
	value->addColumn(_T("MODEL"), DCI_DT_STRING, _T("Model"));

   ObjectArray<ConfigEntry> *deviceList = conf.getSubEntries(_T("/devices"), _T("interface"));
   for(int i = 0; i < deviceList->size(); i++)
   {
      value->addRow();
      ConfigEntry *item = deviceList->get(i);
      value->set(1, item->getAttribute(_T("type")));
      ConfigEntry *tmp = item->findEntry(_T("mac"));
      if(tmp != NULL)
         value->set(0, tmp->getAttribute(_T("address")));
      tmp = item->findEntry(_T("source"));
      if(tmp != NULL)
         value->set(2, tmp->getAttribute(_T("bridge")));
      tmp = item->findEntry(_T("model"));
      if(tmp != NULL)
         value->set(3, tmp->getAttribute(_T("type")));
   }
   delete deviceList;

   return SYSINFO_RC_SUCCESS;
}

/**
 * Get table of video adapter settings for VM
 */
LONG H_GetVMVideoTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   TCHAR vmName[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	if (!AgentGetParameterArg(cmd, 2, vmName, 256))
		return SYSINFO_RC_UNSUPPORTED;

   _tcsupr(vmName);
   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const char *xml = conn->getDomainDefenitionAndLock(vmName);
   if(xml == NULL)
   {
      conn->unlockDomainDefenition();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "domain", false))
		return SYSINFO_RC_UNSUPPORTED;
   conn->unlockDomainDefenition();

	value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"), true);
	value->addColumn(_T("VRAM"), DCI_DT_UINT64, _T("Video RAM"));

   ObjectArray<ConfigEntry> *deviceList = conf.getSubEntries(_T("/devices"), _T("video"));
   for(int i = 0; i < deviceList->size(); i++)
   {
      value->addRow();
      ConfigEntry *item = deviceList->get(i);
      ConfigEntry *tmp = item->findEntry(_T("model"));
      if(tmp != NULL)
      {
         value->set(0, tmp->getAttribute(_T("type")));
         value->set(1, tmp->getAttributeAsUInt64(_T("vram")) * 1024);
      }
   }
   delete deviceList;

   return SYSINFO_RC_SUCCESS;
}


/**
 * Callback that fills table with Network information
 */
EnumerationCallbackResult FillNetworkData(const TCHAR *key, const void *obj, void *userData)
{
   NXvirNetwork *network = (NXvirNetwork *)obj;
   Table *value = ((VMDataStr *)userData)->value;
   HostConnections *conn = ((VMDataStr *)userData)->conn;

   const char *xml = conn->getNetworkDefenitionAndLock(key, network);
   if(xml == NULL)
   {
      conn->unlockNetworkDefenition();
      return _CONTINUE;
   }
   Config conf;
   if(!conf.loadXmlConfigFromMemory(xml, strlen(xml), NULL, "network", false))
      AgentWriteLog(2, _T("VMGR.FillVMData(): Not possible to parse VM XML definition"));
   conn->unlockNetworkDefenition();

   value->addRow();
   value->set(0, conf.getValue(_T("/name"), _T("Unkonown")));
   value->set(1, conf.getValue(_T("/uuid"), _T("")));

   //concat all interfaces
   String ifaces;
   ObjectArray<ConfigEntry> *ifaceList = conf.getSubEntries(_T("/forward"), _T("interface"));
   for(int i = 0; i < ifaceList->size(); i++)
   {
      ConfigEntry *item = ifaceList->get(i);
      ifaces.append(item->getAttribute(_T("dev")));
      if(i+1 != ifaceList->size())
         ifaces.append(_T(", "));
   }
   value->set(2, ifaces.getBuffer());
   delete ifaceList;

   //concat all portgroups
   String portgroup;
   ObjectArray<ConfigEntry> *portgroupList = conf.getSubEntries(_T("/"), _T("portgroup"));
   for(int i = 0; i < portgroupList->size(); i++)
   {
      ConfigEntry *item = portgroupList->get(i);
      portgroup.append(item->getAttribute(_T("name")));
      if(i+1 != portgroupList->size())
         portgroup.append(_T(", "));
   }
   value->set(3, portgroup.getBuffer());
   delete portgroupList;

   return _CONTINUE;
}

/**
 * Get table of networks
 */
LONG H_GetNetworksTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;


   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const StringObjectMap<NXvirNetwork> *networks = conn->getNetworkListAndLock();
   if(networks == NULL)
   {
      conn->unlockNetworkList();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }

	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
	value->addColumn(_T("UUID"), DCI_DT_UINT64, _T("UUID"));
	value->addColumn(_T("IFACE"), DCI_DT_UINT64, _T("Interfaces"));
	value->addColumn(_T("PORTGROUP"), DCI_DT_UINT64, _T("Port group"));

   VMDataStr data;
   data.value = value;
   data.conn = conn;

   networks->forEach(FillNetworkData, &data);
   conn->unlockNetworkList();
   return SYSINFO_RC_SUCCESS;
}

static const TCHAR *storagePoolState[] =
{
   _T("Not running"),
   _T("Initializing pool, not available"),
   _T("Running normally"),
   _T("Running degraded"),
   _T("Running, but not accessible")
   _T("Unknown")
};

/**
 * Callback that fills table with VM information
 */
EnumerationCallbackResult FillStorageData(const TCHAR *key, const void *obj, void *userData)
{
   NXvirStoragePool *storage = (NXvirStoragePool *)obj;
   Table *value = ((VMDataStr *)userData)->value;
   HostConnections *conn = ((VMDataStr *)userData)->conn;
   const virStoragePoolInfo *info = conn->getStorageInformationAndLock(key, storage);

   value->addRow();
   value->set(0, key);
   value->set(1, storagePoolState[info->state < 5 && info->state >= 0 ? info->state : 5]);
   value->set(2, info->capacity);
   value->set(3, info->allocation);
   value->set(4, info->available);

   conn->unlockStorageInfo();
   return _CONTINUE;
}


/**
 * Get table of storage
 */
LONG H_GetStoragesTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;


   HostConnections *conn = g_connectionList.get(name);
   if(conn == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   const StringObjectMap<NXvirStoragePool> *storages = conn->getStorageListAndLock();
   if(storages == NULL)
   {
      conn->unlockStorageList();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }


	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
	value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
	value->addColumn(_T("CAPACITY"), DCI_DT_UINT64, _T("Capacity"));
	value->addColumn(_T("ALLOCATION"), DCI_DT_UINT64, _T("Allocation"));
	value->addColumn(_T("AVAILABLE"), DCI_DT_UINT64, _T("Available"));

   VMDataStr data;
   data.value = value;
   data.conn = conn;

   storages->forEach(FillStorageData, &data);
   conn->unlockStorageList();
   return SYSINFO_RC_SUCCESS;
}
