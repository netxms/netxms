/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ScheduledTask;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.reporting.widgets.ReportExecutionForm;
import org.netxms.nxmc.tools.ViewerElementUpdater;

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
         case ReportExecutionForm.COLUMN_SCHEDULE_EXEC_TIME:
            return task.getSchedule().isEmpty() ? DateFormatFactory.getDateTimeFormat().format(task.getExecutionTime()) : task.getSchedule();
         case ReportExecutionForm.COLUMN_SCHEDULE_OWNER:
            AbstractUserObject user = Registry.getSession().findUserDBObjectById(task.getOwner(), new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + task.getOwner() + "]");
         case ReportExecutionForm.COLUMN_SCHEDULE_LAST_EXEC_TIME:
            return (task.getLastExecutionTime().getTime() == 0) ? "" : DateFormatFactory.getDateTimeFormat().format(task.getLastExecutionTime());
         case ReportExecutionForm.COLUMN_SCHEDULE_STATUS:
            return task.getStatus();
      }
      return "<INTERNAL ERROR>";
   }
}
