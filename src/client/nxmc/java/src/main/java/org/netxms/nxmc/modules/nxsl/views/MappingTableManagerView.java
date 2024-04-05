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
package org.netxms.nxmc.modules.nxsl.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.mt.MappingTableDescriptor;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.dialogs.MappingTableCreationDialog;
import org.netxms.nxmc.modules.nxsl.views.helpers.MappingTableListComparator;
import org.netxms.nxmc.modules.nxsl.views.helpers.MappingTableListFilter;
import org.netxms.nxmc.modules.nxsl.views.helpers.MappingTableListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * List of mapping tables
 */
public class MappingTableManagerView extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(MappingTableManagerView.class);

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_FLAGS = 2;
	public static final int COLUMN_DESCRIPTION = 3;

   private NXCSession session;
	private SessionListener listener;
	private Map<Integer, MappingTableDescriptor> mappingTables = new HashMap<Integer, MappingTableDescriptor>();
	private SortableTableViewer viewer;
	private Action actionNewTable;
	private Action actionEditTable;
	private Action actionDeleteTables;

   /**
    * Create mapping table manager view
    */
   public MappingTableManagerView()
   {
      super(LocalizationHelper.getI18n(MappingTableManagerView.class).tr("Mapping Tables"), ResourceManager.getImageDescriptor("icons/config-views/mapping-tables.png"),
            "objects.mapping-tables.manager", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		final int[] widths = { 80, 160, 80, 400 };
      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Flags"), i18n.tr("Description") };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new MappingTableListLabelProvider());
		viewer.setComparator(new MappingTableListComparator());
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editTable();
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionEditTable.setEnabled(selection.size() == 1);
				actionDeleteTables.setEnabled(selection.size() > 0);
			}
		});
      MappingTableListFilter filter = new MappingTableListFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "MappingTablesList");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, "MappingTablesList");
			}
		});

		createActions();
		createPopupMenu();

		listener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.MAPPING_TABLE_UPDATED)
				{
					final Integer id = (int)n.getSubCode();
               getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh(id);
						}
					});
				}
				else if (n.getCode() == SessionNotification.MAPPING_TABLE_DELETED)
				{
					final Integer id = (int)n.getSubCode();
               getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							mappingTables.remove(id);
                     viewer.refresh();
						}
					});
				}
			}
		};
		session.addListener(listener);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
	@Override
	public void dispose()
	{
		if ((listener != null) && (session != null))
			session.removeListener(listener);
		super.dispose();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionNewTable = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createNewTable();
			}
		};
      addKeyBinding("M1+N", actionNewTable);

      actionEditTable = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editTable();
			}
		};
		actionEditTable.setEnabled(false);

      actionDeleteTables = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteTables();
			}
		};
		actionDeleteTables.setEnabled(false);
      addKeyBinding("M1+D", actionDeleteTables);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionNewTable);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNewTable);
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
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionNewTable);
		mgr.add(actionEditTable);
		mgr.add(actionDeleteTables);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      refresh(0);
   }

	/**
	 * Refresh
	 */
	private void refresh(final int tableId)
	{
      new Job(i18n.tr("Loading list of mapping tables"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<MappingTableDescriptor> tables = session.listMappingTables();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
                  if (viewer.getControl().isDisposed())
                     return;

						if (tableId > 0)
						{
							for(MappingTableDescriptor d : tables)
							{
								if (d.getId() == tableId)
								{
									MappingTableDescriptor od = mappingTables.get(tableId);
									if (!d.equals(od))
										mappingTables.put(d.getId(), d);
									break;
								}
							}
						}
						else
						{
							mappingTables.clear();
							for(MappingTableDescriptor d : tables)
								mappingTables.put(d.getId(), d);
						}
						viewer.setInput(mappingTables.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot load list of mapping tables");
			}
		}.start();
	}

	/**
	 * Create new table
	 */
	private void createNewTable()
	{
      final MappingTableCreationDialog dlg = new MappingTableCreationDialog(getWindow().getShell());
		if (dlg.open() != Window.OK)
			return;

      new Job(i18n.tr("Creating mapping table"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Integer assignedId = session.createMappingTable(dlg.getName(), dlg.getDescription(), 0);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						// descriptor may already be presented if creation notification
						// was already processed
						MappingTableDescriptor d = mappingTables.get(assignedId);
						if (d == null)
						{
							d = new MappingTableDescriptor(assignedId, dlg.getName(), dlg.getDescription(), 0);
							mappingTables.put(assignedId, d);
						}
						viewer.setInput(mappingTables.values().toArray());
						viewer.setSelection(new StructuredSelection(d));
						editTable();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot create mapping table");
			}
		}.start();
	}

	/**
	 * Edit selected table
	 */
	private void editTable()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
			return;
		
		MappingTableDescriptor d = (MappingTableDescriptor)selection.getFirstElement();
      openView(new MappingTableEditorView(d.getId(), d.getName()));
	}

	/**
	 * Delete selected tables
	 */
	private void deleteTables()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
			return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete"), i18n.tr("Are you sure you want to delete selected mapping tables?")))
			return;

		final List<Integer> tables = new ArrayList<Integer>(selection.size());
		for(Object o : selection.toList())
		{
			if (o instanceof MappingTableDescriptor)
				tables.add(((MappingTableDescriptor)o).getId());
		}
      new Job(i18n.tr("Deleting mapping tables"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				for(Integer id : tables)
					session.deleteMappingTable(id);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(Integer id : tables)
							mappingTables.remove(id);
						viewer.setInput(mappingTables.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot delete mapping table");
			}
		}.start();
	}

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
