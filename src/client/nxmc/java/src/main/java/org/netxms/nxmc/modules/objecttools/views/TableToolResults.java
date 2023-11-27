/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.views;

import java.util.Arrays;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.Table;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableItemComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableLabelProvider;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Display results of table tool execution
 */
public class TableToolResults extends ObjectToolResultView
{
   private final I18n i18n = LocalizationHelper.getI18n(TableToolResults.class);
   
	private SortableTableViewer viewer;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;

   /**
    * Constructor
    */
   public TableToolResults(ObjectContext node, ObjectTool tool)
   {
      super(node, tool, ResourceManager.getImageDescriptor("icons/object-tools/table.gif"), false);
	} 

   /**
    * Clone constructor
    */
   protected TableToolResults()
   {
      super();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		viewer = new SortableTableViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new TableContentProvider());
		viewer.setLabelProvider(new TableLabelProvider());

		createActions();
		createPopupMenu();
		refreshTable();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionExportAllToCsv);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
	}

	/**
	 * Refresh table
	 */
	public void refreshTable()
	{
		viewer.setInput(null);
		new Job(String.format(i18n.tr("Load data for table tool %s"), tool.getName()), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table table = session.executeTableTool(tool.getId(), object.object.getObjectId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   if (!table.getTitle().isEmpty())
					      setName(object.object.getObjectName() + ": " + table.getTitle()); //$NON-NLS-1$
					   updateViewer(table);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(i18n.tr("Cannot get data for table tool %s"), tool.getName());
			}
		}.start();
	}
	
	/**
	 * Update viewer with fresh table data
	 * 
	 * @param table table
	 */
	private void updateViewer(final Table table)
	{
		if (!viewer.isInitialized())
		{
			String[] names = table.getColumnDisplayNames();
			final int[] widths = new int[names.length];
			Arrays.fill(widths, 100);
			viewer.createColumns(names, widths, 0, SWT.UP);
			WidgetHelper.restoreTableViewerSettings(viewer, "TableToolResults." + Long.toString(tool.getId())); //$NON-NLS-1$
			viewer.getTable().addDisposeListener(new DisposeListener() {
				@Override
				public void widgetDisposed(DisposeEvent e)
				{
					WidgetHelper.saveTableViewerSettings(viewer, "TableToolResults." + Long.toString(tool.getId())); //$NON-NLS-1$
				}
			});
			viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
		}
		((TableLabelProvider)viewer.getLabelProvider()).setColumns(table.getColumns());
		viewer.setInput(table);
	}
}
