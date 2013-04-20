/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.handlers.IHandlerService;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;

/**
 * Action for exporting selected table viewer rows to CSV file
 */
public class ExportToCsvAction extends Action
{
	private IViewPart viewPart;
	private TableViewer viewer;
	private boolean selectionOnly;
	
	/**
	 * Create refresh action attached to handler service
	 * 
	 * @param viewPart owning view part
	 */
	public ExportToCsvAction(IViewPart viewPart, TableViewer viewer, boolean selectionOnly)
	{
		super(selectionOnly ? "E&xport to CSV..." : "Export all to CSV...", Activator.getImageDescriptor("icons/csv.png"));
		
		this.viewPart = viewPart;
		this.viewer = viewer;
		this.selectionOnly = selectionOnly;

		// "Object Details" view can contain multiple widgets
		// with "Export to CSV" action defined, so binding it to handler service
		// will cause handler cvonflict
		if (viewPart.getViewSite().getId().equals("org.netxms.ui.eclipse.objectview.view.tabbed_object_view"))
			return;
		
		final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
      setActionDefinitionId("org.netxms.ui.eclipse.library.commands.export_to_csv_" + (selectionOnly ? "selection" : "all")); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
		handlerService.activateHandler(getActionDefinitionId(), new ActionHandler(this));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.action.Action#run()
	 */
	@Override
	public void run()
	{
		FileDialog dlg = new FileDialog(viewPart.getSite().getShell(), SWT.SAVE);
		final String fileName = dlg.open();
		if (fileName == null)
			return;
		
		int numColumns = viewer.getTable().getColumnCount();
		if (numColumns == 0)
			numColumns = 1;
		
		TableItem[] selection = selectionOnly ? viewer.getTable().getSelection() : viewer.getTable().getItems();
		final List<String[]> data = new ArrayList<String[]>(selection.length);
		for(TableItem item : selection)
		{
			String[] row = new String[numColumns];
			for(int i = 0; i < numColumns; i++)
				row[i] = item.getText(i);
			data.add(row);
		}
		
		new ConsoleJob(String.format("Save data to CSV file %s", fileName), viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				BufferedWriter out = new BufferedWriter(new FileWriter(fileName));
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
				return "Cannot save table data to file";
			}
		}.start();
	}
}
