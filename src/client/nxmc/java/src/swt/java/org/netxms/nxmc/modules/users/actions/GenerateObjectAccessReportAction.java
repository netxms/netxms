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

import java.text.SimpleDateFormat;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.reports.acl.AbstractAclReport;
import org.netxms.nxmc.modules.users.reports.acl.AclReport;
import org.netxms.nxmc.modules.users.reports.acl.HbtfAclReport;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action to generate object access report
 */
public class GenerateObjectAccessReportAction extends Action
{
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
      FileDialog fd = new FileDialog(view.getWindow().getShell(), SWT.SAVE);
      fd.setText(i18n.tr("Select Output File"));
      fd.setFileName("netxms-acl-report-" + new SimpleDateFormat("YYYY-MMM-dd").format(new Date()) + ".xlsx");
      WidgetHelper.setFileDialogFilterExtensions(fd, new String[] { "*.xlsx", "*.*" });
      WidgetHelper.setFileDialogFilterNames(fd, new String[] { i18n.tr("Excel Spreadsheet"), i18n.tr("All Files") });
      final String outputFileName = fd.open();
      if (outputFileName == null)
         return;

      NXCSession session = Registry.getSession();
      final AbstractAclReport report = session.getClientConfigurationHint("AclReportVariant", "").equals("HBTF") ? new HbtfAclReport(outputFileName) : new AclReport(outputFileName);
      new Job(i18n.tr("Generating object access report"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            report.execute();
            runInUIThread(() -> view.addMessage(MessageArea.SUCCESS, i18n.tr("Object access report successfully generated and saved as {0}", outputFileName)));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot generate user access report");
         }
      }.start();
   }
}
