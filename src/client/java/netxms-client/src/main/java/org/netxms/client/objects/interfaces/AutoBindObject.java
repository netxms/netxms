/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.client.objects.interfaces;

/**
 * Interface for auto bind objects
 */
public interface AutoBindObject
{   
   final static int OBJECT_BIND_FLAG = 1;
   final static int OBJECT_UNBIND_FLAG = 2;
   
   /**
    * @return true if automatic bind is enabled
    */
   public boolean isAutoBindEnabled();

   /**
    * @return true if automatic unbind is enabled
    */
   public boolean isAutoUnbindEnabled();

   /**
    * @return Filter script for automatic bind
    */
   public String getAutoBindFilter();
   

   /**
    * @return auto bind flags
    */
   public int getAutoBindFlags();
}
