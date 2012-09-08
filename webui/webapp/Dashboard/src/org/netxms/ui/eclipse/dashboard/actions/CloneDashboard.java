package org.netxms.ui.eclipse.dashboard.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.CreateObjectDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class CloneDashboard implements IObjectActionDelegate
{

	private IWorkbenchWindow window;
	private IWorkbenchPart part;
	private Dashboard sourceObject;

	@Override
	public void run(final IAction action)
	{
		final long parentId = sourceObject.getParentIdList()[0];

		final CreateObjectDialog dlg = new CreateObjectDialog(window.getShell(), Messages.CloneDashboard_Dashboard);
		if (dlg.open() == Window.OK)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.CloneDashboard_JobTitle, part, Activator.PLUGIN_ID, null)
			{
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_DASHBOARD, dlg.getObjectName(), parentId);
					final long newDashboardId = session.createObject(cd);

					final NXCObjectModificationData md = new NXCObjectModificationData(newDashboardId);
					md.setDashboardElements(sourceObject.getElements());
					md.setColumnCount(sourceObject.getNumColumns());
					md.setObjectFlags(sourceObject.getOptions());

					session.modifyObject(md);
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.CloneDashboard_ErrorPrefix + dlg.getObjectName() + Messages.CloneDashboard_ErrorSuffix;
				}
			}.start();
		}
	}

	@Override
	public void selectionChanged(final IAction action, final ISelection selection)
	{
		Object obj;
		if ((selection instanceof IStructuredSelection) && (((IStructuredSelection)selection).size() == 1)
				&& ((obj = ((IStructuredSelection)selection).getFirstElement()) instanceof Dashboard))
		{
			sourceObject = ((Dashboard)obj);
		}
		else
		{
			sourceObject = null;
		}
		action.setEnabled(sourceObject != null);
	}

	@Override
	public void setActivePart(final IAction action, final IWorkbenchPart targetPart)
	{
		part = targetPart;
		window = targetPart.getSite().getWorkbenchWindow();
	}

}
