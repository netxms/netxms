/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.snmp.views;

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
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.propertypages.SnmpTrapGeneral;
import org.netxms.nxmc.modules.snmp.propertypages.SnmpTrapParameters;
import org.netxms.nxmc.modules.snmp.propertypages.SnmpTrapTransformation;
import org.netxms.nxmc.modules.snmp.views.helpers.SnmpTrapComparator;
import org.netxms.nxmc.modules.snmp.views.helpers.SnmpTrapLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * SNMP trap configuration editor
 *
 */
public class SnmpTrapEditor extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(SnmpTrapEditor.class);

	public static final int COLUMN_ID = 0;
	public static final int COLUMN_TRAP_OID = 1;
	public static final int COLUMN_EVENT = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	
	private static final String TABLE_CONFIG_PREFIX = "SnmpTrapEditor"; //$NON-NLS-1$
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
	private Map<Long, SnmpTrap> traps = new HashMap<Long, SnmpTrap>();
   private SessionListener sessionListener;

   /**
    * Create SNMP trap editor view
    */
   public SnmpTrapEditor()
   {
      super(LocalizationHelper.getI18n(SnmpTrapEditor.class).tr("SNMP Traps"), ResourceManager.getImageDescriptor("icons/config-views/trapeditor.png"), "configuration.snmp-traps", true);

      session = Registry.getSession();
      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.TRAP_CONFIGURATION_CREATED:
               case SessionNotification.TRAP_CONFIGURATION_MODIFIED:
                  synchronized(traps)
                  {
                     traps.put(n.getSubCode(), (SnmpTrap)n.getObject());
                  }
                  updateTrapList();
                  break;
               case SessionNotification.TRAP_CONFIGURATION_DELETED:
                  synchronized(traps)
                  {
                     traps.remove(n.getSubCode());
                  }
                  updateTrapList();
                  break;
            }
         }
      };
   }

   /**
    * @param parent
    */
	@Override
   public void createContent(Composite parent)
	{
      final String[] columnNames = { i18n.tr("ID"), i18n.tr("OID"), i18n.tr("Event"), i18n.tr("Description") };
		final int[] columnWidths = { 70, 200, 100, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpTrapLabelProvider());
		viewer.setComparator(new SnmpTrapComparator());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEdit.run();
			}
		});
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
			}
		});
		
		createActions();
		createPopupMenu();
		
      session.addListener(sessionListener);

      refresh();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createTrap();
			}
		};
		
      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editTrap();
			}
		};
		
      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteTraps();
			}
		};
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
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
	 * 
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionNew);
      manager.add(actionEdit);
		manager.add(actionDelete);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
	@Override
	public void dispose()
	{
      session.removeListener(sessionListener);
		super.dispose();
	}

	/**
	 * Refresh trap list
	 */
   @Override
   public void refresh()
	{
      new Job(i18n.tr("Load SNMP trap mappings"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				List<SnmpTrap> list = session.getSnmpTrapsConfiguration();
				synchronized(traps)
				{
					traps.clear();
					for(SnmpTrap t : list)
					{
						traps.put(t.getId(), t);
					}
				}
				updateTrapList();
			}	

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load SNMP trap mappings");
         }
      }.start();
	}

	/**
	 * Update trap list
	 */
	private void updateTrapList()
	{
      getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				synchronized(traps)
				{
					viewer.setInput(traps.values().toArray());
				}
			}
		});
	}

	/**
	 * Delete selected traps
	 */
   private void deleteTraps()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		final Object[] objects = selection.toArray();
      new Job(i18n.tr("Delete SNMP trap mapping"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
					session.deleteSnmpTrapConfiguration(((SnmpTrap)objects[i]).getId());
			}

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete SNMP trap mapping");
         }
		}.start();
	}

	/**
	 * Edit selected trap
	 */
	protected void editTrap()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
		if (selection.size() != 1)
			return;

		final SnmpTrap trap = (SnmpTrap)selection.getFirstElement();
      if (showTrapConfigurationDialog(trap))
		{
         new Job(i18n.tr("Update SNMP trap mapping"), this) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
               session.modifySnmpTrapConfiguration(trap);
				}

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot update SNMP trap mapping");
            }
			}.start();
		}
	}

	/**
	 * Create new trap
	 */
	protected void createTrap()
	{
		final SnmpTrap trap = new SnmpTrap();
		trap.setEventCode(500);		// SYS_UNMATCHED_SNMP_TRAP

		if (showTrapConfigurationDialog(trap))
		{
         new Job(i18n.tr("Create SNMP trap mapping"), this) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
					trap.setId(session.createSnmpTrapConfiguration());
					session.modifySnmpTrapConfiguration(trap);
				}

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create SNMP trap mapping");
            }
			}.start();
		}
	}

	/**
	 * Show SNMP trap configuration dialog
	 * 
	 * @param trap SNMP trap configuration object
	 * @return true if OK was pressed
	 */
	private boolean showTrapConfigurationDialog(SnmpTrap trap)
	{
	   PreferenceManager pm = new PreferenceManager();
	   pm.addToRoot(new PreferenceNode("general", new SnmpTrapGeneral(trap)));
      pm.addToRoot(new PreferenceNode("parameters", new SnmpTrapParameters(trap)));
      pm.addToRoot(new PreferenceNode("transformation", new SnmpTrapTransformation(trap)));

      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("Edit SNMP Trap Mapping"));
	   return dlg.open() == Window.OK;
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
