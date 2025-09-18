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

import java.util.Stack;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.keyboard.KeyStroke;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * View stack (single view or multiple views on top of each other)
 */
public class ViewStack extends ViewContainer
{
   private static final Logger logger = LoggerFactory.getLogger(ViewStack.class);

   private final I18n i18n = LocalizationHelper.getI18n(ViewStack.class);

   private Stack<View> views = new Stack<View>();
   private boolean contextAware = true;
   private ToolBar viewList;
   private Composite viewArea;

   /**
    * Create new view stack.
    *
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param enableViewExtraction enable/disable view extraction into separate window
    * @param enableViewPinning enable/disable view extraction into "pinboard" perspective
    * @param enableNavigationHistory enable/disable navigation history controls
    */
   public ViewStack(Window window, Perspective perspective, Composite parent, boolean enableViewExtraction, boolean enableViewPinning, boolean enableNavigationHistory)
   {
      super(window, perspective, parent, SWT.BORDER);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 0;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.numColumns = 3;
      setLayout(layout);

      viewList = new ToolBar(this, SWT.FLAT | SWT.WRAP | SWT.LEFT);
      viewList.setFont(JFaceResources.getBannerFont());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewList.setLayoutData(gd);

      viewToolBarManager = new ToolBarManager(SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      viewToolBar = viewToolBarManager.createControl(this);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.grabExcessHorizontalSpace = false;
      viewToolBar.setLayoutData(gd);

      viewControlBar = new ToolBar(this, SWT.FLAT | SWT.WRAP | SWT.RIGHT);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.grabExcessHorizontalSpace = false;
      viewControlBar.setLayoutData(gd);

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
               popView();
         }
      });

      Label separator = new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      separator.setLayoutData(gd);

      viewArea = new Composite(this, SWT.NONE);
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      viewArea.setLayoutData(gd);
      viewArea.addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            View view = getActiveView();
            if (view != null)
               view.getViewArea().setSize(viewArea.getSize());
         }

         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            for(View view : views)
               view.dispose();
            views.clear();
         }
      });
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
    * Set view to show in this stack. Will dispose any existing view.
    *
    * @param view view to show
    */
   public void setView(View view)
   {
      for(View v : views)
      {
         logger.debug("Existing view " + v.getGlobalId() + " replaced by view " + ((view != null) ? view.getGlobalId() : "(null)"));
         v.dispose();
      }
      views.clear();

      if (view != null)
      {
         pushViewInternal(view, false);
      }
      else
      {
         navigationHistory = null;
         if (navigationForward != null)
            navigationForward.setEnabled(false);
         if (navigationBack != null)
            navigationBack.setEnabled(false);
      }
      updateViewList();
   }

   /**
    * Push view into existing stack.
    *
    * @param view view to push
    */
   public void pushView(View view)
   {
      pushView(view, false);
   }

   /**
    * Push view into existing stack.
    *
    * @param view view to push
    * @param activate true to activate view
    */
   public void pushView(View view, boolean activate)
   {
      if (view == null)
         return;

      pushViewInternal(view, activate);
      addViewListElement(view);
   }

   /**
    * Push view into existing stack (internal implementation).
    *
    * @param view view to push
    * @param activate true to activate view
    */
   private void pushViewInternal(View view, boolean activate)
   {
      views.push(view);
      view.create(this, viewArea, onFilterCloseCallback);
      if (contextAware && (view instanceof ViewWithContext))
         ((ViewWithContext)view).setContext(context);
      activateView(view);
      updateRefreshActionState();
      if (activate)
         view.setFocus();
   }

   /**
    * Pop view (will dispose current view and switch to next in stack). This method may return failure if view is modified and user
    * selected "cancel" as an option.
    * 
    * @return true if view at the top successfully removed
    */
   public boolean popView()
   {
      if (views.empty())
         return true;

      View view = views.pop();
      if ((view instanceof ConfigurationView) && ((ConfigurationView)view).isModified())
      {
         int choice = MessageDialogHelper.openQuestionWithCancel(view.getWindow().getShell(), i18n.tr("Unsaved Changes"), ((ConfigurationView)view).getSaveOnExitPrompt());
         if (choice == MessageDialogHelper.CANCEL)
         {
            views.push(view);
            return false; // Do not change view
         }
         if (choice == MessageDialogHelper.YES)
         {
            ((ConfigurationView)view).save();
         }
      }
      view.dispose();
      int count = viewList.getItemCount();
      viewList.getItem(--count).dispose();
      if (count > 0)
         viewList.getItem(--count).dispose(); // Dispose ">" separator between view names

      updateRefreshActionState();

      view = getActiveView(); // New current view
      if (view == null)
         return true;

      activateView(view);
      return true;
   }

   /**
    * Activate view after push or pop
    *
    * @param view view to activate
    */
   private void activateView(View view)
   {
      updateViewToolBar(view);

      boolean controlBarChanged = updateViewMenu(view);

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
                     view.enableFilter(enableFilter.getSelection());
               }
            });
            controlBarChanged = true;
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
            controlBarChanged = true;
         }
         showFilterAction.setEnabled(false);
      }

      if (controlBarChanged)
         layout(true, true);

      navigationHistory = (view instanceof NavigationView) ? ((NavigationView)view).getNavigationHistory() : null;
      if (navigationForward != null)
         navigationForward.setEnabled((navigationHistory != null) && navigationHistory.canGoForward());
      if (navigationBack != null)
         navigationBack.setEnabled((navigationHistory != null) && navigationHistory.canGoBackward());

      view.getViewArea().setSize(viewArea.getSize());
      view.activate();
   }

   /**
    * Switch to specific view in stack (will dispose all views on top of given view).
    *
    * @param view view to switch to
    * @param focus if true set focus on selected view
    */
   public void switchToView(View view, boolean focus)
   {
      if (!views.contains(view))
         return;

      while(getActiveView() != view)
         if (!popView())
            break;

      if (getActiveView() == view)
         view.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#getActiveView()
    */
   @Override
   protected View getActiveView()
   {
      return views.empty() ? null : views.peek();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewContainer#isViewActive(org.netxms.nxmc.base.views.View)
    */
   @Override
   public boolean isViewActive(View view)
   {
      return isActive && !views.empty() && (views.peek() == view);
   }

   /**
    * Get all views in this stack.
    *
    * @return all views in this stack
    */
   public View[] getAllViews()
   {
      return views.toArray(View[]::new);
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
    * Set context for containing view
    *
    * @param context new context
    */
   public void setContext(Object context)
   {
      if (context == this.context)
         return;

      this.context = context;
      View view = getActiveView();
      if (contextAware && (view != null) && (view instanceof ViewWithContext))
      {
         ((ViewWithContext)view).setContext(context);
         updateViewList();
      }
   }

   /**
    * Update context.
    *
    * @param context updated context
    */
   public void updateContext(Object context)
   {
      this.context = context;
   }

   /**
    * Check if this view stack is context aware and will update view's context.
    * 
    * @return true if this view stack is context aware and will update view's context
    */
   public boolean isContextAware()
   {
      return contextAware;
   }

   /**
    * Set if this view stack should be context aware and update view's context.
    *
    * @param contextAware true if this view stack should be context aware and update view's context
    */
   public void setContextAware(boolean contextAware)
   {
      this.contextAware = contextAware;
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
         viewArea.setFocus();
      return true;
   }

   /**
    * Update view list
    */
   private void updateViewList()
   {
      for(ToolItem i : viewList.getItems())
         i.dispose();

      for(final View v : views)
         addViewListElement(v);
   }

   /**
    * Add new element to view list.
    *
    * @param view view to add
    */
   private void addViewListElement(final View view)
   {
      if (viewList.getItemCount() > 0)
      {
         ToolItem i = new ToolItem(viewList, SWT.PUSH);
         i.setText(">");
         i.setEnabled(false);
      }
      ToolItem i = new ToolItem(viewList, SWT.PUSH);
      i.setText(view.getFullName());
      i.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            switchToView(view, true);
         }
      });
   }
}
