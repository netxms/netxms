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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SaveGraphDlg;
import org.netxms.nxmc.modules.datacollection.propertypages.GeneralChart;
import org.netxms.nxmc.modules.datacollection.propertypages.Graph;
import org.netxms.nxmc.modules.datacollection.propertypages.TemplateDataSources;
import org.netxms.nxmc.modules.datacollection.views.helpers.TemplateGraphLabelProvider;
import org.netxms.nxmc.modules.objecttools.propertypages.Filter;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Template graph manager
 */
public class TemplateGraphView extends ConfigurationView implements SessionListener
{
   private final I18n i18n = LocalizationHelper.getI18n(TemplateGraphView.class);
   
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DCI_NAMES = 1;
   public static final int COLUMN_DCI_DESCRIPTIONS = 2;
   public static final int COLUMN_DCI_TAGS = 3;

	private NXCSession session;
	private SortableTableViewer viewer;
	private Action actionEdit;
	private Action actionDelete;
	private Action actionAdd;
   private boolean updateInProgress = false;
   private List<GraphDefinition> graphList;

   /**
    * Create network credentials view
    */
   public TemplateGraphView()
   {
      super(LocalizationHelper.getI18n(TemplateGraphView.class).tr("Template Graphs"), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), "configuration.template-graphs", false);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
	{
      final String[] names = { i18n.tr("Graph name"), i18n.tr("DCI names"), i18n.tr("DCI descriptions"), i18n.tr("DCI tags") };
      final int[] widths = { 150, 400, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new TemplateGraphLabelProvider());
      viewer.addDoubleClickListener((e) -> editTemplateGraph());

		createActions();
		createPopupMenu();

		refresh();
      session.addListener(this);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{		
		actionEdit = new Action("Edit graph") {
			@Override
			public void run()
			{
			   editTemplateGraph();
			}
		};

		actionAdd = new Action("Create new template graph", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            saveGraph("", null, false);
         }
      };

      actionDelete = new Action("Delete template graph") {
         @Override
         public void run()
         {
            deletePredefinedGraph();
         }
      };
	}
	
   /**
    * Save this graph as predefined
    */
   private void saveGraph(String graphName, String errorMessage, final boolean canBeOverwritten)
   {
      SaveGraphDlg dlg = new SaveGraphDlg(getWindow().getShell(), graphName, errorMessage, canBeOverwritten);
      int result = dlg.open();
      if (result == Window.CANCEL)
         return;

      final GraphDefinition gs = new GraphDefinition(0, session.getUserId(), 0, new ArrayList<AccessListElement>(0));
      gs.setName(dlg.getName());
      gs.setFlags(GraphDefinition.GF_TEMPLATE);

      if (result == SaveGraphDlg.OVERRIDE)
      {
         new Job(i18n.tr("Save graph settings"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.saveGraph(gs, canBeOverwritten);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot save graph settings as predefined graph");
            }
         }.start();
      }
      else
      {
         new Job(i18n.tr("Save graph settings"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.saveGraph(gs, canBeOverwritten);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() == RCC.OBJECT_ALREADY_EXISTS)
                  {
                     runInUIThread(() -> saveGraph(gs.getName(), i18n.tr("Graph with given name already exists"), true));
                  }
                  else if (e.getErrorCode() == RCC.ACCESS_DENIED)
                  {
                     runInUIThread(() -> saveGraph(gs.getName(), i18n.tr("Graph with given name already exists and cannot be be overwritten"), false));
                  }
                  else
                  {
                     throw e;
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot save graph settings as predefined graph");
            }
         }.start();
      }
   }

   /**
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.PREDEFINED_GRAPHS_DELETED:
            viewer.getControl().getDisplay().asyncExec(() -> {
               for(int i = 0; i < graphList.size(); i++)
                  if (graphList.get(i).getId() == n.getSubCode())
                  {
                     Object o = graphList.get(i);
                     graphList.remove(o);
                     viewer.setInput(graphList);
                     break;
                  }
            });
            break;
         case SessionNotification.PREDEFINED_GRAPHS_CHANGED:            
            viewer.getControl().getDisplay().asyncExec(() -> {
               if (!(n.getObject() instanceof GraphDefinition))
                  return;

               final IStructuredSelection selection = viewer.getStructuredSelection();

               boolean objectUpdated = false;
               for(int i = 0; i < graphList.size(); i++)
               {
                  if (graphList.get(i).getId() == n.getSubCode())
                  {
                     graphList.set(i, (GraphDefinition)n.getObject());
                     objectUpdated = true;
                     break;
                  }
               }

               if (!objectUpdated)
               {
                  if (((GraphDefinition)n.getObject()).isTemplate())
                     graphList.add((GraphDefinition)n.getObject());
               }
               viewer.setInput(graphList);

               if (selection.size() == 1)
               {
                  if (selection.getFirstElement() instanceof GraphDefinition)
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

   /**
    * Edit template graph
    */
   private void editTemplateGraph()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      GraphDefinition settings = (GraphDefinition)selection.getFirstElement();
      if (showGraphPropertyPages(settings))
      {
         final GraphDefinition newSettings = settings;
         try
         {
            new Job("Update template graph", this) {
               @Override
               protected void run(IProgressMonitor monitor) throws Exception
               {
                  session.saveGraph(newSettings, false);
                  runInUIThread(() -> viewer.update(newSettings, null));
               }

               @Override
               protected String getErrorMessage()
               {
                  return "Cannot update predefined graph";
               }
            }.start();
         }
         catch(Exception e)
         {
            addMessage(MessageArea.ERROR, i18n.tr("Internal error ({0})", e.getLocalizedMessage()));
         }
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

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Predefined Graphs"), i18n.tr("Selected predefined graphs will be deleted. Are you sure?")))
         return;

      for(final Object o : selection.toList())
      {
         if (!(o instanceof GraphDefinition))
            continue;

         new Job(i18n.tr("Deleting predefined graph \"{0}\"", ((GraphDefinition)o).getShortName()), this) {
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

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionAdd);
		manager.add(actionEdit);
      manager.add(actionDelete);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionAdd);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
      manager.add(actionAdd);
      manager.add(actionEdit);
      manager.add(actionDelete);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   public void refresh()
	{
		if (updateInProgress )
			return;

		updateInProgress = true;

      new Job(i18n.tr("Reading predefined graph list"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final List<GraphDefinition> settings = session.getPredefinedGraphs(true);
            runInUIThread(() -> {
               graphList = settings;
               viewer.setInput(graphList.toArray());
               updateInProgress = false;
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Error reading predefined graph list");
			}
		}.start();
	}	

   
   /**
    * Show Graph configuration dialog
    * 
    * @param trap Object tool details object
    * @return true if OK was pressed
    */
   private boolean showGraphPropertyPages(GraphDefinition settings)
   {
      PreferenceManager pm = new PreferenceManager();    
      pm.addToRoot(new PreferenceNode("graph", new Graph(settings, true)));
      pm.addToRoot(new PreferenceNode("general", new GeneralChart(settings, true)));
      pm.addToRoot(new PreferenceNode("filter", new Filter(settings)));
      pm.addToRoot(new PreferenceNode("template", new TemplateDataSources(settings, true)));

      PreferenceDialog dlg = new PreferenceDialog(getWindow().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText("Properties for " + settings.getDisplayName());
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
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
