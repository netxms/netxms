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
package org.netxms.ui.eclipse.dashboard.propertypages.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.charts.api.ChartDciConfig;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.propertypages.DataSources;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for DCI list
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	private NXCSession session;
	private Map<NodeItemPair, String> dciNameCache = new HashMap<NodeItemPair, String>();
	private List<ChartDciConfig> elementList;
	
	private class NodeItemPair
	{
		long nodeId;
		long dciId;

		public NodeItemPair(long nodeId, long dciId)
		{
			this.nodeId = nodeId;
			this.dciId = dciId;
		}

		@Override
		public int hashCode()
		{
			final int prime = 31;
			int result = 1;
			result = prime * result + (int)(dciId ^ (dciId >>> 32));
			result = prime * result + (int)(nodeId ^ (nodeId >>> 32));
			return result;
		}

		@Override
		public boolean equals(Object obj)
		{
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			NodeItemPair other = (NodeItemPair)obj;
			if (dciId != other.dciId)
				return false;
			if (nodeId != other.nodeId)
				return false;
			return true;
		}
	}
	
	/**
	 * The constructor
	 */
	public DciListLabelProvider(List<ChartDciConfig> elementList)
	{
		this.elementList = elementList;
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
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
				GenericObject object = session.findObjectById(dci.nodeId);
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.nodeId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case DataSources.COLUMN_METRIC:
				String name = dciNameCache.get(new NodeItemPair(dci.nodeId, dci.dciId));
				return (name != null) ? name : Messages.DciListLabelProvider_Unresolved;
			case DataSources.COLUMN_LABEL:
				return dci.name;
			case DataSources.COLUMN_COLOR:
				return dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? Messages.DciListLabelProvider_Auto : dci.color;
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
		new ConsoleJob(Messages.DciListLabelProvider_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final long[] nodeIds = new long[dciList.size()];
				final long[] dciIds = new long[dciList.size()];
				int i = 0;
				for(ChartDciConfig dci : dciList)
				{
					nodeIds[i] = dci.nodeId;
					dciIds[i] = dci.dciId;
					i++;
				}
				final String[] names = session.resolveDciNames(nodeIds, dciIds);
				
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						int i = 0;
						for(ChartDciConfig dci : dciList)
						{
							dciNameCache.put(new NodeItemPair(dci.nodeId, dci.dciId), names[i++]);
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.DciListLabelProvider_JobError;
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
		dciNameCache.put(new NodeItemPair(nodeId, dciId), name);
	}
}
