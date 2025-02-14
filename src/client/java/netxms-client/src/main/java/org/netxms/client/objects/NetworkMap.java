/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.MapType;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Network map object
 */
public class NetworkMap extends GenericObject implements AutoBindObject, PollingTarget
{
	public static final UUID GEOMAP_BACKGROUND = UUID.fromString("ffffffff-ffff-ffff-ffff-ffffffffffff"); 

	public static final int MF_SHOW_STATUS_ICON        = 0x000001;
	public static final int MF_SHOW_STATUS_FRAME       = 0x000002;
	public static final int MF_SHOW_STATUS_BKGND       = 0x000004;
	public static final int MF_SHOW_END_NODES          = 0x000008;
	public static final int MF_CALCULATE_STATUS        = 0x000010;
   public static final int MF_FILTER_OBJECTS          = 0x000020;
   public static final int MF_SHOW_LINK_DIRECTION     = 0x000040;
   public static final int MF_USE_L1_TOPOLOGY         = 0x000080;
   public static final int MF_CENTER_BKGND_IMAGE      = 0x000100;
   public static final int MF_TRANSLUCENT_LABEL_BKGND = 0x000200;
   public static final int MF_DONT_UPDATE_LINK_TEXT   = 0x000400;
   public static final int MF_FIT_BKGND_IMAGE         = 0x000800;
   public static final int MF_FIT_TO_SCREEN           = 0x001000;
   public final static int MF_SHOW_AS_OBJECT_VIEW     = 0x002000;
   public final static int MF_SHOW_TRAFFIC             = 0x004000;

   public static final int MF_BKGND_IMAGE_FLAGS       = 0x000900;

   private MapType mapType;
	private MapLayoutAlgorithm layout;
	private UUID background;
	private GeoLocation backgroundLocation;
	private int backgroundZoom;
   private int mapWidth;
   private int mapHeight;
   private List<Long> seedObjects;
	private int defaultLinkColor;
	private int defaultLinkRouting;
	private int defaultLinkWidth;
	private int defaultLinkStyle;
	private MapObjectDisplayMode objectDisplayMode;
	private int backgroundColor;
	private int discoveryRadius;
	private String filter;
   private String linkStylingScript;
	private List<NetworkMapElement> elements;
	private List<NetworkMapLink> links;
   private boolean updateFailed;
   private int autoBindFlags;
   private String autoBindFilter;
   private int displayPriority; // For maps shown in object context

