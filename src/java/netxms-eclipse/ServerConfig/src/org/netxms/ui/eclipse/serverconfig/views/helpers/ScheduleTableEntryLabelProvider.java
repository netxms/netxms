package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.serverconfig.views.ScheduledTaskView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class ScheduleTableEntryLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session;
   private WorkbenchLabelProvider wbLabelProvider;
   
   /**
    * Default constructor 
    */
   public ScheduleTableEntryLabelProvider()
   {
      session = (NXCSession)ConsoleSharedData.getSession();
      wbLabelProvider = new WorkbenchLabelProvider();
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case ScheduledTaskView.OBJECT:
            if(((ScheduledTask)element).getObjectId() == 0)
               return null;
            AbstractObject object = session.findObjectById(((ScheduledTask)element).getObjectId());
            return (object != null) ? wbLabelProvider.getImage(object) : null;
      }
      return null;
   }
   
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ScheduledTask task = (ScheduledTask)element;
      
      switch(columnIndex)
      {
         case ScheduledTaskView.SCHEDULE_ID:
            return Long.toString(task.getId());
         case ScheduledTaskView.CALLBACK_ID:
            return task.getScheduledTaskId();
         case ScheduledTaskView.OBJECT:
            if(task.getObjectId() == 0)
               return "";
            AbstractObject object = session.findObjectById(task.getObjectId());
            return (object != null) ? object.getObjectName() : ("[" + Long.toString(task.getObjectId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ScheduledTaskView.PARAMETERS:
            return task.getParameters();
         case ScheduledTaskView.EXECUTION_TIME:
            return task.getSchedule().isEmpty() ? task.getExecutionTime().toString() : task.getSchedule();
         case ScheduledTaskView.LAST_EXECUTION_TIME:
            return task.getLastExecutionTime().getTime() == 0 ? "" : task.getLastExecutionTime().toString();
         case ScheduledTaskView.STATUS:
            return task.getStatus();
         case ScheduledTaskView.OWNER:
            AbstractUserObject user = session.findUserDBObjectById(task.getOwner());
            return (user != null) ? user.getName() : ("[" + Long.toString(task.getOwner()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
      }
      return null;
   }

}
