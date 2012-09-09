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
package org.netxms.ui.eclipse.datacollection.actions;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Clear collected data for DCI
 */
public class ClearCollectedData implements IObjectActionDelegate
{
	private IWorkbenchPart part;
	private Set<DCI> objects = null;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		part = targetPart;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		if (objects == null)
			return;
		
		if (!MessageDialog.openQuestion(part.getSite().getShell(), Messages.ClearCollectedData_ConfirmDialogTitle, Messages.ClearCollectedData_ConfirmationText))
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final Set<DCI> dciToClear = objects;
		new ConsoleJob(Messages.ClearCollectedData_TaskName, part, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.ClearCollectedData_TaskName, dciToClear.size());
				for(DCI d : dciToClear)
				{
					session.clearCollectedData(d.nodeId, d.dciId);
					monitor.worked(1);
				}
				monitor.done();
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.ClearCollectedData_ErrorText;
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		Set<DCI> dciList = null;
		if (selection instanceof IStructuredSelection)
		{
			dciList = new HashSet<DCI>();
			for(Object o : ((IStructuredSelection)selection).toList())
			{
				if (o instanceof DciValue)
				{
					dciList.add(new DCI(((DciValue)o).getNodeId(), ((DciValue)o).getId()));
				}
				else if (o instanceof DataCollectionObject)
				{
					dciList.add(new DCI(((DataCollectionObject)o).getNodeId(), ((DataCollectionObject)o).getId()));
				}
				else
				{
					dciList = null;
					break;
				}
				
			}
		}
		
		objects = dciList;
		action.setEnabled((objects != null) && (objects.size() > 0));
	}
	
	/**
	 *
	 */
	private class DCI
	{
		long nodeId;
		long dciId;
		
		/**
		 * @param nodeId
		 * @param dciId
		 */
		public DCI(long nodeId, long dciId)
		{
			this.nodeId = nodeId;
			this.dciId = dciId;
		}

		/* (non-Javadoc)
		 * @see java.lang.Object#hashCode()
		 */
		@Override
		public int hashCode()
		{
			final int prime = 31;
			int result = 1;
			result = prime * result + (int)(dciId ^ (dciId >>> 32));
			result = prime * result + (int)(nodeId ^ (nodeId >>> 32));
			return result;
		}
		
		/* (non-Javadoc)
		 * @see java.lang.Object#equals(java.lang.Object)
		 */
		@Override
		public boolean equals(Object obj)
		{
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			DCI other = (DCI)obj;
			if (dciId != other.dciId)
				return false;
			if (nodeId != other.nodeId)
				return false;
			return true;
		}
	}
}
