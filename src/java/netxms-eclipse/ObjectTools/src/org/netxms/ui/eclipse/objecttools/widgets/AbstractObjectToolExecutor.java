/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Soultions
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
package org.netxms.ui.eclipse.objecttools.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objects.ObjectContext;
import org.netxms.ui.eclipse.objecttools.widgets.helpers.ExecutorStateChangeListener;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.TextConsole;

/**
 * Base class for object tool execution widget
 */
public abstract class AbstractObjectToolExecutor extends Composite
{   
   protected ObjectContext objectContext;
   protected TextConsole console;
   protected ToolBarManager toolBarManager;
   protected ToolBar toolBar;
   protected ViewPart viewPart;

   private Font headerFont;
   private ActionSet actions;
   private Set<ExecutorStateChangeListener> stateChangeListeners = new HashSet<ExecutorStateChangeListener>();
   private boolean running = false;
   private boolean restartEnabled = false;
   private boolean terminateEnabled = false;
   private boolean autoScroll = true;

   /**
    * Constructor for object tool executor. Executor control will be created hidden.
    * 
    * @param parent
    * @param style
    * @param options
    */
   public AbstractObjectToolExecutor(Composite parent, ViewPart viewPart, ObjectContext objectContext, ActionSet actions)
   {
      super(parent, SWT.NONE);
      this.viewPart = viewPart;
      this.objectContext = objectContext;
      this.actions = actions;

      setVisible(false);

      FontData fd = getFont().getFontData()[0];
      fd.setStyle(fd.getStyle() | SWT.BOLD);
      headerFont = new Font(getDisplay(), fd);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            headerFont.dispose();
         }
      });

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginTop = 0;
      layout.marginBottom = 0;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 3;
      setLayout(layout);
      
      Label objectName = new Label(this, SWT.LEFT);
      objectName.setFont(headerFont);
      objectName.setText(objectContext.object.getObjectName());
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      objectName.setLayoutData(gd);

      Label separator = new Label(this, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      gd.heightHint = 5;
      separator.setLayoutData(gd);

      toolBar = new ToolBar(this, SWT.FLAT | SWT.HORIZONTAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalIndent = WidgetHelper.DIALOG_SPACING;
      toolBar.setLayoutData(gd);
      toolBarManager = new ToolBarManager(toolBar);
      
      console = new TextConsole(this, SWT.NONE);
      console.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            actions.actionCopy.setEnabled(console.canCopy());
         }
      });
      console.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 3, 1));
      
      fillToolBar();
      createPopupMenu();

      toolBarManager.update(true);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(console.getConsoleControl());
      console.getConsoleControl().setMenu(menu);
   }
   
   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   private void fillContextMenu(final IMenuManager manager)
   {
      if (isTerminateSupported())
         manager.add(actions.actionTerminate);
      manager.add(actions.actionRestart);
      manager.add(new Separator());
      manager.add(actions.actionClear);
      manager.add(actions.actionScrollLock);
      manager.add(new Separator());
      manager.add(actions.actionSelectAll);
      manager.add(actions.actionCopy);
   }
   
   /**
    * Create toolbar items
    */
   private void fillToolBar()
   {
      if (isTerminateSupported())
         toolBarManager.add(actions.actionTerminate);
      toolBarManager.add(actions.actionRestart);
      toolBarManager.add(new Separator());
      toolBarManager.add(actions.actionClear);
      toolBarManager.add(actions.actionScrollLock);
   }

   /**
    * Enable/disable "restart" action
    * 
    * @param enabled true to enable action
    */
   protected void enableRestart(boolean enabled)
   {
      restartEnabled = enabled;
      actions.actionRestart.setEnabled(enabled);
   }

   /**
    * Enable/disable "terminate" action
    * 
    * @param enabled true to enable action
    */
   protected void enableTerminate(boolean enabled)
   {
      terminateEnabled = enabled;
      actions.actionTerminate.setEnabled(enabled);
   }

   /**
    * Execute command
    * 
    * @param result 
    */
   public abstract void execute();

   /**
    * Terminate running command. Default implementation does nothing.
    */
   public void terminate()
   {
   }

   /**
    * Check if "terminate" action is supported. Default implementation returns false.
    * 
    * @return true if "terminate" action is supported
    */
   protected boolean isTerminateSupported()
   {
      return false;
   }

   /**
    * Get running state.
    * 
    * @return current running state
    */
   public boolean isRunning()
   {
      return running;
   }

   /**
    * Set running state.
    * 
    * @param running new running state
    */
   protected void setRunning(boolean running)
   {
      this.running = running;
      enableRestart(!running);
      enableTerminate(isTerminateSupported() && running);
      for(ExecutorStateChangeListener l : stateChangeListeners)
         l.runningStateChanged(running);
   }

   /**
    * Select all text in output console
    */
   public void selectAll()
   {
      console.selectAll();
   }

   /**
    * Copy selected output text to clipboard
    */
   public void copyOutput()
   {
      console.copy();
   }

   /**
    * Clear output
    */
   public void clearOutput()
   {
      console.clear();
   }

   /**
    * Enable/disable automatic scrolling.
    *
    * @param enabled true to enable
    */
   public void setAutoScroll(boolean enabled)
   {
      autoScroll = enabled;
      console.setAutoScroll(enabled);
   }

   /**
    * Hide this executor
    */
   public void hide()
   {
      setVisible(false);
   }

   /**
    * Show this executor and place it on top of Z-order
    */
   public void show()
   {
      actions.actionCopy.setEnabled(console.canCopy());
      actions.actionRestart.setEnabled(restartEnabled);
      actions.actionTerminate.setEnabled(terminateEnabled);
      actions.actionScrollLock.setChecked(!autoScroll);
      setVisible(true);
      moveAbove(null);
   }

   /**
    * Get associated object
    *
    * @return
    */
   public AbstractObject getObject()
   {
      return objectContext.object;
   }

   /**
    * Add state change listener.
    * 
    * @param listener state change listener
    */
   public void addStateChangeListener(ExecutorStateChangeListener listener)
   {
      stateChangeListeners.add(listener);
   }

   /**
    * Remove state change listener.
    * 
    * @param listener state change listener
    */
   public void removeStateChangeListener(ExecutorStateChangeListener listener)
   {
      stateChangeListeners.remove(listener);
   }

   /**
    * Action set for executor
    */
   public static class ActionSet
   {
      public Action actionClear;
      public Action actionScrollLock;
      public Action actionCopy;
      public Action actionSelectAll;
      public Action actionRestart;
      public Action actionTerminate;
   }
}
