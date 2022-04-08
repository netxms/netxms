/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.actions.ViewerProvider;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.HardwareInventory;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Hardware inventory view
 */
public class HardwareInventoryView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(HardwareInventoryView.class);
   public static final String ID = "org.netxms.ui.eclipse.objectview.views.HardwareInventoryView"; //$NON-NLS-1$
   
   private HardwareInventory inventoryWidget;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;

   /**
    * @param name
    * @param image
    * @param id
    */
   public HardwareInventoryView()
   {
      super(i18n.tr("Hardware Inventory"), ResourceManager.getImageDescriptor("icons/object-views/components.png"), "HardwareInventory", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      inventoryWidget = new HardwareInventory(parent, SWT.NONE, this, "HardwareInventoryView"); //$NON-NLS-1$
      createActions();
      createPopupMenu();
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
   }

   /**
    * Fill view's local toolbar
    *
    * @param manager toolbar manager
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
      manager.add(new Separator());
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
      manager.add(actionExportToCsv);
      manager.add(new Separator());
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

      inventoryWidget.setRootObjectId(object.getObjectId());
      inventoryWidget.refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof Node))
      {
         return (((Node)context).getCapabilities() & Node.NC_IS_NATIVE_AGENT) != 0;
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }
}
