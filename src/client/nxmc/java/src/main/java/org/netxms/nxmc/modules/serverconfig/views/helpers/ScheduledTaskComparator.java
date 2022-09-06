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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.serverconfig.views.ScheduledTasks;

/**
 * Comparator for scheduled task list
 */
public class ScheduledTaskComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;

      ScheduledTask task1 = (ScheduledTask)e1;
      ScheduledTask task2 = (ScheduledTask)e2;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case ScheduledTasks.SCHEDULE_ID:
            result = (int)(task1.getId() - task2.getId());
            break;
         case ScheduledTasks.CALLBACK_ID:
            result = task1.getTaskHandlerId().compareToIgnoreCase(task2.getTaskHandlerId());
            break;
         case ScheduledTasks.OBJECT:
            AbstractObject obj1 = session.findObjectById(task1.getObjectId());
            AbstractObject obj2 = session.findObjectById(task2.getObjectId());
            String name1 = (obj1 != null) ? obj1.getObjectName() : "";
            String name2 = (obj2 != null) ? obj2.getObjectName() : "";
            result = name1.compareToIgnoreCase(name2);
            break;
         case ScheduledTasks.PARAMETERS:
            result = task1.getParameters().compareToIgnoreCase(task2.getParameters());
            break;
         case ScheduledTasks.EXECUTION_TIME:
            if (task1.getSchedule().isEmpty())
            {
               if (task2.getSchedule().isEmpty())
               {
                  result = task1.getExecutionTime().compareTo(task2.getExecutionTime());
                  break;
               }
               else
               {
                  result = 1;
                  break;
               }
            }

            if (task2.getSchedule().isEmpty())
            {
               result = -1;
            }

            result = task1.getSchedule().compareToIgnoreCase(task2.getSchedule());
            break;
         case ScheduledTasks.LAST_EXECUTION_TIME:
            result = task1.getLastExecutionTime().compareTo(task2.getLastExecutionTime());
            break;
         case ScheduledTasks.STATUS:
            result = task1.getStatus().compareToIgnoreCase(task2.getStatus());
            break;
         case ScheduledTasks.OWNER:
            String user1 = "";
            String user2 = "";
            if (task1.isSystem())
            {
               user1 = "system";
            }
            else
            {
               AbstractUserObject user = session.findUserDBObjectById(task1.getOwner(), null);
               user1 = user != null ? user.getName() : ("[" + Long.toString(task1.getOwner()) + "]");
            }

            if (task2.isSystem())
            {
               user2 = "system";
            }
            else
            {
               AbstractUserObject user = session.findUserDBObjectById(task2.getOwner(), null);
               user2 = user != null ? user.getName() : ("[" + Long.toString(task1.getOwner()) + "]");
            }

            result = user1.compareTo(user2);
            break;
         case ScheduledTasks.COMMENTS:
            result = task1.getComments().compareToIgnoreCase(task2.getComments());
            break;
         case ScheduledTasks.TIMER_KEY:
            result = task1.getKey().compareToIgnoreCase(task2.getKey());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
