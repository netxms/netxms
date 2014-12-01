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
package org.netxms.ui.eclipse.alarmviewer.views.helpers;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventInfo;
import org.netxms.ui.eclipse.alarmviewer.views.AlarmDetails;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Comparator for event tree
 */
public class EventTreeComparator extends ViewerComparator
{
   private final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object o1, Object o2)
   {
      TreeColumn sortColumn = ((TreeViewer)viewer).getTree().getSortColumn();
      if (sortColumn == null)
         return 0;
      
      EventInfo e1 = (EventInfo)o1;
      EventInfo e2 = (EventInfo)o2;
      
      int result;
      switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
      {
         case AlarmDetails.EV_COLUMN_MESSAGE:
            result = e1.getMessage().compareToIgnoreCase(e2.getMessage());
            break;
         case AlarmDetails.EV_COLUMN_NAME:
            result = e1.getName().compareToIgnoreCase(e2.getName());
            break;
         case AlarmDetails.EV_COLUMN_SEVERITY:
            result = e1.getSeverity() - e2.getSeverity();
            break;
         case AlarmDetails.EV_COLUMN_SOURCE:
            result = session.getObjectName(e1.getSourceObjectId()).compareToIgnoreCase(session.getObjectName(e2.getSourceObjectId()));
            break;
         case AlarmDetails.EV_COLUMN_TIMESTAMP:
            result = e1.getTimeStamp().compareTo(e2.getTimeStamp());
            break;
         default:
            result = 0;
            break;
      }
      return (((TreeViewer)viewer).getTree().getSortDirection() == SWT.UP) ? result : -result;
   }
}
