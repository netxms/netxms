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
package org.netxms.ui.eclipse.networkmaps.views;

import java.util.Iterator;

import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Subnet;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * IP neighbors for given node
 *
 */
public class IPNeighbors extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.view.ip_neighbors";
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName("IP Neighbors - " + rootObject.getObjectName());
	}

	/**
	 * Build map page
	 */
	protected void buildMapPage()
	{
		mapPage = new NetworkMapPage();

		long rootElementId = mapPage.createElementId();
		mapPage.addElement(new NetworkMapObject(rootElementId, rootObject.getObjectId()));
		
		Iterator<Long> it = rootObject.getParents();
		while(it.hasNext())
		{
			long objectId = it.next();
			GenericObject object = session.findObjectById(objectId);
			if ((object != null) && (object instanceof Subnet))
			{
				long elementId = mapPage.createElementId();
				mapPage.addElement(new NetworkMapObject(elementId, objectId));
				mapPage.addLink(new NetworkMapLink(NetworkMapLink.NORMAL, rootElementId, elementId));
				addNodesFromSubnet((Subnet)object, elementId, rootObject.getObjectId());
			}
		}
	}

	/**
	 * Add nodes connected to given subnet to map
	 * @param subnet Subnet object
	 * @param rootNodeId ID of map's root node (used to prevent recursion)
	 */
	private void addNodesFromSubnet(Subnet subnet, long subnetElementId, long rootNodeId)
	{
		Iterator<Long> it = subnet.getChildren();
		while(it.hasNext())
		{
			long objectId = it.next();
			if (objectId != rootNodeId)
			{
				GenericObject object = session.findObjectById(objectId);
				if ((object != null) && (object instanceof Node))
				{
					long elementId = mapPage.createElementId();
					mapPage.addElement(new NetworkMapObject(elementId, objectId));
					mapPage.addLink(new NetworkMapLink(NetworkMapLink.NORMAL, subnetElementId, elementId));
				}
			}
		}
	}
}
