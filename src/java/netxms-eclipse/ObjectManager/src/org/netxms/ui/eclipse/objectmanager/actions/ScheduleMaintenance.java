package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.handlers.HandlerUtil;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.dialogs.MaintanenceScheduleDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class ScheduleMaintenance extends AbstractHandler
{

   @Override
   public Object execute(ExecutionEvent event) throws ExecutionException
   {
      IWorkbenchWindow window = HandlerUtil.getActiveWorkbenchWindow(event);
      ISelection selection = window.getActivePage().getSelection();
      if ((selection == null) || !(selection instanceof IStructuredSelection) || selection.isEmpty())
         return null;
      
      final MaintanenceScheduleDialog dialog = new MaintanenceScheduleDialog(window.getShell());
      if (dialog.open() != Window.OK)
         return null;
      
      final Object[] objects = ((IStructuredSelection)selection).toArray();
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().SetObjectManagementState_JobTitle, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
               {
                  ScheduledTask taskStart = new ScheduledTask("Maintenance.Enter", "", "", dialog.getStartDate(), ScheduledTask.INTERNAL, ((AbstractObject)o).getObjectId());
                  ScheduledTask taskEnd = new ScheduledTask("Maintenance.Leave", "", "", dialog.getEndDate(), ScheduledTask.INTERNAL, ((AbstractObject)o).getObjectId());
                  session.addSchedule(taskStart);
                  session.addSchedule(taskEnd);                  
               }
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SetObjectManagementState_JobError;
         }
      }.start();
      return null;
   }

}
