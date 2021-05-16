/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.base.views;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * View stack
 */
public class ViewStack extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(ViewStack.class);

   private Window window;
   private Perspective perspective;
   private CTabFolder tabFolder;
   private Composite topRightControl;
   private ToolBarManager viewToolBarManager;
   private ToolBar viewToolBar;
   private ToolBar viewControlBar;
   private ToolItem enableFilter = null;
   private Object context;
   private boolean allViewsAreCloseable = false;
   private boolean useGlobalViewId = false;
   private Map<String, View> views = new HashMap<String, View>();
   private Map<String, CTabItem> tabs = new HashMap<String, CTabItem>();
   private Set<ViewStackSelectionListener> selectionListeners = new HashSet<ViewStackSelectionListener>();
   private Runnable onFilterCloseCallback = null;

   /**
    * Create new view stack.
    *
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param enableViewExtraction enable/disable view extraction into separate window
    * @param enableViewPinning nable/disable view extraction into "Pinboard" perspective
    */
   public ViewStack(Window window, Perspective perspective, Composite parent, boolean enableViewExtraction, boolean enableViewPinning)
   {
      super(parent, SWT.NONE);
      this.window = window;
      this.perspective = perspective;
      setLayout(new FillLayout());
      tabFolder = new CTabFolder(this, SWT.TOP | SWT.BORDER);
      tabFolder.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            CTabItem tabItem = tabFolder.getSelection();
            View view = (tabItem != null) ? (View)tabItem.getData("view") : null;
            if (view != null)
            {
               activateView(view, tabItem);
            }
            fireSelectionListeners(view);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      tabFolder.setUnselectedCloseVisible(true);

      topRightControl = new Composite(tabFolder, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      topRightControl.setLayout(layout);
      tabFolder.setTopRight(topRightControl);

      viewToolBarManager = new ToolBarManager(SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      viewToolBar = viewToolBarManager.createControl(topRightControl);
      viewToolBar.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewControlBar = new ToolBar(topRightControl, SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      viewControlBar.setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));

      ToolItem refreshView = new ToolItem(viewControlBar, SWT.PUSH);
      refreshView.setImage(SharedIcons.IMG_REFRESH);
      refreshView.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            View view = getSelection();
            if (view != null)
            {
               view.refresh();
            }
         }
      });

      enableFilter = new ToolItem(viewControlBar, SWT.CHECK);
      enableFilter.setImage(SharedIcons.IMG_FILTER);
      enableFilter.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            View view = getSelection();
            if (view != null)
            {
               view.enableFilter(enableFilter.getSelection());
            }
         }
      });
      onFilterCloseCallback = new Runnable() {
         
         @Override
         public void run()
         {
            View view = getSelection();
            if (view != null)
            {
               enableFilter.setSelection(false);
               view.enableFilter(enableFilter.getSelection());
            }            
         }
      };

      if (enableViewPinning)
      {
         ToolItem pinView = new ToolItem(viewControlBar, SWT.PUSH);
         pinView.setImage(SharedIcons.IMG_PIN);
         pinView.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               View view = getSelection();
               if (view != null)
               {
                  View clone = view.cloneView();
                  if (clone != null)
                  {
                     Registry.getMainWindow().pinView(clone);
                  }
               }
            }
         });
      }
      if (enableViewExtraction)
      {
         ToolItem popOutView = new ToolItem(viewControlBar, SWT.PUSH);
         popOutView.setImage(SharedIcons.IMG_POP_OUT);
         popOutView.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               View view = getSelection();
               if (view != null)
               {
                  View clone = view.cloneView();
                  if (clone != null)
                  {
                     PopOutViewWindow window = new PopOutViewWindow(clone);
                     window.open();
                  }
               }
            }
         });
      }
   }

   /**
    * Add view to stack. View will be created immediately if it is context-insensitive or is allowed in current context, otherwise
    * it's creation will be delayed until valid context change.
    *
    * @param view view to add
    */
   public void addView(View view)
   {
      addView(view, false, false);
   }

   /**
    * Add view to stack. View will be created immediately if it is context-insensitive or is allowed in current context, otherwise
    * it's creation will be delayed until valid context change.
    *
    * @param view view to add
    * @param activate if set to true, view will be activated
    * @param ignoreContext set to true to ignore current context
    */
   public void addView(View view, boolean activate, boolean ignoreContext)
   {
      String viewId = getViewId(view);
      View currentView = views.get(viewId);
      if (currentView != null)
      {
         if (currentView == view)
            return; // View already added

         // Dispose current view with same ID and replace with provided one
         currentView.dispose();
         CTabItem tabItem = tabs.remove(viewId);
         if (tabItem != null)
            tabItem.dispose();
      }

      views.put(viewId, view);

      if (ignoreContext || !(view instanceof ViewWithContext) || ((ViewWithContext)view).isValidForContext(context))
      {
         view.create(window, perspective, tabFolder, onFilterCloseCallback);
         CTabItem tabItem = createViewTab(view, ignoreContext);
         if (activate)
         {
            tabFolder.setSelection(tabItem);
            activateView(view, tabItem);
         }
      }
   }

   /**
    * Remove view from stack. View content will be destroyed.
    *
    * @param id ID of view to remove
    */
   public void removeView(String id)
   {
      View view = views.remove(id);
      if (view != null)
      {
         view.dispose();
         CTabItem tabItem = tabs.remove(getViewId(view));
         if (tabItem != null)
            tabItem.dispose();
      }
   }

   /**
    * Activate view internal handling of view activation.
    *
    * @param view view to activate
    * @param tabItem tab item holding view
    */
   private void activateView(View view, CTabItem tabItem)
   {
      if ((view instanceof ViewWithContext) && !ignoreContextForView(tabItem))
      {
         if (((ViewWithContext)view).getContext() != context)
            ((ViewWithContext)view).setContext(context);
      }
      viewToolBarManager.removeAll();
      view.fillLocalToolbar(viewToolBarManager);      
      viewToolBarManager.update(true);      

      enableFilter.setSelection(view.isFilterEnabled());
      enableFilter.setEnabled(view.hasFilter());

      view.activate();
   }

   /**
    * Update trim (title, actions on toolbar, etc.) for given view.
    *
    * @param view view to update
    * @return true if view trim was updated
    */
   public boolean updateViewTrim(View view)
   {
      boolean updated = false;
      CTabItem tab = tabs.get(getViewId(view));
      if (tab != null)
      {
         tab.setText(ignoreContextForView(tab) ? view.getFullName() : view.getName());
         tab.setImage(view.getImage());
         updated = true;
      }
      return updated;
   }

   /**
    * Create tab for view
    *
    * @param view view to add
    * @param ignoreContext set to true to ignore current context
    * @return created tab item
    */
   private CTabItem createViewTab(View view, boolean ignoreContext)
   {
      int index = 0;
      int priority = view.getPriority();
      for(CTabItem i : tabFolder.getItems())
      {
         if (((View)i.getData("view")).getPriority() > priority)
            break;
         index++;
      }

      CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE, index);
      tabItem.setControl(view.getViewArea());
      tabItem.setText(ignoreContext ? view.getFullName() : view.getName());
      tabItem.setImage(view.getImage());
      tabItem.setData("view", view);
      tabItem.setData("ignoreContext", Boolean.valueOf(ignoreContext));
      tabItem.setShowClose(allViewsAreCloseable || view.isCloseable());
      tabItem.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            View view = (View)e.widget.getData("view");
            String viewId = getViewId(view);
            if (e.widget.getData("keepView") == null)
            {
               views.remove(viewId);
               view.dispose();
            }
            tabs.remove(viewId);
         }
      });
      tabs.put(getViewId(view), tabItem);
      return tabItem;
   }

   /**
    * Set current context. Will update context in all context-sensitive views and hide/show views as necessary.
    *
    * @param context new context
    */
   public void setContext(Object context)
   {
      this.context = context;
      for(View view : views.values())
      {
         if (!(view instanceof ViewWithContext))
            continue;

         if (((ViewWithContext)view).isValidForContext(context))
         {
            view.setVisible(true);
            if (!tabs.containsKey(getViewId(view)))
            {
               if (view.isCreated())
                  view.setVisible(true);
               else
                  view.create(window, perspective, tabFolder, onFilterCloseCallback);
               createViewTab(view, false);
            }
            ((ViewWithContext)view).setContext(context);
         }
         else
         {
            String viewId = getViewId(view);
            CTabItem tabItem = tabs.remove(viewId);
            if (tabItem != null)
            {
               logger.debug("View " + viewId + " is not valid for current context");
               tabItem.setData("keepView", Boolean.TRUE); // Prevent view dispose by tab's dispose listener
               tabItem.dispose();
               view.setVisible(false);
            }
         }
      }

      // Select first view if none were selected
      if (tabFolder.getSelectionIndex() == -1)
         tabFolder.setSelection(0);
   }

   /**
    * Get currently selected view.
    *
    * @return currently selected view or null
    */
   public View getSelection()
   {
      CTabItem selection = tabFolder.getSelection();
      return (selection != null) ? (View)selection.getData("view") : null;
   }

   /**
    * Add selection listener.
    *
    * @param listener listener to add.
    */
   public void addSelectionListener(ViewStackSelectionListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * Remove selection listener.
    * 
    * @param listener listener to remove
    */
   public void removeSelectionListener(ViewStackSelectionListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * Call registered selection listeners when view selection changed.
    *
    * @param view new selection
    */
   private void fireSelectionListeners(View view)
   {
      for(ViewStackSelectionListener l : selectionListeners)
         l.viewSelected(view);
   }

   /**
    * Check if all views are closeable.
    *
    * @return true if all views are closeable
    */
   public boolean areAllViewsCloseable()
   {
      return allViewsAreCloseable;
   }

   /**
    * If set to true then all views will be marked as closeable and isCloseable() result will be ignored.
    *
    * @param allViewsAreCloseable if true all views will be marked as closeable
    */
   public void setAllViewsAaCloseable(boolean allViewsAreCloseable)
   {
      this.allViewsAreCloseable = allViewsAreCloseable;
   }

   /**
    * Check if context should be ignored for view in given tab.
    *
    * @param tabItem tab item
    * @return true if context should be ignored
    */
   private static boolean ignoreContextForView(CTabItem tabItem)
   {
      Object ignoreContext = tabItem.getData("ignoreContext");
      return (ignoreContext != null) && (ignoreContext instanceof Boolean) && (Boolean)ignoreContext;
   }

   /**
    * Get view identification mode.
    *
    * @return true if view stack uses global ID to check if view already present
    */
   public boolean isUseGlobalViewId()
   {
      return useGlobalViewId;
   }

   /**
    * Set view identification mode. If useGlobalViewId set to true, view stack will use global IDs to check if view already present,
    * otherwise base ID will be used.
    *
    * @param useGlobalViewId true to use global view ID to identify duplicate views
    */
   public void setUseGlobalViewId(boolean useGlobalViewId)
   {
      this.useGlobalViewId = useGlobalViewId;
   }

   /**
    * Get ID for given view. Dependng on operation mode will return base or global view ID.
    * 
    * @param view view to get ID from
    * @return view ID according to current operation mode
    */
   private String getViewId(View view)
   {
      return useGlobalViewId ? view.getGlobalId() : view.getBaseId();
   }
}
