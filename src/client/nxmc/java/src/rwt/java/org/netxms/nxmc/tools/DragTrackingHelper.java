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

import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSource;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.DragSourceListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.ui.presentations.PresentationUtil;

/**
 * Helper for attaching mouse drag tracking (mouse down / move / up) to a canvas in a toolkit-independent way. RAP does not deliver
 * mouse move events, so drag is emulated: drag start is reported as mouse down and cursor position is polled by drag tracker
 * while drag operation is in progress (same approach as used by draw2d port for RAP).
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

      PresentationUtil.addDragListener(control, new Listener() {
         @Override
         public void handleEvent(Event event)
         {
            if (event.type == SWT.DragDetect)
            {
               MouseEvent me = new MouseEvent(event);
               me.stateMask = SWT.BUTTON1;
               mouseListener.mouseDown(me);
            }
         }
      });

      DragSource dragSource = new DragSource(control, DND.DROP_MOVE | DND.DROP_COPY | DND.DROP_LINK);
      dragSource.addDragListener(new DragSourceListener() {
         private RAPDragTracker tracker = null;

         @Override
         public void dragStart(DragSourceEvent event)
         {
            tracker = new RAPDragTracker(moveListener, control);
            tracker.start();
         }

         @Override
         public void dragSetData(DragSourceEvent event)
         {
         }

         @Override
         public void dragFinished(DragSourceEvent event)
         {
            if (tracker != null)
            {
               tracker.stop();
               tracker = null;
            }
         }
      });
   }
}
