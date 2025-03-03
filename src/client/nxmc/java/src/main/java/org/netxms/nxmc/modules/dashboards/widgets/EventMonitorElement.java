/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2023 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.EventMonitorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.events.widgets.EventTraceWidget;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.google.gson.Gson;

/**
 * Event monitor element widget
 */
public class EventMonitorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(EventMonitorElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(EventMonitorElement.class);

   private EventTraceWidget viewer;
   private EventMonitorConfig config;
   private final NXCSession session;

   /**
    * Create event monitor dashboard element.
    *
    * @param parent parent widget
    * @param element dashboard element definition
    * @param view owning view part
    */
   protected EventMonitorElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);
      
      try
      {
         config = new Gson().fromJson(element.getData(), EventMonitorConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new EventMonitorConfig();
      }

      processCommonSettings(config);

      session = Registry.getSession();

      new Job(String.format(i18n.tr("Subscribing to channel %s"), NXCSession.CHANNEL_EVENTS), view, this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.subscribe(NXCSession.CHANNEL_EVENTS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot subscribe to channel %s"), NXCSession.CHANNEL_EVENTS);
         }
      }.start();

      viewer = new EventTraceWidget(getContentArea(), SWT.NONE, view);
      viewer.setRootObject(getEffectiveObjectId(config.getObjectId()));

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
      Job job = new Job(String.format(i18n.tr("Unsuscribing from channel %s"), NXCSession.CHANNEL_EVENTS), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.unsubscribe(NXCSession.CHANNEL_EVENTS);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot unsubscribe from channel %s"), NXCSession.CHANNEL_EVENTS);
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
      super.dispose();
   }
}
