package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.serverconfig.views.ScheduledTaskView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class ScheduleTableEntryComparator extends ViewerComparator
{
   /* (non-Javadoc)
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
         case ScheduledTaskView.SCHEDULE_ID:
            result = (int)(task1.getId() - task2.getId());
            break;
         case ScheduledTaskView.CALLBACK_ID:
            result = task1.getScheduledTaskId().compareToIgnoreCase(task2.getScheduledTaskId());
            break;
         case ScheduledTaskView.OBJECT:
            AbstractObject obj1 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(task1.getObjectId());
            AbstractObject obj2 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(task2.getObjectId());
            String name1 = (obj1 != null) ? obj1.getObjectName() : "Unknown";
            String name2 = (obj2 != null) ? obj2.getObjectName() : "Unknown";
            result = name1.compareToIgnoreCase(name2);
            break;
         case ScheduledTaskView.PARAMETERS:
            result = task1.getParameters().compareToIgnoreCase(task2.getParameters());
            break;
         case ScheduledTaskView.EXECUTION_TIME:
            if(task1.getSchedule().isEmpty())
            {
               if(task2.getSchedule().isEmpty())
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
            
            if(task2.getSchedule().isEmpty())
            {
               result = -1;
            }

            result = task1.getSchedule().compareToIgnoreCase(task2.getSchedule());
            break;
         case ScheduledTaskView.LAST_EXECUTION_TIME:
            result = task1.getLastExecutionTime().compareTo(task2.getLastExecutionTime());
            break;
         case ScheduledTaskView.STATUS:
            result = task1.getStatus().compareToIgnoreCase(task2.getStatus());
            break;
         case ScheduledTaskView.OWNER:
            String user1 = "";
            String user2 = "";
            if((task1.getFlags() & ScheduledTask.INTERNAL)>0)
               user1= "Internal";
            else
               user1 = ((NXCSession)ConsoleSharedData.getSession()).findUserDBObjectById(task1.getOwner()).getName();
            
            if((task2.getFlags() & ScheduledTask.INTERNAL)>0)
               user2= "Internal";
            else
               user2 = ((NXCSession)ConsoleSharedData.getSession()).findUserDBObjectById(task2.getOwner()).getName();
            
            result = user1.compareTo(user2);
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
