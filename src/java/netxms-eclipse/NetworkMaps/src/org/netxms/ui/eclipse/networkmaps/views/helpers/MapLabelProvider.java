/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.core.runtime.Platform;
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
import org.eclipse.gef4.zest.core.viewers.IFigureProvider;
import org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider;
import org.eclipse.gef4.zest.core.widgets.GraphConnection;
import org.eclipse.gef4.zest.core.widgets.GraphNode;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapResource;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.UnknownObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.MapImageProvidersManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Label provider for map
 */
public class MapLabelProvider extends LabelProvider implements IFigureProvider, ISelfStyleProvider
{
	private NXCSession session;
	private ExtendedGraphViewer viewer;
	private Image[] statusImages;
	private Image imgNetMap;
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
	private Image imgAccessPoint;
	private Image imgOther;
	private Image imgUnknown;
	private Image imgResCluster;
	private Font fontLabel;
	private Font fontTitle;
	private boolean showStatusIcons = true;
	private boolean showStatusBackground = false;
	private boolean showStatusFrame = false;
	private boolean enableLongObjectName = false;
	private boolean hideLinkLabel = false;
	private ILabelProvider workbenchLabelProvider;
	private ObjectFigureType objectFigureType = ObjectFigureType.ICON;
	private ColorCache colors;
	private Color defaultLinkColor = null;
	private ManhattanConnectionRouter manhattanRouter = new ManhattanConnectionRouter();
	private BendpointConnectionRouter bendpointRouter = new BendpointConnectionRouter();
	private LinkDciValueProvider dciValueProvider;
	
