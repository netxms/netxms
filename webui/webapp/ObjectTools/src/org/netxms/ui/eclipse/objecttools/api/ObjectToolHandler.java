/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.api;

import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;

/**
 * Interface for "internal" object tools handlers
 */
public interface ObjectToolHandler
{
	/**
	 * Check if given tool can be executed on given node.
	 * 
	 * @param node node object on which tool execution was requested
	 * @param tool object tool to be executed
	 * @return true if tool can be executed and should be shown in menu
	 */
	public abstract boolean canExecuteOnNode(Node node, ObjectTool tool);
	
	/**
	 * Execute object tool. This method called on UI thread.
	 * 
	 * @param node node object on which tool execution was requested
	 * @param tool object tool to be executed
	 */
	public abstract void execute(Node node, ObjectTool tool);
}
