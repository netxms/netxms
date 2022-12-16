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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.NetworkService;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.NetworkServiceView;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * VPN Connector comparator
 */
public class NetworkServiceListComparator extends ViewerComparator
{
   NetworkServiceListLabelProvider lp = null;
   
   public NetworkServiceListComparator(NetworkServiceListLabelProvider lp) 
   {
      this.lp = lp;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final NetworkService ns1 = (NetworkService)e1;
      final NetworkService ns2 = (NetworkService)e2;
      final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
      
      int result = 0;
      switch(column)
      {
         case NetworkServiceView.COLUMN_ID:
            result = Long.compare(ns1.getObjectId(), ns2.getObjectId());
            break;
         case NetworkServiceView.COLUMN_NAME:
            result = ns1.getObjectName().compareTo(ns2.getObjectName());
            break;
         case NetworkServiceView.COLUMN_STATUS:
            result = StatusDisplayInfo.getStatusText(ns1.getStatus()).compareToIgnoreCase(StatusDisplayInfo.getStatusText(ns2.getStatus()));
            break;
         case NetworkServiceView.COLUMN_SERVICE_TYPE:
            result = lp.types[ns1.getServiceType()].compareTo(lp.types[ns2.getServiceType()]);
            break;
         case NetworkServiceView.COLUMN_ADDRESS:
            result = ns1.getIpAddress().getHostAddress().compareTo(ns2.getIpAddress().getHostAddress());
            break;
         case NetworkServiceView.COLUMN_PORT:
            result = ns1.getPort() - ns2.getPort();
            break;
         case NetworkServiceView.COLUMN_REQUEST:
            result = ns1.getRequest().compareTo(ns2.getRequest());
            break;
         case NetworkServiceView.COLUMN_RESPONSE:
            result = ns1.getResponse().compareTo(ns2.getResponse());
            break;
         case NetworkServiceView.COLUMN_POLLER_NODE:
            result = lp.getPollerName(ns1).compareTo(lp.getPollerName(ns2));
            break;
         case NetworkServiceView.COLUMN_POLL_COUNT:
            result = ns1.getPollCount() - ns2.getPollCount();
            break;
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
