/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.AgentTunnel;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class TunnelManagerFilter extends ViewerFilter
{
   private String filterString = null;

   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      AgentTunnel t = (AgentTunnel)element;
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      else if (t.getAgentVersion().toLowerCase().contains(filterString))
         return true;
      else if (t.isBound() ? Integer.toString(t.getActiveChannelCount()).toLowerCase().contains(filterString) : false)
         return true;
      else if (Integer.toString(t.getId()).toLowerCase().contains(filterString))
         return true;
      else if (t.getAddress().getHostAddress().toLowerCase().contains(filterString))
         return true;
      else if (t.isBound() ? ConsoleSharedData.getSession().getObjectName(t.getNodeId()).toLowerCase().contains(filterString) : false)
         return true;
      else if (t.getPlatformName().toLowerCase().contains(filterString))
         return true;
      else if (t.isBound() ? "bound".contains(filterString) : "unbound".contains(filterString))
         return true;
      else if (t.getSystemInformation().toLowerCase().contains(filterString))
         return true;
      else if (t.getSystemName().toLowerCase().contains(filterString))
         return true;
      return false;
   }
   
   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }
}
