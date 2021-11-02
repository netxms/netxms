/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.base.views;

/**
 * Listener for view state changes.
 */
public interface ViewStateListener
{
   /**
    * Called when view is activated.
    *
    * @param view view that was activated
    */
   public void viewActivated(View view);

   /**
    * Called when view is deactivated.
    *
    * @param view view that was deactivated
    */
   public void viewDeactivated(View view);

   /**
    * Called when view is closed.
    *
    * @param view view that was closed
    */
   public void viewClosed(View view);
}
