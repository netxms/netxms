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
package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.perfview.Activator;

/**
 * Label provider for predefined graphs tree 
 *
 */
public class GraphTreeLabelProvider extends LabelProvider
{
	private Image imgFolder;
	private Image imgGraph;
	
	public GraphTreeLabelProvider()
	{
		imgFolder = Activator.getImageDescriptor("icons/folder.png").createImage();
		imgGraph = Activator.getImageDescriptor("icons/chart_line.png").createImage();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element instanceof GraphFolder)
			return imgFolder;
		if (element instanceof GraphSettings)
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
			return ((GraphFolder)element).getName();
		if (element instanceof GraphSettings)
			return ((GraphSettings)element).getShortName();
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
