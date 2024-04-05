/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.packages.PackageDeploymentListener;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.PackageDeploymentMonitor;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for deployment status monitor
 */
public class DeploymentStatusLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class);
	private static final String[] statusText = { 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Pending"), 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Uploading file"), 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Installing"), 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Completed"), 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Failed"), 
	      LocalizationHelper.getI18n(DeploymentStatusLabelProvider.class).tr("Initializing") 
	      }; 

	private BaseObjectLabelProvider objectLabelProvider;
	private Image imageActive;
	private Image imagePending;
	private Image imageCompleted;
	private Image imageFailed;

	/**
	 * Default constructor
	 */
	public DeploymentStatusLabelProvider()
	{
	   objectLabelProvider = new BaseObjectLabelProvider();
      imageActive = ResourceManager.getImage("icons/deployStatus/active.png");
      imagePending = ResourceManager.getImage("icons/deployStatus/pending.png");
      imageCompleted = ResourceManager.getImage("icons/deployStatus/complete.png");
      imageFailed = ResourceManager.getImage("icons/deployStatus/failed.png");
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		DeploymentStatus s = (DeploymentStatus)element;
		switch(columnIndex)
		{
			case PackageDeploymentMonitor.COLUMN_NODE:
				AbstractNode node = s.getNodeObject();
				return objectLabelProvider.getImage(node);
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

   /**
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
					return i18n.tr("Unknown");
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
      objectLabelProvider.dispose();
		imageActive.dispose();
		imageCompleted.dispose();
		imageFailed.dispose();
		imagePending.dispose();
		super.dispose();
	}
}
