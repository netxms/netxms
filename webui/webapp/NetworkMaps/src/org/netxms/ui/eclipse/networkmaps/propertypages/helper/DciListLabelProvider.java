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
package org.netxms.ui.eclipse.networkmaps.propertypages.helper;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.networkmaps.propertypages.LinkDataSources;
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for DCI list on property page for map link
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session;
	private Map<NodeItemPair, String> dciNameCache = new HashMap<NodeItemPair, String>();
	private List<SingleDciConfig> elementList;
	
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
	 * Constructor for DciListLabelProvider class
	 */
	public DciListLabelProvider(List<SingleDciConfig> elementList)
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
	   SingleDciConfig dci = (SingleDciConfig)element;
		switch(columnIndex)
		{
			case LinkDataSources.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case LinkDataSources.COLUMN_NODE:
				AbstractObject object = session.findObjectById(dci.getNodeId());
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.getNodeId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case LinkDataSources.COLUMN_METRIC:
				String name = dciNameCache.get(new NodeItemPair(dci.getNodeId(), dci.dciId));
				return (name != null) ? name : Messages.get().DciListLabelProvider_Unresolved;
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
		new ConsoleJob(Messages.get().DciListLabelProvider_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final long[] nodeIds = new long[dciList.size()];
				final long[] dciIds = new long[dciList.size()];
				int i = 0;
				for(SingleDciConfig dci : dciList)
				{
					nodeIds[i] = dci.getNodeId();
					dciIds[i] = dci.dciId;
					i++;
				}
				final String[] names = session.resolveDciNames(nodeIds, dciIds);
				
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						int i = 0;
						for(SingleDciConfig dci : dciList)
						{
							dciNameCache.put(new NodeItemPair(dci.getNodeId(), dci.dciId), names[i++]);
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DciListLabelProvider_JobError;
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
