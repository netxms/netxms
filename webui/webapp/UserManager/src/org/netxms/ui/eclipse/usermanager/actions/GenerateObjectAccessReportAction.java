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
package org.netxms.ui.eclipse.usermanager.actions;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.ui.IViewPart;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.reports.acl.AbstractAclReport;
import org.netxms.ui.eclipse.usermanager.reports.acl.AclReport;
import org.netxms.ui.eclipse.usermanager.reports.acl.HbtfAclReport;

/**
 * Action to generate object access report
 */
public class GenerateObjectAccessReportAction extends Action
{
   private IViewPart viewPart;

   /**
    * @param text
    * @param image
    */
   public GenerateObjectAccessReportAction(String text, IViewPart viewPart)
   {
      super(text, Activator.getImageDescriptor("icons/acl-report.png"));
      this.viewPart = viewPart;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      try
      {
         Activator.logInfo("Report generation started");
         final File tmpFile = File.createTempFile("ACLReport_" + viewPart.hashCode(), "_" + System.currentTimeMillis());
         final String outputFileName = tmpFile.getAbsolutePath();
         final AbstractAclReport report = ConsoleSharedData.getSession().getClientConfigurationHint("AclReportVariant", "").equals("HBTF") ? new HbtfAclReport(outputFileName) :
            new AclReport(outputFileName);
         new ConsoleJob("Generating object access report", viewPart, Activator.PLUGIN_ID) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               report.execute();
               DownloadServiceHandler.addDownload(tmpFile.getName(), "netxms-acl-report-" + new SimpleDateFormat("YYYY-MMM-dd").format(new Date()) + ".xlsx", tmpFile,
                     "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
               runInUIThread(() -> DownloadServiceHandler.startDownload(tmpFile.getName()));
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot generate user access report";
            }
         }.start();
      }
      catch(IOException e)
      {
         Activator.logError("Unexpected I/O error", e);
         MessageDialogHelper.openError(viewPart.getSite().getShell(), "Error", "Cannot generate user access report: " + e.getMessage());
      }
   }
}