	/**
	 * Create map label provider
	 */
	public MapLabelProvider(ExtendedGraphViewer viewer)
	{
		this.viewer = viewer;
		session = (NXCSession)ConsoleSharedData.getSession();
		
		// Initiate loading of object browser plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new UnknownObject(99, session), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		
		workbenchLabelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();

		statusImages = new Image[9];
		for(int i = 0; i < statusImages.length; i++)
			statusImages[i] = StatusDisplayInfo.getStatusImageDescriptor(i).createImage();

		imgNetMap = Activator.getImageDescriptor("icons/objects/netmap.png").createImage(); //$NON-NLS-1$
		imgNodeGeneric = Activator.getImageDescriptor("icons/objects/node.png").createImage(); //$NON-NLS-1$
		imgNodeOSX = Activator.getImageDescriptor("icons/objects/macserver.png").createImage(); //$NON-NLS-1$
		imgNodeWindows = Activator.getImageDescriptor("icons/objects/windowsserver.png").createImage(); //$NON-NLS-1$
		imgNodeLinux = Activator.getImageDescriptor("icons/objects/linuxserver.png").createImage(); //$NON-NLS-1$
		imgNodeFreeBSD = Activator.getImageDescriptor("icons/objects/freebsdserver.png").createImage(); //$NON-NLS-1$
		imgNodeSwitch = Activator.getImageDescriptor("icons/objects/switch.png").createImage(); //$NON-NLS-1$
		imgNodeRouter = Activator.getImageDescriptor("icons/objects/router.png").createImage(); //$NON-NLS-1$
		imgNodePrinter = Activator.getImageDescriptor("icons/objects/printer.png").createImage(); //$NON-NLS-1$
		imgSubnet = Activator.getImageDescriptor("icons/objects/subnet.png").createImage(); //$NON-NLS-1$
		imgService = Activator.getImageDescriptor("icons/objects/service.png").createImage(); //$NON-NLS-1$
		imgCluster = Activator.getImageDescriptor("icons/objects/cluster.png").createImage(); //$NON-NLS-1$
		imgAccessPoint = Activator.getImageDescriptor("icons/objects/accesspoint.png").createImage(); //$NON-NLS-1$
		imgOther = Activator.getImageDescriptor("icons/objects/other.png").createImage(); //$NON-NLS-1$
		imgUnknown = Activator.getImageDescriptor("icons/objects/unknown.png").createImage(); //$NON-NLS-1$
		imgResCluster = Activator.getImageDescriptor("icons/resources/cluster_res.png").createImage(); //$NON-NLS-1$

		final Display display = viewer.getControl().getDisplay();	
		fontLabel = new Font(display, "Verdana", WidgetHelper.scaleTextPoints(display, 6), SWT.NORMAL); //$NON-NLS-1$
		fontTitle = new Font(display, "Verdana", WidgetHelper.scaleTextPoints(display, 10), SWT.NORMAL); //$NON-NLS-1$

		IPreferenceStore store = Activator.getDefault().getPreferenceStore();
		showStatusIcons = store.getBoolean("NetMap.ShowStatusIcon"); //$NON-NLS-1$
		showStatusFrame = store.getBoolean("NetMap.ShowStatusFrame"); //$NON-NLS-1$
		showStatusBackground = store.getBoolean("NetMap.ShowStatusBackground"); //$NON-NLS-1$
		
		colors = new ColorCache();
		
		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		enableLongObjectName = ps.getBoolean("ENABLE_LONG_OBJECT_NAME"); //$NON-NLS-1$
		dciValueProvider = LinkDciValueProvider.getInstance();
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
			AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
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
			AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
			if (object != null)
			{
				// First, check if object has custom map image set
				final UUID objectImageGuid = object.getImage();
				if (objectImageGuid != null && !objectImageGuid.equals(NXCommon.EMPTY_GUID))
				{
					return ImageProvider.getInstance().getImage(objectImageGuid);
				}
				
				// Try registered network map image providers
				Image img = MapImageProvidersManager.getInstance().getMapImage(object);
				if (img != null)
					return img;

				// Use built-in image as last resort
				switch(object.getObjectClass())
				{
					case AbstractObject.OBJECT_NODE:
						if ((((Node)object).getFlags() & Node.NF_IS_BRIDGE) != 0)
							return imgNodeSwitch;
						if ((((Node)object).getFlags() & Node.NF_IS_ROUTER) != 0)
							return imgNodeRouter;
						if ((((Node)object).getFlags() & Node.NF_IS_PRINTER) != 0)
							return imgNodePrinter;
						if (((Node)object).getPlatformName().startsWith("windows")) //$NON-NLS-1$
							return imgNodeWindows;
						if (((Node)object).getPlatformName().startsWith("Linux")) //$NON-NLS-1$
							return imgNodeLinux;
						if (((Node)object).getPlatformName().startsWith("FreeBSD")) //$NON-NLS-1$
							return imgNodeFreeBSD;
						return imgNodeGeneric;
					case AbstractObject.OBJECT_SUBNET:
						return imgSubnet;
					case AbstractObject.OBJECT_CONTAINER:
						return imgService;
					case AbstractObject.OBJECT_CLUSTER:
						return imgCluster;
					case AbstractObject.OBJECT_ACCESSPOINT:
						return imgAccessPoint;
					case AbstractObject.OBJECT_NETWORKMAP:
						return imgNetMap;
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
	 * org.eclipse.gef4.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
	 */
	@Override
	public IFigure getFigure(Object element)
	{
		if (element instanceof NetworkMapObject)
		{
			switch(objectFigureType)
			{
            case LARGE_LABEL:
               return new ObjectFigureLargeLabel((NetworkMapObject)element, this);
				case SMALL_LABEL:
					return new ObjectFigureSmallLabel((NetworkMapObject)element, this);
				case ICON:
					return new ObjectFigureIcon((NetworkMapObject)element, this);
				case STATUS:
               return new ObjectStatusIcon((NetworkMapObject)element, this);				   
				default:
					return null;
			}
		}
		if (element instanceof NetworkMapResource)
		{
			return new ResourceFigure((NetworkMapResource)element, this);
		}
		if (element instanceof NetworkMapDecoration)
		{
			return new DecorationFigure((NetworkMapDecoration)element, this, viewer);
		}
		if (element instanceof NetworkMapDCIContainer)
      {
         return new DCIContainerFigure((NetworkMapDCIContainer)element, this, viewer);
      }
		if (element instanceof NetworkMapDCIImage)
      {
         return new DCIImageFigure((NetworkMapDCIImage)element, this, viewer);
      }
		return null;
	}

	/**
	 * Get status image for given NetXMS object
	 * 
	 * @param object
	 * @return
	 */
	public Image getStatusImage(AbstractObject object)
	{
		Image image = MapImageProvidersManager.getInstance().getStatusIcon(object.getStatus());
		if (image == null)
		{
			try
			{
				image = statusImages[object.getStatus().getValue()];
			}
			catch(IndexOutOfBoundsException e)
			{
			}
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

		imgNetMap.dispose();
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
		imgAccessPoint.dispose();
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
	 * org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider#selfStyleConnection(java
	 * .lang.Object, org.eclipse.gef4.zest.core.widgets.GraphConnection)
	 */
	@Override
	public void selfStyleConnection(Object element, GraphConnection connection)
	{
		NetworkMapLink link = (NetworkMapLink)connection.getData();

		if (link.getType() == NetworkMapLink.VPN)
		{
		   connection.setLineStyle(SWT.LINE_DOT);
		}
		
		if (link.hasConnectorName1() && !hideLinkLabel)
		{
			ConnectionEndpointLocator sourceEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), false);
			sourceEndpointLocator.setVDistance(0);
			final Label label = new ConnectorLabel(link.getConnectorName1());
			label.setFont(fontLabel);
			connection.getConnectionFigure().add(label, sourceEndpointLocator);
		}
		if (link.hasConnectorName2() && !hideLinkLabel)
		{
			ConnectionEndpointLocator targetEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), true);
			targetEndpointLocator.setVDistance(0);
			final Label label = new ConnectorLabel(link.getConnectorName2());
			label.setFont(fontLabel);
			connection.getConnectionFigure().add(label, targetEndpointLocator);
		}
		
		boolean hasDciData = link.hasDciData();
      boolean hasName = link.hasName();
      
      if ((hasName || hasDciData) && !hideLinkLabel)
      {
         ConnectionLocator nameLocator = new ConnectionLocator(connection.getConnectionFigure());
         nameLocator.setRelativePosition(PositionConstants.CENTER);
         
         String labelString = ""; //$NON-NLS-1$
         if(hasName)
            labelString += link.getName();
         
         if(hasName && hasDciData)
            labelString +="\n"; //$NON-NLS-1$
         
         if(hasDciData)
         {
            labelString += dciValueProvider.getDciDataAsString(link);
         }
         
         final Label label = new ConnectorLabel(labelString);
         label.setFont(fontLabel);
         connection.getConnectionFigure().add(label, nameLocator);
      }

		if (link.getStatusObject() != null && link.getStatusObject().size() != 0)
		{
		   ObjectStatus status = ObjectStatus.UNKNOWN;
		   for(Long id : link.getStatusObject())
		   {
   			AbstractObject object = session.findObjectById(id);
   			if (object != null)
            {
   			   ObjectStatus s = object.getStatus();
   			   if ((s.compareTo(ObjectStatus.UNKNOWN) < 0) && ((status.compareTo(s) < 0) || (status == ObjectStatus.UNKNOWN)))
   			   {
   			      status = s;
   			      if (status == ObjectStatus.CRITICAL)
   			         break;
   			   }
            } 
         }
		   connection.setLineColor(StatusDisplayInfo.getStatusColor(status));   			
		}
		else if (link.getColor() >= 0)
		{
			connection.setLineColor(colors.create(ColorConverter.rgbFromInt(link.getColor())));
		}
		else if (defaultLinkColor != null)
		{
			connection.setLineColor(defaultLinkColor);
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
				connection.setData("ROUTER", bendpointRouter); //$NON-NLS-1$
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
	 * @see org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider#selfStyleNode(java.lang.Object, org.eclipse.gef4.zest.core.widgets.GraphNode)
	 */
	@Override
	public void selfStyleNode(Object element, GraphNode node)
	{
	}

	/**
	 * @return the defaultLinkColor
	 */
	public final Color getDefaultLinkColor()
	{
		return defaultLinkColor;
	}

	/**
	 * @param defaultLinkColor the defaultLinkColor to set
	 */
	public final void setDefaultLinkColor(Color defaultLinkColor)
	{
		this.defaultLinkColor = defaultLinkColor;
	}

	/**
	 * Get cached last DCI values for given node
	 * 
	 * @param nodeId
	 * @return cached last values or null
	 */
	public DciValue[] getNodeLastValues(long nodeId)
	{
		return ((MapContentProvider)viewer.getContentProvider()).getNodeLastValues(nodeId);
	}

   public boolean isLongObjectNameEnabled()
   {
      return enableLongObjectName;
   }

   public void setLabelHideStatus(boolean checked)
   {
     hideLinkLabel = checked;
   }
}
