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
 * Host connection constructor
 */
HostConnections::HostConnections(const TCHAR *name, const char *url, const char *login, const char *password)
{
   m_name = _tcsdup(name);
   m_url = strdup(url);
   m_login = strdup(login);
   m_password = strdup(password);
   m_domains.setMalloc(false);
   m_iface.setMalloc(false);
}

/**
 * Host connection initial constructor
 */
HostConnections::~HostConnections()
{
   disconnect();
   free(m_url);
   free(m_name);
   free(m_login);
   free(m_password);
}

/**
 * Authentication callback
 */
int HostConnections::authCb(virConnectCredentialPtr cred, unsigned int ncred, void *cbdata)
{
   HostConnections *obj = (HostConnections *)cbdata;
   for (int i = 0; i < ncred; i++)
   {
      if (cred[i].type == VIR_CRED_AUTHNAME)
      {
         cred[i].result = strdup(obj->m_login);
         cred[i].resultlen = strlen(obj->m_login);
      }
      else if (cred[i].type == VIR_CRED_PASSPHRASE)
      {
         cred[i].result = strdup(obj->m_password);
         cred[i].resultlen = strlen(obj->m_password);
      }
    }

    return 0;
}

/**
 * Authentication credentials
 */
static int authCreds[] = {
    VIR_CRED_AUTHNAME,
    VIR_CRED_PASSPHRASE,
};

/**
 * Function that initiate connection to host
 */
bool HostConnections::connect()
{
   virConnectAuth auth;
   auth.cb = authCb;
   auth.cbdata = this;
   auth.credtype = authCreds;
   auth.ncredtype = sizeof(authCreds)/sizeof(int);
   m_connection = virConnectOpenAuth(m_url, &auth, 0);
   return m_connection != NULL;
}

/**
 * Function that close connection to host and cleans all libvirt specific data
 */
void HostConnections::disconnect()
{
   if(m_connection != NULL)
      virConnectClose(m_connection);
}

/**
 * Updates capability XML chase if required and returns it or null
 */
const char *HostConnections::getCapabilitiesAndLock()
{
   if(m_capabilities.shouldUpdate())
   {
      m_capabilities.setWriteLock();
      char *caps = virConnectGetCapabilities(m_connection);
      if (caps == NULL)
      {
         virError err;
         virCopyLastError(&err);
         AgentWriteLog(6, _T("VMGR: virConnectGetCapabilities failed: %hs"), err.message);
         virResetError(&err);
         return NULL;
      }
      m_capabilities.updateChaseAndUnlock(caps);
      //AgentWriteLog(NXLOG_DEBUG, _T("VMGR: Capabilities of connection %hs: %hs"), m_url, caps);
   }

   return m_capabilities.getDataAndLock();
}

void HostConnections::unlockCapabilities()
{
   m_capabilities.unlock();
}

/**
 * Retrieves maximum number of virtual CPUs per-guest the underlying virtualization technology supports, or -1 in case of error
 */
UINT32 HostConnections::getMaxVCpuCount()
{
   return virConnectGetMaxVcpus(m_connection, NULL);
}

/**
 * Retrieves the total amount of free memory on the virtualization host in KiB, or 0 in case of error
 */
UINT64 HostConnections::getHostFreeMemory()
{
   return virNodeGetFreeMemory(m_connection);
}

/**
 * Updates virNodeInfo structure chase if required and returns it or null in case of error
 */
const virNodeInfo *HostConnections::getNodeInfoAndLock()
{
   if(m_nodeInfo.shouldUpdate())
   {
      m_nodeInfo.setWriteLock();
      virNodeInfo *nodeInfo = (virNodeInfo *) malloc(sizeof(virNodeInfo));
      if (virNodeGetInfo(m_connection, nodeInfo) == -1)
      {
         virError err;
         virCopyLastError(&err);
         AgentWriteLog(6, _T("VMGR: virConnectGetCapabilities failed: %hs"), err.message);
         virResetError(&err);
         free(nodeInfo);
         return NULL;
      }
      m_nodeInfo.updateChaseAndUnlock(nodeInfo);
   }

   return m_nodeInfo.getDataAndLock();
}

