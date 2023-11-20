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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.lang.reflect.Constructor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.modules.dashboards.api.CustomDashboardElement;
import org.netxms.nxmc.modules.dashboards.config.CustomWidgetConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Dashboard element containing custom widget
 */
public class CustomWidgetElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(CustomWidgetElement.class);

	/**
	 * @param parent
	 * @param element
	 */
   public CustomWidgetElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, SWT.NONE, element, view);

		CustomWidgetConfig config;
		try
		{
         config = XMLTools.createFromXml(CustomWidgetConfig.class, element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new CustomWidgetConfig();
		}

      processCommonSettings(config);

      try
      {
         Class<?> widgetClass = Class.forName(config.getClassName());
         if (CustomDashboardElement.class.isAssignableFrom(widgetClass))
         {
            Constructor<?> c = widgetClass.getConstructor(Composite.class, String.class);
            c.newInstance(getContentArea(), element.getData());
         }
         else
         {
            logger.error("Cannot instantiate widget for custom dashboard element - class " + config.getClassName() + " is not assignable to CustomDashboardElement");
         }
      }
      catch(Exception e)
      {
         logger.error("Cannot instantiate widget for custom dashboard element", e);
      }
	}
}
