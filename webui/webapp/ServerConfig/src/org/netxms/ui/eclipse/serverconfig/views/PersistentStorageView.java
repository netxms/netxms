/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.dialogs.KeyValuePairEditDialog;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.helpers.AttributeLabelProvider;

/**
 * Persistent storage view
 */
public class PersistentStorageView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.PersistentStorageView"; //$NON-NLS-1$

   private NXCSession session;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Map<String, String> pStorageSet = new HashMap<String, String>();

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   { 
      final String[] setColumnNames = { "Key", "Value" };
      final int[] setColumnWidths = { 200, 400 };
      viewer = new SortableTableViewer(parent, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AttributeLabelProvider());
      viewer.setComparator(new ObjectLabelComparator((ILabelProvider)viewer.getLabelProvider()));
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
      createActions();
      contributeToActionBars();
      createPopupMenu();
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

      // Register menu for extension.
      getSite().setSelectionProvider(viewer);
      getSite().registerContextMenu(menuMgr, viewer);      
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
    * Contribute to action bars
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
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionCreate = new Action("Create new...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createValue();
         }
      };
      actionCreate.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.new_task"); //$NON-NLS-1$
      handlerService.activateHandler(actionCreate.getActionDefinitionId(), new ActionHandler(actionCreate));
      
      actionEdit = new Action("Change value...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editValue();
         }
      };
      actionEdit.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.edit_task"); //$NON-NLS-1$
      handlerService.activateHandler(actionEdit.getActionDefinitionId(), new ActionHandler(actionEdit));

      actionDelete = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteValue();
         }
      };
   }

   /**
    * Refresh this window
    */
   private void refresh()
   {
      new ConsoleJob("Reloading persistent storage value list", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            pStorageSet = session.getPersistentStorageList();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(pStorageSet.entrySet().toArray());
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of persistent storage values";
         }
      }.start();
   }
   
   /**
    * Create 
    */
   private void createValue()
   {
      final KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getSite().getShell(), null, null, true, true);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob("Create persistent storage value", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getAtributeName(), dlg.getAttributeValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {                  
                  pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
                  viewer.setInput(pStorageSet.entrySet().toArray());   
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot create persistent storage value";
         }
      }.start();   
   }

   /**
    * Delete value
    */
   private void deleteValue()
   {      
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      new ConsoleJob("Delete persistent storage value", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {            
            Iterator<?> it = selection.iterator();
            if (it.hasNext())
            {
               while(it.hasNext())
               {
                  @SuppressWarnings("unchecked")
                  final Entry<String, String> e = (Entry<String, String>) it.next();
                  session.deletePersistentStorageValue(e.getKey());
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {                  
                        pStorageSet.remove(e.getKey());
                        viewer.setInput(pStorageSet.entrySet().toArray());   
                     }
                  });
               }
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete persistent storage value";
         }
      }.start();        
   }   

   /**
    * Edit value
    */
   private void editValue()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      @SuppressWarnings("unchecked")
      final Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
      final KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getSite().getShell(), attr.getKey(), attr.getValue(), true, false);
      if (dlg.open() != Window.OK)
         return;

      new ConsoleJob("Change persistent storage value", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.setPersistentStorageValue(dlg.getAtributeName(), dlg.getAttributeValue());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {            
                  pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
                  viewer.setInput(pStorageSet.entrySet().toArray());   
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot edit persistent storage value";
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
}
