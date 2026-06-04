/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.EventForwarder;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.EventForwarderPropertiesDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.EventForwarderFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.EventForwarderLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.EventForwarderListComparator;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event forwarders view
 */
public class EventForwarders extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(EventForwarders.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_DRIVER = 2;
   public static final int COLUMN_TOTAL_EVENTS = 3;
   public static final int COLUMN_FAILED_EVENTS = 4;
   public static final int COLUMN_DROPPED_EVENTS = 5;
   public static final int COLUMN_QUEUE_SIZE = 6;
   public static final int COLUMN_LAST_STATUS = 7;
   public static final int COLUMN_ERROR_MESSAGE = 8;

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionNewForwarder;
   private Action actionEditForwarder;
   private Action actionDeleteForwarder;

   /**
    * Create event forwarders view
    */
   public EventForwarders()
   {
      super(LocalizationHelper.getI18n(EventForwarders.class).tr("Event Forwarders"), ResourceManager.getImageDescriptor("icons/config-views/nchannels.png"), "config.event-forwarders",
            true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 160, 250, 100, 100, 100, 100, 100, 80, 400 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Driver"), i18n.tr("Events"), i18n.tr("Failures"), i18n.tr("Dropped"), i18n.tr("Queue"), i18n.tr("Status"), i18n.tr("Error message") };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "EventForwarderList");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new EventForwarderLabelProvider());
      viewer.setComparator(new EventForwarderListComparator());
      EventForwarderFilter filter = new EventForwarderFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.addDoubleClickListener((e) -> editForwarder());
      viewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = viewer.getStructuredSelection();
         actionEditForwarder.setEnabled(selection.size() == 1);
         actionDeleteForwarder.setEnabled(selection.size() > 0);
      });

      createActions();
      createContextMenu();

      final Display display = viewer.getControl().getDisplay();
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.EVENT_FORWARDER_CHANGED)
            {
               display.asyncExec(() -> refresh());
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
      actionNewForwarder = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewForwarder();
         }
      };
      addKeyBinding("M1+N", actionNewForwarder);

      actionEditForwarder = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editForwarder();
         }
      };
      actionEditForwarder.setEnabled(false);
      addKeyBinding("M1+E", actionEditForwarder);

      actionDeleteForwarder = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteForwarders();
         }
      };
      actionDeleteForwarder.setEnabled(false);
      addKeyBinding("M1+D", actionDeleteForwarder);
   }

   /**
    * Create pop-up menu
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));

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
      manager.add(actionNewForwarder);
      manager.add(actionEditForwarder);
      manager.add(actionDeleteForwarder);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewForwarder);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
      manager.add(new Separator());
      manager.add(actionNewForwarder);
   }

   /**
    * Refresh
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Reading list of event forwarders"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<EventForwarder> forwarders = session.getEventForwarders();
            runInUIThread(() -> viewer.setInput(forwarders));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of event forwarders");
         }
      }.start();
   }

   /**
    * Create new forwarder
    */
   private void createNewForwarder()
   {
      final EventForwarderPropertiesDialog dlg = new EventForwarderPropertiesDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final EventForwarder forwarder = dlg.getEventForwarder();

      new Job(i18n.tr("Creating event forwarder"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createEventForwarder(forwarder);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create event forwarder");
         }
      }.start();
   }

   /**
    * Edit selected event forwarder
    */
   private void editForwarder()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final EventForwarder forwarder = (EventForwarder)selection.getFirstElement();
      final String oldName = forwarder.getName();
      final EventForwarderPropertiesDialog dlg = new EventForwarderPropertiesDialog(getWindow().getShell(), forwarder);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating event forwarder"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (!forwarder.getName().equals(oldName))
            {
               session.renameEventForwarder(oldName, forwarder.getName());
            }
            session.updateEventForwarder(forwarder);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update event forwarder");
         }
      }.start();
   }

   /**
    * Delete selected forwarders
    */
   private void deleteForwarders()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Event Forwarders"), i18n.tr("Are you sure you want to delete selected event forwarders?")))
         return;

      final List<String> forwarders = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof EventForwarder)
            forwarders.add(((EventForwarder)o).getName());
      }

      new Job(i18n.tr("Deleting event forwarder"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String name : forwarders)
               session.deleteEventForwarder(name);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete event forwarder");
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
