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
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.viewers.ColumnViewer;

/**
 * Callback for user database object synchronization completion
 */
public class UserRefreshRunnable implements Runnable
{
   private ColumnViewer viewer;
   private Object element;
   
   /**
    * Constructor
    */
   public UserRefreshRunnable(ColumnViewer viewer, Object element)
   {
      this.viewer = viewer;
      this.element = element;
   }

   /**
    * @see java.lang.Runnable#run()
    */
   @Override
   public void run()
   {
      viewer.getControl().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            viewer.update(element, null);
         }
      });
   }
}
