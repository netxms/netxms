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
import org.netxms.client.objects.interfaces.Asset;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.EditAssetAttributeInstanceDialog;
import org.netxms.nxmc.modules.objects.views.helpers.AssetAttributeInstanceComparator;
import org.netxms.nxmc.modules.objects.views.helpers.AssetAttributeInstanceFilter;
import org.netxms.nxmc.modules.objects.views.helpers.AssetAttributeInstanceLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute instances
 */
public class AssetView extends ObjectView
{
   static final I18n i18n = LocalizationHelper.getI18n(AssetView.class);

   public static final int NAME = 0;
   public static final int VALUE = 1;
   public static final int IS_MANDATORY = 2;
   public static final int IS_UNIQUE = 3;
   public static final int SYSTEM_TYPE = 4;

   private SortableTableViewer viewer;
   private AssetAttributeInstanceFilter filter;
   private Map<String, String> attributes;
   private AssetAttributeInstanceLabelProvider labelProvider;
   private MenuManager attributeSelectionMenu;
   private Action actionAdd;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Constructor
    */
   public AssetView()
   {
      super(i18n.tr("Asset"), ResourceManager.getImageDescriptor("icons/object-views/components.png"), "Asset", true);
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
      labelProvider = new AssetAttributeInstanceLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AssetAttributeInstanceComparator(labelProvider));
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      filter = new AssetAttributeInstanceFilter(labelProvider);
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "Asset");
      viewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "Asset");
         }
      });

      createActions();
      createContextMenu();

      refresh();
   }   

   /**
    * Check if all mandatory attributes are set and notify if something is missing
    */
   private void checkMandatoryAttributes()
   {
      if (getObject() == null)
         return;

      StringBuilder missingEntries = new StringBuilder();
      for (AssetManagementAttribute definition : session.getAssetManagementSchema().values())
      {
         if (definition.isMandatory() && !attributes.containsKey(definition.getName()))
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
         addMessage(MessageArea.WARNING, i18n.tr("The following mandatory attributes are not set: {0}", missingEntries.toString()));
      }
   }

   /**
    * Create actions
    */
   void createActions()
   {    
      attributeSelectionMenu = new MenuManager(i18n.tr("&Add"));
      attributeSelectionMenu.setImageDescriptor(SharedIcons.ADD_OBJECT);
      attributeSelectionMenu.setRemoveAllWhenShown(true);
      attributeSelectionMenu.addMenuListener(new IMenuListener() {         
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            for(final AssetManagementAttribute definition : session.getAssetManagementSchema().values())
            {
               if (!attributes.containsKey(definition.getName()))
               {
                  attributeSelectionMenu.add(new Action(definition.getDisplayName().isBlank() ? definition.getName() : definition.getDisplayName()) {
                     @Override
                     public void run()
                     {
                        addAttribute(definition.getName());
                     }
                  });
               }
            }
         }
      });

      actionAdd = new Action(i18n.tr("&Add"), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
         }
      };

      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            updateAttribute();
         }         
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteAttribute();
         }         
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (getObject() != null)
         attributes = ((Asset)getObject()).getAssetInformation();
      else
         attributes = new HashMap<String, String>();

      viewer.setInput(attributes.entrySet().toArray());  
      checkMandatoryAttributes();
   }

   /**
    * Add attribute.
    *
    * @param name attribute name
    */
   private void addAttribute(String name)
   {      
      EditAssetAttributeInstanceDialog dlg = new EditAssetAttributeInstanceDialog(getWindow().getShell(), name, null);
      if (dlg.open() != Window.OK)
         return;

      final String value = dlg.getValue(); 
      new Job(i18n.tr("Adding asset attribute instance"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAssetAttribute(getObjectId(), name, value);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (attributes.size() == session.getAssetManagementSchemaSize())
                  {
                     actionAdd.setEnabled(false);
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add asset attribute instance");
         }
      }.start();
   }

   /**
    * Update attribute instance
    */
   @SuppressWarnings("unchecked")
   private void updateAttribute()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, String> element = (Entry<String, String>)selection.getFirstElement();
      String name = element.getKey();

      EditAssetAttributeInstanceDialog dlg = new EditAssetAttributeInstanceDialog(getWindow().getShell(), name, element.getValue());
      if (dlg.open() != Window.OK)
         return;

      final String value = dlg.getValue();
      new Job(i18n.tr("Updating asset management information"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAssetAttribute(getObjectId(), name, value);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update asset attribute instance");
         }
      }.start();
   }

   /**
    * Delete attribute instance
    */
   @SuppressWarnings("unchecked")
   private void deleteAttribute()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Attribute"), i18n.tr("Selected attributes will be deleted. Are you sure?")))
         return;

      List<String> attributes = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
         attributes.add(((Entry<String, String>)o).getKey());

      new Job(i18n.tr("Deleting asset management attribute instance"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String a : attributes)
               session.deleteAssetAttribute(getObjectId(), a);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete asset management attribute instance");
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
      manager.add(actionAdd);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      attributeSelectionMenu.update(true);
      manager.add(attributeSelectionMenu);
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
      if (attributes.size() < session.getAssetManagementSchemaSize())
      {
         attributeSelectionMenu.update(true);
         mgr.add(attributeSelectionMenu);
      }
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof Asset));
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
