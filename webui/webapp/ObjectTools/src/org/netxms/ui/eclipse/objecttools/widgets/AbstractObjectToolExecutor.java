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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.TextConsole;

/**
 * Base class for object tool execution widget
 */
public abstract class AbstractObjectToolExecutor extends Composite
{   
   protected TextConsole console;
   protected ToolBar toolBar;
   protected Composite result;
   protected ViewPart viewPart;
   private ToolItem restertItem;

   private Action actionClear;
   private Action actionScrollLock;
   private Action actionCopy;
   private Action actionSelectAll;
   private Action actionRestart;
   
   /**
    * Constructor for object tool executor
    * 
    * @param parent
    * @param style
    * @param options
    */
   public AbstractObjectToolExecutor(Composite parent, int style, ViewPart viewPart)
   {
      super(parent, style);  
      this.viewPart = viewPart;

      result = new Composite(parent, SWT.NONE); 
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginTop = 0;
      layout.marginBottom = 0;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      result.setLayout(layout);
      
      toolBar = new ToolBar(result, SWT.FLAT| SWT.HORIZONTAL);
      toolBar.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      
      console = new TextConsole(result, SWT.NONE);
      console.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            actionCopy.setEnabled(console.canCopy());
         }
      });
      console.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      result.setVisible(false);  

      createActions(viewPart);
      createToolbarItems();
      createPopupMenu();
   }
   
   /**
    * Create actions
    */
   protected void createActions(ViewPart viewPart)
   {
      final IHandlerService handlerService = (IHandlerService)viewPart.getSite().getService(IHandlerService.class);
      
      actionClear = new Action(Messages.get().LocalCommandResults_ClearConsole, SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            console.clear();
         }
      };
      actionClear.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.clear_output"); //$NON-NLS-1$
      handlerService.activateHandler(actionClear.getActionDefinitionId(), new ActionHandler(actionClear));

      actionScrollLock = new Action(Messages.get().LocalCommandResults_ScrollLock, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            console.setAutoScroll(!actionScrollLock.isChecked());
         }
      };
      actionScrollLock.setImageDescriptor(Activator.getImageDescriptor("icons/scroll_lock.gif")); //$NON-NLS-1$
      actionScrollLock.setChecked(false);
      actionScrollLock.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.scroll_lock"); //$NON-NLS-1$
      handlerService.activateHandler(actionScrollLock.getActionDefinitionId(), new ActionHandler(actionScrollLock));
      
      actionCopy = new Action(Messages.get().LocalCommandResults_Copy) {
         @Override
         public void run()
         {
            console.copy();
         }
      };
      actionCopy.setEnabled(false);
      actionCopy.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.copy"); //$NON-NLS-1$
      handlerService.activateHandler(actionCopy.getActionDefinitionId(), new ActionHandler(actionCopy));
      
      actionSelectAll = new Action(Messages.get().LocalCommandResults_SelectAll) {
         @Override
         public void run()
         {
            console.selectAll();
         }
      };
      actionSelectAll.setActionDefinitionId("org.netxms.ui.eclipse.objecttools.commands.select_all"); //$NON-NLS-1$
      handlerService.activateHandler(actionSelectAll.getActionDefinitionId(), new ActionHandler(actionSelectAll));      

      actionRestart = new Action(Messages.get().LocalCommandResults_Restart, SharedIcons.RESTART) {
         @Override
         public void run()
         {
            execute();
         }
      };
      actionRestart.setEnabled(false);
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
      Menu menu = menuMgr.createContextMenu(console);
      console.setMenu(menu);
   }
   
   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionRestart);
      manager.add(new Separator());
      manager.add(actionClear);
      manager.add(actionScrollLock);
      manager.add(new Separator());
      manager.add(actionSelectAll);
      manager.add(actionCopy);
   }
   
   /**
    * Is restart action and item should be enabled
    * 
    * @param enabled true to enable
    */
   protected void enableRestartAction(boolean enabled)
   {
      actionRestart.setEnabled(enabled);
      restertItem.setEnabled(enabled);
   }

   /**
    * Create toolbar items
    */
   protected void createToolbarItems()
   {
      ToolItem item = new ToolItem(toolBar, SWT.PUSH);
      item.setText(Messages.get().LocalCommandResults_ClearConsole);
      item.setToolTipText("Clear result");
      item.setImage(SharedIcons.IMG_CLEAR_LOG);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionClear.run();
         }
      });
      
      item = new ToolItem(toolBar, SWT.PUSH);
      item.setText(Messages.get().LocalCommandResults_ScrollLock);
      item.setToolTipText("Lock the scrolling");
      item.setImage(Activator.getImageDescriptor("icons/scroll_lock.gif").createImage());
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionScrollLock.run();
         }
      });
      
      item = new ToolItem(toolBar, SWT.PUSH);
      item.setText(Messages.get().LocalCommandResults_Copy);
      item.setToolTipText("Copy selected text");
      item.setImage(SharedIcons.IMG_COPY);
      item.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionCopy.run();
         }
      });
      
      restertItem = new ToolItem(toolBar, SWT.PUSH);
      restertItem.setText(Messages.get().LocalCommandResults_Restart);
      restertItem.setToolTipText("Rerun action");
      restertItem.setImage(SharedIcons.IMG_RESTART);
      restertItem.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            actionRestart.run();
         }
      });
   }

   /**
    * Execute command
    * 
    * @param result 
    */
   public abstract void execute();

   /**
    * Hide element
    */
   public void hide() 
   {
      result.setVisible(true);
      result.setSize(0, 0);
      result.getParent().layout(true, true);      
   }

   /**
    * Show element
    */
   public void show()
   {
      result.setVisible(true);      
      result.setSize(result.getParent().getSize());
      result.getParent().layout(true, true);
   } 
}
