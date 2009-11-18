package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;

public class BindObject implements IObjectActionDelegate
{
	private Shell shell;

	/**
	 * Constructor
	 */
	public BindObject()
	{
	}

	/**
	 * @see IObjectActionDelegate#setActivePart(IAction, IWorkbenchPart)
	 */
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
	}

	/**
	 * @see IActionDelegate#run(IAction)
	 */
	public void run(IAction action)
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(shell, null, ObjectSelectionDialog.createNodeSelectionFilter());
		dlg.open();
		if (dlg.getReturnCode() == Window.OK)
		{
			MessageDialog.openInformation(shell, "Bind", "OK pressed");
		}
	}

	/**
	 * @see IActionDelegate#selectionChanged(IAction, ISelection)
	 */
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
		{
			Object obj = ((IStructuredSelection)selection).getFirstElement();
			action.setEnabled((obj instanceof ServiceRoot) || (obj instanceof Container));
		}
	}
}
