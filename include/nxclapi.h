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
#include <nms_util.h>
#include <nxevent.h>
#include <nximage.h>

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
#define MAX_USER_NAME            64
#define MAX_USER_FULLNAME        128
#define MAX_USER_DESCR           256
#define MAX_ITEM_NAME            256
#define MAX_STRING_VALUE         256
#define GROUP_FLAG               ((DWORD)0x80000000)
#define GROUP_EVERYONE           ((DWORD)0x80000000)
#define INVALID_UID              ((DWORD)0xFFFFFFFF)
#define OBJECT_STATUS_COUNT      9


//
// Image formats
//

#define IMAGE_FORMAT_ICO      0
#define IMAGE_FORMAT_PNG      1


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
#define OBJECT_CONTAINER   5
#define OBJECT_ZONE        6
#define OBJECT_SERVICEROOT 7


//
// Object's status
//

#define STATUS_NORMAL      0
#define STATUS_WARNING     1
#define STATUS_MINOR       2
#define STATUS_MAJOR       3
#define STATUS_CRITICAL    4
#define STATUS_UNKNOWN     5
#define STATUS_UNMANAGED   6
#define STATUS_DISABLED    7
#define STATUS_TESTING     8


//
// Node flags
//

#define NF_IS_SNMP         0x0001
#define NF_IS_NATIVE_AGENT 0x0002
#define NF_IS_BRIDGE       0x0004
#define NF_IS_ROUTER       0x0008
#define NF_IS_LOCAL_MGMT   0x0010
#define NF_IS_PRINTER      0x0020
#define NS_IS_OSPF         0x0040


//
// Events
//

#define NXC_EVENT_STATE_CHANGED        1
#define NXC_EVENT_ERROR                2
#define NXC_EVENT_LOGIN_RESULT         3
#define NXC_EVENT_NEW_ELOG_RECORD      4
#define NXC_EVENT_USER_DB_CHANGED      5
#define NXC_EVENT_OBJECT_CHANGED       6
#define NXC_EVENT_NOTIFICATION         7


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
#define STATE_LOAD_DCI        8


//
// Notification codes
//

#define NX_NOTIFY_SHUTDOWN          1
#define NX_NOTIFY_EVENTDB_CHANGED   2


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
#define RCC_ALREADY_EXIST           ((DWORD)8)
#define RCC_COMM_FAILURE            ((DWORD)9)
#define RCC_SYSTEM_FAILURE          ((DWORD)10)
#define RCC_INVALID_USER_ID         ((DWORD)11)
#define RCC_INVALID_ARGUMENT        ((DWORD)12)
#define RCC_DUPLICATE_DCI           ((DWORD)13)
#define RCC_INVALID_DCI_ID          ((DWORD)14)
#define RCC_OUT_OF_MEMORY           ((DWORD)15)
#define RCC_IO_ERROR                ((DWORD)16)
#define RCC_INCOMPATIBLE_OPERATION  ((DWORD)17)
#define RCC_OBJECT_CREATION_FAILED  ((DWORD)18)
#define RCC_OBJECT_LOOP             ((DWORD)19)


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
#define SYSTEM_ACCESS_EPP                 0x0040

#define SYSTEM_ACCESS_FULL                0x007F


//
// Object access rights
//

#define OBJECT_ACCESS_READ          0x00000001
#define OBJECT_ACCESS_MODIFY        0x00000002
#define OBJECT_ACCESS_CREATE        0x00000004
#define OBJECT_ACCESS_DELETE        0x00000008
#define OBJECT_ACCESS_MOVE          0x00000010
#define OBJECT_ACCESS_CONTROL       0x00000020


//
// User/group flags
//

#define UF_MODIFIED                 0x0001
#define UF_DELETED                  0x0002
#define UF_DISABLED                 0x0004
#define UF_CHANGE_PASSWORD          0x0008
#define UF_CANNOT_CHANGE_PASSWORD   0x0010


//
// User database change notification types
//

#define USER_DB_CREATE              0
#define USER_DB_DELETE              1
#define USER_DB_MODIFY              2


//
// Data types
//

#define DTYPE_INTEGER   0
#define DTYPE_INT64     1
#define DTYPE_STRING    2
#define DTYPE_FLOAT     3


