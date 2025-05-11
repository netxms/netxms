/*
** NetXMS Tuxedo subagent
** Copyright (C) 2014-2020 Raden Solutions
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
** File: main.cpp
**
**/

#include "tuxedo_subagent.h"
#include <netxms-version.h>

#ifndef _WIN32
#include <sys/utsname.h>
#endif

/**
 * Handlers
 */
LONG H_ClientInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ClientsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ClientsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_DomainInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LocalMachineId(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MachineInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_MachinesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_MachinesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_QueueInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_QueuesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_QueuesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ServerInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServerInstanceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServerInstancesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ServerInstancesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ServersList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ServersTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ServiceGroupInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServiceGroupsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ServiceGroupsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_ServiceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_ServicesList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_ServicesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);

/**
 * Pollers
 */
void TuxedoQueryClients();
void TuxedoQueryDomain();
void TuxedoQueryMachines();
void TuxedoQueryQueues();
void TuxedoQueryServers();
void TuxedoQueryServiceGroups();
void TuxedoQueryServices();

/**
 * Cleaners
 */
void TuxedoResetClients();
void TuxedoResetDomain();
void TuxedoResetMachines();
void TuxedoResetQueues();
void TuxedoResetServers();
void TuxedoResetServiceGroups();
void TuxedoResetServices();

/**
 * Local data query flags
 */
UINT32 g_tuxedoQueryLocalData = LOCAL_DATA_MACHINES | LOCAL_DATA_QUEUES | LOCAL_DATA_SERVERS | LOCAL_DATA_SERVICE_GROUPS;

/**
 * Local host LMID filter settings
 */
bool g_tuxedoLocalMachineFilter = true;

/**
 * Handler for Tuxedo.IsMasterMachine parameter
 */
static LONG H_IsMasterMachine(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR master[MAX_RESULT_LENGTH];
   LONG rc = H_DomainInfo(_T("Tuxedo.Domain.Master"), _T("M"), master, session);
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   TCHAR *p = _tcschr(master, _T(','));
   if (p != NULL)
      *p = 0;

   char pmid[64];
   if (!TuxedoGetMachinePhysicalID(master, pmid))
      return SYSINFO_RC_ERROR;

#ifdef _WIN32
   char nodename[MAX_COMPUTERNAME_LENGTH + 1];
   DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
   if (!GetComputerNameA(nodename, &size))
      return SYSINFO_RC_ERROR;
   ret_int(value, !stricmp(pmid, nodename) ? 1 : 0);
#else
   struct utsname un;
   if (uname(&un) != 0)
      return SYSINFO_RC_ERROR;
   ret_int(value, !strcmp(pmid, un.nodename) ? 1 : 0);
#endif
   return SYSINFO_RC_SUCCESS;
}

/**
 * Poller thread
 */
static THREAD_RESULT THREAD_CALL TuxedoPollerThread(void *arg)
{
   ThreadSetName("TuxedoPoller");
   UINT32 sleepTime = CAST_FROM_POINTER(arg, UINT32) * 1000;
   int errors = 0;
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Poller thread started with polling interval %u milliseconds"), sleepTime);
   while(!AgentSleepAndCheckForShutdown(sleepTime))
   {
      if (!TuxedoConnect())
      {
         if (errors % 40 == 0)  // log error once per 10 minutes with 15 sec polling interval
            nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("tpinit() call failed (%hs)"), tpstrerrordetail(tperrno, 0));
         errors++;

         TuxedoResetClients();
         TuxedoResetDomain();
         TuxedoResetMachines();
         TuxedoResetQueues();
         TuxedoResetServers();
         TuxedoResetServiceGroups();
         TuxedoResetServices();
         continue;
      }

      TuxedoQueryClients();
      TuxedoQueryDomain();
      TuxedoQueryMachines();
      TuxedoQueryQueues();
      TuxedoQueryServers();
      TuxedoQueryServiceGroups();
      TuxedoQueryServices();

      TuxedoDisconnect();
   }
   return THREAD_OK;
}

/**
 * Poller thread handle
 */
static THREAD s_pollerThread = INVALID_THREAD_HANDLE;

/**
 * Calculate stack size for Tuxedo client thread
 */
