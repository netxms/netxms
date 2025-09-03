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

import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IViewPart;
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
      FileDialog fd = new FileDialog(viewPart.getSite().getShell(), SWT.SAVE);
      fd.setText("Select Output File");
      fd.setFileName("netxms-acl-report-" + new SimpleDateFormat("YYYY-MMM-dd").format(new Date()) + ".xlsx");
      fd.setFilterExtensions(new String[] { "*.xlsx", "*.*" });
      fd.setFilterNames(new String[] { "Excel Spreadsheet", "All Files" });
      final String outputFileName = fd.open();
      if (outputFileName == null)
         return;

      final AbstractAclReport report = ConsoleSharedData.getSession().getClientConfigurationHint("AclReportVariant", "").equals("HBTF") ? new HbtfAclReport(outputFileName) :
            new AclReport(outputFileName);
      new ConsoleJob("Generating object access report", viewPart, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            report.execute();
            runInUIThread(
                  () -> MessageDialogHelper.openInformation(viewPart.getSite().getShell(), "Success", String.format("Object access report successfully generated and saved as %s", outputFileName)));
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot generate user access report";
         }
      }.start();
   }
}
