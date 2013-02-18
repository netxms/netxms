package org.netxms.ui.eclipse.imagelibrary.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PartInitException;
import org.netxms.ui.eclipse.imagelibrary.views.ImageLibrary;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

public class OpenLibraryManager implements IWorkbenchWindowActionDelegate
{

	private IWorkbenchWindow window;

	@Override
	public void run(IAction action)
	{
		if (window != null)
		{
			try
			{
				window.getActivePage().showView(ImageLibrary.ID);
			}
			catch(PartInitException e)
			{
				MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
		}
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
	}

	@Override
	public void dispose()
	{
	}

	@Override
	public void init(IWorkbenchWindow window)
	{
		this.window = window;
	}

}
