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

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <nms_common.h>
#include <nms_agent.h>
#include <nxcpapi.h>

#define NEVER 0
#define DATA_COLLECTION_CASHE_TIME 10

/**
 * Parameter, list, table providers
 */
LONG H_GetHostList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_GetVMList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);

LONG H_GetFromCapabilities(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_GetUIntParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_GetUInt64Param(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_GetStringParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_GetVMInfoAsParam(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);

LONG H_GetVMTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetIfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetVMDiskTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetVMControllerTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetVMInterfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetVMVideoTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetNetworksTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_GetStoragesTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);

class NXvirDomain
{
private:
   virDomainPtr vm;
public:
   NXvirDomain(virDomainPtr vm) { this->vm = vm; }
   ~NXvirDomain() { virDomainFree(vm); }
   operator virDomainPtr() const { return vm; }
};

class NXvirNetwork
{
private:
   virNetworkPtr network;
public:
   NXvirNetwork(virNetworkPtr network) { this->network = network; }
   ~NXvirNetwork() { virNetworkFree(network); }
   operator virNetworkPtr() const { return network; }
};

class NXvirStoragePool
{
private:
   virStoragePoolPtr storage;
public:
   NXvirStoragePool(virStoragePoolPtr storage) { this->storage = storage; }
   ~NXvirStoragePool() {virStoragePoolFree(storage); }
   operator virStoragePoolPtr() const { return storage; }
};

/**
 * Data cashing class
 */
template <class T> class Cashe
{
private:
   time_t m_lastCollectionTime;
   T *m_data;
   bool mallocObj;

public:
   Cashe(bool mallocObj = true) { m_lastCollectionTime = NEVER; m_data = NULL; this->mallocObj = mallocObj;}
   ~Cashe() { if(mallocObj){ free(m_data); } else { delete(m_data); } }
   const T *getData() { return m_data; }
   const time_t getLastCollecitonTime() { return m_lastCollectionTime; }
   void update(T* data) { m_lastCollectionTime = time(NULL); if(mallocObj){ free(m_data); } else { delete(m_data); } m_data = data; }
   bool shouldUpdate() { return (time(NULL) - m_lastCollectionTime) > DATA_COLLECTION_CASHE_TIME; }
};

/**
 * Data cashing class
 */
template <class T> class CasheAndLock : public Cashe<T>
{
private:
   MUTEX m_mutex;
public:
   CasheAndLock(bool mallocObj = true) : Cashe<T>(mallocObj) { m_mutex = MutexCreate(); }
   ~CasheAndLock() { MutexDestroy(m_mutex); }
   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }
};

/**
 * Class that
 */
class HostConnections
{
private:
   virConnectPtr m_connection;
   char *m_url;
   TCHAR *m_name;
   char *m_login;
   char *m_password;

   CasheAndLock<char> m_capabilities;
   CasheAndLock<virNodeInfo> m_nodeInfo;
   CasheAndLock<StringObjectMap<NXvirDomain> > m_domains;
   CasheAndLock<StringList> m_iface;
   CasheAndLock<StringObjectMap<NXvirNetwork> > m_networks;
   CasheAndLock<StringObjectMap<NXvirStoragePool> > m_storages;

   MUTEX m_vmInfoMutex;
   StringObjectMap<Cashe<virDomainInfo> > m_vmInfo;
   MUTEX m_vmXMLMutex;
   StringObjectMap<Cashe<char> > m_vmXMLs;
   MUTEX m_networkXMLMutex;
   StringObjectMap<Cashe<char> > m_networkXMLs;
   MUTEX m_storageInfoMutex;
   StringObjectMap<Cashe<virStoragePoolInfo> > m_storageInfo;

   static int authCb(virConnectCredentialPtr cred, unsigned int ncred, void *cbdata);

public:
   HostConnections(const TCHAR *name, const char *url, const char *login, const char *password);
   ~HostConnections();
   bool connect();
   void disconnect();

   const char *getCapabilitiesAndLock();
   void unlockCapabilities();
   const virNodeInfo *getNodeInfoAndLock();
   void unlockNodeInfo();
   //Domains
   const StringObjectMap<NXvirDomain> *getDomainListAndLock();
   void unlockDomainList();
   const char *getDomainDefenitionAndLock(const TCHAR *name, NXvirDomain *vm = NULL);
   void unlockDomainDefenition();
   const virDomainInfo *getDomainInfoAndLock(const TCHAR *domainName, NXvirDomain *vm = NULL);
   void unlockDomainInfo();
   //Iface
   const StringList *getIfaceListAndLock();
   void unlockIfaceList();
   //Networks
   const StringObjectMap<NXvirNetwork> *getNetworkListAndLock();
   void unlockNetworkList();
   const char *getNetworkDefenitionAndLock(const TCHAR *name, NXvirNetwork *network = NULL);
   void unlockNetworkDefenition();
   //Storage
   const StringObjectMap<NXvirStoragePool> *getStorageListAndLock();
   void unlockStorageList();
   const virStoragePoolInfo *getStorageInformationAndLock(const TCHAR *name, NXvirStoragePool *storage = NULL);
   void unlockStorageInfo();


   UINT32 getMaxVCpuCount();
   UINT64 getHostFreeMemory();
   const char *getConnectionType();
   UINT64 getConnectionVersion();
   UINT64 getLibraryVersion();

   const TCHAR *getName() { return m_name; }
   /***
   Functions that can be added:
   virDomainScreenshot - get screenshot form VM

   Under question:
   virDomainShutdown
   virDomainSuspend


   Check whay not working:
   virInterfaceLookupByName, when fixed use virInterfaceIsActive, virInterfaceGetXMLDesc to find more information


   Add callbacks that will generate nxevents on call(there are network and domain callbacks at least)
   ****/
};

/**
 * General declarations
 */
void InitLibvirt();
extern StringObjectMap<HostConnections> g_connectionList;
