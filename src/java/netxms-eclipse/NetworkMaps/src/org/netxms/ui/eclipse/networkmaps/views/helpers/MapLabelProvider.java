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

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.eclipse.draw2d.AbsoluteBendpoint;
import org.eclipse.draw2d.Bendpoint;
import org.eclipse.draw2d.BendpointConnectionRouter;
import org.eclipse.draw2d.ConnectionEndpointLocator;
import org.eclipse.draw2d.ConnectionLocator;
import org.eclipse.draw2d.ConnectionRouter;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.core.viewers.IFigureProvider;
import org.eclipse.zest.core.viewers.ISelfStyleProvider;
import org.eclipse.zest.core.widgets.GraphConnection;
import org.eclipse.zest.core.widgets.GraphNode;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapResource;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Label provider for map
 */
public class MapLabelProvider extends LabelProvider implements IFigureProvider, ISelfStyleProvider
{
	private NXCSession session;
	private GraphViewer viewer;
	private Image[] statusImages;
	private Image imgNodeGeneric;
	private Image imgNodeWindows;
	private Image imgNodeOSX;
	private Image imgNodeLinux;
	private Image imgNodeFreeBSD;
	private Image imgNodeSwitch;
	private Image imgNodeRouter;
	private Image imgNodePrinter;
	private Image imgSubnet;
	private Image imgService;
	private Image imgCluster;
	private Image imgOther;
	private Image imgUnknown;
	private Image imgResCluster;
	private Font fontLabel;
	private Font fontTitle;
	private boolean showStatusIcons = true;
	private boolean showStatusBackground = false;
	private boolean showStatusFrame = false;
	private ILabelProvider workbenchLabelProvider;
	private ObjectFigureType objectFigureType = ObjectFigureType.ICON;
	private ColorCache colors;
	private ManhattanConnectionRouter manhattanRouter = new ManhattanConnectionRouter();
	private BendpointConnectionRouter bendpointRouter = new BendpointConnectionRouter();
	

	/**
	 * Create map label provider
	 */
	public MapLabelProvider(GraphViewer viewer)
	{
		this.viewer = viewer;
		session = (NXCSession)ConsoleSharedData.getSession();
		workbenchLabelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();

		statusImages = new Image[9];
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i] = StatusDisplayInfo.getStatusImageDescriptor(i).createImage();

		imgNodeGeneric = Activator.getImageDescriptor("icons/objects/node.png").createImage();
		imgNodeOSX = Activator.getImageDescriptor("icons/objects/macserver.png").createImage();
		imgNodeWindows = Activator.getImageDescriptor("icons/objects/windowsserver.png").createImage();
		imgNodeLinux = Activator.getImageDescriptor("icons/objects/linuxserver.png").createImage();
		imgNodeFreeBSD = Activator.getImageDescriptor("icons/objects/freebsdserver.png").createImage();
		imgNodeSwitch = Activator.getImageDescriptor("icons/objects/switch.png").createImage();
		imgNodeRouter = Activator.getImageDescriptor("icons/objects/router.png").createImage();
		imgNodePrinter = Activator.getImageDescriptor("icons/objects/printer.png").createImage();
		imgSubnet = Activator.getImageDescriptor("icons/objects/subnet.png").createImage();
		imgService = Activator.getImageDescriptor("icons/objects/service.png").createImage();
		imgCluster = Activator.getImageDescriptor("icons/objects/cluster.png").createImage();
		imgOther = Activator.getImageDescriptor("icons/objects/other.png").createImage();
		imgUnknown = Activator.getImageDescriptor("icons/objects/unknown.png").createImage();
		imgResCluster = Activator.getImageDescriptor("icons/resources/cluster_res.png").createImage();

		fontLabel = new Font(Display.getDefault(), "Verdana", 7, SWT.NORMAL);
		fontTitle = new Font(Display.getDefault(), "Verdana", 10, SWT.NORMAL);

		IPreferenceStore store = Activator.getDefault().getPreferenceStore();
		showStatusIcons = store.getBoolean("NetMap.ShowStatusIcon");
		showStatusFrame = store.getBoolean("NetMap.ShowStatusFrame");
		showStatusBackground = store.getBoolean("NetMap.ShowStatusBackground");
		
