/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

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
import org.eclipse.draw2d.PolygonDecoration;
import org.eclipse.draw2d.PolylineConnection;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.gef4.zest.core.viewers.IFigureProvider;
import org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider;
import org.eclipse.gef4.zest.core.widgets.GraphConnection;
import org.eclipse.gef4.zest.core.widgets.GraphNode;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.networkmaps.MapImageProvidersManager;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ColorCache;

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
   private Image imgCircuit;
   private Image imgCluster;
   private Image imgWirelessDomain;
   private Image imgAccessPoint;
   private Image imgInterface;
   private Image imgOther;
   private Image imgUnknown;
   private Font[] fontLabel;
   private Font[] fontTitle;
   private boolean showStatusIcons = false;
   private boolean showStatusBackground = true;
   private boolean showStatusFrame = false;
   private boolean showLinkDirection = false;
   private boolean translucentLabelBackground = true;
   private boolean enableLongObjectName = false;
   private boolean connectionLabelsVisible = true;
   private boolean connectionsVisible = true;
   private MapObjectDisplayMode objectFigureType = MapObjectDisplayMode.ICON;
   private ColorCache colors;
   private Color defaultLinkColor = null;
   private int defaultLinkColorSource = 0;
   private int defaultLinkStyle = SWT.LINE_SOLID;
   private int defaultLinkWidth;
   private ManhattanConnectionRouter manhattanRouter = new ManhattanConnectionRouter();
   private BendpointConnectionRouter bendpointRouter = new BendpointConnectionRouter();
   private LinkDciValueProvider dciValueProvider;
   private DecoratingObjectLabelProvider objectLabelProvider;
   private long mapId = 0;

   /**
    * Create map label provider
    */
   public MapLabelProvider(ExtendedGraphViewer viewer)
   {
      this.viewer = viewer;
      session = Registry.getSession();

      statusImages = new Image[9];
      for(int i = 0; i < statusImages.length; i++)
         statusImages[i] = StatusDisplayInfo.getStatusImageDescriptor(i).createImage();

      imgNetMap = ResourceManager.getImage("icons/netmap/objects/netmap.png");
      imgNodeGeneric = ResourceManager.getImage("icons/netmap/objects/node.png");
      imgNodeOSX = ResourceManager.getImage("icons/netmap/objects/macserver.png");
      imgNodeWindows = ResourceManager.getImage("icons/netmap/objects/windowsserver.png");
      imgNodeLinux = ResourceManager.getImage("icons/netmap/objects/linuxserver.png");
      imgNodeFreeBSD = ResourceManager.getImage("icons/netmap/objects/freebsdserver.png");
      imgNodeSwitch = ResourceManager.getImage("icons/netmap/objects/switch.png");
      imgNodeRouter = ResourceManager.getImage("icons/netmap/objects/router.png");
      imgNodePrinter = ResourceManager.getImage("icons/netmap/objects/printer.png");
      imgSubnet = ResourceManager.getImage("icons/netmap/objects/subnet.png");
      imgService = ResourceManager.getImage("icons/netmap/objects/service.png");
      imgCircuit = ResourceManager.getImage("icons/netmap/objects/circuit.png");
      imgCluster = ResourceManager.getImage("icons/netmap/objects/cluster.png");
      imgWirelessDomain = ResourceManager.getImage("icons/netmap/objects/wireless-domain.png");
      imgAccessPoint = ResourceManager.getImage("icons/netmap/objects/accesspoint.png");
      imgInterface = ResourceManager.getImage("icons/netmap/objects/interface.png");
      imgOther = ResourceManager.getImage("icons/netmap/objects/other.png");
      imgUnknown = ResourceManager.getImage("icons/netmap/objects/unknown.png");

      final Display display = viewer.getControl().getDisplay();
      fontLabel = new Font[ExtendedGraphViewer.zoomLevels.length];
      fontTitle = new Font[ExtendedGraphViewer.zoomLevels.length];
      for(int i = 0; i < ExtendedGraphViewer.zoomLevels.length; i++)
      {
         if (ExtendedGraphViewer.zoomLevels[i] < 1)
         {
            fontLabel[i] = new Font(display, "Verdana", (int)Math.round(7 / ExtendedGraphViewer.zoomLevels[i]), SWT.NORMAL); //$NON-NLS-1$
            fontTitle[i] = new Font(display, "Verdana", (int)Math.round(10 / ExtendedGraphViewer.zoomLevels[i]), SWT.NORMAL); //$NON-NLS-1$
         }
         else
         {
            fontLabel[i] = new Font(display, "Verdana", 7, SWT.NORMAL); //$NON-NLS-1$
            fontTitle[i] = new Font(display, "Verdana", 10, SWT.NORMAL); //$NON-NLS-1$
         }
      }

      PreferenceStore settings = PreferenceStore.getInstance();
      showStatusIcons = settings.getAsBoolean("NetMap.ShowStatusIcon", false);
      showStatusFrame = settings.getAsBoolean("NetMap.ShowStatusFrame", false);
      showStatusBackground = settings.getAsBoolean("NetMap.ShowStatusBackground", true);
      showLinkDirection = settings.getAsBoolean("NetMap.ShowLinkDirection", false);
      enableLongObjectName = settings.getAsBoolean("NetMap.LongObjectNames", false);
      translucentLabelBackground = settings.getAsBoolean("NetMap.TranslucentLabelBkgnd", true);
      defaultLinkWidth = settings.getAsInteger("NetMap.DefaultLinkWidth", 5);

      colors = new ColorCache();
      dciValueProvider = LinkDciValueProvider.getInstance();
      objectLabelProvider = new DecoratingObjectLabelProvider();
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      if (element instanceof NetworkMapObject)
      {
         AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId(), true);
         return (object != null) ? object.getNameOnMap() : null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (element instanceof NetworkMapObject)
      {
         AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId(), true);
         if (object != null)
         {
            // First, check if object has custom map image set
            final UUID objectImageGuid = object.getMapImage();
            if ((objectImageGuid != null) && !objectImageGuid.equals(NXCommon.EMPTY_GUID))
            {
               return ImageProvider.getInstance().getImage(objectImageGuid, () -> {
                  viewer.refresh(element);
               });
            }

            // Try registered network map image providers
            Image img = MapImageProvidersManager.getInstance().getMapImage(object);
            if (img != null)
               return img;

            // Use built-in image as last resort
            switch(object.getObjectClass())
            {
               case AbstractObject.OBJECT_NODE:
                  if ((((Node)object).getCapabilities() & Node.NC_IS_BRIDGE) != 0)
                     return imgNodeSwitch;
                  if ((((Node)object).getCapabilities() & Node.NC_IS_ROUTER) != 0)
                     return imgNodeRouter;
                  if ((((Node)object).getCapabilities() & Node.NC_IS_PRINTER) != 0)
                     return imgNodePrinter;
                  if (((Node)object).getPlatformName().startsWith("windows"))
                     return imgNodeWindows;
                  if (((Node)object).getPlatformName().startsWith("Linux"))
                     return imgNodeLinux;
                  if (((Node)object).getPlatformName().startsWith("FreeBSD"))
                     return imgNodeFreeBSD;
                  return imgNodeGeneric;
               case AbstractObject.OBJECT_SUBNET:
                  return imgSubnet;
               case AbstractObject.OBJECT_CIRCUIT:
                  return imgCircuit;
               case AbstractObject.OBJECT_COLLECTOR:
               case AbstractObject.OBJECT_CONTAINER:
                  return imgService;
               case AbstractObject.OBJECT_CLUSTER:
                  return imgCluster;
               case AbstractObject.OBJECT_ACCESSPOINT:
                  return imgAccessPoint;
               case AbstractObject.OBJECT_NETWORKMAP:
                  return imgNetMap;
               case AbstractObject.OBJECT_INTERFACE:
                  return imgInterface;
               case AbstractObject.OBJECT_WIRELESSDOMAIN:
                  return imgWirelessDomain;
               default:
                  return imgOther;
            }
         }
         else
         {
            return imgUnknown;
         }
      }
      return null;
   }

   /**
    * @see org.eclipse.gef4.zest.core.viewers.IFigureProvider#getFigure(java.lang.Object)
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
            case FLOOR_PLAN:
               return new ObjectFloorPlan((NetworkMapObject)element, this, viewer);
            default:
               return null;
         }
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
      if (element instanceof NetworkMapTextBox)
      {
         return new TextBoxFigure((NetworkMapTextBox)element, this, viewer);
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

   /**
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
      imgCircuit.dispose();
      imgCluster.dispose();
      imgWirelessDomain.dispose();
      imgAccessPoint.dispose();
      imgInterface.dispose();
      imgOther.dispose();
      imgUnknown.dispose();

      for(int i = 0; i < ExtendedGraphViewer.zoomLevels.length; i++)
      {
         fontLabel[i].dispose();
         fontTitle[i].dispose();
      }

      colors.dispose();

      objectLabelProvider.dispose();

      super.dispose();
   }

   /**
    * @return the font
    */
   public Font getLabelFont()
   {
      return fontLabel[viewer.getCurrentZoomIndex()];
   }

   /**
    * @return the font
    */
   public Font getTitleFont()
   {
      return fontTitle[viewer.getCurrentZoomIndex()];
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
    * @return show link direction
    */
   public boolean isShowLinkDirection()
   {
      return showLinkDirection;
   }

   /**
    * @param showLinkDirection
    */
   public void setShowLinkDirection(boolean showLinkDirection)
   {
      this.showLinkDirection = showLinkDirection;
   }

   /**
    * @return translucentLabelBkgnd
    */
   public boolean isTranslucentLabelBackground()
   {
      return translucentLabelBackground;
   }

   /**
    * @param translucent
    */
   public void setTranslucentLabelBackground(boolean translucent)
   {
      translucentLabelBackground = translucent;
   }

   public void updateMapId(long objectId)
   {
      mapId = objectId;
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

	/**
	 * Re-resolve and apply the line color of an existing connection. Used for incremental refresh when a link's status object
	 * changes, since {@link #selfStyleConnection} rebuilds labels and decorations and would duplicate them if called repeatedly.
	 *
	 * @param link map link
	 * @param connection existing graph connection
	 */
	public void refreshConnectionColor(NetworkMapLink link, GraphConnection connection)
	{
		Color color = LinkStylingHelper.resolveLinkColor(link, session, dciValueProvider, colors, defaultLinkColor, defaultLinkColorSource);
		if (color != null)
			connection.setLineColor(color);
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider#selfStyleConnection(java.lang.Object,
    *      org.eclipse.gef4.zest.core.widgets.GraphConnection)
    */
	@Override
	public void selfStyleConnection(Object element, GraphConnection connection)
	{
      connection.setVisible(connectionsVisible);

		NetworkMapLink link = (NetworkMapLink)connection.getData();

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

      connection.setLineStyle(LinkStylingHelper.resolveLinkStyle(link, defaultLinkStyle));

      if (connectionLabelsVisible)
		{
         String name = getConnectorName(link, false);
         if (name != null)
         {
            ConnectionEndpointLocator sourceEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), false);
            sourceEndpointLocator.setVDistance(0);
            final Label label = new ConnectorLabel(name, this);
            label.setFont(getLabelFont());
            connection.getConnectionFigure().add(label, sourceEndpointLocator);
         }

         name = getConnectorName(link, true);
         if (name != null)
         {
            ConnectionEndpointLocator targetEndpointLocator = new ConnectionEndpointLocator(connection.getConnectionFigure(), true);
            targetEndpointLocator.setVDistance(0);
            final Label label = new ConnectorLabel(name, this);
            label.setFont(getLabelFont());
            connection.getConnectionFigure().add(label, targetEndpointLocator);
         }
		}

      connection.setLineWidth(LinkStylingHelper.resolveLinkWidth(link, defaultLinkWidth));

		if (showLinkDirection)
		{
		   PolygonDecoration decoration = new PolygonDecoration();
		   double scale = connection.getLineWidth() * 0.9 + 2;
		   decoration.setScale(scale * 2.3, scale);
         ((PolylineConnection)connection.getConnectionFigure()).setTargetDecoration(decoration);
		}

		IFigure owner = ((PolylineConnection)connection.getConnectionFigure()).getTargetAnchor().getOwner();
      ((PolylineConnection)connection.getConnectionFigure()).setTargetAnchor(new MultiConnectionAnchor(owner, link));
      owner = ((PolylineConnection)connection.getConnectionFigure()).getSourceAnchor().getOwner();
      ((PolylineConnection)connection.getConnectionFigure()).setSourceAnchor(new MultiConnectionAnchor(owner, link));

		boolean hasDciData = link.hasDciData();
      boolean hasName = link.hasName();

      Color resolvedColor = LinkStylingHelper.resolveLinkColor(link, session, dciValueProvider, colors,
            defaultLinkColor, defaultLinkColorSource);
      if (resolvedColor != null)
         connection.setLineColor(resolvedColor);

      if ((hasName || hasDciData) && connectionLabelsVisible)
      {
         ConnectionLocator nameLocatorCenter;
         ConnectionLocator nameLocatorObject1;
         ConnectionLocator nameLocatorObject2;
         if (link.getRouting() == NetworkMapLink.ROUTING_BENDPOINTS && link.getBendPoints() != null && link.getBendPoints().length > 0)
         {
            nameLocatorCenter = new ConnectionLocator(connection.getConnectionFigure());
            nameLocatorCenter.setRelativePosition(PositionConstants.CENTER);
            nameLocatorObject1 = new ConnectionLocator(connection.getConnectionFigure(), ConnectionLocator.SOURCE);
            nameLocatorObject2 = new ConnectionLocator(connection.getConnectionFigure(), ConnectionLocator.TARGET);
            //TODO: add support for bend points in MultiLabelConnectionLocator
         }
         else
         {
            nameLocatorCenter = new MultiLabelConnectionLocator(connection.getConnectionFigure(), link, LinkDataLocation.CENTER);
            nameLocatorObject1 = new MultiLabelConnectionLocator(connection.getConnectionFigure(), link, LinkDataLocation.OBJECT1);
            nameLocatorObject2 = new MultiLabelConnectionLocator(connection.getConnectionFigure(), link, LinkDataLocation.OBJECT2);
         }


         String labelString = "";
         String labelObj1String = "";
         String labelObj2String = "";
         if (hasName)
            labelString += link.getName();

         if (hasDciData)
         {
            String label = dciValueProvider.getDciDataAsString(link, LinkDataLocation.CENTER);
            if (hasName && !label.isEmpty())
               labelString += "\n";
            labelString += label;

            labelObj1String = dciValueProvider.getDciDataAsString(link, LinkDataLocation.OBJECT1);
            labelObj2String = dciValueProvider.getDciDataAsString(link, LinkDataLocation.OBJECT2);
         }

         final Label label;
         if (link.getType() == NetworkMapLink.AGENT_TUNEL ||
             link.getType() == NetworkMapLink.AGENT_PROXY ||
             link.getType() == NetworkMapLink.ICMP_PROXY ||
             link.getType() == NetworkMapLink.SNMP_PROXY ||
             link.getType() == NetworkMapLink.SSH_PROXY ||
             link.getType() == NetworkMapLink.ZONE_PROXY)
            label = new ConnectorLabel(labelString, this, connection.getLineColor());
         else
            label = new ConnectorLabel(labelString, this);

         if (!labelString.isEmpty())
            connection.getConnectionFigure().add(label, nameLocatorCenter);
         if (!labelObj1String.isEmpty())
            connection.getConnectionFigure().add( new ConnectorLabel(labelObj1String, this), nameLocatorObject1);
         if (!labelObj2String.isEmpty())
            connection.getConnectionFigure().add( new ConnectorLabel(labelObj2String, this), nameLocatorObject2);
      }
	}

   /**
    * Get connector name for map link
    *
    * @param link map link
    * @param second true if name for second connector is requested
    * @return name for connector or null
    */
   private String getConnectorName(NetworkMapLink link, boolean second)
   {
      String name = second ? link.getConnectorName2() : link.getConnectorName1();
      if ((name != null) && !name.isBlank())
         return name;

      long interfaceId = second ? link.getInterfaceId2() : link.getInterfaceId1();
      return (interfaceId > 0) ? session.getObjectName(interfaceId) : null;
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
    * @param showStatusFrame the showStatusFrame to set
    */
   public void setShowStatusFrame(boolean showStatusFrame)
   {
      this.showStatusFrame = showStatusFrame;
   }

   /**
    * @return the defaultLinkStyle
    */
   public int geDefaultLinkStyle()
   {
      return defaultLinkStyle;
   }

   /**
    * @param defaultLinkStyle the defaultLinkStyle to set
    */
   public void setDefaultLinkStyle(int defaultLinkStyle)
   {
      this.defaultLinkStyle = defaultLinkStyle;
   }

   /**
    * @return the defaultLinkWidth
    */
   public int getDefaultLinkWidth()
   {
      return defaultLinkWidth;
   }

   /**
    * Set default link width (to be used when not redefined on link level). Setting this value to 0 will revert to client default
    * width.
    *
    * @param defaultLinkWidth new default link width (0 to revert to client default)
    */
   public void setDefaultLinkWidth(int defaultLinkWidth)
   {
      this.defaultLinkWidth = (defaultLinkWidth > 0) ? defaultLinkWidth : PreferenceStore.getInstance().getAsInteger("NetMap.DefaultLinkWidth", 5);
   }

   /**
    * @param object
    * @return
    */
   public Image getSmallIcon(Object object)
   {
      return objectLabelProvider.getImage(object);
   }

   /**
    * @return the objectFigureType
    */
   public MapObjectDisplayMode getObjectFigureType()
   {
      return objectFigureType;
   }

   /**
    * @param objectFigureType the objectFigureType to set
    */
   public void setObjectFigureType(MapObjectDisplayMode objectFigureType)
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

   /**
    * @see org.eclipse.gef4.zest.core.viewers.ISelfStyleProvider#selfStyleNode(java.lang.Object,
    *      org.eclipse.gef4.zest.core.widgets.GraphNode)
    */
   @Override
   public void selfStyleNode(Object element, GraphNode node)
   {
      node.setTooltip(new ObjectTooltip((NetworkMapObject)element, mapId, this));
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
    * @return the defaultLinkColorSource
    */
   public int getDefaultLinkColorSource()
   {
      return defaultLinkColorSource;
   }

   /**
    * @param defaultLinkColorSource the defaultLinkColorSource to set
    */
   public void setDefaultLinkColorSource(int defaultLinkColorSource)
   {
      this.defaultLinkColorSource = defaultLinkColorSource;
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

   /**
    * Check if long object names are enabled.
    *
    * @return true if long object names are enabled
    */
   public boolean areLongObjectNamesEnabled()
   {
      return enableLongObjectName;
   }

   /**
    * @return the connectionLabelsVisible
    */
   public boolean areConnectionLabelsVisible()
   {
      return connectionLabelsVisible;
   }

   /**
    * @param connectionLabelsVisible the connectionLabelsVisible to set
    */
   public void setConnectionLabelsVisible(boolean connectionLabelsVisible)
   {
      this.connectionLabelsVisible = connectionLabelsVisible;
   }

   /**
    * @return the connectionsVisible
    */
   public boolean areConnectionsVisible()
   {
      return connectionsVisible;
   }

   /**
    * @param connectionsVisible the connectionsVisible to set
    */
   public void setConnectionsVisible(boolean connectionsVisible)
   {
      this.connectionsVisible = connectionsVisible;
   }

   /**
    * Get owning viewer.
    *
    * @return the viewer owning viewer
    */
   public ExtendedGraphViewer getViewer()
   {
      return viewer;
   }
}
