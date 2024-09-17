/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.widgets.ChassisWidget;
import org.netxms.nxmc.modules.objects.widgets.helpers.ElementSelectionListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Chassis view
 */
public class ChassisView extends ObjectView implements ISelectionProvider
{
   private ScrolledComposite scroller;
   private Composite content;
   private ChassisWidget chassisFrontWidget;
   private ChassisWidget chassisRearWidget;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * Create new chassis view
    */
   public ChassisView()
   {
      super(LocalizationHelper.getI18n(ChassisView.class).tr("Chassis"), ResourceManager.getImageDescriptor("icons/object-views/chassis.png"), "objects.chassis", false);
   }

   /**
    * Create new chassis view with ID extended by given sub ID. Intended for use by subclasses implementing ad-hoc views.
    * 
    * @param subId extension for base ID
    */
   protected ChassisView(String subId)
   {
      super(LocalizationHelper.getI18n(ChassisView.class).tr("Chassis"), ResourceManager.getImageDescriptor("icons/object-views/chassis.png"), "objects.chassis." + subId, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Chassis);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 15;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      scroller = new ScrolledComposite(parent, SWT.H_SCROLL);

      content = new Composite(scroller, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            if ((chassisFrontWidget == null) || (chassisRearWidget == null) || (wHint == SWT.DEFAULT))
               return super.computeSize(wHint, hHint, changed);

            Point s = chassisFrontWidget.computeSize(wHint, hHint, changed);
            return new Point(s.x, s.y * 2);
         }
      };
      content.setBackground(ThemeEngine.getBackgroundColor("Rack"));
      content.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            if ((chassisFrontWidget == null) || (chassisRearWidget == null))
               return;
            updateChassisWidgetsSize();
         }
      });

      scroller.setContent(content);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(content.computeSize(SWT.DEFAULT, scroller.getSize().y));
         }
      });
   }

   /**
    * Update size and position of chassis widgets
    */
   protected void updateChassisWidgetsSize()
   {
      int width = content.getSize().x;
      int height = content.getSize().y;
      Point size = chassisFrontWidget.computeSize(width, height, true);
      chassisFrontWidget.setSize(size);
      chassisRearWidget.setSize(size);
      chassisRearWidget.setLocation(0, size.y);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null);

      Menu menu = menuMgr.createContextMenu(chassisFrontWidget);
      chassisFrontWidget.setMenu(menu);

      menu = menuMgr.createContextMenu(chassisRearWidget);
      chassisRearWidget.setMenu(menu);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      if (chassisFrontWidget != null)
      {
         chassisFrontWidget.dispose();
         chassisFrontWidget = null;
      }
      if (chassisRearWidget != null)
      {
         chassisRearWidget.dispose();
         chassisRearWidget = null;
      }

      if (object != null)
      {
         buildViewForChassis((Chassis)object);
      }
   }

   /**
    * Build view for given chassis.
    *
    * @param chassis chassis object to build view for
    */
   protected void buildViewForChassis(final Chassis chassis)
   {
      ElementSelectionListener listener = new ElementSelectionListener() {
         @Override
         public void objectSelected(Object object)
         {
            selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
            for(ISelectionChangedListener listener : selectionListeners)
               listener.selectionChanged(new SelectionChangedEvent(ChassisView.this, selection));
         }
      };

      chassisFrontWidget = new ChassisWidget(content, SWT.NONE, chassis, RackOrientation.FRONT, true);
      chassisFrontWidget.addSelectionListener(listener);

      chassisRearWidget = new ChassisWidget(content, SWT.NONE, chassis, RackOrientation.REAR, true);
      chassisRearWidget.addSelectionListener(listener);

      scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
      updateChassisWidgetsSize();
      createPopupMenu();
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return selection;
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
   }
}
