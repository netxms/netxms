/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SnmpTrapMonitorConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.widgets.SnmpTrapTraceWidget;

public class SnmpTrapMonitorElement extends ElementWidget
{
   private SnmpTrapTraceWidget viewer;
   private SnmpTrapMonitorConfig config;
   private final NXCSession session;

   protected SnmpTrapMonitorElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);

      try
      {
         config = SnmpTrapMonitorConfig.createFromXml(element.getData());
      }
      catch(Exception e)
      {
         e.printStackTrace();
         config = new SnmpTrapMonitorConfig();
      }

      session = ConsoleSharedData.getSession();

      new ConsoleJob(String.format("Subscribing to channel ", NXCSession.CHANNEL_SNMP_TRAPS), viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_SNMP_TRAPS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot subscribe to channel ", NXCSession.CHANNEL_SNMP_TRAPS);
         }
      }.start();

      FillLayout layout = new FillLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      setLayout(layout);

      viewer = new SnmpTrapTraceWidget(this, SWT.NONE, viewPart);
      viewer.setRootObject(config.getObjectId());
      viewer.getViewer().getControl().addFocusListener(new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
         }

         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(viewer.getSelectionProvider());
         }
      });

   }

   @Override
   public void dispose()
   {
      ConsoleJob job = new ConsoleJob(String.format("Unsuscribing from channel ", NXCSession.CHANNEL_SNMP_TRAPS), null,
            Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(NXCSession.CHANNEL_SNMP_TRAPS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot unsubscribe from channel ", NXCSession.CHANNEL_SNMP_TRAPS);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
      super.dispose();
   }
}
