/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.ScheduledTasks;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.ViewerElementUpdater;
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
   private static final int RUNNING = 2;
   private static final int DISABLED = 3;

   private NXCSession session;
   private BaseObjectLabelProvider objectLabelProvider;
   private Image statusImages[];
   private SortableTableViewer viewer;

   /**
    * Default constructor
    * 
    * @param viewer
    */
   public ScheduledTaskLabelProvider(SortableTableViewer viewer)
   {
      session = Registry.getSession();
      objectLabelProvider = new BaseObjectLabelProvider();
      this.viewer = viewer;

      statusImages = new Image[4];
      statusImages[EXECUTED] = ResourceManager.getImage("icons/scheduler/completed.png");
      statusImages[PENDING] = ResourceManager.getImage("icons/scheduler/scheduled.png");
      statusImages[RUNNING] = ResourceManager.getImage("icons/scheduler/running.png");
      statusImages[DISABLED] = ResourceManager.getImage("icons/scheduler/disabled.png");
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
         case ScheduledTasks.SCHEDULE_ID:
            if (task.isDisabled())
               return statusImages[DISABLED];
            if ((task.getFlags() & ScheduledTask.RUNNING) != 0)
               return statusImages[RUNNING];
            if ((task.getFlags() & ScheduledTask.EXECUTED) != 0)
               return statusImages[EXECUTED];
            return statusImages[PENDING];
         case ScheduledTasks.OBJECT:
            if (task.getObjectId() == 0)
               return null;
            AbstractObject object = session.findObjectById(((ScheduledTask)element).getObjectId());
            return (object != null) ? objectLabelProvider.getImage(object) : null;
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
         case ScheduledTasks.SCHEDULE_ID:
            return Long.toString(task.getId());
         case ScheduledTasks.CALLBACK_ID:
            return task.getTaskHandlerId();
         case ScheduledTasks.OBJECT:
            return (task.getObjectId() == 0) ? "" : session.getObjectName(task.getObjectId());
         case ScheduledTasks.PARAMETERS:
            return task.getParameters();
         case ScheduledTasks.EXECUTION_TIME:
            return task.getSchedule().isEmpty() ? DateFormatFactory.getDateTimeFormat().format(task.getExecutionTime()) : task.getSchedule();
         case ScheduledTasks.EXECUTION_TIME_DESCRIPTION:
            if (task.getSchedule().isEmpty())
            {
               return String.format("Exactly at %s", DateFormatFactory.getDateTimeFormat().format(task.getExecutionTime()));
            }
            try
            {
               return CronExpressionDescriptor.getDescription(task.getSchedule());
            }
            catch(Exception e)
            {
               // CronExpressionDescriptor can throw exception if it cannot parse expression
               return "";
            }
         case ScheduledTasks.LAST_EXECUTION_TIME:
            return (task.getLastExecutionTime().getTime() == 0) ? "" : DateFormatFactory.getDateTimeFormat().format(task.getLastExecutionTime());
         case ScheduledTasks.STATUS:
            return task.getStatus();
         case ScheduledTasks.MANAGMENT_STATE:
            return task.isDisabled() ? "Disabled" : "Enabled";
         case ScheduledTasks.OWNER:
            if (task.isSystem())
               return "system";
            AbstractUserObject user = session.findUserDBObjectById(task.getOwner(), new ViewerElementUpdater(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(task.getOwner()) + "]");
         case ScheduledTasks.COMMENTS:
            return task.getComments();
         case ScheduledTasks.TIMER_KEY:
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

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      for(Image i : statusImages)
         i.dispose();
      super.dispose();
   }
}
