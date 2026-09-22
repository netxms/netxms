/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.tools;

import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.widgets.Canvas;

/**
 * Helper for attaching mouse drag tracking (mouse down / move / up) to a canvas in a toolkit-independent way.
 */
public final class DragTrackingHelper
{
   /**
    * Attach mouse listeners so that <code>mouseMove</code> is called while mouse button is pressed and moved over the canvas.
    *
    * @param control canvas
    * @param mouseListener listener for button press and release
    * @param moveListener listener for mouse movement
    */
   public static void attach(Canvas control, MouseListener mouseListener, MouseMoveListener moveListener)
   {
      control.addMouseListener(mouseListener);
      control.addMouseMoveListener(moveListener);
   }
}
