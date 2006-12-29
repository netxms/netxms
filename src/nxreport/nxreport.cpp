/* 
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxreport.cpp
**
**/

#include <stdarg.h>
#include "nxreport.h"

//
// Static data
//

// Global output configuration
static BOOL gs_bDebug = FALSE;
static BOOL gs_dwRootObj = 0; // If it will stay 0 - we'll show all objects.

// Session is used in many places, so we keep it global to avoid parameter passing overhead
static NXC_SESSION gs_hSession;


//
// Callback function for debug printing
//
static void DebugCallback(char *pszMsg)
{
	if (gs_bDebug)
   	_tprintf("*debug* %s\n", pszMsg);
}

void CloseTag(int nType,NXC_OBJECT *pObj, DWORD dwLevel)
{
	switch (nType)
	{
		case OBJECT_NODE:
			print_tag(TAG_NODE_CLOSE);		
			break;

		case OBJECT_INTERFACE:
			print_tag(TAG_IFACE_CLOSE);		
			break;

		case OBJECT_CONTAINER:
			print_tag(TAG_CONT_CLOSE);
			break;
	
		case OBJECT_SUBNET:
			print_tag(TAG_SUBNET_CLOSE);
			break;
	
		case OBJECT_NETWORK:
			print_tag(TAG_NETW_CLOSE);
			break;
	
		case OBJECT_ZONE:
			print_tag(TAG_ZONE_CLOSE);
			break;
	
		case OBJECT_SERVICEROOT:
			print_tag(TAG_SERVROOT_CLOSE);
			break;
	
		case OBJECT_TEMPLATE:
			print_tag(TAG_TEMPL_CLOSE);
			break;
	
		case OBJECT_TEMPLATEGROUP:
			print_tag(TAG_TEMPLGRP_CLOSE);
			break;
	
		case OBJECT_VPNCONNECTOR:
			print_tag(TAG_VPNCONN_CLOSE);
			break;
	
		case OBJECT_CONDITION:
			print_tag(TAG_COND_CLOSE);
			break;
	
		case OBJECT_GENERIC:
			print_tag(TAG_GENERIC,pObj->szName);
			break;	

		case OBJECT_NETWORKSERVICE:	
			print_tag(TAG_SERVICE_CLOSE); 
			break;

		default:
			break;
	}

}

