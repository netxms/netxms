package org.netxms.nxmc.objectbrowser.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.*;
import org.netxms.nxmc.objectbrowser.ObjectSelectionDialog;

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
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(shell);
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
		if (selection instanceof TreeSelection)
		{
			Object obj = ((TreeSelection)selection).getFirstElement();
			action.setEnabled((obj instanceof NXCServiceRoot) || (obj instanceof NXCContainer));
		}
	}
}
