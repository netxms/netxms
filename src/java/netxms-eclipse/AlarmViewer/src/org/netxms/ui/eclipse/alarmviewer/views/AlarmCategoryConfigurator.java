/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCategoryList;

public class AlarmCategoryConfigurator extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.alarmviewer.view.alarm_category_configurator"; //$NON-NLS-1$
   private static final String TABLE_CONFIG_PREFIX = "AlarmCategoryConfigurator"; //$NON-NLS-1$

   private AlarmCategoryList dataView;
   private Action actionRefresh;

   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      setPartName("Alarm Category Configuration");
   }

   @Override
   public void createPartControl(Composite parent)
   {
      FormLayout formLayout = new FormLayout();
      parent.setLayout(formLayout);

      dataView = new AlarmCategoryList(this, parent, SWT.NONE, TABLE_CONFIG_PREFIX, true);
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      dataView.setLayoutData(fd);
      
      dataView.getViewer().addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            dataView.editCategory();
         }
      });
      
      createActions();
      contributeToActionBars();
      activateContext();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.alarmviewer.context.AlarmCategoryConfigurator"); //$NON-NLS-1$
      }
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(dataView.getAddCategoryAction());
      manager.add(dataView.getDeleteCategoryAction());
      manager.add(dataView.getEditCategoryAction());
      manager.add(dataView.getShowFilterAction());
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(dataView.getAddCategoryAction());
      manager.add(dataView.getDeleteCategoryAction());
      manager.add(dataView.getEditCategoryAction());
      manager.add(dataView.getShowFilterAction());
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionRefresh = new RefreshAction() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            dataView.refreshView();
         }
      };

      handlerService.activateHandler(dataView.getShowFilterAction().getActionDefinitionId(),
            new ActionHandler(dataView.getShowFilterAction()));
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      dataView.setFocus();
   }
}
