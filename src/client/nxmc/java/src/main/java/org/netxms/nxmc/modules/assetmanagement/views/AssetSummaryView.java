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
import java.util.List;
import java.util.stream.Collectors;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.client.objects.AssetGroup;
import org.netxms.client.objects.AssetRoot;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetListComparator;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetListFilter;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetListLabelProvider;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetPropertyReader;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Asset summary view
 */
public class AssetSummaryView extends ObjectView
{
   static final I18n i18n = LocalizationHelper.getI18n(AssetSummaryView.class);

   private SortableTableViewer viewer;
   private AssetPropertyReader propertyReader;
   private Action actionHideEmptyColumns;
   private Action actionExportToCSV;
   private Action actionExportAllToCSV;

   /**
    * Constructor
    */
   public AssetSummaryView()
   {
      super(LocalizationHelper.getI18n(AssetSummaryView.class).tr("Assets"), ResourceManager.getImageDescriptor("icons/object-views/asset.png"), "objects.assets", true);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && ((context instanceof Container) || (context instanceof Collector) || (context instanceof Subnet) || (context instanceof Zone) || (context instanceof Rack) || (context instanceof Cluster) ||
            (context instanceof AssetGroup) || (context instanceof AssetRoot));
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

      viewer = new SortableTableViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);

      viewer.addColumn(i18n.tr("Asset"), SWT.DEFAULT);
      List<AssetAttribute> attributes = new ArrayList<>(session.getAssetManagementSchema().values());
      attributes.sort((a1, a2) -> a1.getEffectiveDisplayName().compareToIgnoreCase(a2.getEffectiveDisplayName()));
      for(AssetAttribute a : attributes)
      {
         if (!a.isHidden())
         {
            TableColumn tc = viewer.addColumn(a.getEffectiveDisplayName(), SWT.DEFAULT);
            tc.setData("Attribute", a.getName());
         }
      }

      viewer.getTable().setSortColumn(viewer.getTable().getColumn(0));
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AssetListLabelProvider(viewer, propertyReader));
      viewer.setComparator(new AssetListComparator(propertyReader));

      AssetListFilter filter = new AssetListFilter(propertyReader);
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createActions();
      createContextMenu();
   }

   /**
    * Create actions
    */
   void createActions()
   {
      actionExportAllToCSV = new ExportToCsvAction(this, viewer, false);
      actionExportToCSV = new ExportToCsvAction(this, viewer, true);

      actionHideEmptyColumns = new Action(i18n.tr("&Hide empty columns"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            boolean hideEmptyColumns = isChecked();
            if (hideEmptyColumns)
               hideEmptyColumns();
            else
               showAllColumns();
            viewer.packColumns();
            PreferenceStore.getInstance().set("AssetSummaryView.hideEmptyColumns", hideEmptyColumns);
         }
      };
      actionHideEmptyColumns.setChecked(PreferenceStore.getInstance().getAsBoolean("AssetSummaryView.hideEmptyColumns", true));
      addKeyBinding("M1+H", actionHideEmptyColumns);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      AbstractObject root = getObject();
      if (root == null)
      {
         viewer.setInput(new Object[0]);
         return;
      }

      List<AbstractObject> assets = root.getAllChildren(-1).stream().filter((object) -> (object instanceof Asset) || (object.getAssetId() != 0)).collect(Collectors.toList());
      viewer.setInput(assets);

      // Re-read "hide" setting in case it was changed by another view
      boolean hideEmptyColumns = PreferenceStore.getInstance().getAsBoolean("AssetSummaryView.hideEmptyColumns", true);
      actionHideEmptyColumns.setChecked(hideEmptyColumns);
      if (!assets.isEmpty() && hideEmptyColumns)
         hideEmptyColumns();
      else
         showAllColumns();

      viewer.packColumns();
   }

   /**
    * Hide columns with empty values for all assets
    */
   private void hideEmptyColumns()
   {
      TableColumn[] columns = viewer.getTable().getColumns();
      for(int i = 1; i < columns.length; i++)
      {
         boolean empty = true;
         String attr = (String)columns[i].getData("Attribute");
         for(Object o : (List<?>)viewer.getInput())
         {
            Asset asset = getAsset(o);
            if (asset != null)
            {
               String v = asset.getProperties().get(attr);
               if ((v != null) && !v.isEmpty())
               {
                  empty = false;
                  break;
               }
            }
         }
         columns[i].setWidth(empty ? 0 : 100);
         columns[i].setResizable(!empty);
      }
   }

   /**
    * Show all columns
    */
   private void showAllColumns()
   {
      TableColumn[] columns = viewer.getTable().getColumns();
      for(int i = 1; i < columns.length; i++)
         columns[i].setResizable(true);
   }

   /**
    * Get asset from element
    *
    * @param element list element
    * @return asset object
    */
   private Asset getAsset(Object element)
   {
      if (element instanceof Asset)
         return (Asset)element;
      return session.findObjectById(((AbstractObject)element).getAssetId(), Asset.class);
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
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      propertyReader.dispose();
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCSV);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionHideEmptyColumns);
      manager.add(new Separator());
      manager.add(actionExportAllToCSV);
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
      mgr.add(actionExportToCSV);
      mgr.add(actionExportAllToCSV);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isRelatedObject(long)
    */
   @Override
   protected boolean isRelatedObject(long objectId)
   {
      AbstractObject object = getObject();
      return (object != null) && object.isParentOf(objectId);
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
