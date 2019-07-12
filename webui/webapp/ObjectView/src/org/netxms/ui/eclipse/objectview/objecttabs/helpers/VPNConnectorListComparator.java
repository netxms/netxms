/**
 * NetXMS - open source network management system
 * Copyright (C) 2019 Raden Solutions
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
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.VPNConnector;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.VPNTab;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * VPN Connector comparator
 */
public class VPNConnectorListComparator extends ViewerComparator
{
   VPNConnectorListLabelProvider lp = null;
   
   public VPNConnectorListComparator(VPNConnectorListLabelProvider lp) 
   {
      this.lp = lp;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final VPNConnector vpn1 = (VPNConnector)e1;
      final VPNConnector vpn2 = (VPNConnector)e2;
      final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
      
      int result = 0;
      switch(column)
      {
         case VPNTab.COLUMN_ID:
            result = Long.compare(vpn1.getObjectId(), vpn2.getObjectId());
            break;
         case VPNTab.COLUMN_NAME:
            result = vpn1.getObjectName().compareTo(vpn2.getObjectName());
            break;
         case VPNTab.COLUMN_STATUS:
            result = StatusDisplayInfo.getStatusText(vpn1.getStatus()).compareToIgnoreCase(StatusDisplayInfo.getStatusText(vpn2.getStatus()));
            break;
         case VPNTab.COLUMN_PEER_GATEWAY:
            result = lp.getPeerName(vpn1).compareTo(lp.getPeerName(vpn2));
            break;
         case VPNTab.COLUMN_LOCAL_SUBNETS:
            result = lp.getSubnetsAsString(vpn1.getLocalSubnets()).compareTo(lp.getSubnetsAsString(vpn2.getLocalSubnets()));
            break;
         case VPNTab.COLUMN_REMOTE_SUBNETS:
            result = lp.getSubnetsAsString(vpn1.getRemoteSubnets()).compareTo(lp.getSubnetsAsString(vpn2.getRemoteSubnets()));
            break;
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
