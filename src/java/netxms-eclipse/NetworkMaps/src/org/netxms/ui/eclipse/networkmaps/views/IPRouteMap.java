/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.topology.HopInfo;
import org.netxms.client.topology.NetworkPath;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;

/**
 * IP route map
 */
public class IPRouteMap extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.IPRouteMap";
	
	private GenericObject targetObject;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName("IP Route Map - [" + rootObject.getObjectName() + " - " + targetObject.getObjectName() + "]");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#parseSecondaryId(java.lang.String[])
	 */
	@Override
	protected void parseSecondaryId(String[] parts) throws PartInitException
	{
		super.parseSecondaryId(parts);
		
		if (parts.length < 2)
			throw new PartInitException("Incorrect view invocation");
		
		targetObject = session.findObjectById(Long.parseLong(parts[1]));
		if (targetObject == null)
			throw new PartInitException("Target object does not exist or cannot be accessed");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		super.createPartControl(parent);
		setLayoutAlgorithm(LAYOUT_HTREE);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.networkmaps.views.NetworkMap#buildMapPage()
	 */
	@Override
	protected void buildMapPage()
	{
		if (mapPage == null)
			mapPage = new NetworkMapPage();
		
		new ConsoleJob("Get IP route " + rootObject.getObjectName() + " - " + targetObject.getObjectName(), 
				this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				getRoute();
			}

			@Override
			protected void jobFailureHandler()
			{
				// On failure, create map with root object only
				NetworkMapPage page = new NetworkMapPage();
				page.addElement(new NetworkMapObject(mapPage.createElementId(), rootObject.getObjectId()));
				replaceMapPage(page);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get route information for " + rootObject.getObjectName() + " - " + targetObject.getObjectName();
			}
		}.start();
	}
	
	/**
	 * Get route between objects and build map page
	 * 
	 * @throws Exception 
	 */
	private void getRoute() throws Exception
	{
		final NetworkPath path = session.getNetworkPath(rootObject.getObjectId(), targetObject.getObjectId());
		final NetworkMapPage page = new NetworkMapPage();
		long prevElementId = 0;
		for(final HopInfo h : path.getPath())
		{
			final long elementId = page.createElementId();
			page.addElement(new NetworkMapObject(elementId, h.getNodeId()));
			if (prevElementId != 0)
				page.addLink(new NetworkMapLink(NetworkMapLink.NORMAL, prevElementId, elementId));
			prevElementId = elementId;
		}
		replaceMapPage(page);
	}
}
