/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.topology.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IViewReference;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.VlanView;

/**
 * Show vlans configured on network device
 */
public class ShowVlans implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private long objectId;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		IViewReference vr = window.getActivePage().findViewReference(VlanView.ID, Long.toString(objectId));
		if (vr != null)
		{
			VlanView view = (VlanView)vr.getView(true);
			if (view != null)
			{
				window.getActivePage().activate(view);
			}
		}
		else
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.get().ShowVlans_JobTitle, null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final List<VlanInfo> vlans = session.getVlans(objectId);
					PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							try
							{
								VlanView view = (VlanView)window.getActivePage().showView(VlanView.ID, Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE);
								view.setVlans(vlans);
							}
							catch(PartInitException e)
							{
								MessageDialogHelper.openError(window.getShell(), Messages.get().ShowVlans_Error, Messages.get().ShowVlans_CannotOpenView + e.getLocalizedMessage());
							}
						}
					});
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.get().ShowVlans_JobError;
				}
			}.start();
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) &&
		    (((IStructuredSelection)selection).size() == 1))
		{
			Object obj = ((IStructuredSelection)selection).getFirstElement();
			if (obj instanceof AbstractNode)
			{
				action.setEnabled(true);
				objectId = ((AbstractObject)obj).getObjectId();
			}
			else
			{
				action.setEnabled(false);
				objectId = 0;
			}
		}
		else
		{
			action.setEnabled(false);
			objectId = 0;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		window = targetPart.getSite().getWorkbenchWindow();
	}
}
