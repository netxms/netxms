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
package org.netxms.ui.eclipse.perfview.actions;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;

/**
 * Show line graph for selected DCI(s)
 *
 */
public class ShowHistoryGraph implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private Object[] currentSelection = null;
	private long uniqueId = 0;

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
			String id = Long.toString(uniqueId++);
			for(int i = 0; i < currentSelection.length; i++)
			{
				long dciId = 0, nodeId = 0;
				int source = 0, dataType = 0;
				String name = null, description = null;
				
				try
				{
					if (currentSelection[i] instanceof DciValue)
					{
						dciId = ((DciValue)currentSelection[i]).getId(); 
						nodeId = ((DciValue)currentSelection[i]).getNodeId();
						source = ((DciValue)currentSelection[i]).getSource();
						dataType = ((DciValue)currentSelection[i]).getDataType();
						name = URLEncoder.encode(((DciValue)currentSelection[i]).getName(), "UTF-8");
						description = URLEncoder.encode(((DciValue)currentSelection[i]).getDescription(), "UTF-8");
					}
					else if (currentSelection[i] instanceof DataCollectionItem)
					{
						dciId = ((DataCollectionItem)currentSelection[i]).getId(); 
						nodeId = ((DataCollectionItem)currentSelection[i]).getNodeId();
						source = ((DataCollectionItem)currentSelection[i]).getOrigin();
						dataType = ((DataCollectionItem)currentSelection[i]).getDataType();
						name = URLEncoder.encode(((DataCollectionItem)currentSelection[i]).getName(), "UTF-8");
						description = URLEncoder.encode(((DataCollectionItem)currentSelection[i]).getDescription(), "UTF-8");
					}
				}
				catch(UnsupportedEncodingException e)
				{
					description = "<description unavailable>";
				}
				
				id += "&" + Long.toString(nodeId) + "@" + Long.toString(dciId) + "@" + 
					Integer.toString(source) + "@" + Integer.toString(dataType) + "@" + name + "@" + description;
			}
			
			try
			{
				window.getActivePage().showView(HistoricalGraphView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
			}
			catch(PartInitException e)
			{
				MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
			}
			catch(IllegalArgumentException e)
			{
				MessageDialog.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
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
			if ((element instanceof DciValue) || (element instanceof DataCollectionItem))
			{
				currentSelection = ((IStructuredSelection)selection).toArray();
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
		
		action.setEnabled((currentSelection != null) && (currentSelection.length > 0));
	}
}
