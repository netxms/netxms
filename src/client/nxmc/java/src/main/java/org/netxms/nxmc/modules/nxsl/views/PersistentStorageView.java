/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.nxsl.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.dialogs.KeyValuePairEditDialog;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.helpers.KeyValuePairFilter;
import org.netxms.nxmc.base.widgets.helpers.KeyValuePairLabelProvider;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.xnap.commons.i18n.I18n;

/**
 * Persistent storage view
 */
public class PersistentStorageView extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(PersistentStorageView.class);

   private NXCSession session;
   private SortableTableViewer viewer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Map<String, String> pStorageSet = new HashMap<String, String>();

   /**
    * Create script library view
    */
   public PersistentStorageView()
   {
      super(LocalizationHelper.getI18n(PersistentStorageView.class).tr("Persistent Storage"), ResourceManager.getImageDescriptor("icons/config-views/persistent-storage.png"),
            "configuration.persistent-storage", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   { 
      final String[] setColumnNames = { i18n.tr("Key"), i18n.tr("Value") };
      final int[] setColumnWidths = { 200, 400 };
      viewer = new SortableTableViewer(parent, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new KeyValuePairLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = ((IStructuredSelection)viewer.getSelection()).size();
            actionEdit.setEnabled(size == 1);
            actionDelete.setEnabled(size > 0);
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editValue();
         }
      });
      KeyValuePairFilter filter = new KeyValuePairFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createActions();
      createPopupMenu();
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
    * Create and populate pop-up menu
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

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr menu manager
    */
   private void fillContextMenu(IMenuManager mgr)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1) 
      {         
         mgr.add(actionEdit);
      }

      if (selection.size() > 0)
         mgr.add(actionDelete);

      mgr.add(new Separator());
      mgr.add(actionCreate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createValue();
         }
      };
      addKeyBinding("M1+N", actionCreate);

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editValue();
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteValue();
         }
      };
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading persistent storage entries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            pStorageSet = session.getPersistentStorageList();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(pStorageSet.entrySet());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load persistent storage entries");
         }
      }.start();
   }

   /**
    * Create 
    */
   private void createValue()
   {
      final KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getWindow().getShell(), null, null, true, true, i18n.tr("Key"), i18n.tr("Value"));
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Creating persistent storage entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getKey(), dlg.getValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                  
                  pStorageSet.put(dlg.getKey(), dlg.getValue());
                  viewer.refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create persistent storage entry");
         }
      }.start();   
   }

   /**
    * Delete value
    */
   @SuppressWarnings("unchecked")
   private void deleteValue()
   {      
      IStructuredSelection selection = viewer.getStructuredSelection();

      final List<String> keys = new ArrayList<>();
      for(Object o : selection.toList())
         keys.add(((Entry<String, String>)o).getKey());

      new Job(i18n.tr("Deleting persistent storage entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String key : keys)
            {
               session.deletePersistentStorageValue(key);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     pStorageSet.remove(key);
                     viewer.refresh();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete persistent storage entry");
         }
      }.start();        
   }   

   /**
    * Edit value
    */
   @SuppressWarnings("unchecked")
   private void editValue()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
      final KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getWindow().getShell(), attr.getKey(), attr.getValue(), true, false, i18n.tr("Key"), i18n.tr("Value"));
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating persistent storage entry"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getKey(), dlg.getValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {            
                  pStorageSet.put(dlg.getKey(), dlg.getValue());
                  viewer.refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update persistent storage entry");
         }
      }.start();      
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();
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
