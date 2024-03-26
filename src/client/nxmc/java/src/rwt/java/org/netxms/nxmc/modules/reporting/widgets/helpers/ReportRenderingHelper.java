/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCSession;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.nxmc.DownloadServiceHandler;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.reporting.views.ReportResultView;
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
   private boolean preview;

   /**
    * Create helper.
    *
    * @param view owning view
    * @param report report definition
    * @param jobId job ID
    * @param executionTime job execution time
    * @param format render format (ignored for preview)
    * @param preview true if rendered report should be open inside application view
    */
   public ReportRenderingHelper(View view, ReportDefinition report, UUID jobId, Date executionTime, ReportRenderFormat format, boolean preview)
   {
      this.view = view;
      this.report = report;
      this.jobId = jobId;
      this.executionTime = executionTime;
      this.format = preview ? ReportRenderFormat.PDF : format; // Ignore provided format for preview
      this.preview = preview;
   }

   /**
    * Render report.
    */
   public void renderReport()
   {
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Rendering report"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final File reportFile = session.renderReport(report.getId(), jobId, format);
            if (preview)
            {
               DownloadServiceHandler.addDownload(reportFile.getName(), "report.pdf", reportFile,
                     (format == ReportRenderFormat.PDF) ? "application/pdf" : "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", true, false);
               runInUIThread(() -> {
                  view.openView(new ReportResultView(i18n.tr("Result from {0}", new SimpleDateFormat("yyyy-MM-dd HH:mm").format(executionTime)), UUID.randomUUID(),
                        DownloadServiceHandler.createDownloadUrl(reportFile.getName()), () -> DownloadServiceHandler.removeDownload(reportFile.getName())));
               });
            }
            else
            {
               StringBuilder nameTemplate = new StringBuilder();
               nameTemplate.append(report.getName());
               nameTemplate.append(" ");
               nameTemplate.append(new SimpleDateFormat("ddMMyyyy HHmm").format(executionTime));
               nameTemplate.append(".");
               nameTemplate.append(format.getExtension());

               DownloadServiceHandler.addDownload(reportFile.getName(), nameTemplate.toString(), reportFile,
                     (format == ReportRenderFormat.PDF) ? "application/pdf" : "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
               runInUIThread(() -> DownloadServiceHandler.startDownload(reportFile.getName()));
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot render report {0} (job ID {1})", report.getName(), jobId);
         }
      }.start();
   }
}
