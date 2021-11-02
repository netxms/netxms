/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2021 RadenSolutions
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
package org.netxms.nxmc.base.widgets.helpers;

import org.eclipse.swt.internal.SWTEventListener;

/**
 * Classes which implement this interface provide a method that can provide the style information for a line that is to be drawn.
 */
public interface LineStyleListener extends SWTEventListener
{

   /**
    * This method is called when a line is about to be drawn in order to get the line's style information.
    *
    * @param event the given event
    */
   public void lineGetStyle(LineStyleEvent e);
}
