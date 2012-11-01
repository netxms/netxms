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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;

/**
 * Service dependency for service object
 *
 */
public class ServiceComponents extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.ServiceComponents";

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName("Service Components - " + ((rootObject != null) ? rootObject.getObjectName() : "<error>"));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#buildMapPage()
	 */
	@Override
	protected void buildMapPage()
	{
		mapPage = new NetworkMapPage();
		long elementId = mapPage.createElementId();
		mapPage.addElement(new NetworkMapObject(elementId, rootObject.getObjectId()));
		addServiceComponents(rootObject, elementId);
	}

	/**
	 * Add parent services for given object
	 * 
	 * @param object
	 */
	private void addServiceComponents(GenericObject object, long parentElementId)
	{
		Iterator<Long> it = object.getChilds();
		while(it.hasNext())
		{
			long objectId = it.next();
			GenericObject child = session.findObjectById(objectId);
			if ((child != null) && 
					((child instanceof Container) || 
					 (child instanceof Cluster) || 
					 (child instanceof Node) ||
					 (child instanceof Condition)))
			{
				long elementId = mapPage.createElementId();
				mapPage.addElement(new NetworkMapObject(elementId, objectId));
				mapPage.addLink(new NetworkMapLink(NetworkMapLink.NORMAL, parentElementId, elementId));
				addServiceComponents(child, elementId);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		super.createPartControl(parent);
		setLayoutAlgorithm(LAYOUT_SPARSE_VTREE, true);
	}
}