inline int GetTuxedoClientThreadStackSize()
{
#ifdef NDRX_VERSION
   return ndrx_platf_stack_get_size();
#else
   return 0;
#endif
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
#ifdef NDRX_VERSION
   static const char *envNames[] = {
      "NDRX_CONFIG",
      "NDRX_CCONFIG",
      "NDRX_CCONFIG1",
      "NDRX_CCONFIG2",
      "NDRX_CCONFIG3",
      "NDRX_CCONFIG4",
      "NDRX_CCONFIG5",
      NULL
   };
   const char *nc = NULL;
   for(int i = 0; envNames[i] != NULL; i++)
   {
      nc = getenv(envNames[i]);
      if (nc != NULL)
         break;
   }
   if (nc == NULL)
   {
      nxlog_write_tag(NXLOG_ERROR, TUXEDO_DEBUG_TAG, _T("Enduro/X environment is not configured"));
      return false;
   }
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 2, _T("Using Enduro/X configuration %hs"), nc);
#else
   const char *tc = getenv("TUXCONFIG");
   if (tc == NULL)
   {
      nxlog_write_tag(NXLOG_ERROR, TUXEDO_DEBUG_TAG, _T("TUXCONFIG environment variable not set"));
      return false;
   }
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 2, _T("Using Tuxedo configuration file %hs"), tc);
#endif

   g_tuxedoQueryLocalData = config->getValueAsUInt(_T("/Tuxedo/QueryLocalData"), g_tuxedoQueryLocalData);
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Query local data for machines is %s"),
            (g_tuxedoQueryLocalData & LOCAL_DATA_MACHINES) ? _T("ON") : _T("OFF"));
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Query local data for queues is %s"),
            (g_tuxedoQueryLocalData & LOCAL_DATA_QUEUES) ? _T("ON") : _T("OFF"));
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Query local data for servers is %s"),
            (g_tuxedoQueryLocalData & LOCAL_DATA_SERVERS) ? _T("ON") : _T("OFF"));
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Query local data for service groups is %s"),
            (g_tuxedoQueryLocalData & LOCAL_DATA_SERVICE_GROUPS) ? _T("ON") : _T("OFF"));

   g_tuxedoLocalMachineFilter = config->getValueAsBoolean(_T("/Tuxedo/FilterByLocalMachineId"), true);
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 3, _T("Filter by local machine ID is %s"), g_tuxedoLocalMachineFilter ? _T("ON") : _T("OFF"));
   s_pollerThread = ThreadCreateEx(TuxedoPollerThread, GetTuxedoClientThreadStackSize(),
         CAST_TO_POINTER(config->getValueAsUInt(_T("/Tuxedo/PollingInterval"), 10), void*));
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   ThreadJoin(s_pollerThread);
   nxlog_debug_tag(TUXEDO_DEBUG_TAG, 2, _T("Tuxedo subagent shutdown completed"));
}

