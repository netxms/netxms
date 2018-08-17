/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
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
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.dashboard.widgets.internal.PortViewConfig;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.widgets.DeviceView;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortInfo;
import org.netxms.ui.eclipse.topology.widgets.helpers.PortSelectionListener;

/**
 * Alarm viewer element for dashboard
 */
public class PortViewElement extends ElementWidget
{
   private ScrolledComposite scroller;
   private Composite content;
	private Map<Long, DeviceView> deviceViews = new HashMap<Long, DeviceView>();
	private PortViewConfig config;
	private NXCSession session;
	private FocusListener focusListener;
	private ISelectionProvider selectionProvider;
	private ISelection selection = new StructuredSelection();
   private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   private MenuManager popupMenuManager;
	
	/**
	 * Create new alarm viewer element
	 * 
	 * @param parent Dashboard control
	 * @param element Dashboard element
	 * @param viewPart viewPart
	 */
	public PortViewElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		session = ConsoleSharedData.getSession();

		try
		{
			config = PortViewConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new PortViewConfig();
		}
		
		selectionProvider = new ISelectionProvider() {
         @Override
         public void setSelection(ISelection selection)
         {
            PortViewElement.this.selection = selection;
         }
         
         @Override
         public void removeSelectionChangedListener(ISelectionChangedListener listener)
         {
            selectionListeners.remove(listener);
         }
         
         @Override
         public ISelection getSelection()
         {
            return selection;
         }
         
         @Override
         public void addSelectionChangedListener(ISelectionChangedListener listener)
         {
            selectionListeners.add(listener);
         }
      };
		
		focusListener = new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
         }
         
         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(selectionProvider);
         }
      };
      
      createPopupMenu();
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		setLayout(layout);

		scroller = new ScrolledComposite(this, SWT.H_SCROLL | SWT.V_SCROLL);
      scroller.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, getDisplay()));
		
		content = new Composite(scroller, SWT.NONE);
		content.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, getDisplay()));
		GridLayout contentLayout = new GridLayout();
		contentLayout.verticalSpacing = WidgetHelper.OUTER_SPACING * 3;
		content.setLayout(contentLayout);

		buildView();
		
      scroller.setContent(content);
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(content.computeSize(SWT.DEFAULT, SWT.DEFAULT));
         }
      });
      
      final SessionListener sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() != SessionNotification.OBJECT_CHANGED)
               return;
            
            AbstractObject object = (AbstractObject)n.getObject();
            if ((n.getSubCode() == config.getRootObjectId()) || object.isChildOf(config.getRootObjectId()))
            {
               getDisplay().asyncExec(new Runnable() {
                  public void run()
                  {
                     if (needRebuild())
                     {
                        for(DeviceView d : deviceViews.values())
                           d.dispose();
                        deviceViews.clear();
                        buildView();
                        content.layout(true, true);
                        scroller.setMinSize(content.computeSize(SWT.DEFAULT, SWT.DEFAULT));
                     }
                  }
               });
            }
         }
      };
      session.addListener(sessionListener);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            session.removeListener(sessionListener);
         }
      });
	}
	
	/**
	 * Build view
	 */
	private void buildView()
	{
      AbstractObject root = session.findObjectById(config.getRootObjectId());
      if (root instanceof Node)
      {
         if (((Node)root).isBridge())
            addDeviceView((Node)root);
      }
      else
      {
         List<AbstractObject> nodes = new ArrayList<AbstractObject>(root.getAllChilds(AbstractObject.OBJECT_NODE));
         Collections.sort(nodes, new Comparator<AbstractObject>() {
            @Override
            public int compare(AbstractObject o1, AbstractObject o2)
            {
               return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
            }
         });
         for(AbstractObject o : nodes)
         {
            if (((Node)o).isBridge())
               addDeviceView((Node)o);
         }
      }
	}

   /**
    * Check if view re-build is needed 
    */
   private boolean needRebuild()
   {
      AbstractObject root = session.findObjectById(config.getRootObjectId());
      if (root instanceof Node)
         return (!deviceViews.containsKey(root.getObjectId()) && ((Node)root).isBridge()) ||
               (deviceViews.containsKey(root.getObjectId()) && !((Node)root).isBridge());
      
      Set<AbstractObject> nodes = root.getAllChilds(AbstractObject.OBJECT_NODE);
      for(AbstractObject o : nodes)
      {
         if (((Node)o).isBridge())
         {
            if (!deviceViews.containsKey(o.getObjectId()))
               return true;
         }
         else
         {
            if (deviceViews.containsKey(o.getObjectId()))
               return true;
         }
      }
      
      for(Long id : deviceViews.keySet())
      {
         boolean found = false;
         for(AbstractObject o : nodes)
         {
            if (o.getObjectId() == id)
            {
               found = true;
               break;
            }
         }
         if (!found)
            return true;
      }
      
      return false;
   }
	
   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      popupMenuManager = new MenuManager();
      popupMenuManager.setRemoveAllWhenShown(true);
      popupMenuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            ObjectContextMenu.fill(mgr, viewPart.getSite(), selectionProvider);
         }
      });

      // Register menu for extension.
      viewPart.getSite().registerContextMenu(popupMenuManager, selectionProvider);
   }
	
	/**
	 * Add device view
	 * 
	 * @param n node
	 */
	private void addDeviceView(Node n)
	{
      DeviceView d = new DeviceView(content, SWT.NONE);
      d.setHeaderVisible(true);
      d.setNodeId(n.getObjectId());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      d.setLayoutData(gd);

      // Create menu.
      Menu menu = popupMenuManager.createContextMenu(d);
      d.setMenu(menu);
      
      d.addFocusListener(focusListener);
      d.addSelectionListener(new PortSelectionListener() {
         @Override
         public void portSelected(PortInfo port)
         {
            if (port != null)
            {
               Interface iface = (Interface)session.findObjectById(port.getInterfaceObjectId(), Interface.class);
               if (iface != null)
               {
                  selection = new StructuredSelection(iface);
               }
               else
               {
                  selection = new StructuredSelection();
               }
            }
            else
            {
               selection = new StructuredSelection();
            }
            
            for(ISelectionChangedListener listener : selectionListeners)
               listener.selectionChanged(new SelectionChangedEvent(getSelectionProvider(), selection));
         }
      });
      
      deviceViews.put(n.getObjectId(), d);
	}
}
