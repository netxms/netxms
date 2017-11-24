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
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.dashboard.widgets.internal.RackDiagramConfig;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 *Rack diagram element for dashboard
 */
public class RackDiagramElement extends ElementWidget
{
   private RackWidget rackFrontWidget;
   private RackWidget rackRearWidget;
   private NXCSession session;
   private RackDiagramConfig config;
   private Composite rackArea;

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
      {
         rackArea = new Composite(this, SWT.NONE) {
            @Override
            public Point computeSize(int wHint, int hHint, boolean changed)
            {
               if ((rackFrontWidget == null) || (rackRearWidget == null) || (hHint == SWT.DEFAULT))
                  return super.computeSize(wHint, hHint, changed);
               
               Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
               return new Point(s.x * 2, s.y);
            }
         };
         rackArea.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
         rackArea.addControlListener(new ControlAdapter() {
            @Override
            public void controlResized(ControlEvent e)
            {
               if ((rackFrontWidget == null) || (rackRearWidget == null))
                  return;
               
               int height = rackArea.getSize().y;
               Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
               rackFrontWidget.setSize(size);
               rackRearWidget.setSize(size);
               rackRearWidget.setLocation(size.x, 0);
            }
         });
         
         setRackFrontWidget(new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.FRONT));
         setRackRearWidget(new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.REAR));
      }
      setLayout(new FillLayout());
   }

   /**
    * Get rear rack widget
    * 
    * @return Rear rack widget
    */
   public RackWidget getRackRearWidget()
   {
      return rackRearWidget;
   }

   /**
    * Set rear rack widget
    * 
    * @param rackRearWidget to set
    */
   public void setRackRearWidget(RackWidget rackRearWidget)
   {
      this.rackRearWidget = rackRearWidget;
   }

   /**
    * Get front rack widget
    * 
    * @return Front rack widget
    */
   public RackWidget getRackFrontWidget()
   {
      return rackFrontWidget;
   }

   /**
    * Set front rack widget
    * 
    * @param rackFrontWidget to set
    */
   public void setRackFrontWidget(RackWidget rackFrontWidget)
   {
      this.rackFrontWidget = rackFrontWidget;
   }
}
