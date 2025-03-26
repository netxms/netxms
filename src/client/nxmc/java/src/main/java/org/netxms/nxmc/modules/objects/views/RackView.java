/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.widgets.RackWidget;
import org.netxms.nxmc.modules.objects.widgets.helpers.ElementSelectionListener;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Rack view
 */
public class RackView extends ObjectView implements ISelectionProvider
{
   private ScrolledComposite scroller;
   private Composite content;
   private RackWidget rackFrontWidget;
   private RackWidget rackRearWidget;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public RackView()
   {
      super(LocalizationHelper.getI18n(RackView.class).tr("Rack"), ResourceManager.getImageDescriptor("icons/object-views/rack.gif"), "objects.rack", false);
   }

   /**
    * Create new rack view with ID extended by given sub ID. Intended for use by subclasses implementing ad-hoc views.
    * 
    * @param subId extension for base ID
    */
   protected RackView(String subId)
   {
      super(LocalizationHelper.getI18n(RackView.class).tr("Rack"), ResourceManager.getImageDescriptor("icons/object-views/rack.gif"), "objects.rack." + subId, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof Rack);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      scroller = new ScrolledComposite(parent, SWT.H_SCROLL | SWT.V_SCROLL);

      content = new Composite(scroller, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            if (rackFrontWidget == null)
               return super.computeSize(wHint, hHint, changed);

            if (rackRearWidget == null)
               return rackFrontWidget.computeSize(wHint, hHint, changed);

            Point s = rackFrontWidget.computeSize((wHint != SWT.DEFAULT) ? wHint / 2 : wHint, hHint, changed);
            return new Point(s.x * 2, s.y);
         }
      };
      content.setBackground(ThemeEngine.getBackgroundColor("Rack"));
      content.addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            if (rackFrontWidget == null)
               return;
            updateRackWidgetsSize();
         }
      });

      scroller.setContent(content);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            Point size = scroller.getSize();
            scroller.setMinSize(content.computeSize(size.x, size.y));
         }
      });
   }

   /**
    * Update size and position of rack widgets
    */
   protected void updateRackWidgetsSize()
   {
      Point scrollerSize = scroller.getSize();
      if (rackRearWidget != null)
      {
         Point size = rackFrontWidget.computeSize(scrollerSize.x / 2, scrollerSize.y, true);
         rackFrontWidget.setSize(size);
         rackRearWidget.setSize(size);
         rackRearWidget.setLocation(size.x, 0);
      }
      else
      {
         rackFrontWidget.setSize(rackFrontWidget.computeSize(scrollerSize.x, scrollerSize.y, true));
      }
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null);
      rackFrontWidget.setMenu(menuMgr.createContextMenu(rackFrontWidget));
      if (rackRearWidget != null)
      {
         rackRearWidget.setMenu(menuMgr.createContextMenu(rackRearWidget));
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      buildRackView((Rack)object);
   }

   /**
    * Build rack view.
    *
    * @param rack rack object
    */
   protected void buildRackView(Rack rack)
   {
      if (rackFrontWidget != null)
      {
         rackFrontWidget.dispose();
         rackFrontWidget = null;
      }
      if (rackRearWidget != null)
      {
         rackRearWidget.dispose();
         rackRearWidget = null;
      }

      if (rack != null)
      {
         ElementSelectionListener listener = (object) -> {
            selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
            for(ISelectionChangedListener l : selectionListeners)
               l.selectionChanged(new SelectionChangedEvent(RackView.this, selection));
         };

         rackFrontWidget = new RackWidget(content, SWT.NONE, rack, RackOrientation.FRONT, this);
         rackFrontWidget.addSelectionListener(listener);

         if (!rack.isFrontSideOnly())
         {
            rackRearWidget = new RackWidget(content, SWT.NONE, rack, RackOrientation.REAR, this);
            rackRearWidget.addSelectionListener(listener);
         }

         Point scrollerSize = scroller.getSize();
         scroller.setMinSize(content.computeSize(scrollerSize.x, scrollerSize.y));
         updateRackWidgetsSize();
         createPopupMenu();
      }
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
