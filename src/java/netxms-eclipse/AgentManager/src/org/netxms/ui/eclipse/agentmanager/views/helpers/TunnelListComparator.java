/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.AgentTunnel;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.agentmanager.views.TunnelManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Tunnel list comparator
 */
public class TunnelListComparator extends ViewerComparator
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      AgentTunnel t1 = (AgentTunnel)e1;
      AgentTunnel t2 = (AgentTunnel)e2;
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case TunnelManager.COL_AGENT_ID:
            result = t1.getAgentId().compareTo(t2.getAgentId());
            break;
         case TunnelManager.COL_AGENT_VERSION:
            result = t1.getAgentVersion().compareToIgnoreCase(t2.getAgentVersion());
            break;
         case TunnelManager.COL_CHANNELS:
            result = t1.getActiveChannelCount() - t2.getActiveChannelCount();
            break;
         case TunnelManager.COL_ID:
            result = t1.getId() - t2.getId();
            break;
         case TunnelManager.COL_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(t1.getAddress(), t2.getAddress());
            break;
         case TunnelManager.COL_NODE:
            NXCSession session = ConsoleSharedData.getSession();
            result = session.getObjectName(t1.getNodeId()).compareToIgnoreCase(session.getObjectName(t2.getNodeId()));
            break;
         case TunnelManager.COL_PLATFORM:
            result = t1.getPlatformName().compareToIgnoreCase(t2.getPlatformName());
            break;
         case TunnelManager.COL_STATE:
            result = (t1.isBound() ? 1 : 0) - (t2.isBound() ? 1 : 0);
            break;
         case TunnelManager.COL_SYSINFO:
            result = t1.getSystemInformation().compareToIgnoreCase(t2.getSystemInformation());
            break;
         case TunnelManager.COL_SYSNAME:
            result = t1.getSystemName().compareToIgnoreCase(t2.getSystemName());
            break;
         default:
            result = 0;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
