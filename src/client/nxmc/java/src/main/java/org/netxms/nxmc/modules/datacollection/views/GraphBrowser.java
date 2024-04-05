/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.datacollection.GraphFolder;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.NavigationView;
import org.netxms.nxmc.base.views.helpers.GraphTreeContentProvider;
import org.netxms.nxmc.base.views.helpers.GraphTreeFilter;
import org.netxms.nxmc.base.views.helpers.GraphTreeLabelProvider;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

public final class GraphBrowser extends NavigationView implements SessionListener
{
   final I18n i18n = LocalizationHelper.getI18n(GraphBrowser.class);
   
   private TreeViewer viewer;
   private NXCSession session;
   private GraphFolder root;
   private Action actionProperties; 
   private Action actionDelete;

   public GraphBrowser()
   {
      super(LocalizationHelper.getI18n(GraphBrowser.class).tr("Graphs"), null, "Graphs", true, false, false);
      session = Registry.getSession();
   }

   @Override
   protected void createContent(Composite parent)
   {
      viewer = new TreeViewer(parent, SWT.NONE);
      viewer.setContentProvider(new GraphTreeContentProvider());
      viewer.setLabelProvider(new GraphTreeLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return (e1 instanceof GraphFolder) ?
                  ((e2 instanceof GraphFolder) ? ((GraphFolder)e1).getDisplayName().compareToIgnoreCase(((GraphFolder)e2).getDisplayName()) : -1) :
                  ((e2 instanceof GraphDefinition) ? ((GraphDefinition)e1).getDisplayName().compareToIgnoreCase(((GraphDefinition)e2).getDisplayName()) : 1);
         }
      });
      GraphTreeFilter filter = new GraphTreeFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {

            Object selection = viewer.getStructuredSelection().getFirstElement();
            if (selection instanceof GraphFolder)
            {
               if (viewer.getExpandedState(selection))
                  viewer.collapseToLevel(selection, TreeViewer.ALL_LEVELS);
               else
                  viewer.expandToLevel(selection, 1);
            }
         }
      });
      
      session.addListener(this);
      createActions();
      createPopupMenu();
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {      
      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deletePredefinedGraph();
         }
      };

      actionProperties = new Action(i18n.tr("Properties")) {
         @Override
         public void run()
         {
            editPredefinedGraph();
         }
      };
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
    * Create pop-up menu for user list
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

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.getFirstElement() instanceof GraphFolder)
         return;
      
      mgr.add(actionDelete);
      if (selection.size() == 1)
      {
         mgr.add(new Separator());
         mgr.add(actionProperties);
      }
   }
   
   /**
    * Edit predefined graph
    */
   private void editPredefinedGraph()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;
      
      GraphDefinition settings = (GraphDefinition)selection.getFirstElement();
      if (HistoricalGraphView.showGraphPropertyPages(settings, getWindow().getShell()))
      {
         final GraphDefinition newSettings = settings;
         new Job(i18n.tr("Update predefined graph"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.saveGraph(newSettings, false);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.update(newSettings, null);
                  }
               });
            }
            
            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot update predefined graph");
            }
         }.start();
      }
   }
   
   /**
    * Delete predefined graph(s)
    */
   private void deletePredefinedGraph()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 0)
         return;
      
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Predefined Graphs"), 
            i18n.tr("Selected predefined graphs will be deleted. Are you sure?")))
         return;
      
      for(final Object o : selection.toList())
      {
         if (!(o instanceof GraphDefinition))
            continue;
         
         new Job(String.format(i18n.tr("Delete predefined graph \"%s\""), ((GraphDefinition)o).getShortName()), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.deletePredefinedGraph(((GraphDefinition)o).getId());
            }
            
            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot delete predefined graph");
            }
         }.start();
      }
   }

   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
   }

   @Override
   public void setSelection(Object selection)
   {
      if (selection != null)
      {
         viewer.setSelection(new StructuredSelection(selection), true);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Load list of predefined graphs"), null) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of predefined graphs");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final GraphFolder graphs = session.getPredefinedGraphsAsTree();
            runInUIThread(new Runnable() {

               @Override
               public void run()
               {
                  root = graphs;
                  viewer.setInput(root);
               }
            });
         }
      }.start();
   }

   /* (non-Javadoc)
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.PREDEFINED_GRAPHS_DELETED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (root.removeGraph(n.getSubCode()))
                     viewer.refresh();
               }
            });
            break;
         case SessionNotification.PREDEFINED_GRAPHS_CHANGED:            
            if (((GraphDefinition)n.getObject()).isTemplate())
               return;
            
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  final IStructuredSelection selection = viewer.getStructuredSelection();  
                  
                  root.updateGraph((GraphDefinition)n.getObject());
                  viewer.refresh();
                  
                  if ((selection.size() == 1) && (selection.getFirstElement() instanceof GraphDefinition))
                  {
                     GraphDefinition element = (GraphDefinition)selection.getFirstElement();
                     if (element.getId() == n.getSubCode())
                        viewer.setSelection(new StructuredSelection((GraphDefinition)n.getObject()), true);
                  }
               }
            });
            break;
      }
   }
}