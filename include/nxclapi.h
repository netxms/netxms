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
#include <nxevent.h>

#ifdef _WIN32
#ifdef LIBNXCL_EXPORTS
#define LIBNXCL_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCL_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCL_EXPORTABLE
#endif


//
// Custom data types
//

typedef unsigned long HREQUEST;


//
// Some constants
//

#define MAX_OBJECT_NAME          64
#define MAX_COMMUNITY_LENGTH     32
#define MAX_OID_LENGTH           1024
#define MAX_EVENT_MSG_LENGTH     256
#define MAX_EVENT_NAME           64
#define INVALID_REQUEST_HANDLE   ((HREQUEST)0xFFFFFFFF)
#define MAX_USER_NAME            64
#define GROUP_FLAG               0x01000000


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
#define OBJECT_LOCATION    5
#define OBJECT_ZONE        6


//
// Node flags
//

#define NF_IS_SNMP         0x0001
#define NF_IS_NATIVE_AGENT 0x0002
#define NF_IS_BRIDGE       0x0004
#define NF_IS_ROUTER       0x0008
#define NF_IS_LOCAL_MGMT   0x0010


//
// Events
//

#define NXC_EVENT_STATE_CHANGED        1
#define NXC_EVENT_ERROR                2
#define NXC_EVENT_LOGIN_RESULT         3
#define NXC_EVENT_NEW_ELOG_RECORD      4
#define NXC_EVENT_REQUEST_COMPLETED    5
#define NXC_EVENT_OBJECT_CHANGED       6


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
#define STATE_LOAD_EVENT_DB   5
#define STATE_LOAD_EPP        6
#define STATE_LOAD_USER_DB    7


//
// Operations
//

#define NXC_OP_SYNC_OBJECTS      1
#define NXC_OP_SYNC_EVENTS       2
#define NXC_OP_OPEN_EVENT_DB     3
#define NXC_OP_CLOSE_EVENT_DB    4
#define NXC_OP_SET_EVENT_INFO    5
#define NXC_OP_MODIFY_OBJECT     6
#define NXC_OP_LOAD_USER_DB      7


//
// Notification codes
//

#define NX_NOTIFY_SHUTDOWN       1


//
// Request completion codes
//

#define RCC_SUCCESS                 ((DWORD)0)
#define RCC_COMPONENT_LOCKED        ((DWORD)1)
#define RCC_ACCESS_DENIED           ((DWORD)2)
#define RCC_INVALID_REQUEST         ((DWORD)3)
#define RCC_TIMEOUT                 ((DWORD)4)
#define RCC_OUT_OF_STATE_REQUEST    ((DWORD)5)
#define RCC_DB_FAILURE              ((DWORD)6)
#define RCC_INVALID_OBJECT_ID       ((DWORD)7)


//
// Mask bits for NXCModifyEventTemplate()
//

#define EM_SEVERITY        ((DWORD)0x01)
#define EM_FLAGS           ((DWORD)0x02)
#define EM_NAME            ((DWORD)0x04)
#define EM_MESSAGE         ((DWORD)0x08)
#define EM_DESCRIPTION     ((DWORD)0x10)
#define EM_ALL             ((DWORD)0x1F)


//
// Mask bits (flags) for NXCModifyObject()
//

#define OBJ_UPDATE_NAME             ((DWORD)0x01)
#define OBJ_UPDATE_AGENT_PORT       ((DWORD)0x02)
#define OBJ_UPDATE_AGENT_AUTH       ((DWORD)0x04)
#define OBJ_UPDATE_AGENT_SECRET     ((DWORD)0x08)
#define OBJ_UPDATE_SNMP_VERSION     ((DWORD)0x10)
#define OBJ_UPDATE_SNMP_COMMUNITY   ((DWORD)0x20)
#define OBJ_UPDATE_ACL              ((DWORD)0x40)
#define OBJ_UPDATE_ALL              ((DWORD)0x7F)


//
// Global user rights
//

#define SYSTEM_ACCESS_MANAGE_USERS        0x0001
#define SYSTEM_ACCESS_VIEW_CONFIG         0x0002
#define SYSTEM_ACCESS_EDIT_CONFIG         0x0004
#define SYSTEM_ACCESS_DROP_CONNECTIONS    0x0008
#define SYSTEM_ACCESS_VIEW_EVENT_DB       0x0010
#define SYSTEM_ACCESS_EDIT_EVENT_DB       0x0020


//
// Object access rights
//

#define OBJECT_ACCESS_READ          0x00000001
#define OBJECT_ACCESS_MODIFY        0x00000002
#define OBJECT_ACCESS_CREATE        0x00000004
#define OBJECT_ACCESS_DELETE        0x00000008
#define OBJECT_ACCESS_MOVE          0x00000010


//
// User/group flags
//

