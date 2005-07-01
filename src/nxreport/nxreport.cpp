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

#include "nxreport.h"

//
// Static data
//

// Global output configuration
static BOOL gs_bDebug = FALSE;
static BOOL gs_bShowContainers = FALSE;
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


//
// Gets reference to object and it's level. Dumps it to current output stream.
// 
static BOOL ReportCurrentObject(NXC_OBJECT *hCurObj, DWORD dwLevel)
{
  DWORD i,t;
  DWORD dwResult;
  NXC_OBJECT *hSubObj;
  NXC_DCI_LIST *pItemList;
  
  // Display only nodes.
  if (hCurObj->iClass == OBJECT_NODE && !hCurObj->bIsDeleted)
  { 
    for(t=0; t < dwLevel; t++) printf("\t");

    // display node header and name
    _tprintf("%s\n", hCurObj->szName);

    // Display also node content.
    for(i = 0; i < hCurObj->dwNumChilds; i++)
    { 
      hSubObj=NXCFindObjectById(gs_hSession, hCurObj->pdwChildList[i]);
      if (hSubObj==NULL)
      {
         _tprintf(_T("Unable to get object with id[%d]\n"), hCurObj->pdwChildList[i]);
         return FALSE; // looks like DB is broken, better to stop enumeration.
      }
      else 
	   if (!hSubObj->bIsDeleted) 
    	 for(t=0; t < dwLevel; t++) printf("\t");
         _tprintf("\t%s\n", hSubObj->szName); 
    }

    // And now display DCIs related to node.    
    dwResult = NXCOpenNodeDCIList(gs_hSession, hCurObj->dwId, &pItemList);
    if (dwResult != RCC_SUCCESS)
    {
      _tprintf(_T("Unable to get DCI list for object %d: %s\n"),hSubObj->dwId, NXCGetErrorText(dwResult));
      return FALSE; // looks like DB is broken, better to stop enumeration.
    }
    else
    {
      for(i = 0; i < pItemList->dwNumItems; i++)
      {
	 for(t=0; t < dwLevel; t++) printf("\t");
         _tprintf("\t%s\n", pItemList->pItems[i].szName);
	      // TCHAR szDescription[MAX_DB_STRING];
      }
      NXCCloseNodeDCIList(gs_hSession, pItemList);
    }
    
    // display node footer here if applicable

  }
  else
  {
      if (gs_bShowContainers)
      {
	for(t=0; t < dwLevel; t++) printf("\t");
      	printf("[%s]\n",hCurObj->szName);
      };
  }
  
  return TRUE; // FALSE will stop enumaration, we don't want if for now.
};


//
// Do Object enumeration over function above one by one.
//
static void ReportNodeList()
{
   NXC_OBJECT *pCurrentObj;
   DWORD dwObjectLevel;
	StrictTree *pObjectTree;

	if (gs_dwRootObj)
		pObjectTree = new StrictTree(gs_hSession,gs_dwRootObj);
	else
		pObjectTree = new StrictTree(gs_hSession);
	
	if (pObjectTree->LoadTree())
   {
	  while ((pCurrentObj = pObjectTree->GetNextObject(&dwObjectLevel)) != NULL)
	  {
		ReportCurrentObject(pCurrentObj,dwObjectLevel);	
	  };
	}
}


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
            _tprintf("Usage: nxreport [<options>] <server> <event_id> [<param_1> [... <param_N>]]\n"
							"Valid options are:\n"
							"   -c            : Show container objects too. Default is not to show.\n"
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
	 case 'b':
	    gs_bShowContainers = TRUE;
	    break;
         case 'd':
            gs_bDebug = TRUE;
            break;
         case 'o':
            gs_dwRootObj = _tcstoul(optarg, NULL, 0);
            break;
         case 'P':
            _tcsncpy(szPassword, optarg, MAX_DB_STRING);
            break;
         case 'u':
            _tcsncpy(szLogin, optarg, MAX_DB_STRING);
            break;
         case 'v':
            printf("NetXMS Event Sender  Version " NETXMS_VERSION_STRING "\n");
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
         strncpy(szServer, argv[optind], 256);
         
         if (!NXCInitialize())
			{
				_tprintf(_T("Failed to initialize NetXMS client library\n"));
				return 5;
			}

	      if (gs_bDebug)
   	      NXCSetDebugCallback(DebugCallback);

      	dwResult = NXCConnect(szServer, szLogin, szPassword, &gs_hSession, FALSE, FALSE);
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
