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
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.objects.Node;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.views.PackageDeploymentMonitor;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Label provider for deployment status monitor
 */
public class DeploymentStatusLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] statusText = { "Pending", "Uploading file", "Installing", "Completed", "Failed", "Initializing" }; 
			
	private WorkbenchLabelProvider workbenchLabelProvider;
	private Image imageActive;
	private Image imagePending;
	private Image imageCompleted;
	private Image imageFailed;
	
	/**
	 * Default constructor
	 */
	public DeploymentStatusLabelProvider()
	{
		workbenchLabelProvider = new WorkbenchLabelProvider();
		imageActive = Activator.getImageDescriptor("icons/active.gif").createImage();
		imagePending = Activator.getImageDescriptor("icons/pending.gif").createImage();
		imageCompleted = Activator.getImageDescriptor("icons/complete.png").createImage();
		imageFailed = Activator.getImageDescriptor("icons/failed.png").createImage();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		DeploymentStatus s = (DeploymentStatus)element;
		switch(columnIndex)
		{
			case PackageDeploymentMonitor.COLUMN_NODE:
				Node node = s.getNodeObject();
				return (node != null) ? workbenchLabelProvider.getImage(node) : SharedIcons.IMG_UNKNOWN_OBJECT;
			case PackageDeploymentMonitor.COLUMN_STATUS:
				switch(s.getStatus())
				{
					case PackageDeploymentListener.COMPLETED:
						return imageCompleted;
					case PackageDeploymentListener.FAILED:
						return imageFailed;
					case PackageDeploymentListener.INITIALIZE:
					case PackageDeploymentListener.INSTALLATION:
					case PackageDeploymentListener.TRANSFER:
						return imageActive;
					case PackageDeploymentListener.PENDING:
						return imagePending;
				}
				break;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		DeploymentStatus s = (DeploymentStatus)element;
		switch(columnIndex)
		{
			case PackageDeploymentMonitor.COLUMN_NODE:
				return s.getNodeName();
			case PackageDeploymentMonitor.COLUMN_STATUS:
				try
				{
					return statusText[s.getStatus()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return "Unknown";
				}
			case PackageDeploymentMonitor.COLUMN_ERROR:
				return s.getMessage();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		workbenchLabelProvider.dispose();
		imageActive.dispose();
		imageCompleted.dispose();
		imageFailed.dispose();
		imagePending.dispose();
		super.dispose();
	}
}
