/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
 * Forward declaration of server classes
 */
class ClientSession;
class MobileDeviceSession;
class Node;
class Event;

/**
 * Command handler return codes
 */
#define NXMOD_COMMAND_IGNORED          -1
#define NXMOD_COMMAND_PROCESSED        0
#define NXMOD_COMMAND_ACCEPTED_ASYNC   1

/**
 * Module registration structure
 */
typedef struct
{
   DWORD dwSize;           // Size of structure in bytes
   TCHAR szName[MAX_OBJECT_NAME];
   void (* pfMain)();  // Pointer to module's main()
   int (* pfClientCommandHandler)(DWORD dwCommand, CSCPMessage *pMsg, ClientSession *pSession);
   int (* pfMobileDeviceCommandHandler)(DWORD dwCommand, CSCPMessage *pMsg, MobileDeviceSession *pSession);
   BOOL (* pfTrapHandler)(SNMP_PDU *pdu, Node *pNode);
   BOOL (* pfEventHandler)(Event *event);
	void (* pfStatusPollHook)(Node *node, ClientSession *session, DWORD rqId, int pollerId);
	void (* pfConfPollHook)(Node *node, ClientSession *session, DWORD rqId, int pollerId);
	void (* pfTopologyPollHook)(Node *node, ClientSession *session, DWORD rqId, int pollerId);
   HMODULE hModule;
} NXMODULE;

/**
 * Functions
 */
void LoadNetXMSModules();

/**
 * Global variables
 */
extern DWORD g_dwNumModules;
extern NXMODULE *g_pModuleList;

#endif   /* _nxmodule_h_ */
