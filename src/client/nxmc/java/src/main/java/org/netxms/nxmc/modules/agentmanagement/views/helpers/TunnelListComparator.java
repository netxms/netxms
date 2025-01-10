/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.AgentTunnel;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.agentmanagement.views.TunnelManager;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Tunnel list comparator
 */
public class TunnelListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      AgentTunnel t1 = (AgentTunnel)e1;
      AgentTunnel t2 = (AgentTunnel)e2;
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case TunnelManager.COL_AGENT_ID:
            result = t1.getAgentId().compareTo(t2.getAgentId());
            break;
         case TunnelManager.COL_AGENT_PROXY:
            result = Boolean.compare(t1.isAgentProxy(), t2.isAgentProxy());
            break;
         case TunnelManager.COL_AGENT_VERSION:
            result = t1.getAgentVersion().compareToIgnoreCase(t2.getAgentVersion());
            break;
         case TunnelManager.COL_CERTIFICATE_EXPIRATION:
            result = (t1.getCertificateExpirationTime() != null) ? 
                  ((t2.getCertificateExpirationTime() != null) ? 
                        t1.getCertificateExpirationTime().compareTo(t2.getCertificateExpirationTime()) : 1) :
                           ((t2.getCertificateExpirationTime() == null) ? 0 : -1);
            break;
         case TunnelManager.COL_CHANNELS:
            result = t1.getActiveChannelCount() - t2.getActiveChannelCount();
            break;
         case TunnelManager.COL_HARDWARE_ID:
            result = t1.getHardwareIdAsText().compareTo(t2.getHardwareIdAsText());
            break;
         case TunnelManager.COL_ID:
            result = t1.getId() - t2.getId();
            break;
         case TunnelManager.COL_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(t1.getAddress(), t2.getAddress());
            break;
         case TunnelManager.COL_NODE:
            NXCSession session = Registry.getSession();
            result = session.getObjectName(t1.getNodeId()).compareToIgnoreCase(session.getObjectName(t2.getNodeId()));
            break;
         case TunnelManager.COL_PLATFORM:
            result = t1.getPlatformName().compareToIgnoreCase(t2.getPlatformName());
            break;
         case TunnelManager.COL_SERIAL_NUMBER:
            result = t1.getSerialNumber().compareToIgnoreCase(t2.getSerialNumber());
            break;
         case TunnelManager.COL_SNMP_PROXY:
            result = Boolean.compare(t1.isSnmpProxy(), t2.isSnmpProxy());
            break;
         case TunnelManager.COL_SNMP_TRAP_PROXY:
            result = Boolean.compare(t1.isSnmpTrapProxy(), t2.isSnmpTrapProxy());
            break;
         case TunnelManager.COL_SYSLOG_PROXY:
            result = Boolean.compare(t1.isSyslogProxy(), t2.isSnmpProxy());
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
         case TunnelManager.COL_USER_AGENT:
            result = Boolean.compare(t1.isUserAgentInstalled(), t2.isUserAgentInstalled());
            break;
         case TunnelManager.COL_CONNECTION_TIME:
            result = (t1.getConnectionTime() != null) ? 
                  ((t2.getConnectionTime() != null) ? 
                        t1.getConnectionTime().compareTo(t2.getConnectionTime()) : 1) :
                           ((t2.getConnectionTime() == null) ? 0 : -1);
            break;
         default:
            result = 0;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
