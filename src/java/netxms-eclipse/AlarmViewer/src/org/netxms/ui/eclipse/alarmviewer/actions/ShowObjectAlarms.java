/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer.actions;

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.HandlerUtil;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.views.ObjectAlarmBrowser;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Handler for "show object alarms" command
 */
public class ShowObjectAlarms extends AbstractHandler
{

   @Override
   public Object execute(ExecutionEvent event) throws ExecutionException
   {
      IWorkbenchWindow window = HandlerUtil.getActiveWorkbenchWindow(event);
      ISelection selection = window.getActivePage().getSelection();
      if ((selection == null) || !(selection instanceof IStructuredSelection) || selection.isEmpty())
         return null;
      
      AbstractObject object = (AbstractObject)((IStructuredSelection)selection).getFirstElement();
      try
      {
         window.getActivePage().showView(ObjectAlarmBrowser.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), Messages.get().ShowObjectAlarms_Error, Messages.get().ShowObjectAlarms_ErrorOpeningView + e.getMessage());
      }
      
      return null;
   }
}
