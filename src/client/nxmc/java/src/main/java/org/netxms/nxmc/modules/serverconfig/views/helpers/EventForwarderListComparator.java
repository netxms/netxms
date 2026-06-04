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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.EventForwarder;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.serverconfig.views.EventForwarders;

/**
 * Comparator for event forwarder list
 */
public class EventForwarderListComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;

      EventForwarder f1 = (EventForwarder)e1;
      EventForwarder f2 = (EventForwarder)e2;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case EventForwarders.COLUMN_DRIVER:
            result = f1.getDriverName().compareToIgnoreCase(f2.getDriverName());
            break;
         case EventForwarders.COLUMN_DESCRIPTION:
            result = f1.getDescription().compareToIgnoreCase(f2.getDescription());
            break;
         case EventForwarders.COLUMN_ERROR_MESSAGE:
            result = f1.getErrorMessage().compareToIgnoreCase(f2.getErrorMessage());
            break;
         case EventForwarders.COLUMN_DROPPED_EVENTS:
            result = Integer.compare(f1.getDroppedCount(), f2.getDroppedCount());
            break;
         case EventForwarders.COLUMN_FAILED_EVENTS:
            result = Integer.compare(f1.getFailureCount(), f2.getFailureCount());
            break;
         case EventForwarders.COLUMN_LAST_STATUS:
            result = f1.getSendStatus() - f2.getSendStatus();
            break;
         case EventForwarders.COLUMN_NAME:
            result = f1.getName().compareToIgnoreCase(f2.getName());
            break;
         case EventForwarders.COLUMN_QUEUE_SIZE:
            result = Integer.compare(f1.getQueueSize(), f2.getQueueSize());
            break;
         case EventForwarders.COLUMN_TOTAL_EVENTS:
            result = Integer.compare(f1.getMessageCount(), f2.getMessageCount());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
