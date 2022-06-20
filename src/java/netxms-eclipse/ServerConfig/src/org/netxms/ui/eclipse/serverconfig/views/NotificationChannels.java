/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2022 Raden Solutions
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
import org.netxms.ui.eclipse.serverconfig.dialogs.NotificationChannelEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.NotificationChannelLabelProvider;
import org.netxms.ui.eclipse.serverconfig.views.helpers.NotificationChannelListComparator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Notification channels view
 */
public class NotificationChannels extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.NotificationChannels"; //$NON-NLS-1$

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_DRIVER = 2;
   public static final int COLUMN_TOTAL_MESSAGES = 3;
   public static final int COLUMN_FAILED_MESSAGES = 4;
   public static final int COLUMN_LAST_STATUS = 5;
   public static final int COLUMN_ERROR_MESSAGE = 6;

   private NXCSession session = ConsoleSharedData.getSession();
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionNewChannel;
   private Action actionEditChannel;
   private Action actionDeleteChannel;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      final int[] widths = { 160, 250, 100, 100, 100, 80, 400 };
      final String[] names = { "Name", "Description", "Driver", "Messages", "Failures", "Status", "Error message" };
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
            IStructuredSelection selection = viewer.getStructuredSelection();
            actionEditChannel.setEnabled(selection.size() == 1);
            actionDeleteChannel.setEnabled(selection.size() > 0);
         }
      });

		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      WidgetHelper.restoreTableViewerSettings(viewer, settings, "NotificationChannelList.V2"); //$NON-NLS-1$
      viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            WidgetHelper.saveTableViewerSettings(viewer, settings, "NotificationChannelList.V2"); //$NON-NLS-1$
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();

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

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
      if (listener != null)
			session.removeListener(listener);
		super.dispose();
	}

   /**
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

      actionNewChannel = new Action("&New...", SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createNewChannel();
			}
		};

      actionEditChannel = new Action("&Edit...", SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editChannel();
			}
		};
		actionEditChannel.setEnabled(false);

      actionDeleteChannel = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteChannels();
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
      new ConsoleJob("Reading list of notification channels", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<NotificationChannel> channels = session.getNotificationChannels();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(channels);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of notification channels";
         }
      }.start();
   }

   /**
    * Create new channel
    */
   private void createNewChannel()
   {
      final NotificationChannelEditDialog dlg = new NotificationChannelEditDialog(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final NotificationChannel channel = dlg.getChannel();
      new ConsoleJob("Create notification channel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.createNotificationChannel(channel);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot create notification channel";
         }
      }.start();
   }

   /**
    * Edit selected notification channel
    */
   private void editChannel()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final NotificationChannel channel = (NotificationChannel)selection.getFirstElement();
      final String oldChannelName = channel.getName();
      final NotificationChannelEditDialog dlg = new NotificationChannelEditDialog(getSite().getShell(), channel);
      if (dlg.open() != Window.OK)
         return;

      new ConsoleJob("Updating notification channel", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (!channel.getName().equals(oldChannelName))
            {
               session.renameNotificationChannel(oldChannelName, channel.getName());
            }
            session.updateNotificationChannel(channel);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update notification channel";
         }
      }.start();
   }

   /**
    * Delete selected channels
    */
   private void deleteChannels()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete Channels", "Are you sure you want to delete selected notification channels?"))
         return;

      final List<String> channels = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof NotificationChannel)
            channels.add(((NotificationChannel)o).getName());
      }

      new ConsoleJob("Deleting notification channel", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(String name : channels)
               session.deleteNotificationChannel(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete notification channel";
         }
      }.start();
   }
}
