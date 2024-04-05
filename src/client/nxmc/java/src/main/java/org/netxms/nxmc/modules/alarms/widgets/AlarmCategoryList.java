/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 RadenSolutions
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
package org.netxms.nxmc.modules.alarms.widgets;

import java.util.HashMap;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.AlarmCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.editors.AlarmCategoryEditor;
import org.netxms.nxmc.modules.alarms.propertypages.AccessControl;
import org.netxms.nxmc.modules.alarms.propertypages.General;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmCategoryLabelProvider;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmCategoryListComparator;
import org.netxms.nxmc.modules.alarms.widgets.helpers.AlarmCategoryListFilter;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm category list
 */
public class AlarmCategoryList extends Composite implements SessionListener
{
   private I18n i18n = LocalizationHelper.getI18n(AlarmCategoryList.class);

   // Columns
   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_DESCRIPTION = 2;

   private AlarmCategoryListFilter filter;
   private Action actionAddCategory;
   private Action actionEditCategory;
   private Action actionDeleteCategory;
   private NXCSession session;
   private IStructuredSelection selection;
   private SortableTableViewer viewer;
   private View viewPart;
   private HashMap<Long, AlarmCategory> alarmCategories;

   /**
    * @param viewPart
    * @param parent
    * @param style
    * @param configPrefix
    * @param showFilter
    */
   public AlarmCategoryList(View viewPart, Composite parent, int style, final String configPrefix, boolean showFilter)
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

      session = Registry.getSession();
      setLayout(new FillLayout());

      // Setup table columns
      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Description") };
      final int[] widths = { 50, 200, 200 };
      viewer = new SortableTableViewer(this, names, widths, 1, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI | SWT.H_SCROLL
            | SWT.V_SCROLL);
      viewer.setLabelProvider(new AlarmCategoryLabelProvider());
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new AlarmCategoryListComparator());
      filter = new AlarmCategoryListFilter();
      viewer.addFilter(filter);

      WidgetHelper.restoreTableViewerSettings(viewer, configPrefix);

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
            WidgetHelper.saveTableViewerSettings(viewer, configPrefix);
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            onSelectionChange();
         }
      });

      createActions();
      createPopupMenu();

      refreshView();
      session.addListener(this);
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
      new Job(i18n.tr("Open alarm category list"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot open alarm category list");
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAddCategory = new Action() {
         @Override
         public void run()
         {
            createCategory();
         }
      };
      actionAddCategory.setText(i18n.tr("Add category"));
      actionAddCategory.setImageDescriptor(SharedIcons.ADD_OBJECT);

      actionEditCategory = new Action() {
         @Override
         public void run()
         {
            editCategory();
         }
      };
      actionEditCategory.setText(i18n.tr("Edit category"));
      actionEditCategory.setImageDescriptor(SharedIcons.EDIT);
      actionEditCategory.setEnabled(false);

      actionDeleteCategory = new Action() {
         @Override
         public void run()
         {
            deleteCategory();
         }
      };
      actionDeleteCategory.setText(i18n.tr("Delete category"));
      actionDeleteCategory.setImageDescriptor(SharedIcons.DELETE_OBJECT);
      actionDeleteCategory.setEnabled(false);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
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
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      manager.add(actionEditCategory);
      manager.add(actionDeleteCategory);
      manager.add(actionAddCategory);
   }

   /**
    * Create new category
    */
   public void createCategory()
   {
      showDCIPropertyPages(new AlarmCategory());
   }

   /**
    * Edit alarm category
    */
   public void editCategory()
   {
      if (selection.isEmpty())
         return;

      showDCIPropertyPages((AlarmCategory)selection.getFirstElement());
   }

   /**
    * Show Alarm category properties dialog
    * 
    * @param alarmCategory Alarm category to edit
    * @return true if OK was pressed
    */
   protected boolean showDCIPropertyPages(AlarmCategory alarmCategory)
   {
      AlarmCategoryEditor editor = new AlarmCategoryEditor(alarmCategory);
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("general", new General(editor)));    
      pm.addToRoot(new PreferenceNode("accessControl", new AccessControl(editor)));

      PreferenceDialog dlg = new PreferenceDialog(viewPart.getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(String.format(i18n.tr("Properties for %s"), alarmCategory.getName()));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }

   /**
    * Delete alarm category
    */
   private void deleteCategory()
   {
      final IStructuredSelection selection = getSelection();
      if (selection.isEmpty())
         return;

      final String message = (selection.size() == 1) ? i18n.tr("Do you really wish to delete the selected category?")
            : i18n.tr("Do you really wish to delete the selected categories?");
      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Confirm category deletion"), message))
         return;

      new Job(i18n.tr("Delete alarm category database entries"), viewPart) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
               session.deleteAlarmCategory(((AlarmCategory)o).getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete alarm category database entry");
         }
      }.start();
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
   public IStructuredSelection getSelection()
   {
      return viewer.getStructuredSelection();
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
   
   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
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

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
   }

   public AbstractViewerFilter getFilter()
   {
      return filter;
   }
}
