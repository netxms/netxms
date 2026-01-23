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
package org.netxms.nxmc.modules.agentmanagement.widgets;

import java.util.List;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Tree;
import org.netxms.client.AgentList;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;
import org.netxms.nxmc.modules.agentmanagement.views.helpers.AgentDataTreeNode;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Agent data tree browser widget
 */
public class AgentDataTree extends Composite
{
   private TreeViewer treeViewer;
   private AgentDataTreeNode[] rootNodes;

   /**
    * Create new agent data tree browser.
    *
    * @param parent parent composite
    * @param style SWT style
    * @param view parent view
    */
   public AgentDataTree(Composite parent, int style)
   {
      super(parent, style);

      setLayout(new FillLayout());

      treeViewer = new TreeViewer(this, SWT.NONE);
      treeViewer.setContentProvider(new AgentDataTreeContentProvider());
      treeViewer.setLabelProvider(new AgentDataTreeLabelProvider());
      treeViewer.setComparator(new AgentDataTreeComparator());
   }

   /**
    * Set tree input data.
    *
    * @param parameters list of parameters
    * @param lists list of lists
    * @param tables list of tables
    */
   public void setInput(List<AgentParameter> parameters, List<AgentList> lists, List<AgentTable> tables)
   {
      if (parameters == null && lists == null && tables == null)
      {
         rootNodes = null;
         treeViewer.setInput(null);
         return;
      }

      rootNodes = AgentDataTreeNode.buildTree(parameters, lists, tables);
      treeViewer.setInput(rootNodes);
      treeViewer.expandToLevel(2);
   }

   /**
    * Get currently selected node.
    *
    * @return currently selected node or null if no selection
    */
   public AgentDataTreeNode getSelection()
   {
      IStructuredSelection selection = treeViewer.getStructuredSelection();
      return (AgentDataTreeNode)selection.getFirstElement();
   }

   /**
    * Add selection changed listener.
    *
    * @param listener listener to add
    */
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      treeViewer.addSelectionChangedListener(listener);
   }

   /**
    * Get tree control.
    *
    * @return tree control
    */
   public Tree getTreeControl()
   {
      return treeViewer.getTree();
   }

   /**
    * Get tree viewer.
    *
    * @return tree viewer
    */
   public TreeViewer getTreeViewer()
   {
      return treeViewer;
   }

   /**
    * Select node by full name.
    *
    * @param name full metric name
    */
   public void selectByName(String name)
   {
      if (rootNodes == null)
         return;

      for(AgentDataTreeNode root : rootNodes)
      {
         AgentDataTreeNode node = findNodeByName(root, name);
         if (node != null)
         {
            treeViewer.setSelection(new StructuredSelection(node), true);
            return;
         }
      }
   }

   /**
    * Find node by full name recursively.
    *
    * @param node current node
    * @param name name to find
    * @return found node or null
    */
   private AgentDataTreeNode findNodeByName(AgentDataTreeNode node, String name)
   {
      if (name.equals(node.getFullName()))
         return node;

      for(AgentDataTreeNode child : node.getChildren())
      {
         AgentDataTreeNode found = findNodeByName(child, name);
         if (found != null)
            return found;
      }
      return null;
   }

   /**
    * Content provider for agent data tree
    */
   private class AgentDataTreeContentProvider implements ITreeContentProvider
   {
      @Override
      public Object[] getElements(Object inputElement)
      {
         return (AgentDataTreeNode[])inputElement;
      }

      @Override
      public Object[] getChildren(Object parentElement)
      {
         return ((AgentDataTreeNode)parentElement).getChildren().toArray();
      }

      @Override
      public Object getParent(Object element)
      {
         return ((AgentDataTreeNode)element).getParent();
      }

      @Override
      public boolean hasChildren(Object element)
      {
         return ((AgentDataTreeNode)element).hasChildren();
      }

      @Override
      public void dispose()
      {
      }

      @Override
      public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
      {
      }
   }

   /**
    * Label provider for agent data tree
    */
   private class AgentDataTreeLabelProvider extends LabelProvider
   {
      private Image imageParameter;
      private Image imageList;
      private Image imageTable;
      private Image imageCategory;

      public AgentDataTreeLabelProvider()
      {
         imageParameter = ResourceManager.getImage("icons/agent-explorer/parameter.png");
         imageList = ResourceManager.getImage("icons/agent-explorer/list.png");
         imageTable = ResourceManager.getImage("icons/agent-explorer/table.png");
         imageCategory = ResourceManager.getImage("icons/agent-explorer/category.png");
      }

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
       */
      @Override
      public String getText(Object element)
      {
         return ((AgentDataTreeNode)element).getDisplayName();
      }

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
       */
      @Override
      public Image getImage(Object element)
      {
         AgentDataTreeNode node = (AgentDataTreeNode)element;
         switch(node.getType())
         {
            case PARAMETER:
               return imageParameter;
            case LIST:
               return imageList;
            case TABLE:
               return imageTable;
            case ROOT:
            case CATEGORY:
            default:
               return imageCategory;
         }
      }
   }

   /**
    * Comparator for agent data tree
    */
   private class AgentDataTreeComparator extends ViewerComparator
   {
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         return ((AgentDataTreeNode)e1).compareTo((AgentDataTreeNode)e2);
      }
   }
}
