/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.views.helpers;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.datacollection.GraphFolder;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Label provider for predefined graphs tree 
 *
 */
public class GraphTreeLabelProvider extends LabelProvider
{
	private Image imgFolder;
	private Image imgGraph;
	
	/**
	 * Constructor
	 */
	public GraphTreeLabelProvider()
	{
		imgFolder = ResourceManager.getImageDescriptor("icons/folder.png").createImage(); //$NON-NLS-1$
		imgGraph = ResourceManager.getImageDescriptor("icons/object-views/chart-line.png").createImage(); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element instanceof GraphFolder)
			return imgFolder;
		if (element instanceof GraphDefinition)
			return imgGraph;
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		if (element instanceof GraphFolder)
			return ((GraphFolder)element).getDisplayName();
		if (element instanceof GraphDefinition)
			return ((GraphDefinition)element).getDisplayName();
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		imgFolder.dispose();
		imgGraph.dispose();
		super.dispose();
	}
}
