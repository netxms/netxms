/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.client.objecttools;

import org.netxms.client.ObjectMenuFilter;
import org.netxms.client.objects.AbstractNode;

/**
 * Generic interface for user-defined object actions (object tools, graph templates, etc.)
 */
public interface ObjectAction
{
   /**
    * Get menu filter associated with the tool
    */
   public ObjectMenuFilter getMenuFilter();

   /**
    * Sets menu filter for the tool
    */
   public void setMenuFilter(ObjectMenuFilter filter);
   
   /**
    * Get tool type
    */
   public int getToolType();

   /**
    * Check if this action is applicable to given node
    * 
    * @param node node object
    * @return true if applicable
    */
   public boolean isApplicableForNode(AbstractNode node);
}
