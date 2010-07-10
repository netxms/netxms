package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class BindObject implements IObjectActionDelegate
{
	private Shell shell;
	private ViewPart viewPart;
	private long parentId;

	/**
	 * @see IObjectActionDelegate#setActivePart(IAction, IWorkbenchPart)
	 */
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
		viewPart = (targetPart instanceof ViewPart) ? (ViewPart)targetPart : null;
	}

	/**
	 * @see IActionDelegate#run(IAction)
	 */
	public void run(IAction action)
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(shell, null, ObjectSelectionDialog.createNodeSelectionFilter());
		dlg.open();
		if (dlg.getReturnCode() == Window.OK)
		{
			final NXCSession session = NXMCSharedData.getInstance().getSession();
			new ConsoleJob("Bind object", viewPart, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot create object binding";
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					GenericObject[] objects = dlg.getSelectedObjects();
					for(int i = 0; i < objects.length; i++)
						session.bindObject(parentId, objects[i].getObjectId());
				}
			}.start();
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
			if ((obj instanceof ServiceRoot) || (obj instanceof Container))
			{
				action.setEnabled(true);
				parentId = ((GenericObject)obj).getObjectId();
			}
			else
			{
				action.setEnabled(false);
				parentId = 0;
			}
		}
		else
		{
			action.setEnabled(false);
			parentId = 0;
		}
	}
}
