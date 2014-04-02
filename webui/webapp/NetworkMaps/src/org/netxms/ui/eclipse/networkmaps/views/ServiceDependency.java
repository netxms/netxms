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
import org.netxms.client.objects.Container;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.networkmaps.Messages;

/**
 * Service dependency for service, cluster, condition, or node object
 *
 */
public class ServiceDependency extends AbstractNetworkMapView
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.ServiceDependency"; //$NON-NLS-1$

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName(Messages.get().ServiceDependency_PartTitle + ((rootObject != null) ? rootObject.getObjectName() : Messages.get().ServiceDependency_Error));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#buildMapPage()
	 */
	@Override
	protected void buildMapPage()
	{
		mapPage = new NetworkMapPage(ID+rootObject.getObjectId());
		long elementId = mapPage.createElementId();
		mapPage.addElement(new NetworkMapObject(elementId, rootObject.getObjectId()));
		addParentServices(rootObject, elementId);
      addDciToRequestList();
	}

	/**
	 * Add parent services for given object
	 * 
	 * @param object
	 */
	private void addParentServices(AbstractObject object, long parentElementId)
	{
		Iterator<Long> it = object.getParents();
		while(it.hasNext())
		{
			long objectId = it.next();
			AbstractObject parent = session.findObjectById(objectId);
			if ((parent != null) && ((parent instanceof Container) || (parent instanceof Cluster)))
			{
				long elementId = mapPage.createElementId();
				mapPage.addElement(new NetworkMapObject(elementId, objectId));
				mapPage.addLink(new NetworkMapLink(NetworkMapLink.NORMAL, parentElementId, elementId));
				addParentServices(parent, elementId);
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
		setLayoutAlgorithm(LAYOUT_VTREE, true);
	}
}
