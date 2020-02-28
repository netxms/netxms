/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.actionmanager.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.actionmanager.Activator;
import org.netxms.ui.eclipse.actionmanager.Messages;
import org.netxms.ui.eclipse.actionmanager.dialogs.EditActionDlg;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionComparator;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionLabelProvider;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionManagerFilter;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Action configuration view
 *
 */
public class ActionManager extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.actionmanager.views.ActionManager"; //$NON-NLS-1$
	
	private static final String TABLE_CONFIG_PREFIX = "ActionList"; //$NON-NLS-1$
	
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_RCPT = 2;
	public static final int COLUMN_SUBJ = 3;
	public static final int COLUMN_DATA = 4;
	public static final int COLUMN_CHANNEL = 5;
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	private Action actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
   private Action actionShowFilter;
	
   private ActionManagerFilter filter;
   private FilterText filterText;
   private IDialogSettings settings;
   private Composite content;

     
	/* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
      settings = Activator.getDefault().getDialogSettings();
   }

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{      
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());
      
      // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
		
		
		final String[] columnNames = { Messages.get().ActionManager_ColumnName, Messages.get().ActionManager_ColumnType, Messages.get().ActionManager_ColumnRcpt, Messages.get().ActionManager_ColumnSubj, Messages.get().ActionManager_ColumnData, "Channel name" };
		final int[] columnWidths = { 150, 90, 100, 120, 200, 100 };
		viewer = new SortableTableViewer(content, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		ActionLabelProvider labelProvider = new ActionLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setComparator(new ActionComparator());
		filter = new ActionManagerFilter(labelProvider);
		viewer.addFilter(filter);
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
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
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTable().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();

      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      // Set initial focus to filter input line
      if (actionShowFilter.isChecked())
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
		
		session.addListener(this);		
		refreshActionList();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
	
	/**
	 * Refresh action list
	 */
	private void refreshActionList()
	{
		new ConsoleJob(Messages.get().ActionManager_LoadJobName, this, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ActionManager_LoadJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				List<ServerAction> actions = session.getActions();
				synchronized(ActionManager.this.actions)
				{
					ActionManager.this.actions.clear();
					for(ServerAction a : actions)
					{
						ActionManager.this.actions.put(a.getId(), a);
					}
				}
				
				updateActionsList();
			}
		}.start();
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
      manager.add(new Separator());
      manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
      manager.add(new Separator());
      manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				refreshActionList();
			}
		};
		
		actionNew = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				createAction();
			}
		};
		actionNew.setText(Messages.get().ActionManager_ActionNew);
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);
		
		actionEdit = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				editAction();
			}
		};
		actionEdit.setText(Messages.get().ActionManager_ActionProperties);
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
		
		actionDelete = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				deleteActions();
			}
		};
		actionDelete.setText(Messages.get().ActionManager_ActionDelete);
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      
      actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(getBooleanFromSettings("ActionManager.showFilter", true));
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
      getSite().setSelectionProvider(viewer);
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionNew);
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}
	
	/**
	 * Create new action
	 */
	private void createAction()
	{
		final ServerAction action = new ServerAction(0);
		final EditActionDlg dlg = new EditActionDlg(getSite().getShell(), action, true);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.get().ActionManager_CreateJobName, this, Activator.PLUGIN_ID) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().ActionManager_CreateJobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					long id = session.createAction(action.getName());
					action.setId(id);
					session.modifyAction(action);
				}
			}.start();
		}
	}
	
	/**
	 * Edit currently selected action
	 */
	private void editAction()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final ServerAction action = (ServerAction)selection.getFirstElement();
		final EditActionDlg dlg = new EditActionDlg(getSite().getShell(), action, false);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.get().ActionManager_UpdateJobName, this, Activator.PLUGIN_ID) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().ActionManager_UpdateJobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.modifyAction(action);
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected actions
	 */
	private void deleteActions()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().ActionManager_Confirmation, Messages.get().ActionManager_ConfirmDelete))
			return;
		
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.get().ActionManager_DeleteJobName, this, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ActionManager_DeleteJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteAction(((ServerAction)objects[i]).getId());
				}
			}
		}.start();
	}

	/**
	 * Update actions list 
	 */
	private void updateActionsList()
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				synchronized(ActionManager.this.actions)
				{
					viewer.setInput(ActionManager.this.actions.values().toArray());
				}
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.ISessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		switch(n.getCode())
		{
			case SessionNotification.ACTION_CREATED:
			case SessionNotification.ACTION_MODIFIED:
				synchronized(actions)
				{
					actions.put(n.getSubCode(), (ServerAction)n.getObject());
				}
				updateActionsList();
				break;
			case SessionNotification.ACTION_DELETED:
				synchronized(actions)
				{
					actions.remove(n.getSubCode());
				}
				updateActionsList();
				break;
		}
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterText.setVisible(enable);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      settings.put("DataCollectionEditor.showFilter", enable);
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   private boolean getBooleanFromSettings(String name, boolean defval)
   {
      String v = settings.get(name);
      return (v != null) ? Boolean.valueOf(v) : defval;
   }
}
