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
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.windows.MainWindow;
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

   private Composite header;
   private SortableTableViewer viewer;
   private AssetPropertyFilter filter;
   private Map<String, String> properties;
   private List<Asset> assets = new ArrayList<>();  // All assets shown by this view: the asset itself, or primary + component assets of the object
   private Asset currentAsset;  // Asset whose properties are displayed
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
      if (context == null)
         return false;
      if (context instanceof Asset)
         return true;
      AbstractObject object = (AbstractObject)context;
      return (object.getAssetId() != 0) || !Registry.getSession().getComponentAssets(object.getObjectId()).isEmpty();
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

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      // Header shows where the asset is linked (for asset context) or lets user pick one of object's assets (for object context)
      header = new Composite(parent, SWT.NONE);
      GridLayout headerLayout = new GridLayout();
      headerLayout.numColumns = 4;
      headerLayout.marginWidth = 8;
      headerLayout.marginHeight = 0;
      headerLayout.marginTop = 4;
      headerLayout.marginBottom = 8;
      headerLayout.horizontalSpacing = 8;
      header.setLayout(headerLayout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      final int[] widths = { 200, 400, 100, 100, 200 };
      final String[] names = { i18n.tr("Property"), i18n.tr("Value"), i18n.tr("Mandatory"), i18n.tr("Unique"), i18n.tr("System type") };
      viewer = new SortableTableViewer(parent, names, widths, NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI, "Asset");
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
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
      long selectedAssetId = (currentAsset != null) ? currentAsset.getObjectId() : 0;
      assets.clear();
      AbstractObject object = getObject();
      if (object instanceof Asset)
      {
         assets.add((Asset)object);
      }
      else if (object != null)
      {
         Asset primaryAsset = (object.getAssetId() != 0) ? session.findObjectById(object.getAssetId(), Asset.class) : null;
         if (primaryAsset != null)
            assets.add(primaryAsset);
         List<Asset> componentAssets = session.getComponentAssets(object.getObjectId());
         componentAssets.sort((a1, a2) -> a1.getObjectName().compareToIgnoreCase(a2.getObjectName()));
         assets.addAll(componentAssets);
      }

      // Keep previously selected asset if it is still linked, otherwise fall back to the first one
      currentAsset = null;
      for(Asset a : assets)
      {
         if (a.getObjectId() == selectedAssetId)
         {
            currentAsset = a;
            break;
         }
      }
      if ((currentAsset == null) && !assets.isEmpty())
         currentAsset = assets.get(0);

      updateHeader();
      showAssetProperties();
   }

   /**
    * Rebuild header content for current context.
    */
   private void updateHeader()
   {
      for(Control c : header.getChildren())
         c.dispose();

      AbstractObject object = getObject();
      if (object instanceof Asset)
      {
         final Asset asset = (Asset)object;
         if (asset.getLinkedObjectId() != 0)
         {
            new Label(header, SWT.NONE).setText(i18n.tr("Linked to:"));
            Link link = new Link(header, SWT.NONE);
            link.setText("<a>" + session.getObjectName(asset.getLinkedObjectId()).replace("&", "&&") + "</a>");
            link.setToolTipText(i18n.tr("Go to object"));
            link.addListener(SWT.Selection, (e) -> MainWindow.switchToObject(asset.getLinkedObjectId(), 0));
            if (asset.isComponent())
            {
               new Label(header, SWT.NONE).setText(i18n.tr("Component:"));
               new Label(header, SWT.NONE).setText(asset.getComponentSerial());
            }
         }
         else
         {
            new Label(header, SWT.NONE).setText(i18n.tr("Not linked to any object"));
         }
      }
      else if (object != null)
      {
         new Label(header, SWT.NONE).setText(i18n.tr("Asset:"));
         final Combo selector = new Combo(header, SWT.READ_ONLY | SWT.DROP_DOWN);
         for(Asset a : assets)
         {
            selector.add(a.isComponent() ? i18n.tr("{0} (component {1})", a.getObjectName(), a.getComponentSerial()) : a.getObjectName());
         }
         selector.select(assets.indexOf(currentAsset));
         selector.setEnabled(assets.size() > 1);
         selector.addListener(SWT.Selection, (e) -> {
            int index = selector.getSelectionIndex();
            if ((index >= 0) && (index < assets.size()) && (assets.get(index) != currentAsset))
            {
               currentAsset = assets.get(index);
               showAssetProperties();
            }
         });
         GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.widthHint = 300;
         selector.setLayoutData(gd);
      }

      header.layout(true, true);
      header.getParent().layout(true, true);
   }

   /**
    * Show properties of currently selected asset.
    */
   private void showAssetProperties()
   {
      properties = (currentAsset != null) ? new HashMap<String, String>(currentAsset.getProperties()) : new HashMap<String, String>(0);
      viewer.setInput(properties.entrySet().toArray());
      checkMandatoryAttributes();

      if (actionAdd != null)
      {
         actionAdd.setEnabled((currentAsset != null) && (properties.size() < session.getAssetManagementSchemaSize()));
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
      for(Asset a : assets)
      {
         if (a.getObjectId() == objectId)
            return true;
      }
      // Asset newly linked to this object (as primary or component asset)
      Asset asset = session.findObjectById(objectId, Asset.class);
      return (asset != null) && (asset.getLinkedObjectId() == getObjectId());
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
      return (currentAsset != null) ? currentAsset.getObjectId() : 0;
   }
}
