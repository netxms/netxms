/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views;

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
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiMemoryEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.dialogs.AiMemoryEntryEditDialog;
import org.netxms.nxmc.modules.ai.views.helpers.AiMemoryComparator;
import org.netxms.nxmc.modules.ai.views.helpers.AiMemoryFilter;
import org.netxms.nxmc.modules.ai.views.helpers.AiMemoryLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI memory management view: environment facts, per-user notes, and per-object notes stored by the AI assistant or by users
 */
public class AiMemoryManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AiMemoryManager.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_SCOPE = 1;
   public static final int COLUMN_TARGET = 2;
   public static final int COLUMN_TITLE = 3;
   public static final int COLUMN_CONTENT = 4;
   public static final int COLUMN_CREATED_BY = 5;
   public static final int COLUMN_SOURCE = 6;
   public static final int COLUMN_CREATED = 7;
   public static final int COLUMN_UPDATED = 8;
   public static final int COLUMN_LOCKED = 9;

   private SortableTableViewer viewer;
   private NXCSession session;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionLock;
   private Action actionUnlock;

   /**
    * Create AI memory manager view
    */
   public AiMemoryManager()
   {
      super(LocalizationHelper.getI18n(AiMemoryManager.class).tr("AI Memory"), ResourceManager.getImageDescriptor("icons/config-views/ai-memory.png"), "configuration.ai-memory", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("ID"), i18n.tr("Scope"), i18n.tr("User / Object"), i18n.tr("Title"), i18n.tr("Content"),
            i18n.tr("Created by"), i18n.tr("Source"), i18n.tr("Created"), i18n.tr("Updated"), i18n.tr("Locked") };
      final int[] columnWidths = { 60, 100, 160, 250, 500, 90, 180, 150, 150, 60 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_UPDATED, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI, "AiMemoryManager");
      viewer.setContentProvider(new ArrayContentProvider());
      AiMemoryLabelProvider labelProvider = new AiMemoryLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AiMemoryComparator(labelProvider));
      AiMemoryFilter filter = new AiMemoryFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener((event) -> {
         IStructuredSelection selection = event.getStructuredSelection();
         actionEdit.setEnabled(selection.size() == 1);
         actionDelete.setEnabled(!selection.isEmpty());
         actionLock.setEnabled(!selection.isEmpty());
         actionUnlock.setEnabled(!selection.isEmpty());
      });
      viewer.addDoubleClickListener((event) -> editEntry());
      createActions();
      createContextMenu();
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

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading AI memory entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AiMemoryEntry> entries = session.getAiMemoryEntries();
            runInUIThread(() -> viewer.setInput(entries.toArray()));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of AI memory entries");
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createEntry();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editEntry();
         }
      };
      actionEdit.setEnabled(false);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteEntries();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);

      actionLock = new Action(i18n.tr("&Lock")) {
         @Override
         public void run()
         {
            setEntriesLocked(true);
         }
      };
      actionLock.setEnabled(false);

      actionUnlock = new Action(i18n.tr("&Unlock")) {
         @Override
         public void run()
         {
            setEntriesLocked(false);
         }
      };
      actionUnlock.setEnabled(false);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    *
    * @param manager Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionEdit);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionLock);
      manager.add(actionUnlock);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
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
      manager.add(actionNew);
   }

   /**
    * Create new memory entry
    */
   private void createEntry()
   {
      AiMemoryEntryEditDialog dlg = new AiMemoryEntryEditDialog(getWindow().getShell(), null);
      if (dlg.open() == Window.OK)
         saveEntry(dlg.getEntry());
   }

   /**
    * Edit selected memory entry
    */
   private void editEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AiMemoryEntryEditDialog dlg = new AiMemoryEntryEditDialog(getWindow().getShell(), (AiMemoryEntry)selection.getFirstElement());
      if (dlg.open() == Window.OK)
         saveEntry(dlg.getEntry());
   }

   /**
    * Save memory entry on server.
    *
    * @param entry entry to save
    */
   private void saveEntry(final AiMemoryEntry entry)
   {
      new Job(i18n.tr("Saving AI memory entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyAiMemoryEntry(entry);
            runInUIThread(() -> refresh());
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save AI memory entry");
         }
      }.start();
   }

   /**
    * Delete selected memory entries
    */
   private void deleteEntries()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"), i18n.tr("Selected AI memory entries will be deleted. Are you sure?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Deleting AI memory entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
               session.deleteAiMemoryEntry(((AiMemoryEntry)o).getId());
            runInUIThread(() -> refresh());
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete AI memory entry");
         }
      }.start();
   }

   /**
    * Lock or unlock selected memory entries.
    *
    * @param locked true to lock
    */
   private void setEntriesLocked(final boolean locked)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      new Job(locked ? i18n.tr("Locking AI memory entries") : i18n.tr("Unlocking AI memory entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               AiMemoryEntry entry = (AiMemoryEntry)o;
               if (entry.isLocked() != locked)
               {
                  AiMemoryEntry updated = new AiMemoryEntry(entry);
                  updated.setLocked(locked);
                  session.modifyAiMemoryEntry(updated);
               }
            }
            runInUIThread(() -> refresh());
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change lock state of AI memory entry");
         }
      }.start();
   }
}
