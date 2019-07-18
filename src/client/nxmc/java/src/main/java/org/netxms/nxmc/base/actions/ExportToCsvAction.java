/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Action for exporting selected table viewer rows to CSV file
 */
public class ExportToCsvAction extends Action
{
   private static I18n i18n = LocalizationHelper.getI18n(ExportToCsvAction.class);

   private View view;
	private ColumnViewer viewer;
	private ViewerProvider viewerProvider;
	private boolean selectionOnly;
	
	/**
	 * Create "Export to CSV" action attached to handler service
	 * 
	 * @param viewPart
	 * @param viewer
	 * @param viewerProvider
	 * @param selectionOnly
	 */
   private ExportToCsvAction(View view, ColumnViewer viewer, ViewerProvider viewerProvider, boolean selectionOnly)
	{
      super(selectionOnly ? i18n.tr("E&xport to CSV...") : i18n.tr("Export all to CSV..."), SharedIcons.CSV);
		
		setId(selectionOnly ? "org.netxms.ui.eclipse.popupActions.ExportToCSV" : "org.netxms.ui.eclipse.actions.ExportToCSV"); //$NON-NLS-1$ //$NON-NLS-2$
		
      this.view = view;
		this.viewer = viewer;
		this.viewerProvider = viewerProvider;
		this.selectionOnly = selectionOnly;
	}
	
	/**
	 * Create "Export to CSV" action attached to handler service
	 * 
	 * @param viewPart
	 * @param viewer
	 * @param selectionOnly
	 */
   public ExportToCsvAction(View view, ColumnViewer viewer, boolean selectionOnly)
	{
      this(view, viewer, null, selectionOnly);
	}

	/**
	 * Create "Export to CSV" action attached to handler service
	 * 
	 * @param viewPart
	 * @param viewerProvider
	 * @param selectionOnly
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
		dlg.setFilterExtensions(new String[] { "*.csv", "*.*" });
		dlg.setFilterNames(new String[] { "CSV files", "All files"});
		final String fileName = dlg.open();
		if (fileName == null)
			return;

		if (viewerProvider != null)
			viewer = viewerProvider.getViewer();

		final List<String[]> data = new ArrayList<String[]>();
		if (viewer instanceof TableViewer)
		{
			int numColumns = ((TableViewer)viewer).getTable().getColumnCount();
			if (numColumns == 0)
				numColumns = 1;
			
			TableColumn[] columns = ((TableViewer)viewer).getTable().getColumns();
			String[] headerRow = new String[numColumns];
			for (int i = 0; i < numColumns; i++)
			{
				headerRow[i] = columns[i].getText();
			}
			data.add(headerRow);
			
			TableItem[] selection = selectionOnly ? ((TableViewer)viewer).getTable().getSelection() : ((TableViewer)viewer).getTable().getItems();
			for(TableItem item : selection)
			{
				String[] row = new String[numColumns];
				for(int i = 0; i < numColumns; i++)
					row[i] = item.getText(i);
				data.add(row);
			}
		}
		else if (viewer instanceof TreeViewer)
		{
			int numColumns = ((TreeViewer)viewer).getTree().getColumnCount();
			if (numColumns == 0)
				numColumns = 1;
			
			TreeItem[] selection = selectionOnly ? ((TreeViewer)viewer).getTree().getSelection() : ((TreeViewer)viewer).getTree().getItems();
			for(TreeItem item : selection)
			{
				String[] row = new String[numColumns];
				for(int i = 0; i < numColumns; i++)
					row[i] = item.getText(i);
				data.add(row);
				if (!selectionOnly)
				{
					addSubItems(item, data, numColumns);
				}
			}
		}
		
      new Job(String.format(i18n.tr("Save data to CSV file %s"), fileName), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				BufferedWriter out = new BufferedWriter(new FileWriter(fileName));
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
            return i18n.tr("Cannot save table data to file");
			}
		}.start();
	}

	/**
	 * @param item
	 * @param data
	 */
	private void addSubItems(TreeItem root, List<String[]> data, int numColumns)
	{
		for(TreeItem item : root.getItems())
		{
			String[] row = new String[numColumns];
			for(int i = 0; i < numColumns; i++)
				row[i] = item.getText(i);
			data.add(row);
			addSubItems(item, data, numColumns);
		}
	}
}
