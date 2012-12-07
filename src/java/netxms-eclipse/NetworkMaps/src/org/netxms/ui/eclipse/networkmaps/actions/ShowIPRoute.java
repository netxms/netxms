/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.actions;

import java.util.List;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.networkmaps.views.IPRouteMap;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;

/**
 * Show IP route between two nodes
 */
public class ShowIPRoute implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private Node sourceNode;
	private Node targetNode;

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
		if ((sourceNode != null) && (targetNode == null))
		{
			ObjectSelectionDialog dlg = new ObjectSelectionDialog(window.getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter(false));
			if (dlg.open() != Window.OK)
				return;
			
			GenericObject[] selection = dlg.getSelectedObjects(Node.class);
			if (selection.length == 0)
			{
				MessageDialog.openError(window.getShell(), "Error", "Invalid target selection");
				return;
			}
			targetNode = (Node)selection[0];
		}
		
		if ((sourceNode != null) && (targetNode != null))
		{
			try
			{
				window.getActivePage().showView(IPRouteMap.ID, Long.toString(sourceNode.getObjectId()) + "&" + Long.toString(targetNode.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
			}
			catch(PartInitException e)
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
		sourceNode = null;
		targetNode = null;
		
		if (selection instanceof IStructuredSelection)
		{
			List<?> l = ((IStructuredSelection)selection).toList();
			if (l.size() > 0)
			{
				if (l.get(0) instanceof Node)
				{
					sourceNode = (Node)l.get(0);
					if ((l.size() > 1) && (l.get(1) instanceof Node))
					{
						targetNode = (Node)l.get(1);
					}
				}
			}
		}
	}
}
