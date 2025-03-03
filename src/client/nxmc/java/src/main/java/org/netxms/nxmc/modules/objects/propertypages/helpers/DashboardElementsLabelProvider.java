/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementLayout;
import org.netxms.nxmc.modules.objects.propertypages.DashboardElements;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Label provider for list of dashboard elements
 */
public class DashboardElementsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(DashboardElementsLabelProvider.class);
   private final String[] ELEMENT_TYPES = {
      i18n.tr("Label"), 
      i18n.tr("Line chart"), 
      i18n.tr("Bar chart"), 
      i18n.tr("Pie chart"), 
      i18n.tr("Bar chart"),
      i18n.tr("Status chart"), 
      i18n.tr("Status indicator"), 
      i18n.tr("Dashboard"), 
      i18n.tr("Network map"), 
      i18n.tr("Custom"), 
      i18n.tr("Geo map"), 
      i18n.tr("Alarm viewer"), 
      i18n.tr("Availability chart"), 
      i18n.tr("Gauge"), 
      i18n.tr("Web page"), 
      i18n.tr("Bar chart for table DCI"),
      i18n.tr("Pie chart for table DCI"),
      i18n.tr("Bar chart for table DCI"),
      i18n.tr("Separator"), 
      i18n.tr("Table value"), 
      i18n.tr("Status map"),
      i18n.tr("DCI summary table"),
      i18n.tr("Syslog monitor"),
      i18n.tr("SNMP trap monitor"),
      i18n.tr("Event monitor"),
      i18n.tr("Service component map"),
      i18n.tr("Rack diagram"),
      i18n.tr("Object tools"),
      i18n.tr("Object query"),
      i18n.tr("Port view"),
      i18n.tr("Scripted bar chart"),
      i18n.tr("Scripted pie chart"),
      i18n.tr("File monitor")
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
               return i18n.tr("<unknown>");
				}
			case DashboardElements.COLUMN_SPAN:
				try
				{
               DashboardElementLayout layout = new Gson().fromJson(de.getLayout(), DashboardElementLayout.class);
               return Integer.toString(layout.horizontalSpan) + " / " + Integer.toString(layout.verticalSpan);
				}
				catch(Exception e)
				{
               return "1 / 1";
				}
			case DashboardElements.COLUMN_HEIGHT:
				try
				{
               DashboardElementLayout layout = new Gson().fromJson(de.getLayout(), DashboardElementLayout.class);
	            if (layout.grabVerticalSpace)
                  return i18n.tr("Fill");

	            if (layout.heightHint > 0)
	               return Integer.toString(layout.heightHint);

               return i18n.tr("Default");
				}
				catch(Exception e)
				{
               return i18n.tr("Fill");
				}
         case DashboardElements.COLUMN_NARROW_SCREEN:
            try
            {
               DashboardElementLayout layout = new Gson().fromJson(de.getLayout(), DashboardElementLayout.class);
               if (!layout.showInNarrowScreenMode)
                  return i18n.tr("Hide");
               return Integer.toString(layout.narrowScreenOrder);
            }
            catch(Exception e)
            {
               return "1 / 1";
            }
         case DashboardElements.COLUMN_TITLE:
            try
            {
               DashboardElementConfig config = new Gson().fromJson(de.getData(), DashboardElementConfig.class);
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
