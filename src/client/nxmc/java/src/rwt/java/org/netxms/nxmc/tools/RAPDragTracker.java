/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.netxms.nxmc.base.jobs.Job;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Drag tracker - copied from draw2d
 */
public class RAPDragTracker
{
   private static final Logger logger = LoggerFactory.getLogger(RAPDragTracker.class);
   
   public boolean cancelled;
   public boolean tracking;
   private final MouseMoveListener listener;
   private final Control control;

   /**
    * Create drag tracker for given control.
    *
    * @param listener mouse move lestener
    * @param control control to track mouse for
    */
   public RAPDragTracker(final MouseMoveListener listener, final Control control)
   {
      this.listener = listener;
      this.control = control;
   }

   /**
    * Start drag tracker
    */
   public void start()
   {
      Job dragJob = new Job("Drag-Job", null) {
         protected void run(IProgressMonitor monitor)
         {
            // Run tracker until mouse up occurs or escape key pressed.
            final Display display = control.getDisplay();
            cancelled = false;
            tracking = true;

            try
            {
               long timeout = 0;
               long refreshRate = 200;
               while(tracking && !cancelled)
               {
                  if (!display.isDisposed())
                  {
                     display.syncExec(new Runnable() {
                        public void run()
                        {
                           if (control.isDisposed())
                           {
                              tracking = false;
                              cancelled = true;
                              return;
                           }
                           Event ev = new Event();
                           ev.display = display;
                           Point loc = control.toControl(display.getCursorLocation());
                           ev.type = SWT.DragDetect;
                           ev.widget = control;
                           ev.button = 1;
                           ev.x = loc.x;
                           ev.y = loc.y;
                           MouseEvent me = new MouseEvent(ev);
                           me.stateMask = SWT.BUTTON1;
                           listener.mouseMove(me);
                        }
                     });
                     timeout += refreshRate;
                     if (timeout >= 60000)
                     {
                        cancelled = true;
                     }
                     Thread.sleep(refreshRate);
                  }

               }
            }
            catch(InterruptedException e)
            {
               logger.error("Error in RAP drag tracker", e);
            }
            finally
            {
               display.syncExec(new Runnable() {
                  public void run()
                  {
                     stop();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "";
         }
      };
      dragJob.setSystem(true);
      dragJob.start();
   }

   /**
    * Stop drag tracker
    */
   public void stop()
   {
      tracking = false;
   }
}
