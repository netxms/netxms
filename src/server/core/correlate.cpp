/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: correlate.cpp
**
**/

#include "nxcore.h"


//
// Correlate SYS_NODE_DOWN event
//

static void C_SysNodeDown(Node *pNode, Event *pEvent)
{
   NETWORK_PATH_TRACE *pTrace;
   Node *pMgmtNode;
   Interface *pInterface;
   NetObj *pObject;
   int i;

   // Trace route from management station to failed node and
   // check for failed intermediate nodes or interfaces
   pMgmtNode = (Node *)FindObjectById(g_dwMgmtNode);
   if (pMgmtNode != NULL)
   {
      pTrace = TraceRoute(pMgmtNode, pNode);
      if (pTrace != NULL)
      {
         for(i = 0; i < pTrace->iNumHops; i++)
         {
            if ((pTrace->pHopList[i].pObject != NULL) &&
                (pTrace->pHopList[i].pObject != pNode))
            {
               if (pTrace->pHopList[i].pObject->Type() == OBJECT_NODE)
               {
                  if (((Node *)pTrace->pHopList[i].pObject)->IsDown())
                  {
                     pEvent->SetRootId(((Node *)pTrace->pHopList[i].pObject)->GetLastEventId(LAST_EVENT_NODE_DOWN));
                  }
                  else
                  {
                     if (pTrace->pHopList[i].bIsVPN)
                     {
                        // Next hop is behind VPN tunnel
                        pObject = FindObjectById(pTrace->pHopList[i].dwIfIndex);
                        if (pObject != NULL)
                        {
                           if ((pObject->Type() == OBJECT_VPNCONNECTOR) &&
                               (pObject->Status() == STATUS_CRITICAL))
                           {
                              /* TODO: set root id */
                           }
                        }
                     }
                     else
                     {
                        pInterface = ((Node *)pTrace->pHopList[i].pObject)->FindInterface(pTrace->pHopList[i].dwIfIndex, INADDR_ANY);
                        if (pInterface != NULL)
                        {
                           if (pInterface->Status() == STATUS_CRITICAL)
                           {
                              pEvent->SetRootId(pInterface->GetLastDownEventId());
                           }
                        }
                     }
                  }
               }
            }
         }
         DestroyTraceData(pTrace);
      }
   }
}


//
// Correlate event
//

void CorrelateEvent(Event *pEvent)
{
   NetObj *pObject;
   Interface *pInterface;

   pObject = FindObjectById(pEvent->SourceId());
   if ((pObject != NULL) && (pObject->Type() == OBJECT_NODE))
   {
      switch(pEvent->Code())
      {
         case EVENT_INTERFACE_DOWN:
            pInterface = ((Node *)pObject)->FindInterface(pEvent->GetParameterAsULong(4), INADDR_ANY);
            if (pInterface != NULL)
            {
               pInterface->SetLastDownEventId(pEvent->Id());
            }
         case EVENT_SERVICE_DOWN:
         case EVENT_SNMP_FAIL:
         case EVENT_AGENT_FAIL:
            if (((Node *)pObject)->RuntimeFlags() & NDF_UNREACHABLE)
            {
               pEvent->SetRootId(((Node *)pObject)->GetLastEventId(LAST_EVENT_NODE_DOWN));
            }
            break;
         case EVENT_NODE_DOWN:
            ((Node *)pObject)->SetLastEventId(LAST_EVENT_NODE_DOWN, pEvent->Id());
            C_SysNodeDown((Node *)pObject, pEvent);
            break;
         case EVENT_NODE_UP:
            ((Node *)pObject)->SetLastEventId(LAST_EVENT_NODE_DOWN, 0);
            break;
         default:
            break;
      }
   }
}
