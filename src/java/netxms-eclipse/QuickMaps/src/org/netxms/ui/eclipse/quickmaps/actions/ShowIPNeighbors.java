/**
 * 
 */
package org.netxms.ui.eclipse.quickmaps.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.NXCNode;
import org.netxms.ui.eclipse.quickmaps.views.IPNeighbors;

/**
 * @author Victor
 *
 */
public class ShowIPNeighbors implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private NXCNode node;

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
		if (node != null)
		{
			try
			{
				window.getActivePage().showView(IPNeighbors.ID, Long.toString(node.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
			}
			catch(PartInitException e)
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
		Object obj;
		if ((selection instanceof TreeSelection) &&
			 ((obj = ((TreeSelection)selection).getFirstElement()) instanceof NXCNode))
		{
			node = (NXCNode)obj;
		}
		else
		{
			node = null;
		}
		action.setEnabled(node != null);
	}
}
