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
package org.netxms.ui.eclipse.serverjobmanager.views.helpers;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerJob;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.serverjobmanager.Activator;
import org.netxms.ui.eclipse.serverjobmanager.views.ServerJobManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for server job list
 *
 */
public class ServerJobLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session;
	private Map<Integer, String> statusTexts = new HashMap<Integer, String>(5);
	private Map<Integer, Image> statusImages = new HashMap<Integer, Image>(5);
	
	/**
	 * The constructor.
	 */
	public ServerJobLabelProvider()
	{
		super();
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		statusTexts.put(ServerJob.ACTIVE, "Active");
		statusTexts.put(ServerJob.CANCEL_PENDING, "Cancel pending");
		statusTexts.put(ServerJob.CANCELLED, "Cancelled");
		statusTexts.put(ServerJob.COMPLETED, "Completed");
		statusTexts.put(ServerJob.FAILED, "Failed");
		statusTexts.put(ServerJob.ON_HOLD, "On hold");
		statusTexts.put(ServerJob.PENDING, "Pending");

		statusImages.put(ServerJob.ACTIVE, Activator.getImageDescriptor("icons/active.gif").createImage());
		statusImages.put(ServerJob.CANCEL_PENDING, Activator.getImageDescriptor("icons/cancel_pending.png").createImage());
		statusImages.put(ServerJob.CANCELLED, Activator.getImageDescriptor("icons/cancel.png").createImage());
		statusImages.put(ServerJob.COMPLETED, Activator.getImageDescriptor("icons/completed.gif").createImage());
		statusImages.put(ServerJob.FAILED, Activator.getImageDescriptor("icons/failed.png").createImage());
		statusImages.put(ServerJob.ON_HOLD, Activator.getImageDescriptor("icons/hold.gif").createImage());
		statusImages.put(ServerJob.PENDING, Activator.getImageDescriptor("icons/pending.gif").createImage());
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object obj, int index)
	{
		if (obj instanceof ServerJob)
		{
			switch(index)
			{
				case ServerJobManager.COLUMN_STATUS:
					return statusTexts.get(((ServerJob)obj).getStatus());
				case ServerJobManager.COLUMN_USER:
					AbstractUserObject user = session.findUserDBObjectById(((ServerJob)obj).getUserId());
					return (user != null) ? user.getName() : "<unknown>";
				case ServerJobManager.COLUMN_NODE:
					AbstractObject object = session.findObjectById(((ServerJob)obj).getNodeId());
					return (object != null) ? object.getObjectName() : "<unknown>";
				case ServerJobManager.COLUMN_DESCRIPTION:
					return ((ServerJob)obj).getDescription();
				case ServerJobManager.COLUMN_PROGRESS:
					return (((ServerJob)obj).getStatus() == ServerJob.ACTIVE) ? Integer.toString(((ServerJob)obj).getProgress()) + "%" : "";
				case ServerJobManager.COLUMN_MESSAGE:
					return ((ServerJob)obj).getFailureMessage();
			}
		}
		return "";
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(final Image image : statusImages.values())
			image.dispose();
		
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object obj)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (!(element instanceof ServerJob) || (columnIndex != ServerJobManager.COLUMN_STATUS))
			return null;
		
		return statusImages.get(((ServerJob)element).getStatus());
	}
}
