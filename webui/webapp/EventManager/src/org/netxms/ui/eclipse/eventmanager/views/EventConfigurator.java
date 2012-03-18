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
package org.netxms.ui.eclipse.eventmanager.views;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.eventmanager.Activator;
import org.netxms.ui.eclipse.eventmanager.dialogs.EditEventTemplateDialog;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateComparator;
import org.netxms.ui.eclipse.eventmanager.views.helpers.EventTemplateLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Event configuration view
 * 
 */
public class EventConfigurator extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.eventmanager.view.event_configurator";
	public static final String JOB_FAMILY = "EventConfiguratorJob";

	private static final String TABLE_CONFIG_PREFIX = "EventTemplateList";
	
	// Columns
	public static final int COLUMN_CODE = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_SEVERITY = 2;
	public static final int COLUMN_FLAGS = 3;
	public static final int COLUMN_MESSAGE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private HashMap<Long, EventTemplate> eventTemplates;
	private SortableTableViewer viewer;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
	private RefreshAction actionRefresh;
	private NXCSession session;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		final String[] names = { "Code", "Name", "Severity", "Flags", "Message", "Description" };
		final int[] widths = { 70, 200, 90, 50, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new EventTemplateLabelProvider());
		viewer.setComparator(new EventTemplateComparator());
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
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});

		makeActions();
		contributeToActionBars();
		createPopupMenu();

		refreshView();
		session.addListener(this);
	}

	/**
	 * Refresh view
	 */
	private void refreshView()
	{
		new ConsoleJob("Open event configuration", this, Activator.PLUGIN_ID, JOB_FAMILY)
		{
			@Override
			protected String getErrorMessage()
			{
				return "Cannot open event configuration";
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
			case NXCNotification.EVENT_TEMPLATE_MODIFIED:
				new UIJob(viewer.getControl().getDisplay(), "Update event template list") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
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
						return Status.OK_STATUS;
					}
				}.schedule();
				break;
			case NXCNotification.EVENT_TEMPLATE_DELETED:
				new UIJob(viewer.getControl().getDisplay(), "Remove event template from list") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						eventTemplates.remove(n.getSubCode());
						viewer.setInput(eventTemplates.values().toArray());
						return Status.OK_STATUS;
					}
				}.schedule();
				break;
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
		manager.add(actionDelete);
		manager.add(actionEdit);
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
		manager.add(actionEdit);
		manager.add(actionDelete);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void makeActions()
	{
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				refreshView();
			}
		};

		actionNew = new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				createNewEventTemplate();
			}
		};
		actionNew.setText("&New event template...");
		actionNew.setImageDescriptor(Activator.getImageDescriptor("icons/new.png"));

		actionEdit = new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				editEventTemplate();
			}
		};
		actionEdit.setText("&Properties...");
		actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png"));
		actionEdit.setEnabled(false);

		actionDelete = new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				deleteEventTemplate();
			}
		};
		actionDelete.setText("&Delete");
		actionDelete.setImageDescriptor(Activator.getImageDescriptor("icons/delete.png"));
		actionDelete.setEnabled(false);
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
			private static final long serialVersionUID = 1L;

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
			new ConsoleJob("Create new event template", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot create new event template";
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
			new ConsoleJob("Update event template", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot update event template";
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

		final String message = "Do you really wish to delete selected event template" + ((selection.size() > 1) ? "s?" : "?");
		final Shell shell = getViewSite().getShell();
		if (!MessageDialog.openQuestion(shell, "Confirm event template deletion", message))
		{
			return;
		}
		
		new ConsoleJob("Delete event templates", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot delete event template";
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
}
