/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.AgentList;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.AgentDataTreeNode;
import org.netxms.nxmc.modules.agentmanagement.widgets.AgentDataTree;
import org.netxms.nxmc.modules.agentmanagement.widgets.AgentItemDetails;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Agent data explorer - allows browsing agent parameters, lists, and tables
 */
public class AgentExplorer extends AdHocObjectView
{
   private I18n i18n = LocalizationHelper.getI18n(AgentExplorer.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_VALUE = 2;
   public static final int COLUMN_STATUS = 3;

   private AgentDataTree agentDataTree;
   private AgentItemDetails details;
   private Action actionQuery;
   private boolean toolView;

   /**
    * Create Agent Explorer for given node.
    *
    * @param objectId node object ID
    * @param context context for the view
    * @param toolView true if this is a tool view
    */
   public AgentExplorer(long objectId, long context, boolean toolView)
   {
      super(LocalizationHelper.getI18n(AgentExplorer.class).tr("Agent Explorer"), ResourceManager.getImageDescriptor("icons/object-views/agent-explorer.png"),
            toolView ? "tools.agent-explorer" : "objects.agent-explorer", objectId, context, false);
      this.toolView = toolView;
   }

   /**
    * Default constructor for restore.
    */
   protected AgentExplorer()
   {
      this(0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.AdHocObjectView#cloneView()
    */
   @Override
   public View cloneView()
   {
      AgentExplorer view = (AgentExplorer)super.cloneView();
      view.toolView = toolView;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      if (toolView)
      {
         Group selectorArea = new Group(parent, SWT.NONE);
         selectorArea.setText(i18n.tr("Context"));
         selectorArea.setLayout(new GridLayout());
         selectorArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

         ObjectSelector objectSelector = new ObjectSelector(selectorArea, SWT.NONE, new SelectorConfigurator().setShowClearButton(true).setShowLabel(false));
         objectSelector.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
         objectSelector.setObjectClass(Node.class);
         objectSelector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
         objectSelector.addModifyListener((e) -> {
            AbstractObject object = objectSelector.getObject();
            setObjectId((object != null) ? object.getObjectId() : 0);
            setContext(object);
            refreshTree();
         });
      }

      SashForm splitter = new SashForm(parent, SWT.HORIZONTAL);
      splitter.setLayout(new FillLayout());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.verticalIndent = toolView ? 3 : 0;
      splitter.setLayoutData(gd);

      agentDataTree = new AgentDataTree(splitter, SWT.BORDER, this);
      agentDataTree.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            AgentDataTreeNode node = agentDataTree.getSelection();
            details.setNode(node);
            actionQuery.setEnabled((node != null) && node.isLeaf());
         }
      });

      details = new AgentItemDetails(splitter, SWT.BORDER, this);

      splitter.setWeights(new int[] { 30, 70 });

      createActions();
      createTreeContextMenu();

      if (getObjectId() != 0)
         refreshTree();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      refreshTree();
   }

   /**
    * Refresh tree data from server.
    */
   private void refreshTree()
   {
      if (getObjectId() == 0)
      {
         agentDataTree.setInput(null, null, null);
         return;
      }

      final long nodeId = getObjectId();
      Job job = new Job(i18n.tr("Loading agent data"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            NXCSession session = Registry.getSession();
            final List<AgentParameter> parameters = session.getSupportedParameters(nodeId);
            final List<AgentTable> tables = session.getSupportedTables(nodeId);
            final List<AgentList> lists = session.getSupportedLists(nodeId);
            runInUIThread(() -> {
               agentDataTree.setInput(parameters, lists, tables);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load agent data");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionQuery = new Action(i18n.tr("&Query")) {
         @Override
         public void run()
         {
            queryCurrentNode(null);
         }
      };
      actionQuery.setEnabled(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      super.fillLocalMenu(manager);
   }

   /**
    * Create context menu for tree
    */
   private void createTreeContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillTreeContextMenu(m));

      Menu menu = menuMgr.createContextMenu(agentDataTree.getTreeControl());
      agentDataTree.getTreeControl().setMenu(menu);
   }

   /**
    * Fill tree context menu
    *
    * @param manager menu manager
    */
   protected void fillTreeContextMenu(final IMenuManager manager)
   {
      manager.add(actionQuery);
   }

   /**
    * Query single selected item
    * 
    * @param arguments command arguments
    */
   public void queryCurrentNode(final String arguments)
   {
      if (getObject() == null)
         return;

      final AgentDataTreeNode treeNode = agentDataTree.getSelection();
      if (treeNode == null || !treeNode.isLeaf())
         return;

      final long nodeId = getObjectId();

      Job job = new Job(i18n.tr("Querying agent data"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            String name = treeNode.getName();
            if (name.endsWith("(*)") && (arguments != null))
            {
               name = name.substring(0, name.length() - 2) + arguments + ")";
            }

            NXCSession session = Registry.getSession();
            Object value;
            String status = "OK";
            try
            {
               switch(treeNode.getType())
               {
                  case PARAMETER:
                     value = session.queryMetric(nodeId, DataOrigin.AGENT, name);
                     break;
                  case LIST:
                     value = session.queryList(nodeId, DataOrigin.AGENT, name);
                     break;
                  case TABLE:
                     value = session.queryTable(nodeId, DataOrigin.AGENT, name);
                     break;
                  default:
                     return;
               }
            }
            catch(Exception e)
            {
               value = null;
               status = e.getMessage();
            }

            final Object finalValue = value;
            final String finalStatus = status;
            runInUIThread(() -> {
               details.setQueryResult(finalValue, finalStatus);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot query agent data");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return toolView ? getName() : super.getFullName();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
   }
}
