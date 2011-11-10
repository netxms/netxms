/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: correlate.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static QWORD m_networkLostEventId = 0;


//
// Correlate SYS_NODE_DOWN event
//

static void C_SysNodeDown(Node *pNode, Event *pEvent)
{
   NetworkPath *pTrace;
   Node *pMgmtNode;
   Interface *pInterface;
   NetObj *pObject;
   int i;

	// Check for NetXMS server netwok connectivity
	if (g_dwFlags & AF_NO_NETWORK_CONNECTIVITY)
	{
		pEvent->setRootId(m_networkLostEventId);
		return;
	}

   // Trace route from management station to failed node and
   // check for failed intermediate nodes or interfaces
   pMgmtNode = (Node *)FindObjectById(g_dwMgmtNode);
   if (pMgmtNode != NULL)
   {
      pTrace = TraceRoute(pMgmtNode, pNode);
      if (pTrace != NULL)
      {
         for(i = 0; i < pTrace->getHopCount(); i++)
         {
				HOP_INFO *hop = pTrace->getHopInfo(i);
            if ((hop->object != NULL) && (hop->object != pNode))
            {
               if (hop->object->Type() == OBJECT_NODE)
               {
                  if (((Node *)hop->object)->isDown())
                  {
                     pEvent->setRootId(((Node *)hop->object)->GetLastEventId(LAST_EVENT_NODE_DOWN));
                  }
                  else
                  {
                     if (hop->isVpn)
                     {
                        // Next hop is behind VPN tunnel
                        pObject = FindObjectById(hop->ifIndex);
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
                        pInterface = ((Node *)hop->object)->findInterface(hop->ifIndex, INADDR_ANY);
                        if (pInterface != NULL)
                        {
                           if (pInterface->Status() == STATUS_CRITICAL)
                           {
                              pEvent->setRootId(pInterface->getLastDownEventId());
                           }
                        }
                     }
                  }
               }
            }
         }
         delete pTrace;
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

   pObject = FindObjectById(pEvent->getSourceId());
   if ((pObject != NULL) && (pObject->Type() == OBJECT_NODE))
   {
      switch(pEvent->getCode())
      {
         case EVENT_INTERFACE_DOWN:
            pInterface = ((Node *)pObject)->findInterface(pEvent->getParameterAsULong(4), INADDR_ANY);
            if (pInterface != NULL)
            {
               pInterface->setLastDownEventId(pEvent->getId());
            }
         case EVENT_SERVICE_DOWN:
         case EVENT_SNMP_FAIL:
         case EVENT_AGENT_FAIL:
            if (((Node *)pObject)->getRuntimeFlags() & NDF_UNREACHABLE)
            {
               pEvent->setRootId(((Node *)pObject)->GetLastEventId(LAST_EVENT_NODE_DOWN));
            }
            break;
         case EVENT_NODE_DOWN:
            ((Node *)pObject)->SetLastEventId(LAST_EVENT_NODE_DOWN, pEvent->getId());
            C_SysNodeDown((Node *)pObject, pEvent);
            break;
         case EVENT_NODE_UP:
            ((Node *)pObject)->SetLastEventId(LAST_EVENT_NODE_DOWN, 0);
            break;
			case EVENT_NETWORK_CONNECTION_LOST:
				m_networkLostEventId = pEvent->getId();
				break;
         default:
            break;
      }
   }
}
