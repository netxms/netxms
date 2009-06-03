/**
 * 
 */
package org.netxms.ui.eclipse.policymanager.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCAgentPolicy;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class UninstallPolicy implements IObjectActionDelegate
{
	private Shell shell;
	private NXCObject currentObject;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		// Read custom root objects
		long[] rootObjects = null;
		Object value = NXMCSharedData.getInstance().getProperty("PolicyManager.rootObjects");
		if ((value != null) && (value instanceof long[]))
		{
			rootObjects = (long[])value;
		}
		
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(shell, rootObjects);
		if (dlg.open() == Window.OK)
		{
			new Job("Uninstall agent policy") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					try
					{
						NXCSession session = NXMCSharedData.getInstance().getSession();
						NXCObject[] nodeList = dlg.getAllCheckedObjects(NXCObject.OBJECT_NODE);
						for(int i = 0; i < nodeList.length; i++)
							session.uninstallAgentPolicy(currentObject.getObjectId(), nodeList[i].getObjectId());
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
			                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
			                    "Cannot uninstall agent policy: " + e.getMessage(), e);
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
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof TreeSelection)
		{
			currentObject = (NXCObject)((TreeSelection)selection).getFirstElement();
			action.setEnabled(currentObject instanceof NXCAgentPolicy);
		}
	}
}
