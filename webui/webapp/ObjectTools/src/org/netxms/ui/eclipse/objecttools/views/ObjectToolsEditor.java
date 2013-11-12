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
package org.netxms.ui.eclipse.objecttools.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.ObjectToolsAdapterFactory;
import org.netxms.ui.eclipse.objecttools.dialogs.CreateNewToolDialog;
import org.netxms.ui.eclipse.objecttools.views.helpers.ObjectToolsComparator;
import org.netxms.ui.eclipse.objecttools.views.helpers.ObjectToolsLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for object tools
 */
@SuppressWarnings("restriction")
public class ObjectToolsEditor extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.objecttools.views.ObjectToolsEditor"; //$NON-NLS-1$

	private static final String TABLE_CONFIG_PREFIX = "ObjectToolsEditor"; //$NON-NLS-1$
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_TYPE = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	
	private Map<Long, ObjectTool> tools = new HashMap<Long, ObjectTool>();
	private SortableTableViewer viewer;
	private NXCSession session;
	private Action actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();

		// Initiate loading of required plugins if they was not loaded yet
		try
		{
			Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		}
		catch(Exception e)
		{
		}
		
		parent.setLayout(new FillLayout());
		
		final String[] columnNames = { Messages.ObjectToolsEditor_ColId, Messages.ObjectToolsEditor_ColName, Messages.ObjectToolsEditor_ColType, Messages.ObjectToolsEditor_ColDescr };
		final int[] columnWidths = { 90, 200, 100, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ObjectToolsLabelProvider());
		viewer.setComparator(new ObjectToolsComparator());
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
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session.addListener(this);
		
		refreshToolList();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refreshToolList();
			}
		};
		
		actionNew = new Action(Messages.ObjectToolsEditor_New) {
			@Override
			public void run()
			{
				createTool();
			}
		};
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);

		actionEdit = new PropertyDialogAction(getSite(), viewer) {
			@Override
			public void run()
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if (selection.size() != 1)
					return;
				final long toolId = ((ObjectTool)selection.getFirstElement()).getId();
				
				// Check if we have details loaded or can load before showing properties dialog
				// If there will be error, adapter factory will show error message to user
				if (Platform.getAdapterManager().getAdapter(selection.getFirstElement(), ObjectToolDetails.class) == null)
					return;
				
				super.run();
				
				ObjectToolDetails details = ObjectToolsAdapterFactory.getDetailsFromCache(toolId);
				if ((details != null) && details.isModified())
				{
					saveObjectTool(details);
				}
			}
		};
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
		
		actionDelete = new Action(Messages.ObjectToolsEditor_Delete) {
			@Override
			public void run()
			{
				deleteTools();
			}
		};
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
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
		manager.add(actionDelete);
		manager.add(actionEdit);
		manager.add(new Separator());
		manager.add(actionRefresh);
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

		// Create menu
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
		mgr.add(actionNew);
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}

	/**
	 * Refresh tool list
	 */
	private void refreshToolList()
	{
		new ConsoleJob(Messages.ObjectToolsEditor_JobGetConfig, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ObjectTool> tl = session.getObjectTools();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						tools.clear();
						for(ObjectTool t : tl)
							tools.put(t.getId(), t);
						viewer.setInput(tools.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.ObjectToolsEditor_JobGetConfigError;
			}
		}.start();
	}
	
	/**
	 * Create new tool
	 */
	private void createTool()
	{
		final CreateNewToolDialog dlg = new CreateNewToolDialog(getSite().getShell());
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.ObjectToolsEditor_JobNewId, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final long toolId = session.generateObjectToolId();
					final ObjectToolDetails details = new ObjectToolDetails(toolId, dlg.getType(), dlg.getName());
					session.modifyObjectTool(details);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, details);
							dlg.open();
							if (details.isModified())
								saveObjectTool(details);
						}
					});
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.ObjectToolsEditor_JobNewIdError;
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected tools
	 */
	private void deleteTools()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.ObjectToolsEditor_Confirmation, Messages.ObjectToolsEditor_DeleteConfirmation))
			return;
		
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.ObjectToolsEditor_JobDelete, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.ObjectToolsEditor_JobDeleteError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteObjectTool(((ObjectTool)objects[i]).getId());
				}
			}
		}.start();
	}
	
	/**
	 * Save object tool configuration on server
	 * 
	 * @param details object tool details
	 */
	private void saveObjectTool(final ObjectToolDetails details)
	{
		new ConsoleJob(Messages.ObjectToolsEditor_JobSave, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObjectTool(details);
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.ObjectToolsEditor_JobSaveError;
			}

			@Override
			protected void jobFailureHandler()
			{
				// Was unable to save configuration on server, invalidate cache
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						ObjectToolsAdapterFactory.deleteFromCache(details.getId());
					}
				});
			}
		}.schedule();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(final SessionNotification n)
	{
		switch(n.getCode())
		{
			case NXCNotification.OBJECT_TOOLS_CHANGED:
				refreshToolList();
				break;
			case NXCNotification.OBJECT_TOOL_DELETED:
				new UIJob(getSite().getShell().getDisplay(), "Delete object tool from list") { //$NON-NLS-1$
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						ObjectToolsAdapterFactory.deleteFromCache(n.getSubCode());
						tools.remove(n.getSubCode());
						viewer.setInput(tools.values().toArray());
						return Status.OK_STATUS;
					}
				}.schedule();
				break;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		ObjectToolsAdapterFactory.clearCache();
		super.dispose();
	}
}
