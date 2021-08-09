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
 * Interface for auto bind dci
 */
public interface AutoBindDCIObject
{   
   final static int DCI_BIND_FLAG = 4;
   final static int DCI_UNBIND_FLAG = 8;
   
   /**
    * @return true if automatic bind is enabled
    */
   public boolean isDciAutoBindEnabled();

   /**
    * @return true if automatic unbind is enabled
    */
   public boolean isDciAutoUnbindEnabled();

   /**
    * @return Filter script for automatic bind
    */
   public String getDciAutoBindFilter();
}
