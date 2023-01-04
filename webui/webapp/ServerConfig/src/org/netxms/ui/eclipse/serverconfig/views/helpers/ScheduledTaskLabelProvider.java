/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.ViewerElementUpdater;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.views.ScheduledTaskView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import it.burning.cron.CronExpressionDescriptor;

/**
 * Label provider for scheduled task list
 */
public class ScheduledTaskLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color COLOR_DISABLED = new Color(Display.getDefault(), new RGB(126, 137, 185));
   private static final Color COLOR_SYSTEM = new Color(Display.getDefault(), new RGB(196, 170, 94));

   private static final int EXECUTED = 0;
   private static final int PENDING = 1;
   private static final int DISABLED = 2;

   private NXCSession session;
   private WorkbenchLabelProvider wbLabelProvider;
   private Image statusImages[];
   private SortableTableViewer viewer;

   /**
    * Default constructor
    * 
    * @param viewer
    */
   public ScheduledTaskLabelProvider(SortableTableViewer viewer)
   {
      session = ConsoleSharedData.getSession();
      wbLabelProvider = new WorkbenchLabelProvider();
      this.viewer = viewer;

      statusImages = new Image[3];
      statusImages[EXECUTED] = Activator.getImageDescriptor("icons/active.gif").createImage(); //$NON-NLS-1$
      statusImages[PENDING] = Activator.getImageDescriptor("icons/pending.gif").createImage(); //$NON-NLS-1$
      statusImages[DISABLED] = Activator.getImageDescriptor("icons/inactive.gif").createImage(); //$NON-NLS-1$
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      ScheduledTask task = (ScheduledTask)element;

      switch(columnIndex)
      {
         case ScheduledTaskView.SCHEDULE_ID:
            if (task.isDisabled())
               return statusImages[DISABLED];
            if ((task.getFlags() & ScheduledTask.EXECUTED) != 0 || (task.getFlags() & ScheduledTask.RUNNING) != 0)
               return statusImages[EXECUTED];
            return statusImages[PENDING];
         case ScheduledTaskView.OBJECT:
            if (task.getObjectId() == 0)
               return null;
            AbstractObject object = session.findObjectById(((ScheduledTask)element).getObjectId());
            return (object != null) ? wbLabelProvider.getImage(object) : null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ScheduledTask task = (ScheduledTask)element;

      switch(columnIndex)
      {
         case ScheduledTaskView.SCHEDULE_ID:
            return Long.toString(task.getId());
         case ScheduledTaskView.CALLBACK_ID:
            return task.getTaskHandlerId();
         case ScheduledTaskView.OBJECT:
            return (task.getObjectId() == 0) ? "" : session.getObjectName(task.getObjectId());
         case ScheduledTaskView.PARAMETERS:
            return task.getParameters();
         case ScheduledTaskView.EXECUTION_TIME:
            if (task.getSchedule().isEmpty())
               return RegionalSettings.getDateTimeFormat().format(task.getExecutionTime());
            else
               return task.getSchedule();
         case ScheduledTaskView.EXECUTION_TIME_DESCRIPTION:
            if (task.getSchedule().isEmpty())
               return String.format("Exactly at %s", RegionalSettings.getDateTimeFormat().format(task.getExecutionTime()));
            else
               return CronExpressionDescriptor.getDescription(task.getSchedule());
         case ScheduledTaskView.LAST_EXECUTION_TIME:
            return (task.getLastExecutionTime().getTime() == 0) ? "" : RegionalSettings.getDateTimeFormat().format(task.getLastExecutionTime());
         case ScheduledTaskView.STATUS:
            return task.getStatus();
         case ScheduledTaskView.MANAGMENT_STATE:
            return task.isDisabled() ? "Disabled" : "Enabled";
         case ScheduledTaskView.OWNER:
            if (task.isSystem())
               return "system";
            AbstractUserObject user = session.findUserDBObjectById(task.getOwner(), new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(task.getOwner()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ScheduledTaskView.COMMENTS:
            return task.getComments();
         case ScheduledTaskView.TIMER_KEY:
            return task.getKey();
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((ScheduledTask)element).isDisabled())
         return COLOR_DISABLED;
      if (((ScheduledTask)element).isSystem())
         return COLOR_SYSTEM;
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
