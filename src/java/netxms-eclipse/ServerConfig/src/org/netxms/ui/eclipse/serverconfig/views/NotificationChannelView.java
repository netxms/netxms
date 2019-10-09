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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.NotificationChannelDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.NotificationChannelLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.NotificationChannelListComparator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * List of notification channels
 */
public class NotificationChannelView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NotificationChannelView"; //$NON-NLS-1$

	// Columns
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_DESCRIPTION = 1;
	public static final int COLUMN_DRIVER = 2;
	
	private NXCSession session;
	private SessionListener listener;
	private List<NotificationChannel> notificationChannelList;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionNewChannel;
	private Action actionEditChannel;
	private Action actionDeleteChannel;
	private NotificationChannel selectedChannel;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final int[] widths = { 80, 200, 80 };
		final String[] names = { "Name", "Description", "Driver" };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new NotificationChannelLabelProvider());
		viewer.setComparator(new NotificationChannelListComparator());
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editChannel();
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionEditChannel.setEnabled(selection.size() == 1);
				actionDeleteChannel.setEnabled(selection.size() > 0);
			}
		});
		
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		WidgetHelper.restoreTableViewerSettings(viewer, settings, "NotificationChannelList"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, settings, "NotificationChannelList"); //$NON-NLS-1$
			}
		});
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = ConsoleSharedData.getSession();
		refresh();

		final Display display = getSite().getShell().getDisplay();
		listener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.NOTIFICATION_CHANNEL_CHANGED)
				{
					display.asyncExec(new Runnable() {
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((listener != null) && (session != null))
			session.removeListener(listener);
		super.dispose();
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
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionNewChannel = new Action("Create new channel", SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createNewChannel();
			}
		};
		
		actionEditChannel = new Action("Edit", SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editChannel();
			}
		};
		actionEditChannel.setEnabled(false);
		
		actionDeleteChannel = new Action("Delete", SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteChannel();
			}
		};
		actionDeleteChannel.setEnabled(false);
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionNewChannel);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNewChannel);
		manager.add(new Separator());
		manager.add(actionRefresh);
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

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionNewChannel);
		mgr.add(actionEditChannel);
		mgr.add(actionDeleteChannel);
	}
	
	/**
	 * Refresh
	 */
	private void refresh()
	{
		new ConsoleJob("Get notification channels", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				notificationChannelList = session.getNotificationChannels();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(notificationChannelList);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Error while getting notificaiton channels";
			}
		}.start();
	}
	
	/**
	 * Create new table
	 */
	private void createNewChannel()
	{
	   final NotificationChannelDialog dlg = new NotificationChannelDialog(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;
		
      selectedChannel = dlg.getNotificaiotnChannel();
      
		new ConsoleJob("Create notification channel", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.createNotificationChannel(selectedChannel);
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Error creating notifiction channel";
			}
		}.start();
	}
	
	/**
	 * Edit selected table
	 */
	private void editChannel()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() != 1))
			return;
		
		selectedChannel = (NotificationChannel)selection.getFirstElement();
      final NotificationChannelDialog dlg = new NotificationChannelDialog(getSite().getShell(), selectedChannel);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Update notification channel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.updateNotificationChannel(selectedChannel);
            if(dlg.isNameChanged())
            {
               session.renameNotificaiotnChannel(selectedChannel.getName(), dlg.getNewName());
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Error updating notifiction channel";
         }
      }.start();
	}
	
	/**
	 * Delete selected tables
	 */
	private void deleteChannel()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() == 0))
			return;

		if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete notification channels", "Are you sure you want to delete notification channels?"))
			return;
		
		final List<String> channels = new ArrayList<String>(selection.size());
		for(Object o : selection.toList())
		{
			if (o instanceof NotificationChannel)
				channels.add(((NotificationChannel)o).getName());
		}
		
      new ConsoleJob("Delete notification channel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(String name : channels)
               session.deleteNotificationChannel(name);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Error deleting notifiction channel";
         }
      }.start();
	}
}
