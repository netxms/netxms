/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.datacollection.GraphSettings;

/**
 * Virtual folder object for predefined graphs
 *
 */
public class GraphFolder
{
	private String name;
	private GraphFolder parent;
	private List<GraphFolder> subfolders;
	private List<GraphSettings> graphs;
	
	/**
	 * @param name
	 * @param parent
	 */
	public GraphFolder(String name, GraphFolder parent)
	{
		this.name = name;
		this.parent = parent;
		subfolders = new ArrayList<GraphFolder>();
		graphs = new ArrayList<GraphSettings>();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the parent
	 */
	public GraphFolder getParent()
	{
		return parent;
	}
	
	/**
	 * Get all child objects (subfolders and graphs) as an array
	 * 
	 * @return array of all child objects
	 */
	public Object[] getChildObjects()
	{
		Object[] objects = new Object[subfolders.size() + graphs.size()];
		int index = 0;
		for(int i = 0; i < subfolders.size(); i++)
			objects[index++] = subfolders.get(i);
		for(int i = 0; i < graphs.size(); i++)
			objects[index++] = graphs.get(i);
		return objects;
	}

	/**
	 * Check if folder has child objects
	 * 
	 * @return true if folder has child objects
	 */
	public boolean hasChildren()
	{
		return (subfolders.size() > 0) || (graphs.size() > 0);
	}
	
	/**
	 * Add graph to folder
	 * @param g graph to add
	 */
	public void addGraph(GraphSettings g)
	{
		graphs.add(g);
	}
	
	/**
	 * Add subfolder
	 * @param f subfolder
	 */
	public void addFolder(GraphFolder f)
	{
		subfolders.add(f);
	}
}
