/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * Utility class for editing map links 
 */
public class LinkEditor
{
	private NetworkMapLink link;
	private NetworkMapPage mapPage;
	private String name;
	private int type;
	private String connectorName1;
	private String connectorName2;
	private int color;
	private long statusObject;
	private int routingAlgorithm;
	private boolean modified = false;
	
	/**
	 * @param link
	 * @param mapPage
	 */
	public LinkEditor(NetworkMapLink link, NetworkMapPage mapPage)
	{
		this.link = link;
		this.mapPage = mapPage;
		name = link.getName();
		type = link.getType();
		connectorName1 = link.getConnectorName1();
		connectorName2 = link.getConnectorName2();
		color = link.getColor();
		statusObject = link.getStatusObject();
		routingAlgorithm = link.getRouting();
	}
	
	/**
	 * 
	 */
	public void update()
	{
		mapPage.removeLink(link);
		link = new NetworkMapLink(name, type, link.getElement1(), link.getElement2(), connectorName1, connectorName2);
		link.setColor(color);
		link.setStatusObject(statusObject);
		link.setRouting(routingAlgorithm);
		mapPage.addLink(link);
		modified = true;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @return the connectorName1
	 */
	public String getConnectorName1()
	{
		return connectorName1;
	}

	/**
	 * @param connectorName1 the connectorName1 to set
	 */
	public void setConnectorName1(String connectorName1)
	{
		this.connectorName1 = connectorName1;
	}

	/**
	 * @return the connectorName2
	 */
	public String getConnectorName2()
	{
		return connectorName2;
	}

	/**
	 * @param connectorName2 the connectorName2 to set
	 */
	public void setConnectorName2(String connectorName2)
	{
		this.connectorName2 = connectorName2;
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return color;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
		this.color = color;
	}

	/**
	 * @return the statusObject
	 */
	public long getStatusObject()
	{
		return statusObject;
	}

	/**
	 * @param statusObject the statusObject to set
	 */
	public void setStatusObject(long statusObject)
	{
		this.statusObject = statusObject;
	}

	/**
	 * @return the routingAlgorithm
	 */
	public int getRoutingAlgorithm()
	{
		return routingAlgorithm;
	}

	/**
	 * @param routingAlgorithm the routingAlgorithm to set
	 */
	public void setRoutingAlgorithm(int routingAlgorithm)
	{
		this.routingAlgorithm = routingAlgorithm;
	}

	/**
	 * @return the mapPage
	 */
	public NetworkMapPage getMapPage()
	{
		return mapPage;
	}
	
	/**
	 * @return
	 */
	public long getObject1()
	{
		NetworkMapObject e = (NetworkMapObject)mapPage.getElement(link.getElement1(), NetworkMapObject.class);
		return (e != null) ? e.getObjectId() : 0;
	}

	/**
	 * @return
	 */
	public long getObject2()
	{
		NetworkMapObject e = (NetworkMapObject)mapPage.getElement(link.getElement2(), NetworkMapObject.class);
		return (e != null) ? e.getObjectId() : 0;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}
}
