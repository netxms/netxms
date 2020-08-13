/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * View stack
 */
public class ViewStack extends Composite
{
   private Window window;
   private Perspective perspective;
   private CTabFolder tabFolder;
   private ToolBar viewControlBar;
   private Object context;
   private boolean allViewsAreCloseable = false;
   private Map<String, View> views = new HashMap<String, View>();
   private Map<String, CTabItem> tabs = new HashMap<String, CTabItem>();
   private Set<ViewStackSelectionListener> selectionListeners = new HashSet<ViewStackSelectionListener>();

   /**
    * Create new view stack.
    *
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param enableViewExtraction enable/disable view extraction into separate window
    * @param enableViewPinning nable/disable view extraction into "Pinboard" perspective
    */
   public ViewStack(Window window, Perspective perspective, Composite parent, boolean enableViewExtraction,
         boolean enableViewPinning)
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
               view.activate();
            fireSelectionListeners(view);
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      tabFolder.setUnselectedCloseVisible(true);

      viewControlBar = new ToolBar(tabFolder, SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      tabFolder.setTopRight(viewControlBar);

      ToolItem refreshView = new ToolItem(viewControlBar, SWT.PUSH);
      refreshView.setImage(SharedIcons.IMG_REFRESH);

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
      addView(view, false);
   }

   /**
    * Add view to stack. View will be created immediately if it is context-insensitive or is allowed in current context, otherwise
    * it's creation will be delayed until valid context change.
    *
    * @param view view to add
    * @param ignoreContext set to true to ignore current context
    */
   public void addView(View view, boolean ignoreContext)
   {
      View currentView = views.get(view.getId());
      if (currentView != null)
      {
         if (currentView == view)
            return; // View already added

         // Dispose current view with same ID and replace with provided one
         currentView.dispose();
         CTabItem tabItem = tabs.remove(view.getId());
         if (tabItem != null)
            tabItem.dispose();
      }

      views.put(view.getId(), view);

      if (ignoreContext || !(view instanceof ViewWithContext) || ((ViewWithContext)view).isValidForContext(context))
      {
         view.create(window, perspective, tabFolder);

         CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
         tabItem.setControl(view.getViewArea());
         tabItem.setText(ignoreContext ? view.getFullName() : view.getName());
         tabItem.setImage(view.getImage());
         tabItem.setData("view", view);
         tabItem.setShowClose(allViewsAreCloseable || view.isCloseable());
         tabItem.addDisposeListener(new DisposeListener() {
            @Override
            public void widgetDisposed(DisposeEvent e)
            {
               ((View)e.widget.getData("view")).dispose();
            }
         });
         tabs.put(view.getId(), tabItem);
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
         CTabItem tabItem = tabs.remove(view.getId());
         if (tabItem != null)
            tabItem.dispose();
      }
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
            if (!tabs.containsKey(view.getId()))
            {
               if (view.isCreated())
                  view.setVisible(true);
               else
                  view.create(window, perspective, tabFolder);

               CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
               tabItem.setControl(view.getViewArea());
               tabItem.setText(view.getName());
               tabItem.setImage(view.getImage());
               tabItem.setData("view", view);
               tabs.put(view.getId(), tabItem);
            }
            ((ViewWithContext)view).setContext(context);
         }
         else
         {
            CTabItem tabItem = tabs.remove(view.getId());
            if (tabItem != null)
            {
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
}
