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

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.AgentTunnel;
import org.netxms.ui.eclipse.agentmanager.views.TunnelManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for tunnel list
 */
public class TunnelListLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color COLOR_BOUND = new Color(Display.getDefault(), new RGB(26, 88, 0));
   private static final Color COLOR_UNBOUND = new Color(Display.getDefault(), new RGB(199, 83, 0));
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AgentTunnel t = (AgentTunnel)element;
      switch(columnIndex)
      {
         case TunnelManager.COL_AGENT_ID:
            return t.getAgentId().toString();
         case TunnelManager.COL_AGENT_VERSION:
            return t.getAgentVersion();
         case TunnelManager.COL_CHANNELS:
            return t.isBound() ? Integer.toString(t.getActiveChannelCount()) : "";
         case TunnelManager.COL_ID:
            return Integer.toString(t.getId());
         case TunnelManager.COL_IP_ADDRESS:
            return t.getAddress().getHostAddress();
         case TunnelManager.COL_NODE:
            return t.isBound() ? ConsoleSharedData.getSession().getObjectName(t.getNodeId()) : "";
         case TunnelManager.COL_PLATFORM:
            return t.getPlatformName();
         case TunnelManager.COL_STATE:
            return t.isBound() ? "Bound" : "Unbound";
         case TunnelManager.COL_SYSINFO:
            return t.getSystemInformation();
         case TunnelManager.COL_SYSNAME:
            return t.getSystemName();
         case TunnelManager.COL_HOSTNAME:
            return t.getHostname();
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return ((AgentTunnel)element).isBound() ? COLOR_BOUND : COLOR_UNBOUND;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
