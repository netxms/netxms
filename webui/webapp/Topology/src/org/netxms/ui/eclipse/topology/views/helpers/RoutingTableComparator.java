/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.topology.Route;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.topology.views.RoutingTableView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for routing table list
 */
public class RoutingTableComparator extends ViewerComparator
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      Route r1 = (Route)e1;
      Route r2 = (Route)e2;
      
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case RoutingTableView.COLUMN_DESTINATION:
            result = ComparatorHelper.compareInetAddresses(r1.getDestination(), r2.getDestination());
            if (result == 0)
            {
               result = r1.getPrefixLength() - r2.getPrefixLength();
            }
            break;
         case RoutingTableView.COLUMN_NEXT_HOP:
            result = ComparatorHelper.compareInetAddresses(r1.getNextHop(), r2.getNextHop());
            break;
         case RoutingTableView.COLUMN_INTERFACE:
            result = r1.getIfName().compareToIgnoreCase(r2.getIfName());
            break;
         case RoutingTableView.COLUMN_TYPE:
            result = r1.getType() - r2.getType();
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
   }
}
