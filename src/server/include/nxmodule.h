/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxmodule.h
**
**/

#ifndef _nxmodule_h_
#define _nxmodule_h_

/**
 * Module metadata
 */
struct NXMODULE_METADATA
{
   uint32_t size;   // structure size in bytes
   uint32_t unicode;  // unicode flag
   char tagBegin[16];
   char name[MAX_OBJECT_NAME];
   char vendor[128];
   char coreVersion[16];
   char coreBuildTag[32];
   char moduleVersion[16];
   char moduleBuildTag[32];
   char compiler[256];
   char tagEnd[16];
};

#ifndef MODULE_NXDBMGR_EXTENSION

#include <nxdbapi.h>
#include "server_console.h"

/**
 * Forward declaration of server classes
 */
class ClientSession;
class GenericClientSession;
class MobileDeviceSession;
class DataCollectionTarget;
class Node;
class Event;
class Alarm;
class NetObj;
class PollerInfo;
class NXSL_Environment;
class NXSL_VM;
class PredictionEngine;
struct NXCORE_LOG;

/**
 * Session ID
 */
typedef int session_id_t;

/**
 * Command handler return codes
 */
#define NXMOD_COMMAND_IGNORED          -1
#define NXMOD_COMMAND_PROCESSED        0
#define NXMOD_COMMAND_ACCEPTED_ASYNC   1

/**
 * Module-specific object data
 */
class NXCORE_EXPORTABLE ModuleData
{
public:
   ModuleData();
   virtual ~ModuleData();

   virtual void fillMessage(NXCPMessage *msg, uint32_t baseId) const;
   virtual json_t *toJson() const;

   virtual bool saveToDatabase(DB_HANDLE hdb, uint32_t objectId);
   virtual bool saveRuntimeData(DB_HANDLE hdb, uint32_t objectId);
   virtual bool deleteFromDatabase(DB_HANDLE hdb, uint32_t objectId);
};

struct DeviceBackupInterface;
struct ResponsibleUser;
struct WirelessControllerBridge;

/**
 * Module registration structure
 */
