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
import org.netxms.ui.eclipse.dashboard.widgets.internal.SyslogMonitorConfig;
import org.netxms.ui.eclipse.eventmanager.widgets.SyslogTraceWidget;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import com.google.gson.Gson;

/**
 * "Syslog monitor" dashboard element
 */
public class SyslogMonitorElement extends ElementWidget
{
   private SyslogTraceWidget viewer;
   private SyslogMonitorConfig config;
   private final NXCSession session;

   /**
    * Create syslog monitor element.
    *
    * @param parent parent composite
    * @param element dashboard element
    * @param viewPart owning view part
    */
   protected SyslogMonitorElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);

      try
      {
         config = new Gson().fromJson(element.getData(), SyslogMonitorConfig.class);
      }
      catch(Exception e)
      {
         Activator.logError("Cannot parse dashboard element configuration", e);
         config = new SyslogMonitorConfig();
      }

      processCommonSettings(config);

      session = ConsoleSharedData.getSession();

      new ConsoleJob(String.format("Subscribing to channel %s", NXCSession.CHANNEL_SYSLOG), viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_SYSLOG);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot subscribe to channel %s", NXCSession.CHANNEL_SYSLOG);
         }
      }.start();

      viewer = new SyslogTraceWidget(getContentArea(), SWT.NONE, viewPart);
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
      ConsoleJob job = new ConsoleJob(String.format("Unsuscribing from channel %s", NXCSession.CHANNEL_SYSLOG), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(NXCSession.CHANNEL_SYSLOG);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot unsubscribe from channel %s", NXCSession.CHANNEL_SYSLOG);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
      super.dispose();
   }
}
