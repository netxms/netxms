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
package org.netxms.ui.eclipse.objectview.objecttabs;

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
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.widgets.ChassisWidget;
import org.netxms.ui.eclipse.objectview.widgets.helpers.ElementSelectionListener;

/**
 * "Chassis" tab
 */
public class ChassisTab extends ObjectTab implements ISelectionProvider
{
   private ScrolledComposite scroller;
   private Composite content;
   private ChassisWidget chassisFrontWidget;
   private ChassisWidget chassisRearWidget;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   
	/* (non-Javadoc)
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
            if ((chassisFrontWidget == null) || (chassisRearWidget == null) || (wHint == SWT.DEFAULT))
               return super.computeSize(wHint, hHint, changed);
            
            Point s = chassisFrontWidget.computeSize(wHint, hHint, changed);
            return new Point(s.x , s.y * 2);
         }
	   };
	   content.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
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
      scroller.getHorizontalBar().setIncrement(20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(content.computeSize(scroller.getSize().x, scroller.getSize().y));
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
      getViewPart().getSite().registerContextMenu(menuMgr, this);
   }
   
   /**
    * Fill port context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      ObjectContextMenu.fill(manager, getViewPart().getSite(), this);
   }

   /* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		objectChanged(object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
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
         ElementSelectionListener listener = new ElementSelectionListener() {
            @Override
            public void objectSelected(Object object)
            {
               selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
               for(ISelectionChangedListener listener : selectionListeners)
                  listener.selectionChanged(new SelectionChangedEvent(ChassisTab.this, selection));
            }
         };
         
         chassisFrontWidget = new ChassisWidget(content, SWT.NONE, (Chassis)object, RackOrientation.FRONT, true);
         chassisFrontWidget.addSelectionListener(listener);

         chassisRearWidget = new ChassisWidget(content, SWT.NONE, (Chassis)object, RackOrientation.REAR, true);
         chassisRearWidget.addSelectionListener(listener);
         
         scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
         updateChassisWidgetsSize();
         createPopupMenu();
	   }
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof Chassis);
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

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      scroller.setContent(content);
   }
}
