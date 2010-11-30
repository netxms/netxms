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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.IFigure;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for map
 */
public class MapLabelProvider extends LabelProvider implements IFigureProvider
{
	private NXCSession session;
	private GraphViewer viewer;
	private Image[] statusImages;
	private Image imgNode;
	private Image imgSubnet;
	private Font font;
	private boolean showStatusIcons = true;
	private boolean showStatusBackground = false;
	
	/**
	 * Create map label provider
	 */
	public MapLabelProvider(GraphViewer viewer)
	{
		this.viewer = viewer;
		session = (NXCSession)ConsoleSharedData.getSession();
		
		statusImages = new Image[9];
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i] = StatusDisplayInfo.getStatusImageDescriptor(i).createImage();
		
		imgNode = Activator.getImageDescriptor("icons/node.png").createImage();
		imgSubnet = Activator.getImageDescriptor("icons/subnet.png").createImage();
		
		font = new Font(Display.getDefault(), "Verdana", 7, SWT.NORMAL);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
	 */
	@Override
	public String getText(Object element)
	{
		if (element instanceof NetworkMapObject)
		{
			GenericObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
			return (object != null) ? object.getObjectName() : null;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
	 */
	@Override
	public Image getImage(Object element)
	{
		if (element instanceof NetworkMapObject)
		{
			GenericObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
			if (object != null)
				return object.getObjectClass() == GenericObject.OBJECT_NODE ? imgNode : imgSubnet;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
	 */
	@Override
	public IFigure getFigure(Object element)
	{
		if (element instanceof NetworkMapObject)
			return new ObjectFigure((NetworkMapObject)element, this);
		return null;
	}
	
	/**
	 * Get status image for given NetXMS object
	 * @param object
	 * @return
	 */
	public Image getStatusImage(GenericObject object)
	{
		Image image = null;
		try
		{
			image = statusImages[object.getStatus()];
		}
		catch(IndexOutOfBoundsException e)
		{
		}
		return image;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i].dispose();
		imgNode.dispose();
		imgSubnet.dispose();
		font.dispose();
		super.dispose();
	}

	/**
	 * @return the font
	 */
	public Font getFont()
	{
		return font;
	}

	/**
	 * @return the showStatusIcons
	 */
	public boolean isShowStatusIcons()
	{
		return showStatusIcons;
	}

	/**
	 * @param showStatusIcons the showStatusIcons to set
	 */
	public void setShowStatusIcons(boolean showStatusIcons)
	{
		this.showStatusIcons = showStatusIcons;
	}

	/**
	 * @return the showStatusBackground
	 */
	public boolean isShowStatusBackground()
	{
		return showStatusBackground;
	}

	/**
	 * @param showStatusBackground the showStatusBackground to set
	 */
	public void setShowStatusBackground(boolean showStatusBackground)
	{
		this.showStatusBackground = showStatusBackground;
	}
	
	/**
	 * Check if given element selected in the viewer
	 * 
	 * @param object Object to test
	 * @return true if given object is selected
	 */
	public boolean isElementSelected(NetworkMapElement element)
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		return (selection != null) ? selection.toList().contains(element) : false;
	}
}
