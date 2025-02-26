/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views;

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
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.MaintenanceJournalEntry;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.MaintenanceJournalEditDialog;
import org.netxms.nxmc.modules.objects.views.helpers.MaintenanceJournalComparator;
import org.netxms.nxmc.modules.objects.views.helpers.MaintenanceJournalFilter;
import org.netxms.nxmc.modules.objects.views.helpers.MaintenanceJournalLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Maintenance journal view
 */
public class MaintenanceJournalView extends ObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(MaintenanceJournalView.class);

   private static final String TABLE_CONFIG_PREFIX = "MaintenanceJournalView";

   public static final int COL_ID = 0;
   public static final int COL_OBJECT = 1;
   public static final int COL_AUHTHOR = 2;
   public static final int COL_EDITOR = 3;
   public static final int COL_DESCRIPTION = 4;
   public static final int COL_CREATE_TIME = 5;
   public static final int COL_MODIFY_TIME = 6;

   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private MaintenanceJournalFilter filter;
   private Action actionAdd;
   private Action actionEdit;
   private int warningMessageId = 0;

   /**
    * Create maintenance journal view
    */
   public MaintenanceJournalView()
   {
      super(LocalizationHelper.getI18n(MaintenanceJournalView.class).tr("Maintenance journal"), ResourceManager.getImageDescriptor("icons/object-views/maintenance_journal.png"),
            "objects.maintenance-journal", true);
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 110;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 50, 100, 100, 100, 500, 300, 300 };
      final String[] names = { i18n.tr("ID"), i18n.tr("Object"), i18n.tr("Author"), i18n.tr("Last edited by"), i18n.tr("Description"), i18n.tr("Creation time"), i18n.tr("Modification time") };
      viewer = new SortableTableViewer(parent, names, widths, COL_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      MaintenanceJournalLabelProvider labelProvider = new MaintenanceJournalLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new MaintenanceJournalComparator(labelProvider));
      filter = new MaintenanceJournalFilter(labelProvider);
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      viewer.addDoubleClickListener(new IDoubleClickListener() {

         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {

         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
            }
         }
      });

      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });

      createActions();
      createPopupMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.MAINTENANCE_JOURNAL_UPDATED) && (n.getSubCode() == getObjectId() || getObject().isParentOf(n.getSubCode())))
            {
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAdd = new Action(i18n.tr("&New...")) {
         @Override
         public void run()
         {
            addEntry();
         }
      };
      actionAdd.setImageDescriptor(SharedIcons.ADD_OBJECT);

      actionEdit = new Action(i18n.tr("&Edit...")) {
         @Override
         public void run()
         {
            editEntry();
         }
      };
      actionEdit.setImageDescriptor(SharedIcons.EDIT);
   }

   /**
    * Create popup menu
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      mgr.add(actionAdd);
      mgr.add(actionEdit);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (object != null)
      {
         refresh();
      }
      else
      {
         viewer.setInput(new MaintenanceJournalEntry[0]);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long objectId = getObjectId();
      if (objectId == 0)
         return;

      new Job(i18n.tr("Load maintenance journal entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<MaintenanceJournalEntry> entries = session.getAllMaintenanceEntries(objectId, 1000);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(entries);
                  deleteMessage(warningMessageId);
                  if (entries.size() == 1000)
                  {
                     warningMessageId = addMessage(MessageArea.WARNING, i18n.tr("Too many entries found. Showing last 1000 ones."), true);
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error getting maintenance journal entries");
         }
      }.start();
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
    * Add new journal entry
    */
   private void addEntry()
   {
      MaintenanceJournalEditDialog dlg = new MaintenanceJournalEditDialog(getWindow().getShell(), session.getObjectName(getObjectId()), null);
      if (dlg.open() != Window.OK)
         return;

      final String description = dlg.getDescription();
      new Job(i18n.tr("Creating new maintenance journal entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createMaintenanceEntry(getObjectId(), description);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create maintenance journal entry");
         }
      }.start();
   }

   /**
    * Edit selected journal entry
    */
   private void editEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      MaintenanceJournalEntry me = (MaintenanceJournalEntry)selection.getFirstElement();
      MaintenanceJournalEditDialog dlg = new MaintenanceJournalEditDialog(getWindow().getShell(), session.getObjectName(me.getObjectId()), me.getDescription());
      if (dlg.open() != Window.OK)
         return;

      final String description = dlg.getDescription();
      new Job(i18n.tr("Updating maintenance journal entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.editMaintenanceEntry(getObjectId(), me.getId(), description);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update maintenance journal entry");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof DataCollectionTarget) || (context instanceof Collector) || (context instanceof Container) ||
            (context instanceof ServiceRoot));
   }
}
