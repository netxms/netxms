/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.MutableObjectCategory;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectCategoryEditDialog;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectCategoryComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectCategoryFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectCategoryLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object category manager
 */
public class ObjectCategoryManager extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectCategoryManager.class);

   public static final int COL_ID = 0;
   public static final int COL_NAME = 1;
   public static final int COL_ICON = 2;
   public static final int COL_MAP_IMAGE = 3;

   private static final String TABLE_CONFIG_PREFIX = "ObjectCategoryManager";

   private Map<Integer, ObjectCategory> categories = new HashMap<Integer, ObjectCategory>();
   private SortableTableViewer viewer;
   private NXCSession session;
   private ObjectCategoryFilter filter;
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Create object category manager view
    */
   public ObjectCategoryManager()
   {
      super(LocalizationHelper.getI18n(ObjectCategoryManager.class).tr("Object Categories"), ResourceManager.getImageDescriptor("icons/config-views/object-categories.png"), "objects.categories", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Icon"), i18n.tr("Map image") };
      final int[] widths = { 100, 500, 200, 200 };
      viewer = new SortableTableViewer(parent, names, widths, 1, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectCategoryLabelProvider());
      viewer.setComparator(new ObjectCategoryComparator());
      filter = new ObjectCategoryFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editCategory();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });

      createActions();
      createContextMenu();

      session.addListener(this);

      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(this);
      super.dispose();
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      if (n.getCode() == SessionNotification.OBJECT_CATEGORY_UPDATED)
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               ObjectCategory category = (ObjectCategory)n.getObject();
               categories.put(category.getId(), category);
               viewer.refresh(true);
            }
         });
      }
      else if (n.getCode() == SessionNotification.OBJECT_CATEGORY_DELETED)
      {
         getDisplay().asyncExec(new Runnable() {
            @Override
            public void run()
            {
               categories.remove((int)n.getSubCode());
               viewer.refresh();
            }
         });
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createCategory();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editCategory();
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteCategories();
         }
      };
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionEdit);
      manager.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param manager Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionEdit);
      manager.add(actionDelete);
   }

   /**
    * Create new category
    */
   private void createCategory()
   {
      ObjectCategoryEditDialog dlg = new ObjectCategoryEditDialog(getWindow().getShell(), null);
      if (dlg.open() == Window.OK)
      {
         updateCategory(dlg.getCategory());
      }
   }

   /**
    * Edit selected category
    */
   private void editCategory()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      ObjectCategoryEditDialog dlg = new ObjectCategoryEditDialog(getWindow().getShell(),
            new MutableObjectCategory((ObjectCategory)selection.getFirstElement()));
      if (dlg.open() == Window.OK)
      {
         updateCategory(dlg.getCategory());
      }
   }

   /**
    * Update category on server
    *
    * @param category category object
    */
   private void updateCategory(final MutableObjectCategory category)
   {
      new Job(i18n.tr("Updating object category"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final int id = session.modifyObjectCategory(category);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  ObjectCategory newCategory = categories.get(id);
                  if (newCategory == null)
                  {
                     newCategory = new ObjectCategory(category);
                     categories.put(id, newCategory);
                  }
                  viewer.refresh();
                  viewer.setSelection(new StructuredSelection(newCategory));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update object category");
         }
      }.start();
   }

   /**
    * Delete selected categories
    */
   private void deleteCategories()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Object Categories"), i18n.tr("Selected object categories will be deleted. Are you sure?")))
         return;

      final int[] idList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         idList[index++] = ((ObjectCategory)o).getId();

      new Job(i18n.tr("Deleting object categories"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), idList.length);
            for(final int id : idList)
            {
               try
               {
                  session.deleteObjectCategory(id, false);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() != RCC.CATEGORY_IN_USE)
                     throw e;

                  final boolean[] retry = new boolean[1];
                  getDisplay().syncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        ObjectCategory c = session.getObjectCategory(id);
                        retry[0] = MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"),
                              String.format(i18n.tr("Object category \"%s\" is in use. Are you sure you want to delete it?"), c.getName()));
                     }
                  });
                  if (retry[0])
                  {
                     session.deleteObjectCategory(id, true);
                  }
               }
               monitor.worked(1);
            }
            monitor.done();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete object category");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      categories.clear();
      for(ObjectCategory c : session.getObjectCategories())
         categories.put(c.getId(), c);
      viewer.setInput(categories.values());
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