//
// Data sources
//

#define DS_INTERNAL        0
#define DS_NATIVE_AGENT    1
#define DS_SNMP_AGENT      2


//
// Item status
//

#define ITEM_STATUS_ACTIVE          0
#define ITEM_STATUS_DISABLED        1
#define ITEM_STATUS_NOT_SUPPORTED   2


//
// Threshold functions and operations
//

#define F_LAST       0
#define F_AVERAGE    1
#define F_DEVIATION  2

#define OP_LE        0
#define OP_LE_EQ     1
#define OP_EQ        2
#define OP_GT_EQ     3
#define OP_GT        4
#define OP_NE        5
#define OP_LIKE      6
#define OP_NOTLIKE   7


//
// Event policy rule flags
//

#define RF_STOP_PROCESSING    0x0001
#define RF_NEGATED_SOURCE     0x0002
#define RF_NEGATED_EVENTS     0x0004
#define RF_GENERATE_ALARM     0x0008
#define RF_ALARM_IS_ACK       0x0010
#define RF_DISABLED           0x0020
#define RF_SEVERITY_INFO      0x0100
#define RF_SEVERITY_MINOR     0x0200
#define RF_SEVERITY_WARNING   0x0400
#define RF_SEVERITY_MAJOR     0x0800
#define RF_SEVERITY_CRITICAL  0x1000


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
// Event name/code pair
//

typedef struct
{
   DWORD dwEventId;
   char szName[MAX_EVENT_NAME];
} NXC_EVENT_NAME;


//
// Image information
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_OBJECT_NAME];
   BYTE hash[MD5_DIGEST_SIZE];
} NXC_IMAGE;


//
// Image list
//

typedef struct
{
   DWORD dwNumImages;
   NXC_IMAGE *pImageList;
} NXC_IMAGE_LIST;


/********************************************************************
 * Following part of this file shouldn't be included by server code *
 ********************************************************************/

#ifndef LIBNXCL_NO_DECLARATIONS


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
   DWORD dwImage;       // Image ID or IMG_DEFAULT for default image
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
      struct
      {
         DWORD dwCategory;
         char *pszDescription;
      } container;
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
   char szFullName[MAX_USER_FULLNAME];    // Only for users
   char szDescription[MAX_USER_DESCR];
} NXC_USER;


//
// Data collection item threshold structure
//

typedef struct
{
   DWORD dwId;
   DWORD dwEvent;
   WORD wFunction;
   WORD wOperation;
   DWORD dwArg1;
   DWORD dwArg2;
   union
   {
      DWORD dwInt32;
      INT64 qwInt64;
      double dFloat;
      char szString[MAX_STRING_VALUE];
   } value;
} NXC_DCI_THRESHOLD;


//
// Data collection item structure
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_ITEM_NAME];
   int iPollingInterval;
   int iRetentionTime;
   BYTE iSource;
   BYTE iDataType;
   BYTE iStatus;
   DWORD dwNumThresholds;
   NXC_DCI_THRESHOLD *pThresholdList;
} NXC_DCI;


//
// Node's data collection list information
//

typedef struct
{
   DWORD dwNodeId;
   DWORD dwNumItems;
   NXC_DCI *pItems;
} NXC_DCI_LIST;


//
// Row structure
//

#pragma pack(1)

typedef struct
{
   DWORD dwTimeStamp;
   union
   {
      DWORD dwInt32;
      INT64 qwInt64;
      double dFloat;
      char szString[MAX_STRING_VALUE];
   } value;
} NXC_DCI_ROW;

#pragma pack()


//
// DCI's collected data
//

typedef struct
{
   DWORD dwNodeId;      // Owner's node id
   DWORD dwItemId;      // Item id
   DWORD dwNumRows;     // Number of rows in set
   WORD wDataType;      // Data type (integer, string, etc.)
   WORD wRowSize;       // Size of single row in bytes
   NXC_DCI_ROW *pRows;  // Array of rows
} NXC_DCI_DATA;


//
// MIB file list
//

typedef struct
{
   DWORD dwNumFiles;
   char **ppszName;
   BYTE **ppHash;
} NXC_MIB_LIST;


