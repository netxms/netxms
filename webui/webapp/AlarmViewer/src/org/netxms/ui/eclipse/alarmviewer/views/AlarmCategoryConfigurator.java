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
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
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
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCategoryList;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmCategoryListFilter;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.widgets.FilterText;

public class AlarmCategoryConfigurator extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.alarmviewer.view.alarm_category_configurator"; //$NON-NLS-1$
   private static final String TABLE_CONFIG_PREFIX = "AlarmCategoryConfigurator"; //$NON-NLS-1$

   private AlarmCategoryList dataView;
   private Action actionRefresh;
   private Action actionShowFilter;
   private AlarmCategoryListFilter filter;
   private FilterText filterText;
   private boolean initShowfilter = true;

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
      
      // Create filter area
      filterText = new FilterText(parent, SWT.NONE, null, true);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      dataView = new AlarmCategoryList(this, parent, SWT.NONE, TABLE_CONFIG_PREFIX, true);
      filter = new AlarmCategoryListFilter();
      dataView.getViewer().addFilter(filter);
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      dataView.setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
      
      dataView.getViewer().addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            dataView.editCategory();
         }
      });
      
      // Get filter settings
      final IDialogSettings ds = Activator.getDefault().getDialogSettings();
      initShowfilter = safeCast(ds.get(TABLE_CONFIG_PREFIX + "initShowfilter"), ds.getBoolean(TABLE_CONFIG_PREFIX + "initShowfilter"),
            initShowfilter);

      // Set initial focus to filter input line
      if (initShowfilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
      
      createActions();
      contributeToActionBars();
      activateContext();
   }
   
   /**
    * @param settings String
    * @param settings Boolean
    * @param defval
    * @return
    */
   private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      initShowfilter = enable;
      filterText.setVisible(initShowfilter);
      FormData fd = (FormData)dataView.getViewer().getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      dataView.getParent().layout();
      if (enable)
         filterText.setFocus();
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
   }

   /**
    * @return the initShowfilter
    */
   public boolean isinitShowfilter()
   {
      return initShowfilter;
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   public void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Get filter text
    * 
    * @return Current filter text
    */
   public String getFilterText()
   {
      return filterText.getText();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      dataView.getViewer().refresh(false);
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
      manager.add(actionShowFilter);
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
      manager.add(actionShowFilter);
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
      
      actionShowFilter = new Action() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setText("Show filter");
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(initShowfilter);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.alarmviewer.commands.show_filter"); //$NON-NLS-1$

      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(),
            new ActionHandler(actionShowFilter));
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
