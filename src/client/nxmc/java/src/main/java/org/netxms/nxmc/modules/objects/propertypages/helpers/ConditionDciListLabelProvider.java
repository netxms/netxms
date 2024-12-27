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
package org.netxms.nxmc.modules.objects.propertypages.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciInfo;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.propertypages.ConditionData;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for DCI list
 */
public class ConditionDciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ConditionDciListLabelProvider.class);
   
	private static final String[] functions = { "last()", "average(", "deviation(", "diff()", "error(", "sum(" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$

	private NXCSession session;
	private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();
	private List<ConditionDciInfo> elementList;
	
	/**
	 * The constructor
	 */
	public ConditionDciListLabelProvider(List<ConditionDciInfo> elementList)
	{
		this.elementList = elementList;
		session = Registry.getSession();
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
		ConditionDciInfo dci = (ConditionDciInfo)element;
		switch(columnIndex)
		{
			case ConditionData.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case ConditionData.COLUMN_NODE:
				AbstractObject object = session.findObjectById(dci.getNodeId());
				return (object != null) ? object.getObjectName() : ("[" + Long.toString(dci.getNodeId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case ConditionData.COLUMN_METRIC:
				String name = dciNameCache.get(dci.getDciId()).displayName;
				return (name != null) ? name : i18n.tr("<unresolved>");
			case ConditionData.COLUMN_FUNCTION:
			   if (dci.getType() == DataCollectionObject.DCO_TYPE_TABLE)
			      return ""; //$NON-NLS-1$
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
		new Job(i18n.tr("Resolve DCI names"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final Map<Long, DciInfo> names = session.dciIdsToNames(dciList);
            getDisplay().asyncExec(() -> {
               dciNameCache = names;
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
