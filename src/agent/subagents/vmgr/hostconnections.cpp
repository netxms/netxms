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
                                 : m_vmInfo(true),m_vmXMLs(true),m_networkXMLs(true),m_storageInfo(true),m_domains(false),m_iface(false),
                                 m_networks(false),m_storages(false)
{
   m_name = _tcsdup(name);
   m_url = strdup(url);
   m_login = strdup(login);
   m_password = strdup(password);
   m_vmXMLMutex = MutexCreate();
   m_networkXMLMutex = MutexCreate();
   m_storageInfoMutex = MutexCreate();
   m_vmInfoMutex = MutexCreate();
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
   MutexDestroy(m_vmXMLMutex);
   MutexDestroy(m_networkXMLMutex);
   MutexDestroy(m_storageInfoMutex);
   MutexDestroy(m_vmInfoMutex);
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
   m_capabilities.lock();
   if(m_capabilities.shouldUpdate())
   {
      char *caps = virConnectGetCapabilities(m_connection);
      if (caps == NULL)
      {
         virError err;
         virCopyLastError(&err);
         AgentWriteLog(6, _T("VMGR: virConnectGetCapabilities failed: %hs"), err.message);
         virResetError(&err);
         return NULL;
      }
      m_capabilities.update(caps);
      //AgentWriteLog(NXLOG_DEBUG, _T("VMGR: Capabilities of connection %hs: %hs"), m_url, caps);
   }

   return m_capabilities.getData();
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
   m_nodeInfo.lock();
   if(m_nodeInfo.shouldUpdate())
   {
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
      m_nodeInfo.update(nodeInfo);
   }

   return m_nodeInfo.getData();
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
const StringObjectMap<NXvirDomain> *HostConnections::getDomainListAndLock()
{
   m_domains.lock();
   if(m_domains.shouldUpdate())
   {
      int numActiveDomains = virConnectNumOfDomains(m_connection);
      int numInactiveDomains = virConnectNumOfDefinedDomains(m_connection);

      char **inactiveDomains = (char **)malloc(sizeof(char *) * numInactiveDomains);
      int *activeDomains = (int *)malloc(sizeof(int) * numActiveDomains);

      numActiveDomains = virConnectListDomains(m_connection, activeDomains, numActiveDomains);
      numInactiveDomains = virConnectListDefinedDomains(m_connection, inactiveDomains, numInactiveDomains);

      StringObjectMap<NXvirDomain> *allDomains = new StringObjectMap<NXvirDomain>(true);

      for (int i = 0 ; i < numActiveDomains ; i++)
      {
         virDomainPtr vm = virDomainLookupByID(m_connection, activeDomains[i]);
#ifdef UNICODE
         allDomains->setPreallocated(WideStringFromMBString(virDomainGetName(vm)), new NXvirDomain(vm));
#else
         allDomains->set(virDomainGetName(vm), new NXvirDomain(vm));
#endif
      }

      for (int i = 0 ; i < numInactiveDomains ; i++)
      {
 #ifdef UNICODE
         allDomains->setPreallocated(WideStringFromMBString(inactiveDomains[i]), new NXvirDomain(virDomainLookupByName(m_connection, inactiveDomains[i])));
         free(inactiveDomains[i]);
#else
         allDomains->setPreallocated(inactiveDomains[i], new NXvirDomain(virDomainLookupByName(m_connection, inactiveDomains[i])));
#endif
      }

      free(activeDomains);
      free(inactiveDomains);

      m_domains.update(allDomains);
   }
   return m_domains.getData();
}

/**
 * Unlocks domain list
 */
void HostConnections::unlockDomainList()
{
   m_domains.unlock();
}

/**
 * Returns domain definition in XML format
 */
const char *HostConnections::getDomainDefenitionAndLock(const TCHAR *name, NXvirDomain *vm)
{
   MutexLock(m_vmXMLMutex);
   Cashe<char> *xmlChase = m_vmXMLs.get(name);
   if (xmlChase == NULL || xmlChase->shouldUpdate())
   {
      bool getDomain = vm == NULL;
      if(getDomain)
      {
         const StringObjectMap<NXvirDomain> *vmMap = getDomainListAndLock();
         vm = vmMap->get(name);
      }
      if(vm != NULL)
      {
         char *xml = virDomainGetXMLDesc(*vm, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
         if(xmlChase == NULL)
         {
            xmlChase = new Cashe<char>();
            m_vmXMLs.set(name, xmlChase);
         }
         xmlChase->update(xml);

      }
      else
      {
         m_vmXMLs.remove(name);
         xmlChase = NULL;
      }

      if(getDomain)
         unlockDomainList();
   }
/*
   if(xmlChase != NULL)
      AgentWriteLog(6, _T("VMGR: ******Domain defenition: %hs"), xmlChase->getData());
   else
      AgentWriteLog(6, _T("VMGR: ******Domain defenition: NULL"));
*/
   return xmlChase != NULL ? xmlChase->getData() : NULL;
}

/**
 * Unlocks domain definition
 */
void HostConnections::unlockDomainDefenition()
{
   MutexUnlock(m_vmXMLMutex);
}

/**
 * Returns domain information
 */
const virDomainInfo *HostConnections::getDomainInfoAndLock(const TCHAR *domainName, NXvirDomain *vm)
{
   MutexLock(m_vmInfoMutex);
   Cashe<virDomainInfo> *infoChase = m_vmInfo.get(domainName);
   if (infoChase == NULL || infoChase->shouldUpdate())
   {
      bool getDomain = vm == NULL;
      if(getDomain)
      {
         const StringObjectMap<NXvirDomain> *vmMap = getDomainListAndLock();
         vm = vmMap->get(domainName);
      }
      if(vm != NULL)
      {
         virDomainInfo *info = new virDomainInfo();
         if(virDomainGetInfo(*vm, info) == 0)
         {
            if(infoChase == NULL)
            {
               infoChase = new Cashe<virDomainInfo>();
               m_vmInfo.set(domainName, infoChase);
            }
            infoChase->update(info);
         }
         else
         {
            delete info;
         }
      }
      else
      {
         m_vmInfo.remove(domainName);
         infoChase = NULL;
      }

      if(getDomain)
         unlockDomainList();
   }

   return infoChase != NULL ? infoChase->getData() : NULL;
}

/**
 * Unlocks domain control information
 */
void HostConnections::unlockDomainInfo()
{
   MutexUnlock(m_vmInfoMutex);
}

/**---------
 * Iface
 */

/**
 * List interfaces
 */
const StringList *HostConnections::getIfaceListAndLock()
{
   m_iface.lock();
   if(m_iface.shouldUpdate())
   {
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
            virInterfaceFree(iface);
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
            virInterfaceFree(iface);
            ifaceList->addMBString(inactive[i]);
            free(inactive[i]);
         }
         free(inactive);
      }
      m_iface.update(ifaceList);
   }
   return m_iface.getData();
}