void OpenTag(NXC_OBJECT *pCurrentObj, DWORD dwLevel)
{
  DWORD dwResult,i;
  NXC_DCI_LIST *pItemList;

    switch (pCurrentObj->iClass)
     {
	// Display only nodes.
	case OBJECT_NODE:
		// display node header and name
		print_tag(TAG_NODE_OPEN, pCurrentObj->szName);
	
		// And now display DCIs related to node.    
		dwResult = NXCOpenNodeDCIList(gs_hSession, pCurrentObj->dwId, &pItemList);
		if (dwResult != RCC_SUCCESS)
		{
		_tprintf(_T("Unable to get DCI list for object %d: %s\n"),pCurrentObj->dwId, NXCGetErrorText(dwResult));
		return; // looks like DB is broken, better to stop enumeration.
		}
		else
		{
			for(i = 0; i < pItemList->dwNumItems; i++)
			{
				print_tag(TAG_DCI_OPEN, pItemList->pItems[i].szName);
				_tprintf("%s\n",pItemList->pItems[i].szDescription);
				print_tag(TAG_DCI_CLOSE);
			}
		}
		NXCCloseNodeDCIList(gs_hSession, pItemList);
		break;

	case OBJECT_NETWORKSERVICE:	
		print_tag(TAG_SERVICE_OPEN, pCurrentObj->szName); 
		break;

	case OBJECT_INTERFACE:
		print_tag(TAG_IFACE_OPEN, pCurrentObj->szName);
	  	break;				 
	
	case OBJECT_CONTAINER:
		print_tag(TAG_CONT_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_SUBNET:
		print_tag(TAG_SUBNET_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_NETWORK:
		print_tag(TAG_NETW_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_ZONE:
		print_tag(TAG_ZONE_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_SERVICEROOT:
		print_tag(TAG_SERVROOT_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_TEMPLATE:
		print_tag(TAG_TEMPL_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_TEMPLATEGROUP:
		print_tag(TAG_TEMPLGRP_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_VPNCONNECTOR:
		print_tag(TAG_VPNCONN_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_CONDITION:
		print_tag(TAG_COND_OPEN,pCurrentObj->szName);
		break;

	case OBJECT_GENERIC:
		print_tag(TAG_GENERIC,pCurrentObj->szName);
		break;
     };
};
//
// Gets reference to object and it's level. Dumps it to current output stream.
// 
static BOOL ReportNodeList()
{
  NXC_OBJECT *pCurrentObj;
  DWORD dwObjectLevel,dwCurrLevel,i;
  StrictTree *pObjectTree;
  int anContTypes[256];

  memset(anContTypes,0,sizeof(int)*256);
  dwCurrLevel=0;

  if (gs_dwRootObj)
     pObjectTree = new StrictTree(gs_hSession,gs_dwRootObj);
  else
     pObjectTree = new StrictTree(gs_hSession);
	
  if (pObjectTree->LoadTree())
  {
   print_tag(TAG_DUMMY_BEGIN);
   
   while ((pCurrentObj = pObjectTree->GetNextObject(&dwObjectLevel)) != NULL)
   {
    if (dwObjectLevel==0) continue;
    if (pCurrentObj->bIsDeleted) continue;

    if (dwCurrLevel+1 <= dwObjectLevel) 
	{
	  anContTypes[dwObjectLevel]=pCurrentObj->iClass;
	  dwCurrLevel = dwObjectLevel;
	}
    else 
      { 
	for (i=dwCurrLevel;i>=dwObjectLevel;i--)
	{	
		CloseTag(anContTypes[i],pCurrentObj,i);
		anContTypes[i]=0;	
	};
	dwCurrLevel = dwObjectLevel;
	anContTypes[dwCurrLevel]=pCurrentObj->iClass;	
      }; 
    OpenTag(pCurrentObj,dwObjectLevel);
   };
   
  if (dwCurrLevel>0) 
	for (i=dwCurrLevel;i>0;i--)
	{	
		CloseTag(anContTypes[i],pCurrentObj,i);
		anContTypes[i]=0;	
	};	
  print_tag(TAG_DUMMY_END);
  };
};

//
// Entry point
//
int main(int argc, char *argv[])
{
   int ch;
   BOOL bStart = TRUE;
   DWORD dwResult;
   TCHAR szServer[MAX_DB_STRING] = _T("127.0.0.1");
	WORD  wServerPort = 4701;
	TCHAR szLogin[MAX_DB_STRING] = _T("guest");
	TCHAR szPassword[MAX_DB_STRING] = _T("");
	DWORD dwTimeOut = 3;
  
   //
   // Parse command line
   // 
   opterr = 1;
   while((ch = getopt(argc, argv, "dho:p:P:u:vw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf("Usage: nxreport [<options>] <server> \n"
							"Valid options are:\n"
							"   -d            : Turn on debug mode.\n"
							"   -h            : Display help and exit.\n"
							"   -o <obj_id>   : Specify report's root object. Default is %d (show all).\n"
							"   -p <port>     : Specify server's port number. Default is %d.\n"
							"   -P <password> : Specify user's password. Default is empty password.\n"
							"   -u <user>     : Login to server as <user>. Default is \"%s\".\n"
							"   -v            : Display version and exit.\n"
							"   -w <seconds>  : Specify command timeout (default is %d seconds).\n"
							"\n", gs_dwRootObj, wServerPort,szLogin,dwTimeOut);
            bStart = FALSE;
            break;
         case 'd':
            gs_bDebug = TRUE;
            break;
         case 'o':
            gs_dwRootObj = _tcstoul(optarg, NULL, 0);
            break;
         case 'P':
            nx_strncpy(szPassword, optarg, MAX_DB_STRING);
            break;
         case 'u':
            nx_strncpy(szLogin, optarg, MAX_DB_STRING);
            break;
         case 'v':
            printf("NetXMS Object Dumper Version " NETXMS_VERSION_STRING "\n");
            bStart = FALSE;
            break;
         case 'w':
            dwTimeOut = _tcstoul(optarg, NULL, 0);
            if ((dwTimeOut < 1) || (dwTimeOut > 120))
            {
               _tprintf(_T("Invalid timeout %s\n"), optarg);
               bStart = FALSE;
            }
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }



   // 
   // Do requested action if everything is OK
   //
   if (bStart)
   {
		if (argc - optind < 1)
      {
         _tprintf(_T("Required arguments missing. Use nxreport -h for help.\n"));
      }
      else
      {
#ifdef _WIN32
         WSADATA wsaData;

         if (WSAStartup(2, &wsaData) != 0)
         {
            _tprintf(_T("Unable to initialize Windows sockets\n"));
            return 4;
         }
#endif
         nx_strncpy(szServer, argv[optind], 256);
         
         if (!NXCInitialize())
			{
				_tprintf(_T("Failed to initialize NetXMS client library\n"));
				return 5;
			}

	      if (gs_bDebug)
   	      NXCSetDebugCallback(DebugCallback);

      	dwResult = NXCConnect(szServer, szLogin, szPassword, &gs_hSession,
                               _T("nxreport/") NETXMS_VERSION_STRING, FALSE, FALSE, NULL);
			if (dwResult != RCC_SUCCESS)
			{
				_tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(dwResult));\
				return 6;
			}

			NXCSetCommandTimeout(gs_hSession, dwTimeOut * 1000);
	
			// Sync local and remote object list.
			dwResult = NXCSyncObjects(gs_hSession);
			if (dwResult != RCC_SUCCESS)
      	{
         	_tprintf(_T("Unable to sync objects: %s\n"), NXCGetErrorText(dwResult));
         	return 7;
      	};
      	
        ReportNodeList();
        NXCDisconnect(gs_hSession);
      }
   }
   return 0;
}
