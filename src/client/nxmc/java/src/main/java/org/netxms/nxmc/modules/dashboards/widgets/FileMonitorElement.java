/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 Raden Solutions
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
import org.netxms.client.AgentFileData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.jobs.JobCallingServerJob;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.FileMonitorConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.filemanager.widgets.DynamicFileViewer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * File monitor element widget
 */
public class FileMonitorElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(FileMonitorElement.class);

   private final I18n i18n = LocalizationHelper.getI18n(FileMonitorElement.class);

   private FileMonitorConfig config;
   private DynamicFileViewer viewer;
   private NXCSession session;
   private AgentFileData file;

   /**
    * Create event monitor dashboard element.
    *
    * @param parent parent widget
    * @param element dashboard element definition
    * @param view owning view part
    */
   protected FileMonitorElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);

      try
      {
         config = FileMonitorConfig.createFromXml(element.getData());
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new FileMonitorConfig();
      }

      processCommonSettings(config);

      session = Registry.getSession();
      viewer = new DynamicFileViewer(getContentArea(), SWT.NONE, view);
      viewer.setLineCountLimit(config.getHistoryLimit());

      new JobCallingServerJob(String.format(i18n.tr("Starting monitor for file \"%s\""), config.getFileName()), view, this) {
         @Override
         protected void run(final IProgressMonitor monitor) throws Exception
         {
            file = session.downloadFileFromAgent(config.getObjectId(), config.getFileName(), 1024, true, null, this);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.showFile(file.getFile(), true);
                  viewer.startTracking(config.getObjectId(), file.getId(), file.getRemoteName());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Error downloading file %s from node %s"), config.getFileName(), session.getObjectName(config.getObjectId()));
         }
      }.start();

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            stop();
         }
      });
   }

   /**
    * Stop file monitoring
    */
   private void stop()
   {
      final Job job = new Job(i18n.tr("Stopping file monitor"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.cancelFileMonitoring(config.getObjectId(), file.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot stop file monitor");
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }
}
