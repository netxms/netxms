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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
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
import org.netxms.ui.eclipse.console.resources.ThemeEngine;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.objectview.widgets.helpers.ElementSelectionListener;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Rack" tab
 */
public class RackTab extends ObjectTab implements ISelectionProvider
{
   private ScrolledComposite scroller;
   private Composite content;
   private RackWidget rackFrontWidget;
   private RackWidget rackRearWidget;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   
   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected void createTabContent(Composite parent)
	{
	   scroller = new ScrolledComposite(parent, SWT.H_SCROLL);

	   content = new Composite(scroller, SWT.NONE) {
         @Override
         public Point computeSize(int wHint, int hHint, boolean changed)
         {
            if ((rackFrontWidget == null) || (hHint == SWT.DEFAULT))
               return super.computeSize(wHint, hHint, changed);

            if (rackRearWidget == null)
               return rackFrontWidget.computeSize(wHint, hHint, changed);

            Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
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
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(content.computeSize(SWT.DEFAULT, scroller.getSize().y));
         }
      });
	}

   /**
    * Update size and position of rack widgets 
    */
   protected void updateRackWidgetsSize()
   {
      int height = content.getSize().y;
      Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
      rackFrontWidget.setSize(size);
      if (rackRearWidget != null)
      {
         rackRearWidget.setSize(size);
         rackRearWidget.setLocation(size.x, 0);
      }
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

      rackFrontWidget.setMenu(menuMgr.createContextMenu(rackFrontWidget));
      
      if (rackRearWidget != null)
         rackRearWidget.setMenu(menuMgr.createContextMenu(rackRearWidget));

      // Register menu for extension.
      getViewPart().getSite().registerContextMenu(menuMgr, this);
   }

   /**
    * Fill port context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      if (selection != null && ((IStructuredSelection)selection).getFirstElement() instanceof AbstractObject)
         ObjectContextMenu.fill(manager, getViewPart().getSite(), this);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		objectChanged(object);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
	@Override
	public void refresh()
	{
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public void objectChanged(final AbstractObject object)
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

      if (object != null)
	   {
         ElementSelectionListener listener = new ElementSelectionListener() {
            @Override
            public void objectSelected(Object object)
            {
               selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
               for(ISelectionChangedListener listener : selectionListeners)
                  listener.selectionChanged(new SelectionChangedEvent(RackTab.this, selection));
            }
         };

         rackFrontWidget = new RackWidget(content, SWT.NONE, (Rack)object, RackOrientation.FRONT);
         rackFrontWidget.addSelectionListener(listener);

         if (!((Rack)object).isFrontSideOnly())
         {
            rackRearWidget = new RackWidget(content, SWT.NONE, (Rack)object, RackOrientation.REAR);
            rackRearWidget.addSelectionListener(listener);
         }

         scroller.setMinSize(content.computeSize(SWT.DEFAULT, scroller.getSize().y));
         updateRackWidgetsSize();
         createPopupMenu();
	   }
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof Rack);
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return selection;
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = selection;
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      scroller.setContent(content);
   }
}
