/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.propertypages.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.propertypages.LinkDataSources;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for DCI list on property page for map link
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private I18n i18n = LocalizationHelper.getI18n(DciListLabelProvider.class);
   private NXCSession session = Registry.getSession();
	private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();
	private List<SingleDciConfig> elementList;
	
	/**
	 * Constructor for DciListLabelProvider class
	 */
	public DciListLabelProvider(List<SingleDciConfig> elementList)
	{
		this.elementList = elementList;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
	   SingleDciConfig dci = (SingleDciConfig)element;
		switch(columnIndex)
		{
			case LinkDataSources.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case LinkDataSources.COLUMN_NODE:
				AbstractObject object = session.findObjectById(dci.getNodeId());
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.getNodeId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case LinkDataSources.COLUMN_METRIC:
				String name = dciNameCache.get(dci.dciId).displayName;
            return (name != null) ? name : i18n.tr("Unresolved DCI name");
			case LinkDataSources.COLUMN_LABEL:
				return dci.name;
		}
		return null;
	}

	/**
	 * Resolve DCI names for given collection of condition DCIs and add to cache
	 * 
	 * @param dciList
	 */
	public void resolveDciNames(final Collection<SingleDciConfig> dciList)
	{
      new Job(i18n.tr("Resolve DCI names"), null) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Map<Long, DciInfo> names = session.dciIdsToNames(dciList);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						dciNameCache = names;
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot resolve DCI names");
			}
		}.runInForeground();
	}

	/**
	 * Add single cache entry
	 * 
	 * @param nodeId
	 * @param dciId
	 * @param name
	 */
	public void addCacheEntry(long nodeId, long dciId, String name)
	{
      dciNameCache.put(dciId, new DciInfo("", name, ""));
	}
}
