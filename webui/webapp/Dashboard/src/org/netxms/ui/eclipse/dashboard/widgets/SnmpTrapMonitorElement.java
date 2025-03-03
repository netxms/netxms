/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.SnmpTrapMonitorConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.widgets.SnmpTrapTraceWidget;
import com.google.gson.Gson;

/**
 * SNMP trap monitor dashboard element
 */
public class SnmpTrapMonitorElement extends ElementWidget
{
   private SnmpTrapTraceWidget viewer;
   private SnmpTrapMonitorConfig config;
   private NXCSession session;

   /**
    * Create SNMP trap monitor element.
    *
    * @param parent parent widget
    * @param element dashboard element definition
    * @param viewPart owning view part
    */
   protected SnmpTrapMonitorElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);

      try
      {
         config = new Gson().fromJson(element.getData(), SnmpTrapMonitorConfig.class);
      }
      catch(Exception e)
      {
         Activator.logError("Cannot parse dashboard element configuration", e);
         config = new SnmpTrapMonitorConfig();
      }

      processCommonSettings(config);

      session = ConsoleSharedData.getSession();

      new ConsoleJob(String.format("Subscribing to channel %s", NXCSession.CHANNEL_SNMP_TRAPS), viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_SNMP_TRAPS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot subscribe to channel %s", NXCSession.CHANNEL_SNMP_TRAPS);
         }
      }.start();

      viewer = new SnmpTrapTraceWidget(getContentArea(), SWT.NONE, viewPart);
      viewer.setRootObject(getEffectiveObjectId(config.getObjectId()));
      viewer.getViewer().getControl().addFocusListener(new FocusAdapter() {
         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(viewer.getSelectionProvider());
         }
      });

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            unsubscribe();
         }
      });
   }

   /**
    * Unsubscribe from notifications
    */
   private void unsubscribe()
   {
      ConsoleJob job = new ConsoleJob(String.format("Unsuscribing from channel %s", NXCSession.CHANNEL_SNMP_TRAPS), null,
            Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(NXCSession.CHANNEL_SNMP_TRAPS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot unsubscribe from channel %s", NXCSession.CHANNEL_SNMP_TRAPS);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
      super.dispose();
   }
}
