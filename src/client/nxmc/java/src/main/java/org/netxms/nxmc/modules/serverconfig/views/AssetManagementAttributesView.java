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
package org.netxms.nxmc.modules.serverconfig.views;

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
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.propertypages.AssetAttributeAutoFillScript;
import org.netxms.nxmc.modules.serverconfig.propertypages.AssetAttributeEnums;
import org.netxms.nxmc.modules.serverconfig.propertypages.AssetAttributeGeneral;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AssetManagementAttributeComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AssetManagementAttributeFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AssetManagementAttributeLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute view
 */
public class AssetManagementAttributesView extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(AssetManagementAttributesView.class);
   
   public static final int NAME = 0;
   public static final int DISPLAY_NAME = 1;
   public static final int DATA_TYPE = 2;
   public static final int IS_MANDATORY = 3;
   public static final int IS_UNIQUE = 4;
   public static final int HAS_SCRIPT = 5;
   public static final int RANGE_MIN = 6;
   public static final int RANGE_MAX = 7;
   public static final int SYSTEM_TYPE = 8;
   
   Map<String, AssetManagementAttribute> attributes = null;
   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private AssetManagementAttributeFilter filter;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   

   /**
    * Create notification channels view
    */
   public AssetManagementAttributesView()
   {
      super(i18n.tr("Asset Management"), ResourceManager.getImageDescriptor("icons/config-views/scheduled-tasks.png"), "AssetManagement", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final int[] widths = { 200, 200, 100, 100, 100, 100, 100, 100, 100 };
      final String[] names = { i18n.tr("Name"), i18n.tr("Display Name"), i18n.tr("Data Type"), i18n.tr("Is mandatory"), 
            i18n.tr("Is Unique"), i18n.tr("Autofill"), i18n.tr("Range min"), i18n.tr("Range max"), i18n.tr("System type") };
      viewer = new SortableTableViewer(parent, names, widths, NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AssetManagementAttributeLabelProvider());
      viewer.setComparator(new AssetManagementAttributeComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      filter = new AssetManagementAttributeFilter();
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
               getDisplay().asyncExec(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     AssetManagementAttribute attr = (AssetManagementAttribute)n.getObject();
                     attributes.put(attr.getName(), attr);
                     viewer.refresh();
                  }
               });
            }
            if (n.getCode() == SessionNotification.AM_ATTRIBUTE_DELETED)
            {
               getDisplay().asyncExec(new Runnable() {                  
                  @Override
                  public void run()
                  {
                     String attrName = (String)n.getObject();
                     attributes.remove(attrName);
                     viewer.refresh();
                  }
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
      actionCreate = new Action(i18n.tr("Create"), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createAttribute();
         }         
      };
      
      actionEdit = new Action(i18n.tr("Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            updateAttribute();
         }         
      };
      
      actionDelete = new Action(i18n.tr("Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteAttribute();
         }         
      };
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
         
      new Job(i18n.tr("Delete attributes"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {
               session.deleteAssetManagementAttribute(((AssetManagementAttribute)o).getName());
            }            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Failed to delete attributes";
         }
      }.start();      
   }

   /**
    * Update new asset management attribute
    */
   protected void updateAttribute()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      AssetManagementAttribute attr = (AssetManagementAttribute)selection.getFirstElement();      
      if (!showAttributePropertyPage(attr, false))
         return;          
      
      new Job(i18n.tr("Create asset management attributes"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAssetManagementAttribute(attr);          
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to get asset management attributes");
         }
      }.start();
      
   }

   /**
    * Create new asset management attribute
    */
   protected void createAttribute()
   {
      final AssetManagementAttribute attr = new AssetManagementAttribute();
      if (!showAttributePropertyPage(attr, true))
         return;      
      
      new Job(i18n.tr("Create asset management attributes"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.createAssetManagementAttribute(attr);          
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to get asset management attributes");
         }
      }.start();
   }
   
   private boolean showAttributePropertyPage(AssetManagementAttribute attribute, boolean isNew)
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
            newShell.setText("Properties for attribute");
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
      manager.add(actionCreate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
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
      mgr.add(actionCreate);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Get asset management attributes"), this) {
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Map<String, AssetManagementAttribute> attr = session.getAssetManagementAttributes();
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  attributes = attr;
                  viewer.setInput(attributes.values());
               }
            });
            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Failed to get asset management attributes");
         }
      }.start();
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
