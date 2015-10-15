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
package org.netxms.ui.eclipse.eventmanager.views;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
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
import org.eclipse.jface.viewers.StructuredSelection;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.Messages;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventTemplateDialog;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateComparator;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateFilter;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Event configuration view
 * 
 */
public class EventConfigurator extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.view.event_configurator"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "EventConfiguratorJob"; //$NON-NLS-1$

	private static final String TABLE_CONFIG_PREFIX = "EventTemplateList"; //$NON-NLS-1$
	
	// Columns
	public static final int COLUMN_CODE = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FLAGS = 3;
	public static final int COLUMN_MESSAGE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private HashMap<Long, EventTemplate> eventTemplates;
	private SortableTableViewer viewer;
   private FilterText filterControl;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
   private Action actionShowFilter;
	private Action actionRefresh;
	private NXCSession session;
	private EventTemplateFilter filter;
	private boolean filterEnabled;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      filterEnabled = settings.getBoolean("EventConfigurator.filterEnabled"); //$NON-NLS-1$
   }

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
      parent.setLayout(new FormLayout());
      
      // Create filter area
      filterControl = new FilterText(parent, SWT.NONE);
      filterControl.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterControl.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });
		
		final String[] names = { Messages.get().EventConfigurator_ColCode, Messages.get().EventConfigurator_ColName, Messages.get().EventConfigurator_ColSeverity, Messages.get().EventConfigurator_ColFlags, Messages.get().EventConfigurator_ColMessage, Messages.get().EventConfigurator_ColDescription };
		final int[] widths = { 70, 200, 90, 50, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new EventTemplateLabelProvider());
		viewer.setComparator(new EventTemplateComparator());
		filter = new EventTemplateFilter();
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
      fd.top = new FormAttachment(filterControl);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterControl.setLayoutData(fd);
		
      activateContext();
		createActions();
		contributeToActionBars();
		createPopupMenu();

		refreshView();
		session.addListener(this);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterControl.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
	}

	/**
	 * Refresh view
	 */
	private void refreshView()
	{
		new ConsoleJob(Messages.get().EventConfigurator_OpenJob_Title, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().EventConfigurator_OpenJob_Error;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<EventTemplate> list = session.getEventTemplates();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						eventTemplates = new HashMap<Long, EventTemplate>(list.size());
						for(final EventTemplate t: list)
						{
							eventTemplates.put(t.getCode(), t);
						}
						viewer.setInput(eventTemplates.values().toArray());
					}
				});
			}
		}.start();
	}

	/**
	 * Process client session notifications
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		switch(n.getCode())
		{
			case SessionNotification.EVENT_TEMPLATE_MODIFIED:
				viewer.getControl().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						EventTemplate oldTmpl = eventTemplates.get(n.getSubCode());
						if (oldTmpl != null)
						{
							oldTmpl.setAll((EventTemplate)n.getObject());
							viewer.update(oldTmpl, null);
						}
						else
						{
							eventTemplates.put(n.getSubCode(), (EventTemplate)n.getObject());
							viewer.setInput(eventTemplates.values().toArray());
						}
					}
				});
				break;
			case SessionNotification.EVENT_TEMPLATE_DELETED:
				viewer.getControl().getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						eventTemplates.remove(n.getSubCode());
						viewer.setInput(eventTemplates.values().toArray());
					}
				});
				break;
		}
	}

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.eventmanager.contexts.EventConfigurator"); //$NON-NLS-1$
      }
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
      
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshView();
			}
		};

		actionNew = new Action(Messages.get().EventConfigurator_NewEvent, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createNewEventTemplate();
			}
		};
		actionNew.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.new_event_template"); //$NON-NLS-1$
      handlerService.activateHandler(actionNew.getActionDefinitionId(), new ActionHandler(actionNew));

		actionEdit = new Action(Messages.get().EventConfigurator_Properties, SharedIcons.EDIT) { //$NON-NLS-1$
			@Override
			public void run()
			{
				editEventTemplate();
			}
		};
		actionEdit.setEnabled(false);

		actionDelete = new Action(Messages.get().EventConfigurator_Delete, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteEventTemplate();
			}
		};
		actionDelete.setEnabled(false);

      actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.eventmanager.commands.show_filter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
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

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(actionNew);
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Create new event template
	 */
	protected void createNewEventTemplate()
	{
		final EventTemplate etmpl = new EventTemplate(0);
		EditEventTemplateDialog dlg = new EditEventTemplateDialog(getSite().getShell(), etmpl, false);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.get().EventConfigurator_CreateJob_Title, this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().EventConfigurator_CreateJob_Error;
				}
	
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					long code = session.generateEventCode();
					etmpl.setCode(code);
					session.modifyEventTemplate(etmpl);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							eventTemplates.put(etmpl.getCode(), etmpl);
							viewer.setInput(eventTemplates.values().toArray());
							viewer.setSelection(new StructuredSelection(etmpl), true);
						}
					});
				}
			}.start();
		}
	}

	/**
	 * Edit currently selected event template
	 */
	protected void editEventTemplate()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final EventTemplate etmpl = new EventTemplate((EventTemplate)selection.getFirstElement());
		EditEventTemplateDialog dlg = new EditEventTemplateDialog(getSite().getShell(), etmpl, false);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.get().EventConfigurator_UpdateJob_Title, this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().EventConfigurator_UpdateJob_Error;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.modifyEventTemplate(etmpl);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							eventTemplates.put(etmpl.getCode(), etmpl);
							viewer.setInput(eventTemplates.values());
							viewer.setSelection(new StructuredSelection(etmpl));
						}
					});
				}
			}.start();
		}
	}

	/**
	 * Delete selected event templates
	 */
	protected void deleteEventTemplate()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();

		final String message = ((selection.size() > 1) ? Messages.get().EventConfigurator_DeleteConfirmation_Plural : Messages.get().EventConfigurator_DeleteConfirmation_Singular);
		final Shell shell = getViewSite().getShell();
		if (!MessageDialogHelper.openQuestion(shell, Messages.get().EventConfigurator_DeleteConfirmationTitle, message))
		{
			return;
		}
		
		new ConsoleJob(Messages.get().EventConfigurator_DeleteJob_Title, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().EventConfigurator_DeleteJob_Error;
			}

			@SuppressWarnings("unchecked")
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Iterator<EventTemplate> it = selection.iterator();
				while(it.hasNext())
				{
					session.deleteEventTemplate(it.next().getCode());
				}
			}
		}.start();
	}

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterControl.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterControl) : new FormAttachment(0, 0);
      viewer.getControl().getParent().layout();
      if (enable)
      {
         filterControl.setFocus();
      }
      else
      {
         filterControl.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      actionShowFilter.setChecked(enable);
   }
	
   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      filter.setFilterText(filterControl.getText());
      viewer.refresh();
   }
   
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      settings.put("EventConfigurator.filterEnabled", filterEnabled); //$NON-NLS-1$
      
		session.removeListener(this);
		super.dispose();
	}
}
