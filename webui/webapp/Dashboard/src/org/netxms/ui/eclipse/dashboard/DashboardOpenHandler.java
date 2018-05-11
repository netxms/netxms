/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard;

import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.views.DashboardView;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Open handler for dashboard objects
 */
public class DashboardOpenHandler implements ObjectOpenHandler
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler#openObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean openObject(AbstractObject object)
	{
		if (!(object instanceof Dashboard))
			return false;
		
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			window.getActivePage().showView(DashboardView.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(window.getShell(), Messages.get().OpenDashboard_Error, Messages.get().OpenDashboard_ErrorText + e.getMessage());
		}
		return true;
	}
}
