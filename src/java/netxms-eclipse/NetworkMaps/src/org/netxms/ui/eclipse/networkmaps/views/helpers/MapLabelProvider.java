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

import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.eclipse.zest.core.viewers.ISelfStyleProvider;
import org.eclipse.zest.core.widgets.GraphConnection;
import org.eclipse.zest.core.widgets.GraphNode;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for map
 */
public class MapLabelProvider extends LabelProvider implements IFigureProvider, ISelfStyleProvider
{
	private NXCSession session;
	private GraphViewer viewer;
	private Image[] statusImages;
	private Image imgNode;
	private Image imgNodeSwitch;
	private Image imgNodeRouter;
	private Image imgSubnet;
	private Image imgService;
	private Image imgOther;
	private Font font;
	private boolean showStatusIcons = true;
	private boolean showStatusBackground = false;
	private boolean showStatusFrame = false;
	
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
		
		imgNode = Activator.getImageDescriptor("icons/objects/node.png").createImage();
		imgNodeSwitch = Activator.getImageDescriptor("icons/objects/switch.png").createImage();
		imgNodeRouter = Activator.getImageDescriptor("icons/objects/router.png").createImage();
		imgSubnet = Activator.getImageDescriptor("icons/subnet.png").createImage();
		imgService = Activator.getImageDescriptor("icons/objects/service.png").createImage();
		imgOther = Activator.getImageDescriptor("icons/other.png").createImage();
		
		font = new Font(Display.getDefault(), "Verdana", 7, SWT.NORMAL);
		
		IPreferenceStore store = Activator.getDefault().getPreferenceStore();
		showStatusIcons = store.getBoolean("NetMap.ShowStatusIcon");
		showStatusFrame = store.getBoolean("NetMap.ShowStatusFrame");
		showStatusBackground = store.getBoolean("NetMap.ShowStatusBackground");
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
		if (element instanceof NetworkMapLink)
		{
			return ((NetworkMapLink)element).getName();
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
			{
				switch(object.getObjectClass())
				{
					case GenericObject.OBJECT_NODE:
						if ((((Node)object).getFlags() & Node.NF_IS_BRIDGE) != 0)
							return imgNodeSwitch;
						if ((((Node)object).getFlags() & Node.NF_IS_ROUTER) != 0)
							return imgNodeRouter;
						return imgNode;
					case GenericObject.OBJECT_SUBNET:
						return imgSubnet;
					case GenericObject.OBJECT_CONTAINER:
						return imgService;
					default:
						return imgOther;
				}
			}
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
		if (element instanceof NetworkMapDecoration)
			return new DecorationFigure((NetworkMapDecoration)element, this);
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
		imgNodeSwitch.dispose();
		imgNodeRouter.dispose();
		imgSubnet.dispose();
		imgService.dispose();
		imgOther.dispose();
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

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.ISelfStyleProvider#selfStyleConnection(java.lang.Object, org.eclipse.zest.core.widgets.GraphConnection)
	 */
	@Override
	public void selfStyleConnection(Object element, GraphConnection connection)
	{
		NetworkMapLink link = (NetworkMapLink)connection.getData();
		
		if (link.getConnectorName1() != null)
		{
			ConnectionEndpointLocator sourceEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), false);
			sourceEndpointLocator.setVDistance(10);
			Label label = new Label(link.getConnectorName1());
			connection.getConnectionFigure().add(label, sourceEndpointLocator);
		}
		if (link.getConnectorName2() != null)
		{
			ConnectionEndpointLocator targetEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), true);
			targetEndpointLocator.setVDistance(10);
			Label label = new Label(link.getConnectorName2());
			connection.getConnectionFigure().add(label, targetEndpointLocator);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.ISelfStyleProvider#selfStyleNode(java.lang.Object, org.eclipse.zest.core.widgets.GraphNode)
	 */
	@Override
	public void selfStyleNode(Object element, GraphNode node)
	{
		IFigure figure = node.getNodeFigure();
		if ((figure != null) && (figure instanceof ObjectFigure))
		{
			((ObjectFigure)figure).update();
			figure.repaint();
		}
	}

	/**
	 * @return the showStatusFrame
	 */
	public boolean isShowStatusFrame()
	{
		return showStatusFrame;
	}

	/**
	 * @param showStatusFrame the showStatusFrame to set
	 */
	public void setShowStatusFrame(boolean showStatusFrame)
	{
		this.showStatusFrame = showStatusFrame;
	}
}
