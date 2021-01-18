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
package org.netxms.ui.eclipse.reporter.widgets.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ScheduledTask;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.UserRefreshRunnable;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for reporting schedules list
 */
public class ScheduleLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private TableViewer viewer;
   
   /**
    * Constructor
    */
   public ScheduleLabelProvider(TableViewer viewer)
   {
      this.viewer = viewer;
   }
   
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      final ScheduledTask task = ((ReportingJob)element).getTask();
      switch(columnIndex)
      {
         case ReportExecutionForm.SCHEDULED_EXECUTION_TIME:
            return task.getSchedule().isEmpty() ? RegionalSettings.getDateTimeFormat().format(task.getExecutionTime()) : task.getSchedule();
         case ReportExecutionForm.SCHEDULE_OWNER:
            AbstractUserObject user = ConsoleSharedData.getSession().findUserDBObjectById(task.getOwner(), new UserRefreshRunnable(viewer, element));
            return (user != null) ? user.getName() : ("[" + task.getOwner() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ReportExecutionForm.LAST_EXECUTION_TIME:
            return (task.getLastExecutionTime().getTime() == 0) ? "" : RegionalSettings.getDateTimeFormat().format(task.getLastExecutionTime());
         case ReportExecutionForm.STATUS:
            return task.getStatus();
      }

      return "<INTERNAL ERROR>"; //$NON-NLS-1$
   }
}
