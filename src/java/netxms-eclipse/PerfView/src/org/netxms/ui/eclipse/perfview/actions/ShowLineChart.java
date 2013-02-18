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

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.datacollection.api.DciOpenHandler;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Show line chart on double-click on DCI's value
 */
public class ShowLineChart implements DciOpenHandler
{
	private long uniqueId = 1000000000L;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.api.DciOpenHandler#open(org.netxms.client.datacollection.DciValue)
	 */
	@Override
	public boolean open(DciValue dci)
	{
		if ((dci.getDcObjectType() != DataCollectionObject.DCO_TYPE_ITEM) ||
		    (dci.getDataType() == DataCollectionObject.DT_STRING))
			return false;
		
		String name = null, description = null;
		try
		{
			name = URLEncoder.encode(dci.getName(), "UTF-8");
			description = URLEncoder.encode(dci.getDescription(), "UTF-8");
		}
		catch(UnsupportedEncodingException e)
		{
			description = "<description unavailable>";
		}
		
		final String id = Long.toString(uniqueId++) + "&" + Long.toString(dci.getNodeId()) + "@" + 
				Long.toString(dci.getId()) + "@" + Integer.toString(dci.getSource()) + "@" + 
				Integer.toString(dci.getDataType()) + "@" + name + "@" + description;
		
		final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
		try
		{
			window.getActivePage().showView(HistoricalGraphView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(window.getShell(), "Error", "Error opening view: " + e.getMessage());
		}
		return true;
	}
}
