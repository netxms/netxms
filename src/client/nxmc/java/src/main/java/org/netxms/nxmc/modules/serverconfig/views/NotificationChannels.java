/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2024 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.NotificationChannelPropertiesDialog;
import org.netxms.nxmc.modules.serverconfig.dialogs.SendNotificationDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelListComparator;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Notification channels view
 */
public class NotificationChannels extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(NotificationChannels.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_DRIVER = 2;
   public static final int COLUMN_TOTAL_MESSAGES = 3;
   public static final int COLUMN_FAILED_MESSAGES = 4;
   public static final int COLUMN_LAST_STATUS = 5;
   public static final int COLUMN_ERROR_MESSAGE = 6;

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionNewChannel;
   private Action actionEditChannel;
   private Action actionDeleteChannel;
   private Action actionSendNotificationGlobal;
   private Action actionSendNotificationContext;

   /**
    * Create notification channels view
    */
   public NotificationChannels()
   {
      super(LocalizationHelper.getI18n(NotificationChannels.class).tr("Notification Channels"), ResourceManager.getImageDescriptor("icons/config-views/nchannels.png"), "config.notification-channels",
            true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 160, 250, 100, 100, 100, 80, 400 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Driver"), i18n.tr("Messages"), i18n.tr("Failures"), i18n.tr("Status"), i18n.tr("Error message") };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, "NotificationChannelList");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new NotificationChannelLabelProvider());
      viewer.setComparator(new NotificationChannelListComparator());
      NotificationChannelFilter filter = new NotificationChannelFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
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

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "NotificationChannelList");
         }
      });

      createActions();
      createContextMenu();

      final Display display = viewer.getControl().getDisplay();
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
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(listener);
      super.dispose();
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
    * Create actions
    */
   private void createActions()
   {      
      actionNewChannel = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewChannel();
         }
      };
      addKeyBinding("M1+N", actionNewChannel);

      actionEditChannel = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editChannel();
         }
      };
      actionEditChannel.setEnabled(false);
      addKeyBinding("M1+E", actionEditChannel);

      actionDeleteChannel = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteChannels();
         }
      };
      actionDeleteChannel.setEnabled(false);
      addKeyBinding("M1+D", actionDeleteChannel);

      actionSendNotificationGlobal = new Action(i18n.tr("&Send notification..."), ResourceManager.getImageDescriptor("icons/send-notification.png")) {
         @Override
         public void run()
         {
            sendNotification(false);
         }
      };
      addKeyBinding("M1+M2+S", actionSendNotificationGlobal);

      actionSendNotificationContext = new Action(i18n.tr("&Send notification..."), ResourceManager.getImageDescriptor("icons/send-notification.png")) {
         @Override
         public void run()
         {
            sendNotification(true);
         }
      };
      addKeyBinding("M1+S", actionSendNotificationContext);
   }

   /**
    * Create pop-up menu for variable list
    */
   private void createContextMenu()
   {
      // Create menu manager.
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));

      // Create menu.
      Menu menu = manager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager Menu manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionNewChannel);
      manager.add(actionEditChannel);
      manager.add(actionDeleteChannel);
      manager.add(new Separator());
      manager.add(actionSendNotificationContext);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewChannel);
      manager.add(actionSendNotificationGlobal);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNewChannel);
      manager.add(actionSendNotificationGlobal);
   }

   /**
    * Refresh
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading list of notification channels"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<NotificationChannel> channels = session.getNotificationChannels();
            runInUIThread(() -> viewer.setInput(channels));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of notification channels");
         }
      }.start();
   }

   /**
    * Create new channel
    */
   private void createNewChannel()
   {
      final NotificationChannelPropertiesDialog dlg = new NotificationChannelPropertiesDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final NotificationChannel channel = dlg.getNotificaiotnChannel();

      new Job(i18n.tr("Create notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createNotificationChannel(channel);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create notification channel");
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
      final NotificationChannelPropertiesDialog dlg = new NotificationChannelPropertiesDialog(getWindow().getShell(), channel);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot update notification channel");
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

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Channels"), i18n.tr("Are you sure you want to delete selected notification channels?")))
         return;

      final List<String> channels = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof NotificationChannel)
            channels.add(((NotificationChannel)o).getName());
      }

      new Job(i18n.tr("Deleting notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String name : channels)
               session.deleteNotificationChannel(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete notification channel");
         }
      }.start();
   }
   
   /**
    * Send notification to channel
    * 
    * @param isContext true if called from context menu
    */
   private void sendNotification(boolean isContext)
   {
      String channelName = isContext ? ((NotificationChannel)viewer.getStructuredSelection().getFirstElement()).getName() : null;
      final SendNotificationDialog dlg = new SendNotificationDialog(getWindow().getShell(), channelName);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Sending notification to {0}", dlg.getRecipient()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.sendNotification(dlg.getChannelName(), dlg.getRecipient(), dlg.getSubject(), dlg.getMessage());
            runInUIThread(() -> addMessage(MessageArea.SUCCESS, i18n.tr("Notification to {0} via {1} has been enqueued", dlg.getRecipient(), dlg.getChannelName())));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot send notification to {0}", dlg.getRecipient());
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
