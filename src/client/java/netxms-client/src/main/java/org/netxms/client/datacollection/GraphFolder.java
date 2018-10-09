/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.client.datacollection;

import java.util.HashSet;
import java.util.Set;

/**
 * Virtual folder object for predefined graphs
 */
public class GraphFolder
{
	private String name;
	private GraphFolder parent;
	private Set<GraphFolder> subfolders;
	private Set<GraphSettings> graphs;
	
	/**
	 * @param name
	 * @param parent
	 */
	public GraphFolder(String name, GraphFolder parent)
	{
		this.name = name;
		this.parent = parent;
		subfolders = new HashSet<GraphFolder>();
		graphs = new HashSet<GraphSettings>();
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
		for(GraphFolder f : subfolders)
		   objects[index++] = f;
      for(GraphSettings s : graphs)
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
	public void addGraph(GraphSettings g)
	{
	   g.setParent(this);
		graphs.add(g);
	}
	
	/**
	 * Add sub-folder
	 * 
	 * @param f sub-folder
	 */
	public void addFolder(GraphFolder f)
	{
	   f.setParent(this);
		subfolders.add(f);
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