typedef struct
{
   uint32_t dwSize;           // Size of structure in bytes
   wchar_t name[MAX_OBJECT_NAME];
   bool (*pfInitialize)(Config *config);
   void (*pfServerStarted)();
	void (*pfShutdown)();
	void (*pfLoadObjects)();
	void (*pfLinkObjects)();
   int (*pfClientCommandHandler)(uint32_t command, NXCPMessage *request, ClientSession *session);
   int (*pfMobileDeviceCommandHandler)(uint32_t dwCommand, const NXCPMessage& request, MobileDeviceSession *pSession);
   bool (*pfTrapHandler)(SNMP_PDU *pdu, const shared_ptr<Node>& node);
   void (*pfEventHandler)(Event *event);
   void (*pfAlarmChangeHook)(uint32_t changeCode, const Alarm *alarm);
	void (*pfStatusPollHook)(Node *node, ClientSession *session, uint32_t rqId, PollerInfo *poller);
	bool (*pfConfPollHook)(Node *node, ClientSession *session, uint32_t rqId, PollerInfo *poller);
	void (*pfTopologyPollHook)(Node *node, ClientSession *session, uint32_t rqId, PollerInfo *poller);
	int (*pfCalculateObjectStatus)(NetObj *object);
   DataCollectionError (*pfGetInternalMetric)(DataCollectionTarget *object, const TCHAR *metric, TCHAR *value, size_t size);
   DataCollectionError (*pfGetInternalList)(DataCollectionTarget *object, const TCHAR *metric, StringList **value);
   DataCollectionError (*pfGetInternalTable)(DataCollectionTarget *object, const TCHAR *metric, shared_ptr<Table> *value);
   bool (*pfNetObjInsert)(const shared_ptr<NetObj>& object);
	bool (*pfNetObjDelete)(const NetObj& object);
	void (*pfPostObjectCreate)(const shared_ptr<NetObj>& object);
	void (*pfPostObjectLoad)(const shared_ptr<NetObj>& object);
	void (*pfPreObjectDelete)(NetObj *object);
	shared_ptr<NetObj> (*pfCreateObject)(int objectClass, const TCHAR *name, const shared_ptr<NetObj>& parent, const NXCPMessage& request);
	bool (*pfIsValidParentClass)(int childClass, int parentClass);
	bool (*pfGetObjectAttribute)(const NetObj& object, const TCHAR *name, TCHAR **value, bool *isAllocated);
	bool (*pfAcceptNewNode)(const InetAddress& addr, uint32_t zoneId, const MacAddress& macAddr);
	uint32_t (*pfValidateObjectCreation)(int objectClass, const TCHAR *name, const InetAddress& ipAddr, uint32_t zoneId, const NXCPMessage& request);
   uint32_t (*pfAdditionalLoginCheck)(uint32_t userId, const NXCPMessage& request);
   void (*pfOnNodeMgmtStatusChange)(const shared_ptr<Node>& node, bool isManaged);
   void (*pfOnResponsibleUserListChange)(NetObj *object, const StructArray<ResponsibleUser> *oldList, const StructArray<ResponsibleUser> *newList, ClientSession *session);
   void (*pfClientSessionClose)(GenericClientSession *session);
   void (*pfNXSLServerEnvConfig)(NXSL_Environment *env);
   void (*pfNXSLServerVMConfig)(NXSL_VM *vm);
   void (*pfOnConnectToAgent)(const shared_ptr<Node>& node, const shared_ptr<AgentConnection>& conn);
   bool (*pfOnAgentMessage)(NXCPMessage *msg, uint32_t nodeId);
   void (*pfHousekeeperHook)();
   bool (*pfProcessServerConsoleCommand)(const TCHAR *command, ServerConsole *console);
   void (*pfProcessAuditRecord)(const TCHAR *subsys, bool isSuccess, uint32_t userId, const TCHAR *workstation,
            session_id_t sessionId, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *text);
   WirelessControllerBridge *(*pfGetWLCBridgeInterface)(const wchar_t *bridgeName);
   ObjectArray<PredictionEngine> *(*pfGetPredictionEngines)();
   std::string (*pfGetAIAgentInstructions)();
   DeviceBackupInterface *deviceBackupInterface;
   NXCORE_LOG *logs;
   HMODULE hModule;
   NXMODULE_METADATA *metadata;
} NXMODULE;

#ifdef UNICODE
#define METADATA_UNICODE   1
#else
#define METADATA_UNICODE   0
#endif

/**
 * Define module metadata
 */
#define DEFINE_MODULE_METADATA(name,vendor,version,tag) \
extern "C" __EXPORT_VAR(NXMODULE_METADATA NXM_metadata); \
__EXPORT_VAR(NXMODULE_METADATA NXM_metadata) = \
{ sizeof(struct NXMODULE_METADATA), METADATA_UNICODE, \
   "$$$NXMINFO>$$$", \
   name, vendor, \
   NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A, \
   version, tag, \
   CPP_COMPILER_VERSION, \
   "$$$NXMINFO<$$$" \
};

/**
 * Enumerate all modules where given entry point available
 */
#define ENUMERATE_MODULES(e) if (!(g_flags & AF_SHUTDOWN)) \
   for(int __i = 0; __i < g_moduleList.size(); __i++) \
      if (g_moduleList.get(__i)-> e != nullptr)

/**
 * Reference to current module in ENUMERATE_MODULES
 */
#define CURRENT_MODULE (*g_moduleList.get(__i))

/**
 * Call module entry point for all loaded modules
 */
#define CALL_ALL_MODULES(e, p) if (!(g_flags & AF_SHUTDOWN)) { \
   for(int __i = 0; __i < g_moduleList.size(); __i++) { \
      if (g_moduleList.get(__i)-> e != nullptr) { g_moduleList.get(__i)-> e p; } \
   } \
}

/**
 * Functions
 */
bool LoadNetXMSModules();

/**
 * Global variables
 */
extern StructArray<NXMODULE> g_moduleList;

#endif   /* MODULE_NXDBMGR_EXTENSION */

#endif   /* _nxmodule_h_ */
