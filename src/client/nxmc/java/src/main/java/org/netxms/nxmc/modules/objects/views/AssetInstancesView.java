/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.Assets;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.EditAssetInstanceDialog;
import org.netxms.nxmc.modules.objects.views.helpers.AssetInstanceComparator;
import org.netxms.nxmc.modules.objects.views.helpers.AssetInstanceFilter;
import org.netxms.nxmc.modules.objects.views.helpers.AssetInstanceLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute instances
 */
public class AssetInstancesView extends ObjectView
{
   static final I18n i18n = LocalizationHelper.getI18n(AssetInstancesView.class);

   public static final int NAME = 0;
   public static final int VALUE = 1;
   public static final int IS_MANDATORY = 2;
   public static final int IS_UNIQUE = 3;
   public static final int SYSTEM_TYPE = 4;
   
   private SortableTableViewer viewer;
   private AssetInstanceFilter filter;
   private Map<String, String> assets;
   private AssetInstanceLabelProvider labelProvider;
   private MenuManager createMenu;
   private Action actionEdit;
   private Action actionDelete;  
     
   
   /**
    * Constructor
    */
   public AssetInstancesView()
   {
      super(i18n.tr("Assets"), ResourceManager.getImageDescriptor("icons/object-views/components.png"), "Assets", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 200, 400, 100, 100, 200 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Value"), i18n.tr("Is mandatory"), 
            i18n.tr("Is Unique"), i18n.tr("System type") };
      viewer = new SortableTableViewer(parent, names, widths, NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      labelProvider = new AssetInstanceLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AssetInstanceComparator(labelProvider));
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      filter = new AssetInstanceFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "AssetInstance");

      viewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "AssetInstance");
         }
      });

      createActions();
      createContextMenu();

      refresh();
   }   

   /**
    * Check all required assets and notify if something is missing
    */
   private void notifyOnMissingAssets()
   {
      if (getObject() == null)
         return;
      
      StringBuilder missingEntries = new StringBuilder();
      for (AssetManagementAttribute definition : session.getAssetManagementAttributes().values())
      {
         if (definition.isMandatory() && !assets.containsKey(definition.getName()))
         {
            if (missingEntries.length() != 0)
            {
               missingEntries.append(", ");
            }
            missingEntries.append(labelProvider.getName(definition.getName()));
         }
      }
      
      clearMessages();
      if (missingEntries.length() != 0)
      {
         missingEntries.insert(0, i18n.tr("Missing entries: "));
         addMessage(MessageArea.WARNING, missingEntries.toString());
      }
   }

   /**
    * Create actions
    */
   void createActions()
   {    
      createMenu = new MenuManager(i18n.tr("&Create"));
      createMenu.setImageDescriptor(SharedIcons.ADD_OBJECT);
      createMenu.setRemoveAllWhenShown(true);
      createMenu.addMenuListener(new IMenuListener() {         
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            for (AssetManagementAttribute definition : session.getAssetManagementAttributes().values())
            {
               if (!assets.containsKey(definition.getName()))
               {
                  addMenuItem(createMenu, labelProvider.getName(definition.getName()), definition.getName());
               }
            }
         }
      });
      
      actionEdit = new Action(i18n.tr("Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            updateAssetInstance();
         }         
      };
      
      actionDelete = new Action(i18n.tr("Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteAssetInstance();
         }         
      };
   }
   
   /**
    * Add asset management attribute as menu item 
    * 
    * @param manager manager to add an item
    * @param actionName action name
    * @param attributeName attribute name
    */
   private void addMenuItem(MenuManager manager, String actionName, String attributeName)
   {
      manager.add(new Action(actionName) {
         @Override
         public void run()
         {
            createAssetInstance(attributeName);
         }
      });      
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (getObject() != null)
         assets = ((Assets)getObject()).getAssets();
      else
         assets = new HashMap<String, String>();
      
      viewer.setInput(assets.entrySet().toArray());  
      notifyOnMissingAssets();
   }

   /**
    * Create asset instance
    */
   protected void createAssetInstance(String name)
   {      
      EditAssetInstanceDialog dlg = new EditAssetInstanceDialog(getWindow().getShell(), name, labelProvider.getName(name), null);
      if (dlg.open() != Window.OK)
         return;
      
      final String value = dlg.getValue(); 
      
      new Job(i18n.tr("Create asset"), this) {      
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAsset(getObjectId(), name, value);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to create asset");
         }
      }.start();
   }

   /**
    * Update asset instance
    */
   @SuppressWarnings("unchecked")
   protected void updateAssetInstance()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      Entry<String, String> element = (Entry<String, String>)selection.getFirstElement();
      String name = element.getKey();
      
      EditAssetInstanceDialog dlg = new EditAssetInstanceDialog(getWindow().getShell(), name, labelProvider.getName(name), element.getValue());
      if (dlg.open() != Window.OK)
         return;
      
      final String value = dlg.getValue();
      
      new Job(i18n.tr("Update asset"), this) {       
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAsset(getObjectId(), name, value);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to update asset");
         }
      }.start();
   }

   /**
    * Delete asset instance
    */
   protected void deleteAssetInstance()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() < 1)
         return;
      
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete asset"), 
            i18n.tr("Selected assets will be deleted. Are you sure?")))
         return;
      
      new Job(i18n.tr("Delete asset"), this) {
         
         @SuppressWarnings("unchecked")
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for (Object o : selection)
               session.deleteAsset(getObjectId(), ((Entry<String, String>)o).getKey());
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to delete asset");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      refresh();
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(createMenu);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(createMenu);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);     
   }

   /**
    * Fill context menu 
    * 
    * @param mgr menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      createMenu.update(true);
      mgr.add(createMenu);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && 
            ((context instanceof Assets));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 70;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }   
}
