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

LONG H_GetVMTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_GetIfaceTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);


/**
 * String map template for holding objects as values
 */
template <class T> class StringObjectMapC : public StringObjectMap<T>
{
private:
	static void destructorC(void *object) { virDomainFree((virDomainPtr)object); }

public:
	StringObjectMapC(bool objectOwner) : StringObjectMap<T>(objectOwner) { StringMapBase::m_objectDestructor = destructorC; }
};


/**
 * Data cashing class
 */
template <class StoredData> class XmlCashe
{
private:
   time_t m_lastCollectionTime;
   StoredData *m_data;
   RWLOCK m_lock;
   bool isMallocAlloc;
public:
   XmlCashe() { m_lastCollectionTime = NEVER; m_data = NULL; m_lock = RWLockCreate(); isMallocAlloc = true;}
   ~XmlCashe() { RWLockDestroy(m_lock); if(isMallocAlloc){ free(m_data); } else { delete(m_data); } }
   //XmlCashe(StoredData *data) { m_lastCollectionTime = time(NULL); m_data = data; }
   void setMalloc(bool value) { isMallocAlloc = value; }
   const StoredData *getDataAndLock() { RWLockReadLock(m_lock, 10000); return m_data; }
   const time_t getLastCollecitonTime() { return m_lastCollectionTime; }
   void updateChaseAndUnlock(StoredData* data) { m_lastCollectionTime = time(NULL); if(isMallocAlloc){ free(m_data); } else { delete(m_data); } m_data = data; RWLockUnlock(m_lock); }
   bool shouldUpdate() { return (time(NULL) - m_lastCollectionTime) > DATA_COLLECTION_CASHE_TIME; }
   void setWriteLock() { RWLockWriteLock(m_lock, INFINITE); }
   void unlock() { RWLockUnlock(m_lock); }
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

   XmlCashe<char> m_capabilities;
   XmlCashe<virNodeInfo> m_nodeInfo;
   XmlCashe<StringObjectMap<virDomain> > m_domains;
   XmlCashe<StringList> m_iface;
   //XmlCashe m_storages;

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
   const StringObjectMap<virDomain> *getDomainListAndLock();
   void unlockDomainList();
   const StringList *getIfaceListAndLock();
   void unlockIfaceList();
   //void getStorages();
   //void getDomains();

   UINT32 getMaxVCpuCount();
   UINT64 getHostFreeMemory();
   const char *getConnectionType();
   UINT64 getConnectionVersion();
   UINT64 getLibraryVersion();

   const TCHAR *getName() { return m_name; }
};

/**
 * General declarations
 */
void InitLibvirt();
extern StringObjectMap<HostConnections> g_connectionList;
