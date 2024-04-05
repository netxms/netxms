/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.ClusterResource;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.ClusterResourceListComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ClusterResourceListFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ClusterResourceListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * "Resources" cluster resources
 */
public class ResourcesView extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(ResourcesView.class);
   
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VIP = 1;
	public static final int COLUMN_OWNER = 2;
	
	private SortableTableViewer resourceList;
	private Cluster cluster;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public ResourcesView()
   {
      super(LocalizationHelper.getI18n(ResourcesView.class).tr("Resources"), ResourceManager.getImageDescriptor("icons/object-views/cluster.png"), "objects.resources", true);
   }	
	
   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{		
		final String[] names = { i18n.tr("Resource"), i18n.tr("VIP"), i18n.tr("Owner") };
		final int[] widths = { 200, 120, 150 };
		resourceList = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		resourceList.setContentProvider(new ArrayContentProvider());
		resourceList.setLabelProvider(new ClusterResourceListLabelProvider());
      resourceList.setComparator(new ClusterResourceListComparator());
      ClusterResourceListFilter filter = new ClusterResourceListFilter();
      resourceList.addFilter(filter);
		setFilterClient(resourceList, filter);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
	{
		if (object instanceof Cluster)
		{
			cluster = (Cluster)object;
			resourceList.setInput(cluster.getResources().toArray());
		}
		else
		{
			cluster = null;
			resourceList.setInput(new ClusterResource[0]);
		}
	}

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 15;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
	{
		return (context != null) && (context instanceof Cluster);
	}
}
