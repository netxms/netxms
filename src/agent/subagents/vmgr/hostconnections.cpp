/*
 ** File management subagent
 ** Copyright (C) 2016-2020 Raden Solutions
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
HostConnections::HostConnections(const TCHAR *name, const char *url, const char *login, const char *password) :
      m_domains(false), m_iface(false), m_networks(false), m_storages(false), m_vmInfo(Ownership::True),
      m_vmXMLs(Ownership::True), m_networkXMLs(Ownership::True), m_storageInfo(Ownership::True)

{
   m_connection = nullptr;
   m_name = MemCopyString(name);
   m_url = MemCopyStringA(url);
   m_login = MemCopyStringA(login);
   m_password = MemCopyStringA(password);
}

/**
 * Host connection initial constructor
 */
HostConnections::~HostConnections()
{
   disconnect();
   MemFree(m_url);
   MemFree(m_name);
   MemFree(m_login);
   MemFree(m_password);
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
static int authCreds[] =
{
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
   return m_connection != nullptr;
}

/**
 * Function that close connection to host and cleans all libvirt specific data
 */
void HostConnections::disconnect()
{
   if (m_connection != nullptr)
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
      if (caps == nullptr)
      {
         virError err;
         virCopyLastError(&err);
         nxlog_debug_tag(VMGR_DEBUG_TAG, 5, _T("HostConnections::getCapabilitiesAndLock(): virConnectGetCapabilities failed (%hs)"), err.message);
         virResetError(&err);
         return nullptr;
      }
      m_capabilities.update(caps);
      nxlog_debug_tag(VMGR_DEBUG_TAG, 5, _T("Connection %hs capabilities: %hs"), m_url, caps);
   }
   return m_capabilities.getData();
}

/**
 * Unlock capabilities
 */
void HostConnections::unlockCapabilities()
{
   m_capabilities.unlock();
}

/**
 * Retrieves maximum number of virtual CPUs per-guest the underlying virtualization technology supports, or -1 in case of error
 */
UINT32 HostConnections::getMaxVCpuCount()
{
   return virConnectGetMaxVcpus(m_connection, nullptr);
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
   if (m_nodeInfo.shouldUpdate())
   {
      virNodeInfo *nodeInfo = MemAllocStruct<virNodeInfo>();
      if (virNodeGetInfo(m_connection, nodeInfo) == -1)
      {
         virError err;
         virCopyLastError(&err);
         nxlog_debug_tag(VMGR_DEBUG_TAG, 6, _T("HostConnections::getNodeInfoAndLock(): virNodeGetInfo failed (%hs)"), err.message);
         virResetError(&err);
         MemFree(nodeInfo);
         return nullptr;
      }
      m_nodeInfo.update(nodeInfo);
   }
   return m_nodeInfo.getData();
}

/**
 * Unlock node information
 */
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
   if (virConnectGetLibVersion(m_connection, &ver) == 0)
      return (UINT64)ver;
   else
      return -1;
}

/**---------
 * Domains
 *----------*/

/**
 * List running VMs(domains)
 */
