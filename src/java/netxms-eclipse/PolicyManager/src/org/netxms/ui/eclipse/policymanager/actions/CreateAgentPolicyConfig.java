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
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.PolicyGroup;
import org.netxms.client.objects.PolicyRoot;
import org.netxms.ui.eclipse.objectbrowser.dialogs.CreateObjectDialog;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * @author Victor
 *
 */
public class CreateAgentPolicyConfig implements IObjectActionDelegate
{
	private Shell shell;
	private GenericObject currentObject;
	
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
		final CreateObjectDialog dlg = new CreateObjectDialog(shell, "Agent Policy (Configuration File)");
		if (dlg.open() == Window.OK)
		{
			new Job("Create agent policy") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					try
					{
						NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_AGENTPOLICY_CONFIG, dlg.getObjectName(), currentObject.getObjectId());
						ConsoleSharedData.getInstance().getSession().createObject(cd);
						status = Status.OK_STATUS;
					}
					catch(Exception e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
			                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
			                    "Cannot create agent policy: " + e.getMessage(), e);
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
			currentObject = (GenericObject)((TreeSelection)selection).getFirstElement();
			action.setEnabled((currentObject instanceof PolicyRoot) || (currentObject instanceof PolicyGroup));
		}
	}
}
