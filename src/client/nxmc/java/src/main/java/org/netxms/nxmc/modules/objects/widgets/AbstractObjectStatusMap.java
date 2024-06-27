/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ScrollBar;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.alarms.views.AdHocAlarmsView;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.RefreshTimer;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Common parent class for status maps
 */
public abstract class AbstractObjectStatusMap extends Composite implements ISelectionProvider
{
   protected ObjectView view;
   protected long rootObjectId = 0;
   protected NXCSession session;
   protected ISelection selection = null;
   protected Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   protected MenuManager menuManager;
   protected int severityFilter = 0xFF;
   protected String textFilter = "";
   protected boolean hideObjectsInMaintenance = false;
   protected Set<Runnable> refreshListeners = new HashSet<Runnable>();
   protected RefreshTimer refreshTimer;
   protected boolean fitToScreen;

   private Composite content;
   private ScrolledComposite scroller;

   /**
    * Create object status map.
    *
    * @param view owning view
    * @param parent parent composite
    * @param style widget's style
    */
   public AbstractObjectStatusMap(ObjectView view, Composite parent, int style)
   {
      super(parent, style);

      this.view = view;
      session = Registry.getSession();
      final SessionListener sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.OBJECT_CHANGED)
               onObjectChange((AbstractObject)n.getObject());
            else if (n.getCode() == SessionNotification.OBJECT_DELETED)
               onObjectDelete(n.getSubCode());
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
      
      setLayout(new FillLayout());

      setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller = new ScrolledComposite(this, SWT.V_SCROLL | SWT.H_SCROLL);
      scroller.setBackground(getBackground());
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            updateScrollBars();
         }
      });
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 30);

      menuManager = new ObjectContextMenuManager(view, this, null);

      content = createContent(scroller); 
      scroller.setContent(content);

      refreshTimer = new RefreshTimer(10000, this, new Runnable() {
         @Override
         public void run()
         {
            refresh();
         }
      });
   }

   /**
    * Create status widget content
    * 
    * @param parent parent composite
    * @return content for scroller
    */
   protected abstract Composite createContent(Composite parent);

   /**
    * Check if given object is accepted by filter
    * 
    * @param object object to test
    * @return true if object can pass filter
    */
   protected boolean isAcceptedByFilter(AbstractObject object)
   {
      if (((1 << object.getStatus().getValue()) & severityFilter) == 0)
         return false;

      if (hideObjectsInMaintenance && object.isInMaintenanceMode())
         return false;

      if (!textFilter.isEmpty())
      {
         boolean match = false;
         for(String s : object.getStrings())
         {
            if (s.toLowerCase().contains(textFilter))
            {
               match = true;
               break;
            }
         }
         if (!match)
            return false;
      }
      
      return true;
   }

   /**
    * Filter given object collection
    * 
    * @param objects object collection to filter
    */
   protected void filterObjects(Collection<AbstractObject> objects)
   {
      if (((severityFilter & 0x3F) == 0x3F) && !hideObjectsInMaintenance && textFilter.isEmpty())
         return;  // filter is not set

      Iterator<AbstractObject> it = objects.iterator();
      while(it.hasNext())
      {
         AbstractObject o = it.next();
         if (!isAcceptedByFilter(o))
            it.remove();
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
      SelectionChangedEvent event = new SelectionChangedEvent(this, selection);
      for(ISelectionChangedListener l : selectionListeners)
         l.selectionChanged(event);
   }

   /**
    * Show object details
    * 
    * @param object object to show details for
    */
   protected void showObjectDetails(AbstractObject object)
   {
      // TODO: implement custom handlers
      // Default implementation will show alarm list for object
      view.openView(new AdHocAlarmsView(view.getObjectId(), object));
   }

   /**
    * Get root object ID.
    *
    * @return root object ID
    */
   public long getRootObjectId()
   {
      return rootObjectId;
   }

   /**
    * Set root object ID.
    *
    * @param rootObjectId new root object ID
    */
   public void setRootObjectId(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }

   /**
    * @return the textFilter
    */
   public String getTextFilter()
   {
      return textFilter;
   }

   /**
    * @param textFilter the textFilter to set
    */
   public void setTextFilter(String textFilter)
   {
      this.textFilter = (textFilter != null) ? textFilter.strip().toLowerCase() : "";
   }

   /**
    * @return
    */
   public int getSeverityFilter()
   {
      return severityFilter;
   }

   /**
    * @param severityFilter
    */
   public void setSeverityFilter(int severityFilter)
   {
      this.severityFilter = severityFilter;
   }

   /**
    * @return the hideObjectsInMaintenance
    */
   public boolean isHideObjectsInMaintenance()
   {
      return hideObjectsInMaintenance;
   }

   /**
    * @param hideObjectsInMaintenance the hideObjectsInMaintenance to set
    */
   public void setHideObjectsInMaintenance(boolean hideObjectsInMaintenance)
   {
      this.hideObjectsInMaintenance = hideObjectsInMaintenance;
   }

   /**
    * @param listener
    */
   public void addRefreshListener(Runnable listener)
   {
      refreshListeners.add(listener);
   }
   
   /**
    * @param listener
    */
   public void removeRefreshListener(Runnable listener)
   {
      refreshListeners.remove(listener);
   }

   /**
    * Refresh view
    */
   abstract public void refresh();
   
   /**
    * Handler for object change
    * 
    * @param object
    */
   abstract protected void onObjectChange(final AbstractObject object);

   /**
    * Handler for object delete
    */
   abstract protected void onObjectDelete(final long objectId);

   /**
    * Handler for widget size calculation
    * @return widget size as a Point
    */
   abstract protected Point computeSize();

   /**
    * Check if given object is a container
    * 
    * @param object
    * @return
    */
   protected static boolean isContainerObject(AbstractObject object)
   {
      return (object instanceof Collector) || (object instanceof Circuit) || (object instanceof Container) || (object instanceof Cluster) || 
            (object instanceof Rack) || (object instanceof Chassis) || (object instanceof ServiceRoot) || 
            (object instanceof WirelessDomain);
   }
   
   /**
    * Check if given object is a leaf object
    * 
    * @param object
    * @return
    */
   protected static boolean isLeafObject(AbstractObject object)
   {
      return (object instanceof Node) || (object instanceof MobileDevice) || (object instanceof Sensor) || (object instanceof AccessPoint);
   }

   /**
    * Sets fit to screen parameter 
    * 
    * @param fitToScreen
    */
   public void setFitToScreen(boolean fitToScreen)
   {
      this.fitToScreen = fitToScreen;      
      refresh();
      updateScrollBars();
   }

   /**
    * Update scrollbars after content change 
    */
   protected void updateScrollBars()
   {
      scroller.setMinSize(computeSize());
   }
   
   /**
    * Get available client area (without scroll bars)
    * 
    * @return
    */
   protected Rectangle getAvailableClientArea()
   {
      Rectangle rect = scroller.getClientArea();
      ScrollBar sb = scroller.getVerticalBar();
      if (sb != null)
         rect.width -= sb.getSize().x;
      sb = scroller.getHorizontalBar();
      if (sb != null)
         rect.height -= sb.getSize().y;
      return rect;
   }
}
