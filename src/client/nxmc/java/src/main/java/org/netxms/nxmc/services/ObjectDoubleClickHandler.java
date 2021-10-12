/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.View;

/**
 * Interface for object double click handler
 */
public interface ObjectDoubleClickHandler
{
   /**
    * Get description of this handler.
    *
    * @return description of this handler
    */
   public String getDescription();

   /**
    * Get priority of this handler.
    *
    * @return priority of this handler
    */
   public int getPriority();

   /**
    * Get class this handler is enabled for.
    *
    * @return class this handler is enabled for or null
    */
   public Class<?> enabledFor();

	/**
    * Called by framework after double click on object.
    * 
    * @param object object to process
    * @param view current view (can be null)
    * @return true if event is processed by handler
    */
   boolean onDoubleClick(AbstractObject object, View view);
}
