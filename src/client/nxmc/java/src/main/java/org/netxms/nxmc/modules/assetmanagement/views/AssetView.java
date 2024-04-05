/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.assetmanagement.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ActionContributionItem;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.dialogs.EditAssetPropertyDialog;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetPropertyComparator;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetPropertyFilter;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetPropertyListLabelProvider;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetPropertyReader;
import org.netxms.nxmc.modules.objects.views.ObjectView;
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
   private final I18n i18n = LocalizationHelper.getI18n(AssetView.class);

   public static final int NAME = 0;
   public static final int VALUE = 1;
   public static final int IS_MANDATORY = 2;
   public static final int IS_UNIQUE = 3;
   public static final int SYSTEM_TYPE = 4;

   private SortableTableViewer viewer;
   private AssetPropertyFilter filter;
   private Map<String, String> properties;
   private AssetPropertyReader propertyReader;
   private MenuManager attributeSelectionSubMenu;
   private MenuManager attributeSelectionPopupMenu;
   private Action actionAdd;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Constructor
    */
   public AssetView()
   {
      super(LocalizationHelper.getI18n(AssetView.class).tr("Asset"), ResourceManager.getImageDescriptor("icons/object-views/asset.png"), "objects.asset", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof Asset) || (((AbstractObject)context).getAssetId() != 0));
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
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      propertyReader = new AssetPropertyReader();

      final int[] widths = { 200, 400, 100, 100, 200 };
      final String[] names = { i18n.tr("Property"), i18n.tr("Value"), i18n.tr("Mandatory"), i18n.tr("Unique"), i18n.tr("System type") };
      viewer = new SortableTableViewer(parent, names, widths, NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AssetPropertyListLabelProvider(propertyReader));
      viewer.setComparator(new AssetPropertyComparator(propertyReader));
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            updateAttribute();
         }
      });

      filter = new AssetPropertyFilter(propertyReader);
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
      for(AssetAttribute definition : session.getAssetManagementSchema().values())
      {
         if (definition.isMandatory() && !properties.containsKey(definition.getName()))
         {
            if (missingEntries.length() != 0)
            {
               missingEntries.append(", ");
            }
            missingEntries.append(propertyReader.getDisplayName(definition.getName()));
         }
      }

      clearMessages();
      if (missingEntries.length() != 0)
      {
         addMessage(MessageArea.WARNING, i18n.tr("The following mandatory properties are not set: {0}", missingEntries.toString()));
      }
   }

   /**
    * Create actions
    */
   void createActions()
   {    
      final IMenuListener menuListener = new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            List<AssetAttribute> attributes = new ArrayList<>(session.getAssetManagementSchema().values());
            attributes.sort((a1, a2) -> a1.getEffectiveDisplayName().compareToIgnoreCase(a2.getEffectiveDisplayName()));
            for(final AssetAttribute attribute : attributes)
            {
               if (!properties.containsKey(attribute.getName()))
               {
                  manager.add(new Action(attribute.getEffectiveDisplayName()) {
                     @Override
                     public void run()
                     {
                        addProperty(attribute.getName());
                     }
                  });
               }
            }
         }
      };

      attributeSelectionSubMenu = new MenuManager(i18n.tr("&Add"));
      attributeSelectionSubMenu.setImageDescriptor(SharedIcons.ADD_OBJECT);
      attributeSelectionSubMenu.setRemoveAllWhenShown(true);
      attributeSelectionSubMenu.addMenuListener(menuListener);

      attributeSelectionPopupMenu = new MenuManager(i18n.tr("Add"));
      attributeSelectionPopupMenu.setRemoveAllWhenShown(true);
      attributeSelectionPopupMenu.addMenuListener(menuListener);

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
      Asset asset = null;
      AbstractObject object = getObject();
      if (object != null)
      {
         if (object instanceof Asset)
         {
            asset = (Asset)object;
         }
         else
         {
            long assetId = object.getAssetId();
            if (assetId != 0)
               asset = session.findObjectById(assetId, Asset.class);
         }
      }
      properties = (asset != null) ? new HashMap<String, String>(asset.getProperties()) : new HashMap<String, String>(0);
      viewer.setInput(properties.entrySet().toArray());
      checkMandatoryAttributes();

      if (actionAdd != null)
      {
         actionAdd.setEnabled(properties.size() < session.getAssetManagementSchemaSize());
      }
   }

   /**
    * Add property.
    *
    * @param name attribute name
    */
   private void addProperty(String name)
   {      
      EditAssetPropertyDialog dlg = new EditAssetPropertyDialog(getWindow().getShell(), name, null);
      if (dlg.open() != Window.OK)
         return;

      final String value = dlg.getValue(); 
      new Job(i18n.tr("Adding asset property"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setAssetProperty(getAssetId(), name, value);
            runInUIThread(() -> {
               if ((actionAdd != null) && (properties.size() == session.getAssetManagementSchemaSize()))
               {
                  actionAdd.setEnabled(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add asset property");
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

      EditAssetPropertyDialog dlg = new EditAssetPropertyDialog(getWindow().getShell(), name, element.getValue());
      if (dlg.open() != Window.OK)
         return;

      final String value = dlg.getValue();
      new Job(i18n.tr("Updating asset property"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setAssetProperty(getAssetId(), name, value);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update asset property");
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

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Property"), i18n.tr("Selected properties will be deleted. Are you sure?")))
         return;

      List<String> attributes = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
         attributes.add(((Entry<String, String>)o).getKey());

      new Job(i18n.tr("Deleting asset property"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String a : attributes)
               session.deleteAssetProperty(getAssetId(), a);
            runInUIThread(() -> actionAdd.setEnabled(properties.size() < session.getAssetManagementSchemaSize()));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete asset property");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      super.onObjectUpdate(object);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      attributeSelectionPopupMenu.update(true);
      actionAdd = new Action(i18n.tr("Add"), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            attributeSelectionPopupMenu.update(true);
            for(ToolItem i : ((ToolBarManager)manager).getControl().getItems())
            {
               if (((ActionContributionItem)i.getData()).getId().equals(getId()))
               {
                  Rectangle rect = i.getBounds();
                  Point pt = i.getParent().toDisplay(new Point(rect.x, rect.y));
                  Menu menu = attributeSelectionPopupMenu.createContextMenu(i.getParent());
                  menu.setLocation(pt.x, pt.y + rect.height);
                  menu.setVisible(true);
                  break;
               }
            }
         }
      };
      actionAdd.setId("nxmc.add_asset_property");
      manager.add(actionAdd);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      attributeSelectionSubMenu.update(true);
      manager.add(attributeSelectionSubMenu);
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
      if (properties.size() < session.getAssetManagementSchemaSize())
      {
         attributeSelectionSubMenu.update(true);
         mgr.add(attributeSelectionSubMenu);
      }
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isRelatedObject(long)
    */
   @Override
   protected boolean isRelatedObject(long objectId)
   {
      return objectId == getAssetId();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      propertyReader.dispose();
      super.dispose();
   }

   /**
    * Get ID of asset object this view works with.
    *
    * @return ID of asset object this view works with
    */
   private long getAssetId()
   {
      AbstractObject object = getObject();
      if (object == null)
         return 0;
      if (object instanceof Asset)
         return object.getObjectId();
      return object.getAssetId();
   }
}
