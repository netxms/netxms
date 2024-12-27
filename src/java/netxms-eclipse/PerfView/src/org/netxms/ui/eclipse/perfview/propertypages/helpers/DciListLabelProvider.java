/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.propertypages.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.propertypages.DataSources;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for DCI list
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session;
	private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();
	private List<ChartDciConfig> elementList;

	/**
	 * The constructor
	 */
	public DciListLabelProvider(List<ChartDciConfig> elementList)
	{
		this.elementList = elementList;
      session = ConsoleSharedData.getSession();
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
		ChartDciConfig dci = (ChartDciConfig)element;
		switch(columnIndex)
		{
			case DataSources.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case DataSources.COLUMN_NODE:
				AbstractObject object = session.findObjectById(dci.nodeId);
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.nodeId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case DataSources.COLUMN_METRIC:
				String name = dciNameCache.get(dci.dciId).displayName;
				return (name != null) ? name : Messages.get().DciListLabelProvider_Unresolved;
			case DataSources.COLUMN_LABEL:
				return dci.name;
			case DataSources.COLUMN_COLOR:
				return dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? Messages.get().DciListLabelProvider_Auto : dci.color;
		}
		return null;
	}

	/**
	 * Resolve DCI names for given collection of condition DCIs and add to cache
	 * 
	 * @param dciList
	 */
	public void resolveDciNames(final Collection<ChartDciConfig> dciList)
	{
		new ConsoleJob(Messages.get().DciListLabelProvider_ResolveJobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
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
				return Messages.get().DciListLabelProvider_ResolveJobError;
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
