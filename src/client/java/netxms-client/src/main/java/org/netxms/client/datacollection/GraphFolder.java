/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.HashMap;
import java.util.Map;

import org.netxms.base.annotations.Internal;

/**
 * Virtual folder object for predefined graphs
 */
public class GraphFolder
{
   @Internal private GraphFolder parent;

   private String name;
   private String displayName;
   private Map<String, GraphFolder> subfolders;
   private Map<Long, GraphDefinition> graphs;

   /**
    * Create new graph folder
    *
    * @param name folder name
    */
   public GraphFolder(String name)
   {
      this.name = name;
      displayName = name.replace("&", "");
      this.parent = null;
      this.subfolders = new HashMap<String, GraphFolder>();
      this.graphs = new HashMap<Long, GraphDefinition>();
   }

   /**
    * Clear folder (remove all children)
    */
   public void clear()
   {
      subfolders.clear();
      graphs.clear();
   }

   /**
    * Get folder name
    *
    * @return folder name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get display name (name with &amp; shortcut marks removed)
    *
    * @return display name
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * Get parent folder
    *
    * @return parent folder
    */
   public GraphFolder getParent()
   {
      return parent;
   }

   /**
    * Set parent folder
    *
    * @param parent new parent folder
    */
   public void setParent(GraphFolder parent)
   {
      this.parent = parent;
   }

   /**
    * @return true if folder has parent
    */
   public boolean hasParent()
   {
      return (parent == null ? false : true);
   }

   /**
    * Get all child objects (subfolders and graphs) as an array
    *
    * @return array of all child objects
    */
   public Object[] getChildren()
   {
      Object[] objects = new Object[subfolders.size() + graphs.size()];
      int index = 0;
      for(GraphFolder f : subfolders.values())
         objects[index++] = f;
      for(ChartConfiguration s : graphs.values())
         objects[index++] = s;
      return objects;
   }

   /**
    * Check if folder has child objects
    *
    * @return true if folder has child objects
    */
   public boolean hasChildren()
   {
      return !subfolders.isEmpty() || !graphs.isEmpty();
   }

   /**
    * Add graph to folder
    *
    * @param g graph to add
    */
   public void addGraph(GraphDefinition g)
   {
      g.setParent(this);
      graphs.put(g.getId(), g);
   }

   /**
    * Add sub-folder
    *
    * @param f sub-folder
    */
   public void addFolder(GraphFolder f)
   {
      f.setParent(this);
      subfolders.put(f.getDisplayName(), f);
   }

   /**
    * Update graph recursively
    *
    * @param graph graph settings
    * @param path  prepared path
    * @param level current tree level
    */
   private void updateGraphInternal(GraphDefinition graph, String[] path, int level)
   {
      if (level == path.length - 1)
      {
         addGraph(graph);
      }
      else
      {
         GraphFolder f = subfolders.get(path[level].replace("&", ""));
         if (f == null)
         {
            f = new GraphFolder(path[level]);
            addFolder(f);
         }
         f.updateGraphInternal(graph, path, level + 1);
      }
   }

   /**
    * Update graph in the tree
    *
    * @param graph new graph settings
    */
   public void updateGraph(GraphDefinition graph)
   {
      if (parent != null)
         return; // should be called only on root

      removeGraph(graph.getId());

      String[] path = graph.getName().split("->");
      updateGraphInternal(graph, path, 0);
   }

   /**
    * Remove graph from the tree. Will also remove any empty folders.
    *
    * @param id graph settings ID
    * @return true if graph was removed
    */
   public boolean removeGraph(long id)
   {
      if (graphs.remove(id) != null)
         return true;

      if (!subfolders.isEmpty())
      {
         for(GraphFolder f : subfolders.values())
         {
            if (f.removeGraph(id))
            {
               if (!f.hasChildren())
                  subfolders.remove(f.getDisplayName());
               return true;
            }
         }
      }
      return false;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "GraphFolder [name=" + name + ", subfolders=" + subfolders + ", graphs=" + graphs + "]";
   }
}
