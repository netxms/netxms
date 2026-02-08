/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.InputField;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.InputFieldEntryDialog;
import org.netxms.nxmc.modules.objects.dialogs.ObjectQueryEditDialog;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectQueryComparator;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectQueryFilter;
import org.netxms.nxmc.modules.objects.views.helpers.ObjectQueryLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.RefreshTimer;
import org.xnap.commons.i18n.I18n;

/**
 * Predefined object query manager
 */
public class ObjectQueryManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectQueryManager.class);

   public static final int COL_ID = 0;
   public static final int COL_NAME = 1;
   public static final int COL_DESCRIPTION = 2;
   public static final int COL_IS_VALID = 3;

   private NXCSession session;
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private RefreshTimer refreshTimer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionExecute;

   /**
    * Create object category manager view
    */
   public ObjectQueryManager()
   {
      super(LocalizationHelper.getI18n(ObjectQueryManager.class).tr("Object Queries"), ResourceManager.getImageDescriptor("icons/config-views/object-queries.png"), "object-query.manager", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] names = new String[] { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Description"), i18n.tr("Valid") };
      final int[] widths = new int[] { 120, 300, 600, 60 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectQueryLabelProvider());
      viewer.setComparator(new ObjectQueryComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editQuery();
         }
      });
      ObjectQueryFilter filter = new ObjectQueryFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createActions();
      createContextMenu();

      refreshTimer = new RefreshTimer(500, viewer.getControl(), new Runnable() {
         @Override
         public void run()
         {
            refresh();
         }
      });

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.OBJECT_QUERY_UPDATED) || (n.getCode() == SessionNotification.OBJECT_QUERY_DELETED))
               refreshTimer.execute();
         }
      };
      session.addListener(sessionListener);
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
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createQuery();
         }
      };
      addKeyBinding("M1+N", actionCreate);

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editQuery();
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteQueries();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionExecute = new Action("E&xecute", SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeQuery();
         }
      };
      addKeyBinding("F9", actionExecute);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            manager.add(actionCreate);
            if (selection.size() == 1)
               manager.add(actionEdit);
            if (!selection.isEmpty())
               manager.add(actionDelete);
            if (selection.size() == 1)
            {
               manager.add(new Separator());
               manager.add(actionExecute);
            }
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(viewer.getTable());
      viewer.getTable().setMenu(menu);
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
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading predefined object queries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ObjectQuery> queries = session.getObjectQueries();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  IStructuredSelection selection = viewer.getStructuredSelection();
                  viewer.setInput(queries);
                  if (!selection.isEmpty())
                  {
                     List<ObjectQuery> newSelection = new ArrayList<>();
                     for(Object q : selection.toList())
                     {
                        int id = ((ObjectQuery)q).getId();
                        for(ObjectQuery nq : queries)
                        {
                           if (nq.getId() == id)
                           {
                              newSelection.add(nq);
                              break;
                           }
                        }
                     }
                     if (!newSelection.isEmpty())
                        viewer.setSelection(new StructuredSelection(newSelection));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load predefined object queries");
         }
      }.start();
   }

   /**
    * Create new query
    */
   private void createQuery()
   {
      ObjectQueryEditDialog dlg = new ObjectQueryEditDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final ObjectQuery query = dlg.getQuery();
      new Job(i18n.tr("Creating object query"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObjectQuery(query);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create object query");
         }
      }.start();
   }

   /**
    * Edit selected query
    */
   private void editQuery()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ObjectQuery query = (ObjectQuery)selection.getFirstElement();
      ObjectQueryEditDialog dlg = new ObjectQueryEditDialog(getWindow().getShell(), query);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Updating object query"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObjectQuery(query);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update object query");
         }
      }.start();
   }

   /**
    * Delete selected queries
    */
   private void deleteQueries()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete"), i18n.tr("Selected object queries will be permanently deleted. Are you sure?")))
         return;

      final List<Integer> idList = new ArrayList<>(selection.size());
      for(Object o : selection.toList())
         idList.add(((ObjectQuery)o).getId());

      new Job(i18n.tr("Deleting object queries"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Integer id : idList)
               session.deleteObjectQuery(id);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete object query");
         }
      }.start();
   }

   /**
    * Execute selected query
    */
   private void executeQuery()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ObjectQuery query = (ObjectQuery)selection.getFirstElement();

      final Map<String, String> inputValues;
      final List<InputField> fields = query.getInputFields();
      if (!fields.isEmpty())
      {
         fields.sort(new Comparator<InputField>() {
            @Override
            public int compare(InputField f1, InputField f2)
            {
               return f1.getSequence() - f2.getSequence();
            }
         });
         inputValues = InputFieldEntryDialog.readInputFields(getWindow().getShell(), query.getName(), fields.toArray(new InputField[fields.size()]));
         if (inputValues == null)
            return; // cancelled
      }
      else
      {
         inputValues = new HashMap<String, String>(0);
      }

      actionExecute.setEnabled(false);

      new Job(i18n.tr("Executing object query"), this) {
         static int progress = 0;

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), 100);
            final List<ObjectQueryResult> resultSet = session.queryObjectDetails(query.getSource(), 0, null, null, inputValues, 0, true, 0, (p) -> {
               monitor.worked(p - progress);
               progress = p;
            }, null);
            monitor.done();
            runInUIThread(() -> openView(new ObjectQueryResultView(query.getName(), resultSet)));
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               actionExecute.setEnabled(true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot execute object query");
         }
      }.start();
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
