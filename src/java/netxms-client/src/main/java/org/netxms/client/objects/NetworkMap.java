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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Network map object
 *
 */
public class NetworkMap extends GenericObject
{
	public static final UUID GEOMAP_BACKGROUND = UUID.fromString("ffffffff-ffff-ffff-ffff-ffffffffffff"); 

	public static final int TYPE_CUSTOM = 0;
	public static final int TYPE_LAYER2_TOPOLOGY = 1;
	public static final int TYPE_IP_TOPOLOGY = 2;
	
	public static final int LAYOUT_MANUAL = 0x7FFF;
	public static final int LAYOUT_SPRING = 0;
	public static final int LAYOUT_RADIAL = 1;
	public static final int LAYOUT_HTREE = 2;
	public static final int LAYOUT_VTREE = 3;
	public static final int LAYOUT_SPARSE_VTREE = 4;
	
	public static final int MF_SHOW_STATUS_ICON  = 0x000001;
	public static final int MF_SHOW_STATUS_FRAME = 0x000002;
	public static final int MF_SHOW_STATUS_BKGND = 0x000004;
	public static final int MF_SHOW_END_NODES    = 0x000008;
	
	private int mapType;
	private int layout;
	private int flags;
	private UUID background;
	private GeoLocation backgroundLocation;
	private int backgroundZoom;
	private long seedObjectId;
	private int defaultLinkColor;
	private int defaultLinkRouting;
	private int backgroundColor;
	private int discoveryRadius;
	private List<NetworkMapElement> elements;
	private List<NetworkMapLink> links;
	
	/**
	 * @param msg
	 * @param session
	 */
	public NetworkMap(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		mapType = msg.getVariableAsInteger(NXCPCodes.VID_MAP_TYPE);
		layout = msg.getVariableAsInteger(NXCPCodes.VID_LAYOUT);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		background = msg.getVariableAsUUID(NXCPCodes.VID_BACKGROUND);
		backgroundLocation = new GeoLocation(msg.getVariableAsReal(NXCPCodes.VID_BACKGROUND_LATITUDE), msg.getVariableAsReal(NXCPCodes.VID_BACKGROUND_LONGITUDE));
		backgroundZoom = msg.getVariableAsInteger(NXCPCodes.VID_BACKGROUND_ZOOM);
		seedObjectId = msg.getVariableAsInt64(NXCPCodes.VID_SEED_OBJECT);
		defaultLinkColor = msg.getVariableAsInteger(NXCPCodes.VID_LINK_COLOR);
		defaultLinkRouting = msg.getVariableAsInteger(NXCPCodes.VID_LINK_ROUTING);
		backgroundColor = msg.getVariableAsInteger(NXCPCodes.VID_BACKGROUND_COLOR);
		discoveryRadius = msg.getVariableAsInteger(NXCPCodes.VID_DISCOVERY_RADIUS);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ELEMENTS);
		elements = new ArrayList<NetworkMapElement>(count);
		long varId = NXCPCodes.VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			elements.add(NetworkMapElement.createMapElement(msg, varId));
			varId += 100;
		}
		
		count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_LINKS);
		links = new ArrayList<NetworkMapLink>(count);
		varId = NXCPCodes.VID_LINK_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			links.add(new NetworkMapLink(msg, varId));
			varId += 10;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "NetworkMap";
	}

	/* (non-Javadoc)
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
	public int getMapType()
	{
		return mapType;
	}

	/**
	 * @return the layout
	 */
	public int getLayout()
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
	 * @return the seedObjectId
	 */
	public long getSeedObjectId()
	{
		return seedObjectId;
	}
	
	/**
	 * Create map page from map object's data
	 * 
	 * @return new map page
	 */
	public NetworkMapPage createMapPage()
	{
		NetworkMapPage page = new NetworkMapPage(getObjectName());
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
	 * @return the flags
	 */
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
}