		colors = new ColorCache();
	}

	/*
	 * (non-Javadoc)
	 * 
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

	/*
	 * (non-Javadoc)
	 * 
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
				final UUID objectImageGuid = object.getImage();
				if (objectImageGuid != null && !objectImageGuid.equals(NXCommon.EMPTY_GUID))
				{
					return ImageProvider.getInstance().getImage(objectImageGuid);
				}

				switch(object.getObjectClass())
				{
					case GenericObject.OBJECT_NODE:
						if ((((Node)object).getFlags() & Node.NF_IS_BRIDGE) != 0)
							return imgNodeSwitch;
						if ((((Node)object).getFlags() & Node.NF_IS_ROUTER) != 0)
							return imgNodeRouter;
						if ((((Node)object).getFlags() & Node.NF_IS_PRINTER) != 0)
							return imgNodePrinter;
						if (((Node)object).getPlatformName().startsWith("windows"))
							return imgNodeWindows;
						if (((Node)object).getPlatformName().startsWith("Linux"))
							return imgNodeLinux;
						if (((Node)object).getPlatformName().startsWith("FreeBSD"))
							return imgNodeFreeBSD;
						return imgNodeGeneric;
					case GenericObject.OBJECT_SUBNET:
						return imgSubnet;
					case GenericObject.OBJECT_CONTAINER:
						return imgService;
					case GenericObject.OBJECT_CLUSTER:
						return imgCluster;
					default:
						return imgOther;
				}
			}
			else
			{
				return imgUnknown;
			}
		}
		else if (element instanceof NetworkMapResource)
		{
			return imgResCluster;
		}
		return null;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
	 */
	@Override
	public IFigure getFigure(Object element)
	{
		if (element instanceof NetworkMapObject)
		{
			switch(objectFigureType)
			{
				case SMALL_LABEL:
					return new ObjectFigureSmallLabel((NetworkMapObject)element, this);
				case ICON:
					return new ObjectFigureIcon((NetworkMapObject)element, this);
			}
		}
		if (element instanceof NetworkMapResource)
		{
			return new ResourceFigure((NetworkMapResource)element, this);
		}
		if (element instanceof NetworkMapDecoration)
		{
			return new DecorationFigure((NetworkMapDecoration)element, this);
		}
		return null;
	}

	/**
	 * Get status image for given NetXMS object
	 * 
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i].dispose();

		imgNodeGeneric.dispose();
		imgNodeWindows.dispose();
		imgNodeLinux.dispose();
		imgNodeOSX.dispose();
		imgNodeFreeBSD.dispose();

		imgNodeSwitch.dispose();
		imgNodeRouter.dispose();
		imgNodePrinter.dispose();
		
		imgSubnet.dispose();
		imgService.dispose();
		imgCluster.dispose();
		imgOther.dispose();
		imgUnknown.dispose();
		
		imgResCluster.dispose();
		
		fontLabel.dispose();
		fontTitle.dispose();
		
		colors.dispose();
		
		workbenchLabelProvider.dispose();
		super.dispose();
	}

	/**
	 * @return the font
	 */
	public Font getLabelFont()
	{
		return fontLabel;
	}

	/**
	 * @return the font
	 */
	public Font getTitleFont()
	{
		return fontTitle;
	}

	/**
	 * @return the showStatusIcons
	 */
	public boolean isShowStatusIcons()
	{
		return showStatusIcons;
	}

	/**
	 * @param showStatusIcons
	 *           the showStatusIcons to set
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
	 * @param showStatusBackground
	 *           the showStatusBackground to set
	 */
	public void setShowStatusBackground(boolean showStatusBackground)
	{
		this.showStatusBackground = showStatusBackground;
	}

	/**
	 * Check if given element selected in the viewer
	 * 
	 * @param object
	 *           Object to test
	 * @return true if given object is selected
	 */
	public boolean isElementSelected(NetworkMapElement element)
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		return (selection != null) ? selection.toList().contains(element) : false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.zest.core.viewers.ISelfStyleProvider#selfStyleConnection(java
	 * .lang.Object, org.eclipse.zest.core.widgets.GraphConnection)
	 */
	@Override
	public void selfStyleConnection(Object element, GraphConnection connection)
	{
		NetworkMapLink link = (NetworkMapLink)connection.getData();
		
		if (link.hasConnectorName1())
		{
			ConnectionEndpointLocator sourceEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), false);
			sourceEndpointLocator.setVDistance(0);
			final Label label = new ConnectorLabel(link.getConnectorName1());
			label.setFont(fontLabel);
			connection.getConnectionFigure().add(label, sourceEndpointLocator);
		}
		if (link.hasConnectorName2())
		{
			ConnectionEndpointLocator targetEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), true);
			targetEndpointLocator.setVDistance(0);
			final Label label = new ConnectorLabel(link.getConnectorName2());
			label.setFont(fontLabel);
			connection.getConnectionFigure().add(label, targetEndpointLocator);
		}
		if (link.hasName())
		{
			ConnectionLocator nameLocator = new ConnectionLocator(connection.getConnectionFigure());
			nameLocator.setRelativePosition(PositionConstants.CENTER);
			final Label label = new ConnectorLabel(link.getName());
			label.setFont(fontLabel);
			connection.getConnectionFigure().add(label, nameLocator);
		}

		if (link.getStatusObject() != 0)
		{
			GenericObject object = session.findObjectById(link.getStatusObject());
			if (object != null)
			{
				connection.setLineColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
			}
			else
			{
				connection.setLineColor(StatusDisplayInfo.getStatusColor(Severity.UNKNOWN));
			}
		}
		else if (link.getColor() >= 0)
		{
			connection.setLineColor(colors.create(ColorConverter.rgbFromInt(link.getColor())));
		}
		
		switch(link.getRouting())
		{
			case NetworkMapLink.ROUTING_DIRECT:
				connection.setRouter(ConnectionRouter.NULL);
				break;
			case NetworkMapLink.ROUTING_MANHATTAN:
				connection.setRouter(manhattanRouter);
				break;
			case NetworkMapLink.ROUTING_BENDPOINTS:
				connection.setRouter(bendpointRouter);
				connection.setData("ROUTER", bendpointRouter);
				Object bp = getConnectionPoints(link);
				bendpointRouter.setConstraint(connection.getConnectionFigure(), bp);
				connection.getConnectionFigure().setRoutingConstraint(bp);
				break;
			default:
				connection.setRouter(null);
				break;
		}
		
		connection.setLineWidth(2);
	}
	
	/**
	 * @param link
	 * @return
	 */
	private List<Bendpoint> getConnectionPoints(NetworkMapLink link)
	{
		long[] points = link.getBendPoints();
		if (points != null)
		{
			List<Bendpoint> list = new ArrayList<Bendpoint>(points.length / 2); 
			for(int i = 0; i < points.length; i += 2)
			{
				if (points[i] == 0x7FFFFFFF)
					break;
				list.add(new AbsoluteBendpoint((int)points[i], (int)points[i + 1]));
			}
			return list;
		}
		else
		{
			return new ArrayList<Bendpoint>(0);
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
	 * @param showStatusFrame
	 *           the showStatusFrame to set
	 */
	public void setShowStatusFrame(boolean showStatusFrame)
	{
		this.showStatusFrame = showStatusFrame;
	}
	
	/**
	 * @param object
	 * @return
	 */
	public Image getWorkbenchIcon(Object object)
	{
		return workbenchLabelProvider.getImage(object);
	}

	/**
	 * @return the objectFigureType
	 */
	public ObjectFigureType getObjectFigureType()
	{
		return objectFigureType;
	}

	/**
	 * @param objectFigureType the objectFigureType to set
	 */
	public void setObjectFigureType(ObjectFigureType objectFigureType)
	{
		this.objectFigureType = objectFigureType;
	}

	/**
	 * @return the colors
	 */
	protected ColorCache getColors()
	{
		return colors;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.ISelfStyleProvider#selfStyleNode(java.lang.Object, org.eclipse.zest.core.widgets.GraphNode)
	 */
	@Override
	public void selfStyleNode(Object element, GraphNode node)
	{
	}
}