#define UF_MODIFIED                 0x0001
#define UF_DELETED                  0x0002
#define UF_DISABLED                 0x0004
#define UF_CHANGE_PASSWORD          0x0008


//
// Custom data types
//

typedef unsigned long HREQUEST;


//
// Callbacks data types
//

typedef void (* NXC_EVENT_HANDLER)(DWORD dwEvent, DWORD dwCode, void *pArg);
typedef void (* NXC_DEBUG_CALLBACK)(char *pMsg);


//
// Event configuration structure
//

typedef struct
{
   DWORD dwCode;
   DWORD dwSeverity;
   DWORD dwFlags;
   char szName[MAX_EVENT_NAME];
   char *pszMessage;
   char *pszDescription;
} NXC_EVENT_TEMPLATE;


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
// Entry in object's ACL
//

typedef struct
{
   DWORD dwUserId;
   DWORD dwAccessRights;
} NXC_ACL_ENTRY;


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
   BOOL bIsDeleted;     // TRUE for deleted objects
   BOOL bInheritRights;
   DWORD dwAclSize;
   NXC_ACL_ENTRY *pAccessList;
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
// Structure with data for NXCModifyObject
//

typedef struct
{
   DWORD dwObjectId;
   DWORD dwFlags;
   char *pszName;
   int iAgentPort;
   int iAuthType;
   char *pszSecret;
   int iSnmpVersion;
   char *pszCommunity;
   BOOL bInheritRights;
   DWORD dwAclSize;
   NXC_ACL_ENTRY *pAccessList;
} NXC_OBJECT_UPDATE;


//
// NetXMS user information structure
//

typedef struct
{
   char szName[MAX_USER_NAME];
   DWORD dwId;
   WORD wFlags;
   WORD wSystemRights;
   DWORD dwNumMembers;     // Only for groups
   DWORD *pdwMemberList;   // Only for groups
} NXC_USER;


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

DWORD LIBNXCL_EXPORTABLE NXCGetVersion(void);
const char LIBNXCL_EXPORTABLE *NXCGetErrorText(DWORD dwError);

BOOL LIBNXCL_EXPORTABLE NXCInitialize(void);
BOOL LIBNXCL_EXPORTABLE NXCConnect(char *szServer, char *szLogin, char *szPassword);
void LIBNXCL_EXPORTABLE NXCDisconnect(void);
void LIBNXCL_EXPORTABLE NXCSetEventHandler(NXC_EVENT_HANDLER pHandler);
void LIBNXCL_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc);
HREQUEST LIBNXCL_EXPORTABLE NXCRequest(DWORD dwOperation, ...);

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(DWORD dwId);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(char *pszName);
void LIBNXCL_EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *));
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetRootObject(void);
void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(DWORD *pdwNumObjects);
void LIBNXCL_EXPORTABLE NXCLockObjectIndex(void);
void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(void);

BOOL LIBNXCL_EXPORTABLE NXCGetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords);
void LIBNXCL_EXPORTABLE NXCModifyEventTemplate(NXC_EVENT_TEMPLATE *pEvent, DWORD dwMask, 
                                       DWORD dwSeverity, DWORD dwFlags, const char *pszName,
                                       const char *pszMessage, const char *pszDescription);

NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(DWORD dwId);

#ifdef __cplusplus
}
#endif


//
// Inline functions
//

#ifdef __cplusplus

#ifndef __libnxcl_inline_c__
inline DWORD NXCSyncObjects(void) { return NXCRequest(NXC_OP_SYNC_OBJECTS); }
inline DWORD NXCSyncEvents(void) { return NXCRequest(NXC_OP_SYNC_EVENTS); }
inline DWORD NXCOpenEventDB(void) { return NXCRequest(NXC_OP_OPEN_EVENT_DB); }
inline DWORD NXCCloseEventDB(BOOL bSaveChanges) { return NXCRequest(NXC_OP_CLOSE_EVENT_DB, bSaveChanges); }
inline DWORD NXCModifyObject(NXC_OBJECT_UPDATE *pData) { return NXCRequest(NXC_OP_MODIFY_OBJECT, pData); }
inline DWORD NXCLoadUserDB(void) { return NXCRequest(NXC_OP_LOAD_USER_DB); }
#endif   /* __libnxcl_inline_c__ */

#else    /* __cplusplus */

DWORD LIBNXCL_EXPORTABLE NXCSyncObjects(void);
DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(void);
DWORD LIBNXCL_EXPORTABLE NXCOpenEventDB(void);
DWORD LIBNXCL_EXPORTABLE NXCCloseEventDB(BOOL bSaveChanges);
DWORD LIBNXCL_EXPORTABLE NXCModifyObject(NXC_OBJECT_UPDATE *pData);
DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(void);

#endif   /* __cplusplus */

#endif   /* _nxclapi_h_ */