const StringObjectMap<NXvirDomain> *HostConnections::getDomainListAndLock()
{
   m_domains.lock();
   if (m_domains.shouldUpdate())
   {
      int numActiveDomains = virConnectNumOfDomains(m_connection);
      int numInactiveDomains = virConnectNumOfDefinedDomains(m_connection);

      char **inactiveDomains = MemAllocArray<char*>(numInactiveDomains);
      int *activeDomains = MemAllocArray<int>(numActiveDomains);

      numActiveDomains = virConnectListDomains(m_connection, activeDomains, numActiveDomains);
      numInactiveDomains = virConnectListDefinedDomains(m_connection, inactiveDomains, numInactiveDomains);

      StringObjectMap<NXvirDomain> *allDomains = new StringObjectMap<NXvirDomain>(Ownership::True);

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
const char *HostConnections::getDomainDefinitionAndLock(const TCHAR *name, const NXvirDomain *vm)
{
   m_vmXMLMutex.lock();
   Cache<char> *xmlCache = m_vmXMLs.get(name);
   if (xmlCache == nullptr || xmlCache->shouldUpdate())
   {
      bool getDomain = vm == nullptr;
      if (getDomain)
      {
         const StringObjectMap<NXvirDomain> *vmMap = getDomainListAndLock();
         vm = vmMap->get(name);
      }
      if (vm != nullptr)
      {
         char *xml = virDomainGetXMLDesc(*vm, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
         if (xmlCache == nullptr)
         {
            xmlCache = new Cache<char>();
            m_vmXMLs.set(name, xmlCache);
         }
         xmlCache->update(xml);

      }
      else
      {
         m_vmXMLs.remove(name);
         xmlCache = nullptr;
      }

      if (getDomain)
         unlockDomainList();
   }
   return (xmlCache != nullptr) ? xmlCache->getData() : nullptr;
}

/**
 * Unlocks domain definition
 */
void HostConnections::unlockDomainDefinition()
{
   m_vmXMLMutex.unlock();
}

/**
 * Returns domain information
 */
const virDomainInfo *HostConnections::getDomainInfoAndLock(const TCHAR *domainName, const NXvirDomain *vm)
{
   m_vmInfoMutex.lock();
   Cache<virDomainInfo> *infoCache = m_vmInfo.get(domainName);
   if (infoCache == nullptr || infoCache->shouldUpdate())
   {
      bool getDomain = vm == nullptr;
      if (getDomain)
      {
         const StringObjectMap<NXvirDomain> *vmMap = getDomainListAndLock();
         vm = vmMap->get(domainName);
      }
      if (vm != nullptr)
      {
         virDomainInfo *info = new virDomainInfo();
         if (virDomainGetInfo(*vm, info) == 0)
         {
            if (infoCache == nullptr)
            {
               infoCache = new Cache<virDomainInfo>();
               m_vmInfo.set(domainName, infoCache);
            }
            infoCache->update(info);
         }
         else
         {
            delete info;
         }
      }
      else
      {
         m_vmInfo.remove(domainName);
         infoCache = nullptr;
      }

      if(getDomain)
         unlockDomainList();
   }

   return infoCache != nullptr ? infoCache->getData() : nullptr;
}

/**
 * Unlocks domain control information
 */
void HostConnections::unlockDomainInfo()
{
   m_vmInfoMutex.unlock();
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

      if (activeCount != -1)
      {
         char **active = MemAllocArray<char*>(activeCount);
         virConnectListInterfaces(m_connection, active, activeCount);
         for(int i=0; i < activeCount; i++)
         {
            virInterfacePtr iface = virInterfaceLookupByName(m_connection, active[i]);
            virInterfaceFree(iface);
            ifaceList->addMBString(active[i]);
            MemFree(active[i]);
         }
         MemFree(active);
      }

      if (inactiveCount != -1)
      {
         char **inactive = MemAllocArray<char*>(inactiveCount);
         virConnectListDefinedInterfaces(m_connection, inactive, inactiveCount);
         for(int i=0; i < inactiveCount; i++)
         {
            virInterfacePtr iface = virInterfaceLookupByName(m_connection, inactive[i]);
            virInterfaceFree(iface);
            ifaceList->addMBString(inactive[i]);
            MemFree(inactive[i]);
         }
         MemFree(inactive);
      }
      m_iface.update(ifaceList);
   }
   return m_iface.getData();
}

/**
 * Unlock interface list
 */
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
   if (m_networks.shouldUpdate())
   {
      int numActiveNetworks = virConnectNumOfNetworks(m_connection);
      int numInactiveNetworks = virConnectNumOfDefinedNetworks(m_connection);

      char **inactiveNetworks = MemAllocArray<char*>(numInactiveNetworks);
      char **activeNetworks = MemAllocArray<char*>(numActiveNetworks);

      numActiveNetworks = virConnectListNetworks(m_connection, activeNetworks, numActiveNetworks);
      numInactiveNetworks = virConnectListDefinedNetworks(m_connection, inactiveNetworks, numInactiveNetworks);

      StringObjectMap<NXvirNetwork> *allNetworks = new StringObjectMap<NXvirNetwork>(Ownership::True);

      for (int i = 0 ; i < numActiveNetworks ; i++)
      {
#ifdef UNICODE
         allNetworks->setPreallocated(WideStringFromMBString(activeNetworks[i]), new NXvirNetwork(virNetworkLookupByName(m_connection, activeNetworks[i])));
         MemFree(activeNetworks[i]);
#else
         allNetworks->setPreallocated(activeNetworks[i], new NXvirNetwork(virNetworkLookupByName(m_connection, activeNetworks[i])));
#endif
      }

      for (int i = 0 ; i < numInactiveNetworks ; i++)
      {
 #ifdef UNICODE
         allNetworks->setPreallocated(WideStringFromMBString(inactiveNetworks[i]), new NXvirNetwork(virNetworkLookupByName(m_connection, inactiveNetworks[i])));
         MemFree(inactiveNetworks[i]);
#else
         allNetworks->setPreallocated(inactiveNetworks[i], new NXvirNetwork(virNetworkLookupByName(m_connection, inactiveNetworks[i])));
#endif
      }

      MemFree(activeNetworks);
      MemFree(inactiveNetworks);

      m_networks.update(allNetworks);
   }

   nxlog_debug_tag(VMGR_DEBUG_TAG, 9, _T("HostConnections::getNetworkListAndLock(): network list size: %d"), m_networks.getData()->size());
   return m_networks.getData();
}

/**
 * Unlock network list
 */
void HostConnections::unlockNetworkList()
{
   m_networks.unlock();
}

/**
 * Returns network definition in XML format
 */
const char *HostConnections::getNetworkDefinitionAndLock(const TCHAR *name, const NXvirNetwork *network)
{
   m_networkXMLMutex.lock();
   Cache<char> *xmlCache = m_networkXMLs.get(name);
   if (xmlCache == nullptr || xmlCache->shouldUpdate())
   {
      bool getNetwork = network == nullptr;
      if(getNetwork)
      {
         const StringObjectMap<NXvirNetwork> *networkMap = getNetworkListAndLock();
         network = networkMap->get(name);
      }
      if(network != nullptr)
      {
         char *xml = virNetworkGetXMLDesc(*network, 0);
         if(xmlCache == nullptr)
         {
            xmlCache = new Cache<char>();
            m_networkXMLs.set(name, xmlCache);
         }
         xmlCache->update(xml);
      }
      else
      {
         m_networkXMLs.remove(name);
         xmlCache = nullptr;
      }

      if (getNetwork)
         unlockNetworkList();
   }

   return (xmlCache != nullptr) ? xmlCache->getData() : nullptr;
}

/**
 * Unlocks network definition
 */
void HostConnections::unlockNetworkDefinition()
{
   m_networkXMLMutex.unlock();
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
   if (m_storages.shouldUpdate())
   {
      int numActive = virConnectNumOfStoragePools(m_connection);
      int numInactive = virConnectNumOfDefinedStoragePools(m_connection);

      char **inactive = MemAllocArray<char*>(numInactive);
      char **active = MemAllocArray<char*>(numActive);

      numActive = virConnectListStoragePools(m_connection, active, numActive);
      numInactive = virConnectListDefinedStoragePools(m_connection, inactive, numInactive);

      StringObjectMap<NXvirStoragePool> *allStorage = new StringObjectMap<NXvirStoragePool>(Ownership::True);

      for (int i = 0 ; i < numActive; i++)
      {
#ifdef UNICODE
         allStorage->setPreallocated(WideStringFromMBString(active[i]), new NXvirStoragePool(virStoragePoolLookupByName(m_connection, active[i])));
         MemFree(active[i]);
#else
         allStorage->set(active[i], new NXvirStoragePool(virStoragePoolLookupByName(m_connection, active[i])));
#endif
      }

      for (int i = 0 ; i < numInactive; i++)
      {
 #ifdef UNICODE
         allStorage->setPreallocated(WideStringFromMBString(inactive[i]), new NXvirStoragePool(virStoragePoolLookupByName(m_connection, inactive[i])));
         MemFree(inactive[i]);
#else
         allStorage->setPreallocated(inactive[i], new NXvirStoragePool(virStoragePoolLookupByName(m_connection, inactive[i])));
#endif
      }

      MemFree(active);
      MemFree(inactive);

      m_storages.update(allStorage);
   }
   nxlog_debug_tag(VMGR_DEBUG_TAG, 9, _T("HostConnections::getStorageListAndLock(): storage list size: %d"), m_storages.getData()->size());
   return m_storages.getData();
}

void HostConnections::unlockStorageList()
{
   m_storages.unlock();
}

/**
 * Returns storage definition in XML format
 */
const virStoragePoolInfo *HostConnections::getStorageInformationAndLock(const TCHAR *name, const NXvirStoragePool *storage)
{
   m_storageInfoMutex.lock();
   Cache<virStoragePoolInfo> *info = m_storageInfo.get(name);
   if (info == nullptr || info->shouldUpdate())
   {
      bool getStorage = storage == nullptr;
      if(getStorage)
      {
         const StringObjectMap<NXvirStoragePool> *storageMap = getStorageListAndLock();
         storage = storageMap->get(name);
      }
      if(storage != nullptr)
      {
         virStoragePoolInfoPtr newInfo = MemAllocStruct<virStoragePoolInfo>();
         if (virStoragePoolGetInfo(*storage, newInfo) == 0)
         {
            if (info == nullptr)
            {
               info = new Cache<virStoragePoolInfo>();
               m_storageInfo.set(name, info);
            }
            info->update(newInfo);
         }
         else
         {
            MemFree(newInfo);
         }

      }
      else
      {
         m_storageInfo.remove(name);
         info = nullptr;
      }

      if (getStorage)
         unlockStorageList();
   }
   return info != nullptr ? info->getData() : nullptr;
}

/**
 * Unlocks network definition
 */
void HostConnections::unlockStorageInfo()
{
   m_storageInfoMutex.unlock();
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
