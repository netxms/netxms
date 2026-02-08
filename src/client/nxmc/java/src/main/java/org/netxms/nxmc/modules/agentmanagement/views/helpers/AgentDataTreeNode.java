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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.AgentList;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;

/**
 * Tree node for agent data browser
 */
public class AgentDataTreeNode implements Comparable<AgentDataTreeNode>
{
   /**
    * Node type enumeration
    */
   public enum NodeType
   {
      ROOT,
      CATEGORY,
      PARAMETER,
      LIST,
      TABLE
   }

   private String name;
   private String fullName;
   private String description;
   private String dataType;
   private NodeType type;
   private AgentDataTreeNode parent;
   private List<AgentDataTreeNode> children;

   /**
    * Create a new tree node.
    *
    * @param name display name
    * @param fullName full metric name
    * @param type node type
    * @param description description
    * @param dataType data type (for parameters)
    */
   public AgentDataTreeNode(String name, String fullName, NodeType type, String description, String dataType)
   {
      this.name = name;
      this.fullName = fullName;
      this.type = type;
      this.description = description;
      this.dataType = dataType;
      this.parent = null;
      this.children = new ArrayList<>();
   }

   /**
    * Create root node.
    *
    * @param name root node name
    * @param type root node type
    */
   public AgentDataTreeNode(String name, NodeType type)
   {
      this(name, null, type, null, null);
   }

   /**
    * Add child node.
    *
    * @param child child node to add
    */
   public void addChild(AgentDataTreeNode child)
   {
      child.parent = this;
      children.add(child);
   }

   /**
    * Get child by name.
    *
    * @param name child name
    * @return child node or null
    */
   public AgentDataTreeNode getChild(String name)
   {
      for(AgentDataTreeNode child : children)
      {
         if (child.name.equals(name))
            return child;
      }
      return null;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return (fullName != null) ? fullName : name;
   }

   /**
    * @return the display name
    */
   public String getDisplayName()
   {
      return name;
   }

   /**
    * @return the full name
    */
   public String getFullName()
   {
      return fullName;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return the data type
    */
   public String getDataType()
   {
      return dataType;
   }

   /**
    * @return the type
    */
   public NodeType getType()
   {
      return type;
   }

   /**
    * @return the parent
    */
   public AgentDataTreeNode getParent()
   {
      return parent;
   }

   /**
    * @return the children
    */
   public List<AgentDataTreeNode> getChildren()
   {
      return children;
   }

   /**
    * Check if node has children.
    *
    * @return true if node has children
    */
   public boolean hasChildren()
   {
      return !children.isEmpty();
   }

   /**
    * Check if this is a leaf node (parameter, list, or table).
    *
    * @return true if leaf node
    */
   public boolean isLeaf()
   {
      return (type == NodeType.PARAMETER || type == NodeType.LIST || type == NodeType.TABLE);
   }

   /**
    * Collect all leaf nodes under this node.
    *
    * @return list of leaf nodes
    */
   public List<AgentDataTreeNode> collectLeafNodes()
   {
      List<AgentDataTreeNode> leaves = new ArrayList<>();
      collectLeafNodesRecursive(leaves);
      return leaves;
   }

   /**
    * Recursive helper for collecting leaf nodes.
    *
    * @param leaves list to add leaves to
    */
   private void collectLeafNodesRecursive(List<AgentDataTreeNode> leaves)
   {
      if (isLeaf())
      {
         leaves.add(this);
      }
      else
      {
         for(AgentDataTreeNode child : children)
         {
            child.collectLeafNodesRecursive(leaves);
         }
      }
   }

   /**
    * @see java.lang.Comparable#compareTo(java.lang.Object)
    */
   @Override
   public int compareTo(AgentDataTreeNode other)
   {
      // Categories come before leaves
      boolean thisIsCategory = (type == NodeType.CATEGORY || type == NodeType.ROOT);
      boolean otherIsCategory = (other.type == NodeType.CATEGORY || other.type == NodeType.ROOT);
      if (thisIsCategory && !otherIsCategory)
         return -1;
      if (!thisIsCategory && otherIsCategory)
         return 1;
      return name.compareToIgnoreCase(other.name);
   }

   /**
    * Build tree structure from agent data lists.
    *
    * @param parameters list of parameters (may be null)
    * @param lists list of lists (may be null)
    * @param tables list of tables (may be null)
    * @return array of root nodes (Parameters, Lists, Tables)
    */
   public static AgentDataTreeNode[] buildTree(List<AgentParameter> parameters, List<AgentList> lists, List<AgentTable> tables)
   {
      AgentDataTreeNode parametersRoot = new AgentDataTreeNode("Parameters", NodeType.ROOT);
      AgentDataTreeNode listsRoot = new AgentDataTreeNode("Lists", NodeType.ROOT);
      AgentDataTreeNode tablesRoot = new AgentDataTreeNode("Tables", NodeType.ROOT);

      if (parameters != null)
      {
         for(AgentParameter param : parameters)
         {
            addToTree(parametersRoot, param.getName(), param.getDescription(),
                      param.getDataType().toString(), NodeType.PARAMETER);
         }
      }

      if (lists != null)
      {
         for(AgentList list : lists)
         {
            addToTree(listsRoot, list.getName(), list.getDescription(), null, NodeType.LIST);
         }
      }

      if (tables != null)
      {
         for(AgentTable table : tables)
         {
            addToTree(tablesRoot, table.getName(), table.getDescription(), null, NodeType.TABLE);
         }
      }

      // Sort children
      sortChildren(parametersRoot);
      sortChildren(listsRoot);
      sortChildren(tablesRoot);

      return new AgentDataTreeNode[] { parametersRoot, listsRoot, tablesRoot };
   }

   /**
    * Add item to tree with smart grouping.
    *
    * @param root root node to add to
    * @param fullName full metric name
    * @param description description
    * @param dataType data type
    * @param leafType type for leaf node
    */
   private static void addToTree(AgentDataTreeNode root, String fullName, String description,
                                  String dataType, NodeType leafType)
   {
      // Split name on dots
      String[] parts = fullName.split("\\.");

      if (parts.length == 1)
      {
         // No dots - add directly to root
         root.addChild(new AgentDataTreeNode(fullName, fullName, leafType, description, dataType));
         return;
      }

      // Navigate/create path
      AgentDataTreeNode current = root;
      for(int i = 0; i < parts.length - 1; i++)
      {
         String part = parts[i];
         AgentDataTreeNode child = current.getChild(part);
         if (child == null)
         {
            child = new AgentDataTreeNode(part, null, NodeType.CATEGORY, null, null);
            current.addChild(child);
         }
         current = child;
      }

      // Add leaf node
      String leafName = parts[parts.length - 1];
      current.addChild(new AgentDataTreeNode(leafName, fullName, leafType, description, dataType));
   }

   /**
    * Sort children recursively.
    *
    * @param node node to sort
    */
   private static void sortChildren(AgentDataTreeNode node)
   {
      node.children.sort(null);
      for(AgentDataTreeNode child : node.children)
      {
         sortChildren(child);
      }
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return name;
   }
}
