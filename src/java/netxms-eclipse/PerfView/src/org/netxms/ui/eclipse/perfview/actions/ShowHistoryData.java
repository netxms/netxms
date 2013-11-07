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
package org.netxms.ui.eclipse.perfview.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.SimpleDciValue;
import org.netxms.ui.eclipse.perfview.views.HistoricalDataView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Display historical data for DCI
 */
public class ShowHistoryData implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private Object currentSelection = null;

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
	@Override
	public void run(IAction action)
	{
		if (currentSelection != null)
		{
			final String id =
					(currentSelection instanceof DataCollectionItem) ?
							Long.toString(((DataCollectionItem)currentSelection).getNodeId()) + "&" + Long.toString(((DataCollectionItem)currentSelection).getId()) :
							Long.toString(((SimpleDciValue)currentSelection).getNodeId()) + "&" + Long.toString(((SimpleDciValue)currentSelection).getId());
			try
			{
				window.getActivePage().showView(HistoricalDataView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) && !selection.isEmpty())
		{
			Object element = ((IStructuredSelection)selection).getFirstElement();
			if ((element instanceof DataCollectionItem) || (element instanceof SimpleDciValue))
			{
				currentSelection = element;
			}
			else
			{
				currentSelection = null;
			}
		}
		else
		{
			currentSelection = null;
		}

		action.setEnabled(currentSelection != null);
	}
}
