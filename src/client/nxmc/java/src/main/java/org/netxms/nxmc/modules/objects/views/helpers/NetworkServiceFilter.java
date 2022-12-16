/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 RadenSolutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Filter for network service view
 */
public class NetworkServiceFilter extends NodeSubObjectFilter
{
   private NetworkServiceListLabelProvider lp;
   
   public NetworkServiceFilter(NetworkServiceListLabelProvider lp)
   {
      this.lp = lp;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      final NetworkService ns = (NetworkService)element;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      boolean matched = false;
      
      if (Long.toString(ns.getObjectId()).contains(filterString))
         matched = true;
      else if (ns.getObjectName().contains(filterString))
         matched = true;
      else if (StatusDisplayInfo.getStatusText(ns.getStatus()).contains(filterString))
         matched = true;
      else if (lp.types[ns.getServiceType()].contains(filterString))
         matched = true;
      else if (ns.getIpAddress().getHostAddress().contains(filterString))
         matched = true;
      else if (Integer.toString(ns.getProtocol()).contains(filterString))
         matched = true;
      else if (Integer.toString(ns.getPort()).contains(filterString))
         matched = true;
      else if (ns.getRequest().contains(filterString))
         matched = true;
      else if (ns.getResponse().contains(filterString))
         matched = true;
      else if (lp.getPollerName(ns).contains(filterString))
         matched = true;
      else if (Integer.toString(ns.getPollCount()).contains(filterString))
         matched = true;
      
      return matched;
   }
}
