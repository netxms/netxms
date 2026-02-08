/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.helpers.NavigationHistory;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.keyboard.KeyBindingManager;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for all view containers (folder and stack).
 */
public abstract class ViewContainer extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(ViewContainer.class);

   protected Window window;
   protected Perspective perspective;
   protected MenuManager viewMenuManager;
   protected ToolBarManager viewToolBarManager;
   protected ToolBar viewToolBar;
   protected ToolBar viewControlBar;
   protected ToolItem viewMenu = null;
   protected ToolItem enableFilter = null;
   protected ToolItem navigationBack = null;
   protected ToolItem navigationForward = null;
   protected ToolItem refreshView = null;
   protected Object context;
   protected NavigationHistory navigationHistory = null;
   protected Action showFilterAction;
   protected Runnable onFilterCloseCallback = null;
   protected KeyBindingManager keyBindingManager = new KeyBindingManager();
   protected boolean isActive = true;

   /**
    * Create new view container.
    *
    * @param window owning window
    * @param perspective owning perspective (can be null)
    * @param parent parent composite for container control
    * @param style container control style
    */
   public ViewContainer(Window window, Perspective perspective, Composite parent, int style)
   {
      super(parent, style);
      this.window = window;
      this.perspective = perspective;

      viewMenuManager = new MenuManager();

      onFilterCloseCallback = new Runnable() {
         @Override
         public void run()
         {
            View view = getActiveView();
            if (view != null)
            {
               enableFilter.setSelection(false);
               view.enableFilter(enableFilter.getSelection());
            }
         }
      };

      // Keyboard binding for view menu
      keyBindingManager.addBinding(SWT.NONE, SWT.F10, new Action() {
         @Override
         public void run()
         {
            showViewMenu();
         }
      });

      // Keyboard binding for filter toggle
      showFilterAction = new Action() {
         @Override
         public void run()
         {
            View view = getActiveView();
            if ((view != null) && view.hasFilter())
            {
               view.enableFilter(!view.isFilterEnabled());
               enableFilter.setSelection(view.isFilterEnabled());
            }
         }
      };
      keyBindingManager.addBinding("M1+F2", showFilterAction);

      // Keyboard binding for filter activation
      keyBindingManager.addBinding("M1+F", new Action() {
         @Override
         public void run()
         {
            View view = getActiveView();
            if ((view != null) && view.hasFilter())
            {
               view.enableFilter(true);
               enableFilter.setSelection(true);
               view.getFilterTextControl().setFocus();
            }
         }
      });
   }

   /**
    * Show view menu
    */
   private void showViewMenu()
   {
      if (viewMenuManager.isEmpty())
         return;

      Menu menu = viewMenuManager.createContextMenu(getShell());
      Rectangle bounds = viewMenu.getBounds();
      menu.setLocation(viewControlBar.toDisplay(new Point(bounds.x, bounds.y + bounds.height + 2)));
      menu.setVisible(true);
   }

   /**
    * Update view toolbar.
    *
    * @param view currently selected view
    */
   protected void updateViewToolBar(View view)
   {
      viewToolBarManager.removeAll();
      view.fillLocalToolBar(viewToolBarManager);
      viewToolBarManager.update(true);
   }

   /**
    * Update view menu.
    *
    * @param view currently selected view
    * @return true if changes were made to view control bar
    */
   protected boolean updateViewMenu(View view)
   {
      boolean changed = false;
      viewMenuManager.removeAll();
      view.fillLocalMenu(viewMenuManager);
      if (!viewMenuManager.isEmpty())
      {
         if (viewMenu == null)
         {
            viewMenu = new ToolItem(viewControlBar, SWT.PUSH, viewControlBar.getItemCount());
            viewMenu.setImage(SharedIcons.IMG_VIEW_MENU);
            viewMenu.setToolTipText(i18n.tr("View menu (F10)"));
            viewMenu.addSelectionListener(new SelectionAdapter() {
               @Override
               public void widgetSelected(SelectionEvent e)
               {
                  showViewMenu();
               }
            });
            changed = true;
         }
      }
      else if (viewMenu != null)
      {
         viewMenu.dispose();
         viewMenu = null;
         changed = true;
      }
      return changed;
   }

   /**
    * Get current navigation history object.
    *
    * @return current navigation history object or null
    */
   public NavigationHistory getNavigationHistory()
   {
      return navigationHistory;
   }

   /**
    * Update navigation controls according to current navigation history status
    */
   protected void updateNavigationControls()
   {
      if (navigationForward != null)
         navigationForward.setEnabled((navigationHistory != null) && navigationHistory.canGoForward());
      if (navigationBack != null)
         navigationBack.setEnabled((navigationHistory != null) && navigationHistory.canGoBackward());
   }

   /**
    * Navigate back
    */
   protected void navigateBack()
   {
      if ((navigationHistory == null) || !navigationHistory.canGoBackward())
         return;
      navigationHistory.lock();
      ((NavigationView)getActiveView()).setSelection(navigationHistory.back());
      navigationHistory.unlock();
      navigationBack.setEnabled(navigationHistory.canGoBackward());
      navigationForward.setEnabled(navigationHistory.canGoForward());
   }

   /**
    * Navigate forward
    */
   protected void navigateForward()
   {
      if ((navigationHistory == null) || !navigationHistory.canGoForward())
         return;
      navigationHistory.lock();
      ((NavigationView)getActiveView()).setSelection(navigationHistory.forward());
      navigationHistory.unlock();
      navigationBack.setEnabled(navigationHistory.canGoBackward());
      navigationForward.setEnabled(navigationHistory.canGoForward());
   }

   /**
    * Update refresh action state.
    *
    * @param enabled true to enable
    */
   protected void updateRefreshActionState()
   {
      if (refreshView != null)
      {
         View view = getActiveView();
         refreshView.setEnabled((view != null) && view.isRefreshEnabled());
      }
   }

   /**
    * Process keystroke
    *
    * @param ks keystroke to process
    */
   public void processKeyStroke(KeyStroke ks)
   {
      if (keyBindingManager.processKeyStroke(ks))
         return;

      View view = getActiveView();
      if (view != null)
         view.processKeyStroke(ks);
   }

   /**
    * Refresh view that is currently active
    */
   protected void refreshActiveView()
   {
      if ((refreshView != null) && !refreshView.isEnabled())
         return;

      View view = getActiveView();
      if (view != null)
      {
         view.refresh();
      }
   }

   /**
    * Pin view that is currently active
    * 
    * @param location where top pin view
    */
   protected void pinActiveView(PinLocation location)
   {
      View view = getActiveView();
      if (view != null)
         pinView(view, location);
   }

   /**
    * Pin given view
    * 
    * @param view view to pin
    * @param location where top pin view
    */
   protected void pinView(View view, PinLocation location)
   {
      View clone = view.cloneView();
      if (clone != null)
         Registry.getMainWindow().pinView(clone, location);
   }

   /**
    * Extract view that is currently active
    */
   protected void extractActiveView()
   {
      View view = getActiveView();
      if (view != null)
         extractView(view);
   }

   /**
    * Extract given view
    */
   protected void extractView(View view)
   {
      View clone = view.cloneView();
      if (clone != null)
         PopOutViewWindow.open(clone);
   }
   
   /**
    * Set activation state for container itself 
    * 
    * @param active true to activate, false to deactivate
    */
   public void setActive(boolean active)
   {
      isActive = active;
      if (active)
      {
         View view = getActiveView();
         if (view != null)
            view.activate();
      }
      else
      {
         View view = getActiveView();
         if (view != null)
            view.deactivate();
      }
   }

   /**
    * Get active view.
    *
    * @return active view
    */
   protected abstract View getActiveView();

   /**
    * Check if given view is an active view in this container.
    *
    * @param view view to check
    * @return true if given view is an active view in this container
    */
   public abstract boolean isViewActive(View view);

   /**
    * Get owning window.
    *
    * @return owning window
    */
   public Window getWindow()
   {
      return window;
   }

   /**
    * Get owning perspective (will be null for pop-out views).
    *
    * @return owning perspective or null
    */
   public Perspective getPerspective()
   {
      return perspective;
   }
}
