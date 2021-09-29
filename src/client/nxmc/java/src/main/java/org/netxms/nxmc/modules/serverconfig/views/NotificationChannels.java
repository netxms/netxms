/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
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
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.NotificationChannelDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.NotificationChannelListComparator;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * SNotification channels view
 */
public class NotificationChannels extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(NotificationChannels.class);
   private static final String ID = "NotificationChannels";

   // Columns
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_DRIVER = 2;
   public static final int COLUMN_LAST_STATUS = 3;
   public static final int COLUMN_ERROR_MESSAGE = 4;
   
   private NXCSession session;
   private SessionListener listener;
   private List<NotificationChannel> notificationChannelList;
   private SortableTableViewer viewer;
   private Action actionNewChannel;
   private Action actionEditChannel;
   private Action actionDeleteChannel;
   private NotificationChannel selectedChannel;  
   

   /**
    * Create notification channels view
    */
   public NotificationChannels()
   {
      super(i18n.tr("Notification channels"), ResourceManager.getImageDescriptor("icons/config-views/nchannels.png"), ID, true);
      session = Registry.getSession();
   }

   @Override
   protected void createContent(Composite parent)
   {      
      final int[] widths = { 80, 200, 80, 80, 400 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Driver"), i18n.tr("Status"), i18n.tr("Error message") };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, ID);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new NotificationChannelLabelProvider());
      viewer.setComparator(new NotificationChannelListComparator());
      NotificationChannelFilter filter = new NotificationChannelFilter();
      viewer.setFilters(filter);
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
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            actionEditChannel.setEnabled(selection.size() == 1);
            actionDeleteChannel.setEnabled(selection.size() > 0);
         }
      });
      
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ID);
         }
      });
      
      createActions();
      createPopupMenu();

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
      refresh();
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {      
      actionNewChannel = new Action(i18n.tr("Create new channel"), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewChannel();
         }
      };
      
      actionEditChannel = new Action(i18n.tr("Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editChannel();
         }
      };
      actionEditChannel.setEnabled(false);
      
      actionDeleteChannel = new Action(i18n.tr("Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteChannel();
         }
      };
      actionDeleteChannel.setEnabled(false);
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
   public void refresh()
   {
      new Job(i18n.tr("Get notification channels"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Error while getting notificaiton channels");
         }
      }.start();
   }
   
   /**
    * Create new channel
    */
   private void createNewChannel()
   {
      final NotificationChannelDialog dlg = new NotificationChannelDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;
      
      selectedChannel = dlg.getNotificaiotnChannel();
      
      new Job(i18n.tr("Create notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createNotificationChannel(selectedChannel);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error creating notifiction channel");
         }
      }.start();
   }
   
   /**
    * Edit selected notification channel
    */
   private void editChannel()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection == null) || (selection.size() != 1))
         return;
      
      selectedChannel = (NotificationChannel)selection.getFirstElement();
      final NotificationChannelDialog dlg = new NotificationChannelDialog(getWindow().getShell(), selectedChannel);
      if (dlg.open() != Window.OK)
         return;
      
      new Job(i18n.tr("Update notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateNotificationChannel(selectedChannel);
            if(dlg.isNameChanged())
            {
               session.renameNotificationChannel(selectedChannel.getName(), dlg.getNewName());
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error updating notifiction channel");
         }
      }.start();
   }
   
   /**
    * Delete selected notification channel
    */
   private void deleteChannel()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection == null) || (selection.size() == 0))
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete notification channels"), i18n.tr("Are you sure you want to delete notification channels?")))
         return;
      
      final List<String> channels = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof NotificationChannel)
            channels.add(((NotificationChannel)o).getName());
      }
      
      new Job(i18n.tr("Delete notification channel"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String name : channels)
               session.deleteNotificationChannel(name);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error deleting notifiction channel");
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