	/**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
	 */
	public NetworkMap(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
      mapType = MapType.getByValue(msg.getFieldAsInt16(NXCPCodes.VID_MAP_TYPE));
		layout = MapLayoutAlgorithm.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_LAYOUT));
		background = msg.getFieldAsUUID(NXCPCodes.VID_BACKGROUND);
		backgroundLocation = new GeoLocation(msg.getFieldAsDouble(NXCPCodes.VID_BACKGROUND_LATITUDE), msg.getFieldAsDouble(NXCPCodes.VID_BACKGROUND_LONGITUDE));
		backgroundZoom = msg.getFieldAsInt32(NXCPCodes.VID_BACKGROUND_ZOOM);
      mapWidth = msg.getFieldAsInt32(NXCPCodes.VID_WIDTH);
      mapHeight = msg.getFieldAsInt32(NXCPCodes.VID_HEIGHT);
      seedObjects = Arrays.asList(msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_SEED_OBJECTS));
		defaultLinkColor = msg.getFieldAsInt32(NXCPCodes.VID_LINK_COLOR);
		defaultLinkRouting = msg.getFieldAsInt32(NXCPCodes.VID_LINK_ROUTING);
      defaultLinkStyle = msg.getFieldAsInt32(NXCPCodes.VID_LINK_STYLE);
      defaultLinkWidth = msg.getFieldAsInt32(NXCPCodes.VID_LINK_WIDTH);
		objectDisplayMode = MapObjectDisplayMode.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DISPLAY_MODE));
		backgroundColor = msg.getFieldAsInt32(NXCPCodes.VID_BACKGROUND_COLOR);
		discoveryRadius = msg.getFieldAsInt32(NXCPCodes.VID_DISCOVERY_RADIUS);
		filter = msg.getFieldAsString(NXCPCodes.VID_FILTER);
      linkStylingScript = msg.getFieldAsString(NXCPCodes.VID_LINK_STYLING_SCRIPT);
      updateFailed = msg.getFieldAsBoolean(NXCPCodes.VID_UPDATE_FAILED);
      autoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      autoBindFlags = msg.getFieldAsInt32(NXCPCodes.VID_AUTOBIND_FLAGS);
      displayPriority = msg.getFieldAsInt32(NXCPCodes.VID_DISPLAY_PRIORITY);

		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
		elements = new ArrayList<NetworkMapElement>(count);
		long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			elements.add(NetworkMapElement.createMapElement(msg, fieldId));
			fieldId += 100;
		}

		count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_LINKS);
		links = new ArrayList<NetworkMapLink>(count);
		fieldId = NXCPCodes.VID_LINK_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			links.add(new NetworkMapLink(msg, fieldId));
			fieldId += 20;
		}
	}

	/**
	 * Prepare object creation and modification data to create map copy
	 * 
	 * @param cd object creation data
	 * @param md object modification data
	 */
	public void prepareCopy(NXCObjectCreationData cd, NXCObjectModificationData md)
	{
	   cd.setMapType(mapType);
      cd.setSeedObjectIds(seedObjects);

	   md.setMapLayout(layout);
	   md.setMapBackground(background, backgroundLocation, backgroundZoom, backgroundColor);
      md.setDiscoveryRadius(discoveryRadius);
      md.setFilter(filter);
      md.setLinkStylingScript(linkStylingScript);
	   md.setMapContent(elements, links);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "NetworkMap";
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

	/**
	 * @return the mapType
	 */
   public MapType getMapType()
	{
		return mapType;
	}

	/**
	 * @return the layout
	 */
	public MapLayoutAlgorithm getLayout()
	{
		return layout;
	}

	/**
	 * @return the background
	 */
	public UUID getBackground()
	{
		return background;
	}

	/**
	 * @return the seedObjectIds
	 */
   public List<Long> getSeedObjects()
	{
      return seedObjects;
	}

	/**
	 * Create map page from map object's data
	 * 
	 * @return new map page
	 */
	public NetworkMapPage createMapPage()
	{
		NetworkMapPage page = new NetworkMapPage(getObjectName(), objectId);
		page.addAllElements(elements);
		page.addAllLinks(links);
		return page;
	}

	/**
	 * @return the backgroundLocation
	 */
	public GeoLocation getBackgroundLocation()
	{
		return backgroundLocation;
	}

	/**
	 * @return the backgroundZoom
	 */
	public int getBackgroundZoom()
	{
		return backgroundZoom;
	}

	/**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return the defaultLinkColor
	 */
	public int getDefaultLinkColor()
	{
		return defaultLinkColor;
	}

	/**
	 * @return the defaultLinkRouting
	 */
	public int getDefaultLinkRouting()
	{
		return defaultLinkRouting;
	}

	/**
    * @return the defaultLinkWidth
    */
   public int getDefaultLinkWidth()
   {
      return defaultLinkWidth;
   }

   /**
    * @return the defaultLinkStyle
    */
   public int getDefaultLinkStyle()
   {
      return defaultLinkStyle;
   }

   /**
	 * @return the backgroundColor
	 */
	public int getBackgroundColor()
	{
		return backgroundColor;
	}

	/**
	 * @return the discoveryRadius
	 */
	public final int getDiscoveryRadius()
	{
		return discoveryRadius;
	}

	/**
	 * @return the filter
	 */
	public String getFilter()
	{
		return filter;
	}
	
   /**
    * @return the updateLinkScript
    */
   public String getLinkStylingScript()
   {
      return linkStylingScript;
   }

   /**
    * @return the objectDisplayMode
    */
   public MapObjectDisplayMode getObjectDisplayMode()
   {
      return objectDisplayMode;
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, filter);
      addString(strings, linkStylingScript);
      return strings;
   }

   /**
    * Returns the MF_SHOW_STATUS_ICON flag status
    * 
    * @return true if MF_SHOW_STATUS_ICON flag is set
    */
   public boolean isShowStatusIcon()
   {
      return (flags & NetworkMap.MF_SHOW_STATUS_ICON) != 0;
   }

   /**
    * Returns the MF_SHOW_STATUS_FRAME flag status
    * 
    * @return true if MF_SHOW_STATUS_FRAME flag is set
    */
   public boolean isShowStatusFrame()
   {
      return (flags & NetworkMap.MF_SHOW_STATUS_FRAME) != 0;
   }

   /**
    * Returns the MF_SHOW_STATUS_BKGND flag status
    * 
    * @return true if MF_SHOW_STATUS_BKGND flag is set
    */
   public boolean isShowStatusBackground()
   {
      return (flags & NetworkMap.MF_SHOW_STATUS_BKGND) != 0;
   }

   /**
    * Returns the MF_SHOW_END_NODES flag status
    * 
    * @return true if MF_SHOW_END_NODES flag is set
    */
   public boolean isShowEndNodes()
   {
      return (flags & NetworkMap.MF_SHOW_END_NODES) != 0;
   }

   /**
    * Returns the MF_SHOW_LINK_DIRECTION flag status
    * 
    * @return true if MF_SHOW_LINK_DIRECTION flag is set
    */
   public boolean isShowLinkDirection()
   {
      return (flags & NetworkMap.MF_SHOW_LINK_DIRECTION) != 0;
   }

   /**
    * Returns the MF_CALCULATE_STATUS flag status
    * 
    * @return true if MF_CALCULATE_STATUS flag is set
    */
   public boolean isCalculateStatus()
   {
      return (flags & NetworkMap.MF_CALCULATE_STATUS) != 0;
   }

   /**
    * Returns the MF_CENTER_BKGND_IMAGE flag status
    * 
    * @return true if MF_CENTER_BKGND_IMAGE flag is set
    */
   public boolean isCenterBackgroundImage()
   {
      return (flags & NetworkMap.MF_CENTER_BKGND_IMAGE) != 0;
   }

   /**
    * Returns the MF_TRANSLUCENT_LABEL_BKGND flag status
    * 
    * @return true if MF_TRANSLUCENT_LABEL_BKGND flag is set
    */
   public boolean isTranslucentLabelBackground()
   {
      return (flags & NetworkMap.MF_TRANSLUCENT_LABEL_BKGND) != 0;
   }

   /**
    * Returns the MF_DONT_UPDATE_LINK_TEXT flag status
    * 
    * @return true if MF_DONT_UPDATE_LINK_TEXT flag is set
    */
   public boolean isDontUpdateLinkText()
   {
      return (flags & NetworkMap.MF_DONT_UPDATE_LINK_TEXT) != 0;
   }

   /**
    * @return true if background image should fit map size
    */
   public boolean isFitBackgroundImage()
   {
      return (flags & NetworkMap.MF_FIT_BKGND_IMAGE) != 0;
   }

   /**
    * Returns the MF_FIT_TO_SCREEN flag status
    * 
    * @return true if MF_FIT_TO_SCREEN flag is set
    */
   public boolean isFitToScreen()
   {
      return (flags & NetworkMap.MF_FIT_TO_SCREEN) != 0;
   }

   /**
    * @return the mapWidth
    */
   public int getWidth()
   {
      return mapWidth;
   }

   /**
    * @return the mapHeight
    */
   public int getHeight()
   {
      return mapHeight;
   }

   /**
    * Get map's display priority if it should be shown as object view.
    *
    * @return map's display priority
    */
   public int getDisplayPriority()
   {
      return displayPriority;
   }

   /**
    * Check if last map update failed.
    *
    * @return true if last map update failed
    */
   public boolean isUpdateFailed()
   {
      return updateFailed;
   }

   /**
    * Update modification data with template fields form this map
    * 
    * @param md modification data
    */
   public void updateWithTemplateData(NXCObjectModificationData md)
   {
      md.setObjectFlags(flags);
      md.setMapBackground(background, backgroundLocation, backgroundZoom, backgroundColor);
      md.setMapSize(mapWidth, mapHeight);
      md.setFilter(filter);      
      md.setMapObjectDisplayMode(objectDisplayMode);
      md.setConnectionRouting(defaultLinkRouting);
      md.setLinkColor(defaultLinkColor);
      md.setNetworkMapLinkWidth(defaultLinkWidth);
      md.setNetworkMapLinkStyle(defaultLinkStyle);
      md.setDiscoveryRadius(discoveryRadius);
      md.setLinkStylingScript(linkStylingScript);

      List<NetworkMapElement> result = new ArrayList<>(elements.size());
      for(NetworkMapElement e : elements)
         if ((e.getType() == NetworkMapElement.MAP_ELEMENT_DECORATION) || (e.getType() == NetworkMapElement.MAP_ELEMENT_TEXT_BOX))
            result.add(e);

      md.setMapContent(result, new ArrayList<NetworkMapLink>(0));
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseModbus()
    */
   @Override
   public boolean canUseModbus()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollStates()
    */
   @Override
   public PollState[] getPollStates()
   {
      return pollStates;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoBindEnabled()
    */
   @Override
   public boolean isAutoBindEnabled()
   {
      return (autoBindFlags & OBJECT_BIND_FLAG) != 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#isAutoUnbindEnabled()
    */
   @Override
   public boolean isAutoUnbindEnabled()
   {
      return (autoBindFlags & OBJECT_UNBIND_FLAG) != 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#getAutoBindFilter()
    */
   @Override
   public String getAutoBindFilter()
   {
      return autoBindFilter;
   }

   /**
    * @see org.netxms.client.objects.interfaces.AutoBindObject#getAutoBindFlags()
    */
   @Override
   public int getAutoBindFlags()
   {
      return autoBindFlags;
   }
}
