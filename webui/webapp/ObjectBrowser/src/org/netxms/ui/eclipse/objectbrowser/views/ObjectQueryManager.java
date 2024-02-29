/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.views;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.InputField;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.dialogs.InputFieldEntryDialog;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectQueryEditDialog;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectQueryComparator;
import org.netxms.ui.eclipse.objectbrowser.views.helpers.ObjectQueryLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.RefreshTimer;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Predefined object query manager
 */
public class ObjectQueryManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objectbrowser.views.ObjectQueryManager";

   public static final int COL_ID = 0;
   public static final int COL_NAME = 1;
   public static final int COL_DESCRIPTION = 2;
   public static final int COL_IS_VALID = 3;

   private NXCSession session = ConsoleSharedData.getSession();
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private RefreshTimer refreshTimer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionExecute;
   private Action actionRefresh;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      final String[] names = new String[] { "ID", "Name", "Description", "Valid" };
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

      createActions();
      contributeToActionBars();
      createContextMenu();

      refresh();

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
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };

      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createQuery();
         }
      };

      actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editQuery();
         }
      };

      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteQueries();
         }
      };

      actionExecute = new Action("E&xecute", SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeQuery();
         }
      };
   }

   /**
    * Fill action bars
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
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionRefresh);
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
    * Refresh view
    */
   private void refresh()
   {
      new ConsoleJob("Loading predefined object queries", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return "Cannot load predefined object queries";
         }
      }.start();
   }

   /**
    * Create new query
    */
   private void createQuery()
   {
      ObjectQueryEditDialog dlg = new ObjectQueryEditDialog(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      final ObjectQuery query = dlg.getQuery();
      new ConsoleJob("Creating object query", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObjectQuery(query);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot create object query";
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
      ObjectQueryEditDialog dlg = new ObjectQueryEditDialog(getSite().getShell(), query);
      if (dlg.open() != Window.OK)
         return;

      new ConsoleJob("Updating object query", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObjectQuery(query);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update object query";
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

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete", "Selected object queries will be permanently deleted. Are you sure?"))
         return;

      final List<Integer> idList = new ArrayList<>(selection.size());
      for(Object o : selection.toList())
         idList.add(((ObjectQuery)o).getId());

      new ConsoleJob("Deleting object queries", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Integer id : idList)
               session.deleteObjectQuery(id);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete object query";
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
         inputValues = InputFieldEntryDialog.readInputFields(query.getName(), fields.toArray(new InputField[fields.size()]));
         if (inputValues == null)
            return; // cancelled
      }
      else
      {
         inputValues = new HashMap<String, String>(0);
      }

      new ConsoleJob("Executing object query", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<ObjectQueryResult> resultSet = session.queryObjectDetails(query.getSource(), 0, null, null, inputValues, 0, true, 0);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  try
                  {
                     ObjectQueryResultView view = (ObjectQueryResultView)getSite().getPage().showView(ObjectQueryResultView.ID, UUID.randomUUID().toString(), IWorkbenchPage.VIEW_ACTIVATE);
                     view.setContent(resultSet, query.getName());
                  }
                  catch(PartInitException e)
                  {
                     MessageDialogHelper.openError(getSite().getShell(), "Error", String.format("Cannot open result view (%s)", e.getLocalizedMessage()));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot execute object query";
         }
      }.start();
   }
}
