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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.SimpleDciValue;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.views.PredictedDataView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Display predicted data for DCI
 */
public class ShowPredictedData implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private List <Object> currentSelection;

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
		if (currentSelection != null && currentSelection.size() != 0)
		{
		   for(Object o : currentSelection)
		   {
   			final String id =
   					(o instanceof DataCollectionItem) ?
   							Long.toString(((DataCollectionItem)o).getNodeId()) + "&" + Long.toString(((DataCollectionItem)o).getId()) : //$NON-NLS-1$
   							Long.toString(((SimpleDciValue)o).getNodeId()) + "&" + Long.toString(((SimpleDciValue)o).getId()); //$NON-NLS-1$
   			try
   			{
   				window.getActivePage().showView(PredictedDataView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
   			}
   			catch(Exception e)
   			{
   				MessageDialogHelper.openError(window.getShell(), Messages.get().ShowHistoryData_Error, String.format(Messages.get().ShowHistoryData_ErrorOpeningView, e.getMessage()));
   			}
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
			@SuppressWarnings("unchecked")
         List<Object> element = ((IStructuredSelection)selection).toList();
			currentSelection = new ArrayList<Object>();
			
			for(Object o : element)
   			if ((o instanceof DataCollectionItem) || (o instanceof SimpleDciValue))
   				currentSelection.add(o);
		}

		action.setEnabled(currentSelection != null && currentSelection.size() > 0);
	}
}
