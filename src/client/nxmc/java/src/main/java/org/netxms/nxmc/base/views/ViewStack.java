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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.helpers.NavigationHistory;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.keyboard.KeyBindingManager;
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
public class ViewStack extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(ViewStack.class);

   private I18n i18n = LocalizationHelper.getI18n(ViewStack.class);
   private Window window;
   private Perspective perspective;
   private Stack<View> views = new Stack<View>();
   private boolean contextAware = true;
   private Object context;
   private ToolBar viewList;
   private ToolBarManager viewToolBarManager;
   private ToolBar viewToolBar;
   private ToolBar viewControlBar;
   private Composite viewArea;
   private Runnable onFilterCloseCallback = null;
   private ToolItem enableFilter = null;
   private ToolItem navigationBack = null;
   private ToolItem navigationForward = null;
   private NavigationHistory navigationHistory = null;
   private KeyBindingManager keyBindingManager = new KeyBindingManager();

   /**
    * Create new view stack.
    *
    * @param window owning window
    * @param perspective owning perspective
    * @param parent parent composite
    * @param enableViewExtraction enable/disable view extraction into separate window
    * @param enableViewPinning enable/disable view extraction into "Pinboard" perspective
    * @param enableNavigationHistory enable/disable navigation history controls
    */
   public ViewStack(Window window, Perspective perspective, Composite parent, boolean enableViewExtraction, boolean enableViewPinning, boolean enableNavigationHistory)
   {
      super(parent, SWT.BORDER);
      this.window = window;
      this.perspective = perspective;

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

      enableFilter = new ToolItem(viewControlBar, SWT.CHECK);
      enableFilter.setImage(SharedIcons.IMG_FILTER);
      enableFilter.setToolTipText(String.format(i18n.tr("Show filter (%s)"), KeyStroke.normalizeDefinition("M1+F2")));
      enableFilter.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            View view = getView();
            if (view != null)
               view.enableFilter(enableFilter.getSelection());
         }
      });
      onFilterCloseCallback = new Runnable() {
         @Override
         public void run()
         {
            enableFilter.setSelection(false);
            View view = getView();
            if (view != null)
               view.enableFilter(enableFilter.getSelection());
         }
      };
      keyBindingManager.addBinding(SWT.CTRL, SWT.F2, new Action() {
         @Override
         public void run()
         {
            View view = getView();
            if ((view != null) && view.hasFilter())
            {
               view.enableFilter(!view.isFilterEnabled());
               enableFilter.setSelection(view.isFilterEnabled());
            }
         }
      });

      ToolItem refreshView = new ToolItem(viewControlBar, SWT.PUSH);
      refreshView.setImage(SharedIcons.IMG_REFRESH);
      refreshView.setToolTipText(i18n.tr("Refresh (F5)"));
      refreshView.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            View view = getView();
            if (view != null)
            {
               view.refresh();
            }
         }
      });

      if (enableViewPinning)
      {
         ToolItem pinView = new ToolItem(viewControlBar, SWT.PUSH);
         pinView.setImage(SharedIcons.IMG_PIN);
         pinView.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               View view = getView();
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
               View view = getView();
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
            View view = getView();
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
         public void widgetDisposed(DisposeEvent arg0)
         {
            for(View view : views)
               view.dispose();
            views.clear();
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
         logger.debug("Existing view " + v.getGlobalId() + " replaced by view " + view.getGlobalId());
         v.dispose();
      }
      views.clear();

      if (view != null)
      {
         pushViewInternal(view);
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
      if (view == null)
         return;

      pushViewInternal(view);
      addViewListElement(view);
   }

   /**
    * Push view into existing stack (internal implementation).
    *
    * @param view view to push
    */
   private void pushViewInternal(View view)
   {
      views.push(view);
      view.create(window, perspective, viewArea, onFilterCloseCallback);
      view.getViewArea().setSize(viewArea.getSize());
      if (contextAware && (view instanceof ViewWithContext))
         ((ViewWithContext)view).setContext(context);

      viewToolBarManager.removeAll();
      view.fillLocalToolbar(viewToolBarManager);
      viewToolBarManager.update(true);

      enableFilter.setSelection(view.isFilterEnabled());
      enableFilter.setEnabled(view.hasFilter());

      navigationHistory = (view instanceof NavigationView) ? ((NavigationView)view).getNavigationHistory() : null;
      if (navigationForward != null)
         navigationForward.setEnabled((navigationHistory != null) && navigationHistory.canGoForward());
      if (navigationBack != null)
         navigationBack.setEnabled((navigationHistory != null) && navigationHistory.canGoBackward());

      view.activate();
   }

   /**
    * Pop view (will dispose current view and switch to next in stack). Thes method may return failure if view is modified and user
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

      view = getView(); // New current view
      if (view == null)
         return true;

      viewToolBarManager.removeAll();
      view.fillLocalToolbar(viewToolBarManager);
      viewToolBarManager.update(true);

      enableFilter.setSelection(view.isFilterEnabled());
      enableFilter.setEnabled(view.hasFilter());

      navigationHistory = (view instanceof NavigationView) ? ((NavigationView)view).getNavigationHistory() : null;
      if (navigationForward != null)
         navigationForward.setEnabled((navigationHistory != null) && navigationHistory.canGoForward());
      if (navigationBack != null)
         navigationBack.setEnabled((navigationHistory != null) && navigationHistory.canGoBackward());

      view.getViewArea().setSize(viewArea.getSize());
      view.activate();
      return true;
   }

   /**
    * Switch to specific view in stack (will dispose all views on top of given view).
    *
    * @param view view to switch to
    */
   public void switchToView(View view)
   {
      if (!views.contains(view))
         return;

      while(getView() != view)
         if (!popView())
            break;
   }

   /**
    * Get current view.
    *
    * @return current view or null
    */
   public View getView()
   {
      return views.empty() ? null : views.peek();
   }

   /**
    * Navigate back
    */
   private void navigateBack()
   {
      if ((navigationHistory == null) || !navigationHistory.canGoBackward())
         return;
      navigationHistory.lock();
      ((NavigationView)getView()).setSelection(navigationHistory.back());
      navigationHistory.unlock();
      navigationBack.setEnabled(navigationHistory.canGoBackward());
      navigationForward.setEnabled(navigationHistory.canGoForward());
   }

   /**
    * Navigate forward
    */
   private void navigateForward()
   {
      if ((navigationHistory == null) || !navigationHistory.canGoForward())
         return;
      navigationHistory.lock();
      ((NavigationView)getView()).setSelection(navigationHistory.forward());
      navigationHistory.unlock();
      navigationBack.setEnabled(navigationHistory.canGoBackward());
      navigationForward.setEnabled(navigationHistory.canGoForward());
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
    * Set context for containing view
    *
    * @param context
    */
   public void setContext(Object context)
   {
      this.context = context;
      View view = getView();
      if (contextAware && (view != null) && (view instanceof ViewWithContext))
         ((ViewWithContext)view).setContext(context);
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
      View view = getView();
      if (view != null)
         view.setFocus();
      else
         viewArea.setFocus();
      return true;
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

      View view = getView();
      if (view != null)
         view.processKeyStroke(ks);
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
            switchToView(view);
         }
      });
   }
}
