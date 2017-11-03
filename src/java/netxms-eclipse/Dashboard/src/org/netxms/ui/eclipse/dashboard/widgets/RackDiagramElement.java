/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.dashboard.widgets.internal.RackDiagramConfig;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 *Rack diagram element for dashboard
 */
public class RackDiagramElement extends ElementWidget
{
   private RackWidget rackWidget;
   private NXCSession session;
   private RackDiagramConfig config;

   /**
    * Create new rack diagram element
    * 
    * @param parent Dashboard control
    * @param element Dashboard element
    * @param viewPart viewPart
    */
   protected RackDiagramElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
      
      try
      {
         config = RackDiagramConfig.createFromXml(element.getData());
      }
      catch (Exception e)
      {
         e.printStackTrace();
         config = new RackDiagramConfig();
      }
      
      session = (NXCSession)ConsoleSharedData.getSession();
      Rack rack = session.findObjectById(config.getObjectId(), Rack.class);
      
      if (rack != null)
         setRackWidget(new RackWidget(this, SWT.NONE, rack));
      
      FillLayout layout = new FillLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);
   }

   /**
    * Get rack widget
    * @return rack widget
    */
   public RackWidget getRackWidget()
   {
      return rackWidget;
   }

   /**
    * Set rack widget
    * @param rackWidget to set
    */
   public void setRackWidget(RackWidget rackWidget)
   {
      this.rackWidget = rackWidget;
   }

}
