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

static WORD m_wServerPort = 4701;
static BOOL m_bDebug = FALSE;
static BOOL m_bShowContainers = FALSE;
static TCHAR m_szServer[MAX_DB_STRING] = _T("127.0.0.1");
static TCHAR m_szLogin[MAX_DB_STRING] = _T("guest");
static TCHAR m_szPassword[MAX_DB_STRING] = _T("");
static DWORD m_dwEventCode = 0;
static DWORD m_dwTimeOut = 3;
static BOOL m_dwRootObj = 0; // If it will stay 0 - we'll show all objects.
static NXC_SESSION g_hSession;
static StrictTree *pObjectTree;

//
// Callback function for debug printing
//

static void DebugCallback(char *pMsg)
{
   printf("*debug* %s\n", pMsg);
}



// Will get objects one by one during enumeration and will dump it.
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
      hSubObj=NXCFindObjectById(g_hSession, hCurObj->pdwChildList[i]);
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
    dwResult = NXCOpenNodeDCIList(g_hSession, hCurObj->dwId, &pItemList);
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
      NXCCloseNodeDCIList(g_hSession, pItemList);
    }
    
    // display node footer here if applicable

  }
  else
  {
      if (m_bShowContainers)
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
   DWORD dwResult;
   NXC_OBJECT *pCurrentObj;
   DWORD dwObjectLevel;

   if (!NXCInitialize())
   {
      _tprintf(_T("Failed to initialize NetXMS client library\n"));
   }
   else
   {
      if (m_bDebug)
         NXCSetDebugCallback(DebugCallback);

      dwResult = NXCConnect(m_szServer, m_szLogin, m_szPassword, &g_hSession);
      if (dwResult != RCC_SUCCESS)
      {
         _tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(dwResult));
      }
      else
      {
         NXCSetCommandTimeout(g_hSession, m_dwTimeOut * 1000);
	
	 // Sync local and remote object list.
	 dwResult = NXCSyncObjects(g_hSession);
      	 if (dwResult != RCC_SUCCESS)
      	 {
            _tprintf(_T("Unable to sync objects: %s\n"), NXCGetErrorText(dwResult));
      	 };

	if (m_dwRootObj)
		pObjectTree = new StrictTree(g_hSession,m_dwRootObj);
	else
		pObjectTree = new StrictTree(g_hSession);
	
	if (pObjectTree->LoadTree())
        {
	  while ((pCurrentObj = pObjectTree->GetNextObject(&dwObjectLevel)) != NULL)
	  {
		ReportCurrentObject(pCurrentObj,dwObjectLevel);	
	  };
	}

        NXCDisconnect(g_hSession);
      }
   }
}


//
// Entry point
//

int main(int argc, char *argv[])
{
   int ch;
   BOOL bStart = TRUE;
   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "dho:p:P:u:vw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxreport [<options>] <server> <event_id> [<param_1> [... <param_N>]]\n"
                   "Valid options are:\n"
		   "   -c            : Show container objects too. Default is not to show.\n"
                   "   -d            : Turn on debug mode.\n"
                   "   -h            : Display help and exit.\n"
		   "   -o <obj_id>   : Specify report's root object. Default is 0 (show all).\n"
                   "   -p <port>     : Specify server's port number. Default is %d.\n"
                   "   -P <password> : Specify user's password. Default is empty password.\n"
                   "   -u <user>     : Login to server as <user>. Default is \"guest\".\n"
                   "   -v            : Display version and exit.\n"
                   "   -w <seconds>  : Specify command timeout (default is 3 seconds).\n"
                   "\n", m_wServerPort);
            bStart = FALSE;
            break;
	 case 'b':
	    m_bShowContainers = TRUE;
	    break;
         case 'd':
            m_bDebug = TRUE;
            break;
         case 'o':
            m_dwRootObj = _tcstoul(optarg, NULL, 0);
            break;
         case 'P':
            _tcsncpy(m_szPassword, optarg, MAX_DB_STRING);
            break;
         case 'u':
            _tcsncpy(m_szLogin, optarg, MAX_DB_STRING);
            break;
         case 'v':
            printf("NetXMS Event Sender  Version " NETXMS_VERSION_STRING "\n");
            bStart = FALSE;
            break;
         case 'w':
            m_dwTimeOut = _tcstoul(optarg, NULL, 0);
            if ((m_dwTimeOut < 1) || (m_dwTimeOut > 120))
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

   // Do requested action if everything is OK
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
         strncpy(m_szServer, argv[optind], 256);
         ReportNodeList();
      }
   }

   return 0;
}
