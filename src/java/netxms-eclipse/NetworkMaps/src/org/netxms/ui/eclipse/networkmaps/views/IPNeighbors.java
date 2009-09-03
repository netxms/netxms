/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views;

import java.util.Iterator;

import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.NXCNode;
import org.netxms.client.NXCObject;
import org.netxms.client.NXCSubnet;
import org.netxms.client.maps.NetworkMapObjectData;
import org.netxms.client.maps.NetworkMapObjectLink;
import org.netxms.client.maps.NetworkMapPage;

/**
 * @author Victor
 *
 */
public class IPNeighbors extends NetworkMap
{
	public static final String ID = "org.netxms.ui.eclipse.quickmaps.view.ip_neighbors";
	
	/**
	 * 
	 */
	public IPNeighbors()
	{
		super();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		setPartName("IP Neighbors - " + ((node != null) ? node.getObjectName() : "<error>"));
	}

	/**
	 * Build map page
	 */
	protected void buildMapPage()
	{
		mapPage = new NetworkMapPage();

		mapPage.addObject(new NetworkMapObjectData(node.getObjectId()));
		
		Iterator<Long> it = node.getParents();
		while(it.hasNext())
		{
			long id = it.next();
			NXCObject object = session.findObjectById(id);
			if ((object != null) && (object instanceof NXCSubnet))
			{
				mapPage.addObject(new NetworkMapObjectData(id));
				mapPage.addLink(new NetworkMapObjectLink(NetworkMapObjectLink.NORMAL, node.getObjectId(), id));
				addNodesFromSubnet((NXCSubnet)object, node.getObjectId());
			}
		}
	}

	/**
	 * Add nodes connected to given subnet to map
	 * @param subnet Subnet object
	 * @param rootNodeId ID of map's root node (used to prevent recursion)
	 */
	private void addNodesFromSubnet(NXCSubnet subnet, long rootNodeId)
	{
		Iterator<Long> it = subnet.getChilds();
		while(it.hasNext())
		{
			long id = it.next();
			if (id != rootNodeId)
			{
				NXCObject object = session.findObjectById(id);
				if ((object != null) && (object instanceof NXCNode))
				{
					mapPage.addObject(new NetworkMapObjectData(id));
					mapPage.addLink(new NetworkMapObjectLink(NetworkMapObjectLink.NORMAL, subnet.getObjectId(), id));
				}
			}
		}
	}
}
