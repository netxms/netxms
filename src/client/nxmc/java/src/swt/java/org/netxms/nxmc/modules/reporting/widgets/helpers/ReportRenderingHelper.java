/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.reporting.widgets.helpers;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.client.NXCSession;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Helper for report rendering - handles web/desktop differnces during report rendering.
 */
public class ReportRenderingHelper
{
   private final I18n i18n = LocalizationHelper.getI18n(ReportRenderingHelper.class);

   private View view;
   private ReportDefinition report;
   private UUID jobId;
   private Date executionTime;
   private ReportRenderFormat format;

   /**
    * Create helper.
    *
    * @param view owning view
    * @param report report definition
    * @param jobId job ID
    * @param executionTime job execution time
    * @param format render format
    */
   public ReportRenderingHelper(View view, ReportDefinition report, UUID jobId, Date executionTime, ReportRenderFormat format)
   {
      this.view = view;
      this.report = report;
      this.jobId = jobId;
      this.executionTime = executionTime;
      this.format = format;
   }

   /**
    * Render report.
    */
   public void renderReport()
   {
      final StringBuilder nameTemplate = new StringBuilder();
      nameTemplate.append(report.getName());
      nameTemplate.append(" ");
      nameTemplate.append(new SimpleDateFormat("ddMMyyyy HHmm").format(executionTime)); //$NON-NLS-1$
      nameTemplate.append(".");
      nameTemplate.append(format.getExtension());

      FileDialog fileDialog = new FileDialog(view.getWindow().getShell(), SWT.SAVE);
      switch(format)
      {
         case PDF:
            fileDialog.setFilterNames(new String[] { "PDF Files", "All Files" });
            fileDialog.setFilterExtensions(new String[] { "*.pdf", "*.*" });
            break;
         case XLSX:
            fileDialog.setFilterNames(new String[] { "Excel Files", "All Files" });
            fileDialog.setFilterExtensions(new String[] { "*.xlsx", "*.*" });
            break;
         default:
            fileDialog.setFilterNames(new String[] { "All Files" });
            fileDialog.setFilterExtensions(new String[] { "*.*" });
            break;
      }
      fileDialog.setFileName(nameTemplate.toString());
      final String fileName = fileDialog.open();

      if (fileName == null)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Rendering report"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final File reportFile = session.renderReport(report.getId(), jobId, format);

            // save
            FileInputStream inputStream = null;
            FileOutputStream outputStream = null;
            try
            {
               inputStream = new FileInputStream(reportFile);
               outputStream = new FileOutputStream(fileName);

               byte[] buffer = new byte[1024];
               int size = 0;
               do
               {
                  size = inputStream.read(buffer);
                  if (size > 0)
                  {
                     outputStream.write(buffer, 0, size);
                  }
               } while(size == buffer.length);

               outputStream.close();
               outputStream = null;

               runInUIThread(() -> openReport(fileName, format));
            }
            finally
            {
               if (inputStream != null)
               {
                  inputStream.close();
               }
               if (outputStream != null)
               {
                  outputStream.close();
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot render report {0} (job ID {1})", report.getName(), jobId);
         }
      }.start();
   }

   /**
    * Open rendered report in appropriate external program (like PDF viewer)
    * 
    * @param fileName rendered report file
    */
   private void openReport(String fileName, ReportRenderFormat format)
   {
      final Program program = Program.findProgram(format.getExtension());
      if (program != null)
      {
         program.execute(fileName);
      }
      else
      {
         MessageDialogHelper.openError(view.getWindow().getShell(), i18n.tr("Error"), i18n.tr("Report was rendered successfully, but external viewer cannot be opened"));
      }
   }
}
