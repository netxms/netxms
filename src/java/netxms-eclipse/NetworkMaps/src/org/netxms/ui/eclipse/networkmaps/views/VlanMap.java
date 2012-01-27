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

import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.NXCException;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.Port;
import org.netxms.client.topology.VlanInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;

/**
 * IP neighbors for given node
 *
 */
public class VlanMap extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.VlanMap";
	
	private int vlanId;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		// This view expects secondary ID to be in form nodeId&vlanId
		String[] parts = site.getSecondaryId().split("&");
		if (parts.length < 2)
			throw new PartInitException("Internal error: incorrect view secondary ID");
		try
		{
			vlanId = Integer.parseInt(parts[1]);
		}
		catch(NumberFormatException e)
		{
			throw new PartInitException("Internal error: incorrect view secondary ID", e);
		}
		setPartName("Vlan Map - " + Integer.toString(vlanId) + "@" + rootObject.getObjectName());
	}

	/**
	 * Build map page
	 */
	protected void buildMapPage()
	{
		if (mapPage == null)
			mapPage = new NetworkMapPage();
		
		new ConsoleJob("Get VLAN information for " + rootObject.getObjectName(), this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NetworkMapPage page = new NetworkMapPage();
				collectVlanInfo(page, (Node)rootObject);
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected void jobFailureHandler()
			{
				// On failure, create map with root object only
				NetworkMapPage page = new NetworkMapPage();
				page.addElement(new NetworkMapObject(mapPage.createElementId(), rootObject.getObjectId()));
				replaceMapPage(page, getDisplay());
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get VLAN information for " + rootObject.getObjectName();
			}
		}.start();
	}
	
	/**
	 * Recursively collect VLAN information starting from given node
	 * 
	 * @param page map page
	 * @param root root node
	 * @throws Exception 
	 * @throws NXCException 
	 */
	private long collectVlanInfo(NetworkMapPage page, Node root) throws Exception
	{
		if (page.findObjectElement(root.getObjectId()) != null)
			return -1;
		
		long rootElementId = mapPage.createElementId();
		page.addElement(new NetworkMapObject(rootElementId, root.getObjectId()));
		
		List<VlanInfo> vlans = session.getVlans(root.getObjectId());
		for(VlanInfo vlan : vlans)
		{
			if (vlan.getVlanId() == vlanId)
			{
				Port[] ports = vlan.getPorts();
				for(Port port : ports)
				{
					processVlanPort(page, root, port, rootElementId);
				}
				break;
			}
		}
		
		return rootElementId;
	}
	
	/**
	 * Process single member port of VLAN. Will add connected switch on other site to the map.
	 * 
	 * @param page
	 * @param root
	 * @param port
	 * @throws Exception 
	 */
	private void processVlanPort(NetworkMapPage page, Node root, Port port, long rootElementId) throws Exception
	{
		Interface iface = (Interface)session.findObjectById(port.getObjectId(), Interface.class);
		if (iface != null)
		{
			Node peerNode = (Node)session.findObjectById(iface.getPeerNodeId(), Node.class);
			if ((peerNode != null) && ((peerNode.getFlags() & Node.NF_IS_BRIDGE) != 0))
			{
				try
				{
					long nodeElementId = collectVlanInfo(page, peerNode);
					if (nodeElementId != -1)
					{
						Interface peerIf = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
						page.addLink(new NetworkMapLink(null, NetworkMapLink.NORMAL, rootElementId, nodeElementId,
								       iface.getObjectName(), (peerIf != null) ? peerIf.getObjectName() : "???"));
					}
				}
				catch(NXCException e)
				{
					// Ignore NetXMS errors for remote bridges 
				}
			}
		}
	}
}