/**
 * Subagent parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
#ifdef NDRX_VERSION
   { _T("EnduroX.Client.ActiveConversations(*)"), H_ClientInfo, _T("a"), DCI_DT_INT, _T("Enduro/X client {instance}: active conversations") },
   { _T("EnduroX.Client.ActiveRequests(*)"), H_ClientInfo, _T("A"), DCI_DT_INT, _T("Enduro/X client {instance} machine") },
   { _T("EnduroX.Client.Machine(*)"), H_ClientInfo, _T("N"), DCI_DT_STRING, _T("Enduro/X client {instance} name") },
   { _T("EnduroX.Client.Name(*)"), H_ClientInfo, _T("N"), DCI_DT_STRING, _T("Enduro/X client {instance} name") },
   { _T("EnduroX.Client.State(*)"), H_ClientInfo, _T("S"), DCI_DT_STRING, _T("Enduro/X client {instance} state") },	
   { _T("EnduroX.Domain.ID"), H_DomainInfo, _T("I"), DCI_DT_STRING, _T("Enduro/X domain ID") },
   { _T("EnduroX.Domain.Queues"), H_DomainInfo, _T("Q"), DCI_DT_INT,  _T("Enduro/X: number of queues") },
   { _T("EnduroX.Domain.Servers"), H_DomainInfo, _T("S"), DCI_DT_INT, _T("Enduro/X: number of servers") },
   { _T("EnduroX.Domain.Services"), H_DomainInfo, _T("s"), DCI_DT_INT, _T("Enduro/X: number of services") },
   { _T("EnduroX.Domain.State"), H_DomainInfo, _T("T"), DCI_DT_STRING, _T("Enduro/X domain state") },
   { _T("EnduroX.Machine.Accessers(*)"), H_MachineInfo, _T("A"), DCI_DT_INT, _T("Enduro/X machine {instance}: accessers") },
   { _T("EnduroX.Machine.Clients(*)"), H_MachineInfo, _T("C"), DCI_DT_INT, _T("Enduro/X machine {instance}: clients") },
   { _T("EnduroX.Machine.Conversations(*)"), H_MachineInfo, _T("o"), DCI_DT_INT, _T("Enduro/X machine {instance}: conversations") },
   { _T("EnduroX.Machine.State(*)"), H_MachineInfo, _T("S"), DCI_DT_STRING, _T("Enduro/X machine {instance} state") },
   { _T("EnduroX.Queue.Machine(*)"), H_QueueInfo, _T("M"), DCI_DT_STRING, _T("Enduro/X queue {instance}: hosting machine") },
   { _T("EnduroX.Queue.RequestsCurrent(*)"), H_QueueInfo, _T("r"), DCI_DT_STRING, _T("Enduro/X queue {instance}: current requests queued") },
   { _T("EnduroX.Queue.State(*)"), H_QueueInfo, _T("s"), DCI_DT_STRING, _T("Enduro/X queue {instance} state") },
   { _T("EnduroX.ServerInstance.CommandLine(*)"), H_ServerInstanceInfo, _T("C"), DCI_DT_STRING, _T("Enduro/X server instance {instance}: command line") },
   { _T("EnduroX.ServerInstance.Generation(*)"), H_ServerInstanceInfo, _T("G"), DCI_DT_INT, _T("Enduro/X server instance {instance}: generation") },
   { _T("EnduroX.ServerInstance.Machine(*)"), H_ServerInstanceInfo, _T("M"), DCI_DT_STRING, _T("Enduro/X server instance {instance}: machine ID") },
   { _T("EnduroX.ServerInstance.Name(*)"), H_ServerInstanceInfo, _T("N"), DCI_DT_STRING, _T("Enduro/X server instance {instance}: name") },
   { _T("EnduroX.ServerInstance.PID(*)"), H_ServerInstanceInfo, _T("P"), DCI_DT_INT, _T("Enduro/X server instance {instance}: process ID") },
   { _T("EnduroX.ServerInstance.State(*)"), H_ServerInstanceInfo, _T("S"), DCI_DT_STRING, _T("Enduro/X server instance {instance}: state") },
   { _T("EnduroX.Service.State(*)"), H_ServiceInfo, _T("S"), DCI_DT_STRING, _T("Enduro/X service {instance}: state") },
   { _T("EnduroX.ServiceGroup.CompletedRequests(*)"), H_ServiceGroupInfo, _T("C"), DCI_DT_INT, _T("Enduro/X service group {instance}: completed requests") },
   { _T("EnduroX.ServiceGroup.FailedRequests(*)"), H_ServiceGroupInfo, _T("f"), DCI_DT_INT, _T("Enduro/X service group {instance}: failed requests") },
   { _T("EnduroX.ServiceGroup.LastExecutionTime(*)"), H_ServiceGroupInfo, _T("e"), DCI_DT_INT, _T("Enduro/X service group {instance}: last execution time") },
   { _T("EnduroX.ServiceGroup.MaxExecutionTime(*)"), H_ServiceGroupInfo, _T("x"), DCI_DT_INT, _T("Enduro/X service group {instance}: max execution time") },
   { _T("EnduroX.ServiceGroup.MinExecutionTime(*)"), H_ServiceGroupInfo, _T("m"), DCI_DT_INT, _T("Enduro/X service group {instance}: min execution time") },
   { _T("EnduroX.ServiceGroup.State(*)"), H_ServiceGroupInfo, _T("S"), DCI_DT_STRING, _T("Enduro/X service group {instance}: state") },
   { _T("EnduroX.ServiceGroup.SuccessfulRequests(*)"), H_ServiceGroupInfo, _T("s"), DCI_DT_INT, _T("Enduro/X service group {instance}: successful requests") }
#else
   { _T("Tuxedo.Client.ActiveConversations(*)"), H_ClientInfo, _T("a"), DCI_DT_INT, _T("Tuxedo client {instance}: active conversations") },
   { _T("Tuxedo.Client.ActiveRequests(*)"), H_ClientInfo, _T("A"), DCI_DT_INT, _T("Tuxedo client {instance} machine") },
   { _T("Tuxedo.Client.Machine(*)"), H_ClientInfo, _T("N"), DCI_DT_STRING, _T("Tuxedo client {instance} name") },
   { _T("Tuxedo.Client.Name(*)"), H_ClientInfo, _T("N"), DCI_DT_STRING, _T("Tuxedo client {instance} name") },
   { _T("Tuxedo.Client.State(*)"), H_ClientInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo client {instance} state") },
   { _T("Tuxedo.IsMasterMachine"), H_IsMasterMachine, NULL, DCI_DT_INT, _T("Tuxedo master machine flag") },
   { _T("Tuxedo.Domain.ID"), H_DomainInfo, _T("I"), DCI_DT_STRING, _T("Tuxedo domain ID") },
   { _T("Tuxedo.Domain.Master"), H_DomainInfo, _T("M"), DCI_DT_STRING, _T("Tuxedo domain master/backup machines") },
   { _T("Tuxedo.Domain.Model"), H_DomainInfo, _T("m"), DCI_DT_STRING, _T("Tuxedo domain model") },
   { _T("Tuxedo.Domain.Queues"), H_DomainInfo, _T("Q"), DCI_DT_INT,  _T("Tuxedo: number of queues") },
   { _T("Tuxedo.Domain.Routes"), H_DomainInfo, _T("R"), DCI_DT_INT, _T("Tuxedo: bulletin board routing table entries") },
   { _T("Tuxedo.Domain.Servers"), H_DomainInfo, _T("S"), DCI_DT_INT, _T("Tuxedo: number of servers") },
   { _T("Tuxedo.Domain.Services"), H_DomainInfo, _T("s"), DCI_DT_INT, _T("Tuxedo: number of services") },
   { _T("Tuxedo.Domain.State"), H_DomainInfo, _T("T"), DCI_DT_STRING, _T("Tuxedo domain state") },
   { _T("Tuxedo.LocalMachineId"), H_LocalMachineId, NULL, DCI_DT_STRING, _T("Tuxedo machine ID for local host") },
   { _T("Tuxedo.Machine.Accessers(*)"), H_MachineInfo, _T("A"), DCI_DT_INT, _T("Tuxedo machine {instance}: accessers") },
   { _T("Tuxedo.Machine.Bridge(*)"), H_MachineInfo, _T("B"), DCI_DT_STRING, _T("Tuxedo machine {instance} bridge") },
   { _T("Tuxedo.Machine.Clients(*)"), H_MachineInfo, _T("C"), DCI_DT_INT, _T("Tuxedo machine {instance}: clients") },
   { _T("Tuxedo.Machine.Conversations(*)"), H_MachineInfo, _T("o"), DCI_DT_INT, _T("Tuxedo machine {instance}: conversations") },
   { _T("Tuxedo.Machine.Load(*)"), H_MachineInfo, _T("L"), DCI_DT_INT, _T("Tuxedo machine {instance}: load") },
   { _T("Tuxedo.Machine.PhysicalID(*)"), H_MachineInfo, _T("P"), DCI_DT_STRING, _T("Tuxedo machine {instance} physical machine ID") },
   { _T("Tuxedo.Machine.Role(*)"), H_MachineInfo, _T("R"), DCI_DT_STRING, _T("Tuxedo machine {instance} role") },
   { _T("Tuxedo.Machine.SoftwareRelease(*)"), H_MachineInfo, _T("s"), DCI_DT_STRING, _T("Tuxedo machine {instance}: software release") },
   { _T("Tuxedo.Machine.State(*)"), H_MachineInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo machine {instance} state") },
   { _T("Tuxedo.Machine.Type(*)"), H_MachineInfo, _T("T"), DCI_DT_STRING, _T("Tuxedo machine {instance} type") },
   { _T("Tuxedo.Machine.WorkloadsInitiated(*)"), H_MachineInfo, _T("W"), DCI_DT_INT, _T("Tuxedo machine {instance}: workloads initiated") },
   { _T("Tuxedo.Machine.WorkloadsProcessed(*)"), H_MachineInfo, _T("w"), DCI_DT_INT, _T("Tuxedo machine {instance}: workloads processed") },
   { _T("Tuxedo.Machine.WorkstationClients(*)"), H_MachineInfo, _T("c"), DCI_DT_INT, _T("Tuxedo machine {instance}: workstation clients") },
   { _T("Tuxedo.Queue.Machine(*)"), H_QueueInfo, _T("M"), DCI_DT_STRING, _T("Tuxedo queue {instance}: hosting machine") },
   { _T("Tuxedo.Queue.RequestsCurrent(*)"), H_QueueInfo, _T("r"), DCI_DT_STRING, _T("Tuxedo queue {instance}: current requests queued") },
   { _T("Tuxedo.Queue.RequestsTotal(*)"), H_QueueInfo, _T("R"), DCI_DT_STRING, _T("Tuxedo queue {instance}: total requests queued") },
   { _T("Tuxedo.Queue.Server(*)"), H_QueueInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo queue {instance}: server executable") },
   { _T("Tuxedo.Queue.ServerCount(*)"), H_QueueInfo, _T("C"), DCI_DT_STRING, _T("Tuxedo queue {instance}: server count") },
   { _T("Tuxedo.Queue.State(*)"), H_QueueInfo, _T("s"), DCI_DT_STRING, _T("Tuxedo queue {instance} state") },
   { _T("Tuxedo.Queue.WorkloadsCurrent(*)"), H_QueueInfo, _T("w"), DCI_DT_STRING, _T("Tuxedo queue {instance}: current workloads queued") },
   { _T("Tuxedo.Queue.WorkloadsTotal(*)"), H_QueueInfo, _T("W"), DCI_DT_STRING, _T("Tuxedo queue {instance}: total workloads queued") },
   { _T("Tuxedo.Server.ActiveRequests(*)"), H_ServerInfo, _T("A"), DCI_DT_INT, _T("Tuxedo server {instance}: active requests") },
   { _T("Tuxedo.Server.CommandLine(*)"), H_ServerInfo, _T("C"), DCI_DT_STRING, _T("Tuxedo server {instance}: command line") },
   { _T("Tuxedo.Server.Group(*)"), H_ServerInfo, _T("g"), DCI_DT_STRING, _T("Tuxedo server {instance}: server group") },
   { _T("Tuxedo.Server.Machine(*)"), H_ServerInfo, _T("M"), DCI_DT_STRING, _T("Tuxedo server {instance}: machine ID") },
   { _T("Tuxedo.Server.MaxInstances(*)"), H_ServerInfo, _T("x"), DCI_DT_INT, _T("Tuxedo server {instance}: maximum instances") },
   { _T("Tuxedo.Server.MinInstances(*)"), H_ServerInfo, _T("i"), DCI_DT_INT, _T("Tuxedo server {instance}: minimum instances") },
   { _T("Tuxedo.Server.Name(*)"), H_ServerInfo, _T("N"), DCI_DT_STRING, _T("Tuxedo server {instance}: name") },
   { _T("Tuxedo.Server.ProcessedRequests(*)"), H_ServerInstanceInfo, _T("R"), DCI_DT_INT, _T("Tuxedo server instance {instance}: processed requests") },
   { _T("Tuxedo.Server.ProcessedWorkloads(*)"), H_ServerInstanceInfo, _T("W"), DCI_DT_INT, _T("Tuxedo server instance {instance}: processed workloads") },
   { _T("Tuxedo.Server.RunningInstances(*)"), H_ServerInfo, _T("r"), DCI_DT_INT, _T("Tuxedo server {instance}: running instances") },
   { _T("Tuxedo.ServerInstance.ActiveRequests(*)"), H_ServerInstanceInfo, _T("A"), DCI_DT_INT, _T("Tuxedo server instance {instance}: active requests") },
   { _T("Tuxedo.ServerInstance.BaseID(*)"), H_ServerInstanceInfo, _T("B"), DCI_DT_INT, _T("Tuxedo server instance {instance}: base ID") },
   { _T("Tuxedo.ServerInstance.CommandLine(*)"), H_ServerInstanceInfo, _T("C"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: command line") },
   { _T("Tuxedo.ServerInstance.CurrentService(*)"), H_ServerInstanceInfo, _T("c"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: current service") },
   { _T("Tuxedo.ServerInstance.Generation(*)"), H_ServerInstanceInfo, _T("G"), DCI_DT_INT, _T("Tuxedo server instance {instance}: generation") },
   { _T("Tuxedo.ServerInstance.Group(*)"), H_ServerInstanceInfo, _T("g"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: server group") },
   { _T("Tuxedo.ServerInstance.Machine(*)"), H_ServerInstanceInfo, _T("M"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: machine ID") },
   { _T("Tuxedo.ServerInstance.Name(*)"), H_ServerInstanceInfo, _T("N"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: name") },
   { _T("Tuxedo.ServerInstance.PID(*)"), H_ServerInstanceInfo, _T("P"), DCI_DT_INT, _T("Tuxedo server instance {instance}: process ID") },
   { _T("Tuxedo.ServerInstance.ProcessedRequests(*)"), H_ServerInstanceInfo, _T("R"), DCI_DT_INT, _T("Tuxedo server instance {instance}: processed requests") },
   { _T("Tuxedo.ServerInstance.ProcessedWorkloads(*)"), H_ServerInstanceInfo, _T("W"), DCI_DT_INT, _T("Tuxedo server instance {instance}: processed workloads") },
   { _T("Tuxedo.ServerInstance.State(*)"), H_ServerInstanceInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo server instance {instance}: state") },
   { _T("Tuxedo.Service.Load(*)"), H_ServiceInfo, _T("L"), DCI_DT_INT, _T("Tuxedo service {instance}: load") },
   { _T("Tuxedo.Service.Priority(*)"), H_ServiceInfo, _T("P"), DCI_DT_INT, _T("Tuxedo service {instance}: priority") },
   { _T("Tuxedo.Service.RoutingName(*)"), H_ServiceInfo, _T("R"), DCI_DT_STRING, _T("Tuxedo service {instance}: routing name") },
   { _T("Tuxedo.Service.State(*)"), H_ServiceInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo service {instance}: state") },
   { _T("Tuxedo.ServiceGroup.CompletedRequests(*)"), H_ServiceGroupInfo, _T("C"), DCI_DT_INT, _T("Tuxedo service group {instance}: completed requests") },
   { _T("Tuxedo.ServiceGroup.FailedRequests(*)"), H_ServiceGroupInfo, _T("f"), DCI_DT_INT, _T("Tuxedo service group {instance}: failed requests") },
   { _T("Tuxedo.ServiceGroup.LastExecutionTime(*)"), H_ServiceGroupInfo, _T("e"), DCI_DT_INT, _T("Tuxedo service group {instance}: last execution time") },
   { _T("Tuxedo.ServiceGroup.Load(*)"), H_ServiceGroupInfo, _T("L"), DCI_DT_INT, _T("Tuxedo service group {instance}: load") },
   { _T("Tuxedo.ServiceGroup.MaxExecutionTime(*)"), H_ServiceGroupInfo, _T("x"), DCI_DT_INT, _T("Tuxedo service group {instance}: max execution time") },
   { _T("Tuxedo.ServiceGroup.MinExecutionTime(*)"), H_ServiceGroupInfo, _T("m"), DCI_DT_INT, _T("Tuxedo service group {instance}: min execution time") },
   { _T("Tuxedo.ServiceGroup.Priority(*)"), H_ServiceGroupInfo, _T("P"), DCI_DT_INT, _T("Tuxedo service group {instance}: priority") },
   { _T("Tuxedo.ServiceGroup.QueuedRequests(*)"), H_ServiceGroupInfo, _T("Q"), DCI_DT_INT, _T("Tuxedo service group {instance}: queued requests") },
   { _T("Tuxedo.ServiceGroup.RequestAddress(*)"), H_ServiceGroupInfo, _T("A"), DCI_DT_STRING, _T("Tuxedo service group {instance}: request address") },
   { _T("Tuxedo.ServiceGroup.RoutingName(*)"), H_ServiceGroupInfo, _T("R"), DCI_DT_STRING, _T("Tuxedo service group {instance}: routing name") },
   { _T("Tuxedo.ServiceGroup.State(*)"), H_ServiceGroupInfo, _T("S"), DCI_DT_STRING, _T("Tuxedo service group {instance}: state") },
   { _T("Tuxedo.ServiceGroup.SuccessfulRequests(*)"), H_ServiceGroupInfo, _T("s"), DCI_DT_INT, _T("Tuxedo service group {instance}: successful requests") }
#endif
};

/**
 * Subagent lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
#ifdef NDRX_VERSION
   { _T("EnduroX.Clients"), H_ClientsList, NULL },
   { _T("EnduroX.Machines"), H_MachinesList, NULL },
   { _T("EnduroX.Queues"), H_QueuesList, NULL },
   { _T("EnduroX.ServerInstances"), H_ServerInstancesList, NULL },
   { _T("EnduroX.Servers"), H_ServersList, NULL },
   { _T("EnduroX.ServiceGroups"), H_ServiceGroupsList, NULL },
   { _T("EnduroX.Services"), H_ServicesList, NULL }
#else
   { _T("Tuxedo.Clients"), H_ClientsList, NULL },
   { _T("Tuxedo.Machines"), H_MachinesList, NULL },
   { _T("Tuxedo.Queues"), H_QueuesList, NULL },
   { _T("Tuxedo.ServerInstances"), H_ServerInstancesList, NULL },
   { _T("Tuxedo.Servers"), H_ServersList, NULL },
   { _T("Tuxedo.ServiceGroups"), H_ServiceGroupsList, NULL },
   { _T("Tuxedo.Services"), H_ServicesList, NULL }
#endif
};

/**
 * Subagent tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
#ifdef NDRX_VERSION
   { _T("EnduroX.Clients"), H_ClientsTable, NULL, _T("ID"), _T("Enduro/X clients") },
   { _T("EnduroX.Machines"), H_MachinesTable, NULL, _T("ID"), _T("Enduro/X machines") },
   { _T("EnduroX.Queues"), H_QueuesTable, NULL, _T("NAME"), _T("Enduro/X queues") },
   { _T("EnduroX.ServerInstances"), H_ServerInstancesTable, NULL, _T("ID"), _T("Enduro/X server instances") },
   { _T("EnduroX.Servers"), H_ServersTable, NULL, _T("BASE_ID"), _T("Enduro/X servers") },
   { _T("EnduroX.ServiceGroups"), H_ServiceGroupsTable, NULL, _T("SVCNAME,SRVGROUP"), _T("Enduro/X service groups") },
   { _T("EnduroX.Services"), H_ServicesTable, NULL, _T("NAME"), _T("Enduro/X services") }
#else
   { _T("Tuxedo.Clients"), H_ClientsTable, NULL, _T("ID"), _T("Tuxedo clients") },
   { _T("Tuxedo.Machines"), H_MachinesTable, NULL, _T("ID"), _T("Tuxedo machines") },
   { _T("Tuxedo.Queues"), H_QueuesTable, NULL, _T("NAME"), _T("Tuxedo queues") },
   { _T("Tuxedo.ServerInstances"), H_ServerInstancesTable, NULL, _T("ID"), _T("Tuxedo server instances") },
   { _T("Tuxedo.Servers"), H_ServersTable, NULL, _T("BASE_ID"), _T("Tuxedo servers") },
   { _T("Tuxedo.ServiceGroups"), H_ServiceGroupsTable, NULL, _T("SVCNAME,SRVGROUP"), _T("Tuxedo service groups") },
   { _T("Tuxedo.Services"), H_ServicesTable, NULL, _T("NAME"), _T("Tuxedo services") }
#endif
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("TUXEDO"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   s_tables,
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(TUXEDO)
{
   *ppInfo = &s_info;
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
