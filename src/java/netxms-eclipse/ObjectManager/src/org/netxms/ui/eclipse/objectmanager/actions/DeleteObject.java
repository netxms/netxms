package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class DeleteObject implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private NXCObject object;

	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		window = targetPart.getSite().getWorkbenchWindow();
	}

	@Override
	public void run(IAction action)
	{
		boolean confirmed = MessageDialog.openConfirm(window.getShell(), "Confirm Delete",
				"Are you sure you want to delete '" + object.getObjectName() + "'?");

		if (!confirmed)
		{
			return;
		}

		final NXCSession session = NXMCSharedData.getInstance().getSession();

		try
		{
			session.deleteObject(object.getObjectId());
		}
		catch(Exception e)
		{
			MessageDialog.openError(window.getShell(), "Error", e.getMessage());
		}
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof TreeSelection)
		{
			final Object obj = ((TreeSelection)selection).getFirstElement();
			if (obj instanceof NXCObject)
			{
				object = (NXCObject)obj;
			}
			else
			{
				object = null;
			}
		}

		action.setEnabled(object != null);
	}
}
