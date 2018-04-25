/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.ui.eclipse.objectview.widgets;

import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.objectview.api.ObjectDetailsProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.RefreshTimer;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * Common parent class for status maps
 */
public abstract class AbstractObjectStatusMap extends Composite implements ISelectionProvider
{
   protected IViewPart viewPart;
   protected long rootObjectId;
   protected NXCSession session;
   protected FilterText filterTextControl;
   protected ISelection selection = null;
   protected Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
   protected MenuManager menuManager;
   protected boolean filterEnabled = true;
   protected int severityFilter = 0xFF;
   protected String textFilter = "";
   protected SortedMap<Integer, ObjectDetailsProvider> detailsProviders = new TreeMap<Integer, ObjectDetailsProvider>();
   protected Set<Runnable> refreshListeners = new HashSet<Runnable>();
   protected RefreshTimer refreshTimer;
   protected boolean fitToScreen;

   private Composite content;
   private ScrolledComposite scroller;
   
   /**
    * @param parent
    * @param style
    */
   public AbstractObjectStatusMap(IViewPart viewPart, Composite parent, int style, boolean allowFilterClose)
   {
      super(parent, style);

      initDetailsProviders();
      
      this.viewPart = viewPart;
      session = ConsoleSharedData.getSession();
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
      
      FormLayout formLayout = new FormLayout();
      setLayout(formLayout);
      
      setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, getDisplay()));

      // Create filter area
      filterTextControl = new FilterText(this, SWT.NONE, null, allowFilterClose);
      filterTextControl.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterTextControl.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });
      
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
      //scroller.getVerticalBar().setIncrement(30);
      
      menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterTextControl);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      scroller.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterTextControl.setLayoutData(fd);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterTextControl.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

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
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      ObjectContextMenu.fill(manager, (viewPart != null) ? viewPart.getSite() : null, this); 
   }

   /**
    * @param objectId
    */
   public void setRootObject(long objectId)
   {
      rootObjectId = objectId;
      refresh();
   }
   
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
      if (((severityFilter & 0x3F) == 0x3F) && textFilter.isEmpty())
         return;  // filter is not set
      
      Iterator<AbstractObject> it = objects.iterator();
      while(it.hasNext())
      {
         AbstractObject o = it.next();
         if (!isAcceptedByFilter(o))
            it.remove();
      }
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
      SelectionChangedEvent event = new SelectionChangedEvent(this, selection);
      for(ISelectionChangedListener l : selectionListeners)
         l.selectionChanged(event);
   }

   /**
    * Initialize object details providers
    */
   private void initDetailsProviders()
   {
      // Read all registered extensions and create tabs
      final IExtensionRegistry reg = Platform.getExtensionRegistry();
      IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectview.objectDetailsProvider"); //$NON-NLS-1$
      for(int i = 0; i < elements.length; i++)
      {
         try
         {
            final ObjectDetailsProvider provider = (ObjectDetailsProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
            int priority;
            try
            {
               priority = Integer.parseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
            }
            catch(NumberFormatException e)
            {
               priority = 65535;
            }
            detailsProviders.put(priority, provider);
         }
         catch(CoreException e)
         {
            // TODO Auto-generated catch block
            e.printStackTrace();
         }
      }
   }
   
   /**
    * Call object details provider
    * 
    * @param node
    */
   protected void callDetailsProvider(AbstractObject object)
   {
      for(ObjectDetailsProvider p : detailsProviders.values())
      {
         if (p.canProvideDetails(object))
         {
            p.provideDetails(object, viewPart);
            break;
         }
      }
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
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterTextControl.setVisible(filterEnabled);
      FormData fd = (FormData)scroller.getLayoutData();
      fd.top = enable ? new FormAttachment(filterTextControl) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterTextControl.setFocus();
      else
         setFilterText(""); //$NON-NLS-1$
   }

   /**
    * @return the filterEnabled
    */
   public boolean isFilterEnabled()
   {
      return filterEnabled;
   }
   
   /**
    * Set action to be executed when user press "Close" button in object filter.
    * Default implementation will hide filter area without notifying parent.
    * 
    * @param action
    */
   public void setFilterCloseAction(Action action)
   {
      filterTextControl.setCloseAction(action);
   }
   
   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilterText(final String text)
   {
      filterTextControl.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return filterTextControl.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      String text = filterTextControl.getText().trim().toLowerCase();
      if (!textFilter.equals(text))
      {
         textFilter = text;
         refresh();
      }
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
      return (object instanceof Container) || (object instanceof Cluster) || (object instanceof Rack) || (object instanceof Chassis);
   }
   
   /**
    * Check if given object is a leaf object
    * 
    * @param object
    * @return
    */
   protected static boolean isLeafObject(AbstractObject object)
   {
      return (object instanceof Node) || (object instanceof MobileDevice);
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
