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
package org.netxms.ui.eclipse.alarmviewer.widgets;

import java.util.HashMap;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.AlarmCategory;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.views.helpers.AlarmCategoryLabelProvider;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmCategoryListComparator;
import org.netxms.ui.eclipse.alarmviewer.widgets.helpers.AlarmCategoryListFilter;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class AlarmCategoryList extends Composite implements SessionListener
{
   public static final String JOB_FAMILY = "AlarmCategorySelectorJob"; //$NON-NLS-1$

   // Columns
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_DESCRIPTION = 2;

   private boolean initShowfilter = true;
   private AlarmCategoryListFilter filter;
   private FilterText filterText;
   private Action actionShowFilter;
   private Action actionAddCategory;
   private Action actionEditCategory;
   private Action actionDeleteCategory;
   private NXCSession session;
   private IStructuredSelection selection;
   private SortableTableViewer viewer;
   private ViewPart viewPart;
   private HashMap<Long, AlarmCategory> alarmCategories;

   public AlarmCategoryList(ViewPart viewPart, Composite parent, int style, final String configPrefix, boolean showFilter)
   {
      this(parent, style, configPrefix, showFilter);
      this.viewPart = viewPart;
   }

   /**
    * Create alarm category widget
    * 
    * @param viewPart owning view part
    * @param parent parent widget
    * @param style style
    * @param configPrefix configuration prefix for saving/restoring viewer settings
    */
   public AlarmCategoryList(Composite parent, int style, final String configPrefix, boolean showCloseButton)
   {
      super(parent, style);

      session = (NXCSession)ConsoleSharedData.getSession();

      FormLayout formLayout = new FormLayout();
      setLayout(formLayout);

      // Create filter area
      filterText = new FilterText(this, SWT.NONE, null, showCloseButton);
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

      final IDialogSettings ds = Activator.getDefault().getDialogSettings();

      // Setup table columns
      final String[] names = { "ID", "Name", "Description" };
      final int[] widths = { 50, 200, 200 };
      viewer = new SortableTableViewer(this, names, widths, 1, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.H_SCROLL
            | SWT.V_SCROLL);
      viewer.setLabelProvider(new AlarmCategoryLabelProvider());
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new AlarmCategoryListComparator());
      filter = new AlarmCategoryListFilter();
      viewer.addFilter(filter);

      WidgetHelper.restoreTableViewerSettings(viewer, ds, configPrefix);

      addListener(SWT.Resize, new Listener() {
         @Override
         public void handleEvent(Event e)
         {
            viewer.getControl().setBounds(AlarmCategoryList.this.getClientArea());
         }
      });

      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, ds, configPrefix);
            ds.put(configPrefix + "initShowfilter", initShowfilter);
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange();
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getControl().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      // Get filter settings
      initShowfilter = safeCast(ds.get(configPrefix + "initShowfilter"), ds.getBoolean(configPrefix + "initShowfilter"),
            initShowfilter);

      // Set initial focus to filter input line
      if (initShowfilter)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

      createActions();
      createPopupMenu();
      filterText.setCloseAction(actionShowFilter);

      refreshView();
      session.addListener(this);
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
    * Internal handler for selection change
    */
   private void onSelectionChange()
   {
      selection = (IStructuredSelection)getSelection();
      actionEditCategory.setEnabled(selection.size() == 1);
      actionDeleteCategory.setEnabled(selection.size() > 0);
   }

   /**
    * Refresh dialog
    */
   public void refreshView()
   {
      new ConsoleJob("Open alarm category list", null, Activator.PLUGIN_ID, JOB_FAMILY) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<AlarmCategory> list = session.getAlarmCategories();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  alarmCategories = new HashMap<Long, AlarmCategory>(list.size());
                  for(final AlarmCategory c : list)
                  {
                     alarmCategories.put(c.getId(), c);
                  }
                  viewer.setInput(alarmCategories.values().toArray());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot open alarm category list";
         }
      }.start();
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
      FormData fd = (FormData)viewer.getControl().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      layout();
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
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
      viewer.refresh(false);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAddCategory = new Action() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            createCategory();
         }
      };
      actionAddCategory.setText("Add category");
      actionAddCategory.setImageDescriptor(SharedIcons.ADD_OBJECT);

      actionEditCategory = new Action() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            editCategory();
         }
      };
      actionEditCategory.setText("Edit category");
      actionEditCategory.setImageDescriptor(SharedIcons.EDIT);
      actionEditCategory.setEnabled(false);

      actionDeleteCategory = new Action() {
         /*
          * (non-Javadoc)
          * 
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            deleteCategory();
         }
      };
      actionDeleteCategory.setText("Delete category");
      actionDeleteCategory.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      actionDeleteCategory.setEnabled(false);

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
   }

   /**
    * Create pop-up menu
    */
   public void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      if (viewPart != null)
         viewPart.getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionAddCategory);
      manager.add(actionDeleteCategory);
      manager.add(actionEditCategory);
      manager.add(new Separator());
      manager.add(actionShowFilter);
   }

   /**
    * Create new category
    */
   public void createCategory()
   {
      AlarmCategory category = new AlarmCategory(-1);
      PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, category);
      if (dlg != null)
      {
         dlg.getShell().setText("Properties");
         dlg.open();
      }
   }

   /**
    * Edit alarm category
    */
   public void editCategory()
   {
      if (selection.isEmpty())
         return;

      PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, selection.getFirstElement());
      if (dlg != null)
      {
         dlg.getShell().setText("Properties");
         dlg.open();
      }
   }

   /**
    * Delete alarm category
    */
   private void deleteCategory()
   {
      final IStructuredSelection selection = (IStructuredSelection)getSelection();
      if (selection.isEmpty())
         return;

      final String message = (selection.size() == 1) ? "Do you really wish to delete the selected category?"
            : "Do you really wish to delete the selected categories?";
      if (!MessageDialogHelper.openQuestion(getShell(), "Confirm category deletion", message))
         return;

      new ConsoleJob("Delete alarm category database entries", viewPart, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
               session.deleteAlarmCategory(((AlarmCategory)o).getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete alarm category database entry";
         }
      }.start();
   }

   /**
    * @return Show filter action
    */
   public Action getShowFilterAction()
   {
      return actionShowFilter;
   }

   /**
    * @return Add category action
    */
   public Action getAddCategoryAction()
   {
      return actionAddCategory;
   }

   /**
    * @return Edit category action
    */
   public Action getEditCategoryAction()
   {
      return actionEditCategory;
   }

   /**
    * @return Delete category action
    */
   public Action getDeleteCategoryAction()
   {
      return actionDeleteCategory;
   }

   /**
    * Get selection provider of alarm list
    * 
    * @return
    */
   public ISelection getSelection()
   {
      return viewer.getSelection();
   }

   /**
    * Get underlying table viewer.
    * 
    * @return
    */
   public TableViewer getViewer()
   {
      return viewer;
   }
   
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.ALARM_CATEGORY_UPDATED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  AlarmCategory oldCat = alarmCategories.get(n.getSubCode());
                  if (oldCat != null)
                  {
                     oldCat.setAll((AlarmCategory)n.getObject());
                     viewer.update(oldCat, null);
                  }
                  else
                  {
                     alarmCategories.put(n.getSubCode(), (AlarmCategory)n.getObject());
                     viewer.setInput(alarmCategories.values().toArray());
                  }
               }
            });
            break;
         case SessionNotification.ALARM_CATEGORY_DELETED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  alarmCategories.remove(n.getSubCode());
                  viewer.setInput(alarmCategories.values().toArray());
               }
            });
            break;
      }
   }

   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
   }
}
