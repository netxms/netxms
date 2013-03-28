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
package org.netxms.ui.eclipse.objectmanager.propertypages.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.propertypages.ConditionData;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for DCI list
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] functions = { "last()", "average(", "deviation(", "diff()", "error(", "sum(" };

	private NXCSession session;
	private Map<NodeItemPair, String> dciNameCache = new HashMap<NodeItemPair, String>();
	private List<ConditionDciInfo> elementList;
	
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
	public DciListLabelProvider(List<ConditionDciInfo> elementList)
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
		ConditionDciInfo dci = (ConditionDciInfo)element;
		switch(columnIndex)
		{
			case ConditionData.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case ConditionData.COLUMN_NODE:
				AbstractObject object = session.findObjectById(dci.getNodeId());
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.getNodeId()) + "]");
			case ConditionData.COLUMN_METRIC:
				String name = dciNameCache.get(new NodeItemPair(dci.getNodeId(), dci.getDciId()));
				return (name != null) ? name : "<unresolved>";
			case ConditionData.COLUMN_FUNCTION:
				int f = dci.getFunction();
				StringBuilder text = new StringBuilder(functions[f]);
				if ((f != Threshold.F_DIFF) && (f != Threshold.F_LAST))
				{
					text.append(dci.getPolls());
					text.append(')');
				}
				return text.toString();
		}
		return null;
	}
	
	/**
	 * Resolve DCI names for given collection of condition DCIs and add to cache
	 * 
	 * @param dciList
	 */
	public void resolveDciNames(final Collection<ConditionDciInfo> dciList)
	{
		new ConsoleJob("Resolve DCI names", null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final String[] names = session.resolveDciNames(dciList);
				PlatformUI.getWorkbench().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						int i = 0;
						for(ConditionDciInfo dci : dciList)
						{
							dciNameCache.put(new NodeItemPair(dci.getNodeId(), dci.getDciId()), names[i++]);
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot resolve DCI names";
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
