/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.AgentTunnel;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Filter for tunnel manager view
 */
public class TunnelManagerFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString = null;
   private boolean hideNonProxy = false;
   private boolean hideNonUA = false;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      AgentTunnel t = (AgentTunnel)element;
      
      if (hideNonUA && !t.isUserAgentInstalled())
         return false;

      if (hideNonProxy && !t.isAgentProxy() && !t.isSnmpProxy() && !t.isSnmpTrapProxy() && !t.isSyslogProxy())
         return false;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      if (t.getAgentVersion().toLowerCase().contains(filterString))
         return true;

      if (t.isBound() ? Integer.toString(t.getActiveChannelCount()).toLowerCase().contains(filterString) : false)
         return true;
      
      if (Integer.toString(t.getId()).toLowerCase().contains(filterString))
         return true;
      
      if (t.getAddress().getHostAddress().toLowerCase().contains(filterString))
         return true;
      
      if (t.isBound() ? Registry.getSession().getObjectName(t.getNodeId()).toLowerCase().contains(filterString) : false)
         return true;
      
      if (t.getPlatformName().toLowerCase().contains(filterString))
         return true;
      
      if (t.isBound() ? "bound".contains(filterString) : "unbound".contains(filterString))
         return true;
      
      if (t.getSystemInformation().toLowerCase().contains(filterString))
         return true;
      
      if (t.getSystemName().toLowerCase().contains(filterString))
         return true;

      if (t.getHardwareIdAsText().toLowerCase().contains(filterString))
         return true;

      return false;
   }

   /**
    * Set filter string.
    * 
    * @param filterString new filter string
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }

   /**
    * Show/hide tunnels without proxy function
    * 
    * @param hide true to hide tunnels without proxy function
    */
   public void setHideNonProxy(boolean hide)
   {
      this.hideNonProxy = hide;
   }

   /**
    * Show/hide tunnels without user agent
    * 
    * @param hide true to hide tunnels without user agent
    */
   public void setHideNonUA(boolean hide)
   {
      this.hideNonUA = hide;
   }
}
