/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

#include <nxdbapi.h>

/**
 * Forward declaration of server classes
 */
class ClientSession;
class MobileDeviceSession;
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

   virtual void fillMessage(NXCPMessage *msg, UINT32 baseId);
   virtual bool saveToDatabase(DB_HANDLE hdb, UINT32 objectId);
   virtual bool saveRuntimeData(DB_HANDLE hdb, UINT32 objectId);
   virtual bool deleteFromDatabase(DB_HANDLE hdb, UINT32 objectId);
};

/**
 * Module registration structure
 */
typedef struct
{
   UINT32 dwSize;           // Size of structure in bytes
   TCHAR szName[MAX_OBJECT_NAME];
   void (* pfServerStarted)();
	void (* pfShutdown)();
	void (* pfLoadObjects)();
	void (* pfLinkObjects)();
   int (* pfClientCommandHandler)(UINT32 dwCommand, NXCPMessage *pMsg, ClientSession *pSession);
   int (* pfMobileDeviceCommandHandler)(UINT32 dwCommand, NXCPMessage *pMsg, MobileDeviceSession *pSession);
   BOOL (* pfTrapHandler)(SNMP_PDU *pdu, Node *pNode);
   BOOL (* pfEventHandler)(Event *event);
   void (* pfAlarmChangeHook)(UINT32 changeCode, const Alarm *alarm);
	void (* pfStatusPollHook)(Node *node, ClientSession *session, UINT32 rqId, PollerInfo *poller);
	bool (* pfConfPollHook)(Node *node, ClientSession *session, UINT32 rqId, PollerInfo *poller);
	void (* pfTopologyPollHook)(Node *node, ClientSession *session, UINT32 rqId, PollerInfo *poller);
	int (* pfCalculateObjectStatus)(NetObj *object);
	BOOL (* pfNetObjInsert)(NetObj *object);
	BOOL (* pfNetObjDelete)(NetObj *object);
	void (* pfPostObjectCreate)(NetObj *object);
	void (* pfPostObjectLoad)(NetObj *object);
	void (* pfPreObjectDelete)(NetObj *object);
	NetObj *(* pfCreateObject)(int objectClass, const TCHAR *name, NetObj *parent, NXCPMessage *msg);
	BOOL (* pfIsValidParentClass)(int childClass, int parentClass);
	BOOL (* pfAcceptNewNode)(const InetAddress& addr, UINT32 zoneId, BYTE *macAddr);
	UINT32 (* pfValidateObjectCreation)(int objectClass, const TCHAR *name, const InetAddress& ipAddr, UINT32 zoneId, NXCPMessage *request);
   UINT32 (* pfAdditionalLoginCheck)(UINT32 userId, NXCPMessage *request);
   void (* pfClientSessionClose)(ClientSession *session);
   void (* pfNXSLServerEnvConfig)(NXSL_Environment *env);
   void (* pfNXSLServerVMConfig)(NXSL_VM *vm);
   void (* pfOnConnectToAgent)(Node *node, AgentConnection *conn);
   BOOL (* pfOnAgentMessage)(NXCPMessage *msg, UINT32 nodeId);
   void (* pfHousekeeperHook)();
   ObjectArray<PredictionEngine> *(* pfGetPredictionEngines)();
   NXCORE_LOG *logs;
   HMODULE hModule;
} NXMODULE;

/**
 * Module metadata
 */
struct NXMODULE_METADATA
{
   UINT32 size;   // structure size in bytes
   UINT32 unicode;  // unicode flag
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

#ifdef _WIN32
#define METADATA_EXPORT __declspec(dllexport) 
#else
#define METADATA_EXPORT
#endif

#ifdef UNICODE
#define METADATA_UNICODE   1
#else
#define METADATA_UNICODE   0
#endif

/**
 * Define module metadata
 */
#define DEFINE_MODULE_METADATA(name,vendor,version,tag) \
extern "C" NXMODULE_METADATA METADATA_EXPORT NXM_metadata; \
NXMODULE_METADATA METADATA_EXPORT NXM_metadata = \
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
   for(UINT32 __i = 0; __i < g_dwNumModules; __i++) \
      if (g_pModuleList[__i]. e != NULL)

/**
 * Call module entry point for all loaded modules
 */
#define CALL_ALL_MODULES(e, p) if (!(g_flags & AF_SHUTDOWN)) { \
   for(UINT32 __i = 0; __i < g_dwNumModules; __i++) { \
      if (g_pModuleList[__i]. e != NULL) { g_pModuleList[__i]. e p; } \
   } \
}

/**
 * Functions
 */
bool LoadNetXMSModules();

/**
 * Global variables
 */
extern UINT32 g_dwNumModules;
extern NXMODULE *g_pModuleList;

#endif   /* _nxmodule_h_ */
