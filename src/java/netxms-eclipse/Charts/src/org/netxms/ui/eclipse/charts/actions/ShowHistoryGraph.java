/**
 * 
 */
package org.netxms.ui.eclipse.charts.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.charts.views.HistoryGraph;

/**
 * @author Victor
 *
 */
public class ShowHistoryGraph implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private Object[] currentSelection = null;
	private long uniqueId = 0;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		window = targetPart.getSite().getWorkbenchWindow();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		if (currentSelection != null)
		{
			String id = Long.toString(uniqueId++);
			for(int i = 0; i < currentSelection.length; i++)
				if (currentSelection[i] instanceof DciValue)
					id += "&" + ((DciValue)currentSelection[i]).getId() + "@" + ((DciValue)currentSelection[i]).getNodeId();
			
			try
			{
				window.getActivePage().showView(HistoryGraph.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
			}
			catch(PartInitException e)
			{
				MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
			catch(IllegalArgumentException e)
			{
				MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) &&
				(((IStructuredSelection)selection).getFirstElement() instanceof DciValue))
		{
			currentSelection = ((IStructuredSelection)selection).toArray();
		}
		else
		{
			currentSelection = null;
		}
		
		action.setEnabled((currentSelection != null) && (currentSelection.length > 0));
	}
}
