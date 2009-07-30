package org.netxms.ui.eclipse.objectmanager.actions;

import java.io.IOException;
import java.util.Iterator;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public abstract class MultipleObjectAction implements IObjectActionDelegate
{
	private IStructuredSelection selection;
	private IWorkbenchWindow window;
	
	/**
	 * Returns job description (like "Deleting object ").
	 * 
	 * @return Job description
	 */
	abstract protected String jobDescription();
	
	/**
	 * Returns error message prefix (like "Cannot delete object ")
	 * 
	 * @return Error message prefix
	 */
	abstract protected String errorPrefix();
	
	/**
	 * Run action on given object.
	 * 
	 * @throws Exception
	 */
	abstract protected void runObjectAction(final NXCSession session, final NXCObject object) throws Exception;
	
	/**
	 * Hook for possible confirmation. Default implementation always returns true.
	 * 
	 * @return true if action confirmed by user
	 */
	protected boolean confirm()
	{
		return true;
	}

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
	@SuppressWarnings("unchecked")
	@Override
	public final void run(IAction action)
	{
		if (selection == null)
			return;
		
		if (!confirm())
			return;
		
		Iterator<NXCObject> it = selection.iterator();
		while(it.hasNext())
		{
			final NXCObject object = it.next();
			new Job(jobDescription() + " " + object.getObjectName() + " [" + object.getObjectId() + "]") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					final NXCSession session = NXMCSharedData.getInstance().getSession();
					try
					{
						runObjectAction(session, object);
						status = Status.OK_STATUS;
					}
					catch(NXCException e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, e.getErrorCode(),
									           errorPrefix() + " " + object.getObjectName() + ": " + e.getMessage(), null);
					}
					catch(IOException e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 0,
									           errorPrefix() + " " + object.getObjectName() + ": I/O error (" + e.getMessage() + ")", null);
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 0,
						                    errorPrefix() + " " + object.getObjectName() + ": EXCEPTION (" + e.getMessage() + ")", null);
					}
					return status;
				}
			}.schedule();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public final void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
		{
			this.selection = (IStructuredSelection)selection;
			action.setEnabled(this.selection.size() > 0);
		}
		else
		{
			action.setEnabled(false);
		}
	}

	/**
	 * @return the selection
	 */
	protected IStructuredSelection getSelection()
	{
		return selection;
	}

	/**
	 * @return the window
	 */
	protected IWorkbenchWindow getWindow()
	{
		return window;
	}
}
