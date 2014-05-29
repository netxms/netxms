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
class NetObj;
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

   virtual void fillMessage(CSCPMessage *msg, UINT32 baseId);
   virtual bool saveToDatabase(DB_HANDLE hdb, UINT32 objectId);
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
   int (* pfClientCommandHandler)(UINT32 dwCommand, CSCPMessage *pMsg, ClientSession *pSession);
   int (* pfMobileDeviceCommandHandler)(UINT32 dwCommand, CSCPMessage *pMsg, MobileDeviceSession *pSession);
   BOOL (* pfTrapHandler)(SNMP_PDU *pdu, Node *pNode);
   BOOL (* pfEventHandler)(Event *event);
   void (* pfAlarmChangeHook)(UINT32 changeCode, NXC_ALARM *alarm);
	void (* pfStatusPollHook)(Node *node, ClientSession *session, UINT32 rqId, int pollerId);
	void (* pfConfPollHook)(Node *node, ClientSession *session, UINT32 rqId, int pollerId);
	void (* pfTopologyPollHook)(Node *node, ClientSession *session, UINT32 rqId, int pollerId);
	int (* pfCalculateObjectStatus)(NetObj *object);
	BOOL (* pfNetObjInsert)(NetObj *object);
	BOOL (* pfNetObjDelete)(NetObj *object);
	void (* pfPostObjectCreate)(NetObj *object);
	void (* pfPostObjectLoad)(NetObj *object);
	void (* pfPreObjectDelete)(NetObj *object);
	NetObj *(* pfCreateObject)(int objectClass, const TCHAR *name, NetObj *parent, CSCPMessage *msg);
	BOOL (* pfIsValidParentClass)(int childClass, int parentClass);
	BOOL (* pfAcceptNewNode)(UINT32 ipAddr, UINT32 ipNetMask, UINT32 zoneId, BYTE *macAddr);
	UINT32 (* pfValidateObjectCreation)(int objectClass, const TCHAR *name, UINT32 ipAddr, UINT32 zoneId, CSCPMessage *request);
   UINT32 (* pfAdditionalLoginCheck)(UINT32 userId, CSCPMessage *request);
   void (* pfClientSessionClose)(ClientSession *session);
   NXCORE_LOG *logs;
   HMODULE hModule;
} NXMODULE;

/**
 * Enumerate all modules where given entry point available
 */
#define ENUMERATE_MODULES(e) if (!(g_dwFlags & AF_SHUTDOWN)) \
   for(UINT32 __i = 0; __i < g_dwNumModules; __i++) \
      if (g_pModuleList[__i]. e != NULL)

/**
 * Call module entry point for all loaded modules
 */
#define CALL_ALL_MODULES(e, p) if (!(g_dwFlags & AF_SHUTDOWN)) { \
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
