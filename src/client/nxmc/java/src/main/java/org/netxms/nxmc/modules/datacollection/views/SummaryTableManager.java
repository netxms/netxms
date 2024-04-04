/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.SummaryTableColumns;
import org.netxms.nxmc.modules.datacollection.propertypages.SummaryTableFilterPropertyPage;
import org.netxms.nxmc.modules.datacollection.propertypages.SummaryTableGeneral;
import org.netxms.nxmc.modules.datacollection.views.helpers.SummaryTableComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.SummaryTableFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.SummaryTableLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Manager of DCI summary tables
 */
public class SummaryTableManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryTableManager.class);
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_MENU_PATH = 1;
	public static final int COLUMN_TITLE = 2;
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private SessionListener listener = null;
	private Map<Integer, DciSummaryTableDescriptor> descriptors = new HashMap<Integer, DciSummaryTableDescriptor>();
	
	private Action actionCreateSingleValue;
   private Action actionCreateTableValue;
	private Action actionEdit;
	private Action actionDelete;

   /**
    * Constructor
    */
   public SummaryTableManager() 
   {
      super(LocalizationHelper.getI18n(SummaryTableManager.class).tr("DCI Summary Tables"), ResourceManager.getImageDescriptor("icons/config-views/summary_table.png"), "configuration.summary-tables", true);
      session = Registry.getSession();    
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
   protected void createContent(Composite parent)
   {
		final String[] names = { i18n.tr("ID"), i18n.tr("Menu Path"), i18n.tr("Title") };
		final int[] widths = { 90, 250, 200 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_MENU_PATH, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SummaryTableLabelProvider());
		viewer.setComparator(new SummaryTableComparator());

		SummaryTableFilter filter = new SummaryTableFilter();
		viewer.addFilter(filter);
		setFilterClient(viewer, filter);

		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				actionEdit.setEnabled(selection.size() == 1);
				actionDelete.setEnabled(selection.size() > 0);
			}
		});
		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editSummaryTable();
			}
		});

		createActions();
		createPopupMenu();

		refresh();

		listener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.DCI_SUMMARY_TABLE_DELETED)
				{
					final int id = (int)n.getSubCode();
					getWindow().getShell().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							if (descriptors.remove(id) != null)
								viewer.setInput(descriptors.values().toArray());
						}
					});
				}
				else if (n.getCode() == SessionNotification.DCI_SUMMARY_TABLE_UPDATED)
				{
				   getWindow().getShell().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh();
						}
					});
				}
			}
		};
		session.addListener(listener);
	}

	/**
	 * Refresh viewer
	 */
	@Override
	public void refresh()
	{
		new Job("Get configured DCI summary tables", this) { //$NON-NLS-1$
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<DciSummaryTableDescriptor> list = session.listDciSummaryTables();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						descriptors.clear();
						for(DciSummaryTableDescriptor d : list)
							descriptors.put(d.getId(), d);
						viewer.setInput(descriptors.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get DCI summary tables from server");
			}
		}.start();
	}

	/**
	 * 
	 */
	private void createActions()
	{       
		// create add action for single value table
		actionCreateSingleValue = new Action("Create new summary table...", SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createSummaryTable(false);
			}
		};
		
		actionCreateTableValue = new Action("Create new summary table for table DCIs...", ResourceManager.getImageDescriptor("icons/new_table.png")) {
		   @Override
		   public void run()
		   {
		      createSummaryTable(true);
		   }
		};

		// create edit action
		actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editSummaryTable();
			}
		};
		actionEdit.setEnabled(false);

		// create delete action
		actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteSelection();
			}
		};
		actionDelete.setEnabled(false);
	}

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
	@Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreateSingleValue);
      manager.add(actionCreateTableValue);
      super.fillLocalToolBar(manager);
   }

   /**
	 * Create pop-up menu for variable list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
					mgr.add(actionCreateSingleValue);
					mgr.add(actionCreateTableValue);
					mgr.add(actionEdit);
					mgr.add(actionDelete);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Create new loyalty program
	 */
	private void createSummaryTable(boolean isTableSource)
	{
		DciSummaryTable t = new DciSummaryTable("", "", isTableSource); 
		if (showPropertyPages(t, true))
		{
			// was saved to server
			DciSummaryTableDescriptor d = new DciSummaryTableDescriptor(t);
			descriptors.put(d.getId(), d);
			viewer.setInput(descriptors.values().toArray());
			viewer.setSelection(new StructuredSelection(d));
		}
	}
	
	/**
	 * Show property pages for provided summary table
	 * 
	 * @param t summary table
	 * @return
	 */
	private boolean showPropertyPages(DciSummaryTable t, boolean isNew)
	{
	   PreferenceManager pm = new PreferenceManager();
	   pm.addToRoot(new PreferenceNode("general", new SummaryTableGeneral(t)));
      pm.addToRoot(new PreferenceNode("filter", new SummaryTableFilterPropertyPage(t)));
      if (!t.isTableSoure())
         pm.addToRoot(new PreferenceNode("columns", new SummaryTableColumns(t)));

      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, 
            isNew ? i18n.tr("Create DCI Summary Table") : i18n.tr("Edit Summary Table"));
      return dlg.open() == Window.OK;
	}

	/**
	 * Edit existing dataset
	 */
	private void editSummaryTable()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final DciSummaryTableDescriptor d = (DciSummaryTableDescriptor)selection.getFirstElement();
		new Job(i18n.tr("Read DCI summary table config"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final DciSummaryTable t = session.getDciSummaryTable(d.getId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   showPropertyPages(t, false);
						d.updateFromTable(t);
						viewer.update(d, null);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot read DCI summary table config");
			}
		}.start();
	}
	
	/**
	 * Delete selected datasets
	 */
	private void deleteSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 0)
			return;
		
		if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"), i18n.tr("Selected DCI summary tables will be deleted. Are you really sure?")))
			return;
	
		final int[] idList = new int[selection.size()];
		int i = 0;
		for(Object o : selection.toList())
			idList[i++] = ((DciSummaryTableDescriptor)o).getId();
		
		new Job(i18n.tr("Delete DCI summary tables from server"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				for(int id : idList)
					session.deleteDciSummaryTable(id);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(int id : idList)
							descriptors.remove(id);
						viewer.setInput(descriptors.values().toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot delete DCI summary table");
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((session != null) && (listener != null))
			session.removeListener(listener);
		super.dispose();
	}

   @Override
   public boolean isModified()
   {
      return false;
   }

   @Override
   public void save()
   {
   }
}
