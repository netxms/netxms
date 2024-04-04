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

import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
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
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.propertypages.AssetAttributeAutoFillScript;
import org.netxms.nxmc.modules.assetmanagement.propertypages.AssetAttributeEnums;
import org.netxms.nxmc.modules.assetmanagement.propertypages.AssetAttributeGeneral;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetAttributeComparator;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetAttributeFilter;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetAttributeListLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management schema manager
 */
public class AssetManagementSchemaManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AssetManagementSchemaManager.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DISPLAY_NAME = 1;
   public static final int COLUMN_DATA_TYPE = 2;
   public static final int COLUMN_IS_MANDATORY = 3;
   public static final int COLUMN_IS_UNIQUE = 4;
   public static final int COLUMN_IS_HIDDEN = 5;
   public static final int COLUMN_HAS_SCRIPT = 6;
   public static final int COLUMN_RANGE_MIN = 7;
   public static final int COLUMN_RANGE_MAX = 8;
   public static final int COLUMN_SYSTEM_TYPE = 9;

   Map<String, AssetAttribute> schema = null;
   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private AssetAttributeFilter filter;
   private Action actionAdd;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Create notification channels view
    */
   public AssetManagementSchemaManager()
   {
      super(LocalizationHelper.getI18n(AssetManagementSchemaManager.class).tr("Asset Management Schema"), ResourceManager.getImageDescriptor("icons/config-views/scheduled-tasks.png"),
            "configuration.asset-management-schema", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 200, 200, 100, 100, 100, 100, 100, 100, 100, 100 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Display Name"), i18n.tr("Data Type"), i18n.tr("Mandatory"), 
            i18n.tr("Unique"), i18n.tr("Hidden"), i18n.tr("Autofill"), i18n.tr("Range min"), i18n.tr("Range max"), i18n.tr("System type") };
      viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AssetAttributeListLabelProvider());
      viewer.setComparator(new AssetAttributeComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editAttribute();
         }
      });

      filter = new AssetAttributeFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "AssetManagementAttributes");

      viewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "AssetManagementAttributes");
         }
      });

      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.AM_ATTRIBUTE_UPDATED)
            {
               getDisplay().asyncExec(() -> {
                  AssetAttribute attr = (AssetAttribute)n.getObject();
                  schema.put(attr.getName(), attr);
                  viewer.refresh();
               });
            }
            if (n.getCode() == SessionNotification.AM_ATTRIBUTE_DELETED)
            {
               getDisplay().asyncExec(() -> {
                  String attrName = (String)n.getObject();
                  schema.remove(attrName);
                  viewer.refresh();
               });
            }
         }
      };
      session.addListener(listener);

      createActions();
      createContextMenu();
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAdd = new Action(i18n.tr("&New attribute..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createAttribute();
         }         
      };
      addKeyBinding("M1+N", actionAdd);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editAttribute();
         }         
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteAttribute();
         }         
      };
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * Delete attribute
    */
   protected void deleteAttribute()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Attribute"), 
            i18n.tr("Selected attributes will be deleted. Are you sure?")))
         return;

      new Job(i18n.tr("Updating asset management schema"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {
               session.deleteAssetAttribute(((AssetAttribute)o).getName());
            }            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update asset management schema");
         }
      }.start();      
   }

   /**
    * Edit asset management attribute
    */
   protected void editAttribute()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      AssetAttribute attr = (AssetAttribute)selection.getFirstElement();
      if (!showAttributePropertyPage(attr, false))
         return;          

      new Job(i18n.tr("Updating asset management schema"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAssetAttribute(attr);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update asset management schema");
         }
      }.start();
   }

   /**
    * Create new asset management attribute
    */
   protected void createAttribute()
   {
      final AssetAttribute attr = new AssetAttribute();
      if (!showAttributePropertyPage(attr, true))
         return;

      new Job(i18n.tr("Updating asset management schema"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createAssetAttribute(attr);
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update asset management schema");
         }
      }.start();
   }

   /**
    * Show property page for given attribute.
    *
    * @param attribute attribute to edit
    * @param isNew true if creating new attribute
    * @return true if OK was pressed
    */
   private boolean showAttributePropertyPage(AssetAttribute attribute, boolean isNew)
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("general", new AssetAttributeGeneral(attribute, isNew)));
      pm.addToRoot(new PreferenceNode("script", new AssetAttributeAutoFillScript(attribute)));
      pm.addToRoot(new PreferenceNode("enum", new AssetAttributeEnums(attribute)));

      PreferenceDialog dlg = new PreferenceDialog(getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText("Asset Attribute Properties");
         }
      };
      dlg.setBlockOnOpen(true);

      return dlg.open() == Window.OK;
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
      manager.add(actionAdd);
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
      mgr.add(actionAdd);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      // getAssetManagementSchema() will return cached schema so background job is not needed
      schema = session.getAssetManagementSchema();
      viewer.setInput(schema.values());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if ((listener != null) && (session != null))
         session.removeListener(listener);
      super.dispose();
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
