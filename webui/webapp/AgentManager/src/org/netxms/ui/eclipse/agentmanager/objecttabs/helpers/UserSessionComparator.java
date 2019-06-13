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
package org.netxms.ui.eclipse.agentmanager.objecttabs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.TableRow;
import org.netxms.ui.eclipse.agentmanager.objecttabs.UserSessionsTab;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * User session comparator
 */
public class UserSessionComparator extends ViewerComparator
{
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;

      TableRow row1 = (TableRow)e1;
      TableRow row2 = (TableRow)e2;

      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
      {
         case UserSessionsTab.COLUMN_ID:
            result = row1.get(UserSessionsTab.COLUMN_ID).getValue().compareTo(row2.get(UserSessionsTab.COLUMN_ID).getValue());
            break;
         case UserSessionsTab.COLUMN_SESSION:
            result = row1.get(UserSessionsTab.COLUMN_SESSION).getValue()
                  .compareTo(row2.get(UserSessionsTab.COLUMN_SESSION).getValue());
            break;
         case UserSessionsTab.COLUMN_USER:
            result = row1.get(UserSessionsTab.COLUMN_USER).getValue().compareTo(row2.get(UserSessionsTab.COLUMN_USER).getValue());
            break;
         case UserSessionsTab.COLUMN_CLIENT:
            result = row1.get(UserSessionsTab.COLUMN_CLIENT).getValue()
                  .compareTo(row2.get(UserSessionsTab.COLUMN_CLIENT).getValue());
            break;
         case UserSessionsTab.COLUMN_STATE:
            result = row1.get(UserSessionsTab.COLUMN_STATE).getValue().compareTo(row2.get(UserSessionsTab.COLUMN_STATE).getValue());
            break;
         case UserSessionsTab.COLUMN_AGENT_TYPE:
            result = row1.get(UserSessionsTab.COLUMN_AGENT_TYPE).getValue()
                  .compareTo(row2.get(UserSessionsTab.COLUMN_AGENT_TYPE).getValue());
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }

}