void HostConnections::unlockIfaceList()
{
   m_iface.unlock();
}

/**---------
 * Networks
 */

/**
 * List networks
 */
const StringObjectMap<NXvirNetwork> *HostConnections::getNetworkListAndLock()
{
   m_networks.lock();
   if(m_networks.shouldUpdate())
   {
      int numActiveNetworks = virConnectNumOfNetworks(m_connection);
      int numInactiveNetworks = virConnectNumOfDefinedNetworks(m_connection);

      char **inactiveNetworks = (char **)malloc(sizeof(char *) * numInactiveNetworks);
      char **activeNetworks = (char **)malloc(sizeof(char *) * numActiveNetworks);

      numActiveNetworks = virConnectListNetworks(m_connection, activeNetworks, numActiveNetworks);
      numInactiveNetworks = virConnectListDefinedNetworks(m_connection, inactiveNetworks, numInactiveNetworks);

      StringObjectMap<NXvirNetwork> *allNetworks = new StringObjectMap<NXvirNetwork>(true);

      for (int i = 0 ; i < numActiveNetworks ; i++)
      {
#ifdef UNICODE
         allNetworks->setPreallocated(WideStringFromMBString(activeNetworks[i]), new NXvirNetwork(virNetworkLookupByName(m_connection, activeNetworks[i])));
         free(activeNetworks[i]);
#else
         allNetworks->set(activeNetworks[i], new NXvirNetwork(virNetworkLookupByName(m_connection, activeNetworks[i])));
#endif
      }

      for (int i = 0 ; i < numInactiveNetworks ; i++)
      {
 #ifdef UNICODE
         allNetworks->setPreallocated(WideStringFromMBString(inactiveNetworks[i]), new NXvirNetwork(virNetworkLookupByName(m_connection, inactiveNetworks[i])));
         free(inactiveNetworks[i]);
#else
         allNetworks->setPreallocated(inactiveNetworks[i], new NXvirNetwork(virNetworkLookupByName(m_connection, inactiveNetworks[i])));
#endif
      }

      free(activeNetworks);
      free(inactiveNetworks);

      m_networks.update(allNetworks);
   }
   //AgentWriteLog(6, _T("VMGR: ******Network list size: %d"), m_networks.getData()->size());
   return m_networks.getData();
}

void HostConnections::unlockNetworkList()
{
   m_networks.unlock();
}

/**
 * Returns network definition in XML format
 */
