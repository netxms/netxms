/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2025 Raden Soultions
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
package org.netxms.nxmc.modules.users.actions;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.reports.acl.AbstractAclReport;
import org.netxms.nxmc.modules.users.reports.acl.AclReport;
import org.netxms.nxmc.modules.users.reports.acl.HbtfAclReport;
import org.netxms.nxmc.resources.ResourceManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Action to generate object access report
 */
public class GenerateObjectAccessReportAction extends Action
{
   private static final Logger logger = LoggerFactory.getLogger(GenerateObjectAccessReportAction.class);

   private final I18n i18n = LocalizationHelper.getI18n(GenerateObjectAccessReportAction.class);

   private View view;

   /**
    * @param text
    * @param image
    */
   public GenerateObjectAccessReportAction(String text, View view)
   {
      super(text, ResourceManager.getImageDescriptor("icons/acl-report.png"));
      this.view = view;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      try
      {
         final File tmpFile = File.createTempFile("ACLReport_" + view.hashCode(), "_" + System.currentTimeMillis());
         NXCSession session = Registry.getSession();
         final AbstractAclReport report = session.getClientConfigurationHint("AclReportVariant", "").equals("HBTF") ? new HbtfAclReport(tmpFile.getAbsolutePath()) : new AclReport(tmpFile.getAbsolutePath());
         new Job(i18n.tr("Generating object access report"), view) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               report.execute();
               DownloadServiceHandler.addDownload(tmpFile.getName(), "netxms-acl-report-" + new SimpleDateFormat("YYYY-MMM-dd").format(new Date()) + ".xlsx", tmpFile,
                     "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
               runInUIThread(() -> DownloadServiceHandler.startDownload(tmpFile.getName()));
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot generate user access report");
            }
         }.start();
      }
      catch(IOException e)
      {
         logger.error("Unexpected I/O error", e);
         view.addMessage(MessageArea.ERROR, i18n.tr("Cannot generate user access report (internal server error)"));
      }
   }
}
