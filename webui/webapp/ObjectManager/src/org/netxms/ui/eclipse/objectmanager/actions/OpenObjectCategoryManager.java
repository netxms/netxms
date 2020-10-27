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
package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PartInitException;
import org.netxms.ui.eclipse.objectmanager.views.ObjectCategoryManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Open object categories manager
 */
public class OpenObjectCategoryManager implements IWorkbenchWindowActionDelegate
{
	private IWorkbenchWindow window;
	
   /**
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
    */
	@Override
	public void dispose()
	{
	}

   /**
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
    */
	@Override
	public void init(IWorkbenchWindow window)
	{
		this.window = window;
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
    */
	@Override
	public void run(IAction action)
	{
      if (window != null)
		{	
			try 
			{
            window.getActivePage().showView(ObjectCategoryManager.ID);
			} 
			catch (PartInitException e) 
			{
            MessageDialogHelper.openError(window.getShell(), "Error",
                  String.format("Cannot open object category manager: %s", e.getLocalizedMessage()));
			}
		}
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
	}
}
