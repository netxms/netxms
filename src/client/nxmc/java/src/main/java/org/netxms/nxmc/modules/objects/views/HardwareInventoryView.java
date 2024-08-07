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
package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.actions.ViewerProvider;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.AbstractHardwareInventoryWidget;
import org.netxms.nxmc.modules.objects.widgets.EntityMibTreeViewer;
import org.netxms.nxmc.modules.objects.widgets.HardwareInventoryTable;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Hardware inventory view
 */
public class HardwareInventoryView extends ObjectView
{
   private static enum DisplayMode
   {
      AGENT, ENTITY_MIB
   };

   private final I18n i18n = LocalizationHelper.getI18n(HardwareInventoryView.class);

   private DisplayMode displayMode = DisplayMode.AGENT;
   private AbstractHardwareInventoryWidget inventoryWidget;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionCopy;
   private Action actionCopyName;
   private Action actionCopyDescription;
   private Action actionCopyModel;
   private Action actionCopySerial;
   private Action actionCollapseAll;
   private Action actionExpandAll;

   /**
    * @param name
    * @param image
    * @param id
    */
   public HardwareInventoryView()
   {
      super(LocalizationHelper.getI18n(HardwareInventoryView.class).tr("Hardware Inventory"), ResourceManager.getImageDescriptor("icons/object-views/hardware.png"), "objects.hardware-inventory",
            false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      HardwareInventoryView view = (HardwareInventoryView)super.cloneView();
      view.displayMode = displayMode;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Node) && (((Node)context).hasAgent() || ((Node)context).isEntityMibSupported());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      inventoryWidget = (displayMode == DisplayMode.AGENT) ? new HardwareInventoryTable(parent, SWT.NONE, this) : new EntityMibTreeViewer(parent, SWT.NONE, this);
      createActions();
      createContextMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      ViewerProvider vp = new ViewerProvider() {         
         @Override
         public ColumnViewer getViewer()
         {
            return inventoryWidget.getViewer();
         }
      };
      actionExportToCsv = new ExportToCsvAction(this, vp, true);
      actionExportAllToCsv = new ExportToCsvAction(this, vp, false);

      actionCopy = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(-1);
         }
      };

      actionCopyName = new Action(i18n.tr("Copy &name to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getNameColumnIndex());
         }
      };

      actionCopyDescription = new Action(i18n.tr("Copy &description to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getDescriptionColumnIndex());
         }
      };

      actionCopyModel = new Action(i18n.tr("Copy &model to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getModelColumnIndex());
         }
      };

      actionCopySerial = new Action(i18n.tr("Copy &serial number to clipboard")) {
         @Override
         public void run()
         {
            inventoryWidget.copySelectionToClipboard(inventoryWidget.getSerialColumnIndex());
         }
      };

      actionCollapseAll = new Action(i18n.tr("C&ollapse all"), SharedIcons.COLLAPSE_ALL) {
         @Override
         public void run()
         {
            ((TreeViewer)inventoryWidget.getViewer()).collapseAll();
         }
      };

      actionExpandAll = new Action(i18n.tr("&Expand all"), SharedIcons.EXPAND_ALL) {
         @Override
         public void run()
         {
            ((TreeViewer)inventoryWidget.getViewer()).expandAll();
         }
      };
   }

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
   }

   /**
    * Create pop-up menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });
      inventoryWidget.setViewerMenu(menuMgr);
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionCopy);
      if (inventoryWidget.getNameColumnIndex() != -1)
         manager.add(actionCopyName);
      manager.add(actionCopyDescription);
      manager.add(actionCopyModel);
      manager.add(actionCopySerial);
      manager.add(new Separator());
      manager.add(actionExportToCsv);
      manager.add(actionExportAllToCsv);
      if (inventoryWidget.isTreeViewer())
      {
         manager.add(new Separator());
         manager.add(actionCollapseAll);
         manager.add(actionExpandAll);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      inventoryWidget.refresh();      
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      inventoryWidget.clear();
      if (object == null)
         return;

      if ((displayMode == DisplayMode.AGENT) && !((Node)object).hasAgent())
      {
         inventoryWidget.dispose();
         inventoryWidget = new EntityMibTreeViewer(getClientArea(), SWT.NONE, this);
         displayMode = DisplayMode.ENTITY_MIB;
         createContextMenu();
      }
      else if ((displayMode == DisplayMode.ENTITY_MIB) && !((Node)object).isEntityMibSupported())
      {
         inventoryWidget.dispose();
         inventoryWidget = new HardwareInventoryTable(getClientArea(), SWT.NONE, this);
         displayMode = DisplayMode.AGENT;
         createContextMenu();
      }

      inventoryWidget.refresh();
   }
   
   /**
    * @see org.netxms.nxmc.base.views.Perspective#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {      
      super.saveState(memento);
      memento.set("displayMode", displayMode.toString());
   }
   
   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.Perspective#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      displayMode = DisplayMode.valueOf(memento.getAsString("displayMode"));
   }
}
