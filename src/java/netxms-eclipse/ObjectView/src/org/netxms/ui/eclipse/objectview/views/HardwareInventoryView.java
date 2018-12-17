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
package org.netxms.ui.eclipse.objectview.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.actions.ViewerProvider;
import org.netxms.ui.eclipse.objectview.widgets.HardwareInventory;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Hardware inventory view
 */
public class HardwareInventoryView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objectview.views.HardwareInventoryView"; //$NON-NLS-1$
   
   private long rootObjectId;
   private HardwareInventory inventoryWidget;
   private Action actionRefresh;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      
      rootObjectId = Long.parseLong(site.getSecondaryId());
      setPartName("Hardware Inventory - " + ((NXCSession)ConsoleSharedData.getSession()).getObjectName(rootObjectId));
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      inventoryWidget = new HardwareInventory(parent, SWT.NONE, this, "HardwareInventoryView"); //$NON-NLS-1$
      inventoryWidget.setRootObjectId(rootObjectId);

      createActions();
      contributeToActionBars();
      createPopupMenu();
      
      inventoryWidget.refresh();
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            inventoryWidget.refresh();
         }
      };

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
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionExportAllToCsv);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportAllToCsv);
      manager.add(actionRefresh);
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
      manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      inventoryWidget.setFocus();
   }
}
