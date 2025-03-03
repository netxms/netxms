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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.dashboards.config.EmbeddedDashboardConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Embedded dashboard element
 */
public class EmbeddedDashboardElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(EmbeddedDashboardElement.class);
   
	private Dashboard[] objects;
	private EmbeddedDashboardConfig config;
	private DashboardControl control = null;
	private int current = -1;
   private NXCSession session = Registry.getSession();

	/**
    * @param parent
    * @param element
    * @param view
    */
   public EmbeddedDashboardElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, SWT.NONE, element, view);

      try
      {
         config = EmbeddedDashboardConfig.createFromXmlOrJson(element.getData());
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new EmbeddedDashboardConfig();
      }

      processCommonSettings(config);

		objects = new Dashboard[config.getDashboardObjects().length];
		for(int i = 0; i < objects.length; i++)
			objects[i] = (Dashboard)session.findObjectById(config.getDashboardObjects()[i], Dashboard.class);

		if (objects.length > 1)
		{
			nextDashboard();

         final ViewRefreshController refreshControler = new ViewRefreshController(view, config.getDisplayInterval(), new Runnable() {
            @Override
            public void run()
            {
               if (!isDisposed())
                  nextDashboard();
            }
         });
			
			addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               refreshControler.dispose();
            }
         });
		}
		else
		{
			if ((objects != null) && (objects.length > 0) && (objects[0] != null))
            new DashboardControl(getContentArea(), SWT.NONE, objects[0], getEmbeddedDashboardContext(), view, true, isNarrowScreenMode());
		}
	}

	/**
	 * Show next dashboard in chain
	 */
	private void nextDashboard()
	{
		if (control != null)
			control.dispose();
		current++;
		if (current >= objects.length)
			current = 0;
		if (objects[current] != null)
         control = new DashboardControl(getContentArea(), SWT.NONE, objects[current], getEmbeddedDashboardContext(), view, true, isNarrowScreenMode()); /* TODO: set embedded=false if border=true */
		else
			control = null;
		getParent().layout(true, true);
	}

   /**
    * Get context for embedded dashboard.
    *
    * @return context for embedded dashboard
    */
   private AbstractObject getEmbeddedDashboardContext()
   {
      long contextObjectId = config.getContextObjectId();
      if ((contextObjectId == 0) || (contextObjectId == AbstractObject.CONTEXT))
         return getContext();
      AbstractObject object = session.findObjectById(contextObjectId);
      return (object != null) ? object : getContext();
   }
}
