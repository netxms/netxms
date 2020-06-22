/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.Chassis;
import org.netxms.ui.eclipse.console.resources.ThemeEngine;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.widgets.ChassisWidget;
import org.netxms.ui.eclipse.objectview.widgets.helpers.ElementSelectionListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Chassis view
 */
public class ChassisView extends ViewPart implements ISelectionProvider
{
   public static final String ID = "org.netxms.ui.eclipse.objectview.views.ChassisView";
   
   private Composite chassisArea;
   private Chassis chassis;
   private ChassisWidget chassisFrontWidget;
   private ChassisWidget chassisRearWidget;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
      chassis = session.findObjectById(Long.parseLong((parts.length > 0) ? parts[0] : site.getSecondaryId()), Chassis.class);
      if (chassis == null)
         throw new PartInitException("Chassis object not found");
      setPartName(String.format("Chassis - %s", chassis.getObjectName()));
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      chassisArea = new Composite(parent, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            if ((chassisFrontWidget == null) || (chassisRearWidget == null) || (wHint == SWT.DEFAULT))
               return super.computeSize(wHint, hHint, changed);
            
            Point s = chassisFrontWidget.computeSize(wHint, hHint, changed);
            return new Point(s.x , s.y * 2);
         }
      };
      chassisFrontWidget = new ChassisWidget(chassisArea, SWT.NONE, chassis, RackOrientation.FRONT, true);
      chassisRearWidget = new ChassisWidget(chassisArea, SWT.NONE, chassis, RackOrientation.REAR, true);
      
      chassisArea.setBackground(ThemeEngine.getBackgroundColor("Rack"));
      chassisArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            if ((chassisFrontWidget == null) || (chassisRearWidget == null))
               return;
            
            int width = chassisArea.getSize().x;
            int height = chassisArea.getSize().y;
            Point size = chassisFrontWidget.computeSize(width, height, true);
            chassisFrontWidget.setSize(size);
            chassisRearWidget.setSize(size);
            chassisRearWidget.setLocation(0, size.y);
         }
      });

      ElementSelectionListener listener = new ElementSelectionListener() {
         @Override
         public void objectSelected(Object object)
         {
            selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
            for(ISelectionChangedListener listener : selectionListeners)
               listener.selectionChanged(new SelectionChangedEvent(ChassisView.this, selection));
         }
      };
      chassisFrontWidget.addSelectionListener(listener);
      chassisRearWidget.addSelectionListener(listener);
      
      getSite().setSelectionProvider(this);
      createPopupMenu();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
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

      // Create menu.
      Menu menu = menuMgr.createContextMenu(chassisFrontWidget);
      chassisFrontWidget.setMenu(menu);
      
      menu = menuMgr.createContextMenu(chassisRearWidget);
      chassisRearWidget.setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, this);
   }
   
   /**
    * Fill port context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      ObjectContextMenu.fill(manager, getSite(), this);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.add(listener);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return selection;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
   }
}