void HostConnections::unlockNodeInfo()
{
   m_nodeInfo.unlock();
}

/**
 * Returns the type of virtualization in use on this connection, or null in case of error
 */
const char *HostConnections::getConnectionType()
{
   return virConnectGetType(m_connection);
}

/**
 * Returns the version of the host virtualization software in use, or -1 in case of error
 */
UINT64 HostConnections::getConnectionVersion()
{
   unsigned long int ver;
   if(virConnectGetVersion(m_connection, &ver) == 0)
      return (UINT64)ver;
   else
      return -1;
}

/**
 * Returns the version of the libvirt software in use on the host, or -1 in case of error
 */
UINT64 HostConnections::getLibraryVersion()
{
   unsigned long int ver;
   if(virConnectGetLibVersion(m_connection, &ver) == 0)
      return (UINT64)ver;
   else
      return -1;
}

/**---------
 * Donamins
 *----------/

/**
 * List running VMs(domains)
 */
const StringObjectMap<virDomain> *HostConnections::getDomainListAndLock()
{
   if(m_domains.shouldUpdate())
   {
      m_domains.setWriteLock();
      virDomainPtr *vms;
      int ret = virConnectListAllDomains(m_connection, &vms, 0);
      if (ret < 0)
         return NULL;

      StringObjectMapC<virDomain> *allDomains = new StringObjectMapC<virDomain>(true);

      for (int i = 0; i < ret; i++)
      {
#ifdef UNICODE
         allDomains->setPreallocated(WideStringFromMBString(virDomainGetName(vms[i])), vms[i]);
#else
         allDomains->set(virDomainGetName(vms[i]), vms[i]);
#endif
      }
      free(vms);
      m_domains.updateChaseAndUnlock(allDomains);
   }
   return m_domains.getDataAndLock();
}

void HostConnections::unlockDomainList()
{
   m_domains.unlock();
}

/**---------
 * Iface
 */

/**
 * List interfaces
 */
const StringList *HostConnections::getIfaceListAndLock()
{
   if(m_iface.shouldUpdate())
   {
      m_iface.setWriteLock();

      int activeCount = virConnectNumOfInterfaces(m_connection);
      int inactiveCount = virConnectNumOfDefinedInterfaces(m_connection);
      StringList *ifaceList = new StringList();

      if(activeCount != -1)
      {
         char **active = (char **)malloc(sizeof(char *) * activeCount);
         virConnectListInterfaces(m_connection, active, activeCount);
         for(int i=0; i < activeCount; i++)
         {
            virInterfacePtr iface = virInterfaceLookupByName(m_connection, active[i]);
            //AgentWriteLog(6, _T("VMGR: iface info!!!: %hs"), virInterfaceGetXMLDesc(iface, 0));

            ifaceList->addMBString(active[i]);
            free(active[i]);
         }
         free(active);
      }

      if(inactiveCount != -1)
      {
         char **inactive = (char **)malloc(sizeof(char *) * inactiveCount);
         virConnectListDefinedInterfaces(m_connection, inactive, inactiveCount);
         for(int i=0; i < inactiveCount; i++)
         {
            virInterfacePtr iface = virInterfaceLookupByName(m_connection, inactive[i]);
            //AgentWriteLog(6, _T("VMGR: iface info!!!: %hs"), virInterfaceGetXMLDesc(iface, 0));

            ifaceList->addMBString(inactive[i]);
            free(inactive[i]);
         }
         free(inactive);
      }
      m_iface.updateChaseAndUnlock(ifaceList);
   }
   return m_iface.getDataAndLock();
}

void HostConnections::unlockIfaceList()
{
   m_iface.unlock();
}
