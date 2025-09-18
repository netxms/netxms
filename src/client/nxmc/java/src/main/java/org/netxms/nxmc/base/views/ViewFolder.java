/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabFolder2Adapter;
import org.eclipse.swt.custom.CTabFolderEvent;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * View folder (multiple views represented as tabs)
 */
public class ViewFolder extends ViewContainer
{
   private static final Logger logger = LoggerFactory.getLogger(ViewFolder.class);

   private final I18n i18n = LocalizationHelper.getI18n(ViewFolder.class);

   private CTabFolder tabFolder;
   private Composite topRightControl;
   private boolean allViewsAreCloseable = false;
   private boolean enableViewExtraction;
   private boolean enableViewPinning;
   private boolean disposeWhenEmpty = false;
   private boolean useGlobalViewId = false;
   private boolean enableViewHide = false;
   private String preferredViewId = null;
   private String lastViewId = null;
   private View activeView = null;
   private CTabItem contextMenuTabItem = null;
   private boolean contextChange = false;
   private Map<String, View> views = new HashMap<>();
   private Map<String, CTabItem> tabs = new HashMap<>();
   private Set<ViewFolderSelectionListener> selectionListeners = new HashSet<>();

   /**
    * Create new view folder.
    *
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param enableViewExtraction enable/disable view extraction into separate window
    * @param enableViewPinning enable/disable view extraction into "pinboard" perspective
    * @param enableNavigationHistory enable/disable navigation history
    * @param enableViewHide enable/disable hide of views
    */
   public ViewFolder(Window window, Perspective perspective, Composite parent, boolean enableViewExtraction, boolean enableViewPinning, boolean enableNavigationHistory, boolean enableViewHide)
   {
      super(window, perspective, parent, SWT.NONE);

      this.enableViewExtraction = enableViewExtraction;
      this.enableViewPinning = enableViewPinning;
      this.enableViewHide = enableViewHide;

      setLayout(new FillLayout());
      tabFolder = new CTabFolder(this, SWT.TOP | SWT.BORDER);
      tabFolder.setUnselectedCloseVisible(true);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);
      tabFolder.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (contextChange)
               return; // Do not handle selection change during context change

