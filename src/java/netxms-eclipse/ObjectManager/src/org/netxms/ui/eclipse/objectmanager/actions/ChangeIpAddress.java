package org.netxms.ui.eclipse.objectmanager.actions;

import java.io.IOException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCException;
import org.netxms.client.NXCNode;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.dialogs.EnterIpAddressDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class ChangeIpAddress implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private NXCNode node;

	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		window = targetPart.getSite().getWorkbenchWindow();
	}

	@Override
	public void run(IAction action)
	{
		final EnterIpAddressDialog dlg = new EnterIpAddressDialog(window.getShell());
		if (dlg.open() != Window.OK)
			return;

		new Job("Change IP address for node " + node.getObjectName()) {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				final NXCSession session = NXMCSharedData.getInstance().getSession();
				try
				{
					session.changeNodeIpAddress(node.getObjectId(), dlg.getIpAddress());
					status = Status.OK_STATUS;
				}
				catch(NXCException e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, e.getErrorCode(),
								           "Cannot change IP address for node " + node.getObjectName() + ": " + e.getMessage(), null);
				}
				catch(IOException e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 0,
					                    "Cannot change IP address for node " + node.getObjectName() + ": I/O error (" + e.getMessage() + ")", null);
				}
				return status;
			}
		}.schedule();
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof TreeSelection)
		{
			final Object obj = ((TreeSelection)selection).getFirstElement();
			if (obj instanceof NXCNode)
			{
				node = (NXCNode)obj;
			}
			else
			{
				node = null;
			}
		}

		action.setEnabled(node != null);
	}
}
