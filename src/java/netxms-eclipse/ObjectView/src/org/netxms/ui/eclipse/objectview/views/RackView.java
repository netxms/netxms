/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.objectview.widgets.helpers.RackSelectionListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Rack view
 */
public class RackView extends ViewPart implements ISelectionProvider
{
   public static final String ID = "org.netxms.ui.eclipse.objectview.views.RackView";
   
   private Composite rackArea;
   private Rack rack;
   private RackWidget rackFrontWidget;
   private RackWidget rackRearWidget;
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
      rack = session.findObjectById(Long.parseLong((parts.length > 0) ? parts[0] : site.getSecondaryId()), Rack.class);
      if (rack == null)
         throw new PartInitException("Rack object not found");
      setPartName(String.format("Rack - %s", rack.getObjectName()));
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      rackArea = new Composite(parent, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            if ((rackFrontWidget == null) || (rackRearWidget == null) || (hHint == SWT.DEFAULT))
               return super.computeSize(wHint, hHint, changed);
            
            Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
            return new Point(s.x * 2, s.y);
         }
      };
      rackFrontWidget = new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.FRONT);
      rackRearWidget = new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.REAR);
      
      rackArea.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
      rackArea.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            if ((rackFrontWidget == null) || (rackRearWidget == null))
               return;
            
            int height = rackArea.getSize().y;
            Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
            rackFrontWidget.setSize(size);
            rackRearWidget.setSize(size);
            rackRearWidget.setLocation(size.x, 0);
         }
      });

      RackSelectionListener listener = new RackSelectionListener() {
         @Override
         public void objectSelected(AbstractObject object)
         {
            selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
            for(ISelectionChangedListener listener : selectionListeners)
               listener.selectionChanged(new SelectionChangedEvent(RackView.this, selection));
         }
      };
      rackFrontWidget.addSelectionListener(listener);
      rackRearWidget.addSelectionListener(listener);
      
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
      Menu menu = menuMgr.createContextMenu(rackFrontWidget);
      rackFrontWidget.setMenu(menu);
      
      menu = menuMgr.createContextMenu(rackRearWidget);
      rackRearWidget.setMenu(menu);

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
