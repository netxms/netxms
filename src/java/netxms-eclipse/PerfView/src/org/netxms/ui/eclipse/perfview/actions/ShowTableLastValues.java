/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.ui.PlatformUI;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.TableDciValue;
import org.netxms.ui.eclipse.datacollection.api.DciOpenHandler;
import org.netxms.ui.eclipse.perfview.views.TableLastValues;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Display last values for table DCI
 */
public class ShowTableLastValues implements IObjectActionDelegate, DciOpenHandler
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
					(currentSelection instanceof DataCollectionTable) ?
							Long.toString(((DataCollectionTable)currentSelection).getNodeId()) + "&" + Long.toString(((DataCollectionTable)currentSelection).getId()) :
							Long.toString(((TableDciValue)currentSelection).getNodeId()) + "&" + Long.toString(((TableDciValue)currentSelection).getId());
			try
			{
				window.getActivePage().showView(TableLastValues.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
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
			if ((element instanceof DataCollectionTable) || (element instanceof TableDciValue))
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.api.DciOpenHandler#open(org.netxms.client.datacollection.DciValue)
	 */
	@Override
	public boolean open(DciValue dci)
	{
		if (dci.getDcObjectType() != DataCollectionObject.DCO_TYPE_TABLE)
			return false;
		
		window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		currentSelection = dci;
		run(null);
		return true;
	}
}
