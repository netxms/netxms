/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: np.cpp
**
**/

#include "nms_core.h"


//
// Poll new node for configuration
//

static void PollNode(DWORD dwIpAddr, DWORD dwNetMask, DWORD dwFlags)
{
   Node *pNode;

char buffer[32];
printf("Analyzing new node %s\n",IpToStr(dwIpAddr,buffer));
   // Check for node existence
   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
   {
printf("Node %s already exist in database\n", IpToStr(dwIpAddr,buffer));
      return;
   }

printf("Creating new node object\n");
   pNode = new Node(dwIpAddr, 0, dwFlags);
   if (!pNode->NewNodePoll(dwNetMask))
   {
printf("New node %s cannot be added\n",pNode->Name());
      delete pNode;     // Node poll failed, delete it
   }
   else
   {
      NetObjInsert(pNode, TRUE);
printf("New node %s added\n",pNode->Name());
   }
}


//
// Node poller thread (poll new nodes and put them into the database)
//

void NodePoller(void *arg)
{
   DB_RESULT hResult;
   int iPollInterval;

   // Read configuration
   iPollInterval = ConfigReadInt("NewNodePollingInterval", 60);

   while(!ShutdownInProgress())
   {
      ThreadSleep(iPollInterval);
      if (ShutdownInProgress())
         break;

      hResult = DBSelect(g_hCoreDB, "SELECT id,ip_addr,ip_netmask,discovery_flags FROM NewNodes");
      if (hResult != 0)
      {
         int i, iNumRows;
         DWORD dwId, dwIpAddr, dwNetMask, dwFlags;
         char szQuery[256];

         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwId = DBGetFieldULong(hResult, i, 0);
            dwIpAddr = DBGetFieldULong(hResult, i, 1);
            dwNetMask = DBGetFieldULong(hResult, i, 2);
            dwFlags = DBGetFieldULong(hResult, i, 3);

            PollNode(dwIpAddr, dwNetMask, dwFlags);

            // Delete processed node
            sprintf(szQuery, "DELETE FROM NewNodes WHERE id=%ld", dwId);
            DBQuery(g_hCoreDB, szQuery);
         }
         DBFreeResult(hResult);
      }
   }
}
