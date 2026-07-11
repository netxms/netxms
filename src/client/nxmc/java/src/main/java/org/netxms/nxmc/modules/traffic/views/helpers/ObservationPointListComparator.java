/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.traffic.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.ObservationPoint;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.traffic.views.TrafficObserverView;

/**
 * Comparator for observation point list
 */
public class ObservationPointListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final ObservationPoint p1 = (ObservationPoint)e1;
      final ObservationPoint p2 = (ObservationPoint)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

      int result;
      switch(column)
      {
         case TrafficObserverView.COLUMN_ID:
            result = Long.compare(p1.getObjectId(), p2.getObjectId());
            break;
         case TrafficObserverView.COLUMN_NAME:
            result = p1.getObjectName().compareToIgnoreCase(p2.getObjectName());
            break;
         case TrafficObserverView.COLUMN_EXTERNAL_ID:
            result = p1.getExternalId().compareToIgnoreCase(p2.getExternalId());
            break;
         case TrafficObserverView.COLUMN_TYPE:
            result = p1.getPointType().compareToIgnoreCase(p2.getPointType());
            break;
         case TrafficObserverView.COLUMN_STATE:
            result = Integer.compare(p1.getState(), p2.getState());
            break;
         case TrafficObserverView.COLUMN_PROVIDER_STATE:
            result = p1.getProviderState().compareToIgnoreCase(p2.getProviderState());
            break;
         case TrafficObserverView.COLUMN_SAMPLING_RATE:
            result = Integer.compare(p1.getSamplingRate(), p2.getSamplingRate());
            break;
         case TrafficObserverView.COLUMN_IN_SCOPE:
            result = Boolean.compare(p1.isInScope(), p2.isInScope());
            break;
         case TrafficObserverView.COLUMN_LAST_DISCOVERY:
            long t1 = (p1.getLastDiscoveryTime() != null) ? p1.getLastDiscoveryTime().getTime() : 0;
            long t2 = (p2.getLastDiscoveryTime() != null) ? p2.getLastDiscoveryTime().getTime() : 0;
            result = Long.compare(t1, t2);
            break;
         case TrafficObserverView.COLUMN_STATUS:
            result = p1.getStatus().compareTo(p2.getStatus());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
