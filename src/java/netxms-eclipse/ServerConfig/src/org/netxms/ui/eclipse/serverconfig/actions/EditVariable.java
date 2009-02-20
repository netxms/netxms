/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.client.NXCException;
import org.netxms.client.NXCServerVariable;
import org.netxms.ui.eclipse.console.extensionproviders.NXMCSharedData;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.ServerConfigurationEditor;
import org.netxms.ui.eclipse.serverconfig.VariableEditDialog;

/**
 * @author victor
 *
 */
public class EditVariable implements IObjectActionDelegate
{
	private IWorkbenchPart wbPart;
	private Object[] currentSelection;
	
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}

	@Override
	public void run(IAction action)
	{
		if (currentSelection.length == 1)
		{
			NXCServerVariable var = (NXCServerVariable)currentSelection[0];
			final VariableEditDialog dlg = new VariableEditDialog(wbPart.getSite().getShell(), var.getName(), var.getValue());
			if (dlg.open() == Window.OK)
			{
				Job job = new Job("Modify configuration variable") {
					@Override
					protected IStatus run(IProgressMonitor monitor)
					{
						IStatus status;
						
						try
						{
							NXMCSharedData.getSession().setServerVariable(dlg.getVarName(), dlg.getVarValue());
							if (wbPart instanceof ServerConfigurationEditor)
								((ServerConfigurationEditor)wbPart).refreshVariablesList();
							status = Status.OK_STATUS;
						}
						catch(Exception e)
						{
							status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
							                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
							                    "Cannot modify configuration variable: " + e.getMessage(), e);
						}
						return status;
					}

					
					/* (non-Javadoc)
					 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
					 */
					@Override
					public boolean belongsTo(Object family)
					{
						return family == ServerConfigurationEditor.JOB_FAMILY;
					}
				};
				IWorkbenchSiteProgressService siteService =
			      (IWorkbenchSiteProgressService)wbPart.getSite().getAdapter(IWorkbenchSiteProgressService.class);
				siteService.schedule(job, 0, true);
			}
		}
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
			currentSelection = ((IStructuredSelection)selection).toArray();
	}
}
