/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.HostSearchResults;

/**
 * Find connection point for node or interface
 *
 */
public class FindConnectionPoint implements IObjectActionDelegate
{
	private IWorkbenchPart wbPart;
	private List<Long> objectId = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
	   if ((objectId == null) || (objectId.isEmpty()))
	      return;
	   
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().FindConnectionPoint_JobTitle, wbPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
			   final ConnectionPoint[] cps = new ConnectionPoint[objectId.size()];
			   for (int i = 0; i < objectId.size(); i++)
			   {
			      cps[i] = session.findConnectionPoint(objectId.get(i));
			      if (cps[i] == null)
			         cps[i] = new ConnectionPoint(session.findObjectById(objectId.get(i)).getParentIdList()[0], objectId.get(i), false);
			   }
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   if (cps.length == 1)
					      HostSearchResults.showConnection(cps[0]);
					   else
					      HostSearchResults.showConnection(cps);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().FindConnectionPoint_JobError;
			}
		}.start();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) &&
		    (((IStructuredSelection)selection).size() != 0))
		{
		   objectId = new ArrayList<Long>();
			for (Object s : ((IStructuredSelection)selection).toList())
			{
			   if ((s instanceof Node) || (s instanceof Interface) || (s instanceof AccessPoint))
			   {
			      action.setEnabled(true);
			      objectId.add(((AbstractObject)s).getObjectId());
			   }
			}
		}
		else
		{
			action.setEnabled(false);
			objectId = null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}
}
