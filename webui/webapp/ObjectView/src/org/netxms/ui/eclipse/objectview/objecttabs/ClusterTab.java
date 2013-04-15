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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapResource;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.networkmaps.widgets.NetworkMapWidget;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.ClusterResourceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.ClusterResourceListLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Cluster" tab
 */
public class ClusterTab extends ObjectTab
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VIP = 1;
	public static final int COLUMN_OWNER = 2;
	
	private NetworkMapWidget clusterMap;
	private SortableTableViewer resourceList;
	private Cluster cluster;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		final Composite content = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		content.setLayout(layout);
		
		clusterMap = new NetworkMapWidget(content, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		clusterMap.setLayoutData(gd);
		clusterMap.setMapLayout(NetworkMap.LAYOUT_VTREE);
		
		final String[] names = { "Resource", "VIP", "Owner" };
		final int[] widths = { 200, 120, 150 };
		resourceList = new SortableTableViewer(content, names, widths, COLUMN_NAME, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		resourceList.setContentProvider(new ArrayContentProvider());
		resourceList.setLabelProvider(new ClusterResourceListLabelProvider());
		resourceList.setComparator(new ClusterResourceListComparator());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = false;
		resourceList.getControl().setLayoutData(gd);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(AbstractObject object)
	{
		if (object instanceof Cluster)
		{
			cluster = (Cluster)object;
			resourceList.setInput(cluster.getResources().toArray());
			clusterMap.setContent(buildClusterMap());
		}
		else
		{
			cluster = null;
			resourceList.setInput(new ClusterResource[0]);
			clusterMap.setContent(new NetworkMapPage());
		}
		getClientArea().layout(true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return object instanceof Cluster;
	}
	
	/**
	 * Build hierarchical cluster representation
	 * 
	 * @return
	 */
	private NetworkMapPage buildClusterMap()
	{
		NetworkMapPage page = new NetworkMapPage();
		long id = 1;
		
		page.addElement(new NetworkMapObject(id++, cluster.getObjectId()));
		
		for(AbstractObject o : cluster.getAllChilds(AbstractObject.OBJECT_NODE))
		{
			page.addElement(new NetworkMapObject(id, o.getObjectId()));
			page.addLink(new NetworkMapLink(0, 1, id));
			addOwnedResources(page, id, o.getObjectId(), cluster.getResources());
			id++;
		}
		
		return page;
	}

	/**
	 * @param id
	 * @param objectId
	 * @param resources
	 */
	private void addOwnedResources(NetworkMapPage page, long rootElementId, long objectId, List<ClusterResource> resources)
	{
		long elementId = rootElementId * 100000;
		for(ClusterResource r : resources)
		{
			if (r.getCurrentOwner() == objectId)
			{
				page.addElement(new NetworkMapResource(elementId, NetworkMapResource.CLUSTER_RESOURCE, r));
				page.addLink(new NetworkMapLink(0, rootElementId, elementId));
				elementId++;
			}
		}
	}
}