//
// Event processing policy rule
//

typedef struct
{
   DWORD dwFlags;
   DWORD dwId;
   DWORD dwNumActions;
   DWORD dwNumEvents;
   DWORD dwNumSources;
   DWORD *pdwActionList;
   DWORD *pdwEventList;
   DWORD *pdwSourceList;
   char *pszComment;
   char szAlarmKey[MAX_DB_STRING];
   char szAlarmAckKey[MAX_DB_STRING];
   char szAlarmMessage[MAX_DB_STRING];
   WORD wAlarmSeverity;
} NXC_EPP_RULE;


//
// Event processing policy
//

typedef struct
{
   DWORD dwNumRules;
   NXC_EPP_RULE *pRuleList;
} NXC_EPP;


//
// Object creation information structure
//

typedef struct
{
   int iClass;
   DWORD dwParentId;
   char *pszName;
   union
   {
      struct
      {
         DWORD dwIpAddr;
         DWORD dwNetMask;
      } node;
      struct
      {
         DWORD dwCategory;
         char *pszDescription;
      } container;
   } cs;
} NXC_OBJECT_CREATE_INFO;


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

DWORD LIBNXCL_EXPORTABLE NXCGetVersion(void);
const char LIBNXCL_EXPORTABLE *NXCGetErrorText(DWORD dwError);

BOOL LIBNXCL_EXPORTABLE NXCInitialize(void);
DWORD LIBNXCL_EXPORTABLE NXCConnect(char *szServer, char *szLogin, char *szPassword);
void LIBNXCL_EXPORTABLE NXCDisconnect(void);
void LIBNXCL_EXPORTABLE NXCSetEventHandler(NXC_EVENT_HANDLER pHandler);
void LIBNXCL_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc);
HREQUEST LIBNXCL_EXPORTABLE NXCRequest(DWORD dwOperation, ...);

DWORD LIBNXCL_EXPORTABLE NXCSyncObjects(void);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(DWORD dwId);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(char *pszName);
void LIBNXCL_EXPORTABLE NXCEnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *));
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTopologyRootObject(void);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetServiceRootObject(void);
void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(DWORD *pdwNumObjects);
void LIBNXCL_EXPORTABLE NXCLockObjectIndex(void);
void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(void);
DWORD LIBNXCL_EXPORTABLE NXCModifyObject(NXC_OBJECT_UPDATE *pData);
DWORD LIBNXCL_EXPORTABLE NXCSetObjectMgmtStatus(DWORD dwObjectId, BOOL bIsManaged);
DWORD LIBNXCL_EXPORTABLE NXCCreateObject(NXC_OBJECT_CREATE_INFO *pCreateInfo, DWORD *pdwObjectId);
DWORD LIBNXCL_EXPORTABLE NXCBindObject(DWORD dwParentObject, DWORD dwChildObject);
DWORD LIBNXCL_EXPORTABLE NXCUnbindObject(DWORD dwParentObject, DWORD dwChildObject);

DWORD LIBNXCL_EXPORTABLE NXCSyncEvents(void);
DWORD LIBNXCL_EXPORTABLE NXCOpenEventDB(void);
DWORD LIBNXCL_EXPORTABLE NXCCloseEventDB(BOOL bSaveChanges);
BOOL LIBNXCL_EXPORTABLE NXCGetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords);
void LIBNXCL_EXPORTABLE NXCModifyEventTemplate(NXC_EVENT_TEMPLATE *pEvent, DWORD dwMask, 
                                       DWORD dwSeverity, DWORD dwFlags, const char *pszName,
                                       const char *pszMessage, const char *pszDescription);
DWORD LIBNXCL_EXPORTABLE NXCLoadEventNames(void);
NXC_EVENT_NAME LIBNXCL_EXPORTABLE *NXCGetEventNamesList(DWORD *pdwNumEvents);
const char LIBNXCL_EXPORTABLE *NXCGetEventName(DWORD dwId);

DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(void);
NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(DWORD dwId);
BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_USER **ppUserList, DWORD *pdwNumUsers);
DWORD LIBNXCL_EXPORTABLE NXCLockUserDB(void);
DWORD LIBNXCL_EXPORTABLE NXCUnlockUserDB(void);
DWORD LIBNXCL_EXPORTABLE NXCCreateUser(char *pszName, BOOL bIsGroup, DWORD *pdwNewId);
DWORD LIBNXCL_EXPORTABLE NXCDeleteUser(DWORD dwId);
DWORD LIBNXCL_EXPORTABLE NXCModifyUser(NXC_USER *pUserInfo);
DWORD LIBNXCL_EXPORTABLE NXCSetPassword(DWORD dwUserId, char *pszNewPassword);

DWORD LIBNXCL_EXPORTABLE NXCOpenNodeDCIList(DWORD dwNodeId, NXC_DCI_LIST **ppItemList);
DWORD LIBNXCL_EXPORTABLE NXCCloseNodeDCIList(NXC_DCI_LIST *pItemList);
DWORD LIBNXCL_EXPORTABLE NXCCreateNewDCI(NXC_DCI_LIST *pItemList, DWORD *pdwItemId);
DWORD LIBNXCL_EXPORTABLE NXCUpdateDCI(DWORD dwNodeId, NXC_DCI *pItem);
DWORD LIBNXCL_EXPORTABLE NXCDeleteDCI(NXC_DCI_LIST *pItemList, DWORD dwItemId);
DWORD LIBNXCL_EXPORTABLE NXCItemIndex(NXC_DCI_LIST *pItemList, DWORD dwItemId);
DWORD LIBNXCL_EXPORTABLE NXCGetDCIData(DWORD dwNodeId, DWORD dwItemId, DWORD dwMaxRows,
                                       DWORD dwTimeFrom, DWORD dwTimeTo, NXC_DCI_DATA **ppData);
void LIBNXCL_EXPORTABLE NXCDestroyDCIData(NXC_DCI_DATA *pData);
NXC_DCI_ROW LIBNXCL_EXPORTABLE *NXCGetRowPtr(NXC_DCI_DATA *pData, DWORD dwRow);
DWORD LIBNXCL_EXPORTABLE NXCAddThresholdToItem(NXC_DCI *pItem, NXC_DCI_THRESHOLD *pThreshold);
BOOL LIBNXCL_EXPORTABLE NXCDeleteThresholdFromItem(NXC_DCI *pItem, DWORD dwIndex);
BOOL LIBNXCL_EXPORTABLE NXCSwapThresholds(NXC_DCI *pItem, DWORD dwIndex1, DWORD dwIndex2);

DWORD LIBNXCL_EXPORTABLE NXCGetMIBList(NXC_MIB_LIST **ppMibList);
void LIBNXCL_EXPORTABLE NXCDestroyMIBList(NXC_MIB_LIST *pMibList);
DWORD LIBNXCL_EXPORTABLE NXCDownloadMIBFile(char *pszName, char *pszDestDir);

DWORD LIBNXCL_EXPORTABLE NXCOpenEventPolicy(NXC_EPP **ppEventPolicy);
DWORD LIBNXCL_EXPORTABLE NXCCloseEventPolicy(void);
DWORD LIBNXCL_EXPORTABLE NXCSaveEventPolicy(NXC_EPP *pEventPolicy);
void LIBNXCL_EXPORTABLE NXCDestroyEventPolicy(NXC_EPP *pEventPolicy);
void LIBNXCL_EXPORTABLE NXCDeletePolicyRule(NXC_EPP *pEventPolicy, DWORD dwRule);

DWORD LIBNXCL_EXPORTABLE NXCSyncImages(NXC_IMAGE_LIST **ppImageList, char *pszCacheDir, WORD wFormat);
DWORD LIBNXCL_EXPORTABLE NXCLoadImageFile(DWORD dwImageId, char *pszCacheDir, WORD wFormat);
void LIBNXCL_EXPORTABLE NXCDestroyImageList(NXC_IMAGE_LIST *pImageList);
DWORD LIBNXCL_EXPORTABLE NXCLoadDefaultImageList(DWORD *pdwListSize,
                                                 DWORD **ppdwClassId, DWORD **ppdwImageId);

#ifdef __cplusplus
}
#endif

#endif   /* LIBNXCL_NO_DECLARATIONS */

#endif   /* _nxclapi_h_ */
