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
package org.netxms.ui.eclipse.actions;

import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.handlers.IHandlerService;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

/**
 * Action for exporting selected table viewer rows to CSV file
 */
public class ExportToCsvAction extends TableRowAction
{
	private IViewPart viewPart;

	/**
    * Create "Export to CSV" action attached to handler service
    * 
    * @param viewPart owning view part
    * @param viewer viewer to copy rows from
    * @param viewerProvider viewer provider
    * @param selectionOnly true to copy only selected rows
    */
	private ExportToCsvAction(IViewPart viewPart, ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly)
	{
      super(viewer, viewerProvider, selectionOnly, selectionOnly ? Messages.get().ExportToCsvAction_ExportToCsv : Messages.get().ExportToCsvAction_ExportAllToCsv, SharedIcons.CSV);
      this.viewPart = viewPart;

		setId(selectionOnly ? "org.netxms.ui.eclipse.popupActions.ExportToCSV" : "org.netxms.ui.eclipse.actions.ExportToCSV"); //$NON-NLS-1$ //$NON-NLS-2$

		// "Object Details" view can contain multiple widgets
		// with "Export to CSV" action defined, so binding it to handler service
		// will cause handler conflict
		if ((viewPart == null) || viewPart.getViewSite().getId().equals("org.netxms.ui.eclipse.objectview.view.tabbed_object_view")) //$NON-NLS-1$
			return;

		final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
      setActionDefinitionId("org.netxms.ui.eclipse.library.commands.export_to_csv_" + (selectionOnly ? "selection" : "all")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		handlerService.activateHandler(getActionDefinitionId(), new ActionHandler(this));
	}

	/**
    * Create "Export to CSV" action attached to handler service
    * 
    * @param viewPart owning view part
    * @param viewer viewer to copy rows from
    * @param selectionOnly true to export only selected rows
    */
	public ExportToCsvAction(IViewPart viewPart, ColumnViewer viewer, boolean selectionOnly)
	{
		this(viewPart, viewer, null, selectionOnly);
	}

	/**
    * Create "Export to CSV" action attached to handler service
    * 
    * @param viewPart owning view part
    * @param viewerProvider viewer provider
    * @param selectionOnly true to export only selected rows
    */
	public ExportToCsvAction(IViewPart viewPart, ViewerProvider viewerProvider, boolean selectionOnly)
	{
		this(viewPart, null, viewerProvider, selectionOnly);
	}

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
	@Override
	public void run()
	{
		FileDialog dlg = new FileDialog(viewPart.getSite().getShell(), SWT.SAVE);
      dlg.setOverwrite(true);
		dlg.setFilterExtensions(new String[] { "*.csv", "*.*" });
		dlg.setFilterNames(new String[] { "CSV files", "All files"});
		final String fileName = dlg.open();
		if (fileName == null)
			return;

      final List<String[]> data = getRowsFromViewer(true);
      new ConsoleJob(String.format(Messages.get().ExportToCsvAction_SaveTo, fileName), viewPart, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
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
						out.write(row[i].replace("\"", "\"\"")); //$NON-NLS-1$ //$NON-NLS-2$
						out.write('"');
					}
					out.newLine();
				}
				out.close();
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ExportToCsvAction_SaveError;
			}
		}.start();
	}
}
