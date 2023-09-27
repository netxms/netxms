/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.services;

import java.util.Map;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.views.ViewPlacement;

/**
 * Interface for "internal" object tools handlers
 */
public interface ObjectToolHandler
{
   /**
    * Get ID of internal object tool this handler is intended for.
    *
    * @return ID of internal object tool
    */
   public String getId();

	/**
	 * Check if given tool can be executed on given node.
	 * 
	 * @param node node object on which tool execution was requested
	 * @param tool object tool to be executed
	 * @return true if tool can be executed and should be shown in menu
	 */
   public boolean canExecuteOnNode(AbstractNode node, ObjectTool tool);

	/**
    * Execute object tool. This method called on UI thread.
    * 
    * @param node node object on which tool execution was requested
    * @param tool object tool to be executed
    * @param inputValues inut values provided by user
    * @param viewPlacement view placement information
    */
   public void execute(AbstractNode node, ObjectTool tool, Map<String, String> inputValues, ViewPlacement viewPlacement);
}
