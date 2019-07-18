/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.TreeViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalComponent;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.ComponentTreeContentProvider;
import org.netxms.nxmc.modules.objects.views.helpers.ComponentTreeLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Components" object view
 */
public class EntityMIBView extends ObjectView
{
   private static I18n i18n = LocalizationHelper.getI18n(EntityMIBView.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_CLASS = 1;
   public static final int COLUMN_DESCRIPTION = 2;
   public static final int COLUMN_MODEL = 3;
   public static final int COLUMN_FIRMWARE = 4;
   public static final int COLUMN_SERIAL = 5;
   public static final int COLUMN_VENDOR = 6;
   public static final int COLUMN_INTERFACE = 7;

   private TreeViewer viewer;
   private ComponentTreeLabelProvider labelProvider;
   private Action actionCopy;
   private Action actionCopyName;
   private Action actionCopyModel;
   private Action actionCopySerial;
   private Action actionCollapseAll;
   private Action actionExpandAll;

   /**
    * @param name
    * @param image
    */
   public EntityMIBView()
   {
      super(i18n.tr("Components"), ResourceManager.getImageDescriptor("icons/object-views/components.png"), "EntityMIBView");
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewer = new TreeViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
      addColumn(i18n.tr("Name"), 200);
      addColumn(i18n.tr("Class"), 100);
      addColumn(i18n.tr("Description"), 250);
      addColumn(i18n.tr("Model"), 150);
      addColumn(i18n.tr("Firmware"), 100);
      addColumn(i18n.tr("Serial Number"), 150);
      addColumn(i18n.tr("Vendor"), 150);
      addColumn(i18n.tr("Interface"), 150);
      labelProvider = new ComponentTreeLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ComponentTreeContentProvider());
      viewer.getTree().setHeaderVisible(true);
      viewer.getTree().setLinesVisible(true);
      WidgetHelper.restoreColumnSettings(viewer.getTree(), PreferenceStore.getInstance(), "ComponentTree");
      viewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTree(), PreferenceStore.getInstance(), "ComponentTree");
         }
      });
      createActions();
      createPopupMenu();
   }

   /**
    * @param name
    */
   private void addColumn(String name, int width)
   {
      TreeViewerColumn tc = new TreeViewerColumn(viewer, SWT.LEFT);
      tc.getColumn().setText(name);
      tc.getColumn().setWidth(width);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopy);
      manager.add(actionCopyName);
      manager.add(actionCopyModel);
      manager.add(actionCopySerial);
      manager.add(new Separator());
      manager.add(actionCollapseAll);
      manager.add(actionExpandAll);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCopy = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySelectionToClipboard(-1);
         }
      };

      actionCopyName = new Action(i18n.tr("Copy &name to clipboard")) {
         @Override
         public void run()
         {
            copySelectionToClipboard(COLUMN_NAME);
         }
      };

      actionCopyModel = new Action(i18n.tr("Copy &model to clipboard")) {
         @Override
         public void run()
         {
            copySelectionToClipboard(COLUMN_MODEL);
         }
      };

      actionCopySerial = new Action(i18n.tr("Copy &serial number to clipboard")) {
         @Override
         public void run()
         {
            copySelectionToClipboard(COLUMN_SERIAL);
         }
      };

      actionCollapseAll = new Action(i18n.tr("C&ollapse all"), SharedIcons.COLLAPSE_ALL) {
         @Override
         public void run()
         {
            viewer.collapseAll();
         }
      };

      actionExpandAll = new Action(i18n.tr("&Expand all"), SharedIcons.EXPAND_ALL) {
         @Override
         public void run()
         {
            viewer.expandAll();
         }
      };
   }

   /**
    * Copy selected lines to clipboard
    */
   private void copySelectionToClipboard(int column)
   {
      TreeItem[] selection = viewer.getTree().getSelection();
      if (selection.length > 0)
      {
         final String newLine = SystemUtils.IS_OS_WINDOWS ? "\r\n" : "\n";
         StringBuilder sb = new StringBuilder();
         for(int i = 0; i < selection.length; i++)
         {
            if (i > 0)
               sb.append(newLine);
            if (column == -1)
            {
               sb.append(selection[i].getText(0));
               for(int j = 1; j < viewer.getTree().getColumnCount(); j++)
               {
                  sb.append("\t");
                  sb.append(selection[i].getText(j));
               }
            }
            else
            {
               sb.append(selection[i].getText(column));
            }
         }
         WidgetHelper.copyToClipboard(sb.toString());
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      if (object == null)
         return;

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Get node components"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               final PhysicalComponent root = session.getNodePhysicalComponents(object.getObjectId());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getTree().isDisposed())
                        return;

                     if ((EntityMIBView.this.getObject() != null)
                           && (EntityMIBView.this.getObject().getObjectId() == object.getObjectId()))
                     {
                        labelProvider.setNode((AbstractNode)object);
                        viewer.setInput(new Object[] { root });
                        viewer.expandAll();
                     }
                  }
               });
            }
            catch(NXCException e)
            {
               if (e.getErrorCode() != RCC.NO_COMPONENT_DATA)
                  throw e;
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (viewer.getTree().isDisposed())
                        return;

                     if ((EntityMIBView.this.getObject() != null)
                           && (EntityMIBView.this.getObject().getObjectId() == object.getObjectId()))
                     {
                        labelProvider.setNode(null);
                        viewer.setInput(new Object[0]);
                     }
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get component information for node {0}", object.getObjectName());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof Node))
      {
         return (((Node)context).getCapabilities() & Node.NC_HAS_ENTITY_MIB) != 0;
      }
      return false;
   }
}
