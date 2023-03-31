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
package org.netxms.nxmc.modules.dashboards.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardControl;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Base class for different dashboard views
 */
public abstract class AbstractDashboardView extends ObjectView
{
   private Composite viewArea;
   private ScrolledComposite scroller;
   private DashboardControl dbc;

   /**
    * Create view.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public AbstractDashboardView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id, false); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewArea = parent;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      super.onObjectUpdate(object);
      onObjectChange(object);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      setContext(Registry.getSession().findObjectById(getObjectId()));
      super.refresh();
   }

   /**
    * Request complete dashboard layout
    */
   public void requestDashboardLayout()
   {
      if (scroller != null)
         updateScroller();
      else
         viewArea.layout(true, true);
   }

   /**
    * Rebuild dashboard
    */
   protected void rebuildDashboard(Dashboard dashboard, AbstractObject dashboardContext)
   {
      if (dbc != null)
         dbc.dispose();

      if ((scroller != null) && !dashboard.isScrollable())
      {
         scroller.dispose();
         scroller = null;
      }
      else if ((scroller == null) && dashboard.isScrollable())
      {
         scroller = new ScrolledComposite(viewArea, SWT.V_SCROLL);
         scroller.setExpandHorizontal(true);
         scroller.setExpandVertical(true);
         WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
         viewArea.layout(true, true);
      }

      dbc = new DashboardControl(dashboard.isScrollable() ? scroller : viewArea, SWT.NONE, dashboard, dashboardContext, this, false);
      if (dashboard.isScrollable())
      {
         scroller.setContent(dbc);
         scroller.addControlListener(new ControlAdapter() {
            public void controlResized(ControlEvent e)
            {
               updateScroller();
            }
         });
         updateScroller();
      }
      else
      {
         viewArea.layout(true, true);
      }
   }

   /**
    * Update scroller
    */
   private void updateScroller()
   {
      dbc.layout(true, true);
      Rectangle r = scroller.getClientArea();
      Point s = dbc.computeSize(r.width, SWT.DEFAULT);
      scroller.setMinSize(s);
   }
}
