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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.objects.VPNConnector;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

public class VPNConnectorTabFilter extends NodeComponentTabFilter
{
   private String filterString = null;
   private VPNConnectorListLabelProvider lp;
   
   public VPNConnectorTabFilter(VPNConnectorListLabelProvider lp)
   {
      this.lp = lp;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      final VPNConnector vpn = (VPNConnector)element;
      
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      boolean matched = false;
      
      if (Long.toString(vpn.getObjectId()).contains(filterString))
         matched = true;
      else if (vpn.getObjectName().contains(filterString))
         matched = true;
      else if (lp.getPeerName(vpn).contains(filterString))
         matched = true;
      else if (StatusDisplayInfo.getStatusText(vpn.getStatus()).contains(filterString))
         matched = true;
      else if (lp.getSubnetsAsString(vpn.getLocalSubnets()).contains(filterString))
         matched = true;
      else if (lp.getSubnetsAsString(vpn.getRemoteSubnets()).contains(filterString))
         matched = true;
      
      return matched;
   }
}
