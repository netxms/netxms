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
package org.netxms.ui.eclipse.dashboard.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.propertypages.DashboardElements;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;

/**
 * Label provider for list of dashboard elements
 */
public class DashboardElementsLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private final String[] ELEMENT_TYPES = { 
		Messages.get().DashboardElementsLabelProvider_TypeLabel, 
		Messages.get().DashboardElementsLabelProvider_TypeLineChart, 
		Messages.get().DashboardElementsLabelProvider_TypeBarChart, 
		Messages.get().DashboardElementsLabelProvider_TypePieChart, 
      Messages.get().DashboardElementsLabelProvider_TypeBarChart,
		Messages.get().DashboardElementsLabelProvider_TypeStatusChart, 
		Messages.get().DashboardElementsLabelProvider_TypeStatusIndicator, 
		Messages.get().DashboardElementsLabelProvider_TypeDashboard, 
		Messages.get().DashboardElementsLabelProvider_TypeNetworkMap, 
		Messages.get().DashboardElementsLabelProvider_TypeCustom, 
		Messages.get().DashboardElementsLabelProvider_TypeGeoMap, 
		Messages.get().DashboardElementsLabelProvider_TypeAlarmViewer, 
		Messages.get().DashboardElementsLabelProvider_TypeAvailChart, 
		Messages.get().DashboardElementsLabelProvider_TypeGaugeChart, 
		Messages.get().DashboardElementsLabelProvider_TypeWebPage, 
		Messages.get().DashboardElementsLabelProvider_TypeTableBarChart,
		Messages.get().DashboardElementsLabelProvider_TypeTablePieChart, 
		Messages.get().DashboardElementsLabelProvider_TypeTableTubeChart, 
		Messages.get().DashboardElementsLabelProvider_TypeSeparator, 
		Messages.get().DashboardElementsLabelProvider_TableValue, 
		Messages.get().DashboardElementsLabelProvider_StatusMap,
		Messages.get().DashboardElementsLabelProvider_DciSummaryTable,
		"Syslog monitor",
		"SNMP trap monitor",
		"Event monitor",
      "Service component map",
      "Rack diagram",
      "Object tools",
      "Object query",
      "Port view",
      "Scripted bar chart",
      "Scripted pie chart",
      "File monitor"
	};

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
		DashboardElement de = (DashboardElement)element;
		switch(columnIndex)
		{
			case DashboardElements.COLUMN_TYPE:
				try
				{
					return ELEMENT_TYPES[de.getType()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return Messages.get().DashboardElementsLabelProvider_Unknown;
				}
			case DashboardElements.COLUMN_SPAN:
				try
				{
					DashboardElementLayout layout = DashboardElementLayout.createFromXml(de.getLayout());
					return Integer.toString(layout.horizontalSpan) + " / " + Integer.toString(layout.verticalSpan); //$NON-NLS-1$
				}
				catch(Exception e)
				{
					return "1 / 1"; //$NON-NLS-1$
				}
			case DashboardElements.COLUMN_HEIGHT:
				try
				{
	            DashboardElementLayout layout = DashboardElementLayout.createFromXml(de.getLayout());
	            if (layout.grabVerticalSpace)
	               return "Fill";

	            if (layout.heightHint > 0)
	               return Integer.toString(layout.heightHint);

	            return "Default";
				}
				catch(Exception e)
				{
					return "Fill";
				}
         case DashboardElements.COLUMN_TITLE:
            try
            {
               DashboardElementConfig config = DashboardElementConfig.createFromXml(de.getData());
               return config.getTitle();
            }
            catch(Exception e)
            {
               return "";
            }
		}
		return null;
	}
}
