/* 
** NetXMS - Network Management System
** Client Library API
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: nxclapi.h
**
**/

#ifndef _nxclapi_h_
#define _nxclapi_h_

#include <nms_common.h>


//
// Some constants
//

#define MAX_OBJECT_NAME       64
#define MAX_COMMUNITY_LENGTH  32
#define MAX_OID_LENGTH        1024
#define MAX_EVENT_MSG_LENGTH  256


//
// Authentication methods
//

#define AUTH_NONE          0
#define AUTH_PLAINTEXT     1
#define AUTH_MD5_HASH      2
#define AUTH_SHA1_HASH     3


//
// Object types
//

#define OBJECT_GENERIC     0
#define OBJECT_SUBNET      1
#define OBJECT_NODE        2
#define OBJECT_INTERFACE   3
#define OBJECT_NETWORK     4


//
// Events
//

#define NXC_EVENT_STATE_CHANGED     1
#define NXC_EVENT_ERROR             2
#define NXC_EVENT_LOGIN_RESULT      3
#define NXC_EVENT_NEW_ELOG_RECORD   4


//
// Errors
//

#define NXC_ERR_INTERNAL            1


//
// States
//

#define STATE_DISCONNECTED    0
#define STATE_CONNECTING      1
#define STATE_IDLE            2
#define STATE_SYNC_OBJECTS    3
#define STATE_SYNC_EVENTS     4


//
// Operations
//

#define NXC_OP_SYNC_OBJECTS   1
#define NXC_OP_SYNC_EVENTS    2


//
// Callbacks data types
//

typedef void (* NXC_EVENT_HANDLER)(DWORD dwEvent, DWORD dwCode, void *pArg);
typedef void (* NXC_DEBUG_CALLBACK)(char *pMsg);


//
// Event log record structure
//

typedef struct
{
   DWORD dwTimeStamp;
   DWORD dwEventId;
   DWORD dwSourceId;
   DWORD dwSeverity;
   char  szMessage[MAX_EVENT_MSG_LENGTH];
} NXC_EVENT;


//
// Network object structure
//

typedef struct
{
   DWORD dwId;          // Unique object's identifier
   int iClass;          // Object's class
   int iStatus;         // Object's status
   DWORD dwIpAddr;      // IP address
   char szName[MAX_OBJECT_NAME];
   DWORD dwNumParents;
   DWORD *pdwParentList;
   DWORD dwNumChilds;
   DWORD *pdwChildList;
   union
   {
      struct
      {
         DWORD dwIpNetMask;   // Ip netmask.
         DWORD dwIfIndex;     // Interface index.
         DWORD dwIfType;      // Interface type
      } iface;
      struct
      {
         DWORD dwFlags;
         DWORD dwDiscoveryFlags;
         char szSharedSecret[MAX_SECRET_LENGTH];
         char szCommunityString[MAX_COMMUNITY_LENGTH];
         char szObjectId[MAX_OID_LENGTH];
         WORD wAgentPort;     // Listening TCP port for native agent
         WORD wAuthMethod;    // Native agent's authentication method
      } node;
      struct
      {
         DWORD dwIpNetMask;
      } subnet;
   };
} NXC_OBJECT;


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

DWORD EXPORTABLE NXCGetVersion(void);
BOOL EXPORTABLE NXCInitialize(void);
BOOL EXPORTABLE NXCConnect(char *szServer, char *szLogin, char *szPassword);
void EXPORTABLE NXCDisconnect(void);
void EXPORTABLE NXCSetEventHandler(NXC_EVENT_HANDLER pHandler);
void EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc);
int EXPORTABLE NXCRequest(DWORD dwOperation, ...);

NXC_OBJECT EXPORTABLE *NXCFindObjectById(DWORD dwId);
void EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *));
NXC_OBJECT EXPORTABLE *NXCGetRootObject(void);

#ifdef __cplusplus
}
#endif


//
// Macros
//

#define NXCSyncObjects() NXCRequest(NXC_OP_SYNC_OBJECTS)
#define NXCSyncEvents() NXCRequest(NXC_OP_SYNC_EVENTS)


#endif   /* _nxclapi_h_ */
