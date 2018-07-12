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
package org.netxms.ui.eclipse.dashboard.widgets;

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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.RackView;
import org.netxms.ui.eclipse.dashboard.widgets.internal.RackDiagramConfig;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.objectview.widgets.helpers.RackSelectionListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 *Rack diagram element for dashboard
 */
public class RackDiagramElement extends ElementWidget implements ISelectionProvider
{
   private RackWidget rackFrontWidget = null;
   private RackWidget rackRearWidget = null;
   private NXCSession session;
   private RackDiagramConfig config;
   private Composite rackArea;
   private ScrolledComposite scroller;
   private Label title;
   private Font font = null;
   private IViewPart viewPart;
   private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   
   /**
    * Create new rack diagram element
    * 
    * @param parent Dashboard control
    * @param element Dashboard element
    * @param viewPart viewPart
    */
   protected RackDiagramElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
   {
      super(parent, element, viewPart);
            
      this.viewPart = viewPart;
      
      try
      {
         config = RackDiagramConfig.createFromXml(element.getData());
      }
      catch (Exception e)
      {
         e.printStackTrace();
         config = new RackDiagramConfig();
      }
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);
      
      session = (NXCSession)ConsoleSharedData.getSession();
      Rack rack = session.findObjectById(config.getObjectId(), Rack.class);
      
      if (rack != null)
      {
         if (config.isShowTitle())
         {
            title = createTitleLabel(this, rack.getObjectName());
            title.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
            title.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
         }
                  
         scroller = new ScrolledComposite(this, SWT.H_SCROLL);
         scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
         
         rackArea = new Composite(scroller, SWT.NONE) {
            @Override
            public Point computeSize(int wHint, int hHint, boolean changed)
            {
               if ((rackFrontWidget != null) && (rackRearWidget != null))
               {
                  Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
                  return new Point(s.x * 2, s.y);
               }
               else if (rackFrontWidget != null)
               {
                  Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
                  return new Point(s.x, s.y);
               }
               else if (rackRearWidget != null)
               {
                  Point s = rackRearWidget.computeSize(wHint, hHint, changed);
                  return new Point(s.x, s.y);
               }

               return super.computeSize(wHint, hHint, changed);    
            }
         };
         rackArea.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
         rackArea.addControlListener(new ControlAdapter() {
            @Override
            public void controlResized(ControlEvent e)
            {               
               int height = rackArea.getSize().y;

               if ((rackFrontWidget != null) && (rackRearWidget != null))
               {
                  Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
                  rackFrontWidget.setSize(size);
                  rackRearWidget.setSize(size);
                  rackRearWidget.setLocation(size.x, 0);
               }
               else if (rackFrontWidget != null)
               {
                  Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
                  rackFrontWidget.setSize(size);
               }
               else if (rackRearWidget != null)
               {
                  Point size = rackRearWidget.computeSize(SWT.DEFAULT, height, true);
                  rackRearWidget.setSize(size);
               }
            }
         });
         
         if (config.getView() == RackView.FULL || config.getView() == RackView.FRONT)
            setRackFrontWidget(new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.FRONT));
         if (config.getView() == RackView.FULL || config.getView() == RackView.BACK)
            setRackRearWidget(new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.REAR));

         scroller.setContent(rackArea);
         scroller.setExpandHorizontal(true);
         scroller.setExpandVertical(true);
         scroller.addControlListener(new ControlAdapter() {
            public void controlResized(ControlEvent e)
            {
               scroller.setMinSize(rackArea.computeSize(SWT.DEFAULT, scroller.getSize().y));
            }
         });
      }

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (font != null)
               font.dispose();
         }
      });
      
      RackSelectionListener listener = new RackSelectionListener() {
         @Override
         public void objectSelected(AbstractObject object)
         {
            selection = (object != null) ? new StructuredSelection(object) : new StructuredSelection();
            for(ISelectionChangedListener listener : selectionListeners)
               listener.selectionChanged(new SelectionChangedEvent(RackDiagramElement.this, selection));
         }
      };
      
      if (rackFrontWidget != null)
         rackFrontWidget.addSelectionListener(listener);
      if (rackRearWidget != null)
         rackRearWidget.addSelectionListener(listener);
      
      createPopupMenu();
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
      if (rackFrontWidget != null)
      {
         Menu menu = menuMgr.createContextMenu(rackFrontWidget);
         rackFrontWidget.setMenu(menu);
      }
      
      if (rackRearWidget != null)
      {
         Menu menu = menuMgr.createContextMenu(rackRearWidget);
         rackRearWidget.setMenu(menu);
      }

      // Register menu for extension.
      viewPart.getSite().registerContextMenu(menuMgr, this);
   }

   /**
    * Fill port context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      ObjectContextMenu.fill(manager, viewPart.getSite(), this);
   }
   
   /**
    * Get rear rack widget
    * 
    * @return Rear rack widget
    */
   public RackWidget getRackRearWidget()
   {
      return rackRearWidget;
   }

   /**
    * Set rear rack widget
    * 
    * @param rackRearWidget to set
    */
   public void setRackRearWidget(RackWidget rackRearWidget)
   {
      this.rackRearWidget = rackRearWidget;
   }

   /**
    * Get front rack widget
    * 
    * @return Front rack widget
    */
   public RackWidget getRackFrontWidget()
   {
      return rackFrontWidget;
   }

   /**
    * Set front rack widget
    * 
    * @param rackFrontWidget to set
    */
   public void setRackFrontWidget(RackWidget rackFrontWidget)
   {
      this.rackFrontWidget = rackFrontWidget;
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