            CTabItem tabItem = tabFolder.getSelection();
            View view = (tabItem != null) ? (View)tabItem.getData("view") : null;
            if (view != null)
            {
               activateView(view, tabItem);
               view.setFocus();
            }
            fireSelectionListeners(view);
         }
      });
      tabFolder.addCTabFolder2Listener(new CTabFolder2Adapter() {
         @Override
         public void close(CTabFolderEvent event)
         {
            CTabItem tabItem = tabFolder.getSelection();
            View view = (tabItem != null) ? (View)tabItem.getData("view") : null;
            if (view != null)
            {
               event.doit = view.beforeClose();
            }
            if (event.doit)
            {
               if (activeView != null)
               {
                  activeView.deactivate();
                  activeView = null;
               }
               if (lastViewId != null)
               {
                  showView(lastViewId);
               }
            }
         }
      });
      tabFolder.addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }
         
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 2) //middle mouse button 
            {
               // FInd tab item under mouse and block event if click is not on the tab
               CTabItem tab = null;
               Point pt = new Point(e.x, e.y);
               for(CTabItem t : tabFolder.getItems())
               {
                  if (t.getBounds().contains(pt))
                  {
                     tab = t;
                     break;
                  }
               }
               if (tab != null)
               {
                  View view = (View)tab.getData("view");
                  if(view.isCloseable())
                     closeView(view);                  
               }
            }            
         }
         
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });

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

      if (enableNavigationHistory)
      {
         navigationBack = new ToolItem(viewControlBar, SWT.PUSH);
         navigationBack.setImage(SharedIcons.IMG_NAV_BACKWARD);
         navigationBack.setToolTipText(i18n.tr("Back (Alt+Left)"));
         navigationBack.setEnabled(false);
         navigationBack.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               navigateBack();
            }
         });
         keyBindingManager.addBinding(SWT.ALT, SWT.ARROW_LEFT, new Action() {
            @Override
            public void run()
            {
               navigateBack();
            }
         });

         navigationForward = new ToolItem(viewControlBar, SWT.PUSH);
         navigationForward.setImage(SharedIcons.IMG_NAV_FORWARD);
         navigationForward.setToolTipText(i18n.tr("Forward (Alt+Right)"));
         navigationForward.setEnabled(false);
         navigationForward.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               navigateForward();
            }
         });
         keyBindingManager.addBinding(SWT.ALT, SWT.ARROW_RIGHT, new Action() {
            @Override
            public void run()
            {
               navigateForward();
            }
         });
      }

      refreshView = new ToolItem(viewControlBar, SWT.PUSH);
      refreshView.setImage(SharedIcons.IMG_REFRESH);
      refreshView.setToolTipText(i18n.tr("Refresh (F5)"));
      refreshView.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            refreshActiveView();
         }
      });
      keyBindingManager.addBinding(SWT.NONE, SWT.F5, new Action() {
         @Override
         public void run()
         {
            refreshActiveView();
         }
      });

      if (enableViewPinning)
      {
         final Menu pinMenu = createPinMenu();
         ToolItem pinView = new ToolItem(viewControlBar, SWT.DROP_DOWN);
         pinView.setImage(SharedIcons.IMG_PIN);
         pinView.setToolTipText(i18n.tr("Pin view (F7)"));
         pinView.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               if (e.detail == SWT.ARROW)
               {
                  ToolItem item = (ToolItem)e.widget;
                  Rectangle rect = item.getBounds();
                  Point pt = item.getParent().toDisplay(new Point(rect.x, rect.y));
                  pinMenu.setLocation(pt.x, pt.y + rect.height);
                  pinMenu.setVisible(true);
               }
               else
               {
                  pinActiveView(Registry.getLastViewPinLocation());
               }
            }
         });
         keyBindingManager.addBinding(SWT.NONE, SWT.F7, new Action() {
            @Override
            public void run()
            {
               pinActiveView(Registry.getLastViewPinLocation());
            }
         });
      }
      if (enableViewExtraction)
      {
         ToolItem popOutView = new ToolItem(viewControlBar, SWT.PUSH);
         popOutView.setImage(SharedIcons.IMG_POP_OUT);
         popOutView.setToolTipText(i18n.tr("Pop out view (F8)"));
         popOutView.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               extractActiveView();
            }
         });
         keyBindingManager.addBinding(SWT.NONE, SWT.F8, new Action() {
            @Override
            public void run()
            {
               extractActiveView();
            }
         });
      }

      keyBindingManager.addBinding("M1+W", new Action() {
         @Override
         public void run()
         {
            View view = getActiveView();
            if ((view != null) && view.isCloseable())
               closeView(view);
         }
      });

      createTabContextMenu();
   }

   /**
    * Create context menu for view tabs
    */
   private void createTabContextMenu()
   {
      MenuManager tabMenuManager = new MenuManager();
      tabMenuManager.setRemoveAllWhenShown(true);
      tabMenuManager.addMenuListener(new IMenuListener() {
         @Override
         public void menuAboutToShow(IMenuManager manager)
         {
            fillTabContextMenu(manager);
         }
      });

      Menu menu = tabMenuManager.createContextMenu(tabFolder);
      tabFolder.setMenu(menu);

      tabFolder.addMenuDetectListener(new MenuDetectListener() {
         @Override
         public void menuDetected(MenuDetectEvent e)
         {
            // FInd tab item under mouse and block event if click is not on the tab
            contextMenuTabItem = null;
            Point pt = tabFolder.toControl(e.x, e.y);
            for(CTabItem t : tabFolder.getItems())
            {
               if (t.getBounds().contains(pt))
               {
                  contextMenuTabItem = t;
                  break;
               }
            }
            e.doit = (contextMenuTabItem != null);
         }
      });
   }

   /**
    * Fill context menu for tab
    *
    * @param manager manager
    */
   private void fillTabContextMenu(IMenuManager manager)
   {
      if (contextMenuTabItem == null)
         return;

      final View view = (View)contextMenuTabItem.getData("view");
      if (allViewsAreCloseable || view.isCloseable())
      {
         manager.add(new Action(i18n.tr("&Close") + "\t" + KeyStroke.normalizeDefinition("M1+W")) {
            @Override
            public void run()
            {
               closeView(view);
            }
         });
      }

      if (enableViewPinning)
      {
         manager.add(new Separator());
         createPinMenuItem(manager, view, PinLocation.PINBOARD, i18n.tr("&Pin to pinboard"));
         createPinMenuItem(manager, view, PinLocation.LEFT, i18n.tr("Pin at &left"));
         createPinMenuItem(manager, view, PinLocation.RIGHT, i18n.tr("Pin at &right"));
         createPinMenuItem(manager, view, PinLocation.BOTTOM, i18n.tr("Pin at &bottom"));
      }

      if (enableViewExtraction)
      {
         manager.add(new Separator());
         manager.add(new Action(i18n.tr("Pop &out") + "\t" + KeyStroke.normalizeDefinition("F8")) {
            @Override
            public void run()
            {
               extractView(view);
            }
         });
      }

      if (enableViewHide && !view.isCloseable())
      {
         manager.add(new Separator());
         manager.add(new Action(i18n.tr("&Hide")) {
            @Override
            public void run()
            {
               hideView(view);
            }
         });
      }
   }
   
   /**
    * Hide view
    */
   private void hideView(final View view)
   {
      PreferenceStore preferenceStore = PreferenceStore.getInstance();
      preferenceStore.set("HideView." + view.getBaseId(), true);
      updateViewSet();
   }

   /**
    * Create view pinning menu
    *
    * @return view pinning menu
    */
   private Menu createPinMenu()
   {
      Menu menu = new Menu(viewControlBar);
      createPinMenuItem(menu, PinLocation.PINBOARD, i18n.tr("&Pinboard"));
      createPinMenuItem(menu, PinLocation.LEFT, i18n.tr("&Left"));
      createPinMenuItem(menu, PinLocation.RIGHT, i18n.tr("&Right"));
      createPinMenuItem(menu, PinLocation.BOTTOM, i18n.tr("&Bottom"));
      return menu;
   }

   /**
    * Create item for view pinning menu
    *
    * @param menu menu
    * @param location pin location
    * @param name menu item name
    */
   private void createPinMenuItem(Menu menu, final PinLocation location, String name)
   {
      MenuItem item = new MenuItem(menu, SWT.PUSH);
      item.setText(name);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            Registry.setLastViewPinLocation(location);
            pinActiveView(location);
         }
      });
   }

   /**
    * Create item for view pinning menu
    *
    * @param manager menu manager
    * @param view view to pin
    * @param location pin location
    * @param name menu item name
    */
   private void createPinMenuItem(IMenuManager manager, final View view, final PinLocation location, String name)
   {
      manager.add(new Action(name) {
         @Override
         public void run()
         {
            Registry.setLastViewPinLocation(location);
            pinView(view, location);
         }
      });
   }

   /**
    * Add view to folder. View will be created immediately if it is context-insensitive or is allowed in current context, otherwise
    * it's creation will be delayed until valid context change.
    *
    * @param view view to add
    */
   public void addView(View view)
   {
      addView(view, false, false);
   }

   /**
    * Add view to folder. View will be created immediately if it is context-insensitive or is allowed in current context, otherwise
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
      if (currentView == view)
         return; // View already added

      if (currentView != null)
      {
         // Dispose current view with same ID and replace with provided one
         boolean originalDisposeWhenEmpty = disposeWhenEmpty;
         disposeWhenEmpty = false;
         currentView.dispose();
         CTabItem tabItem = tabs.remove(viewId);
         if (tabItem != null)
            tabItem.dispose();
         disposeWhenEmpty = originalDisposeWhenEmpty;
      }

      views.put(viewId, view);

      if (ignoreContext || !(view instanceof ViewWithContext) || (((ViewWithContext)view).isValidForContext(context) && !((ViewWithContext)view).isHidden()))
      {
         view.create(this, tabFolder, onFilterCloseCallback);
         CTabItem tabItem = createViewTab(view, ignoreContext);
         if (activate || (activeView == null))
         {
            tabFolder.setSelection(tabItem);
            activateView(view, tabItem);
         }
      }
   }

   /**
    * Find view with given ID.
    *
    * @param id view ID
    * @return view with given ID or null if not exist
    */
   public View findView(String id)
   {
      return views.get(id);
   }

   /**
    * Remove view from folder. View content will be destroyed.
    *
    * @param id ID of view to remove
    */
   public void removeView(String id)
   {
      View view = views.remove(id);
      if (view != null)
      {
         closeView(view);
      }
   }

   /**
    * Get all views in this folder.
    *
    * @return all views in this folder
    */
   public View[] getAllViews()
   {
      return views.values().toArray(View[]::new);
   }

   /**
    * Show view with given ID.
    *
    * @param viewId view ID
    * @return true if view was shown
    */
   public boolean showView(String viewId)
   {
      View view = views.get(viewId);
      if (view == null)
         return false;

      CTabItem tabItem = tabs.get(viewId);
      if (tabItem == null)
         return false;

      tabFolder.setSelection(tabItem);
      activateView(view, tabItem);
      return true;
   }

   /**
    * Prepare given view activation.
    *
    * @param view view to prepare
    */
   private void prepareViewActivation(View view)
   {
      updateViewMenu(view);
      updateViewToolBar(view);

      if (view.hasFilter())
      {
         if (enableFilter == null)
         {
            enableFilter = new ToolItem(viewControlBar, SWT.CHECK, (navigationBack != null) ? 2 : 0);
            enableFilter.setImage(SharedIcons.IMG_FILTER);
            enableFilter.setToolTipText(String.format(i18n.tr("Show filter (%s)"), KeyStroke.normalizeDefinition("M1+F2")));
            enableFilter.addSelectionListener(new SelectionAdapter() {
               @Override
               public void widgetSelected(SelectionEvent e)
               {
                  View view = getActiveView();
                  if (view != null)
                  {
                     view.enableFilter(enableFilter.getSelection());
                  }
               }
            });
         }
         enableFilter.setSelection(view.isFilterEnabled());
         showFilterAction.setEnabled(true);
      }
      else
      {
         if (enableFilter != null)
         {
            enableFilter.dispose();
            enableFilter = null;
         }
         showFilterAction.setEnabled(false);
      }

      layout(true, true);
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

      if ((navigationBack != null) && (navigationForward != null))
      {
         navigationHistory = (view instanceof NavigationView) ? ((NavigationView)view).getNavigationHistory() : null;
         navigationForward.setEnabled((navigationHistory != null) && navigationHistory.canGoForward());
         navigationBack.setEnabled((navigationHistory != null) && navigationHistory.canGoBackward());
      }

      if (activeView != null)
         activeView.deactivate();
      if (!contextChange)
         lastViewId = (activeView != null) ? getViewId(activeView) : null;

      prepareViewActivation(view);

      activeView = view;
      if (!contextChange)
         preferredViewId = getViewId(activeView);

      view.activate();
      updateRefreshActionState();
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
         View v = (View)i.getData("view");
         if (v.getPriority() > priority)
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
            if (disposeWhenEmpty && (tabFolder.getItemCount() == 0))
            {
               Composite parent = ViewFolder.this.getParent();
               ViewFolder.this.dispose();
               parent.layout(true, true);
            }
         }
      });
      tabs.put(getViewId(view), tabItem);
      return tabItem;
   }

   /**
    * Close view
    *
    * @param view view to close
    */
   protected void closeView(View view)
   {
      view.dispose();
      CTabItem tabItem = tabs.remove(getViewId(view));
      if (tabItem != null)
         tabItem.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#extractView(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void extractView(View view)
   {
      // Update context of selected view if it is not an active one
      if (view != getActiveView())
      {
         if ((view instanceof ViewWithContext) && !ignoreContextForView(contextMenuTabItem))
         {
            if (((ViewWithContext)view).getContext() != context)
               ((ViewWithContext)view).setContext(context);
         }
      }
      super.extractView(view);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#pinView(org.netxms.nxmc.base.views.View, org.netxms.nxmc.base.views.PinLocation)
    */
   @Override
   protected void pinView(View view, PinLocation location)
   {
      // Update context of selected view if it is not an active one
      if (view != getActiveView())
      {
         if ((view instanceof ViewWithContext) && !ignoreContextForView(contextMenuTabItem))
         {
            if (((ViewWithContext)view).getContext() != context)
               ((ViewWithContext)view).setContext(context);
         }
      }
      super.pinView(view, location);
   }

   /**
    * Get current context.
    *
    * @return current context
    */
   public Object getContext()
   {
      return context;
   }

   /**
    * Set current context. Will update context in all context-sensitive views and hide/show views as necessary.
    *
    * @param context new context
    */
   public void setContext(Object context)
   {
      if (context == this.context)
         return;

      if (activeView != null)
         activeView.deactivate();

      this.context = context;
      updateViewSet();

      if (activeView != null)
      {
         if (activeView instanceof ViewWithContext)
            ((ViewWithContext)activeView).setContext(context);
         prepareViewActivation(activeView);
         activeView.activate();
      }
   }

   /**
    * Update current context. Will not trigger context change for avctive view if it is still valid after context change.
    *
    * @param context updated context
    */
   public void updateContext(Object context)
   {
      if (context == this.context)
         return;

      View currentActiveView = activeView;
      this.context = context;
      updateViewSet();

      if ((activeView != null) && (activeView != currentActiveView))
      {
         if (activeView instanceof ViewWithContext)
            ((ViewWithContext)activeView).setContext(context);
         prepareViewActivation(activeView);
         activeView.activate();
      }
   }

   /**
    * Update view set after context change.
    */
   public void updateViewSet()
   {
      contextChange = true;

      boolean invalidActiveView = false;
      for(View view : views.values())
      {
         if (!(view instanceof ViewWithContext))
            continue;

         if (((ViewWithContext)view).isValidForContext(context) && !((ViewWithContext)view).isHidden())
         {
            view.setVisible(true);
            if (!tabs.containsKey(getViewId(view)))
            {
               if (view.isCreated())
                  view.setVisible(true);
               else
                  view.create(this, tabFolder, onFilterCloseCallback);
               createViewTab(view, false);
            }
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
               if (view == activeView)
                  invalidActiveView = true;
            }
         }
      }

      CTabItem tabItem = tabFolder.getSelection();
      activeView = (tabItem != null) ? (View)tabItem.getData("view") : null;
      if (((preferredViewId != null) || (lastViewId != null)) && ((activeView == null) || !preferredViewId.equals(getViewId(activeView))))
      {
         if (showView(preferredViewId) || showView(lastViewId))
            invalidActiveView = false;
      }

      // Select first view if none were selected
      if (invalidActiveView || (tabFolder.getSelectionIndex() == -1))
      {
         tabFolder.setSelection(0);
         tabItem = tabFolder.getSelection();
         activeView = (tabItem != null) ? (View)tabItem.getData("view") : null;
      }

      contextChange = false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#getActiveView()
    */
   @Override
   protected View getActiveView()
   {
      return tabFolder.isDisposed() ? null : activeView;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#isViewActive(org.netxms.nxmc.base.views.View)
    */
   @Override
   public boolean isViewActive(View view)
   {
      return isActive && !tabFolder.isDisposed() && (activeView == view);
   }

   /**
    * Add selection listener.
    *
    * @param listener listener to add.
    */
   public void addSelectionListener(ViewFolderSelectionListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * Remove selection listener.
    * 
    * @param listener listener to remove
    */
   public void removeSelectionListener(ViewFolderSelectionListener listener)
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
      for(ViewFolderSelectionListener l : selectionListeners)
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
   public void setAllViewsAsCloseable(boolean allViewsAreCloseable)
   {
      this.allViewsAreCloseable = allViewsAreCloseable;
   }

   /**
    * Check if view folder will be disposed when all views are closed.
    *
    * @return true if view folder will be disposed when all views are closed
    */
   public boolean isDisposeWhenEmpty()
   {
      return disposeWhenEmpty;
   }

   /**
    * Set if view folder will be disposed when all views are closed.
    *
    * @param disposeWhenEmpty true to dispose view folder when all views are closed
    */
   public void setDisposeWhenEmpty(boolean disposeWhenEmpty)
   {
      this.disposeWhenEmpty = disposeWhenEmpty;
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
    * @return true if view folder uses global ID to check if view already present
    */
   public boolean isUseGlobalViewId()
   {
      return useGlobalViewId;
   }

   /**
    * Set view identification mode. If useGlobalViewId set to true, view folder will use global IDs to check if view already
    * present, otherwise base ID will be used.
    *
    * @param useGlobalViewId true to use global view ID to identify duplicate views
    */
   public void setUseGlobalViewId(boolean useGlobalViewId)
   {
      this.useGlobalViewId = useGlobalViewId;
   }

   /**
    * Get ID for given view. Depending on operation mode will return base or global view ID.
    * 
    * @param view view to get ID from
    * @return view ID according to current operation mode
    */
   private String getViewId(View view)
   {
      return useGlobalViewId ? view.getGlobalId() : view.getBaseId();
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#setFocus()
    */
   @Override
   public boolean setFocus()
   {
      View view = getActiveView();
      if ((view != null) && !view.isClientAreaDisposed())
         view.setFocus();
      else
         super.setFocus();
      return true;
   }

   /**
    * Save view state
    * 
    * @param memento memento to save state
    */
   public void saveState(Memento memento)
   {
      List<String> viewList = new ArrayList<String>();
      for(View v : views.values())
      {
         String id = v.getGlobalId();        
         viewList.add(id);         
         Memento viewConfig = new Memento();
         v.saveState(viewConfig);
         memento.set(id + ".state", viewConfig);
      }  
      memento.set("ViewFolder.Views", viewList);
   }
}
