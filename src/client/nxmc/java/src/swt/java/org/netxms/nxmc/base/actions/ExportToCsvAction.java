/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.actions;

import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Action for exporting selected table viewer rows to CSV file
 */
public class ExportToCsvAction extends TableRowAction
{
   private I18n i18n = LocalizationHelper.getI18n(ExportToCsvAction.class);

   private View view;

	/**
    * Create "Export to CSV" action.
    * 
    * @param view owning view
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
	 */
   private ExportToCsvAction(View view, ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly)
	{
      super(viewer, viewerProvider, selectionOnly, selectionOnly ? LocalizationHelper.getI18n(ExportToCsvAction.class).tr("E&xport selection to CSV...") : LocalizationHelper.getI18n(ExportToCsvAction.class).tr("Export all to CSV..."), SharedIcons.CSV);
      this.view = view;
	}

	/**
	 * Create "Export to CSV" action.
	 * 
    * @param view owning view
    * @param viewer viewer to copy rows from
    * @param selectionOnly true to copy only selected rows
    */
   public ExportToCsvAction(View view, ColumnViewer viewer, boolean selectionOnly)
	{
      this(view, viewer, null, selectionOnly);
	}

	/**
    * Create "Export to CSV" action.
    * 
    * @param view owning view
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    */
   public ExportToCsvAction(View view, ViewerProvider viewerProvider, boolean selectionOnly)
	{
      this(view, null, viewerProvider, selectionOnly);
	}

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
	@Override
	public void run()
	{
      FileDialog dlg = new FileDialog(view.getWindow().getShell(), SWT.SAVE);
      dlg.setOverwrite(true);
		dlg.setFilterExtensions(new String[] { "*.csv", "*.*" });
		dlg.setFilterNames(new String[] { "CSV files", "All files"});
		final String fileName = dlg.open();
		if (fileName == null)
			return;

      final List<String[]> data = getRowsFromViewer(true);
      new Job(String.format(i18n.tr("Save data to CSV file %s"), fileName), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            BufferedWriter out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(fileName), StandardCharsets.UTF_8));
            out.write('\ufeff'); // write BOM
				for(String[] row : data)
				{
					for(int i = 0; i < row.length; i++)
					{
						if (i > 0)
							out.write(',');
						out.write('"');
                  out.write(row[i].replace("\"", "\"\""));
						out.write('"');
					}
					out.newLine();
				}
				out.close();
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot save table data to file");
			}
		}.start();
	}
}