const char *HostConnections::getNetworkDefenitionAndLock(const TCHAR *name, NXvirNetwork *network)
{
   MutexLock(m_networkXMLMutex);
   Cashe<char> *xmlChase = m_networkXMLs.get(name);
   if (xmlChase == NULL || xmlChase->shouldUpdate())
   {
      bool getNetwork = network == NULL;
      if(getNetwork)
      {
         const StringObjectMap<NXvirNetwork> *networkMap = getNetworkListAndLock();
         network = networkMap->get(name);
      }
      if(network != NULL)
      {
         char *xml = virNetworkGetXMLDesc(*network, 0);
         if(xmlChase == NULL)
         {
            xmlChase = new Cashe<char>();
            m_networkXMLs.set(name, xmlChase);
         }
         xmlChase->update(xml);
      }
      else
      {
         m_networkXMLs.remove(name);
         xmlChase = NULL;
      }

      if(getNetwork)
         unlockNetworkList();
   }
/*
   if(xmlChase != NULL)
      AgentWriteLog(6, _T("VMGR: ******Network defenition: %hs"), xmlChase->getData());
   else
      AgentWriteLog(6, _T("VMGR: ******Network defenition: NULL"));
*/
   return xmlChase != NULL ? xmlChase->getData() : NULL;
}

/**
 * Unlocks network definition
 */
void HostConnections::unlockNetworkDefenition()
{
   MutexUnlock(m_networkXMLMutex);
}

/**---------
 * Storages
 */

/**
 * List volumes
 */
const StringObjectMap<NXvirStoragePool> *HostConnections::getStorageListAndLock()
{
   m_storages.lock();
   if(m_storages.shouldUpdate())
   {
      int numActive = virConnectNumOfStoragePools(m_connection);
      int numInactive = virConnectNumOfDefinedStoragePools(m_connection);

      char **inactive = (char **)malloc(sizeof(char *) * numInactive);
      char **active = (char **)malloc(sizeof(char *) * numActive);

      numActive = virConnectListStoragePools(m_connection, active, numActive);
      numInactive = virConnectListDefinedStoragePools(m_connection, inactive, numInactive);

      StringObjectMap<NXvirStoragePool> *allStorage = new StringObjectMap<NXvirStoragePool>(true);

      for (int i = 0 ; i < numActive; i++)
      {
#ifdef UNICODE
         allStorage->setPreallocated(WideStringFromMBString(active[i]), new NXvirStoragePool(virStoragePoolLookupByName(m_connection, active[i])));
         free(active[i]);
#else
         allStorage->set(active[i], new NXvirStoragePool(virStoragePoolLookupByName(m_connection, active[i])));
#endif
      }

      for (int i = 0 ; i < numInactive; i++)
      {
 #ifdef UNICODE
         allStorage->setPreallocated(WideStringFromMBString(inactive[i]), new NXvirStoragePool(virStoragePoolLookupByName(m_connection, inactive[i])));
         free(inactive[i]);
#else
         allStorage->setPreallocated(inactive[i], new NXvirStoragePool(virStoragePoolLookupByName(m_connection, inactive[i])));
#endif
      }

      free(active);
      free(inactive);

      m_storages.update(allStorage);
   }
   //AgentWriteLog(6, _T("VMGR: ******Storage list size: %d"), m_storages.getData()->size());
   return m_storages.getData();
}

void HostConnections::unlockStorageList()
{
   m_storages.unlock();
}

/**
 * Returns storage definition in XML format
 */
const virStoragePoolInfo *HostConnections::getStorageInformationAndLock(const TCHAR *name, NXvirStoragePool *storage)
{
   MutexLock(m_storageInfoMutex);
   Cashe<virStoragePoolInfo> *info = m_storageInfo.get(name);
   if (info == NULL || info->shouldUpdate())
   {
      bool getStorage = storage == NULL;
      if(getStorage)
      {
         const StringObjectMap<NXvirStoragePool> *storageMap = getStorageListAndLock();
         storage = storageMap->get(name);
      }
      if(storage != NULL)
      {
         virStoragePoolInfoPtr newInfo = (virStoragePoolInfoPtr)malloc(sizeof(virStoragePoolInfo));
         if(virStoragePoolGetInfo(*storage, newInfo) == 0)
         {
            if(info == NULL)
            {
               info = new Cashe<virStoragePoolInfo>();
               m_storageInfo.set(name, info);
            }
            info->update(newInfo);
         }
         else
         {
            free(newInfo);
         }

      }
      else
      {
         m_storageInfo.remove(name);
         info = NULL;
      }

      if(getStorage)
         unlockStorageList();
   }
   return info != NULL ? info->getData() : NULL;
}

/**
 * Unlocks network definition
 */
void HostConnections::unlockStorageInfo()
{
   MutexUnlock(m_storageInfoMutex);
}

/**---------
 * Volume
 */

/*
   TODO: add option to get volumes for storage

Valume:
   virStoragePoolListVolumes - volume information
   virStoragePoolNumOfVolumes - num of volumes
   virStorageVolGetInfo
   virStorageVolGetPath
   virStorageVolGetXMLDesc
   virStorageVolLookupByName
*/
