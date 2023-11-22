/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.reporting.widgets.helpers;

import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.reporting.widgets.ReportExecutionForm;
import org.netxms.nxmc.tools.ViewerElementUpdater;

/**
 * Comparator for report results
 */
public class ReportResultComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      ReportResult r1 = (ReportResult)e1;
      ReportResult r2 = (ReportResult)e2;

      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case ReportExecutionForm.COLUMN_RESULT_EXEC_TIME:
            result = r1.getExecutionTime().compareTo(r2.getExecutionTime());
            break;
         case ReportExecutionForm.COLUMN_RESULT_STARTED_BY:
            result = getUserName(viewer, r1).compareToIgnoreCase(getUserName(viewer, r2));
            break;
         case ReportExecutionForm.COLUMN_RESULT_STATUS:
            result = r1.isSuccess() ? (r2.isSuccess() ? 0 : 1) : (r2.isSuccess() ? -1 : 0);
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }

   /**
    * Get user name from given result object
    *
    * @param viewer owning viewer
    * @param r report result
    * @return user name
    */
   private String getUserName(Viewer viewer, ReportResult r)
   {
      AbstractUserObject user = session.findUserDBObjectById(r.getUserId(), new ViewerElementUpdater((ColumnViewer)viewer, r));
      return (user != null) ? user.getName() : ("[" + r.getUserId() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
   }
}
