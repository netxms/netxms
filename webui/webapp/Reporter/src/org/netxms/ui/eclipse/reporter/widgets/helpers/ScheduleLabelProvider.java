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

import java.text.SimpleDateFormat;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.reporting.ReportingJob;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.reporter.Messages;
import org.netxms.ui.eclipse.reporter.widgets.ReportExecutionForm;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for reporting schedules list
 */
public class ScheduleLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private String[] dayOfWeek = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$ //$NON-NLS-7$

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      final ReportingJob job = (ReportingJob)element;
      switch(columnIndex)
      {
         case ReportExecutionForm.SCHEDULE_START_TIME:
            SimpleDateFormat timeFormat = new SimpleDateFormat("HH:mm"); //$NON-NLS-1$
            String HHmm = timeFormat.format(job.getStartTime().getTime() / 1000);

            switch(job.getType())
            {
               case ReportingJob.TYPE_ONCE:
                  return RegionalSettings.getDateTimeFormat().format(job.getStartTime().getTime() / 1000);
               case ReportingJob.TYPE_DAILY:
                  return HHmm;
               case ReportingJob.TYPE_MONTHLY:
                  String result = ""; //$NON-NLS-1$
                  for(int i = 0; i < 31; i++)
                  {
                     if (((job.getDaysOfMonth() >> i) & 0x01) != 0)
                        result = String.valueOf(31 - i) + (result.length() > 0 ? "," : "") + result; //$NON-NLS-1$ //$NON-NLS-2$
                  }
                  return HHmm + " - " + result; //$NON-NLS-1$
               case ReportingJob.TYPE_WEEKLY:
                  String result1 = ""; //$NON-NLS-1$
                  for(int i = 0; i < 7; i++)
                  {
                     if (((job.getDaysOfWeek() >> i) & 0x01) != 0)
                        result1 = dayOfWeek[7 - (i + 1)] + (result1.length() > 0 ? "," : "") + result1; //$NON-NLS-1$ //$NON-NLS-2$
                  }
                  return HHmm + " - " + result1; //$NON-NLS-1$
               default:
                  return Messages.get().ScheduleLabelProvider_Error;
            }
         case ReportExecutionForm.SCHEDULE_TYPE:
            switch(job.getType())
            {
               case ReportingJob.TYPE_ONCE:
                  return Messages.get().ScheduleLabelProvider_Once;
               case ReportingJob.TYPE_DAILY:
                  return Messages.get().ScheduleLabelProvider_Daily;
               case ReportingJob.TYPE_MONTHLY:
                  return Messages.get().ScheduleLabelProvider_Monthly;
               case ReportingJob.TYPE_WEEKLY:
                  return Messages.get().ScheduleLabelProvider_Weekly;
               default:
                  return Messages.get().ScheduleLabelProvider_Error;
            }
         case ReportExecutionForm.SCHEDULE_OWNER:
            AbstractUserObject user = ((NXCSession)ConsoleSharedData.getSession()).findUserDBObjectById(job.getUserId());
            return (user != null) ? user.getName() : ("[" + job.getUserId() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ReportExecutionForm.SCHEDULE_COMMENTS:
            return job.getComments();
      }

      return "<INTERNAL ERROR>"; //$NON-NLS-1$
   }
}
