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
package org.netxms.nxmc.modules.dashboards.propertypages.helpers;

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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.propertypages.DataSources;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for DCI list
 */
public class DciListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(DciListLabelProvider.class);
   private final DciInfo unresolvedDci = new DciInfo(i18n.tr("<unresolved>"), i18n.tr("<unresolved>"), "");

   private NXCSession session;
	private Map<Long, DciInfo> dciNameCache = new HashMap<Long, DciInfo>();
	private List<ChartDciConfig> elementList;

	/**
	 * The constructor
	 */
   public DciListLabelProvider(List<ChartDciConfig> elementList)
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
		ChartDciConfig dci = (ChartDciConfig)element;
		switch(columnIndex)
		{
			case DataSources.COLUMN_POSITION:
				return Integer.toString(elementList.indexOf(dci) + 1);
			case DataSources.COLUMN_NODE:
            return ((dci.nodeId == 0) || (dci.nodeId == AbstractObject.CONTEXT)) ? i18n.tr("<context>") :
                  ((dci.nodeId == AbstractObject.UNKNOWN) ? i18n.tr("<unknown>") : session.getObjectName(dci.nodeId));
         case DataSources.COLUMN_DCI_DISPLAY_NAME:
            if (dci.dciId == 0)
               return dci.dciDescription;
            String displayName = safeDciLookup(dci.dciId).displayName;
            return (displayName != null) ? displayName : i18n.tr("<unresolved>");
         case DataSources.COLUMN_DCI_TAG:
            if (dci.dciId == 0)
               return dci.dciTag;
            String tag = safeDciLookup(dci.dciId).userTag;
            return (tag != null) ? tag : "";
			case DataSources.COLUMN_METRIC:
            if (dci.dciId == 0)
               return dci.dciName;
            String name = safeDciLookup(dci.dciId).metric;
            return (name != null) ? name : i18n.tr("<unresolved>");
			case DataSources.COLUMN_LABEL:
				return dci.name;
			case DataSources.COLUMN_COLOR:
            return dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR) ? i18n.tr("auto") : dci.color;
		}
		return null;
	}

   /**
    * Safe DCI lookup (never return null).
    *
    * @param dciId DCI ID
    * @return DCI name info
    */
   private DciInfo safeDciLookup(long dciId)
   {
      DciInfo i = dciNameCache.get(dciId);
      return (i != null) ? i : unresolvedDci;
   }

	/**
	 * Resolve DCI names for given collection of condition DCIs and add to cache
	 * 
	 * @param dciList
	 */
	public void resolveDciNames(final Collection<ChartDciConfig> dciList)
	{
      new Job(i18n.tr("Resolving DCI names"), null) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Map<Long, DciInfo> names = session.dciIdsToNames(dciList);
            runInUIThread(() -> {
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
    * @param metric
    * @param displayName
    * @param userTag
    */
   public void addCacheEntry(long nodeId, long dciId, String metric, String displayName, String userTag)
	{
      dciNameCache.put(dciId, new DciInfo(metric, displayName, userTag));
	}
}
